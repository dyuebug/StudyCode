"""开发环境启动入口。

运行：
    python -m forum_project.run
    或
    flask --app forum_project run --debug
"""

from __future__ import annotations

from . import create_app

application = create_app()

if __name__ == "__main__":
    application.run(host="127.0.0.1", port=5006, debug=True)
