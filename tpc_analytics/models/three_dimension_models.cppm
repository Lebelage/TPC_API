module;
#include <expected>
#include <span>
#include <vector>
#include <cmath>
#include <ranges>
#include <algorithm>

export module tpc.analytics.models.three_dimension_model;

import tpc.utilities.chunkview;

export namespace tpc::analytics::models
{
    /// Represents the coordinate system type.
    enum class CoordinateType
    {
        Cartesian,
        Cylindric
    };

    enum class SliceDirection{
        X,
        Y,
        Z
    };


    struct PointComponents
    {
        std::vector<double> components;
        CoordinateType      coordinate_type;

        [[nodiscard]] std::span<const double> get_point_components() const noexcept { return components; }
        [[nodiscard]] std::span<double>       get_point_components() noexcept { return components; }
    };

    struct FieldComponents
    {
        std::vector<double> components;
        CoordinateType      coordinate_type;

        [[nodiscard]] std::span<const double> get_field_components() const noexcept { return components; }
        [[nodiscard]] std::span<double>       get_field_components() noexcept { return components; }
    };

    struct Measurement
    {
        PointComponents point_components{};
        FieldComponents field_components{};

        std::span<const double> get_point_components() const { return point_components.get_point_components(); }
        std::span<const double> get_field_components() const { return field_components.get_field_components(); }
    };

    
    class FieldCollection
    {
    public:
        FieldCollection(std::size_t dimension, CoordinateType coordinates_type, CoordinateType field_type)
            : dimension_(dimension), coordinates_type_(coordinates_type), field_type_(field_type)
        {
        }

        ~FieldCollection() = default;

    public:
        void Reserve(std::size_t capacity)
        {
            coordinates_.reserve(capacity);
            field_.reserve(capacity);
        }

        std::expected<void, std::string> insert_coordinates_range(std::span<const double> coordinates_data)
        {
            if (coordinates_data.empty())
                return std::unexpected("Coordinates data is empty");

            if ((coordinates_data.size() % dimension_) != 0)
                return std::unexpected("Buffer size is not divisible by its dimension");

            coordinates_.insert(coordinates_.end(), coordinates_data.begin(), coordinates_data.end());

            return {};
        }

        std::expected<void, std::string> insert_field_range(std::span<const double> field_data)
        {
            if (field_data.empty())
                return std::unexpected("Field data is empty");

            if ((field_data.size() % dimension_) != 0)
                return std::unexpected("Buffer size is not divisible by its dimension");

            field_.insert(field_.end(), field_data.begin(), field_data.end());

            return {};
        }

        std::expected<void, std::string> transform_coordinates(CoordinateType target_type)
        {
            if (coordinates_.empty())
                return std::unexpected("Coordinates collection is empty");

            if (target_type == coordinates_type_)
                return std::unexpected("Can`t transform coordinates");

            auto chunks = tpc::utilities::ChunkView{std::span<double>(coordinates_), dimension_};

            if (target_type == CoordinateType::Cylindric)
            {
                std::ranges::for_each(chunks,
                                      [](std::span<double> point)
                                      {
                                          const double x = point[0];
                                          const double y = point[1];

                                          point[0] = std::hypot(x, y);
                                          point[1] = std::atan2(y, x);
                                      });
            }
            else if (target_type == CoordinateType::Cartesian)
            {
                std::ranges::for_each(chunks,
                                      [](std::span<double> point)
                                      {
                                          const double r   = point[0];
                                          const double phi = point[1];

                                          point[0] = r * std::cos(phi);
                                          point[1] = r * std::sin(phi);
                                      });
            }

            coordinates_type_ = target_type;
            return {};
        }

        std::expected<void, std::string> transform_field(CoordinateType target_type)
        {
            if (field_.empty())
                return std::unexpected("Field collection is empty");

            if (field_type_ == target_type)
                return {};

            if (dimension_ != 3)
                return std::unexpected("Only 3D fields are supported");

            if (field_type_ != CoordinateType::Cylindric || target_type != CoordinateType::Cartesian)
            {
                return std::unexpected("Unsupported field transformation");
            }

            if (coordinates_type_ != CoordinateType::Cylindric)
                return std::unexpected("Coordinates must be cylindrical");

            if (coordinates_.size() != field_.size())
                return std::unexpected("Coordinates and field sizes differ");

            for (std::size_t offset = 0; offset < field_.size(); offset += dimension_)
            {
                const double phi  = coordinates_[offset + 1];
                const double br   = field_[offset];
                const double bphi = field_[offset + 1];

                field_[offset]     = br * std::cos(phi) - bphi * std::sin(phi);
                field_[offset + 1] = br * std::sin(phi) + bphi * std::cos(phi);
            }

            field_type_ = CoordinateType::Cartesian;
            return {};
        }

        // std::expected<std::span<const double>, std::string> get_coordinate_slice(SliceDirection slice_direction,  ){

        // }

    public:
        std::span<const double> get_field() { return field_; }
        std::span<const double> get_coordinates() { return coordinates_; }

    private:
        std::size_t dimension_{};

        CoordinateType coordinates_type_{};
        CoordinateType field_type_{};

        std::vector<double> coordinates_{};
        std::vector<double> field_{};
    };
}  // namespace tpc::analytics::models
