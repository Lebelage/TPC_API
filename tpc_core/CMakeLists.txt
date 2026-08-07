file(GLOB_RECURSE MODULE_SOURCES CONFIGURE_DEPENDS "*.cppm")
file(GLOB_RECURSE REGULAR_SOURCES CONFIGURE_DEPENDS "*.cpp")

add_library(tpc_core)

target_sources(tpc_core
        PUBLIC
        FILE_SET CXX_MODULES FILES ${MODULE_SOURCES}

        PRIVATE
        ${REGULAR_SOURCES}
)

add_library(TPC::Core ALIAS tpc_core)