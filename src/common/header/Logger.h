#pragma once

#include <fstream>
#include <string>

namespace uwbp::common
{

// simple file logger, creates the log directory if it doesnt exist.
// also prints to stdout/stderr so you still see output in the terminal.
class Logger
{
public:
    // filepath relative to the running binary, e.g. "logs/server.log"
    explicit Logger(const std::string& filepath);
    ~Logger();

    void info(const std::string& msg);
    void error(const std::string& msg);

private:
    void write(const std::string& level, const std::string& msg);
    std::ofstream file_;
};

} // namespace uwbp::common
