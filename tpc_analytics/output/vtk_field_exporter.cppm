export module tpc.analytics.output.vtk_field_exporter;
import std;
export namespace tpc::analytics::output
{
    class VtkFieldExporter
    {
    public:
        static std::expected<void, std::string> export_3d_field_to_vtk(std::span<const double>   field_data,
                                                                       std::span<const double>   coordinates_data,
                                                                       const std::string_view& file_path)
        {
            if (field_data.empty() || coordinates_data.empty())
                return std::unexpected("Field data or coordinates data is empty");

            if (file_path.empty())
                return std::unexpected("File path is empty");

            if (field_data.size() != coordinates_data.size())
                return std::unexpected("Field data and coordinates data have different sizes");

            const std::size_t num_points = coordinates_data.size() / 3;

            std::filesystem::path path = file_path.empty() ? "Default.vtk" : std::filesystem::path{file_path};

            if (path.has_parent_path())
            {
                std::error_code ec;
                std::filesystem::create_directories(path.parent_path(), ec);
                if (ec)
                    return std::unexpected(std::format("Failed to create directory: {}", ec.message()));
            }

            std::ofstream output(path);
            if (!output.is_open())
                return std::unexpected(std::format("Failed to open file for writing: {}", path.string()));

            //output << std::scientific << std::setprecision(6);

            output << "# vtk DataFile Version 3.0\n";
            output << "TPC Analytics Field Data\n";
            output << "ASCII\n";
            output << "DATASET STRUCTURED_GRID\n";
            output << "DIMENSIONS " << 128 << " " << 50 << " " << 128 << "\n";

            output << "POINTS " << num_points << " double\n";
            for (std::size_t i = 0; i < coordinates_data.size(); i += 3)
            {
                output << coordinates_data[i] << " " << coordinates_data[i + 1] << " " << coordinates_data[i + 2] << "\n";
            }

            output << "\nPOINT_DATA " << num_points << "\n";

            if (field_data.size() == num_points)
            {
                output << "SCALARS FieldMagnitude double 1\n";
                output << "LOOKUP_TABLE default\n";
                for (double val : field_data)
                {
                    output << val << "\n";
                }
            }
            else if (field_data.size() == num_points * 3)
            {
                output << "VECTORS VField double\n";
                for (std::size_t i = 0; i < field_data.size(); i += 3)
                {
                    output << field_data[i] << " " << field_data[i + 1] << " " << field_data[i + 2] << "\n";
                }
            }
            else
            {
                return std::unexpected(std::format(
                    "Data size mismatch: expected {} (scalar) or {} (vector), but got {}", num_points, num_points * 3, field_data.size()));
            }

            if (!output)
                return std::unexpected("IO error occurred while writing VTK file (disk full?)");

            output.flush();
            if (!output)
                return std::unexpected("IO error occurred while writing VTK file (disk full or write interrupted)");

            output.close();
            return {};
        }
    };
}  // namespace tpc::analytics::output