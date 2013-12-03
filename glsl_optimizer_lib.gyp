{
  'includes': [
    'target_defaults.gypi',
  ],
  'targets': [
    {
      'target_name': 'glsl_optimizer_tests',
      'type': 'executable',
      'dependencies': [
        'src/glsl_optimizer.gyp:glsl_optimizer',
      ],
      'sources': [
        'tests/glsl_optimizer_tests.cpp',
      ],
      'conditions': [
        ['OS=="win"', {
          'msvs_disabled_warnings': [4506],
          'libraries': [
            'user32.lib', 'gdi32.lib', 'opengl32.lib',
          ],
        }],
        ['OS=="mac"', {
          'libraries': [
            '$(SDKROOT)/System/Library/Frameworks/OpenGL.framework',
          ],
        }],
      ],
    },
  ]
}
