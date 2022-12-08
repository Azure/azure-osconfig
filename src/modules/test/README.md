# `moduletest`

`moduletest` is a functional testing tool that allows developers to easily validate the functionality of a module using a test definition to specifiy a series of steps. These steps con be used to validate the input/output of a module and validate that the returned payloads and status codes are as expected.

## Usage

The `moduletest` command line tool is used to *execute* a test definition.

> By default, modules are loaded from `/usr/lib/osconfig`. Modules can be loaded from a different directory using the `--bin` flag.

> For convenience, module binaries and test definitions are collected under the `build/` directory when building `azure-osconfig`:
> - Shared-object binaries are placed under `build/modules/bin`
> - Test definitions are placed under and renamed to match their module respective shared-object binary `build/test/<module>.json`

```bash
$ moduletest <file>... [--bin <path>] [--verbose] [--help]

# Example
$ moduletest path/to/module/test.json
$ moduletest path/to/module/test.json --bin /usr/lib/osconfig
$ moduletest path/to/module/test1.json path/to/module/test2.json
```

## Test definition

A test definition describes the tests to be executed and provides additional context for the test suite.

- `modules` - A list of modules to load for the duration of the test suite. These names correspond to the shared-object file name that will be loaded (e.g. `["hostname"]` corresponds to `<bin-path>/hostname.so`). These modules will be loaded in the order they are specified and unloaded after the test suite has completed.
- `steps` - A list of steps to execute for the test suite. These steps are executed in the order they are specified.
- `setup` | `teardown` - A script that is run before/after test execution.

### Setup and Teardown

The `setup` and `teardown` steps are a list of commands that are executed before and after the test suite. These steps are not required and can be used to perform any setup or teardown actions that may be needed to save/restore the state of a device before/after test execution.

> The `setup` script is executed before modules are loaded and sessions are opened. Similarly, the `teardown` script is executed after sessions are closed and modules are unloaded. (See [Sequencing](#sequencing) for more information))

```json
{
  "setup": "cp config.txt temp.txt",
  "teardown": [
    "rm temp.txt",
    "echo teardown"
  ]
}
```

### Module steps

Test steps are used to set desired properties and get reported properties of a module and validate the results. These steps are executed in the order they are specified.

  - `type` ***(required)*** - The type of action to preform (e.g. `repored` | `desired`).
  - `component` ***(required)*** - The component to execute the action against (e.g. `HostName`).
  - `object` ***(required)*** - The object to execute the action against (e.g. `name`).
  - `payload` | `json` *(only valid for `set` type)* - The desired payload to send to the module. *(See [JSON payloads](#json-payloads))*
  - `delay` - The number of seconds to wait before executing the current step. This can be used to allow the module to process the previous step. *(default: `0`)*
  - `expect` - An object with the expected results of the action.
    - `status` - The expected status code (number) of the action. (default: `0`)
    - `payload` | `json` (*only valid for `get` type*) - The expected (reported) payload of the action. *(See [JSON payloads](#json-payloads))*

```json
{
  // ...
  "steps": [
    {
      "type": "desired",
      "component": "WiFi",
      "object": "interface",
      "payload": {
        "name": "my-network-interface",
      },
      "expect": {
        "status": 0
      },
      "delay": 1
    },
    {
      "type": "reported",
      "component": "WiFi",
      "object": "interface",
      "expect": {
        "status": 0,
        "payload": "{\"name\": \"my-network-interface\"}"
      },
      "delay": 1
    }
  ]
}
```

### Script steps

Script steps are used to execute a command as part of the test suite. Success or failure of the script is determined by the exit code of the command.

> Script steps are useful for verifying the state of a device before/after a desired property is changed and adding information to the test logs. (e.g. `grep` the contents of a file to verify that a desired property was set correctly.)

> A `delay` can also be added to a script step to allow for processing from the previous step.

```json
{
  // ...
  "steps": [
    {
      // Check if a file exists
      "run": "test -f /path/to/file.config"
    },
    // ...
    {
      // Run multiple commands
      "run": [
        "grep -q 'my-network-interface' /path/to/file.config",
        "cat /path/to/file.config"
      ]
    }
  ]
}
```

### JSON Payloads

JSON payload can be specified in two ways:

- `payload` - A JSON object that will be serialized and sent to the module.
- `json` - A "*raw*" serialized JSON string that will be sent to the module. This can be useful for testing specific payload formats.

```json
{
  "type": "desired",
  "component": "WiFi",
  "object": "interface",
  "payload": {
    "name": "my-network-interface",
  }
}
```

```json
{
  "type": "desired",
  "component": "WiFi",
  "object": "interface",
  "json": "{\"name\": \"my-network-interface\"}"
}
```

### Expectations

Expectations are used to specify the desired outcome of a test. The `expect` object can be used to specify the expected status code and payload of a test. If the expected status code or payload does not match the actual status code or payload, the test will fail.

> If a status code is not specified, the default status code of `0` (success) is used.

> If an expected payload is not omitted from a `get` test, no comparison is performed.

```json
{
  "type": "reported",
  "component": "Network",
  "object": "interface",
  "expect": {
    "status": 0,
    "payload": {
      "name": "my-network-interface",
    }
  }
}
```

```json
{
  "type": "reported",
  "component": "Network",
  "object": "interface",
  "expect": {
    "status": 0,
    "json": "{\"name\": \"my-network-interface\"}"
  }
}
```

## Sequencing

The order of operations for a test suite is important for preserving the state of a device and properly cleaning up after a test. During test execution, the `moduletest` tool will perform the following actions in order for each test definition provided as an argument:

1. Parse the test definition
1. Execute the `setup` script *(if provided)*
1. Load the modules specified in the test definition
1. Open a session for each module
1. Execute the steps specified in the test definition
1. Close the session for each module
1. Unload the modules specified in the test definition
1. Execute the `teardown` script *(if provided)*

This avoids any potential race conditions between the loading/unloading of modules and the setup/teardown scripts.