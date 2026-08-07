file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS "*.cpp")

add_executable(sandbox)

target_sources(sandbox
        PRIVATE
        ${APP_SOURCES}
)

target_link_libraries(sandbox
        PRIVATE
        TPC::System
        TPC::Analytics
)