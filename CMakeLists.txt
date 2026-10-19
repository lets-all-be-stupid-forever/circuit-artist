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

add_compile_definitions(RL_DEFAULT_BATCH_MAX_TEXTURE_UNITS=8)

IF(NOT EMSCRIPTEN)
  add_compile_definitions(CA_DESKTOP=1)
ELSE()
  add_compile_definitions(CA_WEB=1)
ENDIF()

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

IF (EMSCRIPTEN)
  # raylib's src/CMakeLists.txt does:
  #   target_link_options(raylib PUBLIC "-sUSE_GLFW=3" -sEXPORTED_RUNTIME_METHODS=ccall)
  # Being PUBLIC, that propagates into every target linking raylib, and CMake
  # appends propagated interface options *after* the target's own ones. Since
  # emscripten takes the last `-s<NAME>=` wins for a given setting, raylib's
  # bare `ccall` silently clobbered our list and Module.cwrap was undefined at
  # runtime. Drop it from the interface; we set the full list on ca_wasm below.
  get_target_property(_rl_iface_link_opts raylib INTERFACE_LINK_OPTIONS)
  IF (_rl_iface_link_opts)
    list(REMOVE_ITEM _rl_iface_link_opts "-sEXPORTED_RUNTIME_METHODS=ccall")
    set_target_properties(raylib PROPERTIES
      INTERFACE_LINK_OPTIONS "${_rl_iface_link_opts}")
  ENDIF()
ENDIF()
add_subdirectory(third_party/freetype)
add_subdirectory(third_party/json-c)
add_subdirectory(third_party/msgpack-c)
add_subdirectory(third_party/lua)
IF(NOT EMSCRIPTEN)
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
  src/brush.c
  src/discord_integration.cpp
  src/log.c
  src/i18n.c
  src/msg.c
  src/blueprint.c
  src/ui.c
  src/levelsidebar.c
  src/lua_level.c
  src/steam.cpp
  src/sol_widget.c
  src/widgets.c
  src/wnumber.c
  src/wtext.c
  src/script.c
  src/paint.c
  src/game_registry.c
  src/wabout.c
  src/wdialog.c
  src/win_wiki.c
  src/win_mtext.c
  src/win_level.c
  src/win_workshop.c
  src/win_workshopdet.c
  src/win_msg.c
  src/win_blueprint.c
  src/win_pubform.c
  src/win_campaign.c
  src/win_clipboard.c
  src/win_customlvl.c
  src/win_progress.c
  src/win_bpdetail.c
  src/win_settings.c
  src/win_main.c
)

IF(NOT EMSCRIPTEN)
  set(ca_src
    ${ca_src}
    src/modal.c
    src/compilation_thread.c
    src/clipapi.cpp
    third_party/clip/clip.cpp
    third_party/clip/image.cpp
  )
ELSE()
  set(ca_src
    ${ca_src}
    src/compilation_thread_naive.c
  )
ENDIF()

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
  SET(ca_desktop_src ${ca_src}
    third_party/clip/clip_x11.cpp
  )
ENDIF()
# shlwapi is only for win32
IF(WIN32)
  SET(shlwapi_lib shlwapi)
  SET(opengl_lib opengl32)
ENDIF()

IF(NOT EMSCRIPTEN)
  # --- desktop only --
  set(ca_desktop_includes
    third_party/nativefiledialog-extended/src/include
    third_party/tinycthread
    third_party/clip
    third_party/discord-rpc-3.4.0/include
  )
  set(ca_desktop_libs
    nfd
    ${shlwapi_lib}
    ${clip_libs}
    ${steam_libs}
    discord-rpc
    Threads::Threads
  )
ENDIF()

add_library(ca_lib STATIC ${ca_src})
set(ca_lib_includes
  src
  third_party
  third_party/raylib/src
  third_party/raylib/src/external
  third_party/freetype/include
  third_party/lua
  third_party/msgpack-c/include
  third_party/freetype/include
  third_party/msgpack-c/include
  ${ca_desktop_includes}
)
target_include_directories(ca_lib PRIVATE ${ca_lib_includes})
target_link_libraries(ca_lib
  raylib
  ${opengl_lib}
  msgpack-c
  json-c
  freetype
  lualib
  ${ca_desktop_libs}
)

##################################
# Desktop
##################################

IF (NOT EMSCRIPTEN)
  add_executable(ca src/main.c ${ca_icon_src})
  target_link_libraries(ca ca_lib)
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
include(MacOSBundle)
include(SteamBinariesCopy)


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
  # tools/stage_wasm_assets.py copies assets/ into the build tree and rewrites
  # only the staged copy's shaders (it also subsets the UI font). assets/ on
  # disk stays desktop-only.
  #
  # The .data bundle is produced by the *link* step, so the link must be
  # re-run whenever anything under assets/ changes (not just when a .c file
  # does). A stamp file regenerated from the asset list carries that
  # dependency into LINK_DEPENDS.
  # The demo is served by GitHub Pages straight from the repo root:
  # index.html at the root, everything else under static/. ca_wasm is linked
  # as a plain .js (no --shell-file); emscripten's loader resolves .wasm and
  # .data relative to its own script URL, so they are found in static/ too.
  # The shell already references static/ca_wasm.js, so it is copied verbatim.
  # COPYONLY registers it as a configure-time dependency, so editing it
  # re-copies on the next build.
  set(wasm_shell "${CMAKE_SOURCE_DIR}/src/index_template.html")
  set(wasm_out_dir "${CMAKE_SOURCE_DIR}/static")
  configure_file("${wasm_shell}" "${CMAKE_SOURCE_DIR}/index.html" COPYONLY)

  # The UI webfont is fetched over HTTP by @font-face in the shell. CSS cannot
  # read MEMFS, so being inside the --preload-file bundle is not enough: the
  # file has to be served as-is from static/.
  # Generated from assets/font/*.bdf by tools/bdf2woff2.py.
  configure_file("${CMAKE_SOURCE_DIR}/assets/font/ca-ui-font.woff2"
                 "${wasm_out_dir}/ca-ui-font.woff2" COPYONLY)
  # Page favicon, referenced by the shell as static/favicon.png.
  configure_file("${CMAKE_SOURCE_DIR}/assets/imgs/icon32.png"
                 "${wasm_out_dir}/favicon.png" COPYONLY)
  set(wasm_assets_dir "${CMAKE_BINARY_DIR}/wasm_assets")
  set(wasm_assets_stamp "${CMAKE_BINARY_DIR}/wasm_assets.stamp")
  file(GLOB_RECURSE wasm_asset_srcs CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/assets/*")
  find_package(Python3 REQUIRED COMPONENTS Interpreter)
  set(wasm_stage_script "${CMAKE_SOURCE_DIR}/tools/stage_wasm_assets.py")
  add_custom_command(
    OUTPUT "${wasm_assets_stamp}"
    COMMAND ${Python3_EXECUTABLE} "${wasm_stage_script}"
      "${CMAKE_SOURCE_DIR}/assets" "${wasm_assets_dir}"
    COMMAND ${CMAKE_COMMAND} -E touch "${wasm_assets_stamp}"
    DEPENDS ${wasm_asset_srcs} "${wasm_stage_script}"
    COMMENT "Staging assets/ for wasm (rewriting shaders to GLSL ES 300)"
  )
  add_custom_target(wasm_assets ALL DEPENDS "${wasm_assets_stamp}")

  add_executable(ca_wasm
    src/main.c
  )
  add_dependencies(ca_wasm wasm_assets)
  set_target_properties(ca_wasm PROPERTIES
    LINK_DEPENDS "${wasm_assets_stamp}")
  target_include_directories(ca_wasm PRIVATE ${ca_lib_includes})
  target_link_libraries(ca_wasm ca_lib)

  set_target_properties(ca_wasm PROPERTIES
    SUFFIX ".js"
    RUNTIME_OUTPUT_DIRECTORY "${wasm_out_dir}")
  # -DWASM_GL_DEBUG=ON makes every WebGL error print the GL call that caused
  # it (plus a JS stack) in the browser console. Slow; for debugging only.
  option(WASM_GL_DEBUG "Trace WebGL errors per GL call in the wasm build" OFF)
  if (WASM_GL_DEBUG)
    # -g2 keeps the wasm name section so the console shows C function names
    # instead of $funcNNNN, which is what makes a GL_DEBUG trace actionable.
    target_link_options(ca_wasm PRIVATE "-sGL_DEBUG=1" "-sGL_ASSERTIONS=1"
                        "-sASSERTIONS=1" "-g2")
    target_compile_options(ca_wasm PRIVATE "-g2")
  endif()
  target_link_options(ca_wasm PRIVATE
    "-sUSE_GLFW=3"
    "-sASYNCIFY"
    "-sALLOW_MEMORY_GROWTH=1"
   "-sEXPORTED_RUNTIME_METHODS=ccall,cwrap,UTF8ToString,stringToUTF8,lengthBytesUTF8,HEAPU8,FS"
   "-sEXPORTED_FUNCTIONS=_main,_malloc,_free"
    "--preload-file=${wasm_assets_dir}@/assets"
  )
  # "-sEXPORTED_FUNCTIONS=_main,_malloc,_free,_ca_get_tick,_ca_toggle_pause,_ca_is_paused"
  return()
ENDIF()
