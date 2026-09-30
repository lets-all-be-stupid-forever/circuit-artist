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
ENDIF()

