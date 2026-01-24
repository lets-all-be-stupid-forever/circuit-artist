cmake_minimum_required(VERSION 3.5)
project(ca C CXX)
OPTION(STEAM "Steam support")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(LIBRARY_OUTPUT_PATH "${CMAKE_BINARY_DIR}")
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}")

# Configure raylib to use OpenGL and include GLAD
# Enable OpenGL 4.3 for compute shader support
if(MSVC)
  add_compile_options(/wd5105)
  # add_compile_definitions(GRAPHICS_API_OPENGL_43)
  # add_compile_options(/bigobj)
  # Disable annoying warnings such as conversion from 'double' to 'float', possible loss of
  add_compile_options(/wd4244)
endif()

IF (STEAM)
  include_directories(${CMAKE_SOURCE_DIR}/third_party/steam/)
  add_compile_definitions(WITH_STEAM=1)

  IF (WIN32)
    link_directories(${CMAKE_SOURCE_DIR}/third_party/redistributable_bin/win64/)
    SET(steam_libs steam_api64)
  ELSEIF (APPLE)
    link_directories(${CMAKE_SOURCE_DIR}/third_party/redistributable_bin/osx/)
    SET(steam_libs steam_api)
  ELSEIF (UNIX AND NOT APPLE)
    IF (CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
      link_directories(${CMAKE_SOURCE_DIR}/third_party/redistributable_bin/linuxarm64/)
    ELSEIF (CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
      link_directories(${CMAKE_SOURCE_DIR}/third_party/redistributable_bin/linux64/)
    ELSE()
      link_directories(${CMAKE_SOURCE_DIR}/third_party/redistributable_bin/linux32/)
    ENDIF()
    SET(steam_libs steam_api)
  ENDIF()
ENDIF()

# add_compile_definitions(GRAPHICS_API_OPENGL_43)
add_subdirectory(third_party/raylib)
add_subdirectory(third_party/nativefiledialog-extended)
add_subdirectory(third_party/lua)
add_subdirectory(third_party/json-c)
add_subdirectory(third_party/msgpack-c)
add_compile_definitions(CLIP_ENABLE_IMAGE=1)

IF(APPLE AND CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_compile_options(-fsanitize=address)
  add_link_options(-fsanitize=address)
ENDIF()

set(CMAKE_C_STANDARD 11)

SET(ca_src
  src/brush.c
  src/buffer.c
  src/clipapi.cpp
  src/colors.c
  src/connected_components.c
  src/dist_graph.c
  src/elmore.c
  src/event_queue.c
  src/font.c
  src/fs.cpp
  src/renderv2.c
  src/game_registry.c
  src/graph.c
  src/hist.c
  src/hsim.c
  src/img.c
  src/lua_level.c
  src/level_api.c
  src/log.c
  src/modal.c
  src/msg.c
  src/nand_detection.c
  src/paged_cstack.c
  src/paged_stack.c
  src/paint.c
  src/pin_spec.c
  src/pixel_graph.c
  src/plot.c
  src/pq.c
  src/profiler.c
  src/rectint.c
  src/series.c
  src/shaders.c
  src/sim.c
  src/steam.cpp
  src/tex.c
  src/tutorial.c
  src/ui.c
  src/uigraph.c
  src/utils.c
  src/wabout.c
  src/wdialog.c
  src/widgets.c
  src/win_blueprint.c
  src/win_campaign.c
  src/win_level.c
  src/win_msg.c
  src/win_stamp.c
  src/wire_graph.c
  src/wmain.c
  src/wnumber.c
  src/wtext.c
  third_party/clip/clip.cpp
  third_party/clip/image.cpp
  third_party/raylib/src/external/glad.h
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
  third_party/lua
  third_party/nativefiledialog-extended/src/include
  third_party/msgpack-c/include
  third_party/raylib/src
  third_party/raylib/src/external
  )

# shlwapi is only for win32
IF(WIN32)
  SET(shlwapi_lib shlwapi)
    SET(opengl_lib opengl32)
ENDIF()

target_link_libraries(calib
  raylib
  ${opengl_lib}
  ${shlwapi_lib}
  nfd
  ${clip_libs}
  ${steam_libs}
  lualib
  msgpack-c
  json-c)

add_executable(ca src/main.c)
target_link_libraries(ca calib)

# Set RPATH so Linux executables find shared libraries in their own directory
if(UNIX AND NOT APPLE)
    set_target_properties(ca PROPERTIES
        BUILD_RPATH "\$ORIGIN"
        INSTALL_RPATH "\$ORIGIN"
    )
endif()

# Create demo library with DEMO_VERSION enabled
add_library(calib_demo STATIC ${ca_src})

target_include_directories(calib_demo PRIVATE
  src
  third_party
  third_party/clip
  third_party/lua
  third_party/nativefiledialog-extended/src/include
  third_party/msgpack-c/include
  third_party/raylib/src
  third_party/raylib/src/external
  )

target_compile_definitions(calib_demo PRIVATE DEMO_VERSION=1)

target_link_libraries(calib_demo
  raylib
  ${opengl_lib}
  ${shlwapi_lib}
  nfd
  ${clip_libs}
  ${steam_libs}
  lualib
  msgpack-c
  json-c)

# Demo executable
add_executable(ca_demo src/main.c)
target_link_libraries(ca_demo calib_demo)

# Set RPATH for demo executable
if(UNIX AND NOT APPLE)
    set_target_properties(ca_demo PROPERTIES
        BUILD_RPATH "\$ORIGIN"
        INSTALL_RPATH "\$ORIGIN"
    )
endif()

# Copy Steam library and app ID file to build directory for runtime
IF (STEAM)
  IF (WIN32)
    SET(steam_binary "${CMAKE_SOURCE_DIR}/third_party/redistributable_bin/win64/steam_api64.dll")
  ELSEIF (APPLE)
    SET(steam_binary "${CMAKE_SOURCE_DIR}/third_party/redistributable_bin/osx/libsteam_api.dylib")
  ELSEIF (UNIX AND NOT APPLE)
    IF (CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
      SET(steam_binary "${CMAKE_SOURCE_DIR}/third_party/redistributable_bin/linuxarm64/libsteam_api.so")
    ELSEIF (CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
      SET(steam_binary "${CMAKE_SOURCE_DIR}/third_party/redistributable_bin/linux64/libsteam_api.so")
    ELSE()
      SET(steam_binary "${CMAKE_SOURCE_DIR}/third_party/redistributable_bin/linux32/libsteam_api.so")
    ENDIF()
  ENDIF()

  add_custom_command(TARGET ca POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "${steam_binary}"
      "$<TARGET_FILE_DIR:ca>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "${CMAKE_SOURCE_DIR}/steam_appid.txt"
      "$<TARGET_FILE_DIR:ca>")
  add_custom_command(TARGET ca_demo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "${steam_binary}"
      "$<TARGET_FILE_DIR:ca_demo>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "${CMAKE_SOURCE_DIR}/steam_appid.txt"
      "$<TARGET_FILE_DIR:ca_demo>")
ENDIF()
