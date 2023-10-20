//! Code generation for `#[osc_module]` macro.

use std::mem;

use proc_macro2::{Span, TokenStream};
use quote::{quote, ToTokens};
use syn::{ext::IdentExt as _, parse_quote, spanned::Spanned};

use crate::common::{
    diagnostic,
    parse::{self, to_camel_case, TypeExt as _},
    path_eq_single, SpanContainer,
};

use super::{property, Attr, Definition};

mod lifetime;
mod user_account;
mod version;

pub(crate) use lifetime::Lifetime;
pub(crate) use user_account::UserAccount;
pub(crate) use version::Version;

/// [`diagnostic::Scope`] of errors for `#[osc_module]` macro.
const ERR: diagnostic::Scope = diagnostic::Scope::ModuleAttr;

/// Expands `#[osc_module]` macro into generated code.
pub fn expand(attr_args: TokenStream, body: TokenStream) -> syn::Result<TokenStream> {
    if let Ok(mut ast) = syn::parse2::<syn::ItemImpl>(body) {
        if ast.trait_.is_none() {
            let impl_attrs = parse::attr::unite(("osc_module", &attr_args), &ast.attrs);
            ast.attrs = parse::attr::strip("osc_module", ast.attrs);
            return expand_on_impl(Attr::from_attrs("osc_module", &impl_attrs)?, ast);
        }
    }

    Err(syn::Error::new(
        Span::call_site(),
        "#[osc_module] attribute is applicable to non-trait `impl` blocks only",
    ))
}

/// Expands `#[osc_module]` macro placed on an implementation block.
fn expand_on_impl(attr: Attr, mut ast: syn::ItemImpl) -> syn::Result<TokenStream>
where
    Definition: ToTokens,
{
    let type_span = ast.self_ty.span();
    let type_ident = ast.self_ty.topmost_ident().ok_or_else(|| {
        ERR.custom_error(type_span, "could not determine ident for the `impl` type")
    })?;

    let name = attr
        .name
        .clone()
        .map(SpanContainer::into_inner)
        .unwrap_or_else(|| type_ident.unraw().to_string());

    proc_macro_error::abort_if_dirty();

    let props: Vec<_> = ast
        .items
        .iter_mut()
        .filter_map(|item| {
            if let syn::ImplItem::Fn(f) = item {
                parse_property(f)
            } else {
                None
            }
        })
        .collect();

    proc_macro_error::abort_if_dirty();

    if !property::all_different(&props) {
        ERR.emit_custom(
            type_span,
            "must have a different name for each reported field",
        );
    }

    proc_macro_error::abort_if_dirty();

    let generated_code = Definition {
        name,
        ty: ast.self_ty.unparenthesized().clone(),
        description: attr.description.map(SpanContainer::into_inner),
        manufacturer: attr.manufacturer.map(SpanContainer::into_inner),
        version: attr
            .version
            .map(SpanContainer::into_inner)
            .unwrap_or_default(),
        lifetime: attr
            .lifetime
            .map(SpanContainer::into_inner)
            .unwrap_or_default(),
        user_account: attr
            .user_account
            .map(SpanContainer::into_inner)
            .unwrap_or_default(),
        props,
    };

    Ok(quote! {
        #ast
        #generated_code
    })
}

/// Parses a [`property::Definition`] from the given Rust [`syn::ImplItemMethod`].
///
/// Returns [`None`] if parsing fails, or the method field is ignored.
#[must_use]
fn parse_property(method: &mut syn::ImplItemFn) -> Option<property::Definition> {
    let method_attrs = method.attrs.clone();

    // If the method does not have `#[osc]` attribute, ignore it.
    if !method_attrs
        .iter()
        .any(|attr| path_eq_single(&attr.path(), "osc"))
    {
        return None;
    }

    // Remove repeated attributes from the method, to omit incorrect expansion.
    method.attrs = mem::take(&mut method.attrs)
        .into_iter()
        .filter(|attr| !path_eq_single(&attr.path(), "osc"))
        .collect();

    let attr = property::Attr::from_attrs("osc", &method_attrs).ok()?;

    let method_ident = &method.sig.ident;

    let name = attr
        .name
        .as_ref()
        .map(|m| m.as_ref().value())
        .unwrap_or(method_ident.unraw().to_string());

    let name = to_camel_case(&name).into_owned();

    let arguments: Vec<_> = {
        if let Some(arg) = method.sig.inputs.first() {
            match arg {
                syn::FnArg::Receiver(rcv) => {
                    if rcv.reference.is_none() {
                        return err_invalid_method_receiver(rcv);
                    }
                }
                syn::FnArg::Typed(arg) => {
                    if let syn::Pat::Ident(a) = &*arg.pat {
                        if a.ident == "self" {
                            return err_invalid_method_receiver(arg);
                        }
                    }
                }
            }
        }

        method
            .sig
            .inputs
            .iter_mut()
            .filter_map(|arg| match arg {
                syn::FnArg::Receiver(_) => None,
                syn::FnArg::Typed(arg) => Some(arg.ty.as_ref().clone()),
            })
            .collect()
    };

    let access = attr.access.map(SpanContainer::into_inner)?;

    match access {
        property::AccessType::Read => {
            if !arguments.is_empty() {
                ERR.emit_custom(
                    method.sig.inputs.span(),
                    "reported method should not have any arguments",
                );
            }
        }
        property::AccessType::Write => {
            if arguments.is_empty() {
                ERR.emit_custom(
                    method.sig.inputs.span(),
                    "desired method should have one argument",
                );
            }
        }
    }

    let ty = match &method.sig.output {
        syn::ReturnType::Default => parse_quote! { () },
        syn::ReturnType::Type(_, ty) => ty.unparenthesized().clone(),
    };

    Some(property::Definition {
        name,
        ty,
        ident: method_ident.clone(),
        arguments: Some(arguments),
        has_receiver: method.sig.receiver().is_some(),
        access,
    })
}

/// Emits "invalid method receiver" [`syn::Error`] pointing to the given `span`.
#[must_use]
fn err_invalid_method_receiver<T, S: Spanned>(span: &S) -> Option<T> {
    ERR.emit_custom(
        span.span(),
        "method should have a shared reference receiver `&self`, or no receiver at all",
    );
    None
}
