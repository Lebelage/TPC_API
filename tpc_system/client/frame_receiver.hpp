#pragma once
#include <expected>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace tpc::system::client {

struct ReceivedItem {
    std::string name{};
    double value{};
};

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
    std::expected<void, std::string> add_back(std::string name, double value);

    std::vector<ReceivedItem> get_frame() const;

private:
    explicit FrameReceiver(std::uint16_t received_queue_size);

private:
    std::unordered_map<std::string, double> ids_;
    std::vector<ReceivedItem> received_queue_;

    std::uint16_t max_received_queue_size_{36};

    mutable std::mutex mutex_;
};

} // namespace tpc::system::client