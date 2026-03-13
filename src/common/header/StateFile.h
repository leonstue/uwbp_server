#pragma once

#include <string>
#include <utility>
#include <vector>

namespace uwbp::common
{

// persists active AP conections to a file so the watchdog
// can clean them up if the server dies unexpectedly.
// format: "<connectionPath>\t<activeConnectionPath>" per line
class StateFile
{
public:
    explicit StateFile(std::string path);

    void write(const std::vector<std::pair<std::string, std::string>>& entries) const;
    std::vector<std::pair<std::string, std::string>> read() const;
    void remove() const;

    const std::string& path() const;

private:
    std::string path_;
};

} // namespace uwbp::common
