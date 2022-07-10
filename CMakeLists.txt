cmake_minimum_required(VERSION 3.9)
project(cube VERSION 0.0.0.0 DESCRIPTION "Cube!" LANGUAGES C)

set(C_STANDARD C11)
set(C_STANDARD_REQUIRED True)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

set(SRC_DIR "${CMAKE_SOURCE_DIR}/src")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)

add_library(render SHARED ${SRC_DIR}/render.c)

add_executable(${PROJECT_NAME} ${SRC_DIR}/main.c)

target_link_libraries(
	${PROJECT_NAME}
	PUBLIC
	Vulkan::Vulkan
	glfw
	render
)
