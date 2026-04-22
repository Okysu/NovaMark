# NovaMark 测试

add_executable(nova-test
    test_main.cpp
    ast_source_emitter_test.cpp
    lexer_test.cpp
    parser_test.cpp
    semantic_test.cpp
    vm_test.cpp
)

target_include_directories(nova-test PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(nova-test PRIVATE
    nova-core
    nova-lexer
    nova-parser
    nova-ast
    nova-semantic
    nova-vm
    nova-packer
    GTest::gtest
    GTest::gtest_main
)

# 注册测试
include(GoogleTest)
gtest_discover_tests(nova-test)
