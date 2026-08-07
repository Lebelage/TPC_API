find_package(Eigen3 CONFIG REQUIRED)

file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS "*.cppm")

add_library(third_party STATIC)

target_sources(third_party
        PUBLIC
        FILE_SET CXX_MODULES FILES ${APP_SOURCES}
)

target_link_libraries(third_party PUBLIC Eigen3::Eigen)