add_library(nova-semantic STATIC
    semantic_analyzer.cpp
    diagnostic.cpp
    symbol_table.cpp
)

target_include_directories(nova-semantic PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(nova-semantic PUBLIC
    nova-core
    nova-ast
)
