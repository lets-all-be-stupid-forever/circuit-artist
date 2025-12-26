cmake_minimum_required(VERSION 3.5)
project(lua C)

# Lua core source files
set(LUA_CORE_SRC
  ${CMAKE_CURRENT_SOURCE_DIR}/lapi.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lcode.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lctype.c
  ${CMAKE_CURRENT_SOURCE_DIR}/ldebug.c
  ${CMAKE_CURRENT_SOURCE_DIR}/ldo.c
  ${CMAKE_CURRENT_SOURCE_DIR}/ldump.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lfunc.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lgc.c
  ${CMAKE_CURRENT_SOURCE_DIR}/llex.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lmem.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lobject.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lopcodes.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lparser.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lstate.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lstring.c
  ${CMAKE_CURRENT_SOURCE_DIR}/ltable.c
  ${CMAKE_CURRENT_SOURCE_DIR}/ltm.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lundump.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lvm.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lzio.c
)

# Lua library source files
set(LUA_LIB_SRC
  ${CMAKE_CURRENT_SOURCE_DIR}/lauxlib.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lbaselib.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lcorolib.c
  ${CMAKE_CURRENT_SOURCE_DIR}/ldblib.c
  ${CMAKE_CURRENT_SOURCE_DIR}/liolib.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lmathlib.c
  ${CMAKE_CURRENT_SOURCE_DIR}/loadlib.c
  ${CMAKE_CURRENT_SOURCE_DIR}/loslib.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lstrlib.c
  ${CMAKE_CURRENT_SOURCE_DIR}/ltablib.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lutf8lib.c
  ${CMAKE_CURRENT_SOURCE_DIR}/linit.c
)

# Build Lua as a static library
add_library(lualib STATIC ${LUA_CORE_SRC} ${LUA_LIB_SRC})

# Platform-specific settings
if(UNIX)
  target_compile_definitions(lualib PRIVATE LUA_USE_POSIX)
  target_link_libraries(lualib m dl)
endif()

if(APPLE)
  target_compile_definitions(lualib PRIVATE LUA_USE_MACOSX)
endif()

if(UNIX AND NOT APPLE)
  target_compile_definitions(lualib PRIVATE LUA_USE_LINUX)
endif()

# Include directory for Lua headers
target_include_directories(lualib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Set C standard
set_property(TARGET lualib PROPERTY C_STANDARD 99)
