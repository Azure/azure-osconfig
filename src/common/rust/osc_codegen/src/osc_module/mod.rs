//! Code generation for [OSConfig modules][1].
//!
//! [1]: https://github.com/Azure/azure-osconfig/blob/main/docs/modules.md

use proc_macro2::TokenStream;
use quote::{quote, ToTokens};
use syn::{
    parse::{Parse, ParseStream},
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

pub mod attr;
mod property;

use self::attr::{Lifetime, UserAccount, Version};

/// Available arguments behind `#[osc]` (or `#[osc_module]`) attribute
/// when generating code for an [OSConfig module][1].
///
/// [1]: https://github.com/Azure/azure-osconfig/blob/main/docs/modules.md
#[derive(Debug, Default)]
pub(crate) struct Attr {
    /// Name of the module.
    pub(crate) name: Option<SpanContainer<String>>,

    /// Description of the module.
    pub(crate) description: Option<SpanContainer<String>>,

    /// Manufacturer of the module.
    pub(crate) manufacturer: Option<SpanContainer<String>>,

    /// Version of the module.
    pub(crate) version: Option<SpanContainer<Version>>,

    /// Lifetime of the module.
    pub(crate) lifetime: Option<SpanContainer<Lifetime>>,

    /// User account of the module.
    pub(crate) user_account: Option<SpanContainer<UserAccount>>,
}

impl Parse for Attr {
    fn parse(input: ParseStream<'_>) -> syn::Result<Self> {
        let mut out = Self::default();
        while !input.is_empty() {
            let ident = input.parse_any_ident()?;
            match ident.to_string().as_str() {
                "name" => {
                    input.parse::<token::Eq>()?;
                    let name = input.parse::<syn::LitStr>()?;
                    out.name
                        .replace(SpanContainer::new(
                            ident.span(),
                            Some(name.span()),
                            name.value(),
                        ))
                        .none_or_else(|_| err::dup_arg(&ident))?
                }
                "desc" | "description" => {
                    input.parse::<token::Eq>()?;
                    let desc = input.parse::<syn::LitStr>()?;
                    out.description
                        .replace(SpanContainer::new(
                            ident.span(),
                            Some(desc.span()),
                            desc.value(),
                        ))
                        .none_or_else(|_| err::dup_arg(&ident))?
                }
                "manufacturer" => {
                    input.parse::<token::Eq>()?;
                    let manufacturer = input.parse::<syn::LitStr>()?;
                    out.manufacturer
                        .replace(SpanContainer::new(
                            ident.span(),
                            Some(manufacturer.span()),
                            manufacturer.value(),
                        ))
                        .none_or_else(|_| err::dup_arg(&ident))?
                }
                "version" => {
                    input.parse::<token::Eq>()?;
                    let version = input.parse::<Version>()?;
                    out.version
                        .replace(SpanContainer::new(ident.span(), None, version))
                        .none_or_else(|_| err::dup_arg(&ident))?
                }
                "lifetime" => {
                    input.parse::<token::Eq>()?;
                    let lifetime = input.parse::<Lifetime>()?;
                    out.lifetime
                        .replace(SpanContainer::new(ident.span(), None, lifetime))
                        .none_or_else(|_| err::dup_arg(&ident))?
                }
                "user" | "user_account" => {
                    input.parse::<token::Eq>()?;
                    let user_account = input.parse::<UserAccount>()?;
                    out.user_account
                        .replace(SpanContainer::new(ident.span(), None, user_account))
                        .none_or_else(|_| err::dup_arg(&ident))?
                }
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
            description: try_merge_opt!(description: self, another),
            manufacturer: try_merge_opt!(manufacturer: self, another),
            version: try_merge_opt!(version: self, another),
            lifetime: try_merge_opt!(lifetime: self, another),
            user_account: try_merge_opt!(user_account: self, another),
        })
    }

    /// Parses [`Attr`] from the given multiple `name`d [`syn::Attribute`]s
    /// placed on a struct or impl block definition.
    pub(crate) fn from_attrs(name: &str, attrs: &[syn::Attribute]) -> syn::Result<Self> {
        filter_attrs(name, attrs)
            .map(|attr| attr.parse_args())
            .try_fold(Self::default(), |prev, curr| prev.try_merge(curr?))
    }
}

/// Definition of a module for code generation.
pub(crate) struct Definition {
    /// Name of this module.
    pub(crate) name: String,

    /// Rust type that this module is represented with.
    pub(crate) ty: syn::Type,

    /// Description of this module to use in the module metadata.
    pub(crate) description: Option<String>,

    /// The manufacturer of this module to use in the module metadata.
    pub(crate) manufacturer: Option<String>,

    /// The version of this module to use in the module metadata.
    pub(crate) version: Version,

    /// The lifetime of this module to use in the module metadata.
    pub(crate) lifetime: Lifetime,

    /// The user account of this module to use in the module metadata.
    pub(crate) user_account: UserAccount,

    /// The properties of this module.
    pub(crate) props: Vec<property::Definition>,
}

impl Definition {
    fn impl_meta_tokens(&self) -> TokenStream {
        let name = &self.name;

        let description = self
            .description
            .as_ref()
            .map(|s| quote! { Some(#s.to_string()) })
            .unwrap_or_else(|| quote! { None });

        let manufacturer = self
            .manufacturer
            .as_ref()
            .map(|s| quote! { Some(#s.to_string()) })
            .unwrap_or_else(|| quote! { None });

        let (lifetime, user_account, version) = (&self.lifetime, &self.user_account, &self.version);

        quote! {
            ::osc::module::Meta {
                name: #name.to_string(),
                description: #description,
                manufacturer: #manufacturer,
                version: #version,
                components: vec![#name.to_string()],
                lifetime: #lifetime,
                user_account: #user_account,
            }
        }
    }

    fn impl_osc_module_meta_tokens(&self) -> TokenStream {
        let meta = self.impl_meta_tokens();

        quote! {
            #[no_mangle]
            pub extern "C" fn MmiGetInfo(
                client: *const ::std::ffi::c_char,
                payload: *mut ::osc::module::bind::JsonString,
                payload_size: *mut ::std::ffi::c_int,
            ) -> ::std::ffi::c_int {
                if client.is_null() {
                    ::osc::log::error!("invalid 'null' client name");
                    return ::osc::module::bind::libc::EINVAL;
                }

                if payload.is_null() {
                    ::osc::log::error!("invalid 'null' payload");
                    return ::osc::module::bind::libc::EINVAL;
                }

                if payload_size.is_null() {
                    ::osc::log::error!("invalid 'null' payload size");
                    return ::osc::module::bind::libc::EINVAL;
                }

                unsafe {
                    *payload = ::std::ptr::null_mut();
                    *payload_size = 0;
                }

                let meta = #meta;

                let json = meta.to_string();
                let json = ::std::ffi::CString::new(json.as_str()).unwrap();
                let size = json.as_bytes().len() as ::std::ffi::c_int;

                unsafe {
                    *payload = json.into_raw();
                    *payload_size = size as ::std::ffi::c_int;
                };

                ::osc::module::bind::OK
            }
        }
    }

    fn impl_osc_module_open_tokens(&self) -> TokenStream {
        let ty = &self.ty;

        quote! {
            #[no_mangle]
            pub extern "C" fn MmiOpen(
                client: *const ::std::ffi::c_char,
                max_size: ::std::ffi::c_uint
            ) -> *mut ::std::ffi::c_void {
                ::osc::log::init();

                if client.is_null() {
                    ::osc::log::error!("invalid 'null' client name");
                    return std::ptr::null_mut();
                }

                let instance = Box::new(#ty::default());
                Box::into_raw(instance) as *mut ::std::ffi::c_void
            }
        }
    }

    fn impl_osc_module_close_tokens(&self) -> TokenStream {
        let ty = &self.ty;
        quote! {
            #[no_mangle]
            pub extern "C" fn MmiClose(handle: ::osc::module::bind::Handle) {
                if !handle.is_null() {
                    let _ = unsafe { Box::from_raw(handle as *mut #ty) };
                }
            }
        }
    }

    fn impl_osc_module_set_tokens(&self) -> TokenStream {
        let (name, ty) = (&self.name, &self.ty);

        let desired = self
            .props
            .iter()
            .filter(|prop| prop.is_write())
            .map(|prop| prop.method_resolve_desired_prop_tokens(ty));

        quote! {
            #[no_mangle]
            pub extern "C" fn MmiSet(
                handle: ::osc::module::bind::Handle,
                component: *const ::std::ffi::c_char,
                property: *const ::std::ffi::c_char,
                payload: ::osc::module::bind::JsonString,
                payload_size: ::std::ffi::c_int,
            ) -> ::std::ffi::c_int {
                if handle.is_null() {
                    ::osc::log::error!("invalid 'null' handle");
                    return ::osc::module::bind::libc::EINVAL;
                }

                if component.is_null() {
                    ::osc::log::error!("invalid 'null' component");
                    return ::osc::module::bind::libc::EINVAL;
                }

                if property.is_null() {
                    ::osc::log::error!("invalid 'null' property");
                    return ::osc::module::bind::libc::EINVAL;
                }

                let component = unsafe { std::ffi::CStr::from_ptr(component) };
                let component = component.to_str().unwrap();

                if #name != component {
                    ::osc::log::error!("invalid component name: {}", component);
                    return ::osc::module::bind::libc::EINVAL;
                }

                let property = unsafe { std::ffi::CStr::from_ptr(property) };
                let property = property.to_str().unwrap();

                let payload = unsafe { std::slice::from_raw_parts(payload as *const u8, payload_size as usize) };
                let payload = String::from_utf8_lossy(payload).to_string();

                let instance: &mut #ty = unsafe { &mut *(handle as *mut #ty) };

                let result = match property {
                    #(#desired)*
                    _ => {
                        ::osc::module::PropertyResult::Err(
                            ::osc::module::PropertyError::new(
                                format!("invalid property name: {}.{}", component, property)
                            )
                        )
                    }
                };

                match result {
                    Ok(_) => ::osc::module::bind::OK,
                    Err(err) => {
                        ::osc::log::error!("{}", err);
                        ::osc::module::bind::ERROR
                    }
                }
            }
        }
    }

    fn impl_osc_module_get_tokens(&self) -> TokenStream {
        let (name, ty) = (&self.name, &self.ty);

        let reported = self
            .props
            .iter()
            .filter(|prop| prop.is_read())
            .map(|prop| prop.method_resolve_reported_prop_tokens(ty));

        quote! {
            #[no_mangle]
            pub extern "C" fn MmiGet(
                handle: ::osc::module::bind::Handle,
                component: *const ::std::ffi::c_char,
                property: *const ::std::ffi::c_char,
                payload: *mut ::osc::module::bind::JsonString,
                payload_size: *mut ::std::ffi::c_int,
            ) -> ::std::ffi::c_int {
                if handle.is_null() {
                    ::osc::log::error!("invalid 'null' handle");
                    return ::osc::module::bind::libc::EINVAL;
                }

                if component.is_null() {
                    ::osc::log::error!("invalid 'null' component");
                    return ::osc::module::bind::libc::EINVAL;
                }

                if property.is_null() {
                    ::osc::log::error!("invalid 'null' property");
                    return ::osc::module::bind::libc::EINVAL;
                }

                if payload.is_null() {
                    ::osc::log::error!("invalid 'null' payload");
                    return ::osc::module::bind::libc::EINVAL;
                }

                if payload_size.is_null() {
                    ::osc::log::error!("invalid 'null' payload size");
                    return ::osc::module::bind::libc::EINVAL;
                }

                unsafe {
                    *payload = std::ptr::null_mut();
                    *payload_size = 0;
                }

                let component = unsafe { ::std::ffi::CStr::from_ptr(component) };
                let component = component.to_str().unwrap();

                if #name != component {
                    ::osc::log::error!("invalid component name: {}", component);
                    return ::osc::module::bind::libc::EINVAL;
                }

                let property = unsafe { ::std::ffi::CStr::from_ptr(property) };
                let property = property.to_str().unwrap();

                let instance: &#ty = unsafe { &*(handle as *const #ty) };

                let res = match property {
                    #(#reported)*
                    _ => {
                        ::osc::module::PropertyResult::Err(
                            ::osc::module::PropertyError::new(
                                format!("invalid property name: {}.{}", component, property)
                            )
                        )
                    }
                };

                match res {
                    Ok(val) => {
                        let json = ::std::ffi::CString::new(val).unwrap();
                        let size = json.as_bytes().len() as ::std::ffi::c_int;

                        unsafe {
                            *payload = json.into_raw();
                            *payload_size = size as ::std::ffi::c_int;
                        };

                        ::osc::module::bind::OK
                    }
                    Err(err) => {
                        ::osc::log::error!("{}", err);
                        -1
                    }
                }
            }
        }
    }

    fn impl_osc_module_free_tokens(&self) -> TokenStream {
        quote! {
            #[no_mangle]
            pub extern "C" fn MmiFree(ptr: *mut ::std::ffi::c_char) {
                if !ptr.is_null() {
                    let _ = unsafe { Box::from_raw(ptr as *mut ::std::ffi::c_char) };
                }
            }
        }
    }
}

impl ToTokens for Definition {
    fn to_tokens(&self, into: &mut TokenStream) {
        self.impl_osc_module_meta_tokens().to_tokens(into);
        self.impl_osc_module_open_tokens().to_tokens(into);
        self.impl_osc_module_close_tokens().to_tokens(into);
        self.impl_osc_module_set_tokens().to_tokens(into);
        self.impl_osc_module_get_tokens().to_tokens(into);
        self.impl_osc_module_free_tokens().to_tokens(into);
    }
}
