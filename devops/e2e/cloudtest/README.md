# Cloud Test Directory
This directory contains 1ES Managed Image definitions for the distros being exercised by our end-to-end tests. If the distro is EOL, see [Universal NRP End-to-End Tests Local/GitHub](../../src/tests/universal-nrp-e2e/README.md) for more information.

# General Guidance for onboarding images

Its best to use a pre-existing Azure Marketplace images as they are updated regularly but some EOL distros might require [local testing](../../src/tests/universal-nrp-e2e/README.md).

## Dependencies

The images defined here are used as VM definitions to be used in Azure infrastructure where we use the VMs as test environments for testing azure-osconfig. The following are requirements for every linux image [Preparing Linux for imaging in Azure](https://learn.microsoft.com/azure/virtual-machines/linux/create-upload-generic) with specific distributions having other set of instructions, see the url provided.

In addition to performing distro specific setup for [Preparing Linux for imaging in Azure](https://learn.microsoft.com/azure/virtual-machines/linux/create-upload-generic) the images must also have the following:

 - [NodeJS v20+](https://github.com/nodejs/node) - Needed by GitHub runners
 - [GLIBCXX_3.4.21+](https://github.com/gcc-mirror/gcc/tree/releases/gcc-7.3.0) - Needed by CloudTest runtime (GCC 7.3+)
 - [PowerShell](https://github.com/PowerShell/PowerShell)
 - [OMI](https://github.com/microsoft/omi/)

 # Azure Helpers

 The PowerShell module [AzureInfrastructureHelpers.psm1](./AzureInfrastructureHelpers.psm1) provides the `Create-AzureManagedDisk` PowerShell cmdlet which helps onboard a generalized Hyper-V Linux VHD into an Azure Compute Gallery. See the internal documentation for full onboarding instructions.