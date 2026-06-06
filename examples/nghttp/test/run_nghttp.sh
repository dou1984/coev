#!/bin/bash

# sudo sysctl -w kernel.perf_event_paranoid=0
# nghttp 压力测试脚本 (带 CPU 分析)

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$(dirname "$(dirname "$SCRIPT_DIR")")")"
BUILD_DIR="$PROJECT_ROOT/build"
BIN_DIR="$PROJECT_ROOT/bin"

# 确保 perf 可用
if ! command -v perf &> /dev/null; then
    echo "✗ perf 命令不可用，请安装 perf 工具"
    exit 1
fi

echo "=== 编译并运行 nghttp 压力测试 (带 perf 分析) ==="
echo ""

# 编译
echo "1. 编译项目..."
cd "$BUILD_DIR"
# 添加调试信息到 CMake 配置
echo "   配置 CMake 以包含调试信息..."
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo .. > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "✗ CMake 配置失败！"
    exit 1
fi
echo "   编译中..."
make -j$(nproc) co_nghttp > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "✗ 编译失败！"
    exit 1
fi
echo "✓ 编译完成"
echo ""

# 清理
echo "2. 清理旧进程和日志..."
pkill -9 co_nghttp 2>/dev/null || true
sleep 1
rm -f "$BIN_DIR"/*.log "$BIN_DIR"/perf.*
echo ""

# 启动服务器 (带 perf 分析)
echo "3. 启动服务器 (带 perf 分析) ..."
cd "$BIN_DIR"
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

# 启动客户端 (带 perf 分析)
echo "4. 启动客户端 (带 perf 分析) ..."
./co_nghttp client > client.log 2>&1 &
CLIENT_PID=$!
echo "✓ 客户端已启动 (PID: $CLIENT_PID)"
echo ""

# 监控
echo "5. 监控测试进度..."
TIMEOUT=60
SAMPLE_TIME=$((TIMEOUT - 5))
if [ "$SAMPLE_TIME" -lt 5 ]; then
    SAMPLE_TIME=5
fi

# 启动 CPU 利用率监控（在主循环中通过 ps 采样）
echo "   启动 CPU 利用率监控..."
CPU_LOG_S="$BIN_DIR/perf.server.cpu.log"
CPU_LOG_C="$BIN_DIR/perf.client.cpu.log"
echo "# CPU monitoring for server PID $SERVER_PID" > "$CPU_LOG_S"
echo "# CPU monitoring for client PID $CLIENT_PID" > "$CPU_LOG_C"
echo "   CPU 监控已启动 (每秒采样)"

# perf record 采样分析
echo "   启动 perf 采样分析 (perf record)..."
perf record -g -p $CLIENT_PID -o "$BIN_DIR/perf.client.data" -- sleep $SAMPLE_TIME 2>/dev/null &
PERF_CLIENT_PID=$!
perf record -g -p $SERVER_PID -o "$BIN_DIR/perf.server.data" -- sleep $SAMPLE_TIME 2>/dev/null &
PERF_SERVER_PID=$!
echo "   perf 采样已启动 (采样时长 ${SAMPLE_TIME}s)"
echo ""
for i in $(seq 1 $TIMEOUT); do
    sleep 1
    # CPU 采样 (通过 ps)
    srv_cpu=$(ps -p $SERVER_PID -o %cpu= 2>/dev/null | tr -d ' ')
    cli_cpu=$(ps -p $CLIENT_PID -o %cpu= 2>/dev/null | tr -d ' ')
    [ -n "$srv_cpu" ] && echo "$(date +%H:%M:%S) CPU: ${srv_cpu}%" >> "$CPU_LOG_S"
    [ -n "$cli_cpu" ] && echo "$(date +%H:%M:%S) CPU: ${cli_cpu}%" >> "$CPU_LOG_C"
    if ! kill -0 $CLIENT_PID 2>/dev/null; then
        echo "   客户端已完成，耗时 ${i}s"
        break
    fi
    if [ $((i % 10)) -eq 0 ]; then
        echo "   测试进行中... ${i}s  (服务端CPU:${srv_cpu:-0}%, 客户端CPU:${cli_cpu:-0}%)"
    fi
done

# 检查是否超时
if kill -0 $CLIENT_PID 2>/dev/null; then
    echo ""
    echo "   测试已达到最大时间 ${TIMEOUT}s，停止客户端..."
fi

# 停止监控进程
kill $PERF_CLIENT_PID $PERF_SERVER_PID 2>/dev/null || true
wait $PERF_CLIENT_PID $PERF_SERVER_PID 2>/dev/null || true

# 优雅停止
echo ""
echo "6. 停止进程和分析..."
# 先发送 SIGTERM 让客户端优雅退出
kill -TERM $CLIENT_PID 2>/dev/null || true
sleep 2
# 如果还没退出，再强制杀死
if kill -0 $CLIENT_PID 2>/dev/null; then
    kill -9 $CLIENT_PID 2>/dev/null || true
fi
kill $SERVER_PID 2>/dev/null || true
sleep 1

# 分析性能数据
echo ""
echo "================================================================================"
echo "性能统计"
echo "================================================================================"
echo ""

# 统计请求和错误
echo "请求统计:"
if [ -f client.log ]; then
    # 提取统计信息
    REQUESTS=$(grep "总请求数:" client.log | tail -1 | sed 's/.*总请求数: *//' | grep -oE '[0-9]+')
    if [ -n "$REQUESTS" ]; then
        echo "  总请求数：$REQUESTS"
    fi
    
    ERRORS=$(grep "错误数:" client.log | tail -1 | sed 's/.*错误数: *//' | grep -oE '[0-9]+')
    if [ -n "$ERRORS" ]; then
        echo "  错误数：$ERRORS"
    fi
    
    BYTES=$(grep "总数据量:" client.log | tail -1 | grep -oE '\([0-9.]+ MB\)' | tr -d '()')
    if [ -n "$BYTES" ]; then
        echo "  总数据量：$BYTES"
    fi
    
    ELAPSED=$(grep "总耗时:" client.log | tail -1 | sed 's/.*总耗时: *//' | grep -oE '[0-9]+')
    if [ -n "$ELAPSED" ]; then
        echo "  总耗时：${ELAPSED} ms"
    fi
    
    QPS=$(grep "QPS:" client.log | tail -1 | sed 's/.*QPS: *//' | grep -oE '[0-9.]+')
    if [ -n "$QPS" ]; then
        echo "  QPS: $QPS"
    fi
fi

if [ -f client.log ]; then
    CLIENT_ERRORS=$(grep -c "ERROR" client.log 2>/dev/null || echo "0")
    echo "  客户端 ERROR 日志数：$CLIENT_ERRORS"
fi

if [ -f server.log ]; then
    SERVER_ERRORS=$(grep -c "ERROR" server.log 2>/dev/null || echo "0")
    echo "  服务器 ERROR 日志数：$SERVER_ERRORS"
fi

if [ -f perf.server.stats ]; then
    grep -E "CPU|cycles|instructions|cache-misses" perf.server.stats | head -10
fi
if [ -f perf.client.stats ]; then
    grep -E "CPU|cycles|instructions|cache-misses" perf.client.stats | head -10
fi

echo ""
echo "================================================================================"
echo "CPU 利用率分析"
echo "================================================================================"
echo ""

echo "服务端:"
if [ -f perf.server.cpu.log ]; then
    # 提取 CPU% 数值并计算平均值、最小值、最大值
    awk '/^#/ {next} {gsub(/.*CPU: /,""); gsub(/%/,""); if($1+0>=0){sum+=$1; count++; if(count==1||$1<min) min=$1; if(count==1||$1>max) max=$1}} END {if(count>0) printf "  平均 CPU 利用率: %.1f%%  (最小: %.1f%%, 最大: %.1f%%, 采样次数: %d)\n", sum/count, min, max, count; else print "  无数据"}' perf.server.cpu.log
    echo "  CPU 利用率时序:"
    tail -20 perf.server.cpu.log | sed 's/^/    /'
    echo "  ... (完整数据: perf.server.cpu.log)"
else
    echo "  无数据"
fi
echo ""

echo "客户端:"
if [ -f perf.client.cpu.log ]; then
    awk '/^#/ {next} {gsub(/.*CPU: /,""); gsub(/%/,""); if($1+0>=0){sum+=$1; count++; if(count==1||$1<min) min=$1; if(count==1||$1>max) max=$1}} END {if(count>0) printf "  平均 CPU 利用率: %.1f%%  (最小: %.1f%%, 最大: %.1f%%, 采样次数: %d)\n", sum/count, min, max, count; else print "  无数据"}' perf.client.cpu.log
    echo "  CPU 利用率时序:"
    tail -20 perf.client.cpu.log | sed 's/^/    /'
    echo "  ... (完整数据: perf.client.cpu.log)"
else
    echo "  无数据"
fi

echo ""
echo "================================================================================"
echo "perf 调用栈热点分析"
echo "================================================================================"
echo ""

if [ -f perf.client.data ]; then
    echo "客户端热点函数 (top 20):"
    perf report --stdio -i perf.client.data --sort comm,dso,symbol --no-children -n 2>/dev/null | head -30
    echo ""
else
    echo "  perf.client.data 未生成（可能因权限/容器限制），perf record 不可用"
    echo ""
fi
if [ -f perf.server.data ]; then
    echo "服务端热点函数 (top 20):"
    perf report --stdio -i perf.server.data --sort comm,dso,symbol --no-children -n 2>/dev/null | head -30
    echo ""
else
    echo "  perf.server.data 未生成（可能因权限/容器限制），perf record 不可用"
    echo ""
fi


