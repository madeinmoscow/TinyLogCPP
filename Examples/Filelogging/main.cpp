// Logs to: logs/YYYY-MM-DD_HH-MM-SS.tiny (or .txt if you change extension)
#define TINYLOG_IMPLEMENTATION
#include "TinyLog/tinylog.hpp"
#include <ctime>
#include <iomanip>
#include <sstream>

static std::string ts_name(bool utc=false){
    std::time_t tt = std::time(nullptr);
    std::tm tm{};
#if defined(_WIN32)
    if (utc) gmtime_s(&tm,&tt); else localtime_s(&tm,&tt);
#else
    if (utc) gmtime_r(&tt,&tm); else localtime_r(&tt,&tm);
#endif
    std::ostringstream o; o<<std::put_time(&tm, "%Y-%m-%d_%H-%M-%S");
    return o.str();
}

int main(){
    auto& L = tinylog::Logger::instance();
    L.set_utc(false);                         // line timestamps in local time
    L.add_console_sink(true);

    L.set_log_directory("logs");
    L.set_log_extension(".tiny");             // change to ".txt" if you like
    L.set_log_basename(ts_name(false));       // filename = <timestamp>.<ext>
    L.add_default_file_sink(2*1024*1024, 3);  // rotate at 2MB, keep 3 backups

    LOG_INFO("timestamped file example started");
    { LOG_SCOPE("init"); }
    LOG_INFO("log path ready");
}
