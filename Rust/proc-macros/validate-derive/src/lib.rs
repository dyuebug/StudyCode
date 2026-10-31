// ============================================================================
//  过程宏实操 · #[derive(Validate)]
// ============================================================================
//  日期：2026-11-09（第45天）
//  难度：⭐⭐⭐⭐
//
//  这个 crate 只有一件事：把字段上的 `#[validate(...)]` 变成真正的校验代码。
//
//  为什么必须单独成一个 crate？
//    `proc-macro = true` 的 crate 不能导出普通函数/类型，只能导出宏。
//    所以「宏」和「宏生成代码所依赖的类型」必须分家 —— 这就是
//    `validate-derive`（宏）+ `validate-demo`（消费者）两个 crate 的原因，
//    也是所有 derive 库（serde / thiserror / clap）的真实结构。
//
//  输入（消费者写的）：
//      #[derive(Validate)]
//      struct User {
//          #[validate(min_len = 3, max_len = 20)]
//          name: String,
//          #[validate(min = 1, max = 150)]
//          age: u8,
//      }
//
//  输出（本宏生成的）：
//      impl User {
//          pub fn validate(&self) -> Result<(), Vec<String>> {
//              let mut errs: Vec<String> = Vec::new();
//              let __len_name = self.name.chars().count();
//              if __len_name < 3 { errs.push(format!("name: 长度 {} < 最小 {}", __len_name, 3)); }
//              if __len_name > 20 { errs.push(format!("name: 长度 {} > 最大 {}", __len_name, 20)); }
//              if self.age < 1 { errs.push(format!("age: {} < 最小 {}", self.age, 1)); }
//              if self.age > 150 { errs.push(format!("age: {} > 最大 {}", self.age, 150)); }
//              if errs.is_empty() { Ok(()) } else { Err(errs) }
//          }
//      }
//
//  三条关键认知：
//    1. 过程宏的输入是 TokenStream，不是字符串 —— 所以「解析」要靠 syn，不是正则
//    2. 报错要用 `syn::Error::to_compile_error()`，它会把错误定位到用户写的
//       那一行（`new_spanned` 的 span 决定箭头指向哪里），而不是宏自己的源码
//    3. 生成代码里的标识符（`__len_name`）必须用 `format_ident!` 拼，
//       手写 `format!` 再 parse 会把 span 丢掉，报错位置全错
// ============================================================================

use proc_macro::TokenStream;
use proc_macro2::TokenStream as TokenStream2;
use quote::{format_ident, quote};
use syn::{parse_macro_input, Data, DeriveInput, Fields, LitInt};

/// 字段级规则。四个都是 `Option`，因为一个字段可能只约束一端。
#[derive(Default)]
struct FieldRule {
    /// 字符数下限（不是字节数：中文按字算才符合直觉）
    min_len: Option<LitInt>,
    /// 字符数上限
    max_len: Option<LitInt>,
    /// 数值下限
    min: Option<LitInt>,
    /// 数值上限
    max: Option<LitInt>,
}

#[proc_macro_derive(Validate, attributes(validate))]
pub fn derive_validate(input: TokenStream) -> TokenStream {
    // parse_macro_input! 会在语法错误时直接返回 compile_error!，不会 panic
    let input = parse_macro_input!(input as DeriveInput);

    // 关键：把 Err 转成 compile_error! 而不是 unwrap()。
    // unwrap() 的报错会指向「宏内部」，用户看不懂；to_compile_error() 指向用户的字段。
    expand(input)
        .unwrap_or_else(|err| err.to_compile_error())
        .into()
}

fn expand(input: DeriveInput) -> syn::Result<TokenStream2> {
    let name = &input.ident;

    // 只支持具名字段：错误消息里要用字段名，元组结构体没有名字可写
    let fields = match &input.data {
        Data::Struct(data) => match &data.fields {
            Fields::Named(named) => &named.named,
            _ => {
                return Err(syn::Error::new_spanned(
                    name,
                    "#[derive(Validate)] 只支持具名（named-field）结构体：错误消息需要字段名",
                ))
            }
        },
        _ => {
            return Err(syn::Error::new_spanned(
                name,
                "#[derive(Validate)] 只能用于 struct，不能用于 enum / union",
            ))
        }
    };

    let mut checks = TokenStream2::new();

    for field in fields {
        // 上面已经排除了元组结构体，这里一定有 ident
        let ident = field.ident.as_ref().expect("具名字段必然有 ident");
        let field_name = ident.to_string();
        let mut rule = FieldRule::default();

        for attr in &field.attrs {
            if !attr.path().is_ident("validate") {
                continue;
            }
            // parse_nested_meta 是 syn 2 的正规写法：
            // 它按 `key = value` / `key(...)` 的语法逐个回调，不需要自己写 parser
            attr.parse_nested_meta(|meta| {
                if meta.path.is_ident("min_len") {
                    rule.min_len = Some(meta.value()?.parse()?);
                } else if meta.path.is_ident("max_len") {
                    rule.max_len = Some(meta.value()?.parse()?);
                } else if meta.path.is_ident("min") {
                    rule.min = Some(meta.value()?.parse()?);
                } else if meta.path.is_ident("max") {
                    rule.max = Some(meta.value()?.parse()?);
                } else {
                    // error() 的 span 指向这个未知的键，用户一眼看到自己拼错的地方
                    return Err(meta.error("#[validate] 只认 min_len / max_len / min / max"));
                }
                Ok(())
            })?;
        }

        // ---- 长度类规则：只算一次 chars().count()，两条规则共用 ----
        if rule.min_len.is_some() || rule.max_len.is_some() {
            // 用 format_ident! 拼局部变量名：span 来自字段 ident，报错位置才对得上
            let len_var = format_ident!("__len_{}", ident);

            let min_check = rule.min_len.as_ref().map(|n| {
                quote! {
                    if #len_var < #n {
                        errs.push(format!("{}: 长度 {} < 最小 {}", #field_name, #len_var, #n));
                    }
                }
            });
            let max_check = rule.max_len.as_ref().map(|n| {
                quote! {
                    if #len_var > #n {
                        errs.push(format!("{}: 长度 {} > 最大 {}", #field_name, #len_var, #n));
                    }
                }
            });

            checks.extend(quote! {
                {
                    let #len_var = self.#ident.chars().count();
                    #min_check
                    #max_check
                }
            });
        }

        // ---- 数值类规则 ----
        if let Some(n) = &rule.min {
            checks.extend(quote! {
                if self.#ident < #n {
                    errs.push(format!("{}: {} < 最小 {}", #field_name, self.#ident, #n));
                }
            });
        }
        if let Some(n) = &rule.max {
            checks.extend(quote! {
                if self.#ident > #n {
                    errs.push(format!("{}: {} > 最大 {}", #field_name, self.#ident, #n));
                }
            });
        }
    }

    // 泛型：如果用户写了 `struct Wrapper<T: ...>`，impl 头必须原样带上，
    // 否则生成的代码对泛型类型不成立
    let (impl_generics, ty_generics, where_clause) = input.generics.split_for_impl();

    Ok(quote! {
        impl #impl_generics #name #ty_generics #where_clause {
            /// 校验全部字段，返回**所有**失败项（不是第一个）。
            ///
            /// 为什么一次返回全部：表单场景下一次只报一个错误，用户要提交 N 次才改完。
            pub fn validate(&self) -> ::std::result::Result<(), ::std::vec::Vec<::std::string::String>> {
                let mut errs: ::std::vec::Vec<::std::string::String> = ::std::vec::Vec::new();
                #checks
                if errs.is_empty() {
                    ::std::result::Result::Ok(())
                } else {
                    ::std::result::Result::Err(errs)
                }
            }
        }
    })
}
