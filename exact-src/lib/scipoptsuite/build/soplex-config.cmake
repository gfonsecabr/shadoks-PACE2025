if(NOT TARGET libsoplex)
  include("${CMAKE_CURRENT_LIST_DIR}/soplex-targets.cmake")
endif()

set(SOPLEX_LIBRARIES libsoplex)
set(SOPLEX_PIC_LIBRARIES libsoplex-pic)
set(SOPLEX_INCLUDE_DIRS "/home/florent/UQAM/MaxSAT2024/code/EvalMaxSAT/lib/scipoptsuite/soplex/src;/home/florent/UQAM/MaxSAT2024/code/EvalMaxSAT/lib/scipoptsuite/build/soplex")
set(SOPLEX_FOUND TRUE)

if()
  find_package(Boost 1.65.0)
  include_directories(${Boost_INCLUDE_DIRS})
endif()

if(off)
  set(SOPLEX_WITH_PAPILO TRUE)
endif()
