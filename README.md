# TinyLog 🪶

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](LICENSE)
![C++17](https://img.shields.io/badge/C%2B%2B-17-orange.svg)
![Header-only](https://img.shields.io/badge/header--only-lightgrey.svg)
![Status](https://img.shields.io/badge/build-passing-brightgreen.svg)

**TinyLog** is a **header-only C++17 logging library** that’s small but powerful.  
It was built for **mods, tools, and lightweight apps** where you need structured, reliable logs without heavy dependencies.

---

## ✨ Features
- 🔹 **Header-only** — just drop `tinylog.hpp` in your project.
- 🔹 **Log levels** — `trace, debug, info, warn, error, critical`.
- 🔹 **Thread-safe** (mutex-protected).
- 🔹 **Optional async mode** (`TINYLOG_ASYNC`).
- 🔹 **Colored console output**.
- 🔹 **File sink with rotation** (size-based).
- 🔹 **Source location** — file, line, function, thread ID.
- 🔹 **Timestamps** (UTC or local).
- 🔹 **Compile-time filtering** (`TINYLOG_LEVEL`).
- 🔹 **RAII scope timers** (`LOG_SCOPE`).
- 🔹 **Zero external deps** — no `fmt`, no `spdlog`.

---

## 🚀 Quick Start
```cpp
#define TINYLOG_IMPLEMENTATION
#include "tinylog.hpp"

int main() {
    auto& log = tinylog::Logger::instance();

    // Add sinks
    log.add_console_sink(true); // colored console
    log.add_file_sink("tinylog.log", 2*1024*1024, 3); // 2MB rotate, 3 backups

    LOG_INFO("TinyLog initialized!");
    {
        LOG_SCOPE("asset-load");
        // ... work ...
    }
    LOG_WARN("health below threshold: ", 42);
}
```



