cmake_minimum_required(VERSION 3.14)

project(WeiXins VERSION 1.0.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

cmake_policy(SET CMP0135 NEW)
include(FetchContent)

add_executable(${PROJECT_NAME} src/main.cpp res/resource.rc)
set_target_properties(${PROJECT_NAME} PROPERTIES VS_MANIFEST_INPUT "${CMAKE_CURRENT_SOURCE_DIR}/res/app.manifest")

set_target_properties(${PROJECT_NAME} PROPERTIES CXX_EXTENSIONS OFF)
if((CMAKE_CXX_COMPILER_ID MATCHES "GNU") OR (CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wshadow -pedantic-errors -Werror)
    # 静态编译
    target_link_options(${PROJECT_NAME} PRIVATE -static-libgcc -static-libstdc++)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX)
    target_compile_options(${PROJECT_NAME} PRIVATE /utf-8 /permissive)
    # 静态编译
    set_target_properties(${PROJECT_NAME} PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    # 支持win7
    target_compile_definitions(${PROJECT_NAME} PRIVATE _WIN32_WINNT=0x0601 WINVER=0x0601)
endif()
# cxxui 依赖
target_include_directories(${PROJECT_NAME} PRIVATE include)
FetchContent_Declare(cxxui GIT_REPOSITORY https://github.com/liehuoe/cxxui.git GIT_TAG main)
set(CXXUI_USE_WEB_WINDOW ON CACHE BOOL "Use WebWindow" FORCE)
FetchContent_MakeAvailable(cxxui)
target_link_libraries(${PROJECT_NAME} PRIVATE cxxui)
