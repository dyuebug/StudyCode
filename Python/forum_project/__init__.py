"""论坛系统示例包。

对外只暴露应用工厂，便于测试与部署复用。
"""

from .app import create_app

__all__ = ["create_app"]
