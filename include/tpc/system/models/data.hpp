#pragma once

#include <cstddef>
#include <open62541pp/client.hpp>
#include <string>
#include <unordered_map>

#include "tpc/core/definitions/analytics_definitions.hpp"
#include "tpc/analytics/models/three_dimension_models.hpp"

namespace tpc::system::models {

struct NodeIdHash {
    std::size_t operator()(const opcua::NodeId& node_id) const noexcept {
        return static_cast<std::size_t>(node_id.hash());
    }
};

struct NodeIdEqual {
    bool operator()(const opcua::NodeId& lhs, const opcua::NodeId& rhs) const noexcept {
        return lhs == rhs;
    }
};

struct DiscoveryResult {
    std::unordered_map<opcua::NodeId, std::string, NodeIdHash, NodeIdEqual> nodes;
};

struct ReceivedItem {
    std::string name{};
    double value{};
};

struct HallCalibration {
    double k;      // Гс/мВ
    double v0_mv;  // мВ
};

struct NamedHallCalibration {
    std::string_view name;
    HallCalibration calibration;
};

struct HallCalibrationCollection {
    static inline constexpr std::array kCalibrations{
        NamedHallCalibration{"W1R", {.k = 0.569788, .v0_mv = -25.37}},
        NamedHallCalibration{"W1F",  {.k = 0.747579, .v0_mv = 10.46}},
        NamedHallCalibration{"W1Z",  {.k = 0.751044, .v0_mv = -3.22}}
    };

    [[nodiscard]]
    static constexpr const HallCalibration* find(std::string_view name) noexcept {
        for (const auto& entry : kCalibrations) {
            if (entry.name == name) {
                return &entry.calibration;
            }
        }

        return nullptr;
    }
};

struct TpcGeometry {
    static inline constexpr double kRadius = 4;
    static inline constexpr double kLength = 7;
};

struct CalculationData {
    std::array<size_t, tpc::core::definitions::DIMENSION> grid;
    std::vector<analytics::models::Measurement> measurements;

    double radius;
    double length;
};
}  // namespace tpc::system::models
