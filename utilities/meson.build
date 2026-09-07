tpc_utilities_sources = [
    'event_handler.cppm',
    'header_function.cppm',
    'chunkview.cppm'
]

tpc_utilities_lib = static_library('tpc_utilities',
                                   tpc_utilities_sources,
)

tpc_utilities_dep = declare_dependency(
    link_with: tpc_utilities_lib,
)
