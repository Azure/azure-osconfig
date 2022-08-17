// Copyright (c) Microsoft Corporation. All rights reserved..
// Licensed under the MIT License.

use serde::{Deserialize, Serialize};
use serde_repr::{Serialize_repr, Deserialize_repr};

use crate::MmiError;

const COMPONENT_NAME: &str = "DaemonConfiguration";
const OBJECT_NAME: &str = "dameons";


// r# Denotes a Rust Raw String
const INFO: &str = r#"{
    "Name": "Daemon Configuration",
    "Description": "Reports current systemd daemons and allows configuring of them",
    "Manufacturer": "Microsoft",
    "VersionMajor": 1,
    "VersionMinor": 0,
    "VersionInfo": "",
    "Components": ["DaemonConfiguration"],
    "Lifetime": 1,
    "UserAccount": 0}"#;

#[derive(Serialize_repr, Deserialize_repr, Debug, PartialEq, Eq)]
#[repr(u8)]
enum State {
    Unknown = 0,
    Running = 1,
    Failed = 2,
    Stopped = 3,
}

#[derive(Serialize_repr, Deserialize_repr, Debug, PartialEq, Eq)]
#[repr(u8)]
enum Status {
    Unknown = 0,
    Enabled = 1,
    Disabled = 2,
    Static = 3,
    Indirect = 4,
}

// A daemon_config object with all possible setting types
#[derive(Serialize, Deserialize, Debug, PartialEq, Eq)]
struct Daemon<'a> {
    name: &'a str,
    state: State,
    status: Status,
}

#[derive(Default, Debug)]
pub struct DaemonConfiguration {
    max_payload_size_bytes: u32,

}

impl DaemonConfiguration {
    pub fn new(max_payload_size_bytes: u32) -> Self {
        // The result is returned if the ending semicolon is omitted
        DaemonConfiguration {
            max_payload_size_bytes: max_payload_size_bytes,
        }
    }

    pub fn get_info(_client_name: &str) -> Result<&str, MmiError> {
        // This daemon_config module makes no use of the client_name, but
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
        unimplemented!("Set hasn't been implemented");
    }

    pub fn get(&self, component_name: &str, object_name: &str) -> Result<String, MmiError> {
        if !COMPONENT_NAME.eq(component_name) {
            println!("Invalid component name: {}", component_name);
            Err(MmiError::InvalidArgument)
        } else if !OBJECT_NAME.eq(object_name) {
            println!("Invalid object name: {}", object_name);
            Err(MmiError::InvalidArgument)
        } else {
            let enabled_services = systemctl::list_units(None, None)?;
            println!("{:?}", enabled_services);
            let disabled_services = systemctl::list_disabled_services()?;
            println!("{:?}", disabled_services);
            Ok(String::from(""))
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
    fn build_daemon_config() {
        let daemon_config = DaemonConfiguration::new(MAX_PAYLOAD_BYTES);
        assert_eq!(daemon_config.get_max_payload_size_bytes(), MAX_PAYLOAD_BYTES);
    }

    #[test]
    fn info_size() {
        let daemon_config_info: Result<&str, MmiError> = DaemonConfiguration::get_info("Test_client_name");
        assert!(daemon_config_info.is_ok());
        let daemon_config_info: &str = daemon_config_info.unwrap();
        assert_eq!(INFO, daemon_config_info);
        assert_eq!(INFO.len() as i32, daemon_config_info.len() as i32);
    }

    #[test]
    fn invalid_get() {
        let daemon_config = DaemonConfiguration::new(MAX_PAYLOAD_BYTES);
        let invalid_component_result: Result<String, MmiError> =
            daemon_config.get("Invalid component", OBJECT_NAME);
        assert!(invalid_component_result.is_err());
        if let Err(e) = invalid_component_result {
            assert_eq!(e, MmiError::InvalidArgument);
        }
        let invalid_object_result: Result<String, MmiError> =
            daemon_config.get(COMPONENT_NAME, "Invalid object");
        assert!(invalid_object_result.is_err());
        if let Err(e) = invalid_object_result {
            assert_eq!(e, MmiError::InvalidArgument);
        }
        daemon_config.get(COMPONENT_NAME, OBJECT_NAME);
        assert!(false);
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
