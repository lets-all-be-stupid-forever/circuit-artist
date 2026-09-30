IF(APPLE AND CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_compile_options(-fsanitize=address)
  add_link_options(-fsanitize=address)
ENDIF()

