cmake_minimum_required(VERSION 3.24)
project(algorithm_pragmatic LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(PRACTICE_MODE "asan" CACHE STRING "Build flavour: asan, tsan or release")
set_property(CACHE PRACTICE_MODE PROPERTY STRINGS asan tsan release)
option(PRACTICE_USE_REFERENCE "Compile the tests against solutions/ instead of problems/" OFF)

add_compile_options(-Wall -Wextra -Wno-unused-parameter)

if(PRACTICE_MODE STREQUAL "asan")
  set(sanitize_flags -fsanitize=address,undefined -fno-sanitize-recover=undefined -fno-omit-frame-pointer)
  add_compile_options(-O1 -g ${sanitize_flags})
  add_link_options(${sanitize_flags})
elseif(PRACTICE_MODE STREQUAL "tsan")
  set(sanitize_flags -fsanitize=thread -fno-omit-frame-pointer)
  add_compile_options(-O1 -g ${sanitize_flags})
  add_link_options(${sanitize_flags})
elseif(PRACTICE_MODE STREQUAL "release")
  add_compile_options(-O2 -g -DNDEBUG)
else()
  message(FATAL_ERROR "Unknown PRACTICE_MODE: ${PRACTICE_MODE}")
endif()

add_library(practice_main STATIC harness/main.cpp)
target_include_directories(practice_main PUBLIC harness)

# One test binary per exercise: problems/<course>/<NNNN-slug>/tests/*.cpp.
#   <exercise>/*.cpp and solution.hpp   your code (or solutions/<same path>/ for the reference build)
#   <exercise>/support/                 fixed helpers shared by both builds
#   problems/<course>/_shared/          fixed helpers shared across one course
#   <exercise>/practice.cmake           optional per-exercise flags; may use ${target}
file(GLOB exercise_dirs CONFIGURE_DEPENDS LIST_DIRECTORIES true "${CMAKE_SOURCE_DIR}/problems/*/*")
# Not used directly: these make a later `solutions/...` or practice.cmake trigger a re-configure.
file(GLOB reference_dirs CONFIGURE_DEPENDS LIST_DIRECTORIES true "${CMAKE_SOURCE_DIR}/solutions/*/*")
file(GLOB exercise_cmake_files CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/problems/*/*/practice.cmake")

foreach(exercise IN LISTS exercise_dirs)
  file(GLOB tests CONFIGURE_DEPENDS "${exercise}/tests/*.cpp")
  if(NOT tests)
    continue()
  endif()

  file(RELATIVE_PATH relative "${CMAKE_SOURCE_DIR}/problems" "${exercise}")
  string(REGEX REPLACE "[^A-Za-z0-9]" "_" target "t_${relative}")

  if(PRACTICE_USE_REFERENCE)
    set(implementation "${CMAKE_SOURCE_DIR}/solutions/${relative}")
    if(NOT EXISTS "${implementation}")
      message(WARNING "No reference solution for ${relative}")
      continue()
    endif()
  else()
    set(implementation "${exercise}")
  endif()

  file(GLOB implementation_sources CONFIGURE_DEPENDS "${implementation}/*.cpp")
  file(GLOB support_sources CONFIGURE_DEPENDS "${exercise}/support/*.cpp")
  add_executable(${target} ${tests} ${implementation_sources} ${support_sources})
  cmake_path(GET exercise PARENT_PATH course_dir)
  target_include_directories(${target} PRIVATE "${implementation}" "${exercise}/support" "${course_dir}/_shared")
  target_link_libraries(${target} PRIVATE practice_main)
  set_target_properties(${target} PROPERTIES
    OUTPUT_NAME run
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/${relative}")

  if(EXISTS "${exercise}/practice.cmake")
    include("${exercise}/practice.cmake")
  endif()
endforeach()
