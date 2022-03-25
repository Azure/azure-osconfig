# Package Manager Configuration module

The Package Manager Configuration module is used to configure Debian packages and get information about their state.

## Desired state

The Package Manager Configuration module is used to update Debian packages (install, uninstall, change package version). Package updating is done by executing `apt-get` commands (`apt-get update`, `apt-get install`). The expected package format is the same as for `apt-get install` [command](https://linux.die.net/man/8/apt-get).

### Example of desired payload:

```
"PackageManagerConfiguration": {
    "DesiredState": {
        "Packages": [
            "cowsay sl=5.02-1",
            "fortune-"
        ]
    }
}
```

## Reported state

The module reports the current configuration of packages on a device and the last execution state.

The configuration of the packages is reflected by reporting the versions of specified desired packages (retrieved by running `apt-cache policy _package-name_ | grep Installed`) and the fingerprint of all the installed packages (computed by executing `dpkg-query --showformat='${Package} (=${Version})\n' --show | sha256sum | head -c 64`).

*Important:*
Only versions of the packages that were last set as desired are reported.
When changing the desired packages, the previously modified packages won't be reported anymore, even though they might still be installed on the system.

The `ExecutionState`, `ExecutionSubState` and `ExecutionSubStateDetails` properties reflect the current state and can have the following values:

| ExecutionState | ExecutionSubState             | ExecutionSubStateDetails | Meaning                                                                                      |
| -------------- |------------------------------ | ------------------------ | -------------------------------------------------------------------------------------------- |
| 0 (Unknown)    | 0 (None)                      | empty                    | No desired properties are known to the module. This is the initial default state.            |
| 1 (Running)    | 1 (DeserializingJsonPayload)  | empty                    | Deserializing PackageManagerConfiguration JSON object                                        |
| 1 (Running)    | 2 (DeserializingDesiredState) | empty                    | Deserializing DesiredState JSON object                                                       |
| 1 (Running)    | 3 (DeserializingPackages)     | package_name(s)          | Deserializing Packages JSON array                                                            |
| 1 (Running)    | 4 (UpdatingPackagesLists)     | empty                    | Refreshing list of packages before updating packages (by running `apt-get update` command)   |
| 1 (Running)    | 5 (InstallingPackages)        | package_name(s)          | Installing packages (by running `apt-get install` command)                                   |
| 2 (Succeeded)  | 0 (None)                      | empty                    | All desired properties were applied successfully                                             |
| 3 (Failed)     | 1 (DeserializingJsonPayload)  | empty                    | Deserializing PackageManagerConfiguration JSON object failed                                 |
| 3 (Failed)     | 2 (DeserializingDesiredState) | empty                    | Deserializing DesiredState JSON object failed                                                |
| 3 (Failed)     | 3 (DeserializingPackages)     | package_name(s)          | Deserializing Packages JSON array failed                                                     |
| 3 (Failed)     | 4 (UpdatingPackagesLists)     | empty                    | Refreshing list of packages packages failed                                                  |
| 3 (Failed)     | 5 (InstallingPackages)        | package_name(s)          | Installing packages failed                                                                   |
| 4 (TimedOut)   | 4 (UpdatingPackagesLists)     | empty                    | Refreshing list of packages timed out                                                        |
| 4 (TimedOut)   | 5 (InstallingPackages)        | package_name(s)          | Installing packages timed out                                                                |

### Example of reported payload:

```
"State": {
    "PackagesFingerprint": "ccdd8d47438ce513a6043276c5efd15b52de35c1f80c88dff312a5d8c19b16ea",
    "Packages": [
        "cowsay=3.03+dfsg2-7",
        "sl=5.02-1",
        "fortune=(none)"
    ],
    "ExecutionState": 2,
    "ExecutionSubState": 0,
    "ExecutionSubStateDetails": ""
}
```

## Error reporting

Package Manager Configuration module can fail in different stages. To be able to identify the cause, take a closer look at `ExecutionState`, `ExecutionSubState` and `ExecutionSubStateDetails`. The following table shows failure codes and their possible causes:

| ExecutionState | ExecutionSubState             | ExecutionSubStateDetails | Possible error causes                                                                           |
| -------------- |------------------------------ | ------------------------ | ----------------------------------------------------------------------------------------------- |
| 3 (Failed)     | 1 (DeserializingJsonPayload)  | empty                    | Payload too large, unabled to parse JSON payload, not specified PackageManagerConfiguration     |
| 3 (Failed)     | 2 (DeserializingDesiredState) | empty                    | Invalid DesiredState payload, incorrect types specified, not specified Packages                 |
| 3 (Failed)     | 4 (DeserializingPackages)     | package_name(s)          | Packages is not an array type, invalid array element                                            |
| 3 (Failed)     | 4 (UpdatingPackagesLists)     | empty                    | Refreshing list of packages (by running `apt-get update` command) failed. A source repository may be unreachable or access may be unauthorized |
| 3 (Failed)     | 5 (InstallingPackages)        | package_name(s)          | Installation of package(s) failed because they (or any of their dependencies) weren't found in the source repositories |
| 4 (TimedOut)   | 4 (UpdatingPackagesLists)     | empty                    | Refreshing list of packages (by running `apt-get update` command) took more than 10 min         |
| 4 (TimedOut)   | 5 (InstallingPackages)        | package_name(s)          | Installing packages took more than 10 min                                                       |