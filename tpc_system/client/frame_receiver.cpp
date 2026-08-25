module;
#include <expected>
#include <iostream>
#include <mutex>
module tpc.system.client.frame_receiver;

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
    received_queue_.reserve(received_queue_size);
}

#pragma endregion

#pragma region Public methods

std::expected<void, std::string> FrameReceiver::add_back(std::string name, double value) {
    std::lock_guard lock{mutex_};

    if (max_received_queue_size_ == 0) {
        return std::unexpected{"Received queue capacity is zero"};
    }

    ReceivedItem item{
        .name = std::move(name),
        .value = value,
    };

    if (received_queue_.size() < max_received_queue_size_) {
        received_queue_.push_back(std::move(item));
    } else {
        std::move(received_queue_.begin() + 1, received_queue_.end(), received_queue_.begin());

        received_queue_.back() = std::move(item);
    }

    return {};
}

std::optional<std::vector<ReceivedItem>> FrameReceiver::get_frame() const {
    std::lock_guard lock{mutex_};

    if (received_queue_.empty())
        return std::nullopt;

    return received_queue_;
}

#pragma endregion
} // namespace tpc::system::client