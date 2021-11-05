# Configure the Microsoft Azure Provider
terraform {
  required_providers {
    azurerm = {
      source = "hashicorp/azurerm"
      version = "~>2.0"
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

# Generate random text for a unique resource group name
resource "random_id" "randomId" {
    byte_length = 8
}

# Create a resource group if it doesn't exist
resource "azurerm_resource_group" "osconfig_group" {
    name     = "${var.resource_group_name_prefix}-${random_id.randomId.hex}"
    location = "eastus"

    tags = {
        environment = var.environment_tag
    }
}

# Create an Azure IoT Hub
resource "azurerm_iothub" "osconfig_iothub" {
    name                = "osconfig-iothub-${random_id.randomId.hex}"
    resource_group_name = azurerm_resource_group.osconfig_group.name
    location            = azurerm_resource_group.osconfig_group.location

    sku {
        name     = "S1"
        capacity = 1
    }
}

# Create an IoT Hub Access Policy
data "azurerm_iothub_shared_access_policy" "osconfig_iothubowner" {
    name                = "iothubowner"
    resource_group_name = azurerm_resource_group.osconfig_group.name
    iothub_name         = azurerm_iothub.osconfig_iothub.name
}

resource "azurerm_iothub_dps" "osconfig_dps" {
    name                = "osconfig-dps-${random_id.randomId.hex}"
    resource_group_name = azurerm_resource_group.osconfig_group.name
    location            = azurerm_resource_group.osconfig_group.location

    sku {
        name     = "S1"
        capacity = 1
    }

    linked_hub {
        connection_string = data.azurerm_iothub_shared_access_policy.osconfig_iothubowner.primary_connection_string
        location = azurerm_iothub.osconfig_iothub.location
    }
}

# Add secret into keyvault
resource "azurerm_key_vault_secret" "iothub_owner_secret" {
  name         = "${azurerm_resource_group.osconfig_group.name}-iothubowner"
  value        = data.azurerm_iothub_shared_access_policy.osconfig_iothubowner.primary_connection_string
  key_vault_id = var.key_vault_id
  # expire in 30 days
  expiration_date = timeadd(timestamp(), "720h")
}
