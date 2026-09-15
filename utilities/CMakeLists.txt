file(GLOB_RECURSE HEADER_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
)

add_library(tpc_utilities INTERFACE)

target_sources(tpc_utilities
        INTERFACE
        FILE_SET HEADERS
        BASE_DIRS ${PROJECT_SOURCE_DIR}
        FILES ${HEADER_SOURCES}
)

add_library(TPC::Utilities ALIAS tpc_utilities)
