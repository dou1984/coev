#!/bin/bash

# sudo sysctl -w kernel.perf_event_paranoid=0
# nghttp 压力测试脚本 (带 perf 分析)

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
perf record -F 99 -a -g --call-graph dwarf -e cpu-clock,cycles,instructions,cache-misses,branches,branch-misses -o perf.server.data ./co_nghttp server > server.log 2>&1 &
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
perf record -F 99 -a -g --call-graph dwarf -e cpu-clock,cycles,instructions,cache-misses,branches,branch-misses -o perf.client.data ./co_nghttp client > client.log 2>&1 &
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

# 生成 perf 报告
echo ""
echo "6. 生成 perf 报告..."
perf report -i perf.server.data --stdio --sort=overhead -n > perf.server.report 2>&1
perf report -i perf.client.data --stdio --sort=overhead -n > perf.client.report 2>&1

# 生成热点函数分析
perf report -i perf.server.data --stdio -g graph,0.5 --sort=overhead > perf.server.hotspot 2>&1
perf report -i perf.client.data --stdio -g graph,0.5 --sort=overhead > perf.client.hotspot 2>&1

# 生成事件统计
perf stat -i perf.server.data > perf.server.stats 2>&1
perf stat -i perf.client.data > perf.client.stats 2>&1

echo ""
echo "=== 测试结果 ==="
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
echo "=== CPU 利用率分析 ==="
echo ""
echo "服务器 CPU 利用率分析:"
if [ -f perf.server.stats ]; then
    grep -E "CPU|cycles|instructions|cache-misses" perf.server.stats | head -10
fi
echo ""
echo "客户端 CPU 利用率分析:"
if [ -f perf.client.stats ]; then
    grep -E "CPU|cycles|instructions|cache-misses" perf.client.stats | head -10
fi
echo ""
echo "=== 可能导致 CPU 利用率低的原因分析 ==="
echo ""
echo "1. I/O 等待 (主要原因):"
echo "   - 网络 I/O 等待: 检查 epoll_wait、select、poll 等系统调用"
echo "   - 磁盘 I/O 等待: 检查 read、write、fsync 等系统调用"
echo ""
echo "2. 协程调度开销:"
echo "   - 协程切换频繁: 检查 co_yield、co_await 相关函数"
echo "   - 调度器负载: 检查 runnable、cosys 相关函数"
echo ""
echo "3. 锁竞争:"
echo "   - 互斥锁: 检查 pthread_mutex_* 相关函数"
echo "   - 条件变量: 检查 pthread_cond_* 相关函数"
echo ""
echo "4. 系统调用过多:"
echo "   - 频繁的系统调用导致上下文切换"
echo "   - 检查 gettimeofday、clock_gettime 等高频调用"
echo ""
echo "5. 空闲等待:"
echo "   - 事件循环在等待事件时处于空闲状态"
echo "   - 检查 epoll_wait 时间占比"
echo ""
echo "6. SSL/TLS 握手开销:"
echo "   - TLS 握手过程耗时较长"
echo "   - 检查 SSL_do_handshake 等函数"
echo ""
echo "7. 内存管理:"
echo "   - 频繁的内存分配/释放"
echo "   - 检查 malloc、free、new、delete 等函数"
echo ""
echo "详细分析请查看热点函数报告:"
echo "  - $BIN_DIR/perf.server.hotspot"
echo "  - $BIN_DIR/perf.client.hotspot"
echo ""
echo "✓ 测试完成！"
echo ""
echo "详细的 perf 报告已保存到:"
echo "  - $BIN_DIR/perf.server.report"
echo "  - $BIN_DIR/perf.client.report"
echo "  - $BIN_DIR/perf.server.hotspot"
echo "  - $BIN_DIR/perf.client.hotspot"
echo "  - $BIN_DIR/perf.server.stats"
echo "  - $BIN_DIR/perf.client.stats"
echo "  - $BIN_DIR/perf.server.data"
echo "  - $BIN_DIR/perf.client.data"

