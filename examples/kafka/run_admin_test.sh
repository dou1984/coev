#!/bin/bash

# 运行 Kafka admin 测试用例

# 设置测试参数
HOST="127.0.0.1"
PORT="9092"
TOPIC="test-topic"

# 检查 admin_test 可执行文件是否存在
if [ ! -f "./admin_test" ]; then
    echo "Error: admin_test executable not found!"
    echo "Please build the project first."
    exit 1
fi

# 运行 admin 测试
echo "Running Kafka admin test..."
echo "Host: $HOST"
echo "Port: $PORT"
echo "Topic: $TOPIC"
echo ""

# 执行测试
./admin_test $HOST $PORT $TOPIC

# 检查测试结果
if [ $? -eq 0 ]; then
    echo ""
    echo "Admin test completed successfully!"
else
    echo ""
    echo "Admin test failed!"
    exit 1
fi