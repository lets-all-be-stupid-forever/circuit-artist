# macOS .app bundle: gives the game a Dock/Finder icon. The bundle is meant to
# be shipped as bin/ca.app next to assets/ (same layout as the bare binary);
# paths.c resolves assets relative to the bundle when CA_MACOS_BUNDLE is set.
IF (APPLE AND MACOS_BUNDLE)
  FOREACH (tgt ca)
    set_target_properties(${tgt} PROPERTIES
      MACOSX_BUNDLE TRUE
      MACOSX_BUNDLE_BUNDLE_NAME "Circuit Artist"
      MACOSX_BUNDLE_GUI_IDENTIFIER "com.circuitartist.${tgt}"
      MACOSX_BUNDLE_ICON_FILE ca.icns
      MACOSX_BUNDLE_HIGH_RESOLUTION_CAPABLE TRUE)
    target_compile_definitions(${tgt} PRIVATE CA_MACOS_BUNDLE=1)
  ENDFOREACH()
  target_compile_definitions(ca_lib PRIVATE CA_MACOS_BUNDLE=1)
ENDIF()
