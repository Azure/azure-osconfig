# Package Manager Configuration module

The Package Manager Configuration module is used to configure Debian packages and get information about their state.

## Desired state

The Package Manager Configuration module is used to update Debian packages (install, uninstall, change package version) and/or configure package repositories (sources). Package updating is done by executing `apt-get` commands (`apt-get update`, `apt-get install`) and source configuration by creating or deleting files in `/etc/apt/sources.list.d/` directory. The expected package format is the same as for `apt-get install` [command](https://linux.die.net/man/8/apt-get) and for sources the same as [sources.list format](https://wiki.debian.org/SourcesList#sources.list_format).

### Configuring GPG keys for custom package sources

The Package Manager Configuration module supports the recommended way to configure third-party repositories as documented [here](https://wiki.debian.org/DebianRepository/UseThirdParty).

The desired `gpgKeys` map allows the configuration of GPG keys by providing for each GPG-key
- a key: unique `key-id` that will be used as filename when creating/updating/deleting `/usr/share/keyrings/{key-id}.gpg`
- a value: URL where the GPG key can be downloaded from (`.asc` or `.gpg`).

The desired `sources` map allows the configuration of source repositories by providing for each source repository
- a key: unique `src-id` that will be used as filename when creating/updating/deleting `/etc/apt/sources.list.d/{src-id}.list`
- a value: the source configuration as expected by the `apt` tool with an additional `[... signed-by={key-id}]` option.

The module will automatically replace the placeholder in `signed-by={key-id}` with the actual file path where the key is located if `key-id` is used as key for any of the entries in the desired `gpgKeys` map.

### Example of desired payload
```
"PackageManager": {
    "desiredState": {
        "packages": [
            "cowsay sl=5.02-1",
            "fortune-"
        ],
        "gpgKeys": {
            "microsoft-key": "https://packages.microsoft.com/keys/microsoft.asc"
        },
        "sources": {
            "microsoft-repo": "deb [arch=amd64,arm64,armhf signed-by=microsoft-key] https://packages.microsoft.com/ubuntu/20.04/prod focal main"
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

The `executionState`, `executionSubstate` and `executionSubstateDetails` properties reflect the current state and can have the following values:

| executionState | executionSubstate             | executionSubstateDetails | Meaning                                                                                      |
| -------------- |------------------------------ | ------------------------ | -------------------------------------------------------------------------------------------- |
| 0 (Unknown)    | 0 (None)                      | empty                    | No desired properties are known to the module. This is the initial default state.            |
| 1 (Running)    | 1 (DeserializingJsonPayload)  | empty                    | Deserializing PackageManager JSON object                                        |
| 1 (Running)    | 2 (DeserializingDesiredState) | empty                    | Deserializing DesiredState JSON object                                                       |
| 1 (Running)    | 3 (DeserializingGpgKeys)      | key_id                   | Deserializing GpgKeys JSON object                                                            |
| 1 (Running)    | 4 (DeserializingSources)      | source_id                | Deserializing Sources JSON object                                                            |
| 1 (Running)    | 5 (DeserializingPackages)     | package_name(s)          | Deserializing Packages JSON array                                                            |
| 1 (Running)    | 6 (DownloadingGpgKeys)        | key_id                   | Downloading GPG keys to `/usr/share/keyrings/` directory                                     |
| 1 (Running)    | 7 (ModifyingSources)          | source_id                | Modifying (creating/updating/deleting) Sources files in `/etc/apt/sources.list.d/` directory |
| 1 (Running)    | 8 (UpdatingPackagesLists)     | empty                    | Refreshing list of packages before updating packages (by running `apt-get update` command)   |
| 1 (Running)    | 9 (InstallingPackages)        | package_name(s)          | Installing packages (by running `apt-get install` command)                                   |
| 2 (Succeeded)  | 0 (None)                      | empty                    | All desired properties were applied successfully                                             |
| 3 (Failed)     | 1 (DeserializingJsonPayload)  | empty                    | Deserializing PackageManager JSON object failed                                 |
| 3 (Failed)     | 2 (DeserializingDesiredState) | empty                    | Deserializing DesiredState JSON object failed                                                |
| 3 (Failed)     | 3 (DeserializingGpgKeys)      | key_id                   | Deserializing GpgKeys JSON object failed                                                     |
| 3 (Failed)     | 4 (DeserializingSources)      | source_id                | Deserializing Sources JSON object failed                                                     |
| 3 (Failed)     | 5 (DeserializingPackages)     | package_name(s)          | Deserializing Packages JSON array failed                                                     |
| 3 (Failed)     | 6 (DownloadingGpgKeys)        | key_id                   | Downloading GPG keys failed                                                                  |
| 3 (Failed)     | 7 (ModifyingSources)          | source_id                | Modifying Sources files failed                                                               |
| 3 (Failed)     | 8 (UpdatingPackagesLists)     | empty                    | Refreshing list of packages packages failed                                                  |
| 3 (Failed)     | 9 (InstallingPackages)        | package_name(s)          | Installing packages failed                                                                   |
| 4 (TimedOut)   | 8 (UpdatingPackagesLists)     | empty                    | Refreshing list of packages timed out                                                        |
| 4 (TimedOut)   | 9 (InstallingPackages)        | package_name(s)          | Installing packages timed out                                                                |

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
    "executionSubstate": 0,
    "executionSubstateDetails": "",
    "sourcesFingerprint": "94ed99a55df5ff3264dc0e8175d438e7219fcc216e49845cbd426895bbdbeaa9",
    "sourcesFilenames": [
        "microsoft-custom"
    ]
}
```

## Error reporting

Package Manager Configuration module can fail in different stages. To be able to identify the cause, take a closer look at `executionState`, `executionSubstate` and `executionSubstateDetails`. The following table shows failure codes and their possible causes:

| executionState | executionSubstate             | executionSubstateDetails | Possible error causes                                                                           |
| -------------- |------------------------------ | ------------------------ | ----------------------------------------------------------------------------------------------- |
| 3 (Failed)     | 1 (DeserializingJsonPayload)  | empty                    | Payload too large, unabled to parse JSON payload, not specified PackageManager     |
| 3 (Failed)     | 2 (DeserializingDesiredState) | empty                    | Invalid DesiredState payload, incorrect types specified, neither specified Sources nor Packages |
| 3 (Failed)     | 3 (DeserializingGpgKeys)      | key_id                   | GpgKeys is not a map type, invalid map value                                                    |
| 3 (Failed)     | 4 (DeserializingSources)      | source_id                | Sources is not a map type, invalid map value                                                    |
| 3 (Failed)     | 5 (DeserializingPackages)     | package_name(s)          | Packages is not an array type, invalid array element                                            |
| 3 (Failed)     | 6 (DownloadingGpgKeys)        | key_id                   | Provided URL is not accessible                                                                  |
| 3 (Failed)     | 7 (ModifyingSources)          | source_id                | Failed to delete or create source file                                                          |
| 3 (Failed)     | 8 (UpdatingPackagesLists)     | empty                    | Refreshing list of packages by running `apt-get update` failed. A source repository may be unreachable or access may be unauthorized. |
| 3 (Failed)     | 9 (InstallingPackages)        | package_name(s)          | Installation of package(s) failed because they (or any of their dependencies) weren't found in the source repositories |
| 4 (TimedOut)   | 8 (UpdatingPackagesLists)     | empty                    | Refreshing list of packages by running `apt-get update` took more than 10 min                   |
| 4 (TimedOut)   | 9 (InstallingPackages)        | package_name(s)          | Installing packages took more than 10 min                                                       |
