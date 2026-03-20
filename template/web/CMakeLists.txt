# NovaMark WebAssembly Runtime
# 仅在 ENABLE_WASM=ON 且使用 Emscripten 工具链时构建

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    message(FATAL_ERROR "This directory requires Emscripten toolchain")
endif()

# 输出文件扩展名
set(CMAKE_EXECUTABLE_SUFFIX ".js")

# WASM 运行时可执行文件
add_executable(nova-wasm-runtime
    src/nova_wasm_main.cpp
    ${CMAKE_SOURCE_DIR}/src/renderer/wasm/nova_wasm_exports.cpp
)

target_include_directories(nova-wasm-runtime PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/template/web/src
)

# 链接核心库
target_link_libraries(nova-wasm-runtime PRIVATE
    nova-vm
    nova-packer
    nova-renderer
)

# Emscripten 编译和链接选项
set_target_properties(nova-wasm-runtime PROPERTIES
    OUTPUT_NAME "nova"
)

# 链接标志
target_link_options(nova-wasm-runtime PRIVATE
    # 导出的函数
    "SHELL:-s EXPORTED_FUNCTIONS=[\
        _nova_wasm_init,\
        _nova_wasm_load_package,\
        _nova_wasm_start,\
        _nova_wasm_next,\
        _nova_wasm_select_choice,\
        _nova_wasm_get_status,\
        _nova_wasm_get_bg,\
        _nova_wasm_get_bgm,\
        _nova_wasm_get_bgm_volume,\
        _nova_wasm_get_bgm_loop,\
        _nova_wasm_has_dialogue,\
        _nova_wasm_get_dialogue_speaker,\
        _nova_wasm_get_dialogue_text,\
        _nova_wasm_get_dialogue_color,\
        _nova_wasm_has_choices,\
        _nova_wasm_get_choice_count,\
        _nova_wasm_get_choice_text,\
        _nova_wasm_is_choice_disabled,\
        _nova_wasm_get_sprite_count,\
        _nova_wasm_get_sprite_url,\
        _nova_wasm_get_sprite_x,\
        _nova_wasm_get_sprite_y,\
        _nova_wasm_get_sprite_opacity,\
        _nova_wasm_get_sprite_z_index,\
        _nova_wasm_get_asset_size,\
        _nova_wasm_get_asset_bytes,\
        _nova_wasm_export_save_json,\
        _nova_wasm_import_save_json,\
        _nova_wasm_free,\
        _nova_get_current_scene,\
        _nova_get_current_label,\
        _nova_get_status,\
        _nova_is_ended,\
        _nova_get_ending_id,\
        _malloc,\
        _free\
    ]"
    
    # 导出的运行时方法
    "SHELL:-s EXPORTED_RUNTIME_METHODS=[ccall,cwrap,UTF8ToString,stringToUTF8,lengthBytesUTF8,getValue,setValue]"
    
    # 内存配置
    "SHELL:-s ALLOW_MEMORY_GROWTH=1"
    "SHELL:-s INITIAL_MEMORY=16777216"
    "SHELL:-s STACK_SIZE=1048576"
    
    # 模块化输出
    "SHELL:-s MODULARIZE=1"
    "SHELL:-s EXPORT_NAME=createNovaRuntime"
    "SHELL:-s ENVIRONMENT=web"
    
    # 异常支持
    "SHELL:-s DISABLE_EXCEPTION_CATCHING=0"
    
    # 文件系统（用于资源）
    "SHELL:-s FORCE_FILESYSTEM=1"
    "SHELL:-s NODERAWFS=0"
)

# 编译选项
target_compile_options(nova-wasm-runtime PRIVATE
    -O2
    -fno-rtti
    -fno-exceptions
)

# Debug 模式额外选项
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(nova-wasm-runtime PRIVATE -g4)
    target_link_options(nova-wasm-runtime PRIVATE "SHELL:-s ASSERTIONS=2")
endif()

# 安装 WASM 输出文件
install(FILES 
    ${CMAKE_CURRENT_BINARY_DIR}/nova.js
    ${CMAKE_CURRENT_BINARY_DIR}/nova.wasm
    DESTINATION share/novamark/web
)

# 复制模板文件到构建目录
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/index.html
    ${CMAKE_CURRENT_BINARY_DIR}/index.html
    COPYONLY
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/style.css
    ${CMAKE_CURRENT_BINARY_DIR}/style.css
    COPYONLY
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/app.js
    ${CMAKE_CURRENT_BINARY_DIR}/app.js
    COPYONLY
)
