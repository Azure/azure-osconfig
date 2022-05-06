# Local testing - must create a SP and plumb client_id/client_secret below using scripts/create-sp-terraform.sh
variable "client_id" {}
variable "client_secret" {}
variable "subscription_id" {}
variable "tenant_id" {}
variable "key_vault_id" {}
variable resource_group_name_prefix {
    default = "osconfig"
}
variable "runner_token" {}

# Provided by iothub.tf outputs
variable resource_group_name {}

variable "vm_name" {
    default = "test-device"
}

variable "image_publisher" {
    default = "Canonical"
}

variable "image_offer" {
    default = "UbuntuServer"
}

variable "image_sku" {
    default = "18.04-LTS"
}

variable "image_version" {
    default = "latest"
}

variable "environment_tag" {
    default = "osconfig-e2etest"
}

variable "vm_script" {
    default = ""
}

variable "github_runner_tar_gz_package" {
    default = "https://github.com/actions/runner/releases/download/v2.290.1/actions-runner-linux-x64-2.290.1.tar.gz"
}