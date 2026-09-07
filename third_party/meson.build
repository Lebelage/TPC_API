eigen3_dep = dependency('Eigen3', modules: ['Eigen3::Eigen'])

third_party_sources = [
    'eigen_module.cppm'
]

third_party_lib = static_library('third_party',
                                 third_party_sources,
                                 dependencies: [eigen3_dep]
)

third_party_dep = declare_dependency(
    link_with: third_party_lib,
    dependencies: [eigen3_dep]
)
