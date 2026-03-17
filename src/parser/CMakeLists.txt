# nova-parser - 语法分析器

add_library(nova-parser STATIC
    parser.cpp
)

target_include_directories(nova-parser PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(nova-parser PUBLIC
    nova-core
    nova-lexer
    nova-ast
)
