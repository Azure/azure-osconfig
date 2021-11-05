# Local testing - must create a SP and plumb client_id/client_secret below using scripts/create-sp-terraform.sh
variable "client_id" {}
variable "client_secret" {}
variable "subscription_id" {}
variable "tenant_id" {}
variable "key_vault_id" {}
variable "sas_token" {}
variable "osconfig_package_path" {}

# Provided by iothub.tf outputs
variable "device_identity_connstr" {}
variable resource_group_name {}

variable "vm_name" {
    default = "test-device"
}

variable "upload_url" {
    default = "https://osconfige2elogs.blob.core.windows.net/logs/"
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

variable "vm_pre_osconfig_install_script" {
    default = ""
}

variable "vm_post_osconfig_install_script" {
    default = ""
}

# TODO: List of env variables to have on VM