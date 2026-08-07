export module tpc.analytics.svd.basis;
import std;
import tpc.utilities.header_function;
import tpc.analytics.models.three_dimension_model;
export namespace tpc::analytics
{
    template <typename... Args>
    concept AllConvertibleToDouble = (std::convertible_to<Args, double> && ...);

    template <typename... Args>
        requires AllConvertibleToDouble<Args...>
    struct Basis
    {
        std::size_t modes{};

        std::vector<utilities::header_function<double(std::size_t, Args...)>> functions;

        models::CoordinateType basis_type{};

        constexpr std::size_t get_arity() { return sizeof...(Args); }
    };

}