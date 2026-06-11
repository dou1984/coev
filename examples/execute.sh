#!/bin/bash

set -e

PROJECT_DIR=$(cd "$(dirname "$0")/.." && pwd)
BUILD_DIR="$PROJECT_DIR/build"
BIN_DIR="$PROJECT_DIR/bin"

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

log_info() { echo -e "${GREEN}[INFO]${NC} $1"; }
log_warn() { echo -e "${YELLOW}[WARN]${NC} $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1"; }

# 编译函数
build() {
    log_info "Building project..."
    cd "$PROJECT_DIR"
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    cmake .. -DCMAKE_BUILD_TYPE=Debug
    make -j$(nproc)
    log_info "Build completed."
}

# 运行单个测试（无 valgrind）
run_test() {
    local bin="$1"
    local timeout="${2:-5}"
    local mode="${3:-}"
    local finish_mode="${4:-}"  # "finish" = wait for natural exit

    if [ ! -f "$BIN_DIR/$bin" ]; then
        log_error "Binary not found: $BIN_DIR/$bin"
        return 1
    fi

    local cmd_args=()
    if [ -n "$mode" ]; then
        cmd_args+=("$mode")  # 位置参数: server/client
    fi

    if [ "$finish_mode" = "finish" ]; then
        log_info "Running $bin until completion (mode: ${mode:-default})..."
        cd "$BIN_DIR"
        ./"$bin" "${cmd_args[@]}" 2>&1 || {
            local exit_code=$?
            log_warn "$bin exited with code $exit_code"
            return $exit_code
        }
        log_info "$bin completed successfully."
    else
        log_info "Running $bin (timeout: ${timeout}s, mode: ${mode:-default})..."
        cd "$BIN_DIR"
        timeout "$timeout" ./"$bin" "${cmd_args[@]}" 2>&1 || {
            local exit_code=$?
            if [ $exit_code -eq 124 ]; then
                log_warn "$bin timed out"
            else
                log_warn "$bin exited with code $exit_code"
            fi
            return $exit_code
        }
        log_info "$bin completed successfully."
    fi
}

# 使用 valgrind 运行内存检查
run_valgrind() {
    local bin="$1"
    local timeout="${2:-10}"
    local mode="${3:-}"
    local finish_mode="${4:-}"  # "finish" = wait for natural exit
    local suffix="${mode:-run}"
    local report_file="$PROJECT_DIR/valgrind_reports/${bin}_${suffix}_$(date +%Y%m%d_%H%M%S).log"

    mkdir -p "$PROJECT_DIR/valgrind_reports"

    if [ ! -f "$BIN_DIR/$bin" ]; then
        log_error "Binary not found: $BIN_DIR/$bin"
        return 1
    fi

    local cmd_args=()
    if [ -n "$mode" ]; then
        cmd_args+=("$mode")  # 位置参数: server/client
    fi

    local valgrind_base=(
        valgrind
        --tool=memcheck
        --leak-check=full
        --show-leak-kinds=all
        --track-origins=yes
        --verbose
        --log-file="$report_file"
    )

    if [ "$finish_mode" = "finish" ]; then
        log_info "Running valgrind on $bin until completion (mode: ${mode:-default})..."
        cd "$BIN_DIR"
        "${valgrind_base[@]}" ./"$bin" "${cmd_args[@]}" 2>&1
    else
        log_info "Running valgrind on $bin (timeout: ${timeout}s, mode: ${mode:-default})..."
        cd "$BIN_DIR"
        timeout "$timeout" "${valgrind_base[@]}" ./"$bin" "${cmd_args[@]}" 2>&1
    fi

    local exit_code=$?

    if [ $exit_code -eq 124 ]; then
        log_warn "$bin timed out during valgrind check"
    fi

    # 输出报告摘要
    echo ""
    log_info "=== Valgrind Report for $bin ($suffix) ==="
    if [ -f "$report_file" ]; then
        grep -E "(ERROR SUMMARY|LEAK SUMMARY|definitely lost|indirectly lost|possibly lost|still reachable)" "$report_file" || true
    fi
    echo ""

    # 检查是否有错误
    if [ -f "$report_file" ]; then
        local errors=$(grep "ERROR SUMMARY" "$report_file" | grep -oP '\d+ errors' | head -1)
        if [ -n "$errors" ] && [ "$errors" != "0 errors" ]; then
            log_error "Valgrind found errors: $errors (see $report_file)"
            return 1
        else
            log_info "Valgrind check passed for $bin ($suffix)"
        fi
    fi

    return 0
}

# 运行 server，启动 client 后 kill server
run_valgrind_server_client_kill() {
    local bin="$1"
    local client_timeout="${2:-2}"
    local report_file="$PROJECT_DIR/valgrind_reports/${bin}_server_$(date +%Y%m%d_%H%M%S).log"

    mkdir -p "$PROJECT_DIR/valgrind_reports"

    if [ ! -f "$BIN_DIR/$bin" ]; then
        log_error "Binary not found: $BIN_DIR/$bin"
        return 1
    fi

    log_info "=== Running $bin server + client sequence ==="
    cd "$BIN_DIR"

    # 启动 server (with valgrind)
    log_info "Starting $bin server with valgrind..."
    valgrind \
        --tool=memcheck \
        --leak-check=full \
        --show-leak-kinds=all \
        --track-origins=yes \
        --verbose \
        --log-file="$report_file" \
        ./"$bin" server &
    local server_pid=$!

    # 等待 server 启动
    sleep 1

    # 运行 client (timeout 2秒)
    log_info "Running client for ${client_timeout}s..."
    timeout "$client_timeout" ./"$bin" client 2>&1 || true

    # 等待 server 处理 client 断开
    sleep 1

    # kill server
    log_info "Stopping server (PID: $server_pid)..."
    kill -INT $server_pid 2>/dev/null || true
    wait $server_pid 2>/dev/null || true

    # 输出报告
    echo ""
    log_info "=== Valgrind Report for $bin (server mode) ==="
    if [ -f "$report_file" ]; then
        grep -E "(ERROR SUMMARY|LEAK SUMMARY|definitely lost|indirectly lost|possibly lost|still reachable)" "$report_file" || true
    fi
    echo ""

    # 检查是否有错误
    if [ -f "$report_file" ]; then
        local errors=$(grep "ERROR SUMMARY" "$report_file" | grep -oP '\d+ errors' | head -1)
        if [ -n "$errors" ] && [ "$errors" != "0 errors" ]; then
            log_error "Valgrind found errors: $errors (see $report_file)"
            return 1
        else
            log_info "Valgrind check passed for $bin (server mode)"
        fi
    fi

    return 0
}

# 列出所有可用的可执行文件
list_binaries() {
    if [ -d "$BIN_DIR" ]; then
        find "$BIN_DIR" -maxdepth 1 -type f -executable -printf "%f\n" | sort
    fi
}

# 需要特殊测试的项目（server/client 模式）
SPECIAL_BINS=("co_ssl" "co_nghttp" "co_tcp")

# 检查是否是特殊项目
is_special_bin() {
    local bin="$1"
    for special in "${SPECIAL_BINS[@]}"; do
        if [ "$bin" = "$special" ]; then
            return 0
        fi
    done
    return 1
}

# 主逻辑
usage() {
    echo "Usage: $0 [options]"
    echo ""
    echo "Options:"
    echo "  -b, --build             Build the project"
    echo "  -t, --test <binary>     Run a single test binary"
    echo "  -v, --valgrind <binary> Run valgrind on a single binary"
    echo "  -a, --all               Run all available tests"
    echo "  -A, --all-valgrind      Run valgrind on all available tests"
    echo "  -l, --list              List all available binaries"
    echo "  --mode <server|client>  Set server/client mode for special binaries"
    echo "  --timeout <seconds>     Set timeout (default: 5s for test, 10s for valgrind)"
    echo "  --finish                Run until completion (no timeout signal)"
    echo "  --client-timeout <sec>  Set client timeout for server test (default: 2s)"
    echo "  -h, --help              Show this help"
    echo ""
    echo "Examples:"
    echo "  $0 --build                                  # Build project"
    echo "  $0 --test co_task                           # Run co_task"
    echo "  $0 --valgrind co_task --timeout 5           # Valgrind check co_task"
    echo "  $0 --valgrind co_ssl --mode client --timeout 2  # Valgrind co_ssl client 2s"
    echo "  $0 --valgrind co_ssl --mode client --finish     # Valgrind co_ssl client until done"
    echo "  $0 --valgrind co_ssl --mode server              # Server + client test"
    echo "  $0 --all --timeout 3                            # Run all tests with 3s timeout"
    echo ""
}

TIMEOUT=""
ACTION=""
TARGET=""
MODE=""
FINISH_MODE=""
CLIENT_TIMEOUT="2"

while [[ $# -gt 0 ]]; do
    case $1 in
        -b|--build)
            ACTION="build"
            shift
            ;;
        -t|--test)
            ACTION="test"
            TARGET="$2"
            shift 2
            ;;
        -v|--valgrind)
            ACTION="valgrind"
            TARGET="$2"
            shift 2
            ;;
        -a|--all)
            ACTION="all"
            shift
            ;;
        -A|--all-valgrind)
            ACTION="all-valgrind"
            shift
            ;;
        -l|--list)
            ACTION="list"
            shift
            ;;
        --timeout)
            TIMEOUT="$2"
            shift 2
            ;;
        --mode)
            MODE="$2"
            shift 2
            ;;
        --finish)
            FINISH_MODE="finish"
            shift
            ;;
        --client-timeout)
            CLIENT_TIMEOUT="$2"
            shift 2
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            log_error "Unknown option: $1"
            usage
            exit 1
            ;;
    esac
done

if [ -z "$ACTION" ]; then
    log_error "No action specified"
    usage
    exit 1
fi

case "$ACTION" in
    build)
        build
        ;;
    test)
        if [ -n "$TIMEOUT" ]; then
            run_test "$TARGET" "$TIMEOUT" "$MODE" "$FINISH_MODE"
        else
            run_test "$TARGET" "5" "$MODE" "$FINISH_MODE"
        fi
        ;;
    valgrind)
        if [ "$MODE" = "server" ] && is_special_bin "$TARGET"; then
            # Server 模式：启动 server，运行 client 后 kill server
            run_valgrind_server_client_kill "$TARGET" "${CLIENT_TIMEOUT}"
        elif [ -n "$TIMEOUT" ]; then
            run_valgrind "$TARGET" "$TIMEOUT" "$MODE" "$FINISH_MODE"
        else
            run_valgrind "$TARGET" "10" "$MODE" "$FINISH_MODE"
        fi
        ;;
    all)
        FAILED=0
        log_info "=== Running standard tests ==="
        for bin in $(list_binaries); do
            # 跳过特殊项目（需要特殊处理）
            if is_special_bin "$bin"; then
                continue
            fi

            if [ -n "$TIMEOUT" ]; then
                run_test "$bin" "$TIMEOUT" "" "" || FAILED=$((FAILED + 1))
            else
                run_test "$bin" "5" "" "" || FAILED=$((FAILED + 1))
            fi
        done

        # 运行特殊项目的标准测试
        log_info "=== Running special test sequences ==="
        for bin in "${SPECIAL_BINS[@]}"; do
            if [ ! -f "$BIN_DIR/$bin" ]; then
                log_warn "Binary not found: $bin, skipping"
                continue
            fi

            # client timeout 2s
            run_test "$bin" "2" "client" "" || FAILED=$((FAILED + 1))

            # client finish
            run_test "$bin" "30" "client" "finish" || FAILED=$((FAILED + 1))

            # server + client then kill
            log_info "Running $bin server + client sequence..."
            cd "$BIN_DIR"
            ./"$bin" server &
            server_pid=$!
            sleep 1
            timeout "$CLIENT_TIMEOUT" ./"$bin" client 2>&1 || true
            sleep 1
            kill -INT $server_pid 2>/dev/null || true
            wait $server_pid 2>/dev/null || true
        done

        if [ $FAILED -gt 0 ]; then
            log_error "$FAILED tests failed"
            exit 1
        fi
        log_info "All tests passed!"
        ;;
    all-valgrind)
        FAILED=0
        log_info "=== Running standard valgrind tests ==="
        for bin in $(list_binaries); do
            # 跳过特殊项目（需要特殊处理）
            if is_special_bin "$bin"; then
                continue
            fi

            if [ -n "$TIMEOUT" ]; then
                run_valgrind "$bin" "$TIMEOUT" "" "" || FAILED=$((FAILED + 1))
            else
                run_valgrind "$bin" "10" "" "" || FAILED=$((FAILED + 1))
            fi
        done

        # 运行特殊项目的 valgrind 测试序列
        log_info "=== Running special valgrind test sequences ==="
        for bin in "${SPECIAL_BINS[@]}"; do
            if [ ! -f "$BIN_DIR/$bin" ]; then
                log_warn "Binary not found: $bin, skipping"
                continue
            fi

            # 1. client timeout 2s
            run_valgrind "$bin" "2" "client" "" || FAILED=$((FAILED + 1))

            # 2. client finish
            run_valgrind "$bin" "30" "client" "finish" || FAILED=$((FAILED + 1))

            # 3. server + client then kill
            run_valgrind_server_client_kill "$bin" "$CLIENT_TIMEOUT" || FAILED=$((FAILED + 1))
        done

        if [ $FAILED -gt 0 ]; then
            log_error "$FAILED valgrind checks failed"
            exit 1
        fi
        log_info "All valgrind checks passed!"
        ;;
    list)
        list_binaries
        ;;
esac
