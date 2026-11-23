#include "Node.h"
Node::Node(int id) : id_(id) {}
int Node::id() const
{
    return id_;
}
void Node::receiveBatch(std::vector<Record> &rs)
{
    std::lock_guard<std::mutex> l(mutex_);
    received_ += rs.size();
    storage_.putBatch(rs);
}
std::size_t Node::uniqueCount() const
{
    std::lock_guard<std::mutex> l(mutex_);
    return storage_.size();
}
std::size_t Node::receivedCount() const
{
    std::lock_guard<std::mutex> l(mutex_);
    return received_;
}
std::size_t Node::payloadBytes() const
{
    std::lock_guard<std::mutex> l(mutex_);
    return storage_.payloadBytes();
}
std::vector<std::string> Node::keys() const
{
    std::lock_guard<std::mutex> l(mutex_);
    std::vector<std::string> v;
    v.reserve(storage_.all().size());
    for (const auto &x : storage_.all())
        v.push_back(x.first);
    return v;
}
