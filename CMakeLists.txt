cmake_minimum_required(VERSION 3.16)
project(NovaMark VERSION 0.1.0 LANGUAGES CXX)

# C++17 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 导出 compile_commands.json 用于 LSP
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# vcpkg 集成（如果使用 vcpkg）
if(DEFINED CMAKE_TOOLCHAIN_FILE)
    message(STATUS "Using vcpkg toolchain: ${CMAKE_TOOLCHAIN_FILE}")
endif()

# 查找依赖
find_package(nlohmann_json CONFIG REQUIRED)

# 编译选项
if(MSVC)
    add_compile_options(/W4 /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# 子目录
add_subdirectory(src)

# 测试（仅 Release 或 Debug 显式启用时构建）
option(BUILD_TESTS "Build test suite" ON)
if(BUILD_TESTS)
    enable_testing()
    find_package(GTest CONFIG REQUIRED)
    add_subdirectory(tests)
endif()

# 安装规则
install(TARGETS nova-core nova-lexer nova-parser nova-ast nova-cli
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY include/nova DESTINATION include)
