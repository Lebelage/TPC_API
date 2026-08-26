module;
#include <expected>
#include <string>

export module tpc.analytics.svd.basis;
import tpc.utilities.header_function;
import tpc.analytics.models.three_dimension_model;

export namespace tpc::analytics
{
    class BasisCollection
    {
    public:
        static std::expected<BasisCollection, std::string> create(std::size_t arity, std::size_t modes, models::CoordinateType basis_type)
        {
            return BasisCollection(arity, modes, basis_type);
        }

        ~BasisCollection() = default;

    private:
        BasisCollection(std::size_t arity, std::size_t modes, models::CoordinateType basis_type)
            : arity_{arity}, modes_{modes}, basis_type_{basis_type}
        {
            basis_.reserve(arity);
        };

    public:
        BasisCollection(const BasisCollection&)            = delete;
        BasisCollection& operator=(const BasisCollection&) = delete;

        BasisCollection(BasisCollection&&) noexcept            = default;
        BasisCollection& operator=(BasisCollection&&) noexcept = default;

    public:
        bool empty() { return basis_.empty(); }

        std::expected<void, std::string> add_back(tpc::utilities::header_function<double(std::size_t, double, double, double)> function)
        {
            if (basis_.size() >= arity_)
                return std::unexpected("Basis collection is full");

            basis_.emplace_back(std::move(function));
            return {};
        }

    public:
        std::size_t get_arity() const { return arity_; }

        std::size_t get_modes() const { return modes_; }

        models::CoordinateType get_basis_type() const { return basis_type_; }

        std::span<const tpc::utilities::header_function<double(std::size_t, double, double, double)>> get_basis() const { return basis_; }

    private:
        std::size_t            arity_{};
        std::size_t            modes_{};
        models::CoordinateType basis_type_{};

        std::vector<tpc::utilities::header_function<double(std::size_t, double, double, double)>> basis_{};
    };

}  // namespace tpc::analytics