import subprocess
import sys
import shlex
import json

def run_command(path, payload):
    print(f"[REQ] {path} {payload}")
    command = f"curl -X POST --unix-socket /run/osconfig/mpid.sock http://osconfig/{path} -H 'Content-Type: application/json' -d '{payload}'"
    args = shlex.split(command)

    result = subprocess.run(args, text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

    print(f"[RES] {result.stdout}")
    return result.stdout

def read_arguments():
    if len(sys.argv) != 3:
        print("Usage: mpi.py <component> <object>")
        sys.exit(1)
    return sys.argv[1], sys.argv[2]

comp, obj = read_arguments()

session = run_command('MpiOpen', '{ "ClientName": "TestClient", "MaxPayloadSizeBytes": 4096 }')
session = session.replace('"', '')

get_body = {
    "ClientSession": session,
    "ComponentName": comp,
    "ObjectName": obj
}

run_command('MpiGet', json.dumps(get_body))

run_command('MpiClose', '{ "ClientSession": "' + session + '" }')