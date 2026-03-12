#pragma once

#include <string>
#include <utility>
#include <vector>

namespace uwbp::common
{

// Schreibt/liest aktive AP-Connections in eine Textdatei.
// Format: "<connectionPath>\t<activeConnectionPath>" pro Zeile.
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
