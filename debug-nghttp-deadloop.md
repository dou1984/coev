# Debug Session: nghttp-deadloop

## Status: [OPEN]

## Bug Description
co_nghttp server 和 client 运行时可能出现问题（死循环/高 CPU/连接异常）

## Hypotheses
1. **H1**: `__processing_write()` 中 `r == 0` 分支导致死循环（没有等待操作）
2. **H2**: `__send()` 中 `m_want_read`/`m_want_write` 标志未被正确重置
3. **H3**: `__w_waiter()` 实现缺失或不正确
4. **H4**: SSL 层 WANT_READ/WANT_WRITE 处理逻辑有问题

## Pre-fix Analysis
- 代码审查发现 `__processing_write()` L400-403 在 `r == 0` 时只打印日志，没有等待操作
- 会导致 `while (__is_processing())` 无限循环

## Instrumentation Plan
- 在 `__processing_write()` 添加循环计数器，检测死循环
- 在 `__send()` 添加标志状态日志
- 在 `__ssl_write()` / `__ssl_read()` 添加 WANT_READ/WANT_WRITE 日志

## Evidence Collection
[待收集]

## Fix
[待定]

## Verification
[待定]
