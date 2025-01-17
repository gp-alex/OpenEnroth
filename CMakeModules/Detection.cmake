set(BUILD_COMPILER "unknown")
set(BUILD_PLATFORM "unknown")
set(BUILD_TYPE "unknown")

if(WIN32)
  set(BUILD_PLATFORM "win32")
elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
  set(BUILD_PLATFORM "darwin")
  set(APPLE TRUE)
elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
  set(BUILD_PLATFORM "linux")
elseif(CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
  set(BUILD_PLATFORM "freebsd")
endif()

# TODO: We should replace this with proper architecture detection like DetectArchitecture in luajit
if (CMAKE_SIZEOF_VOID_P MATCHES 8)
  set(BUILD_TYPE "x64")
elseif (CMAKE_SIZEOF_VOID_P MATCHES 4)
  set(BUILD_TYPE "x86")
endif ()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(BUILD_COMPILER "gcc")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
  set(BUILD_COMPILER "clang")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  set(BUILD_COMPILER "msvc")
endif()

message(STATUS "Build compiler: ${BUILD_COMPILER}")
message(STATUS "Build platform: ${BUILD_PLATFORM}")
message(STATUS "Build type: ${BUILD_TYPE}")
