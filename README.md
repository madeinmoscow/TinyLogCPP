# TinyLog

A tiny, header-only C++17 logger with powerful features and minimal dependencies.

## Features

- **Header-only** - Just include and go
- **C++17 standard** - Modern C++ with no external dependencies
- **Multiple log levels** - TRACE, DEBUG, INFO, WARN, ERROR, CRITICAL, OFF
- **Multiple sinks** - Console (with colors) and file (with rotation)
- **Thread-safe** - Mutex protection with optional async mode
- **Scope timing** - Built-in RAII performance measurement
- **Compile-time filtering** - Zero-cost disabled log levels
- **Source location** - Automatic file:line:function capture
- **Flexible formatting** - Stream-like API for any printable type

## Quick Start

```cpp
#define TINYLOG_LEVEL 0  // Enable all levels (0=TRACE)
#define TINYLOG_IMPLEMENTATION
#include "tinylog.hpp"

int main() {
    auto& logger = tinylog::Logger::instance();
    
    // Add console output with colors
    logger.add_console_sink(true);
    
    // Add file output with rotation (5MB files, keep 3 backups)
    logger.set_log_directory("logs");
    logger.set_log_basename("myapp");
    logger.set_log_extension(".log");
    logger.add_default_file_sink(5*1024*1024, 3);
    
    // Start logging
    LOG_INFO("Application started");
    LOG_WARN("This is a warning with data: ", 42);
    LOG_ERROR("Something went wrong!");
    
    {
        LOG_SCOPE("database-operation");
        // Automatically times this scope
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    } // Logs: "database-operation took 100123us"
    
    return 0;
}
```

## Output Example

```
LOC 2025-09-15 22:41:31 [INFO] (tid:1234) main.cpp:15 main | Application started
LOC 2025-09-15 22:41:31 [WARN] (tid:1234) main.cpp:16 main | This is a warning with data: 42
LOC 2025-09-15 22:41:31 [ERROR] (tid:1234) main.cpp:17 main | Something went wrong!
LOC 2025-09-15 22:41:31 [DEBUG] (tid:1234) main.cpp:19 main | database-operation took 100123us
```

## Log Levels

```cpp
LOG_TRACE("Detailed debug info");     // Level 0
LOG_DEBUG("Debug information");       // Level 1  
LOG_INFO("General information");      // Level 2
LOG_WARN("Warning message");          // Level 3
LOG_ERROR("Error occurred");          // Level 4
LOG_CRIT("Critical system error");    // Level 5
```

## Compile-Time Filtering

Set `TINYLOG_LEVEL` before including the header to filter out log levels at compile time:

```cpp
#define TINYLOG_LEVEL 2  // Only INFO and above compiled in
#include "tinylog.hpp"

// These become no-ops (zero overhead):
LOG_TRACE("This disappears");  // Compiled out
LOG_DEBUG("This too");         // Compiled out

// These remain active:
LOG_INFO("This stays");        // Compiled in
LOG_ERROR("This too");         // Compiled in
```

## Async Mode (Optional)

For high-performance applications, enable async logging:

```cpp
#define TINYLOG_ASYNC 1
#define TINYLOG_IMPLEMENTATION
#include "tinylog.hpp"

int main() {
    auto& logger = tinylog::Logger::instance();
    logger.add_console_sink();
    logger.start_async();  // Start background thread
    
    // All logging now happens asynchronously
    for (int i = 0; i < 10000; ++i) {
        LOG_INFO("High volume log ", i);
    }
    
    return 0; // Background thread auto-joins on exit
}
```

## File Rotation

```cpp
// Rotate when files reach 1MB, keep 5 backup files
logger.add_file_sink("app.log", 1*1024*1024, 5);

// Creates: app.log, app.log.1, app.log.2, app.log.3, app.log.4
```

## Advanced Configuration

```cpp
// Use UTC timestamps instead of local time
logger.set_utc(true);

// Set runtime log level (can be changed at runtime)
logger.set_level(tinylog::level::warn);

// Multiple file sinks with different configurations  
logger.add_file_sink("errors.log", 5*1024*1024, 3);    // Error log
logger.add_file_sink("debug.log", 10*1024*1024, 10);   // Debug log

// Custom log directory structure
logger.set_log_directory("logs/2025/09");
logger.set_log_basename("myservice");  
logger.set_log_extension(".txt");
logger.add_default_file_sink(); // Creates: logs/2025/09/myservice.txt
```

## Requirements

- C++17 compatible compiler
- Standard library with `<filesystem>` support
- For async mode: `<thread>` and `<condition_variable>`

## Compilation

```bash
# Basic usage
g++ -std=c++17 myapp.cpp -o myapp

# With async mode
g++ -std=c++17 myapp.cpp -o myapp -lpthread

# Visual Studio - just build normally with C++17 enabled
```

## License

GNU General Public License v3.0

## Thread Safety

TinyLog is fully thread-safe:
- All operations are mutex-protected
- Async mode uses lock-free queues where possible
- Safe to call from multiple threads simultaneously
- Automatic cleanup on application exit

## Performance Notes

- Compile-time filtering has zero runtime cost
- Async mode recommended for high-throughput applications  
- Console output auto-flushes for immediate visibility
- File rotation handles large files efficiently
- Scope timers use high-resolution clocks

