# SETUP
cmake_minimum_required(VERSION 3.11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# PROJECT
project(cropper_project VERSION 0.2.0 LANGUAGES CXX C OBJCXX OBJC)

set(CMAKE_POSITION_INDEPENDENT_CODE 1)
set(FORCE_OPENGL 0)

if(NOT DEFINED DEV_INTERFACE)
    set(DEV_INTERFACE 0)
endif()

set(CMAKE_SKIP_INSTALL_RULES 1 QUIET)
set(BX_SILENCE_DEBUG_OUTPUT 1)
set(ENABLE_IMGUI 1)

# MAIN ENGINE
set(SetupLib_include_dirs)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/engine")

include(BGFXCompileShaders)
BGFXCompileShaders(
    embed
    shader_dirs     "${CMAKE_CURRENT_SOURCE_DIR}/src/shader/cropper"
    profile_list    "metal"
    include_dirs    "${CMAKE_CURRENT_SOURCE_DIR}/engine/shader"
)

# EXE
include_directories(${SetupLib_include_dirs})
set(EXE_NAME cropper)
add_executable(${EXE_NAME} "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")

target_compile_definitions(${EXE_NAME} PUBLIC
    DEV_INTERFACE=${DEV_INTERFACE}
    FORCE_OPENGL=${FORCE_OPENGL}
    ENABLE_IMGUI=${ENABLE_IMGUI}
)

target_link_libraries(${EXE_NAME} "game_project_engine" ${SetupLib_libs})
add_dependencies(${EXE_NAME} BGFXShader_target)

# COPY ASSETS
file(GLOB ASSET_FILES "${CMAKE_CURRENT_SOURCE_DIR}/assets/[!.]*")
file(COPY ${ASSET_FILES} DESTINATION "${CMAKE_BINARY_DIR}/assets")
