add_subdirectory(../third_party third_party_build)

file(GLOB_RECURSE MODULE_SOURCES CONFIGURE_DEPENDS "*.cppm")
file(GLOB_RECURSE REGULAR_SOURCES CONFIGURE_DEPENDS "*.cpp")

add_library(tpc_analytics ${REGULAR_SOURCES})

target_sources(tpc_analytics
        PUBLIC
        FILE_SET CXX_MODULES FILES ${MODULE_SOURCES}
)

target_link_libraries(tpc_analytics
        PUBLIC
        third_party
        tpc_utilities
        tpc_core
)

add_library(TPC::Analytics ALIAS tpc_analytics)