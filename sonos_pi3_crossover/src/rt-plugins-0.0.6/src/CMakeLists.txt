include_directories(${PROJECT_SOURCE_DIR}/include)

option(USE_SSE2 "use SSE2 instructions to avoid floating point denormalization issues" off)

add_library(RTallpass1 MODULE RTallpass1.c)
add_library(RTallpass2 MODULE RTallpass2.c)

add_library(RTlowpass MODULE RTlowpass.c)
add_library(RThighpass MODULE RThighpass.c)

add_library(RTlowshelf MODULE RTlowshelf.c)
add_library(RThighshelf MODULE RThighshelf.c)

add_library(RTlr4hipass MODULE RTlr4hipass.c)
add_library(RTlr4lowpass MODULE RTlr4lowpass.c)

add_library(RTparaeq MODULE RTparaeq.c)

add_library(RTlowpass1 MODULE RTlowpass1.c)
add_library(RThighpass1 MODULE RThighpass1.c)

add_library(RTdecorrmls MODULE RTdecorrmls.c)

set_target_properties(RTallpass1 RTallpass2 RTlowpass RThighpass RTlowshelf RThighshelf RTlr4hipass RTlr4lowpass RTparaeq RTlowpass1 RThighpass1 RTdecorrmls
                      PROPERTIES LINK_FLAGS "-nostartfiles")
if(USE_SSE2)
  set_target_properties(RTallpass1 RTallpass2 RTlowpass RThighpass RTlowshelf RThighshelf RTlr4hipass RTlr4lowpass RTparaeq RTlowpass1 RThighpass1 RTdecorrmls
                        PROPERTIES COMPILE_FLAGS "-msse2 -mfpmath=sse")
endif()
install(TARGETS RTallpass1 RTallpass2 RTlowpass RThighpass RTlowshelf RThighshelf RTlr4hipass RTlr4lowpass RTparaeq RTlowpass1 RThighpass1 RTdecorrmls
        LIBRARY DESTINATION lib/ladspa)

