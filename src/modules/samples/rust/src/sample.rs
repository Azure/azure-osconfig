// Copyright (c) Microsoft Corporation. All rights reserved..
// Licensed under the MIT License.

use std::collections::HashMap;

const G_COMPONENT_NAME: &str = "SampleComponent";

const G_DESIRED_STRING_OBJECT_NAME: &str = "desiredStringObject";
const G_REPORTED_STRING_OBJECT_NAME: &str = "reportedStringObject";
const G_DESIRED_INTEGER_OBJECT_NAME: &str = "desiredIntegerObject";
const G_REPORTED_INTEGER_OBJECT_NAME: &str = "reportedIntegerObject";
const G_DESIRED_BOOLEAN_OBJECT_NAME: &str = "desiredBooleanObject";
const G_REPORTED_BOOLEAN_OBJECT_NAME: &str = "reportedBooleanObject";
const G_DESIRED_OBJECT_NAME: &str = "desiredObject";
const G_REPORTED_OBJECT_NAME: &str = "reportedObject";
const G_DESIRED_ARRAY_OBJECT_NAME: &str = "desiredArrayObject";
const G_REPORTED_ARRAY_OBJECT_NAME: &str = "reportedArrayObject";

const G_STRING_SETTING_NAME: &str = "stringSetting";
const G_INTEGER_SETTING_NAME: &str = "integerSetting";
const G_BOOLEAN_SETTING_NAME: &str = "booleanSetting";
const G_INTEGER_ENUMERATION_SETTING_NAME: &str = "integerEnumerationSetting";
const G_STRING_ARRAY_SETTING_NAME: &str = "stringsArraySetting";
const G_INTEGER_ARRAY_SETTING_NAME: &str = "integerArraySetting";
const G_STRING_MAP_SETTING_NAME: &str = "stringMapSetting";
const G_INTEGER_MAP_SETTING_NAME: &str = "integerMapSetting";

// r# Denotes a Rust Raw String
const INFO: &str = r#"""({
    "Name": "C++ Sample",
    "Description": "A sample module written in C++",
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
    integer_etting: i32,
    boolean_setting: bool,
    enumeration_setting: IntegerEnumeration,
    string_array_setting: Vec<String>,
    integer_array_setting: Vec<i32>,
    string_map_setting: HashMap<String, String>,
    integer_map_setting: HashMap<String, i32>,

    // Store removed elements to report as 'null'
    // These vectors must have a maximum size relative to the max payload size recieved by MmiOpen()
    removed_string_map_setting_keys: Vec<String>,
    removed_integer_map_setting_keys: Vec<i32>,
}

pub struct Sample {
    max_payload_size_bytes: u32,
}

impl Sample {
    pub fn new(max_payload_size_bytes: u32) -> Sample {
        // Initializes a Sample struct
        Sample {
            max_payload_size_bytes: max_payload_size_bytes,
        }
        // The result is returned if the ending semicolon is omitted
    }

    #[cfg(test)]
    fn get_max_payload_size_bytes(&self) -> u32 {
        self.max_payload_size_bytes
    }
}

#[cfg(test)]
mod tests {
    #[test]
    fn build_sample() {
        let test = super::Sample::new(16);
        assert_eq!(test.get_max_payload_size_bytes(), 16);
    }
}
