add_library(nova-packer STATIC
    nvmp_format.cpp
    ast_serializer.cpp
    asset_bundler.cpp
    nvmp_writer.cpp
    packer.cpp
)

target_include_directories(nova-packer PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(nova-packer PUBLIC
    nova-core
    nova-ast
    nova-lexer
    nova-parser
    nova-semantic
)