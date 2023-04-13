from __future__ import annotations

import configparser
import csv
import datetime
import shutil
import warnings
from dataclasses import dataclass
from pathlib import Path
from typing import Any

import pytest
from fastfec import FastFEC

_THIS_DIR = Path(__file__).parent
CASES_DIR = _THIS_DIR / "cases"


@dataclass
class CaseConfig:
    description: str = ""
    skip: bool = False
    skip_reason: str = ""
    x_fail: bool = False  # The test is currently broken.
    x_fail_reason: str = ""
    x_parse_fail: bool = False  # We expect to fail to parse this filing

    @classmethod
    def from_ini_file(cls, path: Path) -> CaseConfig:
        config = cls()
        if not path.exists():
            return config

        parser = configparser.ConfigParser()
        parser.read(path)
        section = parser["case"]
        keys_present = set(section.keys())
        keys_allowed = set(config.__dataclass_fields__.keys())
        extra = keys_present - keys_allowed
        if extra:
            raise ValueError(
                f"Unexpected keys in {path}: {extra}\n Allowed: {keys_allowed}",
            )
        config.description = section.get("description", config.description)
        config.skip = section.getboolean("skip", config.skip)
        config.skip_reason = section.get("skip_reason", config.skip_reason)
        config.x_fail = section.getboolean("x_fail", config.x_fail)
        config.x_fail_reason = section.get("x_fail_reason", config.x_fail_reason)
        config.x_parse_fail = section.getboolean("x_parse_fail", config.x_parse_fail)
        return config


class Case:
    def __init__(self, path: Path) -> None:
        self.path = path
        self.name = path.name
        self.fec_path = path / "src.fec"
        self.expected_dir = path / "expected"

        config = CaseConfig.from_ini_file(path / "config.ini")
        self.description = config.description
        self.skip = config.skip
        self.skip_reason = config.skip_reason
        self.x_fail = config.x_fail
        self.x_fail_reason = config.x_fail_reason
        self.x_parse_fail = config.x_parse_fail


def case_to_param(case: Case):
    # Add a pytest marker with the name of the case.
    # So to run the "trailing_commas" test, you can do:
    #   pytest -m trailing_commas
    with warnings.catch_warnings():
        warnings.filterwarnings("ignore", category=pytest.PytestUnknownMarkWarning)
        marks = [
            pytest.mark.skipif(case.skip, reason=case.skip_reason),
            pytest.mark.xfail(case.x_fail, reason=case.x_fail_reason),
            getattr(pytest.mark, case.name),
        ]
    return pytest.param(case, id=case.name, marks=marks)


# Filter by is_dir() so we don't pick up on .DS_Store files.
ALL_TEST_CASES = [Case(path) for path in CASES_DIR.iterdir() if path.is_dir()]
ALL_TEST_CASE_PARAMS = [case_to_param(case) for case in ALL_TEST_CASES]


@pytest.fixture(scope="module", params=ALL_TEST_CASE_PARAMS)
def case(request):
    return request.param


@pytest.fixture()
def fastfec():
    with FastFEC() as fastfec:
        yield fastfec


def test_parse_as_files(case: Case, fastfec):
    with open(case.fec_path, "rb") as filing:
        output_dir = case.path / "output"
        try:
            shutil.rmtree(output_dir)
        except FileNotFoundError:
            pass
        code = fastfec.parse_as_files(filing, output_dir)
        if case.x_parse_fail:
            assert code != 1
            return
        assert code == 1
        assert_dir_contents_equal(output_dir, case.expected_dir)


def test_parse(case: Case, fastfec):
    with open(case.fec_path, "rb") as filing:
        lines = fastfec.parse(filing)
        if case.x_parse_fail:
            assert list(lines) == []
            return
        with CsvReaders.from_dir(case.expected_dir) as readers:
            for form, data in lines:
                data = {k: coerce_to_string(v) for k, v in data.items()}
                expected = readers[form].next_record()
                assert data == expected
            for reader in readers.values():
                assert reader.is_empty()


def coerce_to_string(v: Any):
    if isinstance(v, datetime.date):
        return v.strftime("%Y-%m-%d")
    elif isinstance(v, float):
        return f"{v:.2f}"
    else:
        return v


class CsvReader:
    def __init__(self, path: Path) -> None:
        self.file = open(path, newline="")
        self.reader = csv.DictReader(self.file)

    def next_record(self):
        return next(self.reader)

    def is_empty(self) -> bool:
        try:
            self.next_record()
        except StopIteration:
            return True
        return False


class CsvReaders(dict[str, CsvReader]):
    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        for reader in self.values():
            reader.file.close()

    @classmethod
    def from_dir(cls, dir: Path):
        d = {path.stem: CsvReader(path) for path in dir.iterdir()}
        return cls(d)


def assert_dir_contents_equal(dir1: Path, dir2: Path):
    if not dir1.exists() and not dir2.exists():
        return
    dir1_filenames = {p.name for p in dir1.iterdir()}
    dir2_filenames = {p.name for p in dir2.iterdir()}
    assert dir1_filenames == dir2_filenames
    for filename in dir1_filenames:
        assert_file_contents_equal(dir1 / filename, dir2 / filename)


def assert_file_contents_equal(file1: Path, file2: Path):
    # By putting this in its own function, if we hit the assert,
    # then pytest will show the values of file1 and file2,
    # so we can see which files we're actually comparing.
    assert file1.read_text() == file2.read_text()
