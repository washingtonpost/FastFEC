import os

import pytest

_TEST_FOLDER = os.path.dirname(__file__)
FIXTURE_DIR = os.path.join(_TEST_FOLDER, "fixtures")


@pytest.fixture(scope="session")
def get_fixture():
    """
    Fetch locally stored fixture files.
    """

    def _get_fixture(filename, load=False):
        filepath = os.path.join(FIXTURE_DIR, filename)
        if load:
            with open(filepath) as fileobj:
                return fileobj.read()
        else:
            return filepath

    return _get_fixture


@pytest.fixture
def filing_1550126(get_fixture):
    """
    Returns the file path for 1550126.fec
    """
    return get_fixture("1550126.fec")


@pytest.fixture
def filing_1550548(get_fixture):
    """
    Returns the file path for 1550548.fec
    """
    return get_fixture("1550548.fec")

@pytest.fixture
def filing_1606847(get_fixture):
    """
    Returns the file path for 1606847.fec
    """
    return get_fixture("1606847.fec")

@pytest.fixture
def filing_invalid_version(get_fixture):
    """
    Returns the file path for filing_invalid_version.fec
    """
    return get_fixture("filing_invalid_version.fec")