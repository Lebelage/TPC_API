#include "tpc.hpp"

import tpc.analytics.analytics_manager;
import tpc.analytics.svd.basis;
import tpc.analytics.models.three_dimension_model;
import tpc.core.definitions.analytics_definitions;
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
    auto b = tpc::system::TPC::create("opc.tcp://127.0.0.1:1234");
    b.value()->start_async();
    std::this_thread::sleep_for(std::chrono::seconds(10));
    b.value()->stop_async();
    std::this_thread::sleep_for(std::chrono::seconds(30));
    b.value()->start_async();
    //std::this_thread::sleep_for(std::chrono::seconds(10));
    // a.value()->connect_async();
    std::this_thread::sleep_for(std::chrono::seconds(30));
    // a.value()->stop();
    return 0;
}
