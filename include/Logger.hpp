#pragma once

#include <mutex>
#include <string>

class Logger {
public:
    void log(const std::string& ip, const std::string& method,
             const std::string& path, int status);

private:
    std::mutex logMutex;
};
