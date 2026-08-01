#include "Logger.hpp"

#include <iostream>

void Logger::log(const std::string& ip, const std::string& method,
                 const std::string& path, int status) {
    std::lock_guard<std::mutex> lock(logMutex);
    std::cout << ip << " " << method << " " << path << " " << status << std::endl;
}
