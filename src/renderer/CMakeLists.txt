add_library(nova-renderer STATIC
    nova_c_api.cpp
)

target_compile_definitions(nova-renderer PRIVATE NOVA_EXPORTS)

target_include_directories(nova-renderer PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(nova-renderer PUBLIC
    nova-vm
    nova-packer
)
