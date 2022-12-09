# `moduletest`

`moduletest` is a functional testing tool that allows developers to easily validate the functionality of a module using a test recipe to specifiy a series of steps. These steps con be used to validate the input/output of a module and validate that the returned payloads and status codes are as expected.

## Usage

The `moduletest` command line tool is used to *execute* a test recipe.

> By default, modules are loaded from `/usr/lib/osconfig`. Modules can be loaded from a different directory using the `--bin` flag.

> For convenience, module binaries are collected under the `build/modules/bin` directory when building `azure-osconfig`.

```bash
$ moduletest <file>... [--bin <path>] [--verbose] [--help]

# Examples
$ moduletest path/to/module/test.json
$ moduletest path/to/module/test.json --bin /usr/lib/osconfig
$ moduletest path/to/module/test1.json path/to/module/test2.json
```

## Test recipe

A test recipe describes the tests to be executed and provides additional context for the test suite.

Test steps are used to set desired properties and get reported properties of a module and validate the results. These steps are executed in the order they are specified.

  - `Type` ***(required)*** - The type of action to preform (e.g. `Repored` | `Desired`).
  - `Component` ***(required)*** - The component to execute the action against (e.g. `HostName`).
  - `Object` ***(required)*** - The object to execute the action against (e.g. `name`).
  - The payload used for this test step. If the test type is `reported`, the payload is compared against the reported value. If the test type is `Desired`, the payload is used to set the desired value.
    - `Payload` - A JSON object that will be serialized and sent to the module.
    - `Json` - A "*raw*" serialized JSON string that will be sent to the module. This can be useful for testing specific payload formats.
  - `Delay` - The number of seconds to wait before executing the current step. This can be used to allow the module to process the previous step. *(default: `0`)*
  - `Status` - The expected status code (number) of the action. (default: `0`)

```json
[
  {
    "Type": "Desired",
    "Component": "WiFi",
    "Object": "interface",
    "Payload": {
      "name": "my-network-interface",
    },
    "Status": 0,
    "Delay": 1
  },
  {
    "Type": "Reported",
    "Component": "WiFi",
    "Object": "interface",
    "Json": "{\"name\": \"my-network-interface\"}",
    "Status": 0,
    "Delay": 1
  }
]

```

### Command steps

Command steps are used to execute a command as part of the test suite. Success or failure of the command is determined by the exit code of the command. By default, an exit code of `0` is considered a success, however a `Status` can be provided to override this behavior.

> Command steps are useful for verifying the state of a device before/after a desired property is changed and adding information to the test logs. (e.g. `grep` the contents of a file to verify that a desired property was set correctly.)

> A `Delay` can also be added to a command step to allow for processing from the previous step.

```json
[
  {
    "RunCommand": "grep -q 'my-network-interface' /etc/wpa_supplicant/wpa_supplicant.conf"
  },
  {
    "RunCommand": "exit 123",
    "Status": 123
  }
]
```

### Module steps

Module steps are used to load and unload a module as part of the test suite. This can be useful for testing the behavior of a module when it is loaded/unloaded and gives the opportunity to verify the state of the device before/after the module is loaded/unloaded.

> If the unload step is omitted, the module will be unloaded after the test suite is complete.

```json
[
  {
    "Action": "LoadModule",
    "Module": "my-module.so"
  },
  // ...
  {
    "Action": "UnloadModule"
  }
]
```