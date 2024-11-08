# Distirolis - Concurrent Search Engine

## Overview
Distirolis is a high-performance search engine featuring a concurrent web crawler developed in C++. It utilizes libcurl and MongoDB for efficient indexing and search capabilities.

## Features
- **Concurrent Web Crawler**: Developed using libcurl for fast and efficient page crawling.
- **Search Capabilities**: Implemented logical operations and BM25 ranking, adhering to the robots.txt protocol.
- **Performance**: Successfully crawled 10,000 pages in 5 minutes, showcasing high efficiency.
- **API Management**: Created a Crow server (C++ library) to manage crawling, indexing, and search requests via API endpoints.
- **Concurrency**: Utilized multithreading and condition variables to enhance the crawling process.

## Technologies
- **Programming Languages**: C++
- **Build System**: CMake
- **Database**: MongoDB
- **Libraries**: libcurl, Crow, ASIO, libstemmer

## CMake Configuration Summary
The project uses CMake for build configuration. Key settings include:

- **C++ Standard**: C++20
- **Dependencies**: Crow, ASIO, libcurl, libstemmer, MongoDB libraries.
- **Platform-Specific Configurations**: Different paths and settings for Windows and Linux environments.
- **Build Options**: Debug and Release modes with appropriate compiler options.

### Detailed CMakeLists.txt
For a complete view of the project's CMake configuration, please refer to the `CMakeLists.txt` file in the root directory. This file includes detailed settings for project structure, source files, library dependencies, and platform-specific configurations.

### Build Instructions
To build the project, make sure to have the necessary libraries and tools installed. Follow these steps:

1. Clone the repository or download the source files.
2. Create a build directory:
   ```bash
   mkdir build
   cd build
3. Run CMake:
   ```bash
   cmake ..
4. Compile the project:
   ```bash
   make
