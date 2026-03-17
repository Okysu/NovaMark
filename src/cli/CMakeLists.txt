# nova-cli - 命令行工具

add_executable(nova-cli
    main.cpp
    cli.cpp
)

target_include_directories(nova-cli PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(nova-cli PRIVATE
    nova-core
    nova-lexer
    nova-parser
    nova-ast
    nova-semantic
    nova-packer
    nova-vm
)
