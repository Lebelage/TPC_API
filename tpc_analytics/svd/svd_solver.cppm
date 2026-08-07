export module tpc.analytics.svd_solver;
import std;

import tpc.analytics.models.three_dimension_model;
import tpc.third_party.eigen;
import tpc.utilities.header_function;
import tpc.analytics.concepts.measurement_concept;
export namespace tpc::analytics
{
    using namespace tpc::third_party;

    class SVDSolver
    {
    public:
        using UniversalBasisFunc = std::function<double(std::size_t, std::span<const double>)>;

    public:
        template <concepts::MeasurementConcept Measurement, typename... Args>
        static std::expected<eigen::VectorXd, std::string> SolveSvd(
            std::span<const Measurement>                                                                    measurements,
            std::type_identity_t<std::span<const utilities::header_function<double(std::size_t, Args...)>>> basis_functions,
            std::size_t                                                                                     modes,
            double                                                                                          threshold)
        {
            if (measurements.empty())
                return std::unexpected("Measurement collection is empty");

            if (basis_functions.empty())
                return std::unexpected("Basis functions collection is empty");

            const std::size_t field_dim = measurements[0].GetPointComponents().size();

            if (basis_functions.size() != field_dim)
                return std::unexpected("Mismatch: Number of basis functions must equal field dimensions");

            const std::size_t rows = measurements.size() * field_dim;
            const std::size_t cols = modes;

            eigen::MatrixXd A(rows, cols);
            eigen::VectorXd d(rows);

            for (std::size_t i = 0; i < measurements.size(); ++i)
            {
                std::span<const double> coords = measurements[i].GetPointComponents();
                std::span<const double> values = measurements[i].GetFieldComponents();

                for (std::size_t j = 0; j < field_dim; ++j)
                {
                    const std::size_t row_idx = field_dim * i + j;
                    d(row_idx)                = values[j];

                    for (std::size_t k = 0; k < modes; ++k)
                    {
                        A(row_idx, k) = basis_functions[j].invoke_from_span(k, coords);
                    }
                }
            }

            eigen::JacobiSVD<eigen::MatrixXd> svd(A, eigen::ComputeThinU | eigen::ComputeThinV);
            svd.setThreshold(threshold);

            return svd.solve(d);
        }
    };
}  // namespace tpc::analytics
