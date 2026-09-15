file(GLOB_RECURSE REGULAR_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

file(GLOB_RECURSE HEADER_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
)

find_package(open62541pp CONFIG REQUIRED)
find_package(stdexec CONFIG REQUIRED)

add_library(tpc_system ${REGULAR_SOURCES})

target_sources(tpc_system
        PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${PROJECT_SOURCE_DIR}
        FILES ${HEADER_SOURCES}
)

target_link_libraries(tpc_system
        PUBLIC
        STDEXEC::stdexec

        PUBLIC
        open62541pp::open62541pp
        tpc_utilities
        tpc_core
        tpc_analytics
)

add_library(TPC::System ALIAS tpc_system)
