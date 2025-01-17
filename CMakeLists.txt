cmake_minimum_required(VERSION 3.20.4 FATAL_ERROR)
project("World of Might and Magic")
add_compile_definitions(PACKAGE_BUGREPORT="https://github.com/gp-alex/world-of-might-and-magic/issues")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules)
include(Git)
include(Detection)
include(AppleLibcxxAssertions)
include(ExternalProject)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(ENABLE_TESTS OFF CACHE BOOL "Enable tests")
if(ENABLE_TESTS)
  message(STATUS "Tests have been enabled")
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(CMAKE_EXE_LINKER_FLAGS "-fuse-ld=gold -pthread")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  add_compile_definitions(NOMINMAX) # please don't pull in these macros from <Windows.h>
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS) # STL security warnings are just noise
  add_compile_definitions(_CRT_NONSTDC_NO_DEPRECATE) # POSIX deprecation warnings are also just noise
  add_compile_definitions(_USE_MATH_DEFINES) # Pull in M_PI and other <cmath> defines
  add_compile_options(/Zc:preprocessor) # Use standard compliant preprocessor
  set(CMAKE_CXX_FLAGS "/MP ${CMAKE_CXX_FLAGS}")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SAFESEH:NO")
elseif(APPLE)
  enable_libcxx_assertions(FALSE)
endif()

if(UNIX)
  add_compile_options(-Werror=return-type) # Control reaches the end of non-void function, this is an error on MSVC
endif()

add_compile_definitions(FMT_USE_NONTYPE_TEMPLATE_ARGS) # Enable _cf literals

include(CppLint)
include(Dependencies)

add_subdirectory(thirdparty)

include_directories(${INCLUDE_DIRECTORIES} ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/test ${INCLUDE_THIRDPARTY_DIRS})
add_subdirectory(test)
add_subdirectory(src)

add_executable(World_of_Might_and_Magic MACOSX_BUNDLE WIN32 src/Application/main.cpp)
target_check_style(World_of_Might_and_Magic)
target_fix_libcxx_assertions(World_of_Might_and_Magic)

target_link_libraries(World_of_Might_and_Magic game)

set_property(
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT World_of_Might_and_Magic
)

PREBUILT_DEPENDENCIES_RESOLVE(World_of_Might_and_Magic)
