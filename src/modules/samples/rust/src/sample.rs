// Copyright (c) Microsoft Corporation. All rights reserved..
// Licensed under the MIT License.

use std::collections::HashMap;

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

const STRING_SETTING_NAME: &str = "stringSetting";
const INTEGER_SETTING_NAME: &str = "integerSetting";
const BOOLEAN_SETTING_NAME: &str = "booleanSetting";
const INTEGER_ENUMERATION_SETTING_NAME: &str = "integerEnumerationSetting";
const STRING_ARRAY_SETTING_NAME: &str = "stringsArraySetting";
const INTEGER_ARRAY_SETTING_NAME: &str = "integerArraySetting";
const STRING_MAP_SETTING_NAME: &str = "stringMapSetting";
const INTEGER_MAP_SETTING_NAME: &str = "integerMapSetting";

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

enum IntegerEnumeration {
    None,
    Value1,
    Value2,
}

// A sample object with all possible setting types
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

pub struct Sample {
    max_payload_size_bytes: u32,
}

impl Sample {
    pub fn new(max_payload_size_bytes: u32) -> Self {
        // The result is returned if the ending semicolon is omitted
        Sample {
            max_payload_size_bytes: max_payload_size_bytes,
        }
    }

    pub fn get_info(client_name: &str) -> Result<&str, i32> {
        // This sample module makes no use of the client_name, but
        // it may be copied, compared, etc. here
        // In the case of an error, an error code Err(i32) could be returned instead
        Ok(INFO)
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
        let sample_info_result: Result<&str, i32> = Sample::get_info("Test_client_name");
        assert!(sample_info_result.is_ok());
        let sample_info: &str = sample_info_result.unwrap();
        assert_eq!(INFO, sample_info);
        assert_eq!(INFO.len() as i32, sample_info.len() as i32);
    }
}
