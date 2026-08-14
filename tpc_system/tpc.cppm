module;
#include <open62541pp/client.hpp>
#include <open62541pp/services/attribute_highlevel.hpp>
export module tpc.system;
import std;
export namespace tpc::system
{
    class TPC
    {
    public:
        [[nodiscard]] static std::expected<std::unique_ptr<TPC>, std::string> create(std::string_view endpoint) { return{};}

        ~TPC() = default;

        TPC(const TPC&)            = delete;
        TPC& operator=(const TPC&) = delete;

        TPC(TPC&&) noexcept            = default;
        TPC& operator=(TPC&&) noexcept = default;

        void process_events() {}

    private:
        TPC() {
            //opcua::services::readValueAsync(Client &connection, const NodeId &id, CompletionToken &&token)
        }

    private:
        opcua::Client client_;
    };
}