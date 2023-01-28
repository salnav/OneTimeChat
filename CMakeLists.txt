cmake_minimum_required(VERSION 3.2 FATAL_ERROR)

if(POLICY CMP0025)
    cmake_policy(SET CMP0025 NEW)
endif()

project(PleaseWork LANGUAGES C CXX)

# Enforce the C++ standard, and disable extensions.
if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
endif()

set(CMAKE_CXX_EXTENSIONS OFF)
set(Qt6_DIR /Users/salmannavroz/Qt/6.4.2/macos/lib/cmake/Qt6/)
set(Qt6CoreTools_DIR /Users/salmannavroz/Qt/6.4.2/macos/lib/cmake/Qt6CoreTools)
set(Qt6GuiTools_DIR /Users/salmannavroz/Qt/6.4.2/macos/lib/cmake/Qt6GuiTools)
if (NOT CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror")
endif()

message(STATUS "CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")

find_package(mongocxx REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Widgets)
qt_standard_project_setup()
# Add your source files here
add_executable(PleaseWork main.cpp)

# Visual Studio pre 2017 requires boost polyfill.
if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    if (CXX_STANDARD LESS 17)
        find_package(Boost 1.56.0 REQUIRED)
        if (CMAKE_VERSION VERSION_LESS 3.15.0)
            target_include_directories(PleaseWork PRIVATE ${Boost_INCLUDE_DIRS})
        else()
            target_link_libraries(PleaseWork PRIVATE Boost::boost)
        endif()
        target_compile_definitions(PleaseWork PRIVATE BSONCXX_POLY_USE_BOOST=ON)
    else ()
        target_compile_options(PleaseWork PRIVATE /Zc:__cplusplus)
    endif ()
endif()

include_directories("/usr/local/include/mongocxx/v_noabi")
include_directories("/usr/local/include/bsoncxx/v_noabi")
include_directories("/usr/local/include/libmongoc-1.0")
include_directories("/usr/local/include/libbson-1.0")
include_directories("/usr/local/lib")
include_directories(${Qt5Core_INCLUDE_DIRS} ${Qt5Widgets_INCLUDE_DIRS})

target_link_libraries(PleaseWork PRIVATE mongo::mongocxx_shared)
target_link_libraries(PleaseWork PRIVATE mongo::bsoncxx_shared)
target_link_libraries(PleaseWork PRIVATE Qt6::Widgets)
if(mongocxx_FOUND)
    message(STATUS "libmongocxx found")
else()
    message(FATAL_ERROR "libmongocxx not found")
endif()
