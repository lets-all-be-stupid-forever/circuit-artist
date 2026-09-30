# Application icons live in packaging/. Regenerate with packaging/make_icons.py
IF (WIN32)
  enable_language(RC)
  SET(ca_icon_src packaging/windows/ca.rc)
ELSEIF (APPLE AND MACOS_BUNDLE)
  SET(ca_icon_src packaging/macos/ca.icns)
  set_source_files_properties(packaging/macos/ca.icns PROPERTIES
    MACOSX_PACKAGE_LOCATION Resources)
ENDIF()

