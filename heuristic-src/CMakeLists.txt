cmake_minimum_required(VERSION 3.12)
set(projectName heuristic)
message("cmake for ${projectName}")
project(${projectName})

enable_testing()

# List of local libraries
set(localLibs
    MaLib
    cadical
    EvalMaxSAT
    open-mcs
)

# List of external libraries
set(externLibs
    z
    scip
    soplex
)

option(BUILD_SHARED_LIBS "Build using shared libraries" OFF)
set(CMAKE_EXE_LINKER_FLAGS "-static")

include_directories(lib/scipoptsuite/build/scip)
include_directories(lib/scipoptsuite/scip/src)
include_directories(lib/open-mcs)
link_directories(lib/scipoptsuite/build/lib)
link_directories(lib/open-mcs)

include_directories(lib/eigen)

set(CMAKE_CXX_STANDARD 20)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# Add local libraries
FOREACH(localLib ${localLibs})
    if(NOT TARGET ${localLib})
        add_subdirectory(lib/${localLib})
    endif()
ENDFOREACH(localLib)

find_package(ZLIB)
include_directories(${ZLIB_INCLUDE_DIR})


add_executable(${projectName} main.cpp)

target_link_libraries(${projectName} 
    ${localLibs}
    ${externLibs}
    ${CMAKE_SOURCE_DIR}/libglpk.a
)

install(TARGETS ${projectName}
    DESTINATION ${CMAKE_INSTALL_BINDIR}
)


