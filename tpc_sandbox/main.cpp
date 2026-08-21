
import tpc.analytics.analytics_manager;
import tpc.analytics.svd.basis;
import tpc.analytics.models.three_dimension_model;
import tpc.core.definitions.analytics_definitions;
import tpc.system.connection;
import tpc.system.client;

import std;

static double BasisR(int k, double r, double phi, double z)
{
    switch (k)
    {
        case 0:
            return 0.0;
        case 1:
            return -0.5 * r;
        case 2:
            return -r * z;
        case 3:
            return -1.5 * r * z * z + 0.375 * r * r * r;
        case 4:
            return std::cos(phi);
        case 5:
            return std::sin(phi);
        case 6:
            return z * std::cos(phi);
        case 7:
            return z * std::sin(phi);
        case 8:
            return r * std::cos(2.0 * phi);
        case 9:
            return r * std::sin(2.0 * phi);
        default:
            return 0.0;
    }
}

static double BasisPhi(int k, double r, double phi, double z)
{
    switch (k)
    {
        case 0:
            return 0.0;
        case 1:
            return 0.0;
        case 2:
            return 0.0;
        case 3:
            return 0.0;
        case 4:
            return -std::sin(phi);
        case 5:
            return std::cos(phi);
        case 6:
            return -z * std::sin(phi);
        case 7:
            return z * std::cos(phi);
        case 8:
            return -r * std::sin(2.0 * phi);
        case 9:
            return r * std::cos(2.0 * phi);
        default:
            return 0.0;
    }
}

static double BasisZ(int k, double r, double phi, double z)
{
    switch (k)
    {
        case 0:
            return 1.0;
        case 1:
            return z;
        case 2:
            return z * z - 0.5 * r * r;
        case 3:
            return z * z * z - 1.5 * r * r * z;
        case 4:
            return 0.0;
        case 5:
            return 0.0;
        case 6:
            return r * std::cos(phi);
        case 7:
            return r * std::sin(phi);
        case 8:
            return 0.0;
        case 9:
            return 0.0;
        default:
            return 0.0;
    }
}

int main()
{

    auto a = tpc::system::client::Client::create("opc.tcp://127.0.0.1:1234");

    a.value()->connect_async();
    std::this_thread::sleep_for(std::chrono::seconds(10));
    a.value()->stop();
    // auto connection_result = tpc::system::Connection::create("opc.tcp://127.0.0.1:1234");
    // if (!connection_result)
    // {
    //     return 1;
    // }
    // auto connection = std::move(connection_result.value());
    //
    // std::cout <<  std::this_thread::get_id() << "\n";
    //
    // connection->subscribe_handlers();
    // connection->connect_async();
    // connection->run();
    // // //connection->browse_async();

    // // std::this_thread::sleep_for(std::chrono::seconds(15));
    return 0;
}
