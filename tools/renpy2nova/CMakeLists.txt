# renpy2nova - Ren'Py 到 NovaMark 独立转换工具

add_library(renpy2nova-lib STATIC
    src/app/application.cpp
    src/lexer/renpy_lexer.cpp
    src/analyzer/renpy_analyzer.cpp
    src/emitter/novamark_emitter.cpp
    src/converter/converter.cpp
    src/converter/project_converter.cpp
    src/report/conversion_report.cpp
    src/resource/resource_manifest.cpp
)

target_include_directories(renpy2nova-lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(renpy2nova-lib PUBLIC
    nova-core
)

add_executable(renpy2nova
    src/main.cpp
)

target_link_libraries(renpy2nova PRIVATE
    renpy2nova-lib
)

if(BUILD_TESTS)
    add_executable(renpy2nova-test
        tests/renpy2nova_test.cpp
    )

    target_link_libraries(renpy2nova-test PRIVATE
        renpy2nova-lib
        GTest::gtest
        GTest::gtest_main
    )

    target_compile_definitions(renpy2nova-test PRIVATE
        RENRY2NOVA_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests"
    )

    add_custom_command(TARGET renpy2nova-test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:renpy2nova-test>
            $<TARGET_FILE_DIR:renpy2nova-test>
        COMMAND_EXPAND_LISTS
    )

    include(GoogleTest)
    gtest_discover_tests(renpy2nova-test DISCOVERY_MODE PRE_TEST)
endif()
