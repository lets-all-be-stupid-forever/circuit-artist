cmake_minimum_required(VERSION 3.5)
project(ca C CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)
OPTION(OPENMP "Use OpenMP for thread detection")
OPTION(STEAM "Steam support")

IF (OPENMP)
  find_package(OpenMP REQUIRED)
  add_compile_definitions(WITH_OPENMP=1)
ENDIF()

set(LIBRARY_OUTPUT_PATH "${CMAKE_BINARY_DIR}")
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}")

SET(BUILD_SHARED_LIBS ON CACHE BOOL "Build raylib shared library")

add_subdirectory(third_party/raylib)
add_subdirectory(third_party/nativefiledialog-extended)
add_compile_options(-fsanitize=address)
add_link_options(-fsanitize=address)

# there's also a "rolling" tag but that jams up any sha256 effort
SET(LUAJIT_VERSION 2.1.0-beta3)
SET(LUAJIT_SHA256 409f7fe570d3c16558e594421c47bdd130238323c9d6fd6c83dedd2aaeb082a8)
IF (NOT IS_DIRECTORY ${CMAKE_SOURCE_DIR}/LuaJIT/src)
    FILE(DOWNLOAD
            https://github.com/LuaJIT/LuaJIT/archive/v${LUAJIT_VERSION}.tar.gz
            ${CMAKE_SOURCE_DIR}/third_party/LuaJIT-${LUAJIT_VERSION}.tar.gz
            STATUS luajit_fetch
            EXPECTED_HASH SHA256=${LUAJIT_SHA256}
    )
    LIST(GET luajit_fetch 0 luajit_fetch_status)
    IF (NOT ${luajit_fetch_status} EQUAL 0)
        message(FATAL_ERROR "LuaJIT fetch did not shake out := ${luajit_fetch}")
    ENDIF()
    FILE(ARCHIVE_EXTRACT
            INPUT ${CMAKE_SOURCE_DIR}/third_party/LuaJIT-${LUAJIT_VERSION}.tar.gz
            DESTINATION ${CMAKE_SOURCE_DIR}/third_party/.luajit
    )
    FILE(RENAME
            ${CMAKE_SOURCE_DIR}/third_party/.luajit/LuaJIT-${LUAJIT_VERSION}
            ${CMAKE_SOURCE_DIR}/LuaJIT
    )
    FILE(REMOVE_RECURSE ${CMAKE_SOURCE_DIR}/third_party/.luajit)
ENDIF()
IF(NOT EXISTS ${CMAKE_SOURCE_DIR}/LuaJIT/src/lua.h)
    message(FATAL_ERROR "The LuaJIT unpack did not shake out, lua.h is missing")
ENDIF()

add_custom_command(
  # in my tarball, it always generates .so but we'll need to double check what happens on (WIN32)
  OUTPUT ${CMAKE_SOURCE_DIR}/LuaJIT/src/libluajit.so
  # that nonsense is because it does actually build the .so
  # but barfs linking luajit binary due to dwarf symbols
  COMMAND make --ignore-errors
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/LuaJIT
  VERBATIM
)
link_directories(LuaJIT/src)

IF (STEAM)
    link_directories(third_party/redistributable_bin/win64/)
    SET(steam_libs steam_api64)
    add_compile_definitions(WITH_STEAM=1)
ENDIF()


add_compile_definitions(CLIP_ENABLE_IMAGE=1)

SET(ca_src
  src/brush.c
  src/paint.c
  src/filedialog.c
  src/colors.c
  src/loop_detector.c
  src/tiling.c
  src/msg.c
  src/api.c
  src/font.c
  src/rect_int.c
  src/rendering.c
  src/widgets.c
  src/sim.c
  src/utils.c
  src/img.c
  src/hist.c
  src/ui.c
  src/w_about.c
  src/w_main.c
  src/w_number.c
  src/w_levels.c
  src/w_text.c
  src/w_tutorial.c
  src/w_dialog.c
  src/profiler.c
  src/shaders.c
  src/clip_api.cpp
  src/steam.cpp

  # Stuff for copy-paste
  third_party/clip/image.cpp
  third_party/clip/clip.cpp
  )
IF (WIN32)
  SET(ca_src ${ca_src}
    third_party/clip/clip_win.cpp
    third_party/clip/clip_win_wic.cpp
    third_party/clip/clip_win_bmp.cpp
  )
ELSEIF (APPLE)
  SET(ca_src ${ca_src}
    third_party/clip/clip_osx.mm
  )
ELSEIF (UNIX)
  ADD_COMPILE_DEFINITIONS(HAVE_XCB_XLIB_H=1)
  SET(clip_libs xcb pthread)
  SET(ca_src ${ca_src}
    third_party/clip/clip_x11.cpp
  )
ENDIF()

add_library(calib STATIC ${ca_src})

target_include_directories(calib PRIVATE
  src
  third_party
  third_party/clip
  third_party/nativefiledialog-extended/src/include
  LuaJIT/src
  )

# shlwapi is only for win32
IF(WIN32)
  SET(shlwapi_lib shlwapi)
  # mingw32-make cooks lua51.dll for some reason :-(
  SET(luajit_lib lua51)
ELSE ()
  SET(luajit_lib luajit)
ENDIF()
IF (OPENMP)
  SET(openmp_lib OpenMP::OpenMP_C OpenMP::OpenMP_CXX)
ENDIF ()
target_link_libraries(calib raylib ${shlwapi_lib} nfd ${openmp_lib} ${luajit_lib} ${clip_libs} ${steam_libs})

add_executable(ca src/main.c)
target_link_libraries(ca calib)

IF (UNIX AND NOT APPLE)
  # Need this command so luajit can access the C API from ffi
  SET_PROPERTY(TARGET ca PROPERTY ENABLE_EXPORTS ON)
ENDIF()

# In Windows I create 2 executables: one without the console window (ca.exe)
# and one with the console window (ca_console.exe).
# The idea is that one would use ca_console.exe on windows to develop lua
# scripts to see logs and error messages, while the default version is used as
# main entrypoint of steam.
IF (WIN32)
  add_executable(ca_console src/main.c)
  target_link_libraries(ca_console calib)
  target_compile_definitions(ca_console PRIVATE CA_SHOW_CONSOLE=1)

  # Demo version
  add_executable(ca_demo src/main.c)
  target_link_libraries(ca_demo calib)
  target_compile_definitions(ca_demo PRIVATE DEMO_VERSION=1)

  add_custom_target(ca_all DEPENDS ca ca_console ca_demo)
ENDIF()

add_executable(tmp src/tmp.c)
target_link_libraries(tmp raylib)
