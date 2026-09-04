#include "frame_receiver.hpp"

namespace tpc::system::client {

#pragma region Fabric/Constructor

std::expected<std::unique_ptr<FrameReceiver>, std::string> FrameReceiver::create(std::uint16_t receive_count) {
    try {
        return std::unique_ptr<FrameReceiver>{new FrameReceiver(receive_count)};
    } catch (std::exception& e) {
        return std::unexpected{e.what()};
    }
    return {};
}

FrameReceiver::FrameReceiver(std::uint16_t received_queue_size) {
    received_.reserve(received_queue_size);
}

#pragma endregion

#pragma region Public methods

std::expected<void, std::string> FrameReceiver::add_back(opcua::NodeId node, double value) {
    std::lock_guard lock{mutex_};

    if (max_received_queue_size_ == 0) {
        return std::unexpected{"Received queue capacity is zero"};
    }

    received_.insert_or_assign(node, value);

    return {};
}

std::expected<std::unordered_map<opcua::NodeId, double>, std::string> FrameReceiver::get_frame() const {
    std::lock_guard lock{mutex_};

    if (received_.empty())
        return std::unexpected("Frame received queue is empty");

    return received_;
}

#pragma endregion
} // namespace tpc::system::client