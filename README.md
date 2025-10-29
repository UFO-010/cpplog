# Logging library

## Overview

This logging library provides a compile-time configurable, high-performance logging facility designed for both desktop and embedded environments. The core design emphasizes minimal runtime overhead, static configuration, and extensibility through user-defined data providers and output sinks.

The logger is implemented as a header-only C++17 template library. Logging behavior—including enabled log levels, output formatting, and destination handling—is controlled through compile-time constants and template parameters, enabling aggressive dead-code elimination by the compiler.

## Why Do We Need Another Logging Library?

Because there is clearly the lack of logging librires in the world, especially ones that support custom formatting, constexpr hacks and reivent token parsing to save expensive nanoseconds. This yet another logging library exists because I just wanted to learn about some C++ mechanics, such as:
- Metaprogramming
- Zero-cost abstractions
- Compile-time evaluation
- Type-safety
- Absolute joy of using fixed-size buffers without allocations.

In general, it's less about logging and more about improving low-level C++ craftsmanship with saving type-safety (which is ironic, because my journey began with C). If you want to find yet another production-ready logging framework, this is the wrong place. But if you appreciate a minimal, explicit and inentionally limited implementation forged in the fire of personal learning, you're welcome. And remember, no mention about type safety.

## Core Components

### `Log::Logger`

The `Logger` class is the central component of the logging system. It is parameterized by:

- **`TDataProvider`**: A user-provided type that supplies platform-specific runtime data (timestamps, thread IDs, process name or ID).
- **`TSinkTypes...`**: Zero or more sinks that define where log messages are dispatched.

#### Key Characteristics

- **Non-copyable and non-movable**: Enforces single-instance usage.
- **Compile-time and runtime log level filtering**:  Controlled via macros in `logger_config.h` and function `setLogLevel(level)`
- **Configurable output format**:  Supports pattern-based message formatting
- **Thread-local message buffer**: Uses a  stack-allocated buffer with fixed-size (size can be configured via `LOGGER_MAX_STR_SIZE` in `logger_config.h`).

#### Public Interface

- `setLogLevel(level)`: Sets the runtime log level threshold. Messages with a priority lower than this threshold are discarded.
- `setLogPattern(const char*)`: Configures the output message format. Supported tokens:
  - `%{date}` – current date
  - `%{time}` – current time
  - `%{level}` – log severity (e.g., "INFO")
  - `%{file}` – source filename
  - `%{function}` – function signature
  - `%{line}` – source line number
  - `%{thread}` – thread identifier
  - `%{pid}` – process name or ID
  - `%{message}` – user-provided log content
- `setUserHandler(...)`: Registers a user-defined callback for log messages (enabled only if `ENABLE_PRINT_CALLBACK` is true).
- `log(const LogRecord&, const char*, size_t)`: Primary logging entry point, typically invoked via macros.

### `Log::LogRecord`

A lightweight, compile-time construct that captures static context about a log site:

- Log severity (`level`)
- Source file (`__FILE__` as `std::string_view`)
- Function signature (`__PRETTY_FUNCTION__`, `__FUNCSIG__`, or `__func__`)
- Line number (`__LINE__`)
  
All members are `constexpr`, enabling zero-cost capture of source location metadata.

### Logging Macros

The following macros provide convenient, level-specific logging interfaces:

- `Debug(LoggerType, message, len)`
- `Info(LoggerType, message, len)`
- `Warning(LoggerType, message, len)`
- `Error(LoggerType, message, len)`
- `Fatal(LoggerType, message, len)`

Each macro:
- Is conditionally compiled based on `LOGGER_LOG_*_ENABLED` flags in `logger_config.h`.
- Captures file, function, and line information at the call site.
- Forwards to the logger instance’s `log()` method.


These macros ensure that disabled logging levels take no runtime overhead, including the evaluation of the message expression.

### Sinks

Sinks are output destinations for formatted log messages. A sink must inherit from `Log::ILogSink<ConcreteSink>` and implement:

```cpp
void send(Log::level msgType, const char* data, size_t size) const;
```

The `Logger` dispatches messages to all provided sinks via recursive template expansion. Sink usage is controlled by the `ENABLE_SINKS` compile-time flag in `logger_config.h`.

Example: `ConsoleSink` (provided) outputs to `std::cout` with optional ANSI color support on Windows and POSIX systems.

### Data Provider

The `TDataProvider` template parameter must implement the following methods (signatures as used in `DefaultDataProvider`):

- `size_t getCurrentDate(char* buffer, size_t bufferSize) const`
- `size_t getCurrentTime(char* buffer, size_t bufferSize) const`
- `size_t getThreadId(char* buffer, size_t bufferSize) const`
- `size_t getProcessName(char* buffer, size_t bufferSize) const`

These methods are called only when the corresponding token appears in the log pattern. The `DefaultDataProvider` implements these for desktop platforms (Linux, macOS, Windows).

> **Note**: Embedded implementations will require a platform-specific data provider that avoids standard library dependencies and system calls.

## Configuration

All behavioral parameters are defined in `logger_config.h` as compile-time constants:

| Constant | Description | Default |
|--------|-------------|--------|
| `LOGGER_MAX_SINKS` | Maximum number of sinks | 4 |
| `LOGGER_MAX_STR_SIZE` | Maximum formatted message length | 512 |
| `LOGGER_MAX_MESSAGE_SIZE` | Maximum user message length | 256 |
| `LOGGER_MAX_TOKENS` | Maximum tokens in pattern | 9 |
| `LOGGER_LITERAL_BUFFER_SIZE` | Buffer for literal text in pattern | 64 |
| `ENABLE_PRINT_CALLBACK` | Enable user callback support | `false` |
| `ENABLE_SINKS` | Enable sink dispatch | `true` |
| `LOGGER_MAX_LEVEL` | Highest enabled log level (0=FATAL, 4=DEBUG) | 4 |
| `LOGGER_LOG_*_ENABLED` | Per-level compile-time switches | Derived from `LOGGER_MAX_LEVEL` |

Modifying these values allows tuning memory usage and feature set for resource-constrained environments.

## Usage Example (Desktop)

```cpp
#include "logger.h"
#include "console_sink.h"
#include "default_provider.h"

int main() {
    DefaultDataProvider defaultDataProvider;  // example simple data provider
    ConsoleSink consoleSink;                  // example sink that prints data to console
    Log::Logger myLogger(defaultDataProvider, consoleSink);

    consoleSink.colorize(true);
    myLogger.setLogLevel(Log::level::DebugMsg);  // set minimum logging level

    //  set message pattern
    myLogger.setLogPattern(
        "%{level}\t %{date} %{time} %{pid} file %{file} "
        "function %{function} line %{line} %{message}");

    Error(myLogger, "aaa\n", sizeof("aaa\n"));  // simple log

    return 0;
}
```
