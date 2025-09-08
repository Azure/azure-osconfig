#!/usr/bin/env python3
from dataclasses import dataclass
import os
import re
import glob
from collections import OrderedDict
import json
from typing import List

@dataclass
class Enum:
    mapping: dict
    description: str

# Represents a single parameter in a struct <Name>Params declaration
@dataclass
class Parameter:
    # Type of the parameter, e.g. std::string, Optional<int>, Separated<std::string,','>
    type: str

    # Name of the parameter, e.g. path, mode, ...
    name: str

    # True if the parameter has type Optional<T>. This means the user can skip setting
    # this parameter in the args map.
    optional: bool

    # Description of the parameter, taken from the comment above the parameter declaration
    # starting with: ///
    description: str

    # Optionally, a regular expression pattern that the parameter value must match
    # Taken from the comment above the parameter declaration starting with: /// pattern:
    pattern: str

    # Line number of the parameter declaration
    line: int

# Represents a struct <Name>Params declaration
@dataclass
class Parameters:
    # List of Parameter objects
    params: List

    # File name of the struct is declaration
    filename: str

    # Line number of the struct declaration
    line: int

# Represents a procedure declaration: Audit<Name> or Remediate<Name>
@dataclass
class Procedure:
    # File name of the procedure declaration
    filename: str

    # Line number of the procedure declaration
    line: int

    # Name of the parameters struct, e.g. <Name>Params.
    # It may be None if the procedure has no parameters.
    params_name: str

# Represents the whole model extracted from the header files
@dataclass
class Model:
    # A dictonary of procedure name to Procedure objects representing audits
    audits: dict

    # A dictonary of procedure name to Procedure objects representing remediations
    remediations: dict

    # A dictonary of enum name to Enum objects
    enums: dict

    # A set of supported types, e.g. int, std::string, regex, bool, mode_t, and any enum names
    supported_types: set

    # A dictonary of parameters struct name to Parameters objects
    parameters: dict

    # A set of processed header file names
    file_index: set

def validate_pattern(pattern):
    try:
        re.compile(pattern)
    except re.error as e:
        print(f"Invalid regex pattern: {pattern} - Error: {e}")
        raise

def validate_procderue(procedure_type: str, procedure_name: str, procedure: Procedure, model: Model) -> None:
    if not procedure.filename or not procedure.line:
        raise ValueError(f"{procedure_type}{procedure_name} is missing filename or line number.")
    if procedure.params_name:
        if not procedure.params_name in model.parameters:
            raise ValueError(f"{procedure_type}{procedure_name} has unknown params struct {procedure.params_name}.")
        params = model.parameters[procedure.params_name]
        for parameter in params.params:
            type_name = parameter.type
            if type_name.startswith("Optional<"):
                if not parameter.optional:
                    raise ValueError(f"Parameter {parameter.name} in {procedure_type}{procedure_name} is marked as non-optional but has type {parameter.type}.")
                type_name = type_name[len("Optional<"):-1]  # Strip "Optional<" and ">"
            elif parameter.optional:
                raise ValueError(f"Parameter {parameter.name} in {procedure_type}{procedure_name} is marked as optional but has type {parameter.type}.")
            if type_name.startswith("Separated<") and type_name.endswith(">"):
                type_name = type_name[len("Separated<"):-1]  # Strip "Separated<" and ">"
                # Truncate at the first comma
                type_name = type_name.split(',',1)[0]
            if type_name not in model.supported_types:
                raise ValueError(f"{procedure_type}{procedure_name} has unsupported type {parameter.type} for parameter {parameter.name}. Supported types: {model.supported_types}")

def validate_model(model: Model):
    """Validate the generated output."""
    for name, procedure in model.audits.items():
        validate_procderue("Audit", name, procedure, model)
    for name, procedure in model.remediations.items():
        validate_procderue("Remediate", name, procedure, model)

def basename(filepath: str) -> str:
    return os.path.basename(filepath)

# Process the content of struct Audit<Name>Params declaration
def process_params_block(file, lineno: int):
    fields = []
    description_value = None
    pattern_value = None
    inside = False
    for line in file:
        lineno += 1
        line = line.strip()
        pat = r"^\s*{"
        if re.search(pat, line):
            inside = True
            continue
        if not inside:
            continue
        pat = r"^\s*}\s*;"
        if re.search(pat, line):
            return fields, lineno

        pat = r"///\s*pattern:\s*(.*)"
        match = re.search(pat, line)
        if match:
            pattern_value = match.group(1).strip()
            validate_pattern(pattern_value)
            continue

        pat = r"///\s*(.*)"
        match = re.search(pat, line)
        if match:
            description_value = match.group(1).strip()
            continue

        pat = r"((\w|::)+)(<(.*)>)?\s+(\w+)(\s*;|\s*=.*;)"
        match = re.search(pat, line)
        if match:
            field_type = match.group(1).strip()
            inner_type = match.group(4)
            field_name = match.group(5).strip()
            if field_type == "Optional":
                optional = True
            else:
                optional = False
            if inner_type:
                field_type = field_type + "<" + inner_type + ">"
            fields.append(Parameter(type=field_type, name=field_name, description=description_value, pattern=pattern_value, optional=optional, line=lineno))
            description_value = None
            pattern_value = None
            continue
    return fields, lineno

def process_enum_block(file, lineno: int) -> tuple:
    result = Enum(mapping=dict(), description="")
    inside = False
    label_value = None
    for line in file:
        lineno += 1
        line = line.strip()
        pat = r"{"
        if re.search(pat, line):
            inside = True
            continue
        if not inside:
            continue
        pat = r"}.*;"
        if re.search(pat, line):
            return result, lineno

        pat = r"///\s*label:\s*(.*)"
        match = re.search(pat, line)
        if match:
            label_value = match.group(1).strip()
            continue

        pat = r"///\s*(.*)"
        match = re.search(pat, line)
        if match:
            result.description = match.group(1).strip()
            continue

        pat = r"\s*(\w+)\s*(,|\n)"
        match = re.search(pat, line)
        if match:
            label_name = match.group(1).strip()
            if not label_value:
                label_value = label_name
            result.mapping[label_name] = label_value
            label_value = None
            continue

    raise ValueError("Missing enum closing")

def set_procedure_declaration(procedure_type: str, name: str, params_name, lineno: int, filepath: str, model: Model):
    procedure = None
    filename = basename(filepath)
    if procedure_type not in ("Audit", "Remediate"):
        raise ValueError(f"Invalid procedure type {procedure_type} for {name} in {filepath}:{lineno}")
    if params_name and not params_name in model.parameters:
        raise ValueError(f"Parameters struct {params_name} not found for {procedure_type}{name} in {filepath}:{lineno}")
    if procedure_type == "Audit":
        if name in model.audits:
            raise ValueError(f"Duplicate declaration of {procedure_type}{name} in {filepath}:{lineno}: previous declaration found in {model.audits[name].filename}:{model.audits[name].line}")
        model.audits[name] = Procedure(filename=filename, line=lineno, params_name=params_name)
        procedure = model.audits[name]
    elif procedure_type == "Remediate":
        if name in model.remediations:
            raise ValueError(f"Duplicate declaration of {procedure_type}{name} in {filepath}:{lineno}: previous declaration found in {model.remediations[name].filename}:{model.remediations[name].line}")
        model.remediations[name] = Procedure(filename=filename, line=lineno, params_name=params_name)
        procedure = model.remediations[name]
    if not procedure:
        raise ValueError(f"Internal error: procedure is None for {procedure_type}{name} in {filepath}:{lineno}")
    model.file_index.add(filename)

def set_parameters_declaration(name: str, fields: List[Parameter], lineno: int, filepath: str, model: Model):
    if name in model.parameters:
        raise ValueError(f"Duplicate declaration of {name} in {filepath}:{lineno}: previous declaration found in {model.parameters[name].filename}:{model.parameters[name].line}")
    model.parameters[name] = Parameters(params=fields, filename=basename(filepath), line=lineno)

def process_header_file(filepath: str, model: Model):
    """Extract AUDIT_FN and REMEDIATE_FN information from a C++ header file."""
    print(f"Processing {filepath}")
    with open(filepath, 'r', encoding='utf-8', errors='ignore') as file:
        lineno = 0
        for line in file:
            line = line.strip()
            lineno += 1
            pat = r"struct\s+((\w+)Params)"
            match = re.search(pat, line)
            if match:
                # Process the match
                name = match[1]
                fields, lineno = process_params_block(file, lineno)
                set_parameters_declaration(name, fields, lineno, filepath=filepath, model=model)

            pat = r"enum\s+class\s+(\w+)"
            match = re.search(pat, line)
            if match:
                name = match[1]
                # Process the enum block
                enum, lineno = process_enum_block(file, lineno)
                model.enums[name] = enum
                model.supported_types.add(name)

            pat = r"Result<Status>\s+(Audit|Remediate)(\w+)\s*(.*)"
            match = re.search(pat, line)
            if match:
                procedure_type = match[1]
                name = match[2]
                pat = r".*\(\s*const\s*(\w+)\s*&\s*\w+\s*,\s*IndicatorsTree"
                match = re.search(pat, line)
                params_name = None
                if match:
                    params_name = match[1]
                    if params_name not in model.parameters:
                        raise ValueError(f"Parameters struct {params_name} not found for {procedure_type}{name} in {filepath}:{lineno}")
                set_procedure_declaration(procedure_type, name, params_name, lineno, filepath, model)

def create_procedure_schema(name: str, procedure: Procedure, model: Model) -> OrderedDict:
    """Create a schema for a single procedure"""
    result = OrderedDict()
    result["type"] = "object"
    result["required"] = [name,]
    result["additionalProperties"] = False
    props = dict()
    props["type"] = "object"
    props["required"] = []
    props["additionalProperties"] = False
    props["properties"] = OrderedDict()
    if procedure.params_name:
        params = model.parameters[procedure.params_name]
        for field in params.params:
            if not field.optional:
                props["required"].append(field.name)
        for field in params.params:
            props["properties"][field.name] = {
                "type": "string"
            }
            if field.description is not None:
                props["properties"][field.name]["description"] = field.description
            # We need to "fix" the pattern so that it allows for an argument name to be included instead of the actual argument.
            # For the default value, we need to fix the beginning/ending of the value.
            if field.pattern is not None:
                s_pattern = field.pattern
                if len(s_pattern) > 0 and s_pattern[0] == "^":
                    s_pattern = s_pattern[1:]
                else:
                    s_pattern = ".*" + s_pattern
                if len(s_pattern) > 0 and s_pattern[-1] == "$":
                    s_pattern = s_pattern[:-1]
                else:
                    s_pattern = s_pattern + ".*"

                final_pattern = f"(^\\$[a-zA-Z0-9_]+:({s_pattern})$|({field.pattern}))"
                validate_pattern(final_pattern)
                props["properties"][field.name]["pattern"] = final_pattern

    result["properties"] = { name: props }
    return result

def create_file_schema(filename: str, model: Model):
    """Create a schema for the whole file"""
    schema = OrderedDict()
    schema["$schema"] = "http://json-schema.org/draft-07/schema#"
    audits = []
    remediations = []
    for name, procedure in sorted(model.audits.items()):
        if procedure.filename != filename:
            continue
        audits.append(create_procedure_schema(name, procedure, model))
    for name, procedure in sorted(model.remediations.items()):
        if procedure.filename != filename:
            continue
        remediations.append(create_procedure_schema(name, procedure, model))
    schema["definitions"] = {
        "audit": {
            "anyOf": audits
        },
        "remediation": {
            "anyOf": remediations
        },
    }
    return schema

def process_directory(model: Model, directory_path: str) -> None:
    """Process all CPP files in a directory and return structured information."""
    header_files = glob.glob(os.path.join(directory_path, "**/*.h"), recursive=True)

    # Sort files to ensure alphabetical order by basename
    header_files.sort(key=lambda x: os.path.basename(x))

    # key: procedure name, value: Procedure
    for filepath in header_files:
        process_header_file(filepath, model)

def generate_procedure_map_header(model: Model, filename: str):
    """Generate a header file for the procedure map."""
    includes = set()
    for name, procedure in model.audits.items():
        includes.add(f"#include <{procedure.filename}>\n")
    for name, procedure in model.remediations.items():
        includes.add(f"#include <{procedure.filename}>\n")
    with open(filename,"w") as f:
        f.write("// This file is auto-generated. Do not edit manually.\n")
        f.write("#ifndef COMPLIANCEENGINE_PROCEDURE_MAP_H\n")
        f.write("#define COMPLIANCEENGINE_PROCEDURE_MAP_H\n")
        f.write("\n")
        for line in sorted(includes):
            f.write(line)
        f.write("\n")
        f.write("namespace ComplianceEngine\n")
        f.write("{\n")
        f.write("// Forward declaration, defined in Bindings.h\n")
        f.write("template <typename Params>\n")
        f.write("struct Bindings;\n")
        f.write("\n")
        f.write("// Forward declaration, defined in Bindings.h\n")
        f.write("template <typename Enum>\n")
        f.write("const std::map<std::string, Enum>& MapEnum();\n")
        f.write("\n")
        for name, details in model.enums.items():
            f.write(f"// Maps the {name} enum labels to the enum values.\n")
            f.write("template <>\n")
            f.write(f"inline const std::map<std::string, {name}>& MapEnum<{name}>()\n")
            f.write("{\n")
            f.write(f"    static const std::map<std::string, {name}> map = {{\n")
            for k, v in details.mapping.items():
                f.write(f"        {{\"{v}\", {name}::{k}}},\n")
            f.write("    };\n")
            f.write("    return map;\n")
            f.write("}\n")
            f.write("\n")
        for name, parameters in model.parameters.items():
            f.write(f"// Defines the bindings for the {name} structure.\n")
            f.write("template <>\n")
            f.write(f"struct Bindings<{name}>\n")
            f.write("{\n")
            f.write(f"    using T = {name};\n")
            f.write(f"    static constexpr size_t size = {len(parameters.params)};\n")
            f.write(f"    static const char* names[];\n")
            f.write(f"    static constexpr auto members = std::make_tuple({', '.join(f'&T::{field.name}' for field in parameters.params)});\n")
            f.write("};\n")
            f.write("\n")
        f.write("} // namespace ComplianceEngine\n")
        f.write("\n")
        f.write("namespace std\n")
        f.write("{\n")
        for name, details in model.enums.items():
            f.write(f"// Returns a string representation of the {name} enum value.\n")
            f.write(f"string to_string(ComplianceEngine::{name} value) noexcept(false); // NOLINT(*-identifier-naming)\n")
            f.write("\n")
        f.write("} // namespace std\n")
        f.write("#endif // COMPLIANCEENGINE_PROCEDURE_MAP_H\n")

def generate_procedure_map_impl(model: Model, filename: str):
    """Generate an implementation file for the procedure map."""
    with open(filename,"w") as f:
        f.write("// This file is auto-generated. Do not edit manually.\n")
        f.write("#include <ProcedureMap.h>\n")
        f.write("#include <Bindings.h>\n")
        f.write("#include <RevertMap.h>\n")
        f.write("\n")
        f.write("namespace ComplianceEngine\n")
        f.write("{\n")
        for name, parameters in model.parameters.items():
            f.write(f"// {parameters.filename}:{parameters.line}\n")
            names = ', '.join(f'\"{field.name}\"' for field in parameters.params)
            f.write(f"const char* Bindings<{name}>::names[] = {{{names}}};\n")
            f.write("\n")
        f.write("const ProcedureMap Evaluator::mProcedureMap = {\n")
        handlers = dict() # dict[str, [str,str]]
        for name, procedure in model.audits.items():
            if not name in handlers:
                handlers[name] = ["nullptr", "nullptr"]
            handlers[name][0] = f"MakeHandler(Audit{name})"
        for name, procedure in model.remediations.items():
            if not name in handlers:
                handlers[name] = ["nullptr", "nullptr"]
            handlers[name][1] = f"MakeHandler(Remediate{name})"
        for name, (audit, remediation) in sorted(handlers.items()):
            f.write(f"    {{\"{name}\", {{{audit}, {remediation}}}}},\n")
        f.write("};\n")
        f.write("} // namespace ComplianceEngine\n")
        f.write("\n")
        f.write("namespace std\n")
        f.write("{\n")
        for name, details in model.enums.items():
            f.write(f"string to_string(const ComplianceEngine::{name} value) noexcept(false)\n")
            f.write("{\n")
            f.write(f"    const auto& map = ComplianceEngine::MapEnum<ComplianceEngine::{name}>();\n")
            f.write("    static const auto revmap = ComplianceEngine::RevertMap(map);\n")
            f.write("    const auto it = revmap.find(value);\n")
            f.write("    if (revmap.end() == it)\n")
            f.write("    {\n")
            f.write("        throw std::out_of_range(\"Invalid enum value\");\n")
            f.write("    }\n")
            f.write("    return it->second;\n")
            f.write("}\n")
            f.write("\n")
        f.write("} // namespace std\n")

def generate_global_json_schema(model: Model, basedir: str):
    """Generate a global JSON schema file."""
    print("Reading global payload schema.")
    global_schema = None
    with open(f"{basedir}/payload.schema.json", "r") as f:
        global_schema = json.load(f)
    if not global_schema:
        global_schema = dict()

    # Used for model validation
    definitions = global_schema.get("definitions", {})
    audits = definitions.get("auditProcedure", {}).get("anyOf", [])
    audits_dict = dict()
    for audit in audits:
        pat = r"procedures/([^/]+)\.schema\.json#/definitions/audit"
        match = re.match(pat, audit.get("$ref",""))
        if not match:
            raise ValueError(f"invalid audit reference {audit}")
        fname = match.group(1)
        audits_dict[fname] = audit

    remediations = definitions.get("remediationProcedure", {}).get("anyOf", [])
    remediations_dict = dict()
    for remediation in remediations:
        pat = r"procedures/([^/]+)\.schema\.json#/definitions/remediation"
        match = re.match(pat, remediation.get("$ref",""))
        if not match:
            # This may happend when the missing remediation fallback is present
            continue
        fname = match.group(1)
        remediations_dict[fname] = remediation

    for filename in sorted(model.file_index):
        if not filename.endswith(".h"):
            raise ValueError("filename must end with .h")
        filename = filename[:-2]
        audits_dict[filename] = {"$ref" : f"procedures/{filename}.schema.json#/definitions/audit" }
        remediations_dict[filename] = {"$ref" : f"procedures/{filename}.schema.json#/definitions/remediation" }

    audits = []
    for k, v in sorted(audits_dict.items()):
        audits.append(v)
    remediations = []
    for k, v in sorted(remediations_dict.items()):
        remediations.append(v)

    definitions["auditProcedure"]["anyOf"] = audits
    definitions["remediationProcedure"]["anyOf"] = remediations
    # If there's no remediation we fall back to audit
    fallback = { "$ref": "#definitions/auditProcedure" }
    if fallback not in definitions["remediationProcedure"]["anyOf"]:
        definitions["remediationProcedure"]["anyOf"].append( { "$ref": "#definitions/auditProcedure" })
    print("Dumping global schema.")
    with open(f"{basedir}/payload.schema.json", "w") as f:
        json.dump(global_schema, f, indent=2)
        # Keep EOL check happy
        f.write("\n")

def generate_json_schema(model: Model, basedir: str):
    """Generate JSON schema files for each procedure."""
    for filename in sorted(model.file_index):
        print(f"Writing schema for {filename}.")
        if not filename.endswith(".h"):
            raise ValueError("filename must end with .h")
        json_schema_model = create_file_schema(filename, model)
        # filename = filename[:-2]
        with open(f"{basedir}/procedures/{filename[:-2]}.schema.json","w") as f:
            json.dump(json_schema_model, f, indent=2)
            # Keep EOL check happy
            f.write("\n")

    generate_global_json_schema(model, basedir)

def main():
    """Main function to process a directory and print results."""
    basedir = os.path.dirname(os.path.realpath(__file__))
    model = Model(dict(), dict(), dict(), set(), dict(), set())

    # Supported types are used in model validation
    model.supported_types.add("int")
    model.supported_types.add("std::string")
    model.supported_types.add("regex")
    model.supported_types.add("bool")
    model.supported_types.add("mode_t")
    model.supported_types.add("Pattern")

    # Iterate over header files and update the model
    process_directory(model, f"{basedir}/procedures/")

    # Test model integrity
    validate_model(model)

    # Generate C++ procedure map files
    generate_procedure_map_header(model, f"{basedir}/ProcedureMap.h")
    generate_procedure_map_impl(model, f"{basedir}/ProcedureMap.cpp")

    # Generate JSON schema files
    generate_json_schema(model, basedir)

if __name__ == "__main__":
    main()
