file(GLOB_RECURSE MODULE_SOURCES CONFIGURE_DEPENDS "*.cppm")
file(GLOB_RECURSE REGULAR_SOURCES CONFIGURE_DEPENDS "*.cpp")

find_package(open62541pp CONFIG REQUIRED)
find_package(stdexec CONFIG REQUIRED)

add_library(tpc_system)

target_sources(tpc_system
        PUBLIC
        FILE_SET CXX_MODULES FILES ${MODULE_SOURCES}
        PRIVATE
        ${REGULAR_SOURCES}
)

target_link_libraries(tpc_system
        PRIVATE
        open62541pp::open62541pp
        STDEXEC::stdexec

        PUBLIC
        tpc_utilities
        tpc_core
)

add_library(TPC::System ALIAS tpc_system)