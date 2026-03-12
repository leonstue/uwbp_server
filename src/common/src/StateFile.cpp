#include "../header/StateFile.h"

#include <filesystem>
#include <fstream>

namespace uwbp::common
{

StateFile::StateFile(std::string path)
    : path_(std::move(path))
{
}

void StateFile::write(
    const std::vector<std::pair<std::string, std::string>>& entries) const
{
    auto parent = std::filesystem::path(path_).parent_path();
    if (!parent.empty())
        std::filesystem::create_directories(parent);

    std::ofstream ofs(path_, std::ios::trunc);
    for (const auto& [conn, active] : entries)
        ofs << conn << '\t' << active << '\n';
}

std::vector<std::pair<std::string, std::string>> StateFile::read() const
{
    std::vector<std::pair<std::string, std::string>> out;
    std::ifstream ifs(path_);
    if (!ifs.is_open())
        return out;

    std::string line;
    while (std::getline(ifs, line))
    {
        auto tab = line.find('\t');
        if (tab != std::string::npos)
            out.emplace_back(line.substr(0, tab), line.substr(tab + 1));
    }
    return out;
}

void StateFile::remove() const
{
    std::filesystem::remove(path_);
}

const std::string& StateFile::path() const
{
    return path_;
}

} // namespace uwbp::common
