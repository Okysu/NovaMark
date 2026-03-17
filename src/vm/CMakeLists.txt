add_library(nova-vm STATIC
    vm.cpp
    variable.cpp
    inventory.cpp
    save_data.cpp
    serializer.cpp
)

target_include_directories(nova-vm PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(nova-vm PUBLIC
    nova-core
    nova-ast
)