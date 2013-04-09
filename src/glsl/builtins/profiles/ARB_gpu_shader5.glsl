#version 150
#extension GL_ARB_gpu_shader5 : enable

int   bitfieldExtract(int   value, int offset, int bits);
ivec2 bitfieldExtract(ivec2 value, int offset, int bits);
ivec3 bitfieldExtract(ivec3 value, int offset, int bits);
ivec4 bitfieldExtract(ivec4 value, int offset, int bits);
uint  bitfieldExtract(uint  value, int offset, int bits);
uvec2 bitfieldExtract(uvec2 value, int offset, int bits);
uvec3 bitfieldExtract(uvec3 value, int offset, int bits);
uvec4 bitfieldExtract(uvec4 value, int offset, int bits);

int   bitfieldInsert(int   base, int   insert, int offset, int bits);
ivec2 bitfieldInsert(ivec2 base, ivec2 insert, int offset, int bits);
ivec3 bitfieldInsert(ivec3 base, ivec3 insert, int offset, int bits);
ivec4 bitfieldInsert(ivec4 base, ivec4 insert, int offset, int bits);
uint  bitfieldInsert(uint  base, uint  insert, int offset, int bits);
uvec2 bitfieldInsert(uvec2 base, uvec2 insert, int offset, int bits);
uvec3 bitfieldInsert(uvec3 base, uvec3 insert, int offset, int bits);
uvec4 bitfieldInsert(uvec4 base, uvec4 insert, int offset, int bits);

int   bitfieldReverse(int   value);
ivec2 bitfieldReverse(ivec2 value);
ivec3 bitfieldReverse(ivec3 value);
ivec4 bitfieldReverse(ivec4 value);
uint  bitfieldReverse(uint  value);
uvec2 bitfieldReverse(uvec2 value);
uvec3 bitfieldReverse(uvec3 value);
uvec4 bitfieldReverse(uvec4 value);

int   bitCount(int   value);
ivec2 bitCount(ivec2 value);
ivec3 bitCount(ivec3 value);
ivec4 bitCount(ivec4 value);
int   bitCount(uint  value);
ivec2 bitCount(uvec2 value);
ivec3 bitCount(uvec3 value);
ivec4 bitCount(uvec4 value);

int   findLSB(int   value);
ivec2 findLSB(ivec2 value);
ivec3 findLSB(ivec3 value);
ivec4 findLSB(ivec4 value);
int   findLSB(uint  value);
ivec2 findLSB(uvec2 value);
ivec3 findLSB(uvec3 value);
ivec4 findLSB(uvec4 value);

int   findMSB(int   value);
ivec2 findMSB(ivec2 value);
ivec3 findMSB(ivec3 value);
ivec4 findMSB(ivec4 value);
int   findMSB(uint  value);
ivec2 findMSB(uvec2 value);
ivec3 findMSB(uvec3 value);
ivec4 findMSB(uvec4 value);
