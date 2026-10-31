// ============================================================================
//  config.rs · anyhow 的职责边界
// ============================================================================
//  与 error.rs 对照：这里**不定义错误枚举**，因为「配置可能怎么坏」是开放的
//  （变量缺、不是数字、越界、文件不存在、编码错……），穷举没有收益。
//
//  anyhow 的用法只有三件事：
//    1. `?` 直接传播任意 `E: std::error::Error`
//    2. `.with_context(|| ...)` 在每一层补一句「我在干什么时失败的」
//    3. `{:#}` 打印完整链（`{}` 只打印最外层）
//
//  可测性设计：`from_env()` 只是 `from_pairs()` 的一层薄包装。
//  测试注入假的环境，不碰进程全局状态——否则并发测试会互相污染。
// ============================================================================

use anyhow::{bail, Context, Result};
use std::path::Path;

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Config {
    pub host: String,
    pub port: u16,
    pub log_level: String,
}

impl Default for Config {
    fn default() -> Self {
        Self {
            host: "127.0.0.1".to_string(),
            port: 8080,
            log_level: "info".to_string(),
        }
    }
}

impl Config {
    /// 从进程环境读取。
    pub fn from_env() -> Result<Self> {
        Self::from_pairs(|key| std::env::var(key).ok())
    }

    /// 核心逻辑：只依赖一个「查表函数」，因此可测。
    ///
    /// 每个失败点都带 context——这是 anyhow 唯一不能省的动作：
    /// 没有 context 的 anyhow 错误，等于把 Box<dyn Error> 换了个名字。
    pub fn from_pairs(get: impl Fn(&str) -> Option<String>) -> Result<Self> {
        let default = Config::default();

        let host = get("APP_HOST").unwrap_or_else(|| default.host.clone());
        let log_level = get("APP_LOG_LEVEL").unwrap_or_else(|| default.log_level.clone());

        let port_raw = get("APP_PORT").unwrap_or_else(|| default.port.to_string());
        let port = port_raw
            .parse::<u16>()
            .with_context(|| format!("环境变量 APP_PORT 不是合法端口（0-65535）：{port_raw:?}"))?;

        // 语义校验：0 是「让操作系统挑端口」，服务端不应依赖它的实际值
        if port == 0 {
            bail!("APP_PORT 不能为 0：请求的是任意端口，日志与探测都会指向错误位置");
        }

        let level = log_level.to_ascii_lowercase();
        if !matches!(
            level.as_str(),
            "trace" | "debug" | "info" | "warn" | "error"
        ) {
            bail!("APP_LOG_LEVEL 只支持 trace/debug/info/warn/error，实际：{log_level:?}");
        }

        Ok(Config {
            host,
            port,
            log_level: level,
        })
    }

    /// 从文件读取（演示 io 错误如何被自动加 context）。
    ///
    /// 注意：调用方拿到的是 anyhow::Error，**不需要知道**底层是 io 还是解析错误。
    pub fn from_file(path: &Path) -> Result<Self> {
        let text = std::fs::read_to_string(path)
            .with_context(|| format!("读取配置文件失败：{}", path.display()))?;

        let mut map = std::collections::HashMap::new();
        for (idx, line) in text.lines().enumerate() {
            let line = line.trim();
            if line.is_empty() || line.starts_with('#') {
                continue;
            }
            let (k, v) = line
                .split_once('=')
                .with_context(|| format!("第 {} 行不是 key=value：{line:?}", idx + 1))?;
            map.insert(k.trim().to_string(), v.trim().to_string());
        }

        Self::from_pairs(|key| map.get(key).cloned())
            .with_context(|| format!("配置文件内容不合法：{}", path.display()))
    }

    /// 「让程序把读到的值打出来」——`静默失败检查清单` §2.3 的落地：
    /// 配置不生效时，第一件事是确认程序读到的到底是什么。
    pub fn describe(&self) -> String {
        format!(
            "host={} port={} log_level={}",
            self.host, self.port, self.log_level
        )
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::collections::HashMap;

    fn cfg(pairs: &[(&str, &str)]) -> Result<Config> {
        let map: HashMap<String, String> = pairs
            .iter()
            .map(|(k, v)| (k.to_string(), v.to_string()))
            .collect();
        Config::from_pairs(|key| map.get(key).cloned())
    }

    #[test]
    fn 空环境用默认值() {
        let c = cfg(&[]).unwrap();
        assert_eq!(c.port, 8080);
        assert_eq!(c.host, "127.0.0.1");
    }

    #[test]
    fn 端口不是数字时错误链里有两层() {
        let err = cfg(&[("APP_PORT", "abc")]).unwrap_err();
        let full = format!("{err:#}");
        assert!(full.contains("不是合法端口"), "实际：{full}");
        assert!(full.contains("invalid digit"), "底层错误应保留：{full}");
    }

    #[test]
    fn 端口为_0_被拒绝() {
        let err = cfg(&[("APP_PORT", "0")]).unwrap_err();
        assert!(format!("{err}").contains("不能为 0"));
    }

    #[test]
    fn 日志级别大小写不敏感() {
        assert_eq!(
            cfg(&[("APP_LOG_LEVEL", "DEBUG")]).unwrap().log_level,
            "debug"
        );
    }

    #[test]
    fn 非法日志级别被拒绝() {
        let err = cfg(&[("APP_LOG_LEVEL", "verbose")]).unwrap_err();
        assert!(format!("{err}").contains("只支持"));
    }

    #[test]
    fn 配置文件缺键时用默认值_多键时生效() {
        let dir = std::env::temp_dir().join("errors-and-logs-cfg");
        std::fs::create_dir_all(&dir).unwrap();
        let path = dir.join("app.conf");
        std::fs::write(&path, "# 注释\nAPP_PORT=9000\n").unwrap();

        let c = Config::from_file(&path).unwrap();
        assert_eq!(c.port, 9000);
        assert_eq!(c.host, "127.0.0.1");

        std::fs::remove_file(&path).ok();
        std::fs::remove_dir(&dir).ok();
    }

    #[test]
    fn 文件不存在时_context_带出路径() {
        let err = Config::from_file(Path::new("绝对不存在的配置.conf")).unwrap_err();
        let full = format!("{err:#}");
        assert!(full.contains("读取配置文件失败"), "实际：{full}");
        assert!(
            full.contains("绝对不存在的配置.conf"),
            "路径应出现在链上：{full}"
        );
    }
}
