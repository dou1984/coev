#!/bin/bash
# 运行所有消息类型的版本兼容性测试

HOST=${1:-localhost}
PORT=${2:-9092}
TOPIC=${3:-test-topic}

echo "Running all messages version test..."
echo "Host: $HOST"
echo "Port: $PORT"
echo "Topic: $TOPIC"
echo ""

./all_messages_version_test "$HOST" "$PORT" "$TOPIC"
