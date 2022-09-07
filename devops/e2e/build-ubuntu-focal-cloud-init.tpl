#cloud-config
apt_sources:
  - azcli:
    source: deb [arch=amd64] https://packages.microsoft.com/repos/azure-cli/ $RELEASE main
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
  - msftprod:
    source: deb [arch=amd64] https://packages.microsoft.com/ubuntu/20.04/prod $RELEASE main
package_upgrade: true
packages:
  - apt-transport-https
  - azure-cli
  - bc
  - build-essential
  - ca-certificates
  - cmake
  - curl
  - dotnet-sdk-6.0
  - gcovr
  - gnupg
  - jo
  - jq
  - libcurl4-openssl-dev
  - libgmock-dev
  - libgtest-dev
  - liblttng-ust-dev
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
runcmd:
  # Install GitHub Actions Runner
  - mkdir actions-runner && cd actions-runner && curl -o runner.tar.gz -L ${github_runner_tar_gz_package} && tar xzf ./runner.tar.gz
  - export RUNNER_ALLOW_RUNASROOT="1"
  - ./config.sh --url https://github.com/Azure/azure-osconfig --unattended --ephemeral --name "${resource_group_name}-${vm_name}" --token "${runner_token}" --labels "${resource_group_name}-${vm_name}"
  - ./svc.sh install
  - ./svc.sh start