Scripts in this directory allows to run
[microsoft/AttackSurfaceAnalyzer](https://github.com/microsoft/AttackSurfaceAnalyzer)
scans for
[Azure/azure-osconfig](https://github.com/Azure/azure-osconfig)

Scans are running in the container.
`podman` or `docker` on host system is needed to run the script.
The Default is `docker`, can be configured in [run_asa.sh](run_asa.sh).

Scans will run automatically in all supported distros, for now Ubuntu 22.04 and CentOS 8.

## How to run:

`./run_asa.sh -p https://github.com/Azure/azure-osconfig/releases/download/test_policy_package/AzureLinuxBaseline.zip`

Run `./run_asa.sh` without any options to see the possible options.

Script will fail if ASA scan detect any findings with severity equal to error
(by default, can be configured using `-fo` option)

The `./report/${DISTRO}` directories for every distro the scans were run for will be created and following
output files will be put there:
* ./report/${DISTRO}/after_audit - ASA report after audit, together with related osconfig_nrp.log
* ./report/${DISTRO}/after_remediation - ASA report after remediation, together with related osconfig_nrp.log
* ./report/${DISTRO}/asa.log - output log from all of the ASA tool collect and report generation runs

When running in the CI system `sha256` of policy package should be checked using `-c` option due to security concerns:

```
./run_asa.sh \
-p https://github.com/Azure/azure-osconfig/releases/download/test_policy_package/AzureLinuxBaseline.zip \
-c 2ad11f6869459f2f6412428172ee167cb0b77f1e397b0fc549dc47339840fd85
```

## Known constrains:
* For now container image does not include systemd so the service related functionality and related ASA scans are omitted
* ASA scan does not run TPM checks
* Scans are now executed on Ubuntu 22.04 and CentOS 8

## Planned TODO:
* Run as a part of the GitHub Actions workflow
