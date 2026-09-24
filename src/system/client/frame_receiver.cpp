#include "tpc/system/client/frame_receiver.hpp"

#include <expected>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include "open62541pp/types.hpp"
namespace tpc::system::client {

#pragma region Fabric/Constructor

std::expected<std::unique_ptr<FrameReceiver>, std::string> FrameReceiver::create(std::uint16_t receive_count) {
    if (receive_count == 0)
        return std::unexpected{"Received queue capacity must be greater than zero"};

    try {
        return std::unique_ptr<FrameReceiver>{new FrameReceiver(receive_count)};
    } catch (const std::exception& exception) {
        return std::unexpected{exception.what()};
    } catch (...) {
        return std::unexpected{"Failed to create frame receiver: unknown error"};
    }
}

FrameReceiver::FrameReceiver(std::uint16_t capacity) : capacity_{capacity} {
    received_.reserve(capacity);
}

#pragma endregion

#pragma region Public methods

std::expected<void, std::string> FrameReceiver::add_back(opcua::NodeId node, double value) {
    std::lock_guard lock{mutex_};

    if (!received_.contains(node) && received_.size() >= capacity_)
        return std::unexpected{"Received queue capacity was exceeded"};

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
}  // namespace tpc::system::client
