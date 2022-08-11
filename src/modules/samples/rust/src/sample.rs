// Copyright (c) Microsoft Corporation. All rights reserved..
// Licensed under the MIT License.

use serde::{Deserialize, Serialize};
use std::collections::HashMap;

use crate::MmiError;

const COMPONENT_NAME: &str = "SampleComponent";

const DESIRED_STRING_OBJECT_NAME: &str = "desiredStringObject";
const REPORTED_STRING_OBJECT_NAME: &str = "reportedStringObject";
const DESIRED_INTEGER_OBJECT_NAME: &str = "desiredIntegerObject";
const REPORTED_INTEGER_OBJECT_NAME: &str = "reportedIntegerObject";
const DESIRED_BOOLEAN_OBJECT_NAME: &str = "desiredBooleanObject";
const REPORTED_BOOLEAN_OBJECT_NAME: &str = "reportedBooleanObject";
const DESIRED_OBJECT_NAME: &str = "desiredObject";
const REPORTED_OBJECT_NAME: &str = "reportedObject";
const DESIRED_ARRAY_OBJECT_NAME: &str = "desiredArrayObject";
const REPORTED_ARRAY_OBJECT_NAME: &str = "reportedArrayObject";

// r# Denotes a Rust Raw String
const INFO: &str = r#"""({
    "Name": "Rust Sample",
    "Description": "A sample module written in Rust",
    "Manufacturer": "Microsoft",
    "VersionMajor": 1,
    "VersionMinor": 0,
    "VersionInfo": "",
    "Components": ["SampleComponent"],
    "Lifetime": 1,
    "UserAccount": 0})"""#;

#[derive(Serialize, Deserialize, Debug)]
enum IntegerEnumeration {
    None,
    Value1,
    Value2,
}

impl Default for IntegerEnumeration {
    fn default() -> Self {
        IntegerEnumeration::None
    }
}

// A sample object with all possible setting types
#[derive(Default, Serialize, Deserialize, Debug)]
struct Object {
    string_setting: String,
    integer_setting: i32,
    boolean_setting: bool,
    enumeration_setting: IntegerEnumeration,
    string_array_setting: Vec<String>,
    integer_array_setting: Vec<i32>,
    string_map_setting: HashMap<String, String>,
    integer_map_setting: HashMap<String, i32>,

    // Store removed elements to report as 'null'
    // These vectors must have a maximum size relative to the max payload size
    // recieved by MmiOpen()
    removed_string_map_setting_keys: Vec<String>,
    removed_integer_map_setting_keys: Vec<i32>,
}

#[derive(Default, Serialize, Deserialize, Debug)]
pub struct Sample {
    max_payload_size_bytes: u32,
    string_value: String,
    integer_value: i32,
    boolean_value: bool,
    object_value: Object,
    object_array_value: Vec<Object>,
}

impl Sample {
    pub fn new(max_payload_size_bytes: u32) -> Self {
        // The result is returned if the ending semicolon is omitted
        Sample {
            max_payload_size_bytes: max_payload_size_bytes,
            ..Default::default()
        }
    }

    pub fn get_info(_client_name: &str) -> Result<&str, MmiError> {
        // This sample module makes no use of the client_name, but
        // it may be copied, compared, etc. here
        // In the case of an error, an error code Err(i32) could be returned instead
        Ok(INFO)
    }

    pub fn set(
        &mut self,
        component_name: &str,
        object_name: &str,
        payload_str_slice: &str,
    ) -> Result<i32, MmiError> {
        if COMPONENT_NAME.eq(component_name) {
            match object_name {
                DESIRED_STRING_OBJECT_NAME => {
                    self.string_value = serde_json::from_str::<String>(payload_str_slice)?;
                    Ok(0)
                }
                DESIRED_BOOLEAN_OBJECT_NAME => {
                    self.boolean_value = serde_json::from_str::<bool>(payload_str_slice)?;
                    Ok(0)
                }
                DESIRED_INTEGER_OBJECT_NAME => {
                    self.integer_value = serde_json::from_str::<i32>(payload_str_slice)?;
                    Ok(0)
                }
                DESIRED_OBJECT_NAME => {
                    self.object_value = serde_json::from_str::<Object>(payload_str_slice)?;
                    Ok(0)
                }
                DESIRED_ARRAY_OBJECT_NAME => {
                    self.object_array_value =
                        serde_json::from_str::<Vec<Object>>(payload_str_slice)?;
                    Ok(0)
                }
                _ => Err(MmiError::InvalidArgument(format!(
                    "Invalid object name: {}",
                    object_name
                ))),
            }
        } else {
            Err(MmiError::InvalidArgument(format!(
                "Invalid component name: {}",
                component_name
            )))
        }
    }

    pub fn get(&self, component_name: &str, object_name: &str) -> Result<String, MmiError> {
        if COMPONENT_NAME.eq(component_name) {
            let json_value = match object_name {
                REPORTED_STRING_OBJECT_NAME => serde_json::to_value::<&String>(&self.string_value)?,
                REPORTED_BOOLEAN_OBJECT_NAME => serde_json::to_value::<&bool>(&self.boolean_value)?,
                REPORTED_INTEGER_OBJECT_NAME => serde_json::to_value::<&i32>(&self.integer_value)?,
                REPORTED_OBJECT_NAME => serde_json::to_value::<&Object>(&self.object_value)?,
                REPORTED_ARRAY_OBJECT_NAME => {
                    serde_json::to_value::<&Vec<Object>>(&self.object_array_value)?
                }
                _ => {
                    return Err(MmiError::InvalidArgument(format!(
                        "Invalid object name: {}",
                        object_name
                    )))
                }
            };
            Ok(serde_json::to_string(&json_value)?)
        } else {
            Err(MmiError::InvalidArgument(format!(
                "Invalid component name: {}",
                component_name
            )))
        }
    }

    #[cfg(test)]
    fn get_max_payload_size_bytes(&self) -> u32 {
        self.max_payload_size_bytes
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn build_sample() {
        let test = Sample::new(16);
        assert_eq!(test.get_max_payload_size_bytes(), 16);
    }

    #[test]
    fn info_size() {
        let sample_info_result: Result<&str, MmiError> = Sample::get_info("Test_client_name");
        assert!(sample_info_result.is_ok());
        let sample_info: &str = sample_info_result.unwrap();
        assert_eq!(INFO, sample_info);
        assert_eq!(INFO.len() as i32, sample_info.len() as i32);
    }

    #[test]
    fn invalid_get() {
        let test = Sample::new(16);

        let invalid_component_result: Result<String, MmiError> =
            test.get("Invalid component", DESIRED_STRING_OBJECT_NAME);
        assert!(!invalid_component_result.is_ok());
        match invalid_component_result {
            Err(MmiError::InvalidArgument(err)) => assert_eq!(
                err,
                String::from("Invalid component name: Invalid component")
            ),
            _ => panic!("An Invalid Argument should've been thrown"),
        }
        let invalid_object_result: Result<String, MmiError> =
            test.get(COMPONENT_NAME, "Invalid object");
        assert!(!invalid_object_result.is_ok());
        match invalid_object_result {
            Err(MmiError::InvalidArgument(err)) => {
                assert_eq!(err, String::from("Invalid object name: Invalid object"))
            }
            _ => panic!("An Invalid Argument should've been thrown"),
        }
    }
}
