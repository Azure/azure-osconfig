// Copyright (c) Microsoft Corporation. All rights reserved..
// Licensed under the MIT License.

use lazy_static::lazy_static;
use regex::Regex;
use serde::{Deserialize, Serialize};
use std::process::Command;

use crate::MmiError;

const COMPONENT_NAME: &str = "SystemdDaemonConfiguration";
const OBJECT_NAME: &str = "daemonConfiguration";

// r# Denotes a Rust Raw String
const INFO: &str = r#"{
    "Name": "Daemon Configuration",
    "Description": "Reports current systemd daemons and allows configuring of them",
    "Manufacturer": "Microsoft",
    "VersionMajor": 1,
    "VersionMinor": 0,
    "VersionInfo": "",
    "Components": ["SystemdDaemonConfiguration"],
    "Lifetime": 1,
    "UserAccount": 0}"#;

#[derive(Serialize, Deserialize, Debug, PartialEq, Eq)]
#[serde(rename_all = "camelCase")]
pub enum State {
    Other,
    Running,
    Failed,
    Exited,
    Dead,
}

#[derive(Serialize, Deserialize, Debug, PartialEq, Eq)]
#[serde(rename_all = "camelCase")]
pub enum AutoStartStatus {
    Other,
    Enabled,
    Disabled,
}

// A daemon_config object with all possible setting types
#[derive(Serialize, Deserialize, Debug, PartialEq, Eq)]
#[serde(rename_all = "camelCase")]
pub struct Daemon {
    name: String,
    state: State,
    auto_start_status: AutoStartStatus,
}

pub trait SystemctlInfo {
    fn get_daemons() -> Result<Vec<Daemon>, MmiError>;
    fn list_unit_files() -> Result<String, MmiError>;
    fn show_substate_property(name: &str) -> Result<String, MmiError>;
    fn create_daemon(name: &str, auto_start_status: &str) -> Result<Daemon, MmiError>;
}

pub struct Systemctl {}

impl SystemctlInfo for Systemctl {
    fn list_unit_files() -> Result<String, MmiError> {
        let output = {
            Command::new("systemctl")
                .args(["list-unit-files", "--type=service"])
                .output()?
        };
        Ok(String::from(std::str::from_utf8(&output.stdout)?))
    }

    fn get_daemons() -> Result<Vec<Daemon>, MmiError> {
        let systemctl_output = Self::list_unit_files()?;
        // Lazy_static prevents the regex from being compiled multiple times
        // https://docs.rs/regex/latest/regex/#example-avoid-compiling-the-same-regex-in-a-loop 
        lazy_static! {
            static ref RE: Regex =
                Regex::new(r"(?P<service>[\w_@\-\\\.]*)\.service\s+(?P<status>[\w-]+)").unwrap();
        }
        let mut services = Vec::new();
        for service in RE.captures_iter(&systemctl_output) {
            if service["service"].ends_with("@") {
                continue;
            }
            let daemon = Self::create_daemon(&service["service"], &service["status"])?;
            services.push(daemon);
        }
        Ok(services)
    }

    fn create_daemon(name: &str, auto_start_status: &str) -> Result<Daemon, MmiError> {
        let substate = Self::show_substate_property(name)?;
        if !substate.starts_with("SubState=") {
            return Err(MmiError::SystemctlError);
        }
        let substate = &substate[9..];
        let state = match substate {
            "running" => State::Running,
            "failed" => State::Failed,
            "exited" => State::Exited,
            "dead" => State::Dead,
            _ => State::Other,
        };
        let auto_start_status = match auto_start_status {
            "enabled" => AutoStartStatus::Enabled,
            "disabled" => AutoStartStatus::Disabled,
            _ => AutoStartStatus::Other,
        };
        Ok(Daemon {
            name: String::from(name),
            state: state,
            auto_start_status: auto_start_status,
        })
    }

    fn show_substate_property(name: &str) -> Result<String, MmiError> {
        let output = {
            Command::new("systemctl")
                .args(["show", name, "--property=SubState"])
                .output()?
        };
        Ok(String::from(std::str::from_utf8(&output.stdout)?))
    }
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

    pub fn get<T: SystemctlInfo>(
        &self,
        component_name: &str,
        object_name: &str,
    ) -> Result<String, MmiError> {
        if !libsystemd::daemon::booted() {
            // Whether the caller was booted using Systemd
            Err(MmiError::SystemdError)
        } else if !COMPONENT_NAME.eq(component_name) {
            println!("Invalid component name: {}", component_name);
            Err(MmiError::InvalidArgument)
        } else if !OBJECT_NAME.eq(object_name) {
            println!("Invalid object name: {}", object_name);
            Err(MmiError::InvalidArgument)
        } else {
            let daemons = T::get_daemons()?;
            let json_value = serde_json::to_value::<&Vec<Daemon>>(&daemons)?;
            let payload: String = serde_json::to_string(&json_value)?;
            if self.max_payload_size_bytes != 0
                && payload.len() as u32 > self.max_payload_size_bytes
            {
                println!("Payload size exceeded max payload size bytes in get");
                Err(MmiError::PayloadSizeExceeded)
            } else {
                Ok(payload)
            }
        }
    }

    pub fn set(
        &mut self,
        component_name: &str,
        object_name: &str,
        payload_str_slice: &str,
    ) -> Result<i32, MmiError> {
        unimplemented!("Daemon Manipulation is not yet supported");
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

    struct SystemctlTest {}

    impl SystemctlInfo for SystemctlTest {
        fn list_unit_files() -> Result<String, MmiError> {
            let list_unit_output = r#"UNIT FILE                                  STATE           VENDOR PRESET
            alsa-restore.service                       static          enabled      
            alsa-utils.service                         masked          enabled      
            apport-forward@.service                    static          enabled      
            apport.service                             generated       enabled      
            netplan-ovs-cleanup.service                enabled-runtime enabled
            osconfig.service                           enabled         enabled      
            rtkit-daemon.service                       disabled        enabled      
            saned.service                              masked          enabled           
            spice-vdagent.service                      indirect        enabled           
            
            9 unit files listed."#;
            Ok(String::from(list_unit_output))
        }

        fn get_daemons() -> Result<Vec<Daemon>, MmiError> {
            let systemctl_output = Self::list_unit_files()?;
            lazy_static! {
                static ref RE: Regex =
                    Regex::new(r"(?P<service>[\w_@\-\\\.]*)\.service\s+(?P<status>[\w-]+)")
                        .unwrap();
            }
            let mut services = Vec::new();
            for service in RE.captures_iter(&systemctl_output) {
                if service["service"].ends_with("@") {
                    continue;
                }
                let daemon = Self::create_daemon(&service["service"], &service["status"])?;
                services.push(daemon);
            }
            Ok(services)
        }

        fn create_daemon(name: &str, auto_start_status: &str) -> Result<Daemon, MmiError> {
            let substate = Self::show_substate_property(name)?;
            if !substate.starts_with("SubState=") {
                return Err(MmiError::SystemctlError);
            }
            let substate = &substate[9..];
            let state = match substate {
                "running" => State::Running,
                "failed" => State::Failed,
                "exited" => State::Exited,
                "dead" => State::Dead,
                _ => State::Other,
            };
            let auto_start_status = match auto_start_status {
                "enabled" => AutoStartStatus::Enabled,
                "disabled" => AutoStartStatus::Disabled,
                _ => AutoStartStatus::Other,
            };
            Ok(Daemon {
                name: String::from(name),
                state: state,
                auto_start_status: auto_start_status,
            })
        }

        fn show_substate_property(name: &str) -> Result<String, MmiError> {
            let substate_output = match name {
                "alsa-restore" => "SubState=dead",
                "alsa-utils" => "SubState=running",
                "apport" => "SubState=failed",
                "netplan-ovs-cleanup" => "SubState=exited",
                "osconfig" => "SubState=dead",
                "rtkit-daemon" => "SubState=exited",
                "saned" => "SubState=running",
                "spice-vdagent" => "SubState=dead",
                _ => "SubState=unknown",
            };
            Ok(String::from(substate_output))
        }
    }

    #[test]
    fn build_daemon_config() {
        let daemon_config = DaemonConfiguration::new(MAX_PAYLOAD_BYTES);
        assert_eq!(
            daemon_config.get_max_payload_size_bytes(),
            MAX_PAYLOAD_BYTES
        );
    }

    #[test]
    fn info_size() {
        let daemon_config_info: Result<&str, MmiError> =
            DaemonConfiguration::get_info("Test_client_name");
        assert!(daemon_config_info.is_ok());
        let daemon_config_info: &str = daemon_config_info.unwrap();
        assert_eq!(INFO, daemon_config_info);
        assert_eq!(INFO.len() as i32, daemon_config_info.len() as i32);
    }

    #[test]
    fn invalid_get() {
        let daemon_config = DaemonConfiguration::new(MAX_PAYLOAD_BYTES);
        if libsystemd::daemon::booted() {
            let invalid_component_result: Result<String, MmiError> =
                daemon_config.get::<SystemctlTest>("Invalid component", OBJECT_NAME);
            assert!(invalid_component_result.is_err());
            if let Err(e) = invalid_component_result {
                assert_eq!(e, MmiError::InvalidArgument);
            }
            let invalid_object_result: Result<String, MmiError> =
                daemon_config.get::<SystemctlTest>(COMPONENT_NAME, "Invalid object");
            assert!(invalid_object_result.is_err());
            if let Err(e) = invalid_object_result {
                assert_eq!(e, MmiError::InvalidArgument);
            }
        } else {
            let systemd_result: Result<String, MmiError> =
                daemon_config.get::<SystemctlTest>(COMPONENT_NAME, OBJECT_NAME);
            assert!(systemd_result.is_err());
            if let Err(e) = systemd_result {
                assert_eq!(e, MmiError::SystemdError);
            }
        }
    }

    #[test]
    fn valid_get() {
        let daemon_config = DaemonConfiguration::new(MAX_PAYLOAD_BYTES);
        if libsystemd::daemon::booted() {
            let payload = daemon_config.get::<SystemctlTest>(COMPONENT_NAME, OBJECT_NAME);
            assert!(payload.is_ok());
            let payload = payload.unwrap();
            let expected = "[\
                {\
                    \"name\":\"alsa-restore\",\
                    \"state\":\"dead\",\
                    \"autoStartStatus\":\"other\"\
                },\
                {\
                    \"name\":\"alsa-utils\",\
                    \"state\":\"running\",\
                    \"autoStartStatus\":\"other\"\
                },\
                {\
                    \"name\":\"apport\",\
                    \"state\":\"failed\",\
                    \"autoStartStatus\":\"other\"\
                },\
                {\
                    \"name\":\"netplan-ovs-cleanup\",\
                    \"state\":\"exited\",\
                    \"autoStartStatus\":\"other\"\
                },\
                {\
                    \"name\":\"osconfig\",\
                    \"state\":\"dead\",\
                    \"autoStartStatus\":\"enabled\"\
                },\
                {\
                    \"name\":\"rtkit-daemon\",\
                    \"state\":\"exited\",\
                    \"autoStartStatus\":\"disabled\"\
                },\
                {\
                    \"name\":\"saned\",\
                    \"state\":\"running\",\
                    \"autoStartStatus\":\"other\"\
                },\
                {\
                    \"name\":\"spice-vdagent\",\
                    \"state\":\"dead\",\
                    \"autoStartStatus\":\"other\"\
                }\
            ]";
            assert!(json_strings_eq::<Vec<Daemon>>(payload.as_str(), expected));
        }
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
