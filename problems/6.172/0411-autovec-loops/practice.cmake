# Show the vectorizer's decisions for this exercise in Release builds (see README).
if(PRACTICE_MODE STREQUAL "release")
  target_compile_options(${target} PRIVATE -Rpass=loop-vectorize -Rpass-missed=loop-vectorize -Rpass-analysis=loop-vectorize)
endif()
