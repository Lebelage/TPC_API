export module tpc.system.client.event_handler;
import std;
export namespace tpc::system::client {

    template<class... Args>
    class EventHandler {
    public:
        using Handler = std::function<void(Args...)>;
        using Id = std::uint64_t;

        [[nodiscard]] Id subscribe(Handler handler)
        {
            const Id id = next_id_++;

            handlers_.emplace(id, std::move(handler));

            return id;
        }

        void unsubscribe(Id id)
        {
            handlers_.erase(id);
        }

        void emit(Args... args) const
        {
            const auto handlers = handlers_;

            for (const auto& [id, handler] : handlers)
            {
                handler(args...);
            }
        }

        void operator()(Args... args) const
        {
            emit(std::forward<Args>(args)...);
        }

        [[nodiscard]] bool empty() const noexcept
        {
            return handlers_.empty();
        }

    private:
        Id next_id_ = 1;
        std::unordered_map<Id, Handler> handlers_;
    };
}