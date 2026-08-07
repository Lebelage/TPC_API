import tpc.analytics.analytics_manager;
import tpc.analytics.svd.basis;
import tpc.analytics.models.three_dimension_model;
import tpc.core.definitions.analytics_definitions;

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
    tpc::analytics::Basis<double, double, double> basis{};
    basis.functions.push_back(BasisR);
    basis.functions.push_back(BasisPhi);
    basis.functions.push_back(BasisZ);

    basis.modes      = 10;
    basis.basis_type = tpc::analytics::models::CoordinateType::Cylindric;

    // tpc::analytics::AnalyticsManager manager(std::move(basis));
    auto manager = tpc::analytics::AnalyticsManager<double, double, double>::create(std::move(basis)).value();

    auto a = manager.GenerateTPCSensorMeasurements();
    manager.calculate_svd_coefficients(a);
    std::array<const std::size_t, tpc::analytics::__DIMENSION> components = {512, 50, 512};

    tpc::analytics::AnalyticsManager<double, double, double>::create_grid(512, 50, 512);
    auto b = tpc::analytics::AnalyticsManager<double, double, double>::create_default_basis(BasisR, BasisPhi, BasisZ, 10);
    tpc::analytics::AnalyticsManager<double, double, double>::create(std::move(b));

    return 0;
}
