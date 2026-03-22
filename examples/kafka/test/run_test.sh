#!/bin/bash

# Kafka 压力测试脚本

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$(dirname "$(dirname "$SCRIPT_DIR")")")"
BUILD_DIR="$PROJECT_ROOT/build"
BIN_DIR="$PROJECT_ROOT/bin"

# 默认参数
HOST="localhost"
PORT="9092"
TOPIC="stress-test-topic"
MODE="both"
CLIENTS=10
MESSAGES=1000
SIZE=100
DURATION=30

# 解析命令行参数
while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--host)
            HOST="$2"
            shift 2
            ;;
        -p|--port)
            PORT="$2"
            shift 2
            ;;
        -t|--topic)
            TOPIC="$2"
            shift 2
            ;;
        -m|--mode)
            MODE="$2"
            shift 2
            ;;
        -c|--clients)
            CLIENTS="$2"
            shift 2
            ;;
        -n|--messages)
            MESSAGES="$2"
            shift 2
            ;;
        -s|--size)
            SIZE="$2"
            shift 2
            ;;
        -d|--duration)
            DURATION="$2"
            shift 2
            ;;
        --help)
            echo "用法：$0 [选项]"
            echo ""
            echo "选项:"
            echo "  -h, --host        Kafka 服务器地址 (默认：localhost)"
            echo "  -p, --port        Kafka 服务器端口 (默认：9092)"
            echo "  -t, --topic       主题名称 (默认：stress-test-topic)"
            echo "  -m, --mode        测试模式：push, pull, both (默认：both)"
            echo "  -c, --clients     并发客户端数 (默认：10)"
            echo "  -n, --messages    每个客户端的消息数 (默认：1000)"
            echo "  -s, --size        消息大小 (字节) (默认：100)"
            echo "  -d, --duration    测试持续时间 (秒) (默认：30)"
            echo ""
            exit 0
            ;;
        *)
            echo "未知选项：$1"
            exit 1
            ;;
    esac
done



echo "=== 编译并运行 Kafka 压力测试 ==="
echo ""
echo "配置参数:"
echo "  服务器：$HOST:$PORT"
echo "  主题：$TOPIC"
echo "  模式：$MODE"
echo "  客户端数：$CLIENTS"
echo "  消息数：$MESSAGES"
echo "  消息大小：$SIZE 字节"
echo "  持续时间：$DURATION 秒"
echo ""

# 编译
echo "1. 编译项目..."
cd "$BUILD_DIR"
echo "   配置 CMake 以包含调试信息..."
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo .. > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "✗ CMake 配置失败！"
    exit 1
fi
echo "   编译中..."
make -j$(nproc) co_kafka > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "✗ 编译失败！"
    exit 1
fi
echo "✓ 编译完成"
echo ""

# 清理
echo "2. 清理旧进程和日志..."
pkill -9 co_kafka 2>/dev/null || true
sleep 1
rm -f "$BIN_DIR"/*.log
echo ""

# 启动生产者
if [ "$MODE" = "push" ] || [ "$MODE" = "both" ]; then
    echo "3. 启动生产者 ..."
    cd "$BIN_DIR"
    
    # 启动多个生产者客户端
    PRODUCER_PIDS=()
    for ((i=1; i<=CLIENTS; i++)); do
        ./co_kafka push "$HOST" "$PORT" "$TOPIC" > producer.$i.log 2>&1 &
        PRODUCER_PIDS+=($!)
        echo "   生产者 $i 已启动 (PID: ${PRODUCER_PIDS[-1]})"
    done
    echo "✓ 生产者已启动 (共 $CLIENTS 个)"
    echo ""
    
    # 检查生产者
    sleep 2
    for pid in "${PRODUCER_PIDS[@]}"; do
        if ! kill -0 $pid 2>/dev/null; then
            echo "✗ 生产者启动失败！"
            exit 1
        fi
    done
    
    # 监控生产者
    echo "4. 监控生产者进度..."
    PRODUCER_START_TIME=$(date +%s)
    PRODUCER_RUNNING=true
    
    while $PRODUCER_RUNNING; do
        ELAPSED=$(($(date +%s) - PRODUCER_START_TIME))
        
        # 检查是否超时
        if [ $ELAPSED -ge $((DURATION * 2)) ]; then
            echo "   生产者运行超时，停止测试"
            PRODUCER_RUNNING=false
            break
        fi
        
        # 检查是否还有进程在运行
        RUNNING_COUNT=0
        for pid in "${PRODUCER_PIDS[@]}"; do
            if kill -0 $pid 2>/dev/null; then
                ((RUNNING_COUNT++))
            fi
        done
        
        if [ $RUNNING_COUNT -eq 0 ]; then
            echo "   所有生产者已完成"
            PRODUCER_RUNNING=false
            break
        fi
        
        echo "   测试进行中... ${ELAPSED}s (运行中：$RUNNING_COUNT/$CLIENTS)"
        sleep 2
    done
    
    # 停止生产者
    for pid in "${PRODUCER_PIDS[@]}"; do
        kill $pid 2>/dev/null || true
    done
    sleep 1
fi

# 启动消费者
if [ "$MODE" = "pull" ] || [ "$MODE" = "both" ]; then
    echo "5. 启动消费者 ..."
    cd "$BIN_DIR"
    
    # 启动多个消费者客户端
    CONSUMER_PIDS=()
    for ((i=1; i<=CLIENTS; i++)); do
        ./co_kafka pull "$HOST" "$PORT" "$TOPIC" > consumer.$i.log 2>&1 &
        CONSUMER_PIDS+=($!)
        echo "   消费者 $i 已启动 (PID: ${CONSUMER_PIDS[-1]})"
    done
    echo "✓ 消费者已启动 (共 $CLIENTS 个)"
    echo ""
    
    # 检查消费者
    sleep 2
    for pid in "${CONSUMER_PIDS[@]}"; do
        if ! kill -0 $pid 2>/dev/null; then
            echo "✗ 消费者启动失败！"
            exit 1
        fi
    done
    
    # 监控消费者
    echo "6. 监控消费者进度..."
    CONSUMER_START_TIME=$(date +%s)
    CONSUMER_RUNNING=true
    
    while $CONSUMER_RUNNING; do
        ELAPSED=$(($(date +%s) - CONSUMER_START_TIME))
        
        # 检查是否超时
        if [ $ELAPSED -ge $DURATION ]; then
            echo "   测试时间到达，停止测试"
            CONSUMER_RUNNING=false
            break
        fi
        
        # 检查是否还有进程在运行
        RUNNING_COUNT=0
        for pid in "${CONSUMER_PIDS[@]}"; do
            if kill -0 $pid 2>/dev/null; then
                ((RUNNING_COUNT++))
            fi
        done
        
        if [ $RUNNING_COUNT -eq 0 ]; then
            echo "   所有消费者已完成"
            CONSUMER_RUNNING=false
            break
        fi
        
        echo "   测试进行中... ${ELAPSED}s/${DURATION}s (运行中：$RUNNING_COUNT/$CLIENTS)"
        sleep 2
    done
    
    # 停止消费者
    for pid in "${CONSUMER_PIDS[@]}"; do
        kill $pid 2>/dev/null || true
    done
    sleep 1
fi

echo ""
echo "=== 测试结果 ==="
echo ""

if [ "$MODE" = "push" ] || [ "$MODE" = "both" ]; then
    echo "生产者统计:"
    TOTAL_PRODUCED=0
    TOTAL_PRODUCER_ERRORS=0
    
    for ((i=1; i<=CLIENTS; i++)); do
        if [ -f producer.$i.log ]; then
            # 统计成功消息数
            PRODUCED=$(grep -c "produced message" producer.$i.log 2>/dev/null)
            [ -z "$PRODUCED" ] && PRODUCED=0
            ERRORS=$(grep -c "error\|Error\|ERROR" producer.$i.log 2>/dev/null)
            [ -z "$ERRORS" ] && ERRORS=0
            TOTAL_PRODUCED=$((TOTAL_PRODUCED + PRODUCED))
            TOTAL_PRODUCER_ERRORS=$((TOTAL_PRODUCER_ERRORS + ERRORS))
            
            echo "   生产者 $i: 生产 $PRODUCED 条消息，$ERRORS 个错误"
        fi
    done
    
    echo ""
    echo "   总计：生产 $TOTAL_PRODUCED 条消息，$TOTAL_PRODUCER_ERRORS 个错误"
    echo ""
fi

if [ "$MODE" = "pull" ] || [ "$MODE" = "both" ]; then
    echo "消费者统计:"
    TOTAL_CONSUMED=0
    TOTAL_CONSUMER_ERRORS=0
    
    for ((i=1; i<=CLIENTS; i++)); do
        if [ -f consumer.$i.log ]; then
            # 统计消费消息数
            CONSUMED=$(grep -c "Messages" consumer.$i.log 2>/dev/null)
            [ -z "$CONSUMED" ] && CONSUMED=0
            ERRORS=$(grep -c "error\|Error\|ERROR" consumer.$i.log 2>/dev/null)
            [ -z "$ERRORS" ] && ERRORS=0
            TOTAL_CONSUMED=$((TOTAL_CONSUMED + CONSUMED))
            TOTAL_CONSUMER_ERRORS=$((TOTAL_CONSUMER_ERRORS + ERRORS))
            
            echo "   消费者 $i: 消费 $CONSUMED 条消息，$ERRORS 个错误"
        fi
    done
    
    echo ""
    echo "   总计：消费 $TOTAL_CONSUMED 条消息，$TOTAL_CONSUMER_ERRORS 个错误"
    echo ""
fi

echo "错误统计:"
if [ "$MODE" = "push" ] || [ "$MODE" = "both" ]; then
    echo "  生产者 ERROR 日志数：$TOTAL_PRODUCER_ERRORS"
fi
if [ "$MODE" = "pull" ] || [ "$MODE" = "both" ]; then
    echo "  消费者 ERROR 日志数：$TOTAL_CONSUMER_ERRORS"
fi
echo ""

echo "✓ 测试完成！"
echo ""
echo "日志文件已保存到:"
if [ "$MODE" = "push" ] || [ "$MODE" = "both" ]; then
    echo "  - $BIN_DIR/producer.*.log"
fi
if [ "$MODE" = "pull" ] || [ "$MODE" = "both" ]; then
    echo "  - $BIN_DIR/consumer.*.log"
fi
