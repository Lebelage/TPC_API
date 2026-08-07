export module tpc.analytics.concepts.measurement_concept;
import std;
export namespace tpc::analytics::concepts
{
    template <typename T>
    concept MeasurementConcept = requires(const T& m) {
        { m.GetPointComponents() } -> std::convertible_to<std::span<const double>>;
        { m.GetFieldComponents() } -> std::convertible_to<std::span<const double>>;
    };
}