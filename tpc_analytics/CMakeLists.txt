add_subdirectory(../third_party third_party_build)

file(GLOB_RECURSE HEADER_SOURCES CONFIGURE_DEPENDS "*.hpp")

add_library(tpc_analytics INTERFACE)

target_sources(tpc_analytics
        INTERFACE
        FILE_SET HEADERS
        BASE_DIRS ${PROJECT_SOURCE_DIR}
        FILES ${HEADER_SOURCES}
)

target_link_libraries(tpc_analytics
        INTERFACE
        third_party
        tpc_utilities
        tpc_core
)

add_library(TPC::Analytics ALIAS tpc_analytics)
