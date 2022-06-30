#cloud-config
package_upgrade: true
packages:
  - apt-transport-https
  - azure-cli
  - bc
  - ca-certificates
  - curl
  - gnupg
  - jo
  - jq
  - lsb-release
  - sysstat
# .NET Dependencies
  - libc6
  - libgcc1
  - libgssapi-krb5-2
  - libicu66
  - libssl1.1
  - libstdc++6
  - zlib1g
write_files:
  - path: /etc/sudoers.d/dotnet
    permissions: "0644"
    content: |
      Defaults secure_path="/opt/.dotnet"
    owner: root:root
runcmd:
  - curl https://packages.microsoft.com/keys/microsoft.asc | gpg --dearmor > microsoft.gpg
  - cp ./microsoft.gpg /etc/apt/trusted.gpg.d/
  # Install aziot-identity-service - no arm packages available
  - wget https://github.com/Azure/azure-iotedge/releases/download/1.2.10/aziot-identity-service_1.2.6-1_ubuntu20.04_arm64.deb -O aziot-identity-service.deb
  - apt install -y ./aziot-identity-service.deb
  # Install .NET Core SDK
  - wget https://download.visualstudio.microsoft.com/download/pr/06c4ee8e-bf2c-4e46-ab1c-e14dd72311c1/f7bc6c9677eaccadd1d0e76c55d361ea/dotnet-sdk-6.0.301-linux-arm64.tar.gz -O dotnet-sdk-6.0.tar.gz
  - DOTNET_FILE=dotnet-sdk-6.0.tar.gz
  - export DOTNET_ROOT=/opt/.dotnet
  - mkdir -p "$DOTNET_ROOT" && tar zxf "$DOTNET_FILE" -C "$DOTNET_ROOT"
  - export PATH=$PATH:$DOTNET_ROOT
  # Install Azure CLI
  - curl https://packages.microsoft.com/config/ubuntu/20.04/prod.list > ./microsoft-prod.list
  - cp ./microsoft-prod.list /etc/apt/sources.list.d/
  - apt update -y && apt install -y 
  # Install GitHub Actions Runner
  - mkdir actions-runner && cd actions-runner && curl -o runner.tar.gz -L ${github_runner_tar_gz_package} && tar xzf ./runner.tar.gz
  - export RUNNER_ALLOW_RUNASROOT=\"1\"
  - ./config.sh --url https://github.com/Azure/azure-osconfig --unattended --ephemeral --name "${resource_group_name}-${vm_name}" --token "${runner_token}" --labels "${resource_group_name}-${vm_name}"
  - ./svc.sh install
  - ./svc.sh start