project(
    'TPC_Control',
    'cpp',
    version: '1.0',
    default_options: [
        'cpp_std=c++23',
        'warning_level=2',
    ],
)

open62541pp_dep = dependency('open62541pp')
stdexec_dep = dependency('stdexec')

subdir('third_party')
subdir('utilities')
subdir('tpc_core')
subdir('tpc_analytics')
subdir('tpc_system')