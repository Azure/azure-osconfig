# Setup development environment for OSConfig
# Creates an image, pre-provisioned to check the code into /AzOsConfig
FROM osconfig.azurecr.io/ubuntu20.04-dev-amd64
ENV DEBIAN_FRONTEND=noninteractive
WORKDIR /AzOsConfig
RUN apt install -y gdb