cmake_minimum_required(VERSION 3.0.0)
project (miniwnd LANGUAGES CXX)

if (MSVC)
  string(REGEX REPLACE "/W[0-9]" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
endif (MSVC)

if (UNIX)
  find_package(X11 REQUIRED)
endif ()

message(STATUS "X11_INCLUDE_DIR: ${X11_INCLUDE_DIR}")
message(STATUS "X11_LIBRARIES: ${X11_LIBRARIES}")

add_executable(${PROJECT_NAME} main.cpp)

target_include_directories (${PROJECT_NAME} PRIVATE $<$<PLATFORM_ID:Linux>:${X11_INCLUDE_DIR}>)

# Bug in FindX11.cmake, resulting variable is not genexpr friendly
#
#target_link_libraries (${PROJECT_NAME} PRIVATE $<$<PLATFORM_ID:Linux>:${X11_LIBRARIES}>)
#
if (UNIX)
  target_link_libraries (${PROJECT_NAME} PRIVATE ${X11_LIBRARIES})
endif ()

set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17
                                                 CXX_STANDARD_REQUIRED ON
                                                 CXX_EXTENSIONS OFF)

target_compile_options(${PROJECT_NAME} PRIVATE $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-Wall -Wextra -pedantic>
                                               $<$<CXX_COMPILER_ID:MSVC>:/W4 /permissive->)
