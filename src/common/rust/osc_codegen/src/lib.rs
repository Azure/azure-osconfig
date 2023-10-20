// NOTICE: Unfortunately this macro MUST be defined here, in the crate's root module, because Rust
//         doesn't allow to export `macro_rules!` macros from a `proc-macro` crate type currently,
//         and so we cannot move the definition into a sub-module and use the `#[macro_export]`
//         attribute.
/// Attempts to merge an [`Option`]ed `$field` of a `$self` struct with the same `$field` of
/// `$another` struct. If both are [`Some`], then throws a duplication error with a [`Span`] related
/// to the `$another` struct (a later one).
///
/// The type of [`Span`] may be explicitly specified as one of the [`SpanContainer`] methods.
/// By default, [`SpanContainer::span_ident`] is used.
///
/// [`Span`]: proc_macro2::Span
/// [`SpanContainer`]: crate::common::SpanContainer
/// [`SpanContainer::span_ident`]: crate::common::SpanContainer::span_ident
macro_rules! try_merge_opt {
    ($field:ident: $self:ident, $another:ident => $span:ident) => {{
        if let Some(v) = $self.$field {
            $another
                .$field
                .replace(v)
                .none_or_else(|dup| crate::common::parse::attr::err::dup_arg(&dup.$span()))?;
        }
        $another.$field
    }};

    ($field:ident: $self:ident, $another:ident) => {
        try_merge_opt!($field: $self, $another => span_ident)
    };
}

use proc_macro::TokenStream;
use proc_macro_error::proc_macro_error;

mod common;
mod osc_module;

/// `#[osc_module]` macro for generating an [OSConfig module][1] implementation.
///
/// It enables you to write property resolvers for a single-component module by
/// declaring a regular Rust `impl` block for a struct. Under the hood, the macro
/// implements the module interface and handles all FFI type conversions.
///
/// Desired and reported properties are declared as methods on the impl block of
/// the struct. fields definited on the struct can be used for storing state
/// between method calls, and can be accessed from prooperty resolvers.
///
/// ```rust
/// use osc::osc_module;
///
/// struct Hostname {
///     name: String
/// }
///
/// #[osc_module(
///    name = "Hostname",
///    description = "Provides functionality to observe and configure network hostname",
///    manufacturer = "Microsoft",
///    version = "1.0"
/// )]
/// impl Hostname {
///     #[osc(reported)]
///     fn name(&self) -> String {
///         self.name.clone()
///     }
///
///     #[osc(desired)]
///     fn desired_name(&mut self, name: String) {
///         self.name = name;
///     }
/// }
///  ```
///
/// [1]: https://github.com/Azure/azure-osconfig/blob/main/docs/modules.md
#[proc_macro_error]
#[proc_macro_attribute]
pub fn osc_module(args: TokenStream, input: TokenStream) -> TokenStream {
    osc_module::attr::expand(args.into(), input.into())
        .unwrap()
        .into()
}
