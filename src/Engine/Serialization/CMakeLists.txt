cmake_minimum_required(VERSION 3.20.4 FATAL_ERROR)

set(SERIALIZATION_SOURCES LegacyImages.cpp)

set(SERIALIZATION_HEADERS LegacyImages.h
                          Deserializer.h
                          Serializer.h)

if(WIN32)
else()
endif()

add_library(serialization STATIC ${SERIALIZATION_SOURCES} ${SERIALIZATION_HEADERS})
target_check_style(serialization)
