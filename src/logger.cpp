#include "logger.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>

Logger& Logger::instance() {
    static Logger logger;
    return logger;
}

void Logger::log(const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);

    const auto now = std::chrono::system_clock::now();
    const std::time_t now_time = std::chrono::system_clock::to_time_t(now);

    std::tm tm{};
    localtime_r(&now_time, &tm);

    std::cout << '[' << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "] "
              << message << '\n';
}
