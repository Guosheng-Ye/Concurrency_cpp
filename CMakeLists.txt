cmake_minimum_required(VERSION 3.28.0)
project(cpp_threads VERSION 0.1.0 LANGUAGES C CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set (CMAKE_C_COMPILER "D:\\MinGW13_2\\mingw64\\bin\\gcc.exe")
# set (CMAKE_CXX_COMPILER "D:\\MinGW13_2\\mingw64\\bin\\g++.exe")
# 设置CPack参数
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

# 设置thirdlib中的spdlog库路径
set(SPDLOG_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/thirdlib/spdlog/)

# 设置GTest库路径
set(GTEST_LIB ${CMAKE_SOURCE_DIR}/gtest/lib)
set(GTEST_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/gtest/include)

message("GTEST_LIB: " ${GTEST_LIB})
message("GTEST_INCLUDE_DIRLIB: " ${GTEST_INCLUDE_DIR})

# 包含GTest头文件
include_directories(${GTEST_INCLUDE_DIR})
link_directories(${GTEST_LIB})

# 函数用于创建每个项目的可执行文件
function(create_project_executable project_dir)
    file(GLOB_RECURSE src_files ${project_dir}/*.h ${project_dir}/*.cpp)
    get_filename_component(project_name ${project_dir} NAME)
    add_executable(${project_name} ${src_files})

    # 添加spdlog,GTest
    target_include_directories(${project_name} PRIVATE ${SPDLOG_INCLUDE_DIR})
    target_link_libraries(${project_name} PRIVATE ${GTEST_LIB}/libgtest_main.a  ${GTEST_LIB}/libgtest.a pthread)

endfunction()

include_directories(${CMAKE_CURRENT_LIST_DIR}/thirdlib)

# 遍历子目录，为每个包含main.cpp文件的子目录创建一个可执行文件
file(GLOB children RELATIVE ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/*)
foreach (child ${children})
    if (IS_DIRECTORY ${CMAKE_SOURCE_DIR}/${child})
        file(GLOB main_cpp_files ${CMAKE_SOURCE_DIR}/${child}/*.cpp)
        if (main_cpp_files)
            create_project_executable(${CMAKE_SOURCE_DIR}/${child})
        endif ()
    endif ()
endforeach ()
