#cloud-config
apt_update: true
package_upgrade: true
runcmd:
  # Install GitHub Actions Runner
  - mkdir actions-runner && cd actions-runner && curl -o runner.tar.gz -L ${github_runner_tar_gz_package} && tar xzf ./runner.tar.gz
  - export RUNNER_ALLOW_RUNASROOT="1"
  - ./config.sh --url https://github.com/Azure/azure-osconfig --unattended --ephemeral --name "${resource_group_name}-${vm_name}" --token "${runner_token}" --labels "${resource_group_name}-${vm_name}"
  - ./svc.sh install
  - ./svc.sh start