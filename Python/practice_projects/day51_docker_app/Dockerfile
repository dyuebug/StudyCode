# 第 51 天练习：最小 Docker 镜像
FROM python:3.12-slim

ENV PYTHONDONTWRITEBYTECODE=1 \
    PYTHONUNBUFFERED=1 \
    PORT=8000

WORKDIR /app

COPY app.py /app/app.py

RUN groupadd --system app \
    && useradd --system --gid app --create-home app \
    && mkdir -p /data \
    && chown -R app:app /app /data

USER app

EXPOSE 8000

# slim 镜像没有 curl，使用标准库做健康检查。
HEALTHCHECK --interval=10s --timeout=3s --start-period=5s --retries=3 \
    CMD python -c "import urllib.request; urllib.request.urlopen('http://127.0.0.1:8000/health', timeout=2)"

CMD ["python", "/app/app.py"]
