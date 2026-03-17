# nova-lexer - 词法分析器

add_library(nova-lexer STATIC
    token.cpp
    lexer.cpp
)

target_include_directories(nova-lexer PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(nova-lexer PUBLIC
    nova-core
)
