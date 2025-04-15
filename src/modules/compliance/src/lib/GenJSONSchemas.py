#!/usr/bin/env python3
import os
import re
import glob
from collections import OrderedDict
import json

def parse_macro_args(args):
    """Extract and parse arguments from AUDIT_FN or REMEDIATE_FN lines."""
    # Split by comma but respect quoted strings

    if not args:
        return None, OrderedDict()

    # First arg is the function name
    fn_name = args[0].strip()
    arg_dict = OrderedDict()

    # Process remaining arguments
    for arg in args[1:]:
        arg = arg.strip()
        if len(arg) >= 2 and arg[0] == '"' and arg[-1] == '"':
            arg = arg[1:-1]
        else:
            raise ValueError("Improperly quoted parameter")
        parts = arg.split(':')
        name = parts[0]
        if len(name) == 0:
            raise ValueError(f"Empty argument name found for function '{fn_name}'.")
        if arg_dict.get(name) is not None:
            raise ValueError(f"Duplicate argument '{name}' found for function '{fn_name}'.")
        arg_dict[name] = OrderedDict()
        if len(parts) > 1:
            arg_dict[name]["description"] = parts[1]
        if len(parts) > 2:
            arg_dict[name]["flags"] = parts[2]
        if len(parts) > 3:
            arg_dict[name]["pattern"] = parts[3]

    return fn_name, arg_dict

def process_procedures(content, prefix):
    procedures = OrderedDict()
    startpos = content.find(prefix + "(", 0)
    while startpos != -1:
        startpos += len(prefix + "(")
        endpos = startpos
        quoted = False
        escaped = False
        args = []
        current = ""
        while True:
            char = content[endpos]
            if escaped:
                current += char
                escaped = False
            elif char == ')' and not quoted:
                break
            elif char == '"':
                current += char
                quoted = not quoted
            elif char == '\\':
                current += char
                escaped = True
            elif char == "," and not quoted:
                args.append(current)
                current = ""
            else:
                current += char
            endpos += 1
        if len(current) > 0:
            args.append(current)
        fn_name, arg_dict = parse_macro_args(args)
        if fn_name:
            if fn_name[0].islower():
                raise ValueError(f"Error function name '{fn_name}' starts with Lowercase in '{prefix}'")
            # Check for duplicate function names
            if procedures.get(fn_name) is not None:
                raise ValueError(f"Duplicate function name '{fn_name}' found in '{prefix}'")
            procedures[fn_name] = arg_dict
        startpos = content.find(prefix, endpos)
    return procedures

def process_cpp_file(filepath):
    """Extract AUDIT_FN and REMEDIATE_FN information from a C++ file."""
    print(f"Processing {filepath}")
    with open(filepath, 'r', encoding='utf-8', errors='ignore') as file:
        content = file.read()

    result = {}
    result['audits'] = process_procedures(content, "AUDIT_FN")
    result['remediations'] = process_procedures(content, "REMEDIATE_FN")

    return result

def create_procedure_schema(key, args):
    """Create a schema for a single procedure"""
    result = OrderedDict()
    result["type"] = "object"
    result["required"] = [key,]
    result["additionalProperties"] = False
    props = dict()
    props["type"] = "object"
    props["required"] = []
    for arg in args:
        if "M" in args[arg].get("flags", ""):
            props["required"].append(arg)
    props["additionalProperties"] = False
    props["properties"] = OrderedDict()
    for arg in args:
        props["properties"][arg] = {
            "type": "string"
        }
        if args[arg].get("description") is not None:
            props["properties"][arg]["description"] = args[arg].get("description")
        # We need to "fix" the pattern so that it allows for an argument name to be included instead of the actual argument.
        # For the default value, we need to fix the beginning/ending of the value.
        pattern = args[arg].get("pattern")
        if pattern is not None:
            s_pattern = pattern
            if len(s_pattern) > 0 and s_pattern[0] == "^":
                s_pattern = s_pattern[1:]
            else:
                s_pattern = ".*" + s_pattern
            if len(s_pattern) > 0 and s_pattern[-1] == "$":
                s_pattern = s_pattern[:-1]
            else:
                s_pattern = s_pattern + ".*"
            props["properties"][arg]["pattern"] = f"(^\\$[a-zA-Z0-9_]+:({s_pattern})$|({pattern}))"

    result["properties"] = { key: props }
    return result

def create_file_schema(filedata):
    """Create a schema for the whole file"""
    schema = OrderedDict()
    schema["$schema"] = "http://json-schema.org/draft-07/schema#"
    schema["definitions"] = {
        "audit": {
            "anyOf": [ create_procedure_schema(key, filedata["audits"][key]) for key in filedata["audits"] ]
        },
        "remediation": {
            "anyOf": [ create_procedure_schema(key, filedata["remediations"][key]) for key in filedata["remediations"] ]
        },
    }
    return schema

def process_directory(directory_path):
    """Process all CPP files in a directory and return structured information."""
    cpp_files = glob.glob(os.path.join(directory_path, "**/*.cpp"), recursive=True)

    # Sort files to ensure alphabetical order by basename
    cpp_files.sort(key=lambda x: os.path.basename(x))

    result = OrderedDict()
    subschemas = list()
    for filepath in cpp_files:
        basename = os.path.splitext(os.path.basename(filepath))[0]
        file_result = process_cpp_file(filepath)
        result[basename] = create_file_schema(file_result)

    return result

def main():
    """Main function to process a directory and print results."""
    basedir = os.path.dirname(os.path.realpath(__file__))
    print(f"Processing procedures in {basedir}")
    result = process_directory(f"{basedir}/procedures/")
    for fname in result:
        print(f"Writing schema for {fname}.")
        with open(f"{basedir}/procedures/{fname}.schema.json","w") as f:
            json.dump(result[fname], f, indent=2)
            # Keep EOL check happy
            f.write("\n")

    print("Reading global payload schema.")
    schema = None
    with open(f"{basedir}/payload.schema.json", "r") as f:
        schema = json.load(f)
    schema["definitions"]["auditProcedure"]["anyOf"] = [ {"$ref" : f"procedures/{fname}.schema.json#/definitions/audit" } for fname in result.keys() ]
    schema["definitions"]["remediationProcedure"]["anyOf"] = [ {"$ref" : f"procedures/{fname}.schema.json#/definitions/remediation" } for fname in result.keys() ]
    # If there's no remediation we fall back to audit
    schema["definitions"]["remediationProcedure"]["anyOf"].append( { "$ref": "#definitions/auditProcedure" })
    print("Dumping global schema.")
    with open(f"{basedir}/payload.schema.json", "w") as f:
        json.dump(schema, f, indent=2)
        # Keep EOL check happy
        f.write("\n")

if __name__ == "__main__":
    import sys

    main()
