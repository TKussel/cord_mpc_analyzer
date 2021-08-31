cmake_minimum_required(VERSION 3.13)
project(CORD-MPC)
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

find_package(MOTION QUIET)
if(MOTION_FOUND)
  message(STATUS "Found MOTION")
elseif (NOT ABY_FOUND AND NOT TARGET ABY::aby)
  message("MOTION was not found: add MOTION subdirectory")
  if(NOT EXISTS "${PROJECT_SOURCE_DIR}/extern/MOTION/CMakeLists.txt")
        find_package(Git REQUIRED)
        message("initialize Git submodule: extern/MOTION")
        execute_process(COMMAND git submodule update --init extern/MOTION
                        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
    endif()
    add_subdirectory(extern/MOTION)
endif()

find_package(fmt QUIET)
if (fmt_FOUND)
    message(STATUS "Found fmt")
elseif (NOT fmt_FOUND AND NOT TARGET fmt::fmt)
    message("fmt was not found: add fmt subdirectory")
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
endif ()

find_package(spdlog QUIET)
if (spdlog_FOUND)
    message(STATUS "Found spdlog")
elseif (NOT spdlog_FOUND AND NOT TARGET spdlog::spdlog)
    message("spdlog was not found: add spdlog subdirectory")
    if (NOT EXISTS "${PROJECT_SOURCE_DIR}/extern/spdlog/CMakeLists.txt")
        find_package(Git REQUIRED)
        message("initialize Git submodule: extern/spdlog")
        execute_process(COMMAND git submodule update --init extern/spdlog
                WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
    endif ()
    add_subdirectory(extern/spdlog)
endif ()

find_package(csv2 QUIET)
if (csv2_FOUND)
    message(STATUS "Found csv2")
elseif (NOT csv2_FOUND AND NOT TARGET csv2::csv2)
    message("csv2 was not found: add csv2 subdirectory")
    if (NOT EXISTS "${PROJECT_SOURCE_DIR}/extern/csv2/CMakeLists.txt")
        find_package(Git REQUIRED)
        message("initialize Git submodule: extern/csv2")
        execute_process(COMMAND git submodule update --init extern/csv2
                WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
    endif ()
    add_subdirectory(extern/csv2)
endif ()

set(${P}_MAIN_SOURCES
  src/util.cpp
 )
# main target
add_executable(cord-mpc src/cord-mpc_main.cpp ${${P}_MAIN_SOURCES})
target_link_libraries(cord-mpc
  MOTION::motion spdlog::spdlog_header_only csv2::csv2
  fmt::fmt-header-only Boost::program_options)
target_compile_features(cord-mpc PUBLIC cxx_std_20)
target_compile_options(cord-mpc PRIVATE ${${P}_EXTRA_WARNING_FLAGS})
