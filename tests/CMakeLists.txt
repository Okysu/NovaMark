# NovaMark 测试

add_executable(nova-test
    test_main.cpp
    ast_source_emitter_test.cpp
    c_api_test.cpp
    lexer_test.cpp
    nova_state_v2_test.cpp
    parser_test.cpp
    registry_test.cpp
    semantic_test.cpp
    state_version_test.cpp
    vm_test.cpp
)

target_include_directories(nova-test PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

if(MSVC)
    target_compile_definitions(nova-test PRIVATE NOVA_EXPORTS)
endif()

target_link_libraries(nova-test PRIVATE
    nova-core
    nova-lexer
    nova-parser
    nova-ast
    nova-semantic
    nova-vm
    nova-packer
    nova-renderer
    GTest::gtest
    GTest::gtest_main
)

# 注册测试
include(GoogleTest)
gtest_discover_tests(nova-test)
