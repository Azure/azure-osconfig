# Configure the Microsoft Azure Provider
terraform {
  required_providers {
    azurerm = {
      source = "hashicorp/azurerm"
      version = "3.0.0"
    }
  }
}
provider "azurerm" {
  features {}

  subscription_id   = var.subscription_id
  tenant_id         = var.tenant_id
  
  # Local testing - must create a SP and plumb id/secret below using scripts/create-sp-terraform.sh
  client_id         = var.client_id
  client_secret     = var.client_secret
}

# Create an Azure IoT Hub
resource "azurerm_iothub" "osconfig_iothub" {
    name                = "${var.resource_group_name}-iothub"
    resource_group_name = "${var.resource_group_name}"
    location            = "eastus"

    sku {
        name     = "S1"
        capacity = 1
    }
}

# Create an IoT Hub Access Policy
data "azurerm_iothub_shared_access_policy" "osconfig_iothubowner" {
    name                = "iothubowner"
    resource_group_name = "${var.resource_group_name}"
    iothub_name         = azurerm_iothub.osconfig_iothub.name
}

# Add secret into keyvault
resource "azurerm_key_vault_secret" "iothub_owner_secret" {
  name         = "${var.resource_group_name}-iothubowner"
  value        = data.azurerm_iothub_shared_access_policy.osconfig_iothubowner.primary_connection_string
  key_vault_id = var.key_vault_id
  # expire in 30 days
  expiration_date = timeadd(timestamp(), "720h")
}
