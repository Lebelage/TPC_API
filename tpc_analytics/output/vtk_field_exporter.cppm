module;
#include <expected>
#include <filesystem>
#include <format>
#include <span>
#include <string>
#include <system_error>
#include <fstream>
export module tpc.analytics.output.vtk_field_exporter;

export namespace tpc::analytics::output
{
    class VtkFieldExporter
    {
    public:
        static std::expected<void, std::string>
        export_3d_field_to_vtk(std::span<const double> field_data,
                               std::span<const double> coordinates_data,
                               std::string_view file_path)
        {
            constexpr std::size_t dimension = 3;

            if (coordinates_data.empty())
                return std::unexpected("Coordinates data is empty");

            if (coordinates_data.size() % dimension != 0)
                return std::unexpected("Coordinates data size must be divisible by 3");

            const std::size_t num_points =
                coordinates_data.size() / dimension;

            if (field_data.size() != num_points * dimension)
            {
                return std::unexpected(std::format(
                    "Field data must contain {} values, but contains {}",
                    num_points * dimension,
                    field_data.size()));
            }

            if (file_path.empty())
                return std::unexpected("File path is empty");

            const std::filesystem::path path{file_path};

            if (path.has_parent_path())
            {
                std::error_code ec;
                std::filesystem::create_directories(path.parent_path(), ec);

                if (ec)
                    return std::unexpected(
                        std::format("Failed to create directory: {}", ec.message()));
            }

            std::ofstream output(path);
            if (!output.is_open())
                return std::unexpected(
                    std::format("Failed to open file for writing: {}", path.string()));

            output << std::scientific << std::setprecision(10);

            output << "# vtk DataFile Version 3.0\n";
            output << "TPC Analytics Field Data\n";
            output << "ASCII\n";
            output << "DATASET POLYDATA\n";

            output << "POINTS " << num_points << " double\n";

            // r, phi, z -> x, y, z
            for (std::size_t offset = 0;
                 offset < coordinates_data.size();
                 offset += dimension)
            {
                const double r   = coordinates_data[offset];
                const double phi = coordinates_data[offset + 1];
                const double z   = coordinates_data[offset + 2];

                const double x = r * std::cos(phi);
                const double y = r * std::sin(phi);

                output << x << ' ' << y << ' ' << z << '\n';
            }

            // Описываем каждую точку как отдельную VTK-вершину.
            output << "\nVERTICES " << num_points
                   << ' ' << num_points * 2 << '\n';

            for (std::size_t point_id = 0; point_id < num_points; ++point_id)
                output << "1 " << point_id << '\n';

            output << "\nPOINT_DATA " << num_points << '\n';
            output << "VECTORS MagneticField double\n";

            // Br, Bphi, Bz -> Bx, By, Bz
            for (std::size_t offset = 0;
                 offset < field_data.size();
                 offset += dimension)
            {
                const double phi  = coordinates_data[offset + 1];
                const double br   = field_data[offset];
                const double bphi = field_data[offset + 1];
                const double bz   = field_data[offset + 2];

                const double bx =
                    br * std::cos(phi) - bphi * std::sin(phi);

                const double by =
                    br * std::sin(phi) + bphi * std::cos(phi);

                output << bx << ' ' << by << ' ' << bz << '\n';
            }

            output.flush();

            if (!output)
                return std::unexpected("I/O error while writing VTK file");

            return {};
        }
    };
}