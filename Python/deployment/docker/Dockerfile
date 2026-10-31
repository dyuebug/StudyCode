# 语法版本：Dockerfile 1.x
# 构建（在仓库根目录执行）：
#   docker build -f deployment/docker/Dockerfile -t study-forum:1.0 .
# 运行：
#   docker run --rm -p 8000:8000 -e SECRET_KEY=change-me \
#     -v forum-data:/data study-forum:1.0

FROM python:3.12-slim AS runtime

ENV PYTHONDONTWRITEBYTECODE=1 \
    PYTHONUNBUFFERED=1 \
    PIP_NO_CACHE_DIR=1 \
    DATABASE=/data/forum.db

WORKDIR /app

# 先复制依赖清单，利用镜像层缓存
COPY forum_project/requirements.txt /app/requirements.txt
RUN pip install --no-cache-dir -r /app/requirements.txt

# 再复制应用代码
COPY forum_project /app/forum_project

# 非 root 运行；数据目录单独挂载
RUN groupadd --system app \
    && useradd --system --gid app --create-home app \
    && mkdir -p /data \
    && chown -R app:app /app /data

USER app

EXPOSE 8000

HEALTHCHECK --interval=30s --timeout=3s --start-period=10s --retries=3 \
    CMD python -c "import urllib.request; urllib.request.urlopen('http://127.0.0.1:8000/api/posts', timeout=2)"

# SQLite 单写入者：1 worker + 多线程，避免 database is locked
CMD ["gunicorn", \
     "--bind", "0.0.0.0:8000", \
     "--workers", "1", \
     "--threads", "4", \
     "--access-logfile", "-", \
     "--error-logfile", "-", \
     "forum_project:create_app()"]
