// Copyright (c) Microsoft Corporation. All rights reserved..
// Licensed under the MIT License.

use lazy_static::lazy_static;
use regex::Regex;
use serde::{Deserialize, Serialize};
use std::process::Command;
use systemctl as systemctl_crate;

use crate::MmiError;

const COMPONENT_NAME: &str = "SystemdDaemonConfiguration";
const REPORTED_OBJECT_NAME: &str = "daemonConfiguration";
const DESIRED_OBJECT_NAME: &str = "desiredDaemonConfiguration";

type Result<T> = std::result::Result<T, MmiError>;

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

// An object representing the desired settings for a daemon
#[derive(Serialize, Deserialize, Debug, PartialEq, Eq)]
#[serde(rename_all = "camelCase")]
pub struct DesiredDaemon {
    name: String,
    started: bool,
    enabled: bool,
}

pub trait SystemctlInfo {
    fn new() -> Self;
    fn get_daemons(&self) -> Result<Vec<Daemon>>;
    fn list_unit_files(&self) -> Result<String>;
    fn show_substate_property(&self, name: &str) -> Result<String>;
    fn create_daemon(&self, name: &str, auto_start_status: &str) -> Result<Daemon>;
    fn exists(&self, name: &str) -> Result<bool>;
    fn start(&mut self, name: &str) -> Result<bool>;
    fn stop(&mut self, name: &str) -> Result<bool>;
    fn enable(&mut self, name: &str) -> Result<bool>;
    fn disable(&mut self, name: &str) -> Result<bool>;
}

pub struct Systemctl {}

impl SystemctlInfo for Systemctl {
    fn new() -> Self {
        Systemctl {}
    }

    fn get_daemons(&self) -> Result<Vec<Daemon>> {
        let systemctl_output = self.list_unit_files()?;
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
            let daemon = self.create_daemon(&service["service"], &service["status"])?;
            // Only report enabled daemons with State not being Other
            if (daemon.state != State::Other) && (daemon.state != State::Dead) && (daemon.auto_start_status == AutoStartStatus::Enabled) {
                services.push(daemon);
            }
        }
        Ok(services)
    }

    fn list_unit_files(&self) -> Result<String> {
        let output = {
            Command::new("systemctl")
                .args(["list-unit-files", "--type=service"])
                .output()?
        };
        Ok(String::from(std::str::from_utf8(&output.stdout)?))
    }

    fn create_daemon(&self, name: &str, auto_start_status: &str) -> Result<Daemon> {
        let substate = self.show_substate_property(name)?;
        if !substate.starts_with("SubState=") {
            return Err(MmiError::SystemctlError);
        }
        let substate = &substate[9..];
        let state = match substate.trim() {
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

    fn show_substate_property(&self, name: &str) -> Result<String> {
        let output = {
            Command::new("systemctl")
                .args(["show", name, "--property=SubState"])
                .output()?
        };
        Ok(String::from(std::str::from_utf8(&output.stdout)?))
    }

    fn exists(&self, name: &str) -> Result<bool> {
        Ok(systemctl_crate::exists(name)?)
    }

    // Start systemd daemon with name "name". Returns true if successful, false otherwise.
    fn start(&mut self, name: &str) -> Result<bool> {
        let unit = systemctl::Unit::from_systemctl(name)?;
        if unit.is_active()? {
            // Unit is already started
            Ok(false)
        } else {
            let exit_status = unit.restart()?;
            Ok(exit_status.success())
        }
    }

    // Stop systemd daemon with name "name". Returns true if successful, false otherwise.
    fn stop(&mut self, name: &str) -> Result<bool> {
        let unit = systemctl::Unit::from_systemctl(name)?;
        if !unit.is_active()? {
            // Unit is already stopped
            Ok(false)
        } else {
            let exit_status = systemctl_crate::stop(name)?;
            Ok(exit_status.success())
        }
    }

    // Enable systemd daemon with name "name". Returns true if successful, false otherwise.
    fn enable(&mut self, name: &str) -> Result<bool> {
        let unit = systemctl::Unit::from_systemctl(name)?;
        if unit.auto_start == systemctl_crate::AutoStartStatus::Enabled {
            // Unit is already enabled
            Ok(false)
        } else {
            let output = Command::new("systemctl").args(["enable", name]).output()?;
            Ok(true)
        }
    }

    // Disable systemd daemon with name "name". Returns true if successful, false otherwise.
    fn disable(&mut self, name: &str) -> Result<bool> {
        let unit = systemctl::Unit::from_systemctl(name)?;
        if unit.auto_start == systemctl_crate::AutoStartStatus::Disabled {
            // Unit is already disabled
            Ok(false)
        } else {
            Command::new("systemctl").args(["disable", name]).output()?;
            Ok(true)
        }
    }
}

#[derive(Default, Debug)]
pub struct DaemonConfiguration<SystemCaller: SystemctlInfo> {
    max_payload_size_bytes: u32,
    system_caller: SystemCaller,
}

impl<SystemCaller: SystemctlInfo> DaemonConfiguration<SystemCaller> {
    pub fn new(max_payload_size_bytes: u32, system_caller: SystemCaller) -> Self {
        // The result is returned if the ending semicolon is omitted
        DaemonConfiguration {
            max_payload_size_bytes: max_payload_size_bytes,
            system_caller: system_caller,
        }
    }

    pub fn get_info(_client_name: &str) -> Result<&str> {
        // This daemon_config module makes no use of the client_name, but
        // it may be copied, compared, etc. here
        // In the case of an error, an error code Err(i32) could be returned instead
        Ok(INFO)
    }

    pub fn get(&self, component_name: &str, object_name: &str) -> Result<String> {
        if !libsystemd::daemon::booted() {
            // Whether the caller was booted using Systemd
            Err(MmiError::SystemdError)
        } else if !COMPONENT_NAME.eq(component_name) {
            println!("Invalid component name: {}", component_name);
            Err(MmiError::InvalidArgument)
        } else if !REPORTED_OBJECT_NAME.eq(object_name) {
            println!("Invalid object name: {}", object_name);
            Err(MmiError::InvalidArgument)
        } else {
            let daemons = self.system_caller.get_daemons()?;
            let json_value = serde_json::to_value::<&Vec<Daemon>>(&daemons)?;
            let payload: String = serde_json::to_string(&json_value)?;
            if (self.max_payload_size_bytes != 0)
                && (payload.len() as u32 > self.max_payload_size_bytes)
            {
                println!("Payload size exceeded max payload size bytes in get so it was truncated.");
                let payload_bytes = payload.into_bytes();
                let truncated_payload = String::from_utf8((&payload_bytes[0..self.max_payload_size_bytes as usize]).to_vec())?;
                Ok(truncated_payload)
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
    ) -> Result<i32> {
        if !libsystemd::daemon::booted() {
            // Whether the caller was booted using Systemd
            Err(MmiError::SystemdError)
        } else if !COMPONENT_NAME.eq(component_name) {
            println!("Invalid component name: {}", component_name);
            Err(MmiError::InvalidArgument)
        } else if !DESIRED_OBJECT_NAME.eq(object_name) {
            println!("Invalid object name: {}", object_name);
            Err(MmiError::InvalidArgument)
        } else if self.max_payload_size_bytes != 0
            && payload_str_slice.len() as u32 > self.max_payload_size_bytes
        {
            println!("Payload size exceeds max payload size bytes");
            Err(MmiError::InvalidArgument)
        } else {
            let desired_daemons = serde_json::from_str::<Vec<DesiredDaemon>>(payload_str_slice)?;
            for desired_daemon in desired_daemons {
                if !self.system_caller.exists(&desired_daemon.name)? {
                    println!(
                        "Daemon {} wasn't set. Was not found by systemctl",
                        desired_daemon.name
                    );
                } else if desired_daemon.started && !desired_daemon.enabled {
                    println!(
                        "Daemon {} wasn't set. Disabled daemons cannot be started",
                        desired_daemon.name
                    );
                } else if desired_daemon.started {
                    // Enable and start
                    self.system_caller.enable(&desired_daemon.name)?;
                    self.system_caller.start(&desired_daemon.name)?;
                } else if !desired_daemon.started && desired_daemon.enabled {
                    // Stop and leave Enabled
                    self.system_caller.stop(&desired_daemon.name)?;
                    self.system_caller.enable(&desired_daemon.name)?;
                } else {
                    // Stop and disable
                    self.system_caller.stop(&desired_daemon.name)?;
                    self.system_caller.disable(&desired_daemon.name)?;
                }
            }
            Ok(0)
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
    use std::collections::HashMap;
    const MAX_PAYLOAD_BYTES: u32 = 0;

    struct SystemctlTest {
        auto_start_statuses: HashMap<String, String>,
        states: HashMap<String, String>,
    }

    impl SystemctlInfo for SystemctlTest {
        fn new() -> Self {
            let auto_start_statuses = HashMap::from([
                (String::from("alsa-restore"), String::from("static")),
                (String::from("alsa-utils"), String::from("masked")),
                (String::from("apport-forward@"), String::from("static")),
                (String::from("apport"), String::from("enabled")),
                (String::from("netplan-ovs-cleanup"), String::from("enabled-runtime")),
                (String::from("osconfig"), String::from("disabled")),
                (String::from("rtkit-daemon"), String::from("enabled")),
                (String::from("saned"), String::from("enabled")),
                (String::from("spice-vdagent"), String::from("indirect")),
            ]);
            let states = HashMap::from([
                (String::from("alsa-restore"), String::from("dead")),
                (String::from("alsa-utils"), String::from("running")),
                (String::from("apport-forward@"), String::from("")),
                (String::from("apport"), String::from("failed")),
                (String::from("netplan-ovs-cleanup"), String::from("exited")),
                (String::from("osconfig"), String::from("dead")),
                (String::from("rtkit-daemon"), String::from("exited")),
                (String::from("saned"), String::from("running")),
                (String::from("spice-vdagent"), String::from("dead")),
            ]);
            SystemctlTest {
                auto_start_statuses: auto_start_statuses,
                states: states,
            }
        }

        fn list_unit_files(&self) -> Result<String> {
            let list_unit_output = format!(
                "UNIT FILE                                  STATE           VENDOR PRESET
                alsa-restore.service                       {}          enabled      
                alsa-utils.service                         {}          enabled      
                apport-forward@.service                    {}          enabled      
                apport.service                             {}         enabled      
                netplan-ovs-cleanup.service                {} enabled
                osconfig.service                           {}        enabled      
                rtkit-daemon.service                       {}         enabled      
                saned.service                              {}         enabled           
                spice-vdagent.service                      {}        enabled           
                
                9 unit files listed.",
                self.auto_start_statuses["alsa-restore"],
                self.auto_start_statuses["alsa-utils"],
                self.auto_start_statuses["apport-forward@"],
                self.auto_start_statuses["apport"],
                self.auto_start_statuses["netplan-ovs-cleanup"],
                self.auto_start_statuses["osconfig"],
                self.auto_start_statuses["rtkit-daemon"],
                self.auto_start_statuses["saned"],
                self.auto_start_statuses["spice-vdagent"]
            );
            Ok(list_unit_output)
        }

        fn get_daemons(&self) -> Result<Vec<Daemon>> {
            let systemctl_output = self.list_unit_files()?;
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
                let daemon = self.create_daemon(&service["service"], &service["status"])?;
                // Only report enabled daemons with State not being Other
                if (daemon.state != State::Other) && (daemon.state != State::Dead) && (daemon.auto_start_status == AutoStartStatus::Enabled) {
                    services.push(daemon);
                }
            }
            Ok(services)
        }

        fn create_daemon(&self, name: &str, auto_start_status: &str) -> Result<Daemon> {
            let substate = self.show_substate_property(name)?;
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

        fn show_substate_property(&self, name: &str) -> Result<String> {
            let substate_output = format!("SubState={}", self.states[name]);
            Ok(substate_output)
        }

        fn exists(&self, name: &str) -> Result<bool> {
            Ok(systemctl_crate::exists(name)?)
        }

        fn start(&mut self, name: &str) -> Result<bool> {
            self.states
                .insert(String::from(name), String::from("running"));
            Ok(true)
        }

        fn stop(&mut self, name: &str) -> Result<bool> {
            self.states.insert(String::from(name), String::from("dead"));
            Ok(true)
        }

        fn enable(&mut self, name: &str) -> Result<bool> {
            self.auto_start_statuses
                .insert(String::from(name), String::from("enabled"));
            Ok(true)
        }

        fn disable(&mut self, name: &str) -> Result<bool> {
            self.auto_start_statuses
                .insert(String::from(name), String::from("disabled"));
            Ok(true)
        }
    }

    #[test]
    fn build_daemon_config() {
        let daemon_config =
            DaemonConfiguration::<SystemctlTest>::new(MAX_PAYLOAD_BYTES, SystemctlTest::new());
        assert_eq!(
            daemon_config.get_max_payload_size_bytes(),
            MAX_PAYLOAD_BYTES
        );
    }

    #[test]
    fn info_size() {
        let daemon_config_info: Result<&str> =
            DaemonConfiguration::<SystemctlTest>::get_info("Test_client_name");
        assert!(daemon_config_info.is_ok());
        let daemon_config_info: &str = daemon_config_info.unwrap();
        assert_eq!(INFO, daemon_config_info);
        assert_eq!(INFO.len() as i32, daemon_config_info.len() as i32);
    }

    #[test]
    fn invalid_get() {
        let daemon_config =
            DaemonConfiguration::<SystemctlTest>::new(MAX_PAYLOAD_BYTES, SystemctlTest::new());
        if libsystemd::daemon::booted() {
            let invalid_component_result: Result<String> =
                daemon_config.get("Invalid component", REPORTED_OBJECT_NAME);
            assert!(invalid_component_result.is_err());
            if let Err(e) = invalid_component_result {
                assert_eq!(e, MmiError::InvalidArgument);
            }
            let invalid_object_result: Result<String> =
                daemon_config.get(COMPONENT_NAME, "Invalid object");
            assert!(invalid_object_result.is_err());
            if let Err(e) = invalid_object_result {
                assert_eq!(e, MmiError::InvalidArgument);
            }
        } else {
            let systemd_result: Result<String> =
                daemon_config.get(COMPONENT_NAME, REPORTED_OBJECT_NAME);
            assert!(systemd_result.is_err());
            if let Err(e) = systemd_result {
                assert_eq!(e, MmiError::SystemdError);
            }
        }
    }

    #[test]
    fn invalid_set() {
        let mut daemon_config =
            DaemonConfiguration::<SystemctlTest>::new(MAX_PAYLOAD_BYTES, SystemctlTest::new());
        let valid_json_payload = r#"[{name: "osconfig", started:true, enabled:true}]"#;
        let invalid_json_payload = r#"Invalid payload"#;
        if libsystemd::daemon::booted() {
            let invalid_component_result: Result<i32> =
                daemon_config.set("Invalid component", DESIRED_OBJECT_NAME, valid_json_payload);
            assert!(invalid_component_result.is_err());
            if let Err(e) = invalid_component_result {
                assert_eq!(e, MmiError::InvalidArgument);
            }
            let invalid_object_result: Result<i32> =
                daemon_config.set(COMPONENT_NAME, "Invalid object", valid_json_payload);
            assert!(invalid_object_result.is_err());
            if let Err(e) = invalid_object_result {
                assert_eq!(e, MmiError::InvalidArgument);
            }
            let invalid_payload_result: Result<i32> =
                daemon_config.set(COMPONENT_NAME, DESIRED_OBJECT_NAME, invalid_json_payload);
            assert!(invalid_payload_result.is_err());
            if let Err(e) = invalid_payload_result {
                assert_eq!(e, MmiError::SerdeError);
            }
        } else {
            let systemd_result: Result<i32> =
                daemon_config.set(COMPONENT_NAME, REPORTED_OBJECT_NAME, valid_json_payload);
            assert!(systemd_result.is_err());
            if let Err(e) = systemd_result {
                assert_eq!(e, MmiError::SystemdError);
            }
        }
    }

    #[test]
    fn get_set() {
        let mut daemon_config =
            DaemonConfiguration::<SystemctlTest>::new(MAX_PAYLOAD_BYTES, SystemctlTest::new());
        if libsystemd::daemon::booted() {
            let payload = daemon_config.get(COMPONENT_NAME, REPORTED_OBJECT_NAME);
            assert!(payload.is_ok());
            let payload = payload.unwrap();
            let expected = "[\
                {\
                    \"name\":\"apport\",\
                    \"state\":\"failed\",\
                    \"autoStartStatus\":\"enabled\"\
                },\
                {\
                    \"name\":\"rtkit-daemon\",\
                    \"state\":\"exited\",\
                    \"autoStartStatus\":\"enabled\"\
                },\
                {\
                    \"name\":\"saned\",\
                    \"state\":\"running\",\
                    \"autoStartStatus\":\"enabled\"\
                }\
            ]";
            assert!(json_strings_eq::<Vec<Daemon>>(payload.as_str(), expected));
            let set_payload = r#"[{"name": "osconfig", "started": true, "enabled": true}, {"name": "saned", "started": false, "enabled": false}]"#;
            let set_result = daemon_config.set(COMPONENT_NAME, DESIRED_OBJECT_NAME, set_payload);
            assert!(set_result.is_ok());
            assert_eq!(0, set_result.unwrap());
            let expected = "[\
                {\
                    \"name\":\"apport\",\
                    \"state\":\"failed\",\
                    \"autoStartStatus\":\"enabled\"\
                },\
                {\
                    \"name\":\"osconfig\",\
                    \"state\":\"running\",\
                    \"autoStartStatus\":\"enabled\"\
                },\
                {\
                    \"name\":\"rtkit-daemon\",\
                    \"state\":\"exited\",\
                    \"autoStartStatus\":\"enabled\"\
                }\
            ]";
            let payload: String = daemon_config
                .get(COMPONENT_NAME, REPORTED_OBJECT_NAME)
                .unwrap();
            assert!(json_strings_eq::<Vec<Daemon>>(payload.as_str(), expected));
        }
    }

    #[test]
    fn get_truncated_payload() {
        let daemon_config = DaemonConfiguration::new(16, SystemctlTest::new());
        if libsystemd::daemon::booted() {
            let payload = daemon_config.get(COMPONENT_NAME, REPORTED_OBJECT_NAME);
            assert!(payload.is_ok());
            let payload = payload.unwrap();
            println!("{}", payload);
            let expected = "[\
                {\
                    \"name\":\"appor";
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
