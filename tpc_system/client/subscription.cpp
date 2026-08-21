module tpc.system.client.subscription;
namespace tpc::system::client{

    struct DefaultSubscriptionConfig {
    constexpr static double publishing_interval = 1000.;

    constexpr static uint32_t max_keep_alive_count = 10;
};

std::expected<std::unique_ptr<Subscription>, std::string> create() {
    try {
            SubscriptionParameters parameters{
                .publishingInterval = DefaultSubscriptionConfig::publishing_interval,
                .maxKeepAliveCount = DefaultSubscriptionConfig::max_keep_alive_count};

            return std::unique_ptr<Subscription>{new Subscription(std::move(parameters))};
        } catch (...) {
            return std::unexpected("Failed to create subscription: unknown error");
        }
}
}