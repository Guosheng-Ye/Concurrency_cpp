cmake_minimum_required(VERSION 3.28.0)
project(cpp_threads VERSION 0.1.0 LANGUAGES C CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)

# 设置CPack参数
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

# 查找Qt6的Core、Gui、Widgets和SerialPort模块
set(CMAKE_PREFIX_PATH "D:/Qt/6.2.4/mingw_64")
find_package(Qt6 COMPONENTS Core Gui Widgets SerialPort REQUIRED PATHS "D:/Qt/6.2.4/mingw_64/lib/cmake/Qt6")

# 包含Qt6的头文件搜索路径
set(Qt6_INCLUDE_DIRS "D:/Qt/6.2.4/mingw_64/include/")
include_directories(${Qt6_INCLUDE_DIRS})

# 函数用于创建每个项目的可执行文件
function(create_project_executable project_dir)
    file(GLOB_RECURSE src_files ${project_dir}/*.h ${project_dir}/*.cpp )
    get_filename_component(project_name ${project_dir} NAME)
    add_executable(${project_name} ${src_files})
    
endfunction()


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
