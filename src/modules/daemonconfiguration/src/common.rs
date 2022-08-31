// Copyright (c) Microsoft Corporation. All rights reserved..
// Licensed under the MIT License.

use libc::{c_int, EINVAL, ENOMEM};
use std::ffi::NulError;
use std::fmt;
use std::str::Utf8Error;
use std::string::FromUtf8Error;

#[derive(Debug, PartialEq)]
pub enum MmiError {
    FailedRead,
    FailedAllocate,
    InvalidArgument,
    PayloadSizeExceeded,
    SerdeError,
    SystemctlError,
    SystemdError,
}

impl From<FromUtf8Error> for MmiError {
    fn from(_err: FromUtf8Error) -> MmiError {
        MmiError::FailedRead
    }
}

impl From<Utf8Error> for MmiError {
    fn from(_err: Utf8Error) -> MmiError {
        MmiError::FailedRead
    }
}

impl From<NulError> for MmiError {
    fn from(_err: NulError) -> MmiError {
        MmiError::FailedAllocate
    }
}

impl From<serde_json::Error> for MmiError {
    fn from(_err: serde_json::Error) -> MmiError {
        MmiError::SerdeError
    }
}

impl From<std::io::Error> for MmiError {
    fn from(_err: std::io::Error) -> MmiError {
        MmiError::SystemctlError
    }
}

impl Into<c_int> for MmiError {
    fn into(self) -> c_int {
        match self {
            MmiError::FailedAllocate => ENOMEM,
            MmiError::PayloadSizeExceeded => ENOMEM,
            _ => EINVAL,
        }
    }
}

impl fmt::Display for MmiError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match &*self {
            MmiError::FailedRead => {
                write!(f, "A CString read from a raw pointer failed")
            }
            MmiError::FailedAllocate => {
                write!(f, "A memory allocation failed")
            }
            MmiError::InvalidArgument => {
                write!(f, "There was an invalid argument")
            }
            MmiError::PayloadSizeExceeded => {
                write!(f, "The payload exceeded max payload bytes size")
            }
            MmiError::SerdeError => {
                write!(f, "There was an error serializing or deserializing")
            }
            MmiError::SystemctlError => {
                write!(f, "There was an error fetching daemon information using systemctl")
            }
            MmiError::SystemdError => {
                write!(f, "The caller was not started via Systemd and does not have Systemd Daemons")
            }
        }
    }
}