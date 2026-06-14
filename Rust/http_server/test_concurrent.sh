#!/bin/bash

echo "🧪 测试 HTTP 服务器并发性能"
echo "================================"
echo ""
echo "测试说明："
echo "1. 先访问 /sleep（慢请求，5秒延迟）"
echo "2. 立即访问 /（快速请求）"
echo "3. 如果服务器支持并发，快速请求会立即返回，不会等待慢请求"
echo ""
echo "开始测试..."
echo ""

# 在后台发起慢请求
echo "⏳ 发起慢请求: GET /sleep"
time curl -s http://127.0.0.1:7878/sleep > /dev/null &

# 等待 0.5 秒确保慢请求先开始
sleep 0.5

# 发起快速请求
echo "⚡ 发起快速请求: GET /"
time curl -s http://127.0.0.1:7878/ > /dev/null

echo ""
echo "✅ 测试完成！"
echo ""
echo "如果快速请求立即返回（不到1秒），说明并发工作正常！"
echo "如果快速请求等待了5秒，说明请求被阻塞了。"

# 等待后台任务完成
wait
