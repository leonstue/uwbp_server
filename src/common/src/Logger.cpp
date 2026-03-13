#include "../header/Logger.h"

#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace uwbp::common
{

Logger::Logger(const std::string& filepath)
{
    auto dir = std::filesystem::path(filepath).parent_path();
    if (!dir.empty())
        std::filesystem::create_directories(dir);

    file_.open(filepath, std::ios::app);
}

Logger::~Logger()
{
    if (file_.is_open())
        file_.close();
}

void Logger::info(const std::string& msg)
{
    write("INFO", msg);
    std::cout << msg << "\n";
}

void Logger::error(const std::string& msg)
{
    write("ERROR", msg);
    std::cerr << msg << "\n";
}

void Logger::write(const std::string& level, const std::string& msg)
{
    if (!file_.is_open())
        return;

    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);

    std::ostringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S")
       << " [" << level << "] " << msg;

    file_ << ss.str() << "\n";
    file_.flush();
}

} // namespace uwbp::common
