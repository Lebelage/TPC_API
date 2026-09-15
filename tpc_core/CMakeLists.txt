file(GLOB_RECURSE HEADER_SOURCES CONFIGURE_DEPENDS "*.hpp")

add_library(tpc_core INTERFACE)

target_sources(tpc_core
        INTERFACE
        FILE_SET HEADERS
        BASE_DIRS ${PROJECT_SOURCE_DIR}
        FILES ${HEADER_SOURCES}
)

add_library(TPC::Core ALIAS tpc_core)
