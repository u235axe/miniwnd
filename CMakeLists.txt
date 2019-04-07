cmake_minimum_required(VERSION 3.0.0)
project (miniwnd)

if (MSVC)
  string(REGEX REPLACE "/W[0-9]" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
endif (MSVC)

set(CMAKE_VERBOSE_MAKEFILE ON)
#find_package(X11 REQUIRED)
#set(GCC_CXX_FLAGS "${GCC_CXX_FLAGS} -lX11")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lX11")
#if (CMAKE_SYSTEM_NAME MATHCES Linux)  
#elseif (CMAKE_SYSTEM_NAME MATHCES Windows)
#endif

message(STATUS "X11 libraries libs=${X11_LIBRARIES}")

add_executable(${PROJECT_NAME} main.cpp)

#target_include_directories (${PROJECT_NAME} PRIVATE $<$<PLATFORM_ID:Linux>:${X11_INCLUDE_DIR}>)

#target_link_libraries (${PROJECT_NAME} PRIVATE $<$<PLATFORM_ID:Linux>:${X11_LIBRARIES}>)

set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17
                                                 CXX_STANDARD_REQUIRED ON
                                                 CXX_EXTENSIONS OFF)

target_compile_options(${PROJECT_NAME} PRIVATE $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-Wall -Wextra -pedantic>
                                               $<$<CXX_COMPILER_ID:MSVC>:/W4 /permissive->)