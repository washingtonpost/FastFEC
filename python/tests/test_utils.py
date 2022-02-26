import datetime

from fastfec.utils import array_get, as_bytes, parse_csv_line, parse_date


def test_as_bytes():
    """
    Test to see if the `as_bytes` function returns the expected result.
    """
    assert as_bytes(None) is None
    assert as_bytes(b"test") == b"test"
    assert as_bytes("test") == b"test"


def test_parse_csv_line():
    """
    Test that CSVs are correctly parsed

    TODO: could probably use more cases.
    """
    line = "foo,bar,baz"
    assert parse_csv_line(line) == ["foo", "bar", "baz"]


def test_array_get():
    """
    Test that the array_get helper returns the expected result
    """
    arr = [1, 2, 3]
    assert array_get(arr, 0, None) == 1
    assert array_get(arr, 1, None) == 2
    assert array_get(arr, 2, None) == 3
    assert array_get(arr, 3, None) is None
    assert array_get(arr, 3, "test") == "test"
    assert array_get(arr, 3, 999) == 999


def test_parse_date():
    """
    Test that the parse_date helper correctly parses a date
    """
    assert parse_date("2020-10-01") == datetime.date(2020, 10, 1)
    assert parse_date(None) is None
    assert parse_date("Foo") == "Foo"
    assert parse_date("hello_a_long_string") == "hello_a_long_string"
