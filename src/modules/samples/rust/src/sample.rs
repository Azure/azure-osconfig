// Copyright (c) Microsoft Corporation. All rights reserved..
// Licensed under the MIT License.

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

#[derive(Clone, Copy)]
pub struct Sample {
    max_payload_size_bytes: u32,
}

impl Sample {
    pub fn new(max_payload_size_bytes: u32) -> Sample {
        Sample {
            max_payload_size_bytes: max_payload_size_bytes,
        }
    }
}
