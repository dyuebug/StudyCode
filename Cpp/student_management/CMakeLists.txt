cmake_minimum_required(VERSION 3.16)

project(StudentManagement LANGUAGES CXX)

enable_testing()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(STUDENT_MANAGEMENT_SOURCES
    Course.cpp
    Student.cpp
    StudentManager.cpp
)

add_library(student_management_core ${STUDENT_MANAGEMENT_SOURCES})
target_include_directories(student_management_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(student_management_core PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /utf-8>
)

add_executable(student_management_app main.cpp)
target_link_libraries(student_management_app PRIVATE student_management_core)
target_compile_options(student_management_app PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /utf-8>
)

add_executable(student_management_tests test_student_manager.cpp)
target_link_libraries(student_management_tests PRIVATE student_management_core)
target_compile_options(student_management_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /utf-8>
)

add_test(
    NAME student_management_unit_tests
    COMMAND student_management_tests
            ${CMAKE_CURRENT_BINARY_DIR}/student_data.txt
)
set_tests_properties(student_management_unit_tests PROPERTIES
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
