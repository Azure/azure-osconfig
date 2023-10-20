// Depend on osc_codegen and re-export everything in it.
// This allows users to just depend on osc and automatically get the macro functionality.
pub use osc_codegen::osc_module;

pub mod log;
pub mod module;
