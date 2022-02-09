# OSConfig DevOps
This folder contains various folders in use by OSConfig's Engineering Systems (Build/Packaging/Test Infrastructure). This readme serves to describe the various components as they have been created with reusability in mind.

 * debian
 * docker
 * pipeline
 * scripts
 * terraform

# Containerized Environments
OSConfig's engineering systems make use of containers for building, unit testing and packaging. The container environments are located under the `docker` directory naming convention is as follows `[distro]-[architecture]`. The container environments contain everything needed to build/test/package OSConfig.

### Supported Environments
* Debian 9
* Ubuntu 18.04/20.04 (LTS)

### Supported Architectures
* amd64 (native)
* armhf (QEMU)
* arm64 (QEMU)

The container environments are meant to be run on amd64 hosts, foreign architectures are achieved by using QEMU system emulation by making use of [multiarch/qemu-user-static](https://github.com/multiarch/qemu-user-static) base images to target the ARM-variants.

### Ex. Building a Ubuntu 20.04 amd64 (Native) container image
`docker build devops/docker/ubuntu20.04-amd64 -t osconfig-ubuntu2004-amd64`

This builds a Ubuntu 20.04 amd64 variant and tags the image with _osconfig-ubuntu2004-amd64_

# Build Systems and Pipelines
OSConfig makes use of [Azure Pipelines](https://azure.microsoft.com/services/devops/pipelines/) for many tasks inclufing building OSConfig's binaries, unit tests, end-to-end tests amongst others. All of OSConfig's pipelines are contained under `devops/pipelines`.

 * [binary_package.yaml](https://github.com/Azure/azure-osconfig/blob/main/devops/pipeline/binary_package.yaml) - Produces Debian and source packages and also runs unit-tests
 * [cleanup_e2etest_infra.yaml](https://github.com/Azure/azure-osconfig/blob/main/devops/pipeline/cleanup_e2etest_infra.yaml) - Cleans up end-to-end test infrastructure
 * [create_containers.yaml](https://github.com/Azure/azure-osconfig/blob/main/devops/pipeline/create_containers.yaml) - Creates the container environments and pushes the container to your image repository
 * [e2etest.yaml](https://github.com/Azure/azure-osconfig/blob/main/devops/pipeline/e2etest.yaml) - Performs end-to-end testing of OSConfig (Setting up IoTHub, DPS, OSConfig Hosts)
 * [source_packaging.yaml](https://github.com/Azure/azure-osconfig/blob/main/devops/pipeline/source_packaging.yaml) - Creates a source package and runs unit-tests

## Creating your own pipeline for packaging/testing
In order to setup pipelines, the following is prerequisites are necessary.

### Prerequisites
 * [Azure Pipelines](https://azure.microsoft.com/services/devops/pipelines/) instance
 * [Azure Container Repository (ACR)](https://azure.microsoft.com/services/container-registry/) - or any other container repository

### Creating the Service Connection
The pipelines need to authenticate against the container repository, this is done by [creating a service connection](https://docs.microsoft.com/azure/devops/pipelines/library/service-endpoints?view=azure-devops&tabs=yaml#create-a-service-connection) to a [Docker Registry or others](https://docs.microsoft.com/azure/devops/pipelines/library/service-endpoints?view=azure-devops&tabs=yaml#docker-hub-or-others).

### Creating the Pipelines
In order for packaging/testing to be automated, only two pipelines are required to be onboarded (See [Create your first pipeline](https://docs.microsoft.com/azure/devops/pipelines/create-first-pipeline?view=azure-devops) for more info):

 * [create_containers.yaml](https://github.com/Azure/azure-osconfig/blob/main/devops/pipeline/create_containers.yaml)
 * [binary_package.yaml](https://github.com/Azure/azure-osconfig/blob/main/devops/pipeline/binary_package.yaml)

 Both pipelines require the following variables in order to work against your service connection and container repository. Make sure the `SERVICE_CONNECTION` points to the repository (case-sensitive) service connection created above and the `CONTAINER_REGISTRY` URI for the image repository.

 ```
variables:
  SERVICE_CONNECTION: OSConfig-ACR
  CONTAINER_REGISTRY: osconfig.azurecr.io
 ```

### Define pipeline variables
Only two variables need to be defined before running the binary_package pipeline, these can be defined at the pipeline settings UI (See [Set secret variables](https://docs.microsoft.com/en-us/azure/devops/pipelines/process/variables?view=azure-devops&tabs=yaml#secret-variables)). You may give any version you wish here, but they are required to be defined.

 * MajorVersion
 * MinorVersion
 * PatchVersion

Once the pipelines are setup, you are required to first run the _create_containers.yaml_ pipeline which will create all the containers and push them to your image repository. Once the pipeline completes (ARM-variants take a few hours to build) the containers will then be available for consumption by the _binary_package.yaml_ pipeline.

All of the Debian packages and unit test logs are available as [Build artifacts](https://docs.microsoft.com/en-us/azure/devops/pipelines/artifacts/build-artifacts?view=azure-devops&tabs=yaml) in the pipeline.