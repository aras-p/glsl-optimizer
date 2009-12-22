TGSI
====

TGSI, Tungsten Graphics Shader Instructions, is an intermediate language
for describing shaders. Since Gallium is inherently shaderful, shaders are
an important part of the API. TGSI is the only intermediate representation
used by all drivers.

From GL_NV_vertex_program
-------------------------


ARL - Address Register Load

.. math::

  dst.x = \lfloor src.x\rfloor

  dst.y = \lfloor src.y\rfloor

  dst.z = \lfloor src.z\rfloor

  dst.w = \lfloor src.w\rfloor


MOV - Move

.. math::

  dst.x = src.x

  dst.y = src.y

  dst.z = src.z

  dst.w = src.w


LIT - Light Coefficients

.. math::

  dst.x = 1.0

  dst.y = max(src.x, 0.0)

  dst.z = (src.x > 0.0) ? pow(max(src.y, 0.0), clamp(src.w, -128.0, 128.0)) : 0.0

  dst.w = 1.0


RCP - Reciprocal

.. math::

  dst.x = 1.0 / src.x

  dst.y = 1.0 / src.x

  dst.z = 1.0 / src.x

  dst.w = 1.0 / src.x


RSQ - Reciprocal Square Root

.. math::

  dst.x = 1.0 / sqrt(abs(src.x))

  dst.y = 1.0 / sqrt(abs(src.x))

  dst.z = 1.0 / sqrt(abs(src.x))

  dst.w = 1.0 / sqrt(abs(src.x))


EXP - Approximate Exponential Base 2

.. math::

  dst.x = pow(2.0, \lfloor src.x\rfloor)

  dst.y = src.x - \lfloor src.x\rfloor

  dst.z = pow(2.0, src.x)

  dst.w = 1.0


LOG - Approximate Logarithm Base 2

.. math::

  dst.x = \lfloor lg2(abs(src.x)))\rfloor

  dst.y = abs(src.x) / pow(2.0, \lfloor lg2(abs(src.x))\rfloor )

  dst.z = lg2(abs(src.x))

  dst.w = 1.0


MUL - Multiply

.. math::

  dst.x = src0.x * src1.x

  dst.y = src0.y * src1.y

  dst.z = src0.z * src1.z

  dst.w = src0.w * src1.w


ADD - Add

.. math::

  dst.x = src0.x + src1.x

  dst.y = src0.y + src1.y

  dst.z = src0.z + src1.z

  dst.w = src0.w + src1.w


DP3 - 3-component Dot Product

.. math::

  dst.x = src0.x * src1.x + src0.y * src1.y + src0.z * src1.z

  dst.y = src0.x * src1.x + src0.y * src1.y + src0.z * src1.z

  dst.z = src0.x * src1.x + src0.y * src1.y + src0.z * src1.z

  dst.w = src0.x * src1.x + src0.y * src1.y + src0.z * src1.z


DP4 - 4-component Dot Product

.. math::

  dst.x = src0.x * src1.x + src0.y * src1.y + src0.z * src1.z + src0.w * src1.w

  dst.y = src0.x * src1.x + src0.y * src1.y + src0.z * src1.z + src0.w * src1.w

  dst.z = src0.x * src1.x + src0.y * src1.y + src0.z * src1.z + src0.w * src1.w

  dst.w = src0.x * src1.x + src0.y * src1.y + src0.z * src1.z + src0.w * src1.w


DST - Distance Vector

.. math::

  dst.x = 1.0

  dst.y = src0.y * src1.y

  dst.z = src0.z

  dst.w = src1.w


MIN - Minimum

.. math::

  dst.x = min(src0.x, src1.x)

  dst.y = min(src0.y, src1.y)

  dst.z = min(src0.z, src1.z)

  dst.w = min(src0.w, src1.w)


MAX - Maximum

.. math::

  dst.x = max(src0.x, src1.x)

  dst.y = max(src0.y, src1.y)

  dst.z = max(src0.z, src1.z)

  dst.w = max(src0.w, src1.w)


SLT - Set On Less Than

.. math::

  dst.x = (src0.x < src1.x) ? 1.0 : 0.0

  dst.y = (src0.y < src1.y) ? 1.0 : 0.0

  dst.z = (src0.z < src1.z) ? 1.0 : 0.0

  dst.w = (src0.w < src1.w) ? 1.0 : 0.0


SGE - Set On Greater Equal Than

.. math::

  dst.x = (src0.x >= src1.x) ? 1.0 : 0.0

  dst.y = (src0.y >= src1.y) ? 1.0 : 0.0

  dst.z = (src0.z >= src1.z) ? 1.0 : 0.0

  dst.w = (src0.w >= src1.w) ? 1.0 : 0.0


MAD - Multiply And Add

.. math::

  dst.x = src0.x * src1.x + src2.x

  dst.y = src0.y * src1.y + src2.y

  dst.z = src0.z * src1.z + src2.z

  dst.w = src0.w * src1.w + src2.w


SUB - Subtract

.. math::

  dst.x = src0.x - src1.x

  dst.y = src0.y - src1.y

  dst.z = src0.z - src1.z

  dst.w = src0.w - src1.w


LRP - Linear Interpolate

.. math::

  dst.x = src0.x * (src1.x - src2.x) + src2.x

  dst.y = src0.y * (src1.y - src2.y) + src2.y

  dst.z = src0.z * (src1.z - src2.z) + src2.z

  dst.w = src0.w * (src1.w - src2.w) + src2.w


CND - Condition

.. math::

  dst.x = (src2.x > 0.5) ? src0.x : src1.x

  dst.y = (src2.y > 0.5) ? src0.y : src1.y

  dst.z = (src2.z > 0.5) ? src0.z : src1.z

  dst.w = (src2.w > 0.5) ? src0.w : src1.w


DP2A - 2-component Dot Product And Add

.. math::

  dst.x = src0.x * src1.x + src0.y * src1.y + src2.x

  dst.y = src0.x * src1.x + src0.y * src1.y + src2.x

  dst.z = src0.x * src1.x + src0.y * src1.y + src2.x

  dst.w = src0.x * src1.x + src0.y * src1.y + src2.x


FRAC - Fraction

.. math::

  dst.x = src.x - \lfloor src.x\rfloor

  dst.y = src.y - \lfloor src.y\rfloor

  dst.z = src.z - \lfloor src.z\rfloor

  dst.w = src.w - \lfloor src.w\rfloor


CLAMP - Clamp

.. math::

  dst.x = clamp(src0.x, src1.x, src2.x)
  dst.y = clamp(src0.y, src1.y, src2.y)
  dst.z = clamp(src0.z, src1.z, src2.z)
  dst.w = clamp(src0.w, src1.w, src2.w)


FLR - Floor

This is identical to ARL.

.. math::

  dst.x = \lfloor src.x\rfloor

  dst.y = \lfloor src.y\rfloor

  dst.z = \lfloor src.z\rfloor

  dst.w = \lfloor src.w\rfloor


1.3.9  ROUND - Round

.. math::

  dst.x = round(src.x)
  dst.y = round(src.y)
  dst.z = round(src.z)
  dst.w = round(src.w)


1.3.10  EX2 - Exponential Base 2

.. math::

  dst.x = pow(2.0, src.x)
  dst.y = pow(2.0, src.x)
  dst.z = pow(2.0, src.x)
  dst.w = pow(2.0, src.x)


1.3.11  LG2 - Logarithm Base 2

.. math::

  dst.x = lg2(src.x)
  dst.y = lg2(src.x)
  dst.z = lg2(src.x)
  dst.w = lg2(src.x)


1.3.12  POW - Power

.. math::

  dst.x = pow(src0.x, src1.x)
  dst.y = pow(src0.x, src1.x)
  dst.z = pow(src0.x, src1.x)
  dst.w = pow(src0.x, src1.x)

1.3.15  XPD - Cross Product

.. math::

  dst.x = src0.y * src1.z - src1.y * src0.z
  dst.y = src0.z * src1.x - src1.z * src0.x
  dst.z = src0.x * src1.y - src1.x * src0.y
  dst.w = 1.0


1.4.1  ABS - Absolute

.. math::

  dst.x = abs(src.x)
  dst.y = abs(src.y)
  dst.z = abs(src.z)
  dst.w = abs(src.w)


1.4.2  RCC - Reciprocal Clamped

.. math::

  dst.x = (1.0 / src.x) > 0.0 ? clamp(1.0 / src.x, 5.42101e-020, 1.884467e+019) : clamp(1.0 / src.x, -1.884467e+019, -5.42101e-020)
  dst.y = (1.0 / src.x) > 0.0 ? clamp(1.0 / src.x, 5.42101e-020, 1.884467e+019) : clamp(1.0 / src.x, -1.884467e+019, -5.42101e-020)
  dst.z = (1.0 / src.x) > 0.0 ? clamp(1.0 / src.x, 5.42101e-020, 1.884467e+019) : clamp(1.0 / src.x, -1.884467e+019, -5.42101e-020)
  dst.w = (1.0 / src.x) > 0.0 ? clamp(1.0 / src.x, 5.42101e-020, 1.884467e+019) : clamp(1.0 / src.x, -1.884467e+019, -5.42101e-020)


1.4.3  DPH - Homogeneous Dot Product

.. math::

  dst.x = src0.x * src1.x + src0.y * src1.y + src0.z * src1.z + src1.w
  dst.y = src0.x * src1.x + src0.y * src1.y + src0.z * src1.z + src1.w
  dst.z = src0.x * src1.x + src0.y * src1.y + src0.z * src1.z + src1.w
  dst.w = src0.x * src1.x + src0.y * src1.y + src0.z * src1.z + src1.w


COS - Cosine

.. math::

  dst.x = \cos{src.x}

  dst.y = \cos{src.x}

  dst.z = \cos{src.x}

  dst.w = \cos{src.w}


1.5.2  DDX - Derivative Relative To X

.. math::

  dst.x = partialx(src.x)
  dst.y = partialx(src.y)
  dst.z = partialx(src.z)
  dst.w = partialx(src.w)


1.5.3  DDY - Derivative Relative To Y

.. math::

  dst.x = partialy(src.x)
  dst.y = partialy(src.y)
  dst.z = partialy(src.z)
  dst.w = partialy(src.w)


1.5.7  KILP - Predicated Discard

.. math::

  discard


1.5.10  PK2H - Pack Two 16-bit Floats

  TBD


1.5.11  PK2US - Pack Two Unsigned 16-bit Scalars

  TBD


1.5.12  PK4B - Pack Four Signed 8-bit Scalars

  TBD


1.5.13  PK4UB - Pack Four Unsigned 8-bit Scalars

  TBD


1.5.15  RFL - Reflection Vector

.. math::

  dst.x = 2.0 * (src0.x * src1.x + src0.y * src1.y + src0.z * src1.z) / (src0.x * src0.x + src0.y * src0.y + src0.z * src0.z) * src0.x - src1.x
  dst.y = 2.0 * (src0.x * src1.x + src0.y * src1.y + src0.z * src1.z) / (src0.x * src0.x + src0.y * src0.y + src0.z * src0.z) * src0.y - src1.y
  dst.z = 2.0 * (src0.x * src1.x + src0.y * src1.y + src0.z * src1.z) / (src0.x * src0.x + src0.y * src0.y + src0.z * src0.z) * src0.z - src1.z
  dst.w = 1.0

Considered for removal.


1.5.16  SEQ - Set On Equal

.. math::

  dst.x = (src0.x == src1.x) ? 1.0 : 0.0
  dst.y = (src0.y == src1.y) ? 1.0 : 0.0
  dst.z = (src0.z == src1.z) ? 1.0 : 0.0
  dst.w = (src0.w == src1.w) ? 1.0 : 0.0


1.5.17  SFL - Set On False

.. math::

  dst.x = 0.0
  dst.y = 0.0
  dst.z = 0.0
  dst.w = 0.0

Considered for removal.

1.5.18  SGT - Set On Greater Than

.. math::

  dst.x = (src0.x > src1.x) ? 1.0 : 0.0
  dst.y = (src0.y > src1.y) ? 1.0 : 0.0
  dst.z = (src0.z > src1.z) ? 1.0 : 0.0
  dst.w = (src0.w > src1.w) ? 1.0 : 0.0


SIN - Sine

.. math::

  dst.x = \sin{src.x}

  dst.y = \sin{src.x}

  dst.z = \sin{src.x}

  dst.w = \sin{src.w}


1.5.20  SLE - Set On Less Equal Than

.. math::

  dst.x = (src0.x <= src1.x) ? 1.0 : 0.0
  dst.y = (src0.y <= src1.y) ? 1.0 : 0.0
  dst.z = (src0.z <= src1.z) ? 1.0 : 0.0
  dst.w = (src0.w <= src1.w) ? 1.0 : 0.0


1.5.21  SNE - Set On Not Equal

.. math::

  dst.x = (src0.x != src1.x) ? 1.0 : 0.0
  dst.y = (src0.y != src1.y) ? 1.0 : 0.0
  dst.z = (src0.z != src1.z) ? 1.0 : 0.0
  dst.w = (src0.w != src1.w) ? 1.0 : 0.0


1.5.22  STR - Set On True

.. math::

  dst.x = 1.0
  dst.y = 1.0
  dst.z = 1.0
  dst.w = 1.0


1.5.23  TEX - Texture Lookup

  TBD


1.5.24  TXD - Texture Lookup with Derivatives

  TBD


1.5.25  TXP - Projective Texture Lookup

  TBD


1.5.26  UP2H - Unpack Two 16-Bit Floats

  TBD

  Considered for removal.

1.5.27  UP2US - Unpack Two Unsigned 16-Bit Scalars

  TBD

  Considered for removal.

1.5.28  UP4B - Unpack Four Signed 8-Bit Values

  TBD

  Considered for removal.

1.5.29  UP4UB - Unpack Four Unsigned 8-Bit Scalars

  TBD

  Considered for removal.

1.5.30  X2D - 2D Coordinate Transformation

.. math::

  dst.x = src0.x + src1.x * src2.x + src1.y * src2.y
  dst.y = src0.y + src1.x * src2.z + src1.y * src2.w
  dst.z = src0.x + src1.x * src2.x + src1.y * src2.y
  dst.w = src0.y + src1.x * src2.z + src1.y * src2.w

Considered for removal.


1.6  GL_NV_vertex_program2
--------------------------


1.6.1  ARA - Address Register Add

  TBD

  Considered for removal.

1.6.2  ARR - Address Register Load With Round

.. math::

  dst.x = round(src.x)
  dst.y = round(src.y)
  dst.z = round(src.z)
  dst.w = round(src.w)


1.6.3  BRA - Branch

  pc = target

  Considered for removal.

1.6.4  CAL - Subroutine Call

  push(pc)
  pc = target


1.6.5  RET - Subroutine Call Return

  pc = pop()

  Potential restrictions:  
  * Only occurs at end of function.

1.6.6  SSG - Set Sign

.. math::

  dst.x = (src.x > 0.0) ? 1.0 : (src.x < 0.0) ? -1.0 : 0.0
  dst.y = (src.y > 0.0) ? 1.0 : (src.y < 0.0) ? -1.0 : 0.0
  dst.z = (src.z > 0.0) ? 1.0 : (src.z < 0.0) ? -1.0 : 0.0
  dst.w = (src.w > 0.0) ? 1.0 : (src.w < 0.0) ? -1.0 : 0.0


1.8.1  CMP - Compare

.. math::

  dst.x = (src0.x < 0.0) ? src1.x : src2.x
  dst.y = (src0.y < 0.0) ? src1.y : src2.y
  dst.z = (src0.z < 0.0) ? src1.z : src2.z
  dst.w = (src0.w < 0.0) ? src1.w : src2.w


1.8.2  KIL - Conditional Discard

.. math::

  if (src.x < 0.0 || src.y < 0.0 || src.z < 0.0 || src.w < 0.0)
    discard
  endif


SCS - Sine Cosine

.. math::

  dst.x = \cos{src.x}

  dst.y = \sin{src.x}

  dst.z = 0.0

  dst.y = 1.0


1.8.4  TXB - Texture Lookup With Bias

  TBD


1.9.1  NRM - 3-component Vector Normalise

.. math::

  dst.x = src.x / (src.x * src.x + src.y * src.y + src.z * src.z)
  dst.y = src.y / (src.x * src.x + src.y * src.y + src.z * src.z)
  dst.z = src.z / (src.x * src.x + src.y * src.y + src.z * src.z)
  dst.w = 1.0


1.9.2  DIV - Divide

.. math::

  dst.x = src0.x / src1.x
  dst.y = src0.y / src1.y
  dst.z = src0.z / src1.z
  dst.w = src0.w / src1.w


1.9.3  DP2 - 2-component Dot Product

.. math::

  dst.x = src0.x * src1.x + src0.y * src1.y
  dst.y = src0.x * src1.x + src0.y * src1.y
  dst.z = src0.x * src1.x + src0.y * src1.y
  dst.w = src0.x * src1.x + src0.y * src1.y


1.9.5  TXL - Texture Lookup With LOD

  TBD


1.9.6  BRK - Break

  TBD


1.9.7  IF - If

  TBD


1.9.8  BGNFOR - Begin a For-Loop

  dst.x = floor(src.x)
  dst.y = floor(src.y)
  dst.z = floor(src.z)

  if (dst.y <= 0)
    pc = [matching ENDFOR] + 1
  endif

  Note: The destination must be a loop register.
        The source must be a constant register.

  Considered for cleanup / removal.


1.9.9  REP - Repeat

  TBD


1.9.10  ELSE - Else

  TBD


1.9.11  ENDIF - End If

  TBD


1.9.12  ENDFOR - End a For-Loop

  dst.x = dst.x + dst.z
  dst.y = dst.y - 1.0

  if (dst.y > 0)
    pc = [matching BGNFOR instruction] + 1
  endif

  Note: The destination must be a loop register.

  Considered for cleanup / removal.

1.9.13  ENDREP - End Repeat

  TBD


1.10.1  PUSHA - Push Address Register On Stack

  push(src.x)
  push(src.y)
  push(src.z)
  push(src.w)

  Considered for cleanup / removal.

1.10.2  POPA - Pop Address Register From Stack

  dst.w = pop()
  dst.z = pop()
  dst.y = pop()
  dst.x = pop()

  Considered for cleanup / removal.


1.11  GL_NV_gpu_program4
------------------------

Support for these opcodes indicated by a special pipe capability bit (TBD).

1.11.1  CEIL - Ceiling

.. math::

  dst.x = ceil(src.x)
  dst.y = ceil(src.y)
  dst.z = ceil(src.z)
  dst.w = ceil(src.w)


1.11.2  I2F - Integer To Float

.. math::

  dst.x = (float) src.x
  dst.y = (float) src.y
  dst.z = (float) src.z
  dst.w = (float) src.w


1.11.3  NOT - Bitwise Not

.. math::

  dst.x = ~src.x
  dst.y = ~src.y
  dst.z = ~src.z
  dst.w = ~src.w


1.11.4  TRUNC - Truncate

.. math::

  dst.x = trunc(src.x)
  dst.y = trunc(src.y)
  dst.z = trunc(src.z)
  dst.w = trunc(src.w)


1.11.5  SHL - Shift Left

.. math::

  dst.x = src0.x << src1.x
  dst.y = src0.y << src1.x
  dst.z = src0.z << src1.x
  dst.w = src0.w << src1.x


1.11.6  SHR - Shift Right

.. math::

  dst.x = src0.x >> src1.x
  dst.y = src0.y >> src1.x
  dst.z = src0.z >> src1.x
  dst.w = src0.w >> src1.x


1.11.7  AND - Bitwise And

.. math::

  dst.x = src0.x & src1.x
  dst.y = src0.y & src1.y
  dst.z = src0.z & src1.z
  dst.w = src0.w & src1.w


1.11.8  OR - Bitwise Or

.. math::

  dst.x = src0.x | src1.x
  dst.y = src0.y | src1.y
  dst.z = src0.z | src1.z
  dst.w = src0.w | src1.w


1.11.9  MOD - Modulus

.. math::

  dst.x = src0.x % src1.x
  dst.y = src0.y % src1.y
  dst.z = src0.z % src1.z
  dst.w = src0.w % src1.w


1.11.10  XOR - Bitwise Xor

.. math::

  dst.x = src0.x ^ src1.x
  dst.y = src0.y ^ src1.y
  dst.z = src0.z ^ src1.z
  dst.w = src0.w ^ src1.w


1.11.11  SAD - Sum Of Absolute Differences

.. math::

  dst.x = abs(src0.x - src1.x) + src2.x
  dst.y = abs(src0.y - src1.y) + src2.y
  dst.z = abs(src0.z - src1.z) + src2.z
  dst.w = abs(src0.w - src1.w) + src2.w


1.11.12  TXF - Texel Fetch

  TBD


1.11.13  TXQ - Texture Size Query

  TBD


1.11.14  CONT - Continue

  TBD


1.12  GL_NV_geometry_program4
-----------------------------


1.12.1  EMIT - Emit

  TBD


1.12.2  ENDPRIM - End Primitive

  TBD


1.13  GLSL
----------


1.13.1  BGNLOOP - Begin a Loop

  TBD


1.13.2  BGNSUB - Begin Subroutine

  TBD


1.13.3  ENDLOOP - End a Loop

  TBD


1.13.4  ENDSUB - End Subroutine

  TBD



1.13.10  NOP - No Operation

  Do nothing.



1.16.7  NRM4 - 4-component Vector Normalise

.. math::

  dst.x = src.x / (src.x * src.x + src.y * src.y + src.z * src.z + src.w * src.w)
  dst.y = src.y / (src.x * src.x + src.y * src.y + src.z * src.z + src.w * src.w)
  dst.z = src.z / (src.x * src.x + src.y * src.y + src.z * src.z + src.w * src.w)
  dst.w = src.w / (src.x * src.x + src.y * src.y + src.z * src.z + src.w * src.w)


1.17  ps_2_x
------------


1.17.2  CALLNZ - Subroutine Call If Not Zero

  TBD


1.17.3  IFC - If

  TBD


1.17.5  BREAKC - Break Conditional

  TBD


2  Explanation of symbols used
==============================


2.1  Functions
--------------


  abs(x)            Absolute value of x.
                    (x < 0.0) ? -x : x

  ceil(x)           Ceiling of x.

  clamp(x,y,z)      Clamp x between y and z.
                    (x < y) ? y : (x > z) ? z : x

  :math:`\lfloor x\rfloor` Floor of x.

  lg2(x)            Logarithm base 2 of x.

  max(x,y)          Maximum of x and y.
                    (x > y) ? x : y

  min(x,y)          Minimum of x and y.
                    (x < y) ? x : y

  partialx(x)       Derivative of x relative to fragment's X.

  partialy(x)       Derivative of x relative to fragment's Y.

  pop()             Pop from stack.

  pow(x,y)          Raise x to power of y.

  push(x)           Push x on stack.

  round(x)          Round x.

  sqrt(x)           Square root of x.

  trunc(x)          Truncate x.


2.2  Keywords
-------------


  discard           Discard fragment.

  dst               First destination register.

  dst0              First destination register.

  pc                Program counter.

  src               First source register.

  src0              First source register.

  src1              Second source register.

  src2              Third source register.

  target            Label of target instruction.


3  Other tokens
===============


3.1  Declaration Semantic
-------------------------


  Follows Declaration token if Semantic bit is set.

  Since its purpose is to link a shader with other stages of the pipeline,
  it is valid to follow only those Declaration tokens that declare a register
  either in INPUT or OUTPUT file.

  SemanticName field contains the semantic name of the register being declared.
  There is no default value.

  SemanticIndex is an optional subscript that can be used to distinguish
  different register declarations with the same semantic name. The default value
  is 0.

  The meanings of the individual semantic names are explained in the following
  sections.


3.1.1  FACE

  Valid only in a fragment shader INPUT declaration.

  FACE.x is negative when the primitive is back facing. FACE.x is positive
  when the primitive is front facing.
