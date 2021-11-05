output "iothubowner_connection_string" {
    value = data.azurerm_iothub_shared_access_policy.osconfig_iothubowner.primary_connection_string
    sensitive = true
}

output "resource_group_name" {
    value = azurerm_resource_group.osconfig_group.name
}

output "iothub_name" {
    value = azurerm_iothub.osconfig_iothub.name
}