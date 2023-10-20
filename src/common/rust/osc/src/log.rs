use std::io::Write;

use env_logger::{Builder, Env};

// These are required by the code generated via the `osc_codegen` macros.
pub use log::{debug, error, info, trace, warn};

/// Initializes the logger to provide standard formatting for the [`log`] crate using [`env_logger`].
///
/// Format: `[<timestamp>] [<short file>:<line number>] [<log level>] <message>`
///
/// WARNING: this function should only be called *once* as early in the program execution as possible.
pub fn init() {
    let env = Env::default();

    Builder::from_env(env)
        .format(|buf, record| {
            let timestamp = buf.timestamp();
            let file = record.module_path().unwrap_or("unknown");
            let line_number = record.line().unwrap_or(0);
            let level = record.level();

            writeln!(
                buf,
                "[{}] [{}:{}] [{}] {}",
                timestamp,
                file,
                line_number,
                level,
                record.args()
            )
        })
        .init();
}
