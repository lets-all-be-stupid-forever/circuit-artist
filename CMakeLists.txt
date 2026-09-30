cmake_minimum_required(VERSION 3.5)
project(ca C CXX)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
OPTION(STEAM "Steam support")
OPTION(DEV "Development version")
OPTION(MACOS_BUNDLE "Build ca as a macOS .app bundle (for the Dock/Finder icon)")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(LIBRARY_OUTPUT_PATH "${CMAKE_BINARY_DIR}")
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}")

IF (DEV)
  add_compile_definitions(CA_DEV=1)
ENDIF()

# add_compile_definitions(GRAPHICS_API_OPENGL_43)
IF(NOT EMSCRIPTEN)
  find_package(Threads REQUIRED)
  include(SteamLib)
  include(PackagingIcons)
  include(MSVCFlags)
ENDIF()

set(FT_DISABLE_ZLIB     ON CACHE BOOL "" FORCE)
set(FT_DISABLE_PNG      ON CACHE BOOL "" FORCE)
set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BZIP2    ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BROTLI   ON CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory(third_party/raylib)
add_subdirectory(third_party/freetype)
add_subdirectory(third_party/json-c)
add_subdirectory(third_party/msgpack-c)
IF(NOT EMSCRIPTEN)
  add_subdirectory(third_party/lua)
  add_subdirectory(third_party/nativefiledialog-extended)
  add_subdirectory(third_party/discord-rpc-3.4.0)
ENDIF()

add_compile_definitions(CLIP_ENABLE_IMAGE=1)

include(MacOSSanitize)

set(CLANG_FORMAT_CMD "" CACHE STRING "" FORCE)
unset(CLANG_FORMAT_CMD CACHE)

################################
# Core
###############################
SET(ca_src
  src/buffer.c
  src/common.c
  src/colors.c
  src/dist_graph.c
  src/elmore.c
  src/event_queue.c
  src/font.c
  src/fs.cpp
  src/renderv2.c
  src/graph.c
  src/hist.c
  src/hsim.c
  src/img.c
  src/layout.c
  src/level_api.c
  src/nand_detection.c
  src/paged_cstack.c
  src/paged_stack.c
  src/paths.c
  src/pin_spec.c
  src/pixel_graph.c
  src/plot.c
  src/pq.c
  src/sound.c
  src/profiler.c
  src/rectint.c
  src/series.c
  src/shaders.c
  src/sim.c
  src/toc.c
  src/tex.c
  src/uifont.c
  src/uigraph.c
  src/utils.c
  src/wire_graph.c
  third_party/raylib/src/external/glad.h
  third_party/tinycthread/tinycthread.c
  )
add_library(ca_lib STATIC ${ca_src})
target_include_directories(ca_lib PRIVATE
  third_party
  third_party/msgpack-c/include
  third_party/raylib/src
  third_party/raylib/src/external
  third_party/freetype/include
  )
target_link_libraries(ca_lib
  raylib
  ${opengl_lib}
  msgpack-c
  json-c
  discord-rpc
  freetype)

##################################
# Desktop
##################################
SET(ca_desktop_src
  desktop/brush.c
  desktop/discord_integration.cpp
  desktop/log.c
  desktop/modal.c
  desktop/i18n.c
  desktop/msg.c
  desktop/blueprint.c
  desktop/ui.c
  desktop/levelsidebar.c
  desktop/lua_level.c
  desktop/steam.cpp
  desktop/sol_widget.c
  desktop/widgets.c
  desktop/wnumber.c
  desktop/wtext.c
  desktop/script.c
  desktop/paint.c
  desktop/compilation_thread.c
  desktop/game_registry.c
  desktop/wabout.c
  desktop/wdialog.c
  desktop/win_wiki.c
  desktop/win_mtext.c
  desktop/win_level.c
  desktop/win_workshop.c
  desktop/win_workshopdet.c
  desktop/win_msg.c
  desktop/win_blueprint.c
  desktop/win_pubform.c
  desktop/win_campaign.c
  desktop/win_customlvl.c
  desktop/win_progress.c
  desktop/win_bpdetail.c
  desktop/win_settings.c
  desktop/win_main.c
  desktop/clipapi.cpp
  third_party/clip/clip.cpp
  third_party/clip/image.cpp
)

IF (WIN32)
  SET(ca_desktop_src ${ca_desktop_src}
    third_party/clip/clip_win.cpp
    third_party/clip/clip_win_wic.cpp
    third_party/clip/clip_win_bmp.cpp
  )
ELSEIF (APPLE)
  SET(ca_desktop_src ${ca_desktop_src}
    third_party/clip/clip_osx.mm
  )
ELSEIF (UNIX)
  ADD_COMPILE_DEFINITIONS(HAVE_XCB_XLIB_H=1)
  SET(clip_libs xcb pthread)
  SET(ca_desktop_src ${ca_desktop_src}
    third_party/clip/clip_x11.cpp
  )
ENDIF()
# shlwapi is only for win32
IF(WIN32)
  SET(shlwapi_lib shlwapi)
    SET(opengl_lib opengl32)
ENDIF()
add_library(ca_desktop STATIC ${ca_desktop_src})
target_include_directories(ca_desktop PRIVATE
  src
  desktop
  third_party
  third_party/clip
  third_party/tinycthread
  third_party/lua
  third_party/nativefiledialog-extended/src/include
  third_party/msgpack-c/include
  third_party/raylib/src
  third_party/raylib/src/external
  third_party/discord-rpc-3.4.0/include
  third_party/freetype/include
  )
target_link_libraries(ca_desktop
  ca_lib
  ${shlwapi_lib}
  nfd
  ${clip_libs}
  ${steam_libs}
  lualib
  discord-rpc
  Threads::Threads)


add_executable(ca desktop/main.c ${ca_icon_src})
target_link_libraries(ca ca_desktop)
target_include_directories(ca PRIVATE src)
# Set RPATH so Linux executables find shared libraries in their own directory
if(UNIX AND NOT APPLE)
    set_target_properties(ca PROPERTIES
        BUILD_RPATH "\$ORIGIN"
        INSTALL_RPATH "\$ORIGIN"
    )
endif()

##################################
include(MacOSBundle)
include(SteamBinariesCopy)
