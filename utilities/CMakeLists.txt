file(GLOB_RECURSE MODULE_SOURCES CONFIGURE_DEPENDS "*.cppm")
#file(GLOB_RECURSE REGULAR_SOURCES CONFIGURE_DEPENDS "*.cpp")

add_library(tpc_utilities)

target_sources(tpc_utilities
        PUBLIC
        FILE_SET CXX_MODULES FILES ${MODULE_SOURCES}

        PRIVATE
        ${REGULAR_SOURCES}
)

add_library(TPC::Utilities ALIAS tpc_utilities)