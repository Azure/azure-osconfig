# Create an Azure IoT Hub
resource "azurerm_iothub" "iothub" {
  name                = "iot-${var.name}"
  resource_group_name = var.resource_group_name
  location            = "eastus"

  sku {
    name     = "S1"
    capacity = 1
  }

  tags = var.tags
}

# Create an IoT Hub Access Policy
data "azurerm_iothub_shared_access_policy" "iothubowner" {
  name                = "iothubowner"
  resource_group_name = var.resource_group_name
  iothub_name         = azurerm_iothub.iothub.name
}
