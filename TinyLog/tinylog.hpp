// tinylog.hpp — tiny, header-only C++17 logger
// GNU GPL v3 License
// Features:
// • Header-only, C++17
// • Levels: trace, debug, info, warn, error, critical, off
// • Sinks: console (colored), file (with size-based rotation)
// • Thread-safe (mutex). Optional async mode via TINYLOG_ASYNC
// • Structured log fields via stream-like API
// • Compile-time level filter via TINYLOG_LEVEL
// • Source location (file:line:function)
// • Timestamps (UTC or local). Thread id.
// • RAII scope timer helper (LOG_SCOPE)
// • Minimal dependencies, no <format>, no external libs
//
// Usage:
//   #define TINYLOG_IMPLEMENTATION // in exactly one .cpp TU
//   #include "tinylog.hpp"
//   int main(){
//     tinylog::Logger::instance().add_console_sink();
//     tinylog::Logger::instance().set_log_directory("logs");         // NEW: choose dir
//     tinylog::Logger::instance().set_log_basename("TinyLog");       // NEW: choose base
//     tinylog::Logger::instance().set_log_extension(".tiny");        // NEW: choose ext
//     tinylog::Logger::instance().add_default_file_sink(2*1024*1024, 3); // NEW: logs/TinyLog.tiny
//     LOG_INFO("hello ", 123);
//     {
//       LOG_SCOPE("loading-assets");
//       // ... work
//     }
//   }
//
// Compile-time filtering (lowest level compiled in):
//   #define TINYLOG_LEVEL tinylog::level::debug
// Optional async mode:
//   #define TINYLOG_ASYNC 1

#pragma once
#include <atomic>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <queue>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <condition_variable>   // <-- needed for async queue
#ifdef __has_include
#  if __has_include(<filesystem>)
#    include <filesystem>
namespace fs = std::filesystem;
#  else
#    include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#  endif
#else
#  include <filesystem>
namespace fs = std::filesystem;
#endif

namespace tinylog {

    // Log Levels
    struct level {
        enum value : int { trace = 0, debug, info, warn, error, critical, off };
    };

#ifndef TINYLOG_LEVEL
#define TINYLOG_LEVEL tinylog::level::trace
#endif

    // Helprs (FUCKY)
    inline const char* level_name(int lv) {
        switch (lv) {
        case level::trace: return "TRACE";
        case level::debug: return "DEBUG";
        case level::info: return "INFO";
        case level::warn: return "WARN";
        case level::error: return "ERROR";
        case level::critical: return "CRIT";
        default: return "OFF";
        }
    }

    inline uint64_t now_ns() {
        return (uint64_t)std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    }

    inline std::string fmt_time(std::time_t tt, bool utc) {
        std::tm tm{};
#if defined(_WIN32)
        if (utc) gmtime_s(&tm, &tt); else localtime_s(&tm, &tt);
#else
        if (utc) gmtime_r(&tt, &tm); else localtime_r(&tt, &tm);
#endif
        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }

    // concat variadic to string
    inline void to_string_impl(std::ostringstream&) {}

    template<class T, class... Rest>
    inline void to_string_impl(std::ostringstream& oss, T&& t, Rest&&... rest) {
        oss << std::forward<T>(t);
        if constexpr (sizeof...(rest) > 0) to_string_impl(oss, std::forward<Rest>(rest)...);
    }

    template<class... Ts>
    inline std::string cat(Ts&&... ts) {
        std::ostringstream oss; oss.setf(std::ios::fixed); oss.precision(6);
        to_string_impl(oss, std::forward<Ts>(ts)...); return oss.str();
    }

    // Log Message
    struct LogMessage {
        int level;
        uint64_t ts_ns;   // monotonic
        std::time_t wall; // wall clock seconds
        std::thread::id tid;
        const char* file;
        const char* func;
        int line;
        std::string text;
    };

    // Pretty, unified formatter (used by both sinks)
    inline std::string format_line(const LogMessage& m, bool utc, bool colorize = false) {
        // Example: LOC 2025-09-15 22:13:31 [DEBUG] (thread:1234) main.cpp:24 main | message...
        std::ostringstream line;
        line << (utc ? "UTC" : "LOC") << " " << fmt_time(m.wall, utc)
            << " [" << level_name(m.level) << "] "
            << "(tid:" << m.tid << ") "
            << fs::path(m.file).filename().string() << ":" << m.line << " "
            << m.func << " | " << m.text;
        if (!colorize) return line.str();

        const char* col = "\033[0m";
        switch (m.level) {
        case level::trace:    col = "\033[90m"; break;
        case level::debug:    col = "\033[36m"; break;
        case level::info:     col = "\033[37m"; break;
        case level::warn:     col = "\033[33m"; break;
        case level::error:    col = "\033[31m"; break;
        case level::critical: col = "\033[41;97m"; break;
        default: break;
        }
        std::ostringstream colored;
        colored << col << line.str() << "\033[0m";
        return colored.str();
    }

    // Sink Interface
    class LogSink {
    public:
        virtual ~LogSink() = default;
        virtual void write(const LogMessage& m) = 0;
    };

    // Console Sink (Colored)
    class ConsoleSink : public LogSink {
        bool use_color_;
        bool utc_;
    public:
        explicit ConsoleSink(bool color = true, bool utc = false) : use_color_(color), utc_(utc) {}
        void write(const LogMessage& m) override {
            std::cout << format_line(m, utc_, use_color_) << "\n";
            // Force immediate flush so short-lived programs always show logs:
            std::cout << std::flush;
        }
    };

    // File Sink w/ rotation
    class FileSink : public LogSink {
        std::mutex mtx_;
        std::string path_;
        size_t max_bytes_;
        int max_files_;
        std::ofstream out_;
        bool utc_;

        void ensure_dir() {
            std::error_code ec;
            fs::create_directories(fs::path(path_).parent_path(), ec);
        }
        void open_file() {
            if (out_.is_open()) return;
            ensure_dir();
            out_.open(path_, std::ios::app | std::ios::out);
        }
        uintmax_t size_now() {
            std::error_code ec; auto s = fs::file_size(path_, ec); return ec ? 0 : s;
        }
        void rotate_if_needed() {
            if (max_bytes_ == 0) return;
            auto s = size_now();
            if (s < max_bytes_) return;
            out_.close();
            for (int i = max_files_ - 1; i >= 1; --i) {
                fs::path from = (i == 1 ? fs::path(path_) : fs::path(path_).concat("." + std::to_string(i - 1)));
                fs::path to = fs::path(path_).concat("." + std::to_string(i));
                std::error_code ec; if (fs::exists(from, ec)) {
                    std::error_code ec2; fs::rename(from, to, ec2);
                    if (ec2) { fs::remove(to, ec2); fs::rename(from, to, ec2); }
                }
            }
            open_file();
        }
    public:
        FileSink(std::string path, size_t max_bytes = 5 * 1024 * 1024, int max_files = 3, bool utc = false)
            : path_(std::move(path)), max_bytes_(max_bytes), max_files_(max_files), utc_(utc) {
            open_file();
        }
        void write(const LogMessage& m) override {
            std::lock_guard<std::mutex> lk(mtx_);
            rotate_if_needed();
            if (!out_.is_open()) open_file();
            out_ << format_line(m, utc_, false) << '\n';
            out_.flush();
        }
    };

    // Async Queue (Optional, but handy)
#if TINYLOG_ASYNC
    class MPMCQueue {
        std::mutex mtx_; std::condition_variable cv_; std::queue<LogMessage> q_; bool closed_ = false;
    public:
        void push(LogMessage m) { std::lock_guard<std::mutex> lk(mtx_); q_.push(std::move(m)); cv_.notify_one(); }
        bool pop(LogMessage& out) { std::unique_lock<std::mutex> lk(mtx_); cv_.wait(lk, [&] {return closed_ || !q_.empty(); }); if (q_.empty()) return false; out = std::move(q_.front()); q_.pop(); return true; }
        void close() { std::lock_guard<std::mutex> lk(mtx_); closed_ = true; cv_.notify_all(); }
    };
#endif

    // ---------- Logger ----------
    class Logger {
    public:
        using sink_ptr = std::shared_ptr<LogSink>;
    private:
        std::mutex mtx_;
        std::vector<sink_ptr> sinks_;
        std::atomic<int> level_{ TINYLOG_LEVEL };
        bool utc_{ false };

        // NEW: default file location pieces
        std::string log_dir_ = "logs";
        std::string log_base_ = "TinyLog";
        std::string log_ext_ = ".tiny";

#if TINYLOG_ASYNC
        MPMCQueue q_;
        std::thread worker_;
        std::atomic<bool> running_{ false };
#endif

        Logger() = default;
        ~Logger() {
#if TINYLOG_ASYNC
            if (running_) { running_ = false; q_.close(); if (worker_.joinable()) worker_.join(); }
#endif
        }

        std::string sanitized_ext(const std::string& e) const {
            if (e.empty()) return ".tiny";
            return (e.front() == '.') ? e : (std::string(".") + e);
        }
    public:
        Logger(const Logger&) = delete; Logger& operator=(const Logger&) = delete;
        static Logger& instance() { static Logger L; return L; }

        void set_level(int lv) { level_.store(lv, std::memory_order_relaxed); }
        int  get_level() const { return level_.load(std::memory_order_relaxed); }
        void set_utc(bool v) { utc_ = v; }

        // NEW: configure default log location
        void set_log_directory(const std::string& dir) { log_dir_ = dir; }
        void set_log_basename(const std::string& base) { log_base_ = base; }
        void set_log_extension(const std::string& ext) { log_ext_ = sanitized_ext(ext); }

        std::string default_log_path() const {
            fs::path p = fs::path(log_dir_) / (log_base_ + log_ext_);
            return p.string();
        }

        void add_sink(const sink_ptr& s) { std::lock_guard<std::mutex> lk(mtx_); sinks_.push_back(s); }
        void add_console_sink(bool color = true) { add_sink(std::make_shared<ConsoleSink>(color, utc_)); }

        // Original explicit path API (unchanged)
        void add_file_sink(const std::string& path, size_t max_bytes = 5 * 1024 * 1024, int max_files = 3) {
            add_sink(std::make_shared<FileSink>(path, max_bytes, max_files, utc_));
        }

        // NEW: uses configured dir/base/ext to build the path (e.g., logs/TinyLog.tiny)
        void add_default_file_sink(size_t max_bytes = 5 * 1024 * 1024, int max_files = 3) {
            add_sink(std::make_shared<FileSink>(default_log_path(), max_bytes, max_files, utc_));
        }

#if TINYLOG_ASYNC
        void start_async() { if (running_) return; running_ = true; worker_ = std::thread([this] { LogMessage m; while (running_) { if (!q_.pop(m)) break; dispatch(m); } }); }
#endif

        template<class... Ts>
        void log(int lv, const char* file, int line, const char* func, Ts&&... ts) {
            if (lv < level_.load(std::memory_order_relaxed)) return;
            LogMessage m; m.level = lv; m.ts_ns = now_ns(); m.wall = std::time(nullptr); m.tid = std::this_thread::get_id(); m.file = file; m.line = line; m.func = func; m.text = cat(std::forward<Ts>(ts)...);
#if TINYLOG_ASYNC
            if (running_) { q_.push(std::move(m)); return; }
#endif
            dispatch(m);
        }

    private:
        void dispatch(const LogMessage& m) { std::lock_guard<std::mutex> lk(mtx_); for (auto& s : sinks_) s->write(m); }
    };

    // ---------- Scope Timer ----------
    class ScopeTimer {
        const char* file_; const char* func_; int line_; int lv_; std::string name_;
        std::chrono::high_resolution_clock::time_point start_;
    public:
        ScopeTimer(const char* file, int line, const char* func, int level, std::string name)
            : file_(file), func_(func), line_(line), lv_(level), name_(std::move(name)), start_(std::chrono::high_resolution_clock::now()) {
        }
        ~ScopeTimer() {
            using namespace std::chrono;
            auto end = high_resolution_clock::now();
            auto us = duration_cast<microseconds>(end - start_).count();
            Logger::instance().log(lv_, file_, line_, func_, name_, " took ", us, "us");
        }
    };

    // ---------- Macros ----------
#define LOG_AT(lv, ...) tinylog::Logger::instance().log(lv, __FILE__, __LINE__, __func__, __VA_ARGS__)

#if TINYLOG_LEVEL <= tinylog::level::trace
#  define LOG_TRACE(...) LOG_AT(tinylog::level::trace, __VA_ARGS__)
#else
#  define LOG_TRACE(...) (void)0
#endif
#if TINYLOG_LEVEL <= tinylog::level::debug
#  define LOG_DEBUG(...) LOG_AT(tinylog::level::debug, __VA_ARGS__)
#else
#  define LOG_DEBUG(...) (void)0
#endif
#if TINYLOG_LEVEL <= tinylog::level::info
#  define LOG_INFO(...)  LOG_AT(tinylog::level::info,  __VA_ARGS__)
#else
#  define LOG_INFO(...) (void)0
#endif
#if TINYLOG_LEVEL <= tinylog::level::warn
#  define LOG_WARN(...)  LOG_AT(tinylog::level::warn,  __VA_ARGS__)
#else
#  define LOG_WARN(...) (void)0
#endif
#if TINYLOG_LEVEL <= tinylog::level::error
#  define LOG_ERROR(...) LOG_AT(tinylog::level::error, __VA_ARGS__)
#else
#  define LOG_ERROR(...) (void)0
#endif
#if TINYLOG_LEVEL <= tinylog::level::critical
#  define LOG_CRIT(...)  LOG_AT(tinylog::level::critical, __VA_ARGS__)
#else
#  define LOG_CRIT(...) (void)0
#endif

#define LOG_SCOPE(name) tinylog::ScopeTimer TINYLOG_UNIQUE_NAME(_tl_scope_){__FILE__,__LINE__,__func__, tinylog::level::debug, name}

// helper to create unique var name
#define TINYLOG_CONCAT_INNER(a,b) a##b
#define TINYLOG_CONCAT(a,b) TINYLOG_CONCAT_INNER(a,b)
#define TINYLOG_UNIQUE_NAME(prefix) TINYLOG_CONCAT(prefix, __LINE__)

#ifdef TINYLOG_IMPLEMENTATION
// (header-only; nothing to compile here)
#endif

} // namespace tinylog
