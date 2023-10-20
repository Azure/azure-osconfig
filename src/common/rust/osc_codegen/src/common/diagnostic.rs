use std::fmt;

use proc_macro2::Span;
use proc_macro_error::{Diagnostic, Level};

pub(crate) enum Scope {
    ModuleAttr,
}

impl fmt::Display for Scope {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        let name = match self {
            Self::ModuleAttr => "module",
        };
        write!(f, "OSConfig {name}")
    }
}

impl Scope {
    pub(crate) fn custom<S: AsRef<str>>(&self, span: Span, msg: S) -> Diagnostic {
        Diagnostic::spanned(span, Level::Error, format!("{self} {}", msg.as_ref()))
    }

    pub(crate) fn emit_custom<S: AsRef<str>>(&self, span: Span, msg: S) {
        self.custom(span, msg).emit()
    }

    pub(crate) fn custom_error<S: AsRef<str>>(&self, span: Span, msg: S) -> syn::Error {
        syn::Error::new(span, format!("{self} {}", msg.as_ref()))
    }
}