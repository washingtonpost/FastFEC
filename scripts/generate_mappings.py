import json
import os
import io
import csv
script_dir = os.path.dirname(os.path.realpath(__file__))


def c_escape(str):
    return json.dumps(str)


def generate_c_array(variable_name, col_width, list_of_lists):
    result = f"\nstatic const char *{variable_name}[][{col_width}] = {{\n    "

    first_row = True
    for row in list_of_lists:
        if not first_row:
            result += ",\n    "
        first_row = False

        # Start array
        result += '{'
        first_column = True
        for column in row:
            if not first_column:
                result += ","
            first_column = False
            result += c_escape(column)
        result += "}"

    result += "\n};\n"
    return result


def list_to_csv(l):
    output = io.StringIO()
    writer = csv.writer(output)
    writer.writerow(l)
    # Strip trailing newline
    return output.getvalue()[:-2]


def with_comment(comment, text):
    comment = '\n'.join([f'// {s}' for s in comment.strip().split('\n')])

    return f'{comment}{text}'


type_enum = {
    'float': 'f',
    'date': 'd',
}


if __name__ == '__main__':
    with open(os.path.join(script_dir, 'mappings.json'), 'r') as f:
        mappings_json = json.load(f)
    with open(os.path.join(script_dir, 'types.json'), 'r') as f:
        types_json = json.load(f)

    headers = []
    for form_type in mappings_json:
        for version in mappings_json[form_type]:
            headers.append(
                [version, form_type, list_to_csv(mappings_json[form_type][version])])
    header_table = generate_c_array("headers", 3, headers)

    types = []
    for form_type in types_json:
        for version in types_json[form_type]:
            for type_col in types_json[form_type][version]:
                type_mapping = types_json[form_type][version][type_col]
                type_value = type_mapping.get('type')
                if type_value is None:
                    raise ValueError('Expect to get a type')
                elif type_value == 'date':
                    if type_mapping.get('format') != '%Y%m%d':
                        raise ValueError(
                            f'Unexpected date format: {type_mapping.get("format")}')
                elif type_value != 'float':
                    raise ValueError(f'Unexpected type: {type_value}')

                type_row = [version, form_type,
                            type_col, type_enum[type_value]]
                types.append(type_row)
    type_table = generate_c_array("types", 4, types)

    result = with_comment(
        'Auto-generated from https://github.com/esonderegger/fecfile\nThe mappings.json and types.json files were used to generate this header', '\n\n\n')
    result += with_comment('Mapping of FEC filing version and form type to header names\nThe first two columns are regexes matching the version and form type\nThe last column is a CSV of resulting header values', header_table)
    result += '\n'
    result += with_comment('Mapping of FEC filing version, form type, and column name to type\nThe first three columns are the version, form type, and column name\nspecified as regexes. The last column is a single-letter code, where\nd is YYYYMMDD date and f is float. If nothing matches, the type is\nassumed to be s (string).', type_table)

    with open(os.path.join(script_dir, '../src/mappings_generated.h'), 'w') as f:
        f.write(result)
