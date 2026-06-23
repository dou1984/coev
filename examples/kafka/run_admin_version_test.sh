#!/bin/bash
# 运行 AdminClient 版本兼容性测试

HOST=${1:-localhost}
PORT=${2:-9092}
TOPIC=${3:-test-topic}

echo "Running admin version test..."
echo "Host: $HOST"
echo "Port: $PORT"
echo "Topic: $TOPIC"
echo ""

./admin_version_test "$HOST" "$PORT" "$TOPIC"
