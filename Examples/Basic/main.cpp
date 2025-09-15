// Prints one line per level to console.
#define TINYLOG_IMPLEMENTATION
#include "TinyLog/tinylog.hpp"

int main() {
    auto& L = tinylog::Logger::instance();
    L.add_console_sink(true);                 // colored console
    L.set_level(tinylog::level::trace);      // runtime: allow all

    LOG_TRACE("hello trace");
    LOG_DEBUG("hello debug");
    LOG_INFO ("hello info");
    LOG_WARN ("hello warn");
    LOG_ERROR("hello error");
    LOG_CRIT ("hello critical");

    { LOG_SCOPE("hello-scope"); /* do some quick work */ }
}
