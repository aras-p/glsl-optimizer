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
            'RuntimeLibrary': '0',
          },
        },
      }],
      ['_debugging==1', {
        'msvs_settings': {
          'VCCLCompilerTool': {
            'RuntimeLibrary': '1',
            'Optimization': '0',
            'DebugInformationFormat': '3',
            'ProgramDataBaseFileName': '$(OutDir)\\lib\\$(TargetName).pdb',
          },
          'VCLinkerTool': {
            'GenerateDebugInformation': 'true',
            'ProgramDatabaseFile': '$(OutDir)\\lib\\$(TargetName).pdb',
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
