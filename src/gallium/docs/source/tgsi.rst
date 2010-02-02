TGSI
====

TGSI, Tungsten Graphics Shader Infrastructure, is an intermediate language
for describing shaders. Since Gallium is inherently shaderful, shaders are
an important part of the API. TGSI is the only intermediate representation
used by all drivers.

Instruction Set
---------------

From GL_NV_vertex_program
^^^^^^^^^^^^^^^^^^^^^^^^^


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

  dst.x = 1

  dst.y = max(src.x, 0)

  dst.z = (src.x > 0) ? max(src.y, 0)^{clamp(src.w, -128, 128))} : 0

  dst.w = 1


RCP - Reciprocal

.. math::

  dst.x = \frac{1}{src.x}

  dst.y = \frac{1}{src.x}

  dst.z = \frac{1}{src.x}

  dst.w = \frac{1}{src.x}


RSQ - Reciprocal Square Root

.. math::

  dst.x = \frac{1}{\sqrt{|src.x|}}

  dst.y = \frac{1}{\sqrt{|src.x|}}

  dst.z = \frac{1}{\sqrt{|src.x|}}

  dst.w = \frac{1}{\sqrt{|src.x|}}


EXP - Approximate Exponential Base 2

.. math::

  dst.x = 2^{\lfloor src.x\rfloor}

  dst.y = src.x - \lfloor src.x\rfloor

  dst.z = 2^{src.x}

  dst.w = 1


LOG - Approximate Logarithm Base 2

.. math::

  dst.x = \lfloor\log_2{|src.x|}\rfloor

  dst.y = \frac{|src.x|}{2^{\lfloor\log_2{|src.x|}\rfloor}}

  dst.z = \log_2{|src.x|}

  dst.w = 1


MUL - Multiply

.. math::

  dst.x = src0.x \times src1.x

  dst.y = src0.y \times src1.y

  dst.z = src0.z \times src1.z

  dst.w = src0.w \times src1.w


ADD - Add

.. math::

  dst.x = src0.x + src1.x

  dst.y = src0.y + src1.y

  dst.z = src0.z + src1.z

  dst.w = src0.w + src1.w


DP3 - 3-component Dot Product

.. math::

  dst.x = src0.x \times src1.x + src0.y \times src1.y + src0.z \times src1.z

  dst.y = src0.x \times src1.x + src0.y \times src1.y + src0.z \times src1.z

  dst.z = src0.x \times src1.x + src0.y \times src1.y + src0.z \times src1.z

  dst.w = src0.x \times src1.x + src0.y \times src1.y + src0.z \times src1.z


DP4 - 4-component Dot Product

.. math::

  dst.x = src0.x \times src1.x + src0.y \times src1.y + src0.z \times src1.z + src0.w \times src1.w

  dst.y = src0.x \times src1.x + src0.y \times src1.y + src0.z \times src1.z + src0.w \times src1.w

  dst.z = src0.x \times src1.x + src0.y \times src1.y + src0.z \times src1.z + src0.w \times src1.w

  dst.w = src0.x \times src1.x + src0.y \times src1.y + src0.z \times src1.z + src0.w \times src1.w


DST - Distance Vector

.. math::

  dst.x = 1

  dst.y = src0.y \times src1.y

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

  dst.x = (src0.x < src1.x) ? 1 : 0

  dst.y = (src0.y < src1.y) ? 1 : 0

  dst.z = (src0.z < src1.z) ? 1 : 0

  dst.w = (src0.w < src1.w) ? 1 : 0


SGE - Set On Greater Equal Than

.. math::

  dst.x = (src0.x >= src1.x) ? 1 : 0

  dst.y = (src0.y >= src1.y) ? 1 : 0

  dst.z = (src0.z >= src1.z) ? 1 : 0

  dst.w = (src0.w >= src1.w) ? 1 : 0


MAD - Multiply And Add

.. math::

  dst.x = src0.x \times src1.x + src2.x

  dst.y = src0.y \times src1.y + src2.y

  dst.z = src0.z \times src1.z + src2.z

  dst.w = src0.w \times src1.w + src2.w


SUB - Subtract

.. math::

  dst.x = src0.x - src1.x

  dst.y = src0.y - src1.y

  dst.z = src0.z - src1.z

  dst.w = src0.w - src1.w


LRP - Linear Interpolate

.. math::

  dst.x = src0.x \times src1.x + (1 - src0.x) \times src2.x

  dst.y = src0.y \times src1.y + (1 - src0.y) \times src2.y

  dst.z = src0.z \times src1.z + (1 - src0.z) \times src2.z

  dst.w = src0.w \times src1.w + (1 - src0.w) \times src2.w


CND - Condition

.. math::

  dst.x = (src2.x > 0.5) ? src0.x : src1.x

  dst.y = (src2.y > 0.5) ? src0.y : src1.y

  dst.z = (src2.z > 0.5) ? src0.z : src1.z

  dst.w = (src2.w > 0.5) ? src0.w : src1.w


DP2A - 2-component Dot Product And Add

.. math::

  dst.x = src0.x \times src1.x + src0.y \times src1.y + src2.x

  dst.y = src0.x \times src1.x + src0.y \times src1.y + src2.x

  dst.z = src0.x \times src1.x + src0.y \times src1.y + src2.x

  dst.w = src0.x \times src1.x + src0.y \times src1.y + src2.x


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


ROUND - Round

.. math::

  dst.x = round(src.x)

  dst.y = round(src.y)

  dst.z = round(src.z)

  dst.w = round(src.w)


EX2 - Exponential Base 2

.. math::

  dst.x = 2^{src.x}

  dst.y = 2^{src.x}

  dst.z = 2^{src.x}

  dst.w = 2^{src.x}


LG2 - Logarithm Base 2

.. math::

  dst.x = \log_2{src.x}

  dst.y = \log_2{src.x}

  dst.z = \log_2{src.x}

  dst.w = \log_2{src.x}


POW - Power

.. math::

  dst.x = src0.x^{src1.x}

  dst.y = src0.x^{src1.x}

  dst.z = src0.x^{src1.x}

  dst.w = src0.x^{src1.x}

XPD - Cross Product

.. math::

  dst.x = src0.y \times src1.z - src1.y \times src0.z

  dst.y = src0.z \times src1.x - src1.z \times src0.x

  dst.z = src0.x \times src1.y - src1.x \times src0.y

  dst.w = 1


ABS - Absolute

.. math::

  dst.x = |src.x|

  dst.y = |src.y|

  dst.z = |src.z|

  dst.w = |src.w|


RCC - Reciprocal Clamped

XXX cleanup on aisle three

.. math::

  dst.x = (1 / src.x) > 0 ? clamp(1 / src.x, 5.42101e-020, 1.884467e+019) : clamp(1 / src.x, -1.884467e+019, -5.42101e-020)

  dst.y = (1 / src.x) > 0 ? clamp(1 / src.x, 5.42101e-020, 1.884467e+019) : clamp(1 / src.x, -1.884467e+019, -5.42101e-020)

  dst.z = (1 / src.x) > 0 ? clamp(1 / src.x, 5.42101e-020, 1.884467e+019) : clamp(1 / src.x, -1.884467e+019, -5.42101e-020)

  dst.w = (1 / src.x) > 0 ? clamp(1 / src.x, 5.42101e-020, 1.884467e+019) : clamp(1 / src.x, -1.884467e+019, -5.42101e-020)


DPH - Homogeneous Dot Product

.. math::

  dst.x = src0.x \times src1.x + src0.y \times src1.y + src0.z \times src1.z + src1.w

  dst.y = src0.x \times src1.x + src0.y \times src1.y + src0.z \times src1.z + src1.w

  dst.z = src0.x \times src1.x + src0.y \times src1.y + src0.z \times src1.z + src1.w

  dst.w = src0.x \times src1.x + src0.y \times src1.y + src0.z \times src1.z + src1.w


COS - Cosine

.. math::

  dst.x = \cos{src.x}

  dst.y = \cos{src.x}

  dst.z = \cos{src.x}

  dst.w = \cos{src.x}


DDX - Derivative Relative To X

.. math::

  dst.x = partialx(src.x)

  dst.y = partialx(src.y)

  dst.z = partialx(src.z)

  dst.w = partialx(src.w)


DDY - Derivative Relative To Y

.. math::

  dst.x = partialy(src.x)

  dst.y = partialy(src.y)

  dst.z = partialy(src.z)

  dst.w = partialy(src.w)


KILP - Predicated Discard

  discard


PK2H - Pack Two 16-bit Floats

  TBD


PK2US - Pack Two Unsigned 16-bit Scalars

  TBD


PK4B - Pack Four Signed 8-bit Scalars

  TBD


PK4UB - Pack Four Unsigned 8-bit Scalars

  TBD


RFL - Reflection Vector

.. math::

  dst.x = 2 \times (src0.x \times src1.x + src0.y \times src1.y + src0.z \times src1.z) / (src0.x \times src0.x + src0.y \times src0.y + src0.z \times src0.z) \times src0.x - src1.x

  dst.y = 2 \times (src0.x \times src1.x + src0.y \times src1.y + src0.z \times src1.z) / (src0.x \times src0.x + src0.y \times src0.y + src0.z \times src0.z) \times src0.y - src1.y

  dst.z = 2 \times (src0.x \times src1.x + src0.y \times src1.y + src0.z \times src1.z) / (src0.x \times src0.x + src0.y \times src0.y + src0.z \times src0.z) \times src0.z - src1.z

  dst.w = 1

Considered for removal.


SEQ - Set On Equal

.. math::

  dst.x = (src0.x == src1.x) ? 1 : 0

  dst.y = (src0.y == src1.y) ? 1 : 0

  dst.z = (src0.z == src1.z) ? 1 : 0

  dst.w = (src0.w == src1.w) ? 1 : 0


SFL - Set On False

.. math::

  dst.x = 0

  dst.y = 0

  dst.z = 0

  dst.w = 0

Considered for removal.

SGT - Set On Greater Than

.. math::

  dst.x = (src0.x > src1.x) ? 1 : 0

  dst.y = (src0.y > src1.y) ? 1 : 0

  dst.z = (src0.z > src1.z) ? 1 : 0

  dst.w = (src0.w > src1.w) ? 1 : 0


SIN - Sine

.. math::

  dst.x = \sin{src.x}

  dst.y = \sin{src.x}

  dst.z = \sin{src.x}

  dst.w = \sin{src.x}


SLE - Set On Less Equal Than

.. math::

  dst.x = (src0.x <= src1.x) ? 1 : 0

  dst.y = (src0.y <= src1.y) ? 1 : 0

  dst.z = (src0.z <= src1.z) ? 1 : 0

  dst.w = (src0.w <= src1.w) ? 1 : 0


SNE - Set On Not Equal

.. math::

  dst.x = (src0.x != src1.x) ? 1 : 0

  dst.y = (src0.y != src1.y) ? 1 : 0

  dst.z = (src0.z != src1.z) ? 1 : 0

  dst.w = (src0.w != src1.w) ? 1 : 0


STR - Set On True

.. math::

  dst.x = 1

  dst.y = 1

  dst.z = 1

  dst.w = 1


TEX - Texture Lookup

  TBD


TXD - Texture Lookup with Derivatives

  TBD


TXP - Projective Texture Lookup

  TBD


UP2H - Unpack Two 16-Bit Floats

  TBD

  Considered for removal.

UP2US - Unpack Two Unsigned 16-Bit Scalars

  TBD

  Considered for removal.

UP4B - Unpack Four Signed 8-Bit Values

  TBD

  Considered for removal.

UP4UB - Unpack Four Unsigned 8-Bit Scalars

  TBD

  Considered for removal.

X2D - 2D Coordinate Transformation

.. math::

  dst.x = src0.x + src1.x \times src2.x + src1.y \times src2.y

  dst.y = src0.y + src1.x \times src2.z + src1.y \times src2.w

  dst.z = src0.x + src1.x \times src2.x + src1.y \times src2.y

  dst.w = src0.y + src1.x \times src2.z + src1.y \times src2.w

Considered for removal.


From GL_NV_vertex_program2
^^^^^^^^^^^^^^^^^^^^^^^^^^


ARA - Address Register Add

  TBD

  Considered for removal.

ARR - Address Register Load With Round

.. math::

  dst.x = round(src.x)

  dst.y = round(src.y)

  dst.z = round(src.z)

  dst.w = round(src.w)


BRA - Branch

  pc = target

  Considered for removal.

CAL - Subroutine Call

  push(pc)
  pc = target


RET - Subroutine Call Return

  pc = pop()

  Potential restrictions:  
  * Only occurs at end of function.

SSG - Set Sign

.. math::

  dst.x = (src.x > 0) ? 1 : (src.x < 0) ? -1 : 0

  dst.y = (src.y > 0) ? 1 : (src.y < 0) ? -1 : 0

  dst.z = (src.z > 0) ? 1 : (src.z < 0) ? -1 : 0

  dst.w = (src.w > 0) ? 1 : (src.w < 0) ? -1 : 0


CMP - Compare

.. math::

  dst.x = (src0.x < 0) ? src1.x : src2.x

  dst.y = (src0.y < 0) ? src1.y : src2.y

  dst.z = (src0.z < 0) ? src1.z : src2.z

  dst.w = (src0.w < 0) ? src1.w : src2.w


KIL - Conditional Discard

.. math::

  if (src.x < 0 || src.y < 0 || src.z < 0 || src.w < 0)
    discard
  endif


SCS - Sine Cosine

.. math::

  dst.x = \cos{src.x}

  dst.y = \sin{src.x}

  dst.z = 0

  dst.y = 1


TXB - Texture Lookup With Bias

  TBD


NRM - 3-component Vector Normalise

.. math::

  dst.x = src.x / (src.x \times src.x + src.y \times src.y + src.z \times src.z)

  dst.y = src.y / (src.x \times src.x + src.y \times src.y + src.z \times src.z)

  dst.z = src.z / (src.x \times src.x + src.y \times src.y + src.z \times src.z)

  dst.w = 1


DIV - Divide

.. math::

  dst.x = \frac{src0.x}{src1.x}

  dst.y = \frac{src0.y}{src1.y}

  dst.z = \frac{src0.z}{src1.z}

  dst.w = \frac{src0.w}{src1.w}


DP2 - 2-component Dot Product

.. math::

  dst.x = src0.x \times src1.x + src0.y \times src1.y

  dst.y = src0.x \times src1.x + src0.y \times src1.y

  dst.z = src0.x \times src1.x + src0.y \times src1.y

  dst.w = src0.x \times src1.x + src0.y \times src1.y


TXL - Texture Lookup With LOD

  TBD


BRK - Break

  TBD


IF - If

  TBD


BGNFOR - Begin a For-Loop

  dst.x = floor(src.x)
  dst.y = floor(src.y)
  dst.z = floor(src.z)

  if (dst.y <= 0)
    pc = [matching ENDFOR] + 1
  endif

  Note: The destination must be a loop register.
        The source must be a constant register.

  Considered for cleanup / removal.


REP - Repeat

  TBD


ELSE - Else

  TBD


ENDIF - End If

  TBD


ENDFOR - End a For-Loop

  dst.x = dst.x + dst.z
  dst.y = dst.y - 1.0

  if (dst.y > 0)
    pc = [matching BGNFOR instruction] + 1
  endif

  Note: The destination must be a loop register.

  Considered for cleanup / removal.

ENDREP - End Repeat

  TBD


PUSHA - Push Address Register On Stack

  push(src.x)
  push(src.y)
  push(src.z)
  push(src.w)

  Considered for cleanup / removal.

POPA - Pop Address Register From Stack

  dst.w = pop()
  dst.z = pop()
  dst.y = pop()
  dst.x = pop()

  Considered for cleanup / removal.


From GL_NV_gpu_program4
^^^^^^^^^^^^^^^^^^^^^^^^

Support for these opcodes indicated by a special pipe capability bit (TBD).

CEIL - Ceiling

.. math::

  dst.x = \lceil src.x\rceil

  dst.y = \lceil src.y\rceil

  dst.z = \lceil src.z\rceil

  dst.w = \lceil src.w\rceil


I2F - Integer To Float

.. math::

  dst.x = (float) src.x

  dst.y = (float) src.y

  dst.z = (float) src.z

  dst.w = (float) src.w


NOT - Bitwise Not

.. math::

  dst.x = ~src.x

  dst.y = ~src.y

  dst.z = ~src.z

  dst.w = ~src.w


TRUNC - Truncate

.. math::

  dst.x = trunc(src.x)

  dst.y = trunc(src.y)

  dst.z = trunc(src.z)

  dst.w = trunc(src.w)


SHL - Shift Left

.. math::

  dst.x = src0.x << src1.x

  dst.y = src0.y << src1.x

  dst.z = src0.z << src1.x

  dst.w = src0.w << src1.x


SHR - Shift Right

.. math::

  dst.x = src0.x >> src1.x

  dst.y = src0.y >> src1.x

  dst.z = src0.z >> src1.x

  dst.w = src0.w >> src1.x


AND - Bitwise And

.. math::

  dst.x = src0.x & src1.x

  dst.y = src0.y & src1.y

  dst.z = src0.z & src1.z

  dst.w = src0.w & src1.w


OR - Bitwise Or

.. math::

  dst.x = src0.x | src1.x

  dst.y = src0.y | src1.y

  dst.z = src0.z | src1.z

  dst.w = src0.w | src1.w


MOD - Modulus

.. math::

  dst.x = src0.x \bmod src1.x

  dst.y = src0.y \bmod src1.y

  dst.z = src0.z \bmod src1.z

  dst.w = src0.w \bmod src1.w


XOR - Bitwise Xor

.. math::

  dst.x = src0.x \oplus src1.x

  dst.y = src0.y \oplus src1.y

  dst.z = src0.z \oplus src1.z

  dst.w = src0.w \oplus src1.w


SAD - Sum Of Absolute Differences

.. math::

  dst.x = |src0.x - src1.x| + src2.x

  dst.y = |src0.y - src1.y| + src2.y

  dst.z = |src0.z - src1.z| + src2.z

  dst.w = |src0.w - src1.w| + src2.w


TXF - Texel Fetch

  TBD


TXQ - Texture Size Query

  TBD


CONT - Continue

  TBD


From GL_NV_geometry_program4
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^


EMIT - Emit

  TBD


ENDPRIM - End Primitive

  TBD


From GLSL
^^^^^^^^^^


BGNLOOP - Begin a Loop

  TBD


BGNSUB - Begin Subroutine

  TBD


ENDLOOP - End a Loop

  TBD


ENDSUB - End Subroutine

  TBD


NOP - No Operation

  Do nothing.


NRM4 - 4-component Vector Normalise

.. math::

  dst.x = \frac{src.x}{src.x \times src.x + src.y \times src.y + src.z \times src.z + src.w \times src.w}

  dst.y = \frac{src.y}{src.x \times src.x + src.y \times src.y + src.z \times src.z + src.w \times src.w}

  dst.z = \frac{src.z}{src.x \times src.x + src.y \times src.y + src.z \times src.z + src.w \times src.w}

  dst.w = \frac{src.w}{src.x \times src.x + src.y \times src.y + src.z \times src.z + src.w \times src.w}


ps_2_x
^^^^^^^^^^^^


CALLNZ - Subroutine Call If Not Zero

  TBD


IFC - If

  TBD


BREAKC - Break Conditional

  TBD

Double Opcodes
^^^^^^^^^^^^^^^

DADD - Add Double

.. math::

  dst.xy = src0.xy + src1.xy

  dst.zw = src0.zw + src1.zw


DDIV - Divide Double

.. math::

  dst.xy = src0.xy / src1.xy

  dst.zw = src0.zw / src1.zw

DSEQ - Set Double on Equal

.. math::

  dst.xy = src0.xy == src1.xy ? 1.0F : 0.0F

  dst.zw = src0.zw == src1.zw ? 1.0F : 0.0F

DSLT - Set Double on Less than

.. math::

  dst.xy = src0.xy < src1.xy ? 1.0F : 0.0F

  dst.zw = src0.zw < src1.zw ? 1.0F : 0.0F

DFRAC - Double Fraction

.. math::

  dst.xy = src.xy - \lfloor src.xy\rfloor

  dst.zw = src.zw - \lfloor src.zw\rfloor


DFRACEXP - Convert Double Number to Fractional and Integral Components

.. math::

  dst0.xy = frexp(src.xy, dst1.xy)

  dst0.zw = frexp(src.zw, dst1.zw)

DLDEXP - Multiple Double Number by Integral Power of 2

.. math::

  dst.xy = ldexp(src0.xy, src1.xy)

  dst.zw = ldexp(src0.zw, src1.zw)

DMIN - Minimum Double

.. math::

  dst.xy = min(src0.xy, src1.xy)

  dst.zw = min(src0.zw, src1.zw)

DMAX - Maximum Double

.. math::

  dst.xy = max(src0.xy, src1.xy)

  dst.zw = max(src0.zw, src1.zw)

DMUL - Multiply Double

.. math::

  dst.xy = src0.xy \times src1.xy

  dst.zw = src0.zw \times src1.zw


DMAD - Multiply And Add Doubles

.. math::

  dst.xy = src0.xy \times src1.xy + src2.xy

  dst.zw = src0.zw \times src1.zw + src2.zw


DRCP - Reciprocal Double

.. math::

   dst.xy = \frac{1}{src.xy}

   dst.zw = \frac{1}{src.zw}

DSQRT - Square root double

.. math::

   dst.xy = \sqrt{src.xy}

   dst.zw = \sqrt{src.zw}


Explanation of symbols used
------------------------------


Functions
^^^^^^^^^^^^^^


  :math:`|x|`       Absolute value of `x`.

  :math:`\lceil x \rceil` Ceiling of `x`.

  clamp(x,y,z)      Clamp x between y and z.
                    (x < y) ? y : (x > z) ? z : x

  :math:`\lfloor x\rfloor` Floor of `x`.

  :math:`\log_2{x}` Logarithm of `x`, base 2.

  max(x,y)          Maximum of x and y.
                    (x > y) ? x : y

  min(x,y)          Minimum of x and y.
                    (x < y) ? x : y

  partialx(x)       Derivative of x relative to fragment's X.

  partialy(x)       Derivative of x relative to fragment's Y.

  pop()             Pop from stack.

  :math:`x^y`       `x` to the power `y`.

  push(x)           Push x on stack.

  round(x)          Round x.

  trunc(x)          Truncate x, i.e. drop the fraction bits.


Keywords
^^^^^^^^^^^^^


  discard           Discard fragment.

  dst               First destination register.

  dst0              First destination register.

  pc                Program counter.

  src               First source register.

  src0              First source register.

  src1              Second source register.

  src2              Third source register.

  target            Label of target instruction.


Other tokens
---------------


Declaration Semantic
^^^^^^^^^^^^^^^^^^^^^^^^


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

TGSI_SEMANTIC_POSITION
""""""""""""""""""""""

Position, sometimes known as HPOS or WPOS for historical reasons, is the
location of the vertex in space, in ``(x, y, z, w)`` format. ``x``, ``y``, and ``z``
are the Cartesian coordinates, and ``w`` is the homogenous coordinate and used
for the perspective divide, if enabled.

As a vertex shader output, position should be scaled to the viewport. When
used in fragment shaders, position will be in window coordinates. The convention
used depends on the FS_COORD_ORIGIN and FS_COORD_PIXEL_CENTER properties.

XXX additionally, is there a way to configure the perspective divide? it's
accelerated on most chipsets AFAIK...

Position, if not specified, usually defaults to ``(0, 0, 0, 1)``, and can
be partially specified as ``(x, y, 0, 1)`` or ``(x, y, z, 1)``.

XXX usually? can we solidify that?

TGSI_SEMANTIC_COLOR
"""""""""""""""""""

Colors are used to, well, color the primitives. Colors are always in
``(r, g, b, a)`` format.

If alpha is not specified, it defaults to 1.

TGSI_SEMANTIC_BCOLOR
""""""""""""""""""""

Back-facing colors are only used for back-facing polygons, and are only valid
in vertex shader outputs. After rasterization, all polygons are front-facing
and COLOR and BCOLOR end up occupying the same slots in the fragment, so
all BCOLORs effectively become regular COLORs in the fragment shader.

TGSI_SEMANTIC_FOG
"""""""""""""""""

The fog coordinate historically has been used to replace the depth coordinate
for generation of fog in dedicated fog blocks. Gallium, however, does not use
dedicated fog acceleration, placing it entirely in the fragment shader
instead.

The fog coordinate should be written in ``(f, 0, 0, 1)`` format. Only the first
component matters when writing from the vertex shader; the driver will ensure
that the coordinate is in this format when used as a fragment shader input.

TGSI_SEMANTIC_PSIZE
"""""""""""""""""""

PSIZE, or point size, is used to specify point sizes per-vertex. It should
be in ``(p, n, x, f)`` format, where ``p`` is the point size, ``n`` is the minimum
size, ``x`` is the maximum size, and ``f`` is the fade threshold.

XXX this is arb_vp. is this what we actually do? should double-check...

When using this semantic, be sure to set the appropriate state in the
:ref:`rasterizer` first.

TGSI_SEMANTIC_GENERIC
"""""""""""""""""""""

Generic semantics are nearly always used for texture coordinate attributes,
in ``(s, t, r, q)`` format. ``t`` and ``r`` may be unused for certain kinds
of lookups, and ``q`` is the level-of-detail bias for biased sampling.

These attributes are called "generic" because they may be used for anything
else, including parameters, texture generation information, or anything that
can be stored inside a four-component vector.

TGSI_SEMANTIC_NORMAL
""""""""""""""""""""

Vertex normal; could be used to implement per-pixel lighting for legacy APIs
that allow mixing fixed-function and programmable stages.

TGSI_SEMANTIC_FACE
""""""""""""""""""

FACE is the facing bit, to store the facing information for the fragment
shader. ``(f, 0, 0, 1)`` is the format. The first component will be positive
when the fragment is front-facing, and negative when the component is
back-facing.

TGSI_SEMANTIC_EDGEFLAG
""""""""""""""""""""""

XXX no clue


Properties
^^^^^^^^^^^^^^^^^^^^^^^^


  Properties are general directives that apply to the whole TGSI program.

FS_COORD_ORIGIN
"""""""""""""""

Specifies the fragment shader TGSI_SEMANTIC_POSITION coordinate origin.
The default value is UPPER_LEFT.

If UPPER_LEFT, the position will be (0,0) at the upper left corner and
increase downward and rightward.
If LOWER_LEFT, the position will be (0,0) at the lower left corner and
increase upward and rightward.

OpenGL defaults to LOWER_LEFT, and is configurable with the
GL_ARB_fragment_coord_conventions extension.

DirectX 9/10 use UPPER_LEFT.

FS_COORD_PIXEL_CENTER
"""""""""""""""""""""

Specifies the fragment shader TGSI_SEMANTIC_POSITION pixel center convention.
The default value is HALF_INTEGER.

If HALF_INTEGER, the fractionary part of the position will be 0.5
If INTEGER, the fractionary part of the position will be 0.0

Note that this does not affect the set of fragments generated by
rasterization, which is instead controlled by gl_rasterization_rules in the
rasterizer.

OpenGL defaults to HALF_INTEGER, and is configurable with the
GL_ARB_fragment_coord_conventions extension.

DirectX 9 uses INTEGER.
DirectX 10 uses HALF_INTEGER.



Texture Sampling and Texture Formats
------------------------------------

This table shows how texture image components are returned as (x,y,z,w)
tuples by TGSI texture instructions, such as TEX, TXD, and TXP.
For reference, OpenGL and Direct3D convensions are shown as well::

  Texture Components  Gallium       OpenGL        DX9
  ---------------------------------------------------------
  R,G,B,A             (R,G,B,A)     (R,G,B,A)     (R,G,B,A)
  R,G,B               (R,G,B,1)     (R,G,B,1)     (R,G,B,1)
  R,G                 tbd           (R,G,0,1)     (R,G,1,1)
  R                   tbd           (R,0,0,1)     (R,1,1,1)
  A                   (0,0,0,A)     (0,0,0,A)     (0,0,0,A)
  L                   (L,L,L,1)     (L,L,L,1)     (L,L,L,1)
  LA                  (L,L,L,A)     (L,L,L,A)     (L,L,L,A)
  I                   (I,I,I,I)     (I,I,I,I)     n/a
  UV                  tbd           (0,0,0,1)*    (U,V,1,1)
  Z                   tbd           (Z,Z,Z,Z) or  (0,Z,0,1)
                                    (Z,Z,Z,1) or
                                    (0,0,0,Z)**

  Footnotes:
  * per http://www.opengl.org/registry/specs/ATI/envmap_bumpmap.txt
  ** depends on GL_DEPTH_TEXTURE_MODE state
 