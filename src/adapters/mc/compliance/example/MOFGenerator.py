import json
import base64
import hashlib
import uuid
import sys
from datetime import datetime, timezone

def generate_uuid(name):
    return str(uuid.UUID(hashlib.sha256(name.encode()).hexdigest()[:32]))

def compact_base64_encode(obj):
    return base64.b64encode(json.dumps(obj, separators=(',', ':')).encode()).decode()

def extract_parameters(audit):
    parameters = {}
    def recurse(obj):
        if isinstance(obj, dict):
            for key, value in obj.items():
                if isinstance(value, str) and value.startswith("$"):
                    param, _, default = value[1:].partition(":")
                    parameters[param] = default
                    obj[key] = f"${param}"
                else:
                    recurse(value)
        elif isinstance(obj, list):
            for item in obj:
                recurse(item)
    recurse(audit)
    return parameters

def generate_osconfig_resource(resource, index):
    resource_id = resource["name"]
    payload_key = resource["key"]
    rule_id = generate_uuid(resource_id)

    audit = resource.get("audit")
    remediate = resource.get("remediate")

    parameters = extract_parameters(audit)
    d = {"name": resource_id}
    if remediate is not None:
        d["remediate"] = remediate
    if audit is not None:
        d["audit"] = audit
    if parameters is not None:
        d["parameters"] = parameters
    procedure_object_value = compact_base64_encode(d)
    desired_object_value = " ".join([f"{param}={default}" for param, default in parameters.items()])

    return f'''instance of OsConfigResource as $OsConfigResource{index}ref
{{
   ResourceID = "{resource_id}";
   PayloadKey = "{payload_key}";
   RuleId = "{rule_id}";
   ComponentName = "Compliance";
   ProcedureObjectName = "procedureObject";
   ProcedureObjectValue = "{procedure_object_value}";
   InitObjectName = "initObject";
   ReportedObjectName = "auditObject";
   ExpectedObjectValue = "PASS";
   DesiredObjectName = "remediateObject";
   DesiredObjectValue = "{desired_object_value}";
   ModuleName = "GuestConfiguration";
   ModuleVersion = "1.0.0";
   ConfigurationName = "ComplianceExample";
   SourceInfo = "::4::5::OsConfigResource";
}};'''

def main():
    if len(sys.argv) != 2:
        print("Usage: ComplianceExampleGenerator.py <input_json_file>")
        sys.exit(1)

    input_file = sys.argv[1]
    with open(input_file, 'r') as file:
        resources = json.load(file)

    for index, resource in enumerate(resources):
        print(generate_osconfig_resource(resource, index))

    generation_date = datetime.now(timezone.utc).strftime("%m/%d/%Y %H:%M:%S %Z")
    print(f'''instance of OMI_ConfigurationDocument
{{
    Version="3.0.0";
    CompatibleVersionAdditionalProperties= {{"Omi_BaseResource:ConfigurationName"}};
    Author="Microsoft";
    GenerationDate="{generation_date}";
    Name="ComplianceExample";
}};''')

if __name__ == "__main__":
    main()
