export module tpc.analytics.models.three_dimension_model;
import std;

import tpc.utilities.chunkview;

export namespace tpc::analytics::models {
    enum class CoordinateType {
        Cartesian,
        Cylindric
    };

    struct PointComponents {
        std::array<double,3> components;
        CoordinateType coordinate_type;

        [[nodiscard]] std::span<const double> GetPointComponents() const noexcept { return components; }
        [[nodiscard]] std::span<double> GetPointComponents() noexcept { return components; }
    };

    struct FieldComponents {
        std::array<double,3> components;
        CoordinateType coordinate_type;

        [[nodiscard]] std::span<const double> GetFieldComponents() const noexcept { return components; }
        [[nodiscard]] std::span<double> GetFieldComponents() noexcept { return components; }
    };

    struct Measurement {
        PointComponents point_components{};
        FieldComponents field_components{};

        std::span<const double> GetPointComponents() const { return point_components.GetPointComponents(); }
        std::span<const double> GetFieldComponents() const { return field_components.GetFieldComponents(); }
    };

    class FieldCollection {
    public:
        FieldCollection(std::size_t dimension, CoordinateType coordinates_type, CoordinateType field_type)
            : dimension_(dimension),
              coordinates_type_(coordinates_type),
              field_type_(field_type) {
        }

        ~FieldCollection() = default;

    public:
        void Reserve(std::size_t capacity) {
            coordinates_.reserve(capacity);
            field_.reserve(capacity);
        }

        std::expected<void, std::string> InsertCoordinatesRange(std::span<const double> coordinates_data) {
            if (coordinates_data.empty())
                return std::unexpected("Coordinates data is empty");

            if ((coordinates_data.size() % dimension_) != 0)
                return std::unexpected("Buffer size is not divisible by its dimension");

            coordinates_.insert(coordinates_.end(), coordinates_data.begin(), coordinates_data.end());

            return {};
        }

        std::expected<void, std::string> InsertFieldRange(std::span<const double> field_data) {
            if (field_data.empty())
                return std::unexpected("Field data is empty");

            if ((field_data.size() % dimension_) != 0)
                return std::unexpected("Buffer size is not divisible by its dimension");

            field_.insert(field_.end(), field_data.begin(), field_data.end());

            return {};
        }

        std::expected<void, std::string> TransformCoordinates(CoordinateType target_type) {
            if (coordinates_.empty())
                return std::unexpected("Coordinates collection is empty");

            if (target_type == coordinates_type_)
                return std::unexpected("Can`t transform coordinates");

            auto chunks = tpc::utilities::ChunkView{std::span<double>(coordinates_), dimension_};

            if (target_type == CoordinateType::Cylindric) {
                std::ranges::for_each(chunks, [](std::span<double> point) {
                    const double x = point[0];
                    const double y = point[1];

                    point[0] = std::hypot(x, y);
                    point[1] = std::atan2(y, x);
                });
            } else if (target_type == CoordinateType::Cartesian) {
                std::ranges::for_each(chunks, [](std::span<double> point) {
                    const double r = point[0];
                    const double phi = point[1];

                    point[0] = r * std::cos(phi);
                    point[1] = r * std::sin(phi);
                });
            }

            coordinates_type_ = target_type;
            return {};
        }

        ///TODO
        std::expected<void, std::string> TransformField(CoordinateType target_type) {
        }

    private:
        std::size_t dimension_{};

        CoordinateType coordinates_type_{};
        CoordinateType field_type_{};

        std::vector<double> coordinates_{};
        std::vector<double> field_{};
    };
}
