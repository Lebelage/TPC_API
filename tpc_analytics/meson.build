tpc_analytics_sources = [
    'analytics_manager/analytics_manager.cppm',
    'concepts/measurement_concept.cppm',
    'models/basis_models.cppm',
    'models/three_dimension_models.cppm',
    'output/vtk_field_exporter.cppm',
    'svd/basis.cppm',
    'svd/svd_solver.cppm',
]

tpc_analytics_lib = static_library(
    'tpc_analytics',
    tpc_analytics_sources,
    link_with: [
        third_party_lib,
        tpc_utilities_lib,
        tpc_core_lib,
    ],
    dependencies: [
        eigen3_dep,
    ],
)

tpc_analytics_dep = declare_dependency(
    link_with: tpc_analytics_lib,
    dependencies: [
        third_party_dep,
        tpc_utilities_dep,
        tpc_core_dep,
    ],
)