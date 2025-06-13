
find_program(GIT git)
if(EXISTS ${DST})
   file(STRINGS ${DST} GITHASH_OLD)
   string(REGEX REPLACE "#define SPX_GITHASH \"(.*)\"" "\\1" GITHASH_OLD ${GITHASH_OLD})
endif()
if((GIT) AND (EXISTS /home/florent/UQAM/MaxSAT2024/code/EvalMaxSAT/lib/scipoptsuite/soplex/.git))
   execute_process(
      COMMAND ${GIT} describe --always --dirty
      WORKING_DIRECTORY /home/florent/UQAM/MaxSAT2024/code/EvalMaxSAT/lib/scipoptsuite/soplex
      OUTPUT_VARIABLE GITHASH OUTPUT_STRIP_TRAILING_WHITESPACE)
   string(REGEX REPLACE "^.*-g" "" GITHASH ${GITHASH})
   if(NOT ${GITHASH} STREQUAL "${GITHASH_OLD}")
      file(WRITE ${DST} "#define SPX_GITHASH \"${GITHASH}\"
")
   endif()
else()
   set(GITHASH ${GITHASH_OLD})
   if(NOT GITHASH)
      message(STATUS "Compiling without git information")
      set(GITHASH "NoGitInfo")
   endif()
   file(WRITE ${DST} "#define SPX_GITHASH \"${GITHASH}\"
")
endif()
message(STATUS "Git hash: " ${GITHASH})
