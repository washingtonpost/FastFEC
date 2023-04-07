import os

import pytest

_TEST_FOLDER = os.path.dirname(__file__)
FIXTURE_DIR = os.path.join(_TEST_FOLDER, "fixtures")


def get_fixture(filename):
    """Fetch locally stored fixture files."""
    return os.path.join(FIXTURE_DIR, filename)


@pytest.fixture()
def filing_1550126():
    """Returns the file path for 1550126.fec."""
    return get_fixture("1550126.fec")


@pytest.fixture()
def filing_1550548():
    """Returns the file path for 1550548.fec."""
    return get_fixture("1550548.fec")


@pytest.fixture()
def filing_1606847():
    """Returns the file path for 1606847.fec."""
    return get_fixture("1606847.fec")


@pytest.fixture()
def filing_invalid_version():
    """Returns the file path for filing_invalid_version.fec."""
    return get_fixture("filing_invalid_version.fec")
