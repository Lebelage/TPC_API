#pragma once
#include <expected>
#include <memory>
#include <models/data.hpp>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace tpc::system::client {

class FrameReceiver {
public:
    [[nodiscard]] static std::expected<std::unique_ptr<FrameReceiver>, std::string>
    create(std::uint16_t receive_count = 36);

    ~FrameReceiver() = default;

    FrameReceiver(const FrameReceiver&) = delete;
    FrameReceiver(FrameReceiver&&) = delete;
    FrameReceiver& operator=(const FrameReceiver&) = delete;
    FrameReceiver& operator=(FrameReceiver&&) = delete;

public:
    std::expected<void, std::string> add_back(opcua::NodeId node, double value);

    std::expected<std::unordered_map<opcua::NodeId, double>, std::string> get_frame() const;

private:
    explicit FrameReceiver(std::uint16_t received_queue_size);

private:
    std::unordered_map<opcua::NodeId, double> received_;

    std::uint16_t max_received_queue_size_{36};

    mutable std::mutex mutex_;
};

} // namespace tpc::system::client