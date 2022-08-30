// Copyright (c) Microsoft Corporation. All rights reserved..
// Licensed under the MIT License.

use serde::{Deserialize, Serialize};
use serde_repr::{Serialize_repr, Deserialize_repr};
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
const INFO: &str = r#"{
    "Name": "Rust Sample",
    "Description": "A sample module written in Rust",
    "Manufacturer": "Microsoft",
    "VersionMajor": 1,
    "VersionMinor": 0,
    "VersionInfo": "",
    "Components": ["SampleComponent"],
    "Lifetime": 1,
    "UserAccount": 0}"#;

type Result<T> = std::result::Result<T, MmiError>;

#[derive(Serialize_repr, Deserialize_repr, Debug, PartialEq, Eq)]
// #[repr(u8)] changes the enum to an integer enum akin to one in C
#[repr(u8)]
enum IntegerEnumeration {
    // This explicitly sets what the enum options serialize and
    // deserialize into, but they default to 0-indexed
    None = 0,
    Value1 = 1,
    Value2 = 2,
}

impl Default for IntegerEnumeration {
    fn default() -> Self {
        IntegerEnumeration::None
    }
}

#[derive(Serialize, Deserialize, Debug, PartialEq, Eq)]
// Switches the enum options to camelCase when serializing and deserializing
#[serde(rename_all = "camelCase")]
enum StringEnumeration {
    None,
    Value1,
    Value2,
}

impl Default for StringEnumeration {
    fn default() -> Self {
        StringEnumeration::None
    }
}

// A sample object with all possible setting types
#[derive(Default, Serialize, Deserialize, Debug, PartialEq, Eq)]
// Switches the field names to camelCase when serializing and deserializing
#[serde(rename_all = "camelCase")]
struct Object {
    string_setting: String,
    boolean_setting: bool,
    integer_setting: i32,
    integer_enumeration_setting: IntegerEnumeration,
    string_enumeration_setting: StringEnumeration,
    string_array_setting: Vec<String>,
    integer_array_setting: Vec<i32>,
    // Accepting option types as values accounts for null json values
    string_map_setting: HashMap<String, Option<String>>,
    integer_map_setting: HashMap<String, Option<i32>>,
}

#[derive(Default, Debug)]
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

    pub fn get_info(_client_name: &str) -> Result<&str> {
        // This sample module makes no use of the client_name, but
        // it may be copied, compared, etc. here
        // In the case of an error, an error code Err(i32) could be returned instead
        Ok(INFO)
    }

    pub fn set(
        &mut self,
        component_name: &str,
        object_name: &str,
        payload: &str,
    ) -> Result<i32> {
        if COMPONENT_NAME.eq(component_name) {
            if self.max_payload_size_bytes != 0
                && payload.len() as u32 > self.max_payload_size_bytes
            {
                println!("Payload size exceeded max payload size bytes in set");
                Err(MmiError::PayloadSizeExceeded)
            } else {
                match object_name {
                    DESIRED_STRING_OBJECT_NAME => {
                        self.string_value = serde_json::from_str::<String>(payload)?;
                        Ok(0)
                    }
                    DESIRED_BOOLEAN_OBJECT_NAME => {
                        self.boolean_value = serde_json::from_str::<bool>(payload)?;
                        Ok(0)
                    }
                    DESIRED_INTEGER_OBJECT_NAME => {
                        self.integer_value = serde_json::from_str::<i32>(payload)?;
                        Ok(0)
                    }
                    DESIRED_OBJECT_NAME => {
                        self.object_value = serde_json::from_str::<Object>(payload)?;
                        Ok(0)
                    }
                    DESIRED_ARRAY_OBJECT_NAME => {
                        self.object_array_value =
                            serde_json::from_str::<Vec<Object>>(payload)?;
                        Ok(0)
                    }
                    _ => {
                        println!("Invalid object name: {}", object_name);
                        Err(MmiError::InvalidArgument)
                    }
                }
            }
        } else {
            println!("Invalid component name: {}", component_name);
            Err(MmiError::InvalidArgument)
        }
    }

    pub fn get(&self, component_name: &str, object_name: &str) -> Result<String> {
        if COMPONENT_NAME.eq(component_name) {
            let json_value = match object_name {
                REPORTED_STRING_OBJECT_NAME => {
                    serde_json::to_value::<&String>(&self.string_value)?
                }
                REPORTED_BOOLEAN_OBJECT_NAME => {
                    serde_json::to_value::<&bool>(&self.boolean_value)?
                }
                REPORTED_INTEGER_OBJECT_NAME => {
                    serde_json::to_value::<&i32>(&self.integer_value)?
                }
                REPORTED_OBJECT_NAME => {
                    serde_json::to_value::<&Object>(&self.object_value)?
                }
                REPORTED_ARRAY_OBJECT_NAME => {
                    serde_json::to_value::<&Vec<Object>>(&self.object_array_value)?
                }
                _ => {
                    println!("Invalid object name: {}", object_name);
                    return Err(MmiError::InvalidArgument);
                }
            };
            let payload: String = serde_json::to_string(&json_value)?;
            if self.max_payload_size_bytes != 0
            && payload.len() as u32 > self.max_payload_size_bytes {
                // In other modules you could prioritize parts of the payload
                println!("Payload size exceeded max payload size bytes in get so it was truncated.");
                let payload_bytes = payload.into_bytes();
                let truncated_payload = String::from_utf8((&payload_bytes[0..self.max_payload_size_bytes as usize]).to_vec())?;
                Ok(truncated_payload)
            } else {
                Ok(payload)
            }
        } else {
            println!("Invalid component name: {}", component_name);
            Err(MmiError::InvalidArgument)
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
    const MAX_PAYLOAD_BYTES: u32 = 0;

    #[test]
    fn build_sample() {
        let sample = Sample::new(MAX_PAYLOAD_BYTES);
        assert_eq!(sample.get_max_payload_size_bytes(), MAX_PAYLOAD_BYTES);
    }

    #[test]
    fn info_size() {
        let sample_info: Result<&str> = Sample::get_info("Test_client_name");
        assert!(sample_info.is_ok());
        let sample_info: &str = sample_info.unwrap();
        assert_eq!(INFO, sample_info);
        assert_eq!(INFO.len() as i32, sample_info.len() as i32);
    }

    #[test]
    fn invalid_get() {
        let sample = Sample::new(MAX_PAYLOAD_BYTES);
        let invalid_component_result: Result<String> =
            sample.get("Invalid component", DESIRED_STRING_OBJECT_NAME);
        assert!(invalid_component_result.is_err());
        if let Err(e) = invalid_component_result {
            assert_eq!(e, MmiError::InvalidArgument);
        }
        let invalid_object_result: Result<String> =
            sample.get(COMPONENT_NAME, "Invalid object");
        assert!(invalid_object_result.is_err());
        if let Err(e) = invalid_object_result {
            assert_eq!(e, MmiError::InvalidArgument);
        }
    }

    #[test]
    fn invalid_set() {
        let mut sample = Sample::new(MAX_PAYLOAD_BYTES);
        let valid_json_payload = r#""Rust Sample Module""#;
        let invalid_json_payload = r#"Invalid payload"#;
        let invalid_component_result: Result<i32> = sample.set(
            "Invalid component",
            DESIRED_STRING_OBJECT_NAME,
            valid_json_payload,
        );
        assert!(invalid_component_result.is_err());
        if let Err(e) = invalid_component_result {
            assert_eq!(e, MmiError::InvalidArgument);
        }
        let invalid_object_result: Result<i32> =
            sample.set(COMPONENT_NAME, "Invalid object", valid_json_payload);
        assert!(invalid_object_result.is_err());
        if let Err(e) = invalid_object_result {
            assert_eq!(e, MmiError::InvalidArgument);
        }
        let invalid_payload_result: Result<i32> = sample.set(
            COMPONENT_NAME,
            DESIRED_STRING_OBJECT_NAME,
            invalid_json_payload,
        );
        assert!(invalid_payload_result.is_err());
        if let Err(e) = invalid_payload_result {
            assert_eq!(e, MmiError::SerdeError);
        }
    }

    #[test]
    fn get_set_string_object() {
        let mut sample = Sample::new(MAX_PAYLOAD_BYTES);
        let json_payload = r#""Rust Sample Module""#;
        let set_result: Result<i32> =
            sample.set(COMPONENT_NAME, DESIRED_STRING_OBJECT_NAME, json_payload);
        assert!(set_result.is_ok());
        assert_eq!(0, set_result.unwrap());
        let get_result: Result<String> =
            sample.get(COMPONENT_NAME, REPORTED_STRING_OBJECT_NAME);
        assert!(get_result.is_ok());
        let get_result: String = get_result.unwrap();
        assert!(json_strings_eq::<String>(get_result.as_str(), json_payload));
    }

    #[test]
    fn get_set_integer_object() {
        let mut sample = Sample::new(MAX_PAYLOAD_BYTES);
        let json_payload = r#"12345"#;
        let set_result: Result<i32> =
            sample.set(COMPONENT_NAME, DESIRED_INTEGER_OBJECT_NAME, json_payload);
        assert!(set_result.is_ok());
        assert_eq!(0, set_result.unwrap());
        let get_result: Result<String> =
            sample.get(COMPONENT_NAME, REPORTED_INTEGER_OBJECT_NAME);
        assert!(get_result.is_ok());
        let get_result: String = get_result.unwrap();
        assert!(json_strings_eq::<i32>(get_result.as_str(), json_payload));
    }

    #[test]
    fn get_set_boolean_object() {
        let mut sample = Sample::new(MAX_PAYLOAD_BYTES);
        let json_payload = r#"true"#;
        let set_result: Result<i32> =
            sample.set(COMPONENT_NAME, DESIRED_BOOLEAN_OBJECT_NAME, json_payload);
        assert!(set_result.is_ok());
        assert_eq!(0, set_result.unwrap());
        let get_result: Result<String> =
            sample.get(COMPONENT_NAME, REPORTED_BOOLEAN_OBJECT_NAME);
        assert!(get_result.is_ok());
        let get_result: String = get_result.unwrap();
        assert!(json_strings_eq::<bool>(get_result.as_str(), json_payload));
    }

    #[test]
    fn get_set_object() {
        let mut sample = Sample::new(MAX_PAYLOAD_BYTES);
        let json_payload = "{\
                \"stringSetting\":\"C++ Sample Module\",\
                \"booleanSetting\":true,\
                \"integerSetting\":12345,\
                \"integerEnumerationSetting\":0,\
                \"stringEnumerationSetting\":\"value1\",\
                \"stringArraySetting\":[\"C++ Sample Module 1\",\"C++ Sample Module 2\"],\
                \"integerArraySetting\":[1,2,3,4,5],\
                \"stringMapSetting\":{\
                    \"key1\":\"C++ Sample Module 1\",\
                    \"key2\":\"C++ Sample Module 2\"\
                },\
                \"integerMapSetting\":{\
                    \"key1\":1,\
                    \"key2\":2\
                }\
            }";
        let set_result: Result<i32> =
            sample.set(COMPONENT_NAME, DESIRED_OBJECT_NAME, json_payload);
        assert!(set_result.is_ok());

        assert_eq!(0, set_result.unwrap());
        let get_result: Result<String> = sample.get(COMPONENT_NAME, REPORTED_OBJECT_NAME);
        assert!(get_result.is_ok());
        let get_result: String = get_result.unwrap();
        assert!(json_strings_eq::<Object>(get_result.as_str(), json_payload));
    }

    #[test]
    fn get_set_object_map_null_values() {
        let mut sample = Sample::new(MAX_PAYLOAD_BYTES);
        let json_payload = "{\
                \"stringSetting\":\"C++ Sample Module\",\
                \"booleanSetting\":true,\
                \"integerSetting\":12345,\
                \"integerEnumerationSetting\":0,\
                \"stringEnumerationSetting\":\"value1\",\
                \"stringArraySetting\":[\"C++ Sample Module 1\",\"C++ Sample Module 2\"],\
                \"integerArraySetting\":[1,2,3,4,5],\
                \"stringMapSetting\":{\
                    \"key1\":\"C++ Sample Module 1\",\
                    \"key2\":null\
                },\
                \"integerMapSetting\":{\
                    \"key1\":1,\
                    \"key2\":null\
                }\
            }";
        let set_result: Result<i32> =
            sample.set(COMPONENT_NAME, DESIRED_OBJECT_NAME, json_payload);
        assert!(set_result.is_ok());
        assert_eq!(0, set_result.unwrap());
        let get_result: Result<String> = sample.get(COMPONENT_NAME, REPORTED_OBJECT_NAME);
        assert!(get_result.is_ok());
        let get_result: String = get_result.unwrap();
        assert!(json_strings_eq::<Object>(get_result.as_str(), json_payload));
    }

    #[test]
    fn get_set_array_object() {
        let mut sample = Sample::new(MAX_PAYLOAD_BYTES);
        let json_payload = "[\
                {\
                    \"stringSetting\":\"C++ Sample Module\",\
                    \"booleanSetting\":true,\
                    \"integerSetting\":12345,\
                    \"integerEnumerationSetting\":0,\
                    \"stringEnumerationSetting\":\"value1\",\
                    \"stringArraySetting\":[\"C++ Sample Module 1\",\"C++ Sample Module 2\"],\
                    \"integerArraySetting\":[1,2,3,4,5],\
                    \"stringMapSetting\":{\
                        \"key1\":\"C++ Sample Module 1\",\
                        \"key2\":null\
                    },\
                    \"integerMapSetting\":{\
                        \"key1\":1,\
                        \"key2\":null\
                    }\
                }\
            ]";
        let set_result: Result<i32> =
            sample.set(COMPONENT_NAME, DESIRED_ARRAY_OBJECT_NAME, json_payload);
        assert!(set_result.is_ok());
        assert_eq!(0, set_result.unwrap());
        let get_result: Result<String> =
            sample.get(COMPONENT_NAME, REPORTED_ARRAY_OBJECT_NAME);
        assert!(get_result.is_ok());
        let get_result: String = get_result.unwrap();
        assert!(json_strings_eq::<Vec<Object>>(
            get_result.as_str(),
            json_payload
        ));
    }

    fn json_strings_eq<'a, Deserializable: Deserialize<'a> + PartialEq + Eq>(
        json_str_one: &'a str,
        json_str_two: &'a str,
    ) -> bool {
        let json_one = serde_json::from_str::<Deserializable>(json_str_one).unwrap();
        let json_two = serde_json::from_str::<Deserializable>(json_str_two).unwrap();
        json_one == json_two
    }
}
