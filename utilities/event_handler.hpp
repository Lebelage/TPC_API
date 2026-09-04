#pragma once

#include <cstdint>
#include <functional>
#include <unordered_map>
#include <utility>

namespace tpc::utilities {

template <class... Args> class event_handler {
public:
  using Handler = std::function<void(Args...)>;
  using Id = std::uint64_t;

  [[nodiscard]]
  Id subscribe(Handler handler) {
    const Id id = next_id_++;

    handlers_.emplace(id, std::move(handler));

    return id;
  }

  void unsubscribe(Id id) { handlers_.erase(id); }

  void release() noexcept { handlers_.clear(); }

  void dispose() noexcept {
    handlers_.clear();
    handlers_.rehash(0);
    next_id_ = 1;
  }

  void invoke(Args... args) const {
    const auto handlers = handlers_;

    for (const auto &[id, handler] : handlers) {
      handler(args...);
    }
  }

  void operator()(Args... args) const { invoke(std::forward<Args>(args)...); }

  [[nodiscard]]
  bool empty() const noexcept {
    return handlers_.empty();
  }

  [[nodiscard]]
  std::size_t size() const noexcept {
    return handlers_.size();
  }

private:
  Id next_id_{1};
  std::unordered_map<Id, Handler> handlers_;
};

} // namespace tpc::utilities