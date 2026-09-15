find_package(Eigen3 CONFIG REQUIRED)

file(GLOB_RECURSE HEADER_SOURCES CONFIGURE_DEPENDS "*.hpp")

add_library(third_party INTERFACE)

target_sources(third_party
        INTERFACE
        FILE_SET HEADERS
        BASE_DIRS ${PROJECT_SOURCE_DIR}
        FILES ${HEADER_SOURCES}
)

target_link_libraries(third_party INTERFACE Eigen3::Eigen)
