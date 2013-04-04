FRAG
DCL IN[0], COLOR, LINEAR
DCL OUT[0], COLOR
DCL TEMP[0]
IMM[0] FLT32 {   10.0000,     1.0000,     0.0000,     0.0000}
IMM[1] UINT32 {1, 0, 0, 0}
0: MUL TEMP[0].x, IN[0].xxxx, IMM[0].xxxx
1: F2U TEMP[0].x, TEMP[0].xxxx
2: AND TEMP[0].x, TEMP[0].xxxx, IMM[1].xxxx
3: UCMP OUT[0], TEMP[0].xxxx, IMM[0].yzzz, IMM[0].yyyz
4: END
