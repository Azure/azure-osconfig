use quote::ToTokens;
use syn::parse::{Parse, ParseStream};

#[derive(Debug, Eq, PartialEq)]
pub enum Lifetime {
    Short,
    Long
}

impl Parse for Lifetime {
    fn parse(input: ParseStream<'_>) -> syn::Result<Self> {
        let lifetime = input.parse::<syn::LitStr>()?;
        let value = lifetime.value();

        match value.as_str() {
            "short" => Ok(Lifetime::Short),
            "long" => Ok(Lifetime::Long),
            _ => Err(syn::Error::new(lifetime.span(), "Invalid lifetime"))
        }
    }
}

impl Default for Lifetime {
    fn default() -> Self {
        Lifetime::Long
    }
}

impl ToTokens for Lifetime {
    fn to_tokens(&self, tokens: &mut proc_macro2::TokenStream) {
        let lifetime = match self {
            Lifetime::Short => quote::format_ident!("Short"),
            Lifetime::Long => quote::format_ident!("Long"),
        };

        tokens.extend(quote::quote! {
            ::osc::module::Lifetime::#lifetime
        });
    }
}