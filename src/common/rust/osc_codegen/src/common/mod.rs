//! Common functions, definitions and extensions for code generation, used by this crate.

pub(crate) mod diagnostic;
pub(crate) mod parse;
mod span_container;

pub(crate) use self::span_container::SpanContainer;

/// Filters the provided [`syn::Attribute`] to contain only ones with the
/// specified `name`.
pub(crate) fn filter_attrs<'a>(
    name: &'a str,
    attrs: &'a [syn::Attribute],
) -> impl Iterator<Item = &'a syn::Attribute> + 'a {
    attrs
        .iter()
        .filter(move |attr| path_eq_single(&attr.path(), name))
}

/// Checks whether the specified [`syn::Path`] equals to one-segment string
/// `value`.
pub(crate) fn path_eq_single(path: &syn::Path, value: &str) -> bool {
    path.segments.len() == 1 && path.segments[0].ident == value
}