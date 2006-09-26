$ /*
$ Shader test script.
$ 
$ Author: Michal Krol
$ 
$ Comment line starts with dollar sign and white space.
$ 
$ $program <name> starts a new test program section called <name>. Contains all other sections.
$ 
$ $attrib <name> starts vertex data input section for attrib called <name>. Each line consists of
$   four values that form single vertex attrib.
$ 
$ $vertex starts vertex shader section. Contains $code and &output sections.
$ 
$ $code starts source code section. All text in this section gets compiled into appropriate
$   shader object.
$ 
$ $output starts shader execution results section. These are compared, value-by-value,
$   with results of executing printMESA() functions within a shader.
$ */


$ /*
$ --------------------------------------------------------------------------------------------------
$ Test printMESA() function.
$ */

$program PRINT TEST

$vertex

$code

#version 110

#extension MESA_shader_debug: require

void main () {
   gl_Position = gl_ModelViewMatrix * gl_Vertex;
   gl_FrontColor = vec4 (1.0);

   printMESA (11.1);
   printMESA (111);
   printMESA (true);

   printMESA (vec2 (22.1, 22.2));
   printMESA (vec3 (33.1, 33.2, 33.3));
   printMESA (vec4 (44.1, 44.2, 44.3, 44.4));

   printMESA (ivec2 (221, 222));
   printMESA (ivec3 (331, 332, 333));
   printMESA (ivec4 (441, 442, 443, 444));

   printMESA (bvec2 (false, true));
   printMESA (bvec3 (true, true, false));
   printMESA (bvec4 (true, false, true, false));

   printMESA (mat2 (55.11, 55.12, 55.21, 55.22));
   printMESA (mat3 (66.11, 66.12, 66.13,
                    66.21, 66.22, 66.23,
                    66.31, 66.32, 66.33));
   printMESA (mat4 (77.11, 77.12, 77.13, 77.14,
                    77.21, 77.22, 77.23, 77.24,
                    77.31, 77.32, 77.33, 77.34,
                    77.41, 77.42, 77.43, 77.44));
}

$output

11.1
111
true

22.1
22.2
33.1
33.2
33.3
44.1
44.2
44.3
44.4

221
222
331
332
333
441
442
443
444

false
true
true
true
false
true
false
true
false

55.11
55.12
55.21
55.22

66.11
66.12
66.13
66.21
66.22
66.23
66.31
66.32
66.33

77.11
77.12
77.13
77.14
77.21
77.22
77.23
77.24
77.31
77.32
77.33
77.34
77.41
77.42
77.43
77.44


$ /*
$ --------------------------------------------------------------------------------------------------
$ Test type casting.
$ */

$program TYPE CAST TEST

$attrib gl_Vertex
0.0 0.0 0.0 1.0

$attrib _Zero
0.0 0.0 0.0 0.0

$attrib _One
1.1 0.0 0.0 0.0

$attrib _Two
2.2 0.0 0.0 0.0

$attrib _MinusThree
-3.3 0.0 0.0 0.0

$vertex

$code

#version 110

#extension MESA_shader_debug: require

attribute float _Zero;
attribute float _One;
attribute float _Two;
attribute float _MinusThree;

void main () {
   gl_Position = gl_ModelViewMatrix * gl_Vertex;
   gl_FrontColor = vec4 (1.0);

   printMESA (_Zero);
   printMESA (_One);
   printMESA (_Two);
   printMESA (_MinusThree);

   printMESA (float (_Zero));
   printMESA (float (_One));
   printMESA (float (_Two));
   printMESA (float (_MinusThree));
   printMESA (float (45.99));
   printMESA (float (-6.17));

   printMESA (bool (_Zero));
   printMESA (bool (_One));
   printMESA (bool (_Two));
   printMESA (bool (_MinusThree));
   printMESA (bool (45.99));
   printMESA (bool (-6.17));
   printMESA (bool (0.0001));
   printMESA (bool (0.0));

   printMESA (int (_Zero));
   printMESA (int (_One));
   printMESA (int (_Two));
   printMESA (int (_MinusThree));
   printMESA (int (45.99));
   printMESA (int (45.22));
   printMESA (int (-6.17));
   printMESA (int (-6.87));
}

$output

0.0
1.1
2.2
-3.3

0.0
1.1
2.2
-3.3
45.99
-6.17

false
true
true
true
true
true
true
false

0
1
2
-3
45
45
-6
-6

$ /*
$ --------------------------------------------------------------------------------------------------
$ Test vector swizzles.
$ */

$program SWIZZLE TEST

$attrib gl_Vertex
0.0 0.0 0.0 1.0

$attrib _One
1.1 1.2 1.3 1.4

$attrib _Two
2.1 2.2 2.3 2.4

$vertex

$code

#version 110

#extension MESA_shader_debug: require

attribute vec4 _One;
attribute vec4 _Two;

void assign5678 (out vec4 v)
{
   v.x = 5.5;
   v.y = 6.6;
   v.z = 7.7;
   v.w = 8.8;
}

void main () {
   gl_Position = gl_ModelViewMatrix * gl_Vertex;
   gl_FrontColor = vec4 (1.0);

   printMESA (_One);
   printMESA (_Two);

   printMESA (_One.x);
   printMESA (_One.y);
   printMESA (_One.z);
   printMESA (_One.w);

   printMESA (_Two.xy);
   printMESA (_Two.yx);
   printMESA (_Two.xw);
   printMESA (_Two.wx);
   printMESA (_Two.yz);
   printMESA (_Two.zy);
   printMESA (_Two.xz);
   printMESA (_Two.zx);
   printMESA (_Two.zw);
   printMESA (_Two.wz);

   printMESA (_One.xyz);
   printMESA (_One.yzx);
   printMESA (_One.zxy);
   printMESA (_One.xzy);
   printMESA (_One.yzw);
   printMESA (_One.zwx);

   printMESA (_Two.xyzw);
   printMESA (_Two.yzwx);
   printMESA (_Two.wzyx);
   printMESA (_Two.zwyx);

   printMESA (_One.xx);
   printMESA (_One.zz);
   printMESA (_One.ww);

   printMESA (_Two.xxx);
   printMESA (_Two.yyy);
   printMESA (_Two.www);

   printMESA (_One.xxxx);
   printMESA (_One.zzzz);

   printMESA (_Two.xxyy);
   printMESA (_Two.wwxx);
   printMESA (_Two.zxxw);

   vec4 v;

   v.zxwy = vec4 (5.5, 6.6, 7.7, 8.8);
   printMESA (v);

   assign5678 (v.ywxz);
   printMESA (v);
}

$output

1.1
1.2
1.3
1.4
2.1
2.2
2.3
2.4

1.1
1.2
1.3
1.4

2.1
2.2
2.2
2.1
2.1
2.4
2.4
2.1
2.2
2.3
2.3
2.2
2.1
2.3
2.3
2.1
2.3
2.4
2.4
2.3

1.1
1.2
1.3
1.2
1.3
1.1
1.3
1.1
1.2
1.1
1.3
1.2
1.2
1.3
1.4
1.3
1.4
1.1

2.1
2.2
2.3
2.4
2.2
2.3
2.4
2.1
2.4
2.3
2.2
2.1
2.3
2.4
2.2
2.1

1.1
1.1
1.3
1.3
1.4
1.4

2.1
2.1
2.1
2.2
2.2
2.2
2.4
2.4
2.4

1.1
1.1
1.1
1.1
1.3
1.3
1.3
1.3

2.1
2.1
2.2
2.2
2.4
2.4
2.1
2.1
2.3
2.1
2.1
2.4

6.6
8.8
5.5
7.7

7.7
5.5
8.8
6.6


$ /*
$ --------------------------------------------------------------------------------------------------
$ Test relational operators.
$ */

$program RELATIONAL OPERATOR TEST

$attrib gl_Vertex
0.0 0.0 0.0 1.0

$attrib _Two
2.0 0.0 0.0 0.0

$attrib _Two2
2.0 0.0 0.0 0.0

$attrib _MinusThree
-3.0 0.0 0.0 0.0

$vertex

$code

#version 110

#extension MESA_shader_debug: require

attribute float _Two;
attribute float _Two2;
attribute float _MinusThree;

struct foo
{
	float f;
	vec4 v4;
	vec3 v3;
	mat4 m4;
	int i;
	bool b;
};

void printMESA (const in foo bar)
{
	printMESA (bar.f);
	printMESA (bar.v4);
	printMESA (bar.v3);
	printMESA (bar.m4);
	printMESA (bar.i);
	printMESA (bar.b);
}

void main () {
   gl_Position = gl_ModelViewMatrix * gl_Vertex;
   gl_FrontColor = vec4 (1.0);

   int iTwo = int (_Two);
   int iTwo2 = int (_Two2);
   int iMinusThree = int (_MinusThree);

   printMESA (_Two <= _Two);
   printMESA (_Two <= _Two2);
   printMESA (_Two <= _MinusThree);
   printMESA (_MinusThree <= _Two);
   printMESA (iTwo <= iTwo);
   printMESA (iTwo <= iTwo2);
   printMESA (iTwo <= iMinusThree);
   printMESA (iMinusThree <= iTwo);

   printMESA (_Two >= _Two);
   printMESA (_Two >= _Two2);
   printMESA (_Two >= _MinusThree);
   printMESA (_MinusThree >= _Two);
   printMESA (iTwo >= iTwo);
   printMESA (iTwo >= iTwo2);
   printMESA (iTwo >= iMinusThree);
   printMESA (iMinusThree >= iTwo);

   printMESA (_Two < _Two);
   printMESA (_Two < _Two2);
   printMESA (_Two < _MinusThree);
   printMESA (_MinusThree < _Two);
   printMESA (iTwo < iTwo);
   printMESA (iTwo < iTwo2);
   printMESA (iTwo < iMinusThree);
   printMESA (iMinusThree < iTwo);

   printMESA (_Two > _Two);
   printMESA (_Two > _Two2);
   printMESA (_Two > _MinusThree);
   printMESA (_MinusThree > _Two);
   printMESA (iTwo > iTwo);
   printMESA (iTwo > iTwo2);
   printMESA (iTwo > iMinusThree);
   printMESA (iMinusThree > iTwo);

   printMESA (_Two == _Two);
   printMESA (_Two == _Two2);
   printMESA (_Two == _MinusThree);
   printMESA (_MinusThree == _MinusThree);
   printMESA (iTwo == iTwo);
   printMESA (iTwo == iTwo2);
   printMESA (iTwo == iMinusThree);
   printMESA (iMinusThree == iMinusThree);

   printMESA (_Two != _Two);
   printMESA (_Two != _Two2);
   printMESA (_Two != _MinusThree);
   printMESA (_MinusThree != _MinusThree);
   printMESA (iTwo != iTwo);
   printMESA (iTwo != iTwo2);
   printMESA (iTwo != iMinusThree);
   printMESA (iMinusThree != iMinusThree);

   foo foo1;
   foo1.f = 13.31;
   foo1.v4 = vec4 (44.11, 44.22, 44.33, 44.44);
   foo1.v3 = vec3 (33.11, 33.22, 33.33);
   foo1.m4 = mat4 (17.88);
   foo1.i = 666;
   foo1.b = true;
   printMESA (foo1);

   // make foo2 the same as foo1
   foo foo2;
   foo2.f = 13.31;
   foo2.v4 = vec4 (44.11, 44.22, 44.33, 44.44);
   foo2.v3 = vec3 (33.11, 33.22, 33.33);
   foo2.m4 = mat4 (17.88);
   foo2.i = 666;
   foo2.b = true;

   printMESA (foo1 == foo2);
   printMESA (foo1 != foo2);

   // make them a little bit different
   foo2.m4[2].y = 333.333;
   printMESA (foo2);

   printMESA (foo1 == foo2);
   printMESA (foo1 != foo2);
}

$output

true
true
false
true
true
true
false
true

true
true
true
false
true
true
true
false

false
false
false
true
false
false
false
true

false
false
true
false
false
false
true
false

true
true
false
true
true
true
false
true

false
false
true
false
false
false
true
false

13.31
44.11
44.22
44.33
44.44
33.11
33.22
33.33
17.88
0.0
0.0
0.0
0.0
17.88
0.0
0.0
0.0
0.0
17.88
0.0
0.0
0.0
0.0
17.88
666
true

true
false

13.31
44.11
44.22
44.33
44.44
33.11
33.22
33.33
17.88
0.0
0.0
0.0
0.0
17.88
0.0
0.0
0.0
333.333
17.88
0.0
0.0
0.0
0.0
17.88
666
true

false
true


$ /*
$ --------------------------------------------------------------------------------------------------
$ Test logical operators.
$ */

$program LOGICAL OPERATOR TEST

$attrib gl_Vertex
0.0 0.0 0.0 1.0

$attrib _False
0.0 0.0 0.0 0.0

$attrib _True
1.0 0.0 0.0 0.0

$attrib _False2
0.0 0.0 0.0 0.0

$attrib _True2
1.0 0.0 0.0 0.0

$vertex

$code

#version 110

#extension MESA_shader_debug: require

attribute float _False;
attribute float _True;
attribute float _False2;
attribute float _True2;

void main () {
   gl_Position = gl_ModelViewMatrix * gl_Vertex;
   gl_FrontColor = vec4 (1.0);

   printMESA (_False);
   printMESA (_True);
   printMESA (_False2);
   printMESA (_True2);

   bool False = bool (_False);
   bool True = bool (_True);
   bool False2 = bool (_False2);
   bool True2 = bool (_True2);

   //
   // It is important to test each operator with the following argument types:
   // * Both arguments are different variables, even if they have the same values.
   //   False and False2 are distinct attributes, but are the same in value.
   // * Both arguments may be the same variables. This case tests possible
   //   optimizations, e.g. X && X --> X.
   // * Both arguments are constant. This tests constant folding.
   //

   printMESA (!False);
   printMESA (!True);
   printMESA (!false);
   printMESA (!true);

   printMESA (False ^^ False2);
   printMESA (False ^^ True2);
   printMESA (True ^^ False2);
   printMESA (True ^^ True2);
   printMESA (False ^^ False);
   printMESA (False ^^ True);
   printMESA (True ^^ False);
   printMESA (True ^^ True);
   printMESA (false ^^ false);
   printMESA (false ^^ true);
   printMESA (true ^^ false);
   printMESA (true ^^ true);

   printMESA (False && False2);
   printMESA (False && True2);
   printMESA (True && False2);
   printMESA (True && True2);
   printMESA (False && False);
   printMESA (False && True);
   printMESA (True && False);
   printMESA (True && True);
   printMESA (false && false);
   printMESA (false && true);
   printMESA (true && false);
   printMESA (true && true);

   printMESA (False || False2);
   printMESA (False || True2);
   printMESA (True || False2);
   printMESA (True || True2);
   printMESA (False || False);
   printMESA (False || True);
   printMESA (True || False);
   printMESA (True || True);
   printMESA (false || false);
   printMESA (false || true);
   printMESA (true || false);
   printMESA (true || true);

   //
   // Test short-circuit evaluation of && and ||. The right expression evaluation depends
   // on the value of the left expression. If the right expression has side effects, we
   // can easily test if it happened.
   //

   bool x;

   x = false;
   printMESA (x);
   printMESA (False && (x = true));
   printMESA (x);

   x = false;
   printMESA (x);
   printMESA (false && (x = true));
   printMESA (x);

   x = true;
   printMESA (x);
   printMESA (True || (x = false));
   printMESA (x);

   x = true;
   printMESA (x);
   printMESA (true || (x = false));
   printMESA (x);
}

$output

0.0
1.0
0.0
1.0

true
false
true
false

false
true
true
false
false
true
true
false
false
true
true
false

false
false
false
true
false
false
false
true
false
false
false
true

false
true
true
true
false
true
true
true
false
true
true
true

false
false
false

false
false
false

true
true
true

true
true
true


$ /*
$ --------------------------------------------------------------------------------------------------
$ Test subscript operator/array access.
$ */

$program ARRAY ACCESS TEST

$attrib gl_Vertex
0.0 0.0 0.0 1.0

$attrib _Zero
0.0 0.0 0.0 0.0

$attrib _One
1.1 0.0 0.0 0.0

$attrib _Two
2.9 0.0 0.0 0.0

$attrib _Vec
11.11 22.22 33.33 44.44

$vertex

$code

#version 110

#extension MESA_shader_debug: require

attribute float _Zero;
attribute float _One;
attribute float _Two;
attribute vec4 _Vec;

void main () {
   gl_Position = gl_ModelViewMatrix * gl_Vertex;
   gl_FrontColor = vec4 (1.0);

   printMESA (_Zero);
   printMESA (_One);
   printMESA (_Two);
   printMESA (_Vec);

   printMESA (_Vec[0]);
   printMESA (_Vec[1]);
   printMESA (_Vec[2]);
   printMESA (_Vec[3]);

   printMESA (_Vec[int (_Zero)]);
   printMESA (_Vec[int (_One)]);
   printMESA (_Vec[int (_Two)]);
}

$output

0.0
1.1
2.9
11.11
22.22
33.33
44.44

11.11
22.22
33.33
44.44

11.11
22.22
33.33


$ /*
$ --------------------------------------------------------------------------------------------------
$ Test pre/post-increment/decrement operators.
$ Note: assumes relational operators being correct.
$ */

$program PRE/POST-INC/DEC OPERATOR TEST

$attrib gl_Vertex
0.0 0.0 0.0 1.0

$attrib _Zero
0.0 0.0 0.0 0.0

$attrib _One
1.1 0.0 0.0 0.0

$attrib _Two4
2.1 2.2 2.3 2.4

$vertex

$code

#version 110

#extension MESA_shader_debug: require

attribute float _Zero;
attribute float _One;
attribute vec4 _Two4;

float fZero, fOne;
vec4 fTwo4;
int iZero, iOne;
ivec4 iTwo4;

void reset () {
   fZero = _Zero;
   fOne = _One;
   fTwo4 = _Two4;
   iZero = int (_Zero);
   iOne = int (_One);
   iTwo4 = ivec4 (_Two4);
}

void main () {
   gl_Position = gl_ModelViewMatrix * gl_Vertex;
   gl_FrontColor = vec4 (1.0);

   printMESA (_Zero);
   printMESA (_One);
   printMESA (_Two4);

   // pre-increment
   reset ();
   printMESA (++fZero);
   printMESA (++fOne);
   printMESA (++iZero);
   printMESA (++iOne);
   printMESA (fZero);
   printMESA (fOne);
   printMESA (iZero);
   printMESA (iOne);
   printMESA (++fTwo4 == _Two4 + 1.0);
   printMESA (++iTwo4 == ivec4 (_Two4) + 1);

   // pre-decrement
   reset ();
   printMESA (--fZero);
   printMESA (--fOne);
   printMESA (--iZero);
   printMESA (--iOne);
   printMESA (fZero);
   printMESA (fOne);
   printMESA (iZero);
   printMESA (iOne);
   printMESA (--fTwo4 == _Two4 - 1.0);
   printMESA (--iTwo4 == ivec4 (_Two4) - 1);

   // post-increment
   reset ();
   printMESA (fZero++);
   printMESA (fOne++);
   printMESA (iZero++);
   printMESA (iOne++);
   printMESA (fZero);
   printMESA (fOne);
   printMESA (iZero);
   printMESA (iOne);
   printMESA (fTwo4++ == _Two4);
   printMESA (iTwo4++ == ivec4 (_Two4));

   // post-decrement
   reset ();
   printMESA (fZero--);
   printMESA (fOne--);
   printMESA (iZero--);
   printMESA (iOne--);
   printMESA (fZero);
   printMESA (fOne);
   printMESA (iZero);
   printMESA (iOne);
   printMESA (fTwo4-- == _Two4);
   printMESA (iTwo4-- == ivec4 (_Two4));
}

$output

0.0
1.1
2.1
2.2
2.3
2.4

1.0
2.1
1
2
1.0
2.1
1
2
true
true

-1.0
0.1
-1
0
-1.0
0.1
-1
0
true
true

0.0
1.1
0
1
1.0
2.1
1
2
true
true

0.0
1.1
0
1
-1.0
0.1
-1
0
true
true


$ /*
$ --------------------------------------------------------------------------------------------------
$ Test arithmetical operators.
$ */

$program ARITHMETICAL OPERATOR TEST

$attrib gl_Vertex
0.0 0.0 0.0 1.0

$attrib _Zero
0.0 0.0 0.0 0.0

$attrib _One
1.1 0.0 0.0 0.0

$attrib _Two4
2.1 2.2 2.3 2.4

$vertex

$code

#version 110

#extension MESA_shader_debug: require

attribute float _Zero;
attribute float _One;
attribute vec4 _Two4;

void main () {
   gl_Position = gl_ModelViewMatrix * gl_Vertex;
   gl_FrontColor = vec4 (1.0);

   printMESA (_Zero);
   printMESA (_One);
   printMESA (_Two4);

   int iZero = int (_Zero);
   int iOne = int (_One);
   ivec4 iTwo4 = ivec4 (_Two4);

   printMESA (-_Zero);
   printMESA (-_One);
   printMESA (-_Two4);
   printMESA (-_Two4.z);

   printMESA (_Zero + 0.0);
   printMESA (_One + 0.0);
   printMESA (_Two4 + 0.0);
   printMESA (_Two4.y + 0.0);

   printMESA (_Zero + _Zero);
   printMESA (_Zero + _One);
   printMESA (_Zero + _Two4);
   printMESA (_One + _Zero);
   printMESA (_One + _Two4);
   printMESA (_Two4 + _Two4);

   printMESA (_Zero - 0.0);
   printMESA (_One - 0.0);
   printMESA (_Two4 - 0.0);
   printMESA (_Two4.y - 0.0);

   printMESA (_Zero - _Zero);
   printMESA (_Zero - _One);
   printMESA (_Zero - _Two4);
   printMESA (_One - _Zero);
   printMESA (_One - _Two4);
   printMESA (_Two4 - _Two4);

   printMESA (_Zero * 1.0);
   printMESA (_One * 1.0);
   printMESA (_Two4 * 1.0);
   printMESA (_Two4.x * 1.0);

   printMESA (_Zero * _Zero);
   printMESA (_Zero * _One);
   printMESA (_Zero * _Two4);
   printMESA (_One * _Zero);
   printMESA (_One * _One);
   printMESA (_One * _Two4);
   printMESA (_Two4 * _Two4);

   printMESA (_Zero / 1.0);
   printMESA (_One / 1.0);
   printMESA (_Two4 / 1.0);
   printMESA (_Two4.x / 1.0);

   printMESA (_Zero / _One);
   printMESA (_Zero / _Two4);
   printMESA (_One / _One);
   printMESA (_One / _Two4);
   printMESA (_Two4 / _Two4);
}

$output

0.0
1.1
2.1
2.2
2.3
2.4

0.0
-1.1
-2.1
-2.2
-2.3
-2.4
-2.3

0.0
1.1
2.1
2.2
2.3
2.4
2.2

0.0
1.1
2.1
2.2
2.3
2.4
1.1
3.2
3.3
3.4
3.5
4.2
4.4
4.6
4.8

0.0
1.1
2.1
2.2
2.3
2.4
2.2

0.0
-1.1
-2.1
-2.2
-2.3
-2.4
1.1
-1.0
-1.1
-1.2
-1.3
0.0
0.0
0.0
0.0

0.0
1.1
2.1
2.2
2.3
2.4
2.1

0.0
0.0
0.0
0.0
0.0
0.0
0.0
1.21
2.31
2.42
2.53
2.64
4.41
4.84
5.29
5.76

0.0
1.1
2.1
2.2
2.3
2.4
2.1

0.0
0.0
0.0
0.0
0.0
1.0
0.52381
0.5
0.47826
0.45833
1.0
1.0
1.0
1.0


$ /*
$ --------------------------------------------------------------------------------------------------
$ Test matrix operations.
$ Note: assumes relational operators being correct.
$ */

$program MATRIX TEST

$attrib gl_Vertex
0.0 0.0 0.0 1.0

$attrib _Zero
0.0 0.0 0.0 0.0

$attrib _One
1.0 1.0 1.0 1.0

$attrib _Two
2.0 2.0 2.0 2.0

$vertex

$code

#version 110

#extension MESA_shader_debug: require

attribute vec4 _Zero;
attribute vec4 _One;
attribute vec4 _Two;

void main () {
   gl_Position = gl_ModelViewMatrix * gl_Vertex;
   gl_FrontColor = vec4 (1.0);

   printMESA (_Zero);
   printMESA (_One);
   printMESA (_Two);

   mat4 Identity = mat4 (_One.x);

   printMESA (Identity == mat4 (1.0, 0.0, 0.0, 0.0,
                                0.0, 1.0, 0.0, 0.0,
                                0.0, 0.0, 1.0, 0.0,
                                0.0, 0.0, 0.0, 1.0));
   printMESA (Identity * _Two == _Two);

   mat4 Matrix = mat4 (1.1, 1.2, 1.3, 1.4,
                       2.1, 2.2, 2.3, 2.4,
                       3.1, 3.2, 3.3, 3.4,
                       4.1, 4.2, 4.3, 4.4);

   printMESA (Matrix[2].y);
   printMESA (Matrix[1]);
}

$output

0.0
0.0
0.0
0.0
1.0
1.0
1.0
1.0
2.0
2.0
2.0
2.0
true
true
3.2
2.1
2.2
2.3
2.4


$ /*
$ --------------------------------------------------------------------------------------------------
$ Test vec4 extension operations.
$ */

$program VEC4 EXTENSION OPERATIONS

$attrib gl_Vertex
0.0 0.0 0.0 1.0

$attrib _One
1.1 0.0 0.0 0.0

$attrib _Two4
2.1 2.2 2.3 2.4

$attrib _Three4
3.1 3.2 3.3 3.4

$vertex

$code

#version 110

#extension MESA_shader_debug: require

attribute float _One;
attribute vec4 _Two4;
attribute vec4 _Three4;

void main () {
   gl_Position = gl_ModelViewMatrix * gl_Vertex;
   gl_FrontColor = vec4 (1.0);

   printMESA (_One);
   printMESA (_Two4);
   printMESA (_Three4);

   printMESA (vec4 (_One));

   printMESA (_Two4 + _Three4);
   printMESA (_Two4 - _Three4);
   printMESA (_Two4 * _Three4);
   printMESA (_Two4 / _Three4);

   printMESA (_Two4 + _One);
   printMESA (_Two4 - _One);
   printMESA (_Two4 * _One);
   printMESA (_Two4 / _One);

   printMESA (_One + _Two4);
   printMESA (_One - _Two4);
   printMESA (_One * _Two4);
   printMESA (_One / _Two4);

   printMESA (-_Three4);

   printMESA (dot (_Two4.xyz, _Three4.xyz));
   printMESA (dot (_Two4, _Three4));

   printMESA (length (_Two4.xyz));
   printMESA (length (_Three4));

   printMESA (normalize (_Two4.xyz));
   printMESA (normalize (_Three4));

   vec4 tmp = _Two4;
   printMESA (tmp);

   printMESA (_Two4 == _Three4);
   printMESA (_Two4 != _Three4);
   printMESA (_Two4 == _Two4);
   printMESA (_Three4 != _Three4);
   printMESA (_Two4 != vec4 (_Two4.xyz, 999.0));
   printMESA (_Two4 != vec4 (999.0, _Two4.yzw));
}

$output

1.1
2.1
2.2
2.3
2.4
3.1
3.2
3.3
3.4

1.1
1.1
1.1
1.1

5.2
5.4
5.6
5.8
-1.0
-1.0
-1.0
-1.0
6.51
7.04
7.59
8.16
0.677419
0.6875
0.69697
0.705882

3.2
3.3
3.4
3.5
1.0
1.1
1.2
1.3
2.31
2.42
2.53
2.64
1.909091
2.0
2.090909
2.181818

3.2
3.3
3.4
3.5
-1.0
-1.1
-1.2
-1.3
2.31
2.42
2.53
2.64
0.52381
0.5
0.478261
0.458333

-3.1
-3.2
-3.3
-3.4

21.14
29.3

3.813135
6.503845

0.550728
0.576953
0.603178
0.476641
0.492017
0.507392
0.522768

2.1
2.2
2.3
2.4

false
true
true
false
true
true
