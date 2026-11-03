{
  'target_defaults': {
    'cflags_cc': ['-std=c++17'],
    'xcode_settings': {
      'CLANG_CXX_LANGUAGE_STANDARD': 'c++17',
    },
    'msvs_settings': {
      'VCCLCompilerTool': {
        'AdditionalOptions': ['/std:c++17'],
        'ExceptionHandling': '0',
      },
    },
    'configurations': {
      'Debug': {
        'defines': [
          'DEBUG',
          '_DEBUG',
        ],
      },
      'Release': {
        'defines': [
          'NDEBUG',
        ],
      },
    },
  }
}
