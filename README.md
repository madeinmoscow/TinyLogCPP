# TinyLog 🪶

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](LICENSE)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-orange.svg)]()
[![Header-only](https://img.shields.io/badge/header--only-lightgrey.svg)]()
[![Build Status](https://img.shields.io/badge/build-passing-brightgreen.svg)]()

**TinyLog** is a **header-only C++17 logging library** that’s small but powerful.  
It was built for **mods, tools, and lightweight apps** where you need reliable logging without pulling in heavy dependencies.

---

## ✨ Features
- 🔹 **Header-only**: just drop `microlog.hpp` in your project.
- 🔹 **Log levels**: `trace, debug, info, warn, error, critical`.
- 🔹 **Thread-safe** (mutex protected).
- 🔹 **Optional async mode** (`MICROLOG_ASYNC`).
- 🔹 **Colored console logs**.
- 🔹 **File sink with rotation** (size-based).
- 🔹 **Source location** (file, line, function, thread id).
- 🔹 **Timestamps** (UTC or local).
- 🔹 **Compile-time filtering** via `MICROLOG_LEVEL`.
- 🔹 **RAII scope timers** (`LOG_SCOPE`).

---

## 🚀 Quick Start
```cpp
#define MICROLOG_IMPLEMENTATION
#include "microlog.hpp"

int main() {
    auto& log = microlog::Logger::instance();
    log.add_console_sink(true); // enable colors
    log.add_file_sink("tinylog.log", 2*1024*1024, 3); // rotate @ 2MB, keep 3 backups

    LOG_INFO("TinyLog initialized!");
    {
        LOG_SCOPE("asset-load");
        // do work...
    }
    LOG_WARN("health below threshold: ", 42);
}
⚙️ Configuration
Set minimum level (compile-time):

cpp
Copy code
#define MICROLOG_LEVEL microlog::level::info
Enable async mode:

cpp
Copy code
#define MICROLOG_ASYNC 1
📦 Why TinyLog?
✅ Perfect for mods & DLLs — no dependencies, small footprint.

✅ Minimal overhead — compile-time filtering keeps it fast.

✅ Clear, structured logs — debug memory hooks, patches, or scripts easily.

✅ Safe & production-ready — rotation, thread safety, RAII timers included.

📜 License
This project is licensed under the GNU General Public License v3.0 (GPL-3.0).
See the LICENSE file for details.

💡 Roadmap
 Daily log rollover

 JSON/structured log sink

 ImGui in-game console sink

 Crash-safe append & backtrace support

🤝 Contributing
Pull requests, feature ideas, and feedback are welcome!
