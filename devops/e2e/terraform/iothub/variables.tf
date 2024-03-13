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
