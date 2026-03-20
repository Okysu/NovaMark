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
find_package(nlohmann_json CONFIG QUIET)
if(NOT nlohmann_json_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        nlohmann_json
        URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
    )
    FetchContent_MakeAvailable(nlohmann_json)
endif()

# 编译选项
if(MSVC)
    add_compile_options(/W4 /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# 子目录
add_subdirectory(src)

# 测试（WASM 默认关闭，native 默认开启）
if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    set(NOVA_DEFAULT_BUILD_TESTS OFF)
else()
    set(NOVA_DEFAULT_BUILD_TESTS ON)
endif()

option(BUILD_TESTS "Build test suite" ${NOVA_DEFAULT_BUILD_TESTS})
if(BUILD_TESTS)
    enable_testing()
    find_package(GTest CONFIG REQUIRED)
    add_subdirectory(tests)
endif()

# WebAssembly 构建（需要 Emscripten 工具链）
option(ENABLE_WASM "Build WebAssembly runtime (requires Emscripten)" OFF)
if(ENABLE_WASM)
    if(NOT CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
        message(FATAL_ERROR "ENABLE_WASM=ON requires Emscripten toolchain. Use: -DCMAKE_TOOLCHAIN_FILE=\$EMSDK/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake")
    endif()
    message(STATUS "Building for WebAssembly")
    add_subdirectory(template/web)
endif()

# 安装规则
install(TARGETS nova-core nova-lexer nova-parser nova-ast nova-cli
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY include/nova DESTINATION include)
