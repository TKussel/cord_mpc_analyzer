cmake_minimum_required(VERSION 3.13)
project(mpc_histogram)
set(P ${PROJECT_NAME})

if (CMAKE_COMPILER_IS_GNUCXX AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 8.0)
  message(FATAL_ERROR "CORD-MPC requires at least g++-8")
endif ()

# inspired by https://kristerw.blogspot.com/2017/09/useful-gcc-warning-options-not-enabled.html
set(${P}_EXTRA_WARNING_FLAGS
  "-Wall" "-Wpedantic" "-Wextra"
  "-Wduplicated-cond" "-Wduplicated-branches"
  "-Wlogical-op" "-Wrestrict"
  "-Wnull-dereference"
  #"-Wold-style-cast"
  "-Wuseless-cast"
  "-Wdouble-promotion"
  #"-Wshadow"
)

# Custom compiler and linker flags
add_compile_options("-pipe"
  "-fPIC"
  "$<$<CONFIG:RELEASE>:-O2;-march=native>"
  "$<$<CONFIG:DEBUG>:-O0;-g3;-ggdb>"
  ${CMAKE_FLAGS_EXTRA})

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

# At least boost 1.73.0 is needed to use the C++20 standard.
set(REQUIRED_BOOST_VERSION "1.73.0")

find_package(Boost QUIET ${REQUIRED_BOOST_VERSION}
        COMPONENTS
        program_options
        log
        log_setup
        )
set(Boost_USE_STATIC_LIBS ON)

if(NOT EXISTS "${PROJECT_SOURCE_DIR}/extern/MOTION/CMakeLists.txt")
  find_package(Git REQUIRED)
  message("initialize Git submodule: extern/MOTION")
  execute_process(COMMAND git submodule update --init extern/MOTION
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
endif()
add_subdirectory(extern/MOTION)

if (NOT EXISTS "${PROJECT_SOURCE_DIR}/extern/fmt/CMakeLists.txt")
    find_package(Git REQUIRED)
    message("initialize Git submodule: extern/fmt")
    execute_process(COMMAND git submodule update --init extern/fmt
            WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
endif ()
# we use the header-only version of fmt
# -> add `target_compile_definitions($TARGET PUBLIC FMT_HEADER_ONLY)` for
# targets using fmt
#set(FMT_TEST OFF CACHE BOOL "Enable building fmt tests" FORCE)
#add_subdirectory(extern/fmt)

if (NOT EXISTS "${PROJECT_SOURCE_DIR}/extern/pybind11/CMakeLists.txt")
    find_package(Git REQUIRED)
    message("initialize Git submodule: extern/pybind11")
    execute_process(COMMAND git submodule update --init extern/pybind11
            WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
endif ()
add_subdirectory(extern/pybind11)

set(${P}_MAIN_SOURCES
  src/histogram_circuit_builder.cpp
 )

set(${P}_HEADER
    include/histogram_circuit_builder.h
    )
# main target
pybind11_add_module(${P} SYSTEM src/cord-mpc_main.cpp ${${P}_MAIN_SOURCES} ${${P}_HEADER})
target_link_libraries(${P} PUBLIC
  MOTION::motion
  fmt::fmt-header-only Boost::program_options)
target_include_directories(${P} PRIVATE extern/)
target_compile_features(${P} PUBLIC cxx_std_20)
target_compile_options(${P} PRIVATE ${${P}_EXTRA_WARNING_FLAGS})
