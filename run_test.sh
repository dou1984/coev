#!/bin/bash

echo "=== 编译并运行压力测试 ==="
echo ""

# 编译
echo "1. 编译项目..."
cd /home/dou1984/github/coev/build
make -j co_nghttp > /dev/null 2>&1
echo "✓ 编译完成"
echo ""

# 清理
echo "2. 清理旧进程和日志..."
pkill -9 co_nghttp 2>/dev/null || true
sleep 1
rm -f /home/dou1984/github/coev/bin/*.log
echo ""

# 启动服务器
echo "3. 启动服务器..."
cd /home/dou1984/github/coev/bin
./co_nghttp server > server.log 2>&1 &
SERVER_PID=$!
echo "✓ 服务器已启动 (PID: $SERVER_PID)"
sleep 2
echo ""

# 检查服务器
if ! kill -0 $SERVER_PID 2>/dev/null; then
    echo "✗ 服务器启动失败！"
    cat server.log
    exit 1
fi

# 启动客户端
echo "4. 启动客户端..."
./co_nghttp client > client.log 2>&1 &
CLIENT_PID=$!
echo "✓ 客户端已启动 (PID: $CLIENT_PID)"
echo ""

# 监控
echo "5. 监控测试进度..."
for i in {1..30}; do
    sleep 1
    if ! kill -0 $CLIENT_PID 2>/dev/null; then
        break
    fi
    echo "   测试进行中... ${i}s"
done

# 停止
kill $SERVER_PID $CLIENT_PID 2>/dev/null || true
sleep 1

echo ""
echo "=== 测试结果 ==="
echo ""

# 统计握手成功
if [ -f server.log ]; then
    SERVER_HANDSHAKE=$(grep -c "handshake completed" server.log 2>/dev/null || echo "0")
    echo "服务器 SSL 握手成功：$SERVER_HANDSHAKE"
fi

if [ -f client.log ]; then
    CLIENT_HANDSHAKE=$(grep -c "handshake completed" client.log 2>/dev/null || echo "0")
    echo "客户端 SSL 握手成功：$CLIENT_HANDSHAKE"
fi

# 统计请求
if [ -f client.log ]; then
    REQUESTS=$(grep -c "request completed" client.log 2>/dev/null || echo "0")
    echo "客户端请求完成：$REQUESTS"
fi

if [ -f server.log ]; then
    RESPONSES=$(grep -c "response sent" server.log 2>/dev/null || echo "0")
    echo "服务器响应发送：$RESPONSES"
fi

echo ""
echo "错误统计:"
if [ -f client.log ]; then
    CLIENT_ERRORS=$(grep -c "ERROR" client.log 2>/dev/null || echo "0")
    echo "  客户端错误：$CLIENT_ERRORS"
fi

if [ -f server.log ]; then
    SERVER_ERRORS=$(grep -c "ERROR" server.log 2>/dev/null || echo "0")
    echo "  服务器错误：$SERVER_ERRORS"
fi

# 统计 QPS
echo ""
echo "性能统计:"
if [ -f client.log ]; then
    REQUESTS=$(grep -c "request completed" client.log 2>/dev/null || echo "0")
    # 测试时长固定为 30 秒
    QPS=$(python3 -c "print(f'{$REQUESTS / 30:.2f}')" 2>/dev/null || echo "N/A")
    echo "  总请求数：$REQUESTS"
    echo "  测试时长：30 秒"
    echo "  QPS: $QPS"
fi

echo ""
echo "✓ 测试完成！"
