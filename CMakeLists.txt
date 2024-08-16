cmake_minimum_required(VERSION 3.10)

# Project name
project(DistributedSearchEngine)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Specify the system name and compilers
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_C_COMPILER "C:/msys64/mingw64/bin/gcc.exe")
set(CMAKE_CXX_COMPILER "C:/msys64/mingw64/bin/g++.exe")

# Define source files
set(SOURCES
    source/main.cpp
    source/DataBase.cpp
    source/URLParser.cpp
    source/WebCrawler.cpp
    source/HTMLParser.cpp
    source/InvertedIndex.cpp
    source/WordProcessor.cpp
)

# Add the executable
add_executable(DistributedSearchEngine ${SOURCES})

# Include directories
target_include_directories(DistributedSearchEngine PRIVATE 
    ${PROJECT_SOURCE_DIR}/include
    "E:/CppModules/libstemmer/libstemmer_c-2.2.0/libstemmer_c-2.2.0/include"
    "C:/msys64/mingw64/include/libxml2"  # Correct path for libxml2 headers
    "C:/msys64/mingw64/include/mongoc-1.0/libmongoc-1.0"
    "C:/msys64/mingw64/include/mongoc-1.0/libbson-1.0"
)

# Link directories
link_directories(
    "C:/msys64/mingw64/lib"
    "E:/CppModules/libstemmer/libstemmer_c-2.2.0/libstemmer_c-2.2.0"  # Correct path for libstemmer.a
)

# Link libraries
target_link_libraries(DistributedSearchEngine PRIVATE
    stemmer
    ${CMAKE_DL_LIBS}
    curl
    xml2
    mongoc-1.0
    bson-1.0
)

# Compiler options
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(DistributedSearchEngine PRIVATE DEBUG)
    target_compile_options(DistributedSearchEngine PRIVATE -Wall -g)
else()
    target_compile_definitions(DistributedSearchEngine PRIVATE NDEBUG)
    target_compile_options(DistributedSearchEngine PRIVATE -Wall -O3)
endif()
if (CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(DistributedSearchEngine PRIVATE -Wno-all)  # Disable all warnings
endif()

# Ensure CMake version
if (CMAKE_VERSION VERSION_LESS "3.13")
    message(FATAL_ERROR "CMake 3.13 or higher is required.")
endif()
