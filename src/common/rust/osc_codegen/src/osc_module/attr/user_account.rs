use quote::ToTokens;
use syn::parse::{Parse, ParseStream};

#[derive(Debug)]
pub struct UserAccount(i32);

impl Parse for UserAccount {
    fn parse(input: ParseStream<'_>) -> syn::Result<Self> {
        let user_account = input.parse::<syn::LitInt>()?;
        let value = user_account.base10_parse::<i32>()?;

        Ok(UserAccount(value))
    }
}

impl Default for UserAccount {
    fn default() -> Self {
        UserAccount(0)
    }
}

impl ToTokens for UserAccount {
    fn to_tokens(&self, tokens: &mut proc_macro2::TokenStream) {
        let user_account = self.0;
        tokens.extend(quote::quote! { ::osc::module::UserAccount::from(#user_account) });
    }
}
