# Local testing - must create a SP and plumb client_id/client_secret below using scripts/create-sp-terraform.sh
variable "client_id" {}
variable "client_secret" {}
variable "subscription_id" {}
variable "tenant_id" {}
variable "key_vault_id" {}

variable resource_group_name_prefix {
    default = "osconfig"
}

variable "environment_tag" {
    default = "osconfig-e2etest"
}