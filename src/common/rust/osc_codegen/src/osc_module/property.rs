//! Common functions, definitions and extensions for parsing and code generation
//! of module properties

use proc_macro2::TokenStream;
use quote::quote;
use syn::{
    parse::{Parse, ParseStream},
    spanned::Spanned as _,
    token,
};

use crate::common::{
    filter_attrs,
    parse::{
        attr::{err, OptionExt as _},
        ParseBufferExt as _,
    },
    SpanContainer,
};

/// Available metadata (arguments) behind `#[osc]` attribute placed on a
/// property definition.
#[derive(Default)]
pub struct Attr {
    /// Explicitly specified name of this property.
    ///
    /// If [`None`], then `camelCased` Rust method name is used by default.
    pub(crate) name: Option<SpanContainer<syn::LitStr>>,

    /// The access type of this property.
    pub(crate) access: Option<SpanContainer<AccessType>>,
}

#[derive(PartialEq)]
pub enum AccessType {
    Read,
    Write,
}

impl Parse for Attr {
    fn parse(input: ParseStream<'_>) -> syn::Result<Self> {
        let mut out = Self::default();
        while !input.is_empty() {
            let ident = input.parse::<syn::Ident>()?;
            match ident.to_string().as_str() {
                "name" => {
                    input.parse::<token::Eq>()?;
                    let name = input.parse::<syn::LitStr>()?;
                    out.name
                        .replace(SpanContainer::new(ident.span(), Some(name.span()), name))
                        .none_or_else(|_| err::dup_arg(&ident))?
                }
                "reported" | "read" => out
                    .access
                    .replace(SpanContainer::new(ident.span(), None, AccessType::Read))
                    .none_or_else(|_| err::dup_arg(&ident))?,

                "desired" | "write" => out
                    .access
                    .replace(SpanContainer::new(ident.span(), None, AccessType::Write))
                    .none_or_else(|_| err::dup_arg(&ident))?,
                name => {
                    return Err(err::unknown_arg(&ident, name));
                }
            }
            input.try_parse::<token::Comma>()?;
        }
        Ok(out)
    }
}

impl Attr {
    /// Tries to merge two [`Attr`]s into a single one, reporting about
    /// duplicates, if any.
    fn try_merge(self, mut another: Self) -> syn::Result<Self> {
        Ok(Self {
            name: try_merge_opt!(name: self, another),
            access: try_merge_opt!(access: self, another),
        })
    }

    /// Parses [`Attr`] from the given multiple `name`d [`syn::Attribute`]s
    /// placed on a property definition.
    pub fn from_attrs(name: &str, attrs: &[syn::Attribute]) -> syn::Result<Self> {
        let attr = filter_attrs(name, attrs)
            .map(|attr| attr.parse_args())
            .try_fold(Self::default(), |prev, curr| prev.try_merge(curr?))?;

        if let None = &attr.access {
            return Err(syn::Error::new(
                name.span(),
                "missing `reported` or `desired` attribute argument",
            ));
        }

        Ok(attr)
    }
}

/// Representation of an property for code generation.
pub struct Definition {
    /// Rust type that this property is represented by (method return
    /// type).
    pub ty: syn::Type,

    /// Name of this property in the schema.
    pub name: String,

    /// Ident of the Rust method (or struct field) representing this
    /// property.
    pub ident: syn::Ident,

    /// Rust [`syn::Type`]s required to call the method representing this
    /// property.
    pub arguments: Option<Vec<syn::Type>>,

    /// Indicator whether the Rust method representing this property
    /// has a [`syn::Receiver`].
    pub has_receiver: bool,

    /// The access type of this property (read or write).
    pub access: AccessType,
}

impl Definition {
    #[must_use]
    pub fn method_resolve_reported_prop_tokens(&self, ty_name: &syn::Type) -> TokenStream {
        let (name, ident) = (&self.name, &self.ident);

        let rcv = self.has_receiver.then(|| {
            quote! { &instance }
        });

        quote! {
             #name => {
                ::osc::module::IntoPayload::into_payload(
                    ::osc::module::IntoPropertyResult::into_result(
                        #ty_name::#ident(#rcv)
                    )
                )
            }
        }
    }

    #[must_use]
    pub fn method_resolve_desired_prop_tokens(&self, ty_name: &syn::Type) -> TokenStream {
        let (name, ident) = (&self.name, &self.ident);

        let args = self.arguments.as_ref().unwrap().iter().map(|arg| {
            quote! {
                ::osc::module::IntoPropertyArgument::<#arg>::into_arg(payload)
            }
        });

        let rcv = self.has_receiver.then(|| {
            quote! { instance, }
        });

        quote! {
             #name => {
                ::osc::module::IntoPropertyResult::into_result(
                    #ty_name::#ident(#rcv #( #args ),*)
                )
            }
        }
    }

    #[must_use]
    pub fn is_read(&self) -> bool {
        self.access == AccessType::Read
    }

    #[must_use]
    pub fn is_write(&self) -> bool {
        self.access == AccessType::Write
    }
}

/// Checks whether all properties have different names
#[must_use]
pub fn all_different(props: &[Definition]) -> bool {
    let mut names: Vec<_> = props.iter().map(|def| &def.name).collect();
    names.dedup();
    names.len() == props.len()
}
