import os
import ycm_core

flags = [
  '-Wall',
  '-Wextra',
  '-Werror',
  '-Wno-long-long',
  '-Wno-variadic-macros',
  '-fexceptions',
  '-ferror-limit=10000',
  '-DNDEBUG',
  '-std=c99',
  '-xc',
  '-I../third_party/raylib/src',
  '-I../third_party/nativefiledialog-extended/src/include',
  '-I../third_party/clip',
  '-I../LuaJIT/src',
  '-I../third_party',
  '-isystem/usr/include/',
]

SOURCE_EXTENSIONS = [ '.cpp', '.cxx', '.cc', '.c', ]

def FlagsForFile( filename, **kwargs ):
    return {
      'flags': flags,
      'do_cache': True
    }
