#include "DataLoader.h"
#include <fstream>
#include <stdexcept>
#include <utility>
DataLoader::DataLoader(const Config &c, const Schema &s, const Distributor &d, TcpTransport &t, Node *n)
    : config_(c), schema_(s), distributor_(d), transport_(t), localNode_(n) {}
LoadStats DataLoader::loadFile(const std::string &path, int sourceNodeId)
{
    LoadStats st;
    std::ifstream in(path, std::ios::binary);
    if (!in)
        throw std::runtime_error("Cannot open local input file: " + path);
    in.seekg(0, std::ios::end);
    auto end = in.tellg();
    if (end > 0)
        st.fileBytes = static_cast<std::size_t>(end);
    in.seekg(0);
    std::vector<std::vector<Record>> out(static_cast<std::size_t>(config_.nodes));
    for (auto &b : out)
        b.reserve(config_.batchSize);
    std::string line;
    auto flush = [&](int owner)
    {
        auto &b = out[static_cast<std::size_t>(owner)];
        if (b.empty())
            return;
        if (localNode_ && owner == sourceNodeId)
            localNode_->receiveBatch(b);
        else
            transport_.sendBatch(owner, b);
        ++st.batchesSent;
    };
    while (std::getline(in, line))
    {
        if (line.empty())
            continue;
        ++st.rowsRead;
        auto p = line.find(config_.delimiter);
        if (p == std::string::npos || line.find(config_.delimiter, p + 1) != std::string::npos)
        {
            ++st.invalidRows;
            continue;
        }
        Record r{line.substr(0, p), line.substr(p + 1)};
        std::string why;
        if (!schema_.validate(r, why))
        {
            ++st.invalidRows;
            continue;
        }
        int owner = distributor_.ownerOf(r.key);
        auto &b = out[static_cast<std::size_t>(owner)];
        b.emplace_back(std::move(r));
        if (b.size() >= config_.batchSize)
            flush(owner);
    }
    for (int i = 0; i < config_.nodes; ++i)
        flush(i);
    return st;
}
