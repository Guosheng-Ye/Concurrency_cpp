cmake_minimum_required(VERSION 3.28.0)
project(cpp_threads VERSION 0.1.0 LANGUAGES C CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)

# 设置CPack参数
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

# 设置thirdlib中的spdlog库路径
set(SPDLOG_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/thirdlib/spdlog/include)

# 函数用于创建每个项目的可执行文件
function(create_project_executable project_dir)
    file(GLOB_RECURSE src_files ${project_dir}/*.h ${project_dir}/*.cpp)
    get_filename_component(project_name ${project_dir} NAME)
    add_executable(${project_name} ${src_files})

    # 添加spdlog的包含目录
    target_include_directories(${project_name} PRIVATE ${SPDLOG_INCLUDE_DIR})

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
