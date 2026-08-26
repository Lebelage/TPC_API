file(GLOB_RECURSE MODULE_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cppm"
)

file(GLOB_RECURSE REGULAR_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

file(GLOB_RECURSE HEADER_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

find_package(open62541pp CONFIG REQUIRED)
find_package(stdexec CONFIG REQUIRED)

add_library(tpc_system)

target_sources(tpc_system
        PUBLIC
        FILE_SET CXX_MODULES
        FILES ${MODULE_SOURCES}

        PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
        FILES ${HEADER_SOURCES}

        PRIVATE
        ${REGULAR_SOURCES}
)

target_link_libraries(tpc_system
        PRIVATE
        STDEXEC::stdexec

        PUBLIC
        open62541pp::open62541pp
        tpc_utilities
        tpc_core
)

add_library(TPC::System ALIAS tpc_system)