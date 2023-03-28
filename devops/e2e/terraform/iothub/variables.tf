variable "client_id" {
  type        = string
  sensitive   = true
  description = "Client ID for the service principal"
}

variable "client_secret" {
  type        = string
  sensitive   = true
  description = "Client secret for the service principal"
}

variable "subscription_id" {
  type      = string
  sensitive = true
}

variable "tenant_id" {
  type      = string
  sensitive = true
}

variable "name" {
  type        = string
  description = "The name of the Azure IoT Hub"
}

variable "resource_group_name" {
  type        = string
  description = "Resource group name where the IoT Hub will be created"
  default     = "e2e"
}

variable "tags" {
  type        = map(string)
  description = "Tags to be applied to the IoT Hub"
  default     = {}
}
