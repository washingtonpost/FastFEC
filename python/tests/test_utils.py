from fastfec.utils import as_bytes
import pytest


def test_as_bytes():
    """
    TK
    """
    assert as_bytes(None) == None
    assert as_bytes(b"test") == b"test"
    assert as_bytes("test") == b"test"
