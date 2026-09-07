tpc_system_sources = [
    'tpc.cpp',
    'tpc.cppm',

    'models/data.cppm',

    'client/client.cppm',
    'client/client.cpp',

    'client/subscription.cppm',
    'client/subscription.cpp',

    'client/frame_receiver.cpp',
    'client/frame_receiver.cppm',

    'client/helpers/opcua_browse_adapter.cppm',
]

tpc_system_lib = static_library(
    'tpc_system',
    tpc_system_sources,
    dependencies: [
        stdexec_dep,
        open62541pp_dep,

        tpc_utilities_dep,
        tpc_core_dep,
        tpc_analytics_dep,
    ],
)

tpc_system_dep = declare_dependency(
    link_with: tpc_system_lib,
    dependencies: [
        stdexec_dep,
        open62541pp_dep,

        tpc_utilities_dep,
        tpc_core_dep,
        tpc_analytics_dep,
    ],
)