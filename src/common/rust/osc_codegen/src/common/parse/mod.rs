//! Common functions, definitions and extensions for parsing, normalizing and modifying Rust syntax,
//! used by this crate.

pub(crate) mod attr;

use std::borrow::Cow;

use syn::{
    ext::IdentExt as _,
    parse::{Parse, ParseBuffer},
    token::Token,
};

/// Extension of [`ParseBuffer`] providing common function widely used by this crate for parsing.
pub(crate) trait ParseBufferExt {
    /// Tries to parse `T` as the next token.
    ///
    /// Doesn't move [`ParseStream`]'s cursor if there is no `T`.
    fn try_parse<T: Default + Parse + Token>(&self) -> syn::Result<Option<T>>;

    /// Checks whether next token is `T`.
    ///
    /// Doesn't move [`ParseStream`]'s cursor.
    #[must_use]
    fn is_next<T: Default + Token>(&self) -> bool;

    /// Parses next token as [`syn::Ident`] _allowing_ Rust keywords, while default [`Parse`]
    /// implementation for [`syn::Ident`] disallows keywords.
    ///
    /// Always moves [`ParseStream`]'s cursor.
    fn parse_any_ident(&self) -> syn::Result<syn::Ident>;
}

impl<'a> ParseBufferExt for ParseBuffer<'a> {
    fn try_parse<T: Default + Parse + Token>(&self) -> syn::Result<Option<T>> {
        Ok(if self.is_next::<T>() {
            Some(self.parse()?)
        } else {
            None
        })
    }

    fn is_next<T: Default + Token>(&self) -> bool {
        self.lookahead1().peek(|_| T::default())
    }

    fn parse_any_ident(&self) -> syn::Result<syn::Ident> {
        self.call(syn::Ident::parse_any)
    }
}

/// Extension of [`syn::Type`] providing common function widely used by this crate for parsing.
pub(crate) trait TypeExt {
    /// Retrieves the innermost non-parenthesized [`syn::Type`] from the given
    /// one (unwraps nested [`syn::TypeParen`]s asap).
    #[must_use]
    fn unparenthesized(&self) -> &Self;

    /// Returns the topmost [`syn::Ident`] of this [`syn::TypePath`], if any.
    #[must_use]
    fn topmost_ident(&self) -> Option<&syn::Ident>;
}

impl TypeExt for syn::Type {
    fn unparenthesized(&self) -> &Self {
        match self {
            Self::Paren(ty) => ty.elem.unparenthesized(),
            Self::Group(ty) => ty.elem.unparenthesized(),
            ty => ty,
        }
    }

    fn topmost_ident(&self) -> Option<&syn::Ident> {
        match self.unparenthesized() {
            syn::Type::Path(p) => Some(&p.path),
            syn::Type::Reference(r) => match (*r.elem).unparenthesized() {
                syn::Type::Path(p) => Some(&p.path),
                syn::Type::TraitObject(o) => match o.bounds.iter().next().unwrap() {
                    syn::TypeParamBound::Trait(b) => Some(&b.path),
                    _ => None,
                },
                _ => None,
            },
            _ => None,
        }?
        .segments
        .last()
        .map(|s| &s.ident)
    }
}

/// Convert string to camel case.
#[doc(hidden)]
pub fn to_camel_case(s: &'_ str) -> Cow<'_, str> {
    let mut dest = Cow::Borrowed(s);

    // handle '_' to be more friendly with the
    // _var convention for unused variables
    let s_iter = if let Some(stripped) = s.strip_prefix('_') {
        stripped
    } else {
        s
    }
    .split('_')
    .enumerate();

    for (i, part) in s_iter {
        if i > 0 && part.len() == 1 {
            dest += Cow::Owned(part.to_uppercase());
        } else if i > 0 && part.len() > 1 {
            let first = part
                .chars()
                .next()
                .unwrap()
                .to_uppercase()
                .collect::<String>();
            let second = &part[1..];

            dest += Cow::Owned(first);
            dest += second;
        } else if i == 0 {
            dest = Cow::Borrowed(part);
        }
    }

    dest
}
