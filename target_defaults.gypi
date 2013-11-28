{
  'target_defaults': {
    'config_conditions': [
      ['_bitness==64', {
        'msvs_configuration_platform': 'x64',
      }],
      ['_bitness==32', {
        'msvs_configuration_platform': 'Win32',
      }],
      ['_debugging==0', {
        'msvs_settings': {
          'VCCLCompilerTool': {
            'RuntimeLibrary': '2',
          },
        },
      }],
      ['_debugging==1', {
        'msvs_settings': {
          'VCCLCompilerTool': {
            'RuntimeLibrary': '3',
            'Optimization': '0',
          },
          'VCLinkerTool': {
            'GenerateDebugInformation': 'true',
          },
        },
      }],
     ],
    'configurations': {
      'Debug64': {
        'defines': [
          'DEBUG',
          '_DEBUG',
        ],
        'debugging': "1",
        'bitness': "64",
      },
      'Release64': {
        'defines': [
          'NDEBUG',
        ],
        'debugging': "0",
        'bitness': "64",
      },
      'Debug32': {
        'defines': [
          'DEBUG',
          '_DEBUG',
        ],
        'debugging': "1",
        'bitness': "32",
      },
      'Release32': {
        'defines': [
          'NDEBUG',
        ],
        'debugging': "0",
        'bitness': "32",
      },
    },
    'conditions': [
      ['OS=="win"', {
        'target_defaults': {
          'msvs_settings': {
            'VCCLCompilerTool': {
              'ExceptionHandling': '0',
            },
          },
        },
      }],
    ],
  }
}
