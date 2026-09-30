# Configure raylib to use OpenGL and include GLAD
# Enable OpenGL 4.3 for compute shader support
if(MSVC)
  add_compile_options(/wd5105)
  # add_compile_definitions(GRAPHICS_API_OPENGL_43)
  # add_compile_options(/bigobj)
  # Disable annoying warnings such as conversion from 'double' to 'float', possible loss of
  add_compile_options(/wd4244)
endif()

