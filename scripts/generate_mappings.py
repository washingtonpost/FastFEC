"""
A utility script to generate C code to get FEC headers and type information given form and version.

This script utilizes information in mappings.json and types.json (in the same directory) to
generate mappings_generated.h in the top-level src/ directory.
"""

import csv
import io
import json
import os
import sys

script_dir = os.path.dirname(os.path.realpath(__file__))


def c_escape(text):
    """Escapes the text to be a valid, properly escaped C string"""
    return json.dumps(text)


def generate_c_array(variable_name, col_width, list_of_lists):
    """Generates a properly formatted C array given a name, column width, and data"""
    c_array = f"\nstatic const char *{variable_name}[][{col_width}] = {{\n    "

    first_row = True
    for row in list_of_lists:
        if not first_row:
            c_array += ",\n    "
        first_row = False

        # Start array
        c_array += "{"
        first_column = True
        for column in row:
            if not first_column:
                c_array += ","
            first_column = False
            c_array += c_escape(column)
        c_array += "}"

    c_array += "\n};\n"
    return c_array


def list_to_csv(list_data):
    """Converts a Python list into a CSV string"""
    output = io.StringIO()
    writer = csv.writer(output)
    writer.writerow(list_data)
    # Strip trailing newline
    return output.getvalue()[:-2]


def with_comment(comment, text):
    """Prepends the specified text with a comment that gets formatted in C style"""
    comment = "\n".join([f"// {s}" for s in comment.strip().split("\n")])

    return f"{comment}{text}"


type_enum = {
    "float": "f",
    "date": "d",
}


if __name__ == "__main__":
    test_mode = sys.argv[-1] == "test"
    if test_mode:
        print("Testing mappings")

    with open(os.path.join(script_dir, "mappings.json"), "r", encoding="utf8") as f:
        mappings_json = json.load(f)
    with open(os.path.join(script_dir, "types.json"), "r", encoding="utf8") as f:
        types_json = json.load(f)

    headers = []
    for form_type in mappings_json:
        for version in mappings_json[form_type]:
            headers.append(
                [version, form_type, list_to_csv(mappings_json[form_type][version])]
            )
    header_table = generate_c_array("headers", 3, headers)

    types = []
    for form_type in types_json:
        for version in types_json[form_type]:
            for type_col in types_json[form_type][version]:
                type_mapping = types_json[form_type][version][type_col]
                type_value = type_mapping.get("type")
                if type_value is None:
                    raise ValueError("Expect to get a type")
                if type_value == "date":
                    if type_mapping.get("format") != "%Y%m%d":
                        raise ValueError(
                            f'Unexpected date format: {type_mapping.get("format")}'
                        )
                elif type_value != "float":
                    raise ValueError(f"Unexpected type: {type_value}")

                type_row = [version, form_type, type_col, type_enum[type_value]]
                types.append(type_row)
    type_table = generate_c_array("types", 4, types)

    result = with_comment(
        "Auto-generated from https://github.com/esonderegger/fecfile\n"
        + "The mappings.json and types.json files were used to generate this header",
        "\n\n\n",
    )
    result += with_comment(
        "Mapping of FEC filing version and form type to header names\n"
        + "The first two columns are regexes matching the version and form type\n"
        + "The last column is a CSV of resulting header values",
        header_table,
    )
    result += "\n"
    result += with_comment(
        "Mapping of FEC filing version, form type, and column name to type\n"
        + "The first three columns are the version, form type, and column name\n"
        + "specified as regexes. The last column is a single-letter code, where\n"
        + "d is YYYYMMDD date and f is float. If nothing matches, the type is\n"
        + "assumed to be s (string).",
        type_table,
    )

    if test_mode:
        with open(
            os.path.join(script_dir, "../src/mappings_generated.h"),
            "r",
            encoding="utf8",
        ) as f:
            assert (
                result == f.read()
            ), "Mappings outdated: Update by running `python scripts/generate_mappings.py`"
            print("Mappings are up-to-date")
    else:
        with open(
            os.path.join(script_dir, "../src/mappings_generated.h"),
            "w",
            encoding="utf8",
        ) as f:
            f.write(result)
