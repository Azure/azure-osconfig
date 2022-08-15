// Copyright (c) Microsoft Corporation. All rights reserved..
// Licensed under the MIT License.

use common::MmiError;
use libc::{c_char, c_int, c_uint, c_void, EINVAL, ENOMEM};
use sample::Sample;
use std::ffi::{CStr, CString};
use std::ptr;

mod common;
mod sample;

// Constants and Method headers were generated by bindgen
// (https://github.com/rust-lang/rust-bindgen)
pub const MMI_OK: i32 = 0;
pub type MmiHandle = *mut c_void;
pub type MmiJsonString = *mut c_char;

// MSFT change: Added the #[no_mangle] annotation, renamed
// parameters, and defined the methods
#[no_mangle]
pub extern "C" fn MmiGetInfo(
    client_name: *const c_char,
    payload: *mut MmiJsonString,
    payload_size_bytes: *mut c_int,
) -> c_int {
    if client_name.is_null() {
        println!("MmiGetInfo called with null clientName");
        EINVAL
    } else if payload.is_null() {
        println!("MmiGetInfo called with null payload");
        EINVAL
    } else if payload_size_bytes.is_null() {
        println!("MmiGetInfo called with null payloadSizeBytes");
        EINVAL
    } else {
        // Borrow the client_name ptr
        let client_name: &CStr = unsafe { CStr::from_ptr(client_name) };
        let result: Result<i32, MmiError> =
            mmi_get_info_helper(client_name, payload, payload_size_bytes);
        match result {
            Ok(code) => code,
            Err(e) => {
                println!("{}", e);
                match e {
                    MmiError::FailedAllocate => ENOMEM,
                    _ => EINVAL,
                }
            }
        }
    }
}

fn mmi_get_info_helper(
    client_name: &CStr,
    payload: *mut MmiJsonString,
    payload_size_bytes: *mut c_int,
) -> Result<i32, MmiError> {
    // The question operator will either unwrap and continue or return an Err(MmiError)
    let client_name: &str = client_name.to_str()?;
    let info: &str = Sample::get_info(client_name)?;
    let payload_string: CString = CString::new(info)?;
    let payload_size = payload_string.as_bytes().len();
    let payload_ptr: MmiJsonString = CString::into_raw(payload_string);
    unsafe {
        *payload = payload_ptr;
        *payload_size_bytes = payload_size as i32;
    }
    Ok(MMI_OK)
}

#[no_mangle]
pub extern "C" fn MmiOpen(client_name: *const c_char, max_payload_size_bytes: c_uint) -> MmiHandle {
    if client_name.is_null() {
        println!("MmiOpen called with null clientName");
        ptr::null_mut() as *mut c_void
    } else {
        let sample_box: Box<Sample> = Box::<Sample>::new(Sample::new(max_payload_size_bytes));
        Box::into_raw(sample_box) as *mut c_void
    }
}

#[no_mangle]
pub extern "C" fn MmiClose(client_session: MmiHandle) {
    if !client_session.is_null() {
        // The "_" variable name is to throwaway anything stored into it
        let _: Box<Sample> = unsafe { Box::from_raw(client_session as *mut Sample) };
    }
}

#[no_mangle]
pub extern "C" fn MmiSet(
    client_session: MmiHandle,
    component_name: *const c_char,
    object_name: *const c_char,
    payload: MmiJsonString,
    payload_size_bytes: c_int,
) -> c_int {
    unimplemented!("MmiSet is not yet implemented");
}

#[no_mangle]
pub extern "C" fn MmiGet(
    client_session: MmiHandle,
    component_name: *const c_char,
    object_name: *const c_char,
    payload: *mut MmiJsonString,
    payload_size_bytes: *mut c_int,
) -> c_int {
    if client_session.is_null() {
        println!("MmiGet called with null clientSession");
        EINVAL
    } else if payload_size_bytes.is_null() {
        println!("MmiGet called with Invalid payloadSizeBytes");
        EINVAL
    } else {
        let sample: &Sample = unsafe { &*(client_session as *mut Sample) };
        let component_name: &CStr = unsafe { CStr::from_ptr(component_name) };
        let object_name: &CStr = unsafe { CStr::from_ptr(object_name) };
        let result: Result<i32, MmiError> = mmi_get_helper(
            sample,
            component_name,
            object_name,
            payload,
            payload_size_bytes,
        );
        match result {
            Ok(code) => code,
            Err(e) => {
                println!("{}", e);
                match e {
                    MmiError::FailedAllocate => ENOMEM,
                    _ => EINVAL,
                }
            }
        }
    }
}

fn mmi_get_helper(
    sample: &Sample,
    component_name: &CStr,
    object_name: &CStr,
    payload: *mut MmiJsonString,
    payload_size_bytes: *mut c_int,
) -> Result<i32, MmiError> {
    let component_name: &str = component_name.to_str()?;
    let object_name: &str = object_name.to_str()?;
    let payload_string: String = sample.get(component_name, object_name)?;
    let payload_string: CString = CString::new(payload_string)?;
    let payload_size = payload_string.as_bytes().len();
    let payload_ptr: MmiJsonString = CString::into_raw(payload_string);
    unsafe {
        *payload = payload_ptr;
        *payload_size_bytes = payload_size as i32;
    }
    Ok(MMI_OK)
}

#[no_mangle]
pub extern "C" fn MmiFree(payload: MmiJsonString) {
    if !payload.is_null() {
        let _ = unsafe { CString::from_raw(payload) };
    }
}
