cmake_minimum_required(VERSION 3.12)

get_filename_component(projectName ${CMAKE_CURRENT_SOURCE_DIR} NAME)
message("cmake for ${projectName}")
project(${projectName})

set(CMAKE_CXX_STANDARD 20)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

file(
	GLOB_RECURSE
	source_files
	./*
)

add_library (
    open-mcs
    ${source_files}
)

target_include_directories(open-mcs PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(open-mcs)
