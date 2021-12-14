# import json
# import logging
# import os
# import sys
# from unittest import mock

# import pytest

# _TEST_FOLDER = os.path.dirname(__file__)
# FIXTURE_DIR = os.path.join(_TEST_FOLDER, "fixtures")


# @pytest.fixture(scope="session", autouse=True)
# def tests_setup():
#     """
#     Set environment variables before the first test
#     """
#     temp_env_vars = {
#         "DJANGO_API_ROOT": "https://dummy_api_route",
#         "DJANGO_API_AUTH_TOKEN": "dummy_api_auth_token",
#     }
#     os.environ.update(temp_env_vars)


# @pytest.fixture(autouse=True, scope="session")
# def setup_logging():
#     """
#     Set up the logger.
#     """
#     app_logger = logging.getLogger("document_processing")
#     handler = logging.StreamHandler(sys.stdout)
#     handler.setLevel(logging.DEBUG)
#     handler.setFormatter(
#         logging.Formatter(fmt="%(asctime)s %(levelname)s %(name)s %(message)s")
#     )
#     app_logger.addHandler(handler)


# @pytest.fixture(scope="session")
# def get_fixture():
#     """
#     Fetch locally stored fixture data.
#     """

#     def _get_fixture(filename, load=False):
#         with open(os.path.join(FIXTURE_DIR, filename)) as fileobj:
#             if load:
#                 return json.load(fileobj)
#             return fileobj

#     return _get_fixture
