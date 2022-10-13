#cloud-config
apt_update: true
package_upgrade: true
runcmd:
  # Install aziot-identity-service
  - wget https://github.com/Azure/azure-iotedge/releases/download/1.4.1/aziot-identity-service_1.4.1-1_debian10_amd64.deb -O aziot-identity-service.deb
  - apt install -y ./aziot-identity-service.deb
  # Install GitHub Actions Runner
  - mkdir actions-runner && cd actions-runner && curl -o runner.tar.gz -L ${github_runner_tar_gz_package} && tar xzf ./runner.tar.gz
  - export RUNNER_ALLOW_RUNASROOT="1"
  - ./config.sh --url https://github.com/Azure/azure-osconfig --unattended --ephemeral --name "${resource_group_name}-${vm_name}" --token "${runner_token}" --labels "${resource_group_name}-${vm_name}"
  - ./svc.sh install
  - ./svc.sh start