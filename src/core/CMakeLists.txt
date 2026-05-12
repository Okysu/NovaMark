# nova-core - 核心工具库

add_library(nova-core STATIC
    source_location.cpp
    string_utils.cpp
    game_metadata.cpp
)

target_include_directories(nova-core PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(nova-core PUBLIC
    nlohmann_json::nlohmann_json
)
