{
  'includes': [
    '../target_defaults.gypi',
  ],
  'targets': [
    {
      'target_name': 'glsl_optimizer_lib',
      'type': 'static_library',
      'include_dirs': [
        'glsl',
        'mesa',
        '../include',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          'glsl',
        ],
      },
      'sources': [
        '<!@(find glsl -name "*.cpp" -or -name "*.c" -or -name "*.h")',
        '<!@(find mesa -name "*.cpp" -or -name "*.c" -or -name "*.h")',
      ],
      'conditions': [
        ['OS=="win"', {
          'include_dirs': [
            '../include/c99',          
          ],
          'defines': [
            '_LIB',
            'NOMINMAX',
            '_CRT_SECURE_NO_WARNINGS',
            '_CRT_SECURE_NO_DEPRECATE',
            '__STDC_VERSION__=199901L',
            '__STDC__',
            'strdup=_strdup',
          ],
          'msvs_disabled_warnings': [4028, 4244, 4267, 4996],
        }],
      ],        
    }
  ]
}