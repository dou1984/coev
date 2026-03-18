#!/bin/bash

# cossl 压力测试脚本
# 位置：/home/dou1984/github/coev/examples/cossl/test/run_cossl.sh

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$(dirname "$(dirname "$SCRIPT_DIR")")")"
BUILD_DIR="$PROJECT_ROOT/build"
BIN_DIR="$PROJECT_ROOT/bin"

echo "=== 编译并运行 cossl 压力测试 ==="
echo ""

# 编译
echo "1. 编译项目..."
cd "$BUILD_DIR"
make -j co_ssl > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "✗ 编译失败！"
    exit 1
fi
echo "✓ 编译完成"
echo ""

# 清理
echo "2. 清理旧进程和日志..."
pkill -9 co_ssl 2>/dev/null || true
sleep 1
rm -f "$BIN_DIR"/*.log
echo ""

# 启动服务器
echo "3. 启动服务器..."
cd "$BIN_DIR"
./co_ssl server > server.log 2>&1 &
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
./co_ssl client > client.log 2>&1 &
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

# 统计请求和错误
echo "请求统计:"
if [ -f client.log ]; then
    # 提取统计信息 (cossl 使用英文日志)
    REQUESTS=$(grep "Total requests:" client.log | tail -1 | sed 's/.*Total requests: *//' | grep -oE '[0-9]+')
    if [ -n "$REQUESTS" ]; then
        echo "  总请求数：$REQUESTS"
    fi
    
    ELAPSED=$(grep "Duration:" client.log | tail -1 | sed 's/.*Duration: *//' | grep -oE '[0-9.]+')
    if [ -n "$ELAPSED" ]; then
        echo "  总耗时：${ELAPSED} s"
    fi
    
    QPS=$(grep "QPS:" client.log | tail -1 | sed 's/.*QPS: *//' | grep -oE '[0-9.]+')
    if [ -n "$QPS" ]; then
        echo "  QPS: $QPS"
    fi
fi

echo ""
echo "错误统计:"
if [ -f client.log ]; then
    CLIENT_ERRORS=$(grep -c "ERROR" client.log 2>/dev/null || echo "0")
    echo "  客户端 ERROR 日志数：$CLIENT_ERRORS"
fi

if [ -f server.log ]; then
    SERVER_ERRORS=$(grep -c "ERROR" server.log 2>/dev/null || echo "0")
    echo "  服务器 ERROR 日志数：$SERVER_ERRORS"
fi

echo ""
echo "✓ 测试完成！"
