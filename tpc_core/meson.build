tpc_core_sources = [
    'definitions/analytics_definitions.cppm',
    'definitions/client_definitions.cppm',
    'logger/logger.cppm'
]

tpc_core_lib = static_library('tpc_core',
                              tpc_core_sources,
)

tpc_core_dep = declare_dependency(
    link_with: tpc_core_lib,
)
