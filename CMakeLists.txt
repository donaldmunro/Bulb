cmake_minimum_required(VERSION 3.10)
project(Bulb)

#Assumes filament has been cloned/unzipped to ${FILAMENT_DIR}, built for example using
#CC=/usr/bin/clang CXX=/usr/bin/clang++ ./build.sh -j -p desktop debug
# and installed to ${FILAMENT_DIR}/out using for example
#CC=/usr/bin/clang CXX=/usr/bin/clang++ ./build.sh -j -p desktop -i debug
set(FILAMENT_DIR "/opt/filament")

include(CheckIncludeFiles)
include(CheckIncludeFileCXX)
include(CheckCXXSourceCompiles)
include(CheckCXXSourceRuns)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ -std=c++17 -fno-rtti -Wall")

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

set(BULB_FLAGS -stdlib=libc++ -std=c++14 -Wall)
set(INCLUDE "include/bulb/")
set(OPT_INCLUDES "")
set(OPT_LIBS "")
set(FILAMENT_INCLUDE "${FILAMENT_DIR}/out/debug/filament/include/"
                     "${FILAMENT_DIR}/libs/utils/include/"
                     "${FILAMENT_DIR}/third_party/robin-map/"
                     "${FILAMENT_DIR}/libs/imageio/include")
if (WIN32)
   list(APPEND OPT_INCLUDES "include/win/")
endif()

if (NOT CMAKE_VERSION VERSION_LESS 3.7.0)
   message(STATUS "Using module to find Vulkan")
   find_package(Vulkan)
endif ()

IF (NOT Vulkan_FOUND)
   find_library(Vulkan_LIBRARY NAMES vulkan REQUIRED)
ENDIF ()
MESSAGE(STATUS "Vulkan include directory: ${Vulkan_INCLUDE_DIRS}")
MESSAGE(STATUS "Vulkan library: ${Vulkan_LIBRARIES}")

set(OpenGL_GL_PREFERENCE "GLVND")
#set(OpenGL_GL_PREFERENCE "LEGACY")
FIND_PACKAGE(OpenGL REQUIRED)
MESSAGE(STATUS "OpenGL include directory: ${OPENGL_INCLUDE_DIR}")
MESSAGE(STATUS "OpenGL library: ${OPENGL_LIBRARY}")

find_package(Threads REQUIRED)
MESSAGE(STATUS "Threads Lib: " ${CMAKE_THREAD_LIBS_INIT})
MESSAGE(STATUS "Using PTHreads?: " ${CMAKE_USE_PTHREADS_INIT})

find_package(X11 REQUIRED)
MESSAGE(STATUS "X11 Include: ${X11_INCLUDE_DIR}")
MESSAGE(STATUS "X11 Lib:     ${X11_LIBRARIES}")

find_package(LibZip)
if(LIBZIP_LIBRARY)
   list(APPEND BULB_FLAGS "-DHAVE_LIBZIP")
   list(APPEND OPT_INCLUDES "${LIBZIP_INCLUDE_DIR_ZIP}")
   list(APPEND OPT_LIBS "${LIBZIP_LIBRARY}")
   MESSAGE(STATUS "LibZip Include: ${LIBZIP_INCLUDE_DIR_ZIP}")
   MESSAGE(STATUS "LibZip Lib:     ${LIBZIP_LIBRARY}")
endif()

set(SAMPLE_FLAGS -stdlib=libc++ -std=c++17 -fstandalone-debug -Wno-unused-function)
set(SAMPLE_INCLUDES "")
set(SAMPLE_LIBS "")

find_package(SDL2)
if(SDL2_FOUND)
   list(APPEND SAMPLE_FLAGS "-DHAVE_SDL2")
   list(APPEND SAMPLE_INCLUDES "${SDL2_INCLUDE_DIR}")
   list(APPEND SAMPLE_LIBS "${SDL2_LIBRARY}")
   MESSAGE(STATUS "SDL2 Include: ${SDL2_INCLUDE_DIR}")
   MESSAGE(STATUS "SDL2 Lib:     ${SDL2_LIBRARY}")
else()
   MESSAGE(STATUS "SDL2 not found. Some samples cannot be built without SDL2 support.")
endif()

find_package(PNG)
if(PNG_FOUND)
list(APPEND SAMPLE_FLAGS "-DHAVE_LIBPNG")
list(APPEND SAMPLE_INCLUDES "${PNG_INCLUDE_DIRS}")
list(APPEND SAMPLE_LIBS "${PNG_LIBRARIES}")
MESSAGE(STATUS "libpng Include: ${PNG_INCLUDE_DIRS}")
MESSAGE(STATUS "libpng Lib:     ${PNG_LIBRARIES}")
else()
MESSAGE(STATUS "libpng not found. orbit sample can't save screenshots")
endif()


set(FILAMENT_LIBDIR "${FILAMENT_DIR}/out/debug/filament/lib/x86_64/"
                    "${FILAMENT_DIR}/out/cmake-debug/libs/gltfio/"
                    "${FILAMENT_DIR}/out/cmake-debug/libs/imageio/"
                    )
#Using imageio may require including include/exr/tinyexr.h in an implementation file to link correctly:
#define TINYEXR_IMPLEMENTATION
#include "exr/tinyexr.h"
#Filament code that requires stb image may require including stb_image in an implementation file eg:
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
set(FILAMENT_LIBS  gltfio gltfio_core gltfio_resources filament filaflat filamat shaders filabridge filameshio
                   meshoptimizer backend utils smol-v bluevk bluegl geometry
                   image imageio )

MESSAGE(STATUS "Flags:     ${BULB_FLAGS}")
add_library(bulb ${INCLUDE}/ut.hh src/ut.cc ${INCLUDE}/AssetReader.hh src/Managers.cc ${INCLUDE}/Managers.hh
            src/SceneGraph.cc ${INCLUDE}/SceneGraph.hh ${INCLUDE}/nodes/Visitor.hh src/nodes/Visitor.cc
            ${INCLUDE}/nodes/Node.hh src/nodes/Node.cc ${INCLUDE}/nodes/Composite.hh src/nodes/Composite.cc
            ${INCLUDE}/nodes/Transform.hh ${INCLUDE}/nodes/AffineTransform.hh ${INCLUDE}/nodes/CustomTransform.hh
            src/nodes/Material.cc ${INCLUDE}/nodes/Material.hh
            ${INCLUDE}/nodes/Drawable.hh src/nodes/Drawable.cc ${INCLUDE}/nodes/Geometry.hh src/nodes/Geometry.cc
            ${INCLUDE}/nodes/PositionalLight.hh src/nodes/PositionalLight.cc include/bulb/nodes/Materializable.hh
            ${INCLUDE}/nodes/MultiGeometry.hh src/nodes/MultiGeometry.cc)
target_compile_options(bulb PRIVATE ${BULB_FLAGS})
target_include_directories(bulb PRIVATE ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/include ${Vulkan_INCLUDE_DIRS}
                           ${OPENGL_INCLUDE_DIR} ${FILAMENT_INCLUDE} ${INCLUDE} ${OPT_INCLUDES})
target_link_directories(bulb PRIVATE ${FILAMENT_LIBDIR})
target_link_libraries(bulb dl Threads::Threads ${Vulkan_LIBRARIES} ${FILAMENT_LIBS} ${OPT_LIBS})
target_link_options(bulb PRIVATE -stdlib=libc++ -std=c++14)

set(SAMPLES "samples/src/")
add_executable(simpletest ${SAMPLES}/simpletest.cc ${SAMPLES}/Win.hh ${SAMPLES}/Win.cc)
target_compile_options(simpletest PRIVATE ${SAMPLE_FLAGS})
target_include_directories(simpletest PRIVATE ${PROJECT_SOURCE_DIR}/samples ${PROJECT_SOURCE_DIR}/include ${SAMPLE_INCLUDES}
                          ${X11_INCLUDE_DIR} ${FILAMENT_INCLUDE} ${INCLUDE} ${OPENGL_INCLUDE_DIR} ${Vulkan_INCLUDE_DIRS})
target_link_directories(simpletest PRIVATE ${FILAMENT_LIBDIR})
target_link_libraries(simpletest dl ${X11_LIBRARIES} ${SAMPLE_LIBS} Threads::Threads ${Vulkan_LIBRARIES} ${FILAMENT_LIBS} bulb)
target_link_options(simpletest PRIVATE -stdlib=libc++)

if(SDL2_FOUND)
   set(ORBIT_SRC "")
else()
   set(ORBIT_SRC "${SAMPLES}/Win.cc" "${SAMPLES}/Win.hh")
endif()
add_executable(orbit ${SAMPLES}/orbits.cc  ${SAMPLES}/orbitanim.hh ${SAMPLES}/orbitanim.cc ${SAMPLES}/CameraManipulator.h
${SAMPLES}/CameraManipulator.cc ${ORBIT_SRC})
target_compile_options(orbit PRIVATE ${SAMPLE_FLAGS})
target_include_directories(orbit PRIVATE ${PROJECT_SOURCE_DIR}/samples/src ${PROJECT_SOURCE_DIR}/include ${SAMPLE_INCLUDES}
                        ${X11_INCLUDE_DIR} ${FILAMENT_INCLUDE} ${INCLUDE} ${OPENGL_INCLUDE_DIR} ${Vulkan_INCLUDE_DIRS})
target_link_directories(orbit PRIVATE ${FILAMENT_LIBDIR})
target_link_libraries(orbit dl ${SAMPLE_LIBS}  ${X11_LIBRARIES} Threads::Threads ${Vulkan_LIBRARIES} ${FILAMENT_LIBS} bulb)
target_link_options(orbit PRIVATE -stdlib=libc++)

