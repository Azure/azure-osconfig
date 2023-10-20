use std::ffi::{c_char, c_void};

pub use libc;

pub const OK: i32 = 0;
pub const ERROR: i32 = -1;

pub type Handle = *mut c_void;
pub type JsonString = *mut c_char;
