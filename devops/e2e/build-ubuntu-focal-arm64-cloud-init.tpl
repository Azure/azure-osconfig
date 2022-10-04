#cloud-config
apt_sources:
  - msftprod:
    source: deb [arch=arm64] https://packages.microsoft.com/ubuntu/20.04/prod/ $RELEASE main
    key: |
      -----BEGIN PGP PUBLIC KEY BLOCK-----
      Version: GnuPG v1.4.7 (GNU/Linux)

      mQENBFYxWIwBCADAKoZhZlJxGNGWzqV+1OG1xiQeoowKhssGAKvd+buXCGISZJwT
      LXZqIcIiLP7pqdcZWtE9bSc7yBY2MalDp9Liu0KekywQ6VVX1T72NPf5Ev6x6DLV
      7aVWsCzUAF+eb7DC9fPuFLEdxmOEYoPjzrQ7cCnSV4JQxAqhU4T6OjbvRazGl3ag
      OeizPXmRljMtUUttHQZnRhtlzkmwIrUivbfFPD+fEoHJ1+uIdfOzZX8/oKHKLe2j
      H632kvsNzJFlROVvGLYAk2WRcLu+RjjggixhwiB+Mu/A8Tf4V6b+YppS44q8EvVr
      M+QvY7LNSOffSO6Slsy9oisGTdfE39nC7pVRABEBAAG0N01pY3Jvc29mdCAoUmVs
      ZWFzZSBzaWduaW5nKSA8Z3Bnc2VjdXJpdHlAbWljcm9zb2Z0LmNvbT6JATUEEwEC
      AB8FAlYxWIwCGwMGCwkIBwMCBBUCCAMDFgIBAh4BAheAAAoJEOs+lK2+EinPGpsH
      /32vKy29Hg51H9dfFJMx0/a/F+5vKeCeVqimvyTM04C+XENNuSbYZ3eRPHGHFLqe
      MNGxsfb7C7ZxEeW7J/vSzRgHxm7ZvESisUYRFq2sgkJ+HFERNrqfci45bdhmrUsy
      7SWw9ybxdFOkuQoyKD3tBmiGfONQMlBaOMWdAsic965rvJsd5zYaZZFI1UwTkFXV
      KJt3bp3Ngn1vEYXwijGTa+FXz6GLHueJwF0I7ug34DgUkAFvAs8Hacr2DRYxL5RJ
      XdNgj4Jd2/g6T9InmWT0hASljur+dJnzNiNCkbn9KbX7J/qK1IbR8y560yRmFsU+
      NdCFTW7wY0Fb1fWJ+/KTsC4=
      =J6gs
      -----END PGP PUBLIC KEY BLOCK-----
  - kitware:
    source: deb [arch=arm64] https://apt.kitware.com/ubuntu/ $RELEASE main
    key: |
      -----BEGIN PGP PUBLIC KEY BLOCK-----

      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
      =cUKW
      -----END PGP PUBLIC KEY BLOCK-----
package_upgrade: true
packages:
  - apt-transport-https
  - azure-cli
  - bc
  - build-essential
  - ca-certificates
  - cmake
  - curl
  - gcovr
  - gnupg
  - jo
  - jq
  - libcurl4-openssl-dev
  - libgmock-dev
  - libgtest-dev
  - libssl-dev
  - lsb-release
  - ninja-build
  - python-is-python3
  - python3
  - python3-jsonschema
  - rapidjson-dev
  - sysstat
  - uuid-dev
  - wget
# .NET Dependencies
  - libc6
  - libgcc1
  - libgssapi-krb5-2
  - libicu66
  - libssl1.1
  - libstdc++6
  - zlib1g
runcmd:
  # Install aziot-identity-service - no arm packages available
  - wget https://github.com/Azure/azure-iotedge/releases/download/1.2.10/aziot-identity-service_1.2.6-1_ubuntu20.04_arm64.deb -O aziot-identity-service.deb
  - apt install -y ./aziot-identity-service.deb
  # Install .NET Core SDK
  - wget https://download.visualstudio.microsoft.com/download/pr/06c4ee8e-bf2c-4e46-ab1c-e14dd72311c1/f7bc6c9677eaccadd1d0e76c55d361ea/dotnet-sdk-6.0.301-linux-arm64.tar.gz -O dotnet-sdk-6.0.tar.gz
  - DOTNET_FILE=dotnet-sdk-6.0.tar.gz
  - export DOTNET_ROOT=/opt/.dotnet
  - mkdir -p "$DOTNET_ROOT" && tar zxf "$DOTNET_FILE" -C "$DOTNET_ROOT"
  - ln -s /opt/.dotnet/dotnet /usr/bin/dotnet
  # Install GitHub Actions Runner
  - mkdir actions-runner && cd actions-runner && curl -o runner.tar.gz -L ${github_runner_tar_gz_package} && tar xzf ./runner.tar.gz
  - export RUNNER_ALLOW_RUNASROOT=\"1\"
  - ./config.sh --url https://github.com/Azure/azure-osconfig --unattended --ephemeral --name "${resource_group_name}-${vm_name}" --token "${runner_token}" --labels "${resource_group_name}-${vm_name}"
  - ./svc.sh install
  - ./svc.sh start