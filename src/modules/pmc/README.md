# Package Manager Configuration module

The Package Manager Configuration module is used to configure Debian packages and get information about their state.

## Desired state

The Package Manager Configuration module is used to update Debian packages (install, uninstall, change package version) and/or configure package repositories (sources). Package updating is done by executing `apt-get` commands (`apt-get update`, `apt-get install`) and source configuration by creating or deleting files in `/etc/apt/sources.list.d/` directory. The expected package format is the same as for `apt-get install` [command](https://linux.die.net/man/8/apt-get) and for sources the same as [sources.list format](https://wiki.debian.org/SourcesList#sources.list_format).

### Example of desired payload:

```
"PackageManagerConfiguration": {
    "desiredState": {
        "packages": [
            "cowsay sl=5.02-1",
            "fortune-"
        ],
        "sources": {
            "microsoft-custom": "deb [arch=amd64,arm64,armhf] https://packages.microsoft.com/ubuntu/18.04/multiarch/prod bionic main"
        }
    }
}
```

## Reported state

The module reports the current configuration of packages and sources on a device and the last execution state.

The configuration of the packages is reflected by reporting the versions of specified desired packages (retrieved by running `apt-cache policy _package-name_ | grep Installed`) and the fingerprint of all the installed packages (computed by executing `dpkg-query --showformat='${Package} (=${Version})\n' --show | sha256sum | head -c 64`).

The state of the sources  is reported by listing all files from `/etc/apt/sources.list.d/` and their fingerprint (computed by executing `find /etc/apt/sources.list.d/ -type f -name '*.list' -exec cat {} \; | sha256sum | head -c 64`).

*Important:*
Only versions of the packages that were last set as desired are reported.
When changing the desired packages, the previously modified packages won't be reported anymore, even though they might still be installed on the system.

The `executionState`, `executionSubState` and `executionSubStateDetails` properties reflect the current state and can have the following values:

| executionState | executionSubState             | executionSubStateDetails | Meaning                                                                                      |
| -------------- |------------------------------ | ------------------------ | -------------------------------------------------------------------------------------------- |
| 0 (unknown)    | 0 (None)                      | empty                    | No desired properties are known to the module. This is the initial default state.            |
| 1 (running)    | 1 (deserializingJsonPayload)  | empty                    | Deserializing PackageManagerConfiguration JSON object                                        |
| 1 (running)    | 2 (deserializingDesiredState) | empty                    | Deserializing DesiredState JSON object                                                       |
| 1 (running)    | 3 (deserializingSources)      | source_name              | Deserializing Sources JSON object                                                            |
| 1 (running)    | 4 (deserializingPackages)     | package_name(s)          | Deserializing Packages JSON array                                                            |
| 1 (running)    | 5 (modifyingSources)          | source_name              | Modifying (creating/updating/deleting) Sources files in `/etc/apt/sources.list.d/` directory |
| 1 (running)    | 6 (updatingPackageLists)     | empty                    | Refreshing list of packages before updating packages (by running `apt-get update` command)   |
| 1 (running)    | 7 (installingPackages)        | package_name(s)          | Installing packages (by running `apt-get install` command)                                   |
| 2 (succeeded)  | 0 (none)                      | empty                    | All desired properties were applied successfully                                             |
| 3 (failed)     | 1 (deserializingJsonPayload)  | empty                    | Deserializing PackageManagerConfiguration JSON object failed                                 |
| 3 (failed)     | 2 (deserializingDesiredState) | empty                    | Deserializing DesiredState JSON object failed                                                |
| 3 (failed)     | 3 (deserializingSources)      | source_name              | Deserializing Sources JSON object failed                                                     |
| 3 (failed)     | 4 (deserializingPackages)     | package_name(s)          | Deserializing Packages JSON array failed                                                     |
| 3 (failed)     | 5 (modifyingSources)          | source_name              | Modifying Sources files failed                                                               |
| 3 (failed)     | 6 (updatingPackageLists)     | empty                    | Refreshing list of packages packages failed                                                  |
| 3 (failed)     | 7 (installingPackages)        | package_name(s)          | Installing packages failed                                                                   |
| 4 (timedOut)   | 6 (updatingPackageLists)     | empty                    | Refreshing list of packages timed out                                                        |
| 4 (timedOut)   | 7 (installingPackages)        | package_name(s)          | Installing packages timed out                                                                |

### Example of reported payload:

```
"state": {
    "packagesFingerprint": "ccdd8d47438ce513a6043276c5efd15b52de35c1f80c88dff312a5d8c19b16ea",
    "packages": [
        "cowsay=3.03+dfsg2-7",
        "sl=5.02-1",
        "fortune=(none)"
    ],
    "executionState": 2,
    "executionSubState": 0,
    "executionSubStateDetails": "",
    "sourcesFingerprint": "94ed99a55df5ff3264dc0e8175d438e7219fcc216e49845cbd426895bbdbeaa9",
    "sourcesFilenames": [
        "microsoft-custom"
    ]
}
```

## Error reporting

Package Manager Configuration module can fail in different stages. To be able to identify the cause, take a closer look at `executionState`, `executionSubState` and `executionSubStateDetails`. The following table shows failure codes and their possible causes:

| executionState | executionSubState             | executionSubStateDetails | Possible error causes                                                                           |
| -------------- |------------------------------ | ------------------------ | ----------------------------------------------------------------------------------------------- |
| 3 (failed)     | 1 (deserializingJsonPayload)  | empty                    | Payload too large, unabled to parse JSON payload, not specified PackageManagerConfiguration     |
| 3 (failed)     | 2 (deserializingDesiredState) | empty                    | Invalid DesiredState payload, incorrect types specified, not specified Packages                 |
| 3 (failed)     | 3 (deserializingSources)      | source_name              | Sources is not a map type, invalid map value                                                    |
| 3 (failed)     | 4 (deserializingPackages)     | package_name(s)          | Packages is not an array type, invalid array element                                            |
| 3 (failed)     | 5 (modifyingSources)          | source_name              | Failed to delete or create source file                                                          |
| 3 (failed)     | 4 (updatingPackageLists)     | empty                    | Refreshing list of packages (by running `apt-get update` command) failed. A source repository may be unreachable or access may be unauthorized |
| 3 (failed)     | 5 (installingPackages)        | package_name(s)          | Installation of package(s) failed because they (or any of their dependencies) weren't found in the source repositories |
| 4 (timedOut)   | 4 (updatingPackageLists)     | empty                    | Refreshing list of packages (by running `apt-get update` command) took more than 1 min         |
| 4 (timedOut)   | 5 (installingPackages)        | package_name(s)          | Installing packages took more than 10 min                                                       |
