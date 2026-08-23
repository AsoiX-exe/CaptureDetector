cmake_minimum_required(VERSION 3.10)
project(CaptureDetector VERSION 1.0 LANGUAGES C)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_executable(capture-detector source/capture-detector.c)

if(WIN32)
    add_executable(test_protected source/_test_protected.c)

elseif(APPLE)
    find_library(CORE_GRAPHICS CoreGraphics REQUIRED)
    find_library(CORE_FOUNDATION CoreFoundation REQUIRED)
    
    target_link_libraries(capture-detector PRIVATE 
        ${CORE_GRAPHICS} 
        ${CORE_FOUNDATION}
    )

elseif(UNIX AND NOT APPLE)
    find_package(X11 REQUIRED)
    
    target_include_directories(capture-detector PRIVATE ${X11_INCLUDE_DIR})
    target_link_libraries(capture-detector PRIVATE ${X11_LIBRARIES})
endif()