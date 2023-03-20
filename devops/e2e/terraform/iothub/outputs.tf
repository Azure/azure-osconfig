output "iothub_name" {
  value = azurerm_iothub.iothub.name
}

output "connection_string" {
  value     = data.azurerm_iothub_shared_access_policy.iothubowner.primary_connection_string
  sensitive = true
}
