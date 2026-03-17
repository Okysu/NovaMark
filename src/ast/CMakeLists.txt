# nova-ast - AST 节点定义

add_library(nova-ast STATIC
    ast_node.cpp
)

target_include_directories(nova-ast PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(nova-ast PUBLIC
    nova-core
    nlohmann_json::nlohmann_json
)
