cmake_minimum_required(VERSION 3.16)

project(CppBasics LANGUAGES CXX)

set(CMAKE_CXX_EXTENSIONS OFF)

function(add_cpp_basics_example target_name source_file language_standard)
    add_executable(${target_name} ${source_file})
    set_target_properties(${target_name} PROPERTIES
        CXX_STANDARD ${language_standard}
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
    )
    target_compile_options(${target_name} PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /utf-8>
    )
endfunction()

add_cpp_basics_example(cpp_basics_hello 01_hello_world.cpp 11)
add_cpp_basics_example(cpp_basics_file_io 34_file_io_basics.cpp 11)
add_cpp_basics_example(cpp_basics_vector 58_vector_basics.cpp 11)
add_cpp_basics_example(cpp_basics_smart_pointers 69_smart_pointers.cpp 14)
add_cpp_basics_example(cpp_basics_concepts 85_concepts_basics.cpp 20)
add_cpp_basics_example(cpp_basics_ranges 88_ranges_basics.cpp 20)

set_target_properties(
    cpp_basics_hello
    cpp_basics_file_io
    cpp_basics_vector
    cpp_basics_smart_pointers
    cpp_basics_concepts
    cpp_basics_ranges
    PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
