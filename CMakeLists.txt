cmake_minimum_required(VERSION 3.5)
project(ca C CXX)
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)

find_package(OpenMP)

set(LIBRARY_OUTPUT_PATH "${CMAKE_BINARY_DIR}")
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}")

SET(BUILD_SHARED_LIBS ON CACHE BOOL "Build raylib shared library")

add_subdirectory(third_party/raylib)
add_subdirectory(third_party/nativefiledialog-extended)
link_directories(LuaJIT/src)

add_compile_definitions(CLIP_ENABLE_IMAGE 1)

if (OPENMP_FOUND)
  set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
  set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
ENDIF()

add_executable(ca
  src/main.c
  src/brush.c
  src/paint.c
  src/filedialog.c
  src/colors.c
  src/tiling.c
  src/msg.c
  src/minimap.c
  src/api.c
  src/font.c
  src/rect_int.c
  src/rendering.c
  src/pyramid.c
  src/widgets.c
  src/sim.c
  src/utils.c
  src/img.c
  src/hist.c
  src/ui.c
  src/w_about.c
  src/w_main.c
  src/w_levels.c
  src/w_text.c
  src/w_cpedia.c
  src/w_dialog.c
  src/clip_api.cpp

  # Stuff for copy-paste
  third_party/clip/image.cpp
  third_party/clip/clip.cpp
  third_party/clip/clip_win.cpp
  third_party/clip/clip_win_wic.cpp
  third_party/clip/clip_win_bmp.cpp
  )

target_include_directories(ca PRIVATE
  src
  third_party
  third_party/clip
  third_party/nativefiledialog-extended/src/include
  LuaJIT/src
  )

# shlwapi is only for win32
target_link_libraries(ca raylib shlwapi nfd OpenMP::OpenMP_C lua51 luajit)
