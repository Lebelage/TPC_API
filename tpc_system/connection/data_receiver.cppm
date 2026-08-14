module;

#include <exec/static_thread_pool.hpp>
#include <open62541pp/client.hpp>
#include <open62541pp/services/attribute_highlevel.hpp>
#include <stdexec/execution.hpp>
#include <exec/start_detached.hpp>

export module tpc.system.data_receiver;
import std;

export namespace tpc::system
{
    class DataReceiver final
    {
    public:
        using Samples = std::vector<double>;
        using Handler = std::function<void(Samples)>;

        [[nodiscard]] static std::expected<std::unique_ptr<DataReceiver>, std::string> create(std::string endpoint, Handler handler)
        {
            try
            {
                auto receiver = std::unique_ptr<DataReceiver>(new DataReceiver{std::move(endpoint), std::move(handler)});

                receiver->client_.connect(receiver->endpoint_);
                return receiver;
            }
            catch (const std::exception& error)
            {
                return std::unexpected{std::format("OPC UA connection failed: {}", error.what())};
            }
        }

        ~DataReceiver()
        {
            stop();
            client_.disconnect();
        }

        DataReceiver(const DataReceiver&)            = delete;
        DataReceiver& operator=(const DataReceiver&) = delete;

    private:
        DataReceiver(std::string endpoint, Handler handler)
            : endpoint_(std::move(endpoint)), handler_(std::move(handler)), workers_(std::max(1u, std::thread::hardware_concurrency()))
        {
        }

    public:
        void start()
        {
            if (io_thread_.joinable())
                return;

            io_thread_ = std::jthread([this](std::stop_token stop_token) { run(stop_token); });
        }

        void stop()
        {
            if (io_thread_.joinable())
            {
                io_thread_.request_stop();
                io_thread_.join();
            }
        }

    private:
        void run(std::stop_token stop_token)
        {
            const opcua::NodeId w1r{2, "W1R"};

            auto next_read = std::chrono::steady_clock::now();

            while (!stop_token.stop_requested())
            {
                // Вызывать только в этом I/O-потоке.
                client_.runIterate(10);

                const auto now = std::chrono::steady_clock::now();

                if (!request_in_flight_ && now >= next_read)
                {
                    request_in_flight_ = true;
                    next_read          = now + std::chrono::milliseconds{100};

                    opcua::services::readValueAsync(client_,
                                                    w1r,
                                                    [this](opcua::Result<opcua::Variant>& result)
                                                    {
                                                        request_in_flight_ = false;

                                                        if (!result)
                                                        {
                                                            std::cerr << "W1R read failed: " << result.code().name() << '\n';
                                                            return;
                                                        }

                                                        try
                                                        {
                                                            const auto received = result.value().array<double>();

                                                            Samples    samples{received.begin(), received.end()};

                                                            auto task =
                                                                stdexec::schedule(workers_.get_scheduler())
                                                                | stdexec::then([handler = handler_, samples = std::move(samples)]() mutable
                                                                                { handler(std::move(samples)); });

                                                            exec::start_detached(std::move(task));
                                                        }
                                                        catch (const std::exception& error)
                                                        {
                                                            std::cerr << "Invalid W1R payload: " << error.what() << '\n';
                                                        }
                                                    });
                }

                std::this_thread::sleep_for(std::chrono::milliseconds{1});
            }
        }

    private:
        std::string endpoint_;
        Handler     handler_;

        opcua::Client            client_;
        exec::static_thread_pool workers_;
        std::jthread             io_thread_;

        // Используется только I/O-потоком.
        bool request_in_flight_{false};
    };
}  // namespace tpc::system