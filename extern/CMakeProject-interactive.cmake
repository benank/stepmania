set(INTERACTIVE_DIR "${SM_EXTERN_DIR}/interactive-cpp")

file(GLOB_RECURSE INTERACTIVE_SRC ${INTERACTIVE_DIR}/include/interactive/internal/*.cpp)
file(GLOB_RECURSE INTERACTIVE_HPP ${INTERACTIVE_DIR}/include/interactive/internal/*.h)

source_group("Internal" FILES ${INTERACTIVE_SRC} ${INTERACTIVE_HPP})

set(INCLUDE_HPP "${INTERACTIVE_DIR}/include/interactive/interactivity.h")

list(APPEND INTERACTIVE_INCLUDE
            "${INCLUDE_HPP}")

source_group("" FILES ${INTERACTIVE_INCLUDE})

list(APPEND INTERACTIVE_HPP
            "${INCLUDE_HPP}")

add_library("interactive-cpp" STATIC ${INTERACTIVE_SRC} ${INTERACTIVE_HPP})

set_property(TARGET "interactive-cpp" PROPERTY FOLDER "External Libraries")

target_include_directories("interactive-cpp" PUBLIC ${INTERACTIVE_DIR}/include/interactive)

# include_directories(src)

if(MSVC)
  sm_add_compile_definition("interactive-cpp" _CRT_SECURE_NO_WARNINGS)
endif(MSVC)

disable_project_warnings("interactive-cpp")
