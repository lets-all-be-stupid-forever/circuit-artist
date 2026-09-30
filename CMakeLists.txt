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

IF (EMSCRIPTEN)
  # This repo's rlgl.h uses MRT calls (glDrawBuffers, GL_COLOR_ATTACHMENTn)
  # unconditionally, which don't exist in WebGL1/GLES2. Force WebGL2/GLES3.
  set(GRAPHICS "GRAPHICS_API_OPENGL_ES3")
ENDIF()

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
  src/cam2d.c
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
set(ca_lib_includes
  src
  third_party
  third_party/raylib/src
  third_party/msgpack-c/include
  third_party/raylib/src/external
  third_party/freetype/include
)
target_include_directories(ca_lib PRIVATE ${ca_lib_includes})
target_link_libraries(ca_lib
  raylib
  ${opengl_lib}
  msgpack-c
  json-c
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

IF (NOT EMSCRIPTEN)
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

ENDIF()
##################################
# Player
##################################
SET(ca_player_src
  player/main.c
  player/player.c
)

add_library(ca_player STATIC ${ca_player_src})
target_include_directories(ca_player PRIVATE player ${ca_lib_includes})
target_link_libraries(ca_player ca_lib ${shlwapi_lib})

add_executable(cap player/main.c)
target_link_libraries(cap ca_player)

IF (EMSCRIPTEN)
  # WASM viewer: exactly the same calib + ca_viewer core and the same
  # src/main_viewer.c entry point as the native "cav" viewer (main_viewer.c
  # picks the right frame-loop strategy internally via PLATFORM_WEB). assets/
  # is baked into MEMFS at build time via --preload-file, so
  # LoadImage("../assets/example.png") resolves the same way it does on
  # desktop (cwd is "/", and ".." above the MEMFS root clamps back to "/",
  # landing on "/assets/example.png").
  #
  # Desktop GL (ca/cav) needs "#version 330" shaders; WebGL2/GLES3 needs
  # "#version 300 es" + an explicit precision qualifier. Rather than forking
  # assets/shaders/*.glsl (which both platforms load from the same path),
  # stage_wasm_assets.sh copies assets/ into the build tree and rewrites only
  # the staged copy's shaders. assets/shaders/ on disk stays desktop-only.
  #
  # The .data bundle is produced by the *link* step, so the link must be
  # re-run whenever anything under assets/ changes (not just when a .c file
  # does). A stamp file regenerated from the asset list carries that
  # dependency into LINK_DEPENDS.
  set(wasm_assets_dir "${CMAKE_BINARY_DIR}/wasm_assets")
  set(wasm_assets_stamp "${CMAKE_BINARY_DIR}/wasm_assets.stamp")
  file(GLOB_RECURSE wasm_asset_srcs CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/assets/*")
  add_custom_command(
    OUTPUT "${wasm_assets_stamp}"
    COMMAND bash "${CMAKE_SOURCE_DIR}/stage_wasm_assets.sh"
      "${CMAKE_SOURCE_DIR}/assets" "${wasm_assets_dir}"
    COMMAND ${CMAKE_COMMAND} -E touch "${wasm_assets_stamp}"
    DEPENDS ${wasm_asset_srcs} "${CMAKE_SOURCE_DIR}/stage_wasm_assets.sh"
    COMMENT "Staging assets/ for wasm (rewriting shaders to GLSL ES 300)"
  )
  add_custom_target(wasm_assets ALL DEPENDS "${wasm_assets_stamp}")

  add_executable(ca_wasm player/main.c)
  add_dependencies(ca_wasm wasm_assets)
  set_target_properties(ca_wasm PROPERTIES LINK_DEPENDS "${wasm_assets_stamp}")
  target_include_directories(ca_wasm PRIVATE ${calib_includes})
  target_link_libraries(ca_wasm ca_player)

  set_target_properties(ca_wasm PROPERTIES SUFFIX ".html")
  # -DWASM_GL_DEBUG=ON makes every WebGL error print the GL call that caused
  # it (plus a JS stack) in the browser console. Slow; for debugging only.
  option(WASM_GL_DEBUG "Trace WebGL errors per GL call in the wasm build" OFF)
  if (WASM_GL_DEBUG)
    target_link_options(ca_wasm PRIVATE "-sGL_DEBUG=1" "-sGL_ASSERTIONS=1" "-sASSERTIONS=1")
  endif()
  target_link_options(ca_wasm PRIVATE
    "-sUSE_GLFW=3"
    "-sASYNCIFY"
    "-sALLOW_MEMORY_GROWTH=1"
    "--shell-file=${CMAKE_SOURCE_DIR}/player/wasm_shell.html"
    "--preload-file=${wasm_assets_dir}@/assets"
  )

  return()
ENDIF()





##################################
include(MacOSBundle)
include(SteamBinariesCopy)
