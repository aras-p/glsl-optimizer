r600-sb
=======

* * * * *

Debugging
---------

### Environment variables

-   **R600\_DEBUG**

    There are new flags:

    -   **sb** - Enable optimization of graphics shaders
    -   **sbcl** - Enable optimization of compute shaders (experimental)
    -   **sbdry** - Dry run, optimize but use source bytecode - 
        useful if you only want to check shader dumps 
        without the risk of lockups and other problems
    -   **sbstat** - Print optimization statistics (only time so far)
    -   **sbdump** - Print IR after some passes.

### Regression debugging

If there are any regressions as compared to the default backend
(R600\_SB=0), it's possible to use the following environment variables
to find the incorrectly optimized shader that causes the regression.

-   **R600\_SB\_DSKIP\_MODE** - allows to skip optimization for some
    shaders
    -   0 - disabled (default)
    -   1 - skip optimization for the shaders in the range
        [R600\_SB\_DSKIP\_START; R600\_SB\_DSKIP\_END], that is,
        optimize only the shaders that are not in this range
    -   2 - optimize only the shaders in the range
        [R600\_SB\_DSKIP\_START; R600\_SB\_DSKIP\_END]

-   **R600\_SB\_DSKIP\_START** - start of the range (1-based)

-   **R600\_SB\_DSKIP\_END** - end of the range (1-based)

Example - optimize only the shaders 5, 6, and 7:

    R600_SB_DSKIP_START=5 R600_SB_DSKIP_END=7 R600_SB_DSKIP_MODE=2

All shaders compiled by the application are numbered starting from 1,
the number of shaders used by the application may be obtained by running
it with "R600_DEBUG=sb,sbstat" - it will print "sb: shader \#index\#"
for each compiled shader.

After figuring out the total number of shaders used by the application,
the variables above allow to use bisection to find the shader that is
the cause of regression. E.g. if the application uses 100 shaders, we
can divide the range [1; 100] and run the application with the
optimization enabled only for the first half of the shaders:

    R600_SB_DSKIP_START=1 R600_SB_DSKIP_END=50 R600_SB_DSKIP_MODE=2 <app>

If the regression is reproduced with these parameters, then the failing
shader is in the range [1; 50], if it's not reproduced - then it's in
the range [51; 100]. Then we can divide the new range again and repeat
the testing, until we'll reduce the range to a single failing shader.

*NOTE: This method relies on the assumption that the application
produces the same sequence of the shaders on each run. It's not always
true - some applications may produce different sequences of the shaders,
in such cases the tools like apitrace may be used to record the trace
with the application, then this method may be applied when replaying the
trace - also this may be faster and/or more convenient than testing the
application itself.*

* * * * *

Intermediate Representation
---------------------------

### Values

All kinds of the operands (literal constants, references to kcache
constants, references to GPRs, etc) are currently represented by the
**value** class (possibly it makes sense to switch to hierarchy of
classes derived from **value** instead, to save some memory).

All values (except some pseudo values like the exec\_mask or predicate
register) represent 32bit scalar values - there are no vector values,
CF/FETCH instructions use groups of 4 values for src and dst operands.

### Nodes

Shader programs are represented using the tree data structure, some
nodes contain a list of subnodes.

#### Control flow nodes

Control flow information is represented using four special node types
(based on the ideas from [[1]](#references) )

-   **region\_node** - single-entry, single-exit region.

    All loops and if's in the program are enclosed in region nodes.
    Region nodes have two containers for phi nodes -
    region\_node::loop\_phi contains the phi expressions to be executed
    at the region entry, region\_node::phi contains the phi expressions
    to be executed at the region exit. It's the only type of the node
    that contains associated phi expressions.

-   **depart\_node** - "depart region \$id after { ... }"

    Depart target region (jump to exit point) after executing contained
    code.

-   **repeat\_node** - "repeat region \$id after { ... }"

    Repeat target region (jump to entry point) after executing contained
    code.

-   **if\_node** - "if (cond) { ... }"

    Execute contained code if condition is true. The difference from
    [[1]](#references) is that we don't have associated phi expressions
    for the **if\_node**, we enclose **if\_node** in the
    **region\_node** and store corresponding phi's in the
    **region\_node**, this allows more uniform handling.

The target region of depart and repeat nodes is always the region where
they are located (possibly in the nested region), there are no arbitrary
jumps/goto's - control flow in the program is always structured.

Typical control flow constructs can be represented as in the following
examples:

GLSL:

    if (cond) {
        < 1 >
    } else {
        < 2 >
    }

IR:

    region #0 {
        depart region #0 after {
            if (cond) {
                depart region #0 after {
                    < 1 >
                }
            }
            < 2 >
        }
        <region #0 phi nodes >
    }

GLSL:

    while (cond) {
        < 1 >
    }

IR:

    region #0 {
        <region #0 loop_phi nodes>
        repeat region #0 after {
            region #1 {
                depart region #1 after {
                    if (!cond) {
                        depart region #0
                    }
                }
            }
            < 1 >
        }
        <region #0 phi nodes>
    }

'Break' and 'continue' inside the loops are directly translated to the
depart and repeat nodes for the corresponding loop region.

This may look a bit too complicated, but in fact this allows more simple
and uniform handling of the control flow.

All loop\_phi and phi nodes for some region always have the same number
of source operands. The number of source operands for
region\_node::loop\_phi nodes is 1 + number of repeat nodes that
reference this region as a target. The number of source operands for
region\_node::phi nodes is equal to the number of depart nodes that
reference this region as a target. All depart/repeat nodes for the
region have unique indices equal to the index of source operand for
phi/loop\_phi nodes.

First source operand for region\_node::loop\_phi nodes (src[0]) is an
incoming value that enters the region from the outside. Each remaining
source operand comes from the corresponding repeat node.

More complex example:

GLSL:

    a = 1;
    while (a < 5) {
        a = a * 2;
        if (b == 3) {
            continue;
        } else {
            a = 6;
        }
        if (c == 4)
            break;
        a = a + 1;
    }

IR with SSA form:

    a.1 = 1;
    region #0 {
        // loop phi values: src[0] - incoming, src[1] - from repeat_1, src[2] - from repeat_2
        region#0 loop_phi: a.2 = phi a.1, a.6, a.3

        repeat_1 region #0 after {
            a.3 = a.2 * 2;
            cond1 = (b == 3);
            region #1 {
                depart_0 region #1 after {
                    if (cond1) {
                        repeat_2 region #0;
                    }
                }
                a.4 = 6;

                region #1 phi: a.5 = phi a.4; // src[0] - from depart_0
            }
            cond2 = (c == 4);
            region #2 {
                depart_0 region #2 after {
                    if (cond2) {
                        depart_0 region #0;
                    }
                }
            }
            a.6 = a.5 + 1;
        }

        region #0 phi: a.7 = phi a.5 // src[0] from depart_0
    }

Phi nodes with single source operand are just copies, they are not
really necessary, but this allows to handle all **depart\_node**s in the
uniform way.

#### Instruction nodes

Instruction nodes represent different kinds of instructions -
**alu\_node**, **cf\_node**, **fetch\_node**, etc. Each of them contains
the "bc" structure where all fields of the bytecode are stored (the type
is **bc\_alu** for **alu\_node**, etc). The operands are represented
using the vectors of pointers to **value** class (node::src, node::dst)

#### SSA-specific nodes

Phi nodes currently don't have special node class, they are stored as
**node**. Destination vector contains a single destination value, source
vector contains 1 or more source values.

Psi nodes [[5], [6]](#references) also don't have a special node class
and stored as **node**. Source vector contains 3 values for each source
operand - the **value** of predicate, **value** of corresponding
PRED\_SEL field, and the source **value** itself.

### Indirect addressing

Special kind of values (VLK\_RELREG) is used to represent indirect
operands. These values don't have SSA versions. The representation is
mostly based on the [[2]](#references). Indirect operand contains the
"offset/address" value (value::rel), (e.g. some SSA version of the AR
register value, though after some passes it may be any value - constant,
register, etc), also it contains the maydef and mayuse vectors of
pointers to **value**s (similar to dst/src vectors in the **node**) to
represent the effects of aliasing in the SSA form.

E.g. if we have the array R5.x ... R8.x and the following instruction :

    MOV R0.x, R[5 + AR].x

then source indirect operand is represented with the VLK\_RELREG value,
value::rel is AR, value::maydef is empty (in fact it always contain the
same number of elements as mayuse to simplify the handling, but they are
NULLs), value::mayuse contains [R5.x, R6.x, R7.x, R8.x] (or the
corresponding SSA versions after ssa\_rename).

Additional "virtual variables" as in [HSSA [2]](#references) are not
used, also there is no special handling for "zero versions". Typical
programs in our case are small, indirect addressing is rare, array sizes
are limited by max gpr number, so we don't really need to use special
tricks to avoid the explosion of value versions. Also this allows more
precise liveness computation for array elements without modifications to
the algorithms.

With the following instruction:

    MOV R[5+AR].x, R0.x

we'll have both maydef and mayuse vectors for dst operand filled with
array values initially: [R5.x, R6.x, R7.x, R8.x]. After the ssa\_rename
pass mayuse will contain previous versions, maydef will contain new
potentially-defined versions.

* * * * *

Passes
------

-   **bc\_parser** - creates the IR from the source bytecode,
    initializes src and dst value vectors for instruction nodes. Most
    ALU nodes have one dst operand and the number of source operands is
    equal to the number of source operands for the ISA instruction.
    Nodes for PREDSETxx instructions have 3 dst operands - dst[0] is dst
    gpr as in the original instruction, other two are pseudo-operands
    that represent possibly updated predicate and exec\_mask. Predicate
    values are used in the predicated alu instructions (node::pred),
    exec\_mask values are used in the if\_nodes (if\_node::cond). Each
    vector operand in the CF/TEX/VTX instructions is represented with 4
    values - components of the vector.

-   **ssa\_prepare** - creates phi expressions.

-   **ssa\_rename** - renames the values (assigns versions).

-   **liveness** - liveness computation, sets 'dead' flag for unused
    nodes and values, optionally computes interference information for
    the values.

-   **dce\_cleanup** - eliminates 'dead' nodes, also removes some
    unnecessary nodes created by bc\_parser, e.g. the nodes for the JUMP
    instructions in the source, containers for ALU groups (they were
    only needed for the ssa\_rename pass)

-   **if\_conversion** - converts control flow with if\_nodes to the
    data flow in cases where it can improve performance (small alu-only
    branches). Both branches are executed speculatively and the phi
    expressions are replaced with conditional moves (CNDxx) to select
    the final value using the same condition predicate as was used by
    the original if\_node. E.g. **if\_node** used dst[2] from PREDSETxx
    instruction, CNDxx now uses dst[0] from the same PREDSETxx
    instruction.

-   **peephole** - peephole optimizations

-   **gvn** - Global Value Numbering [[2]](#references),
    [[3]](#references)

-   **gcm** - Global Code Motion [[3]](#references). Also performs
    grouping of the instructions of the same kind (CF/FETCH/ALU).

-   register allocation passes, some ideas are used from
    [[4]](#references), but implementation is simplified to make it more
    efficient in terms of the compilation speed (e.g. no recursive
    recoloring) while achieving good enough results.

    -   **ra\_split** - prepares the program to register allocation.
        Splits live ranges for constrained values by inserting the
        copies to/from temporary values, so that the live range of the
        constrained values becomes minimal.

    -   **ra\_coalesce** - performs global allocation on registers used
        in CF/FETCH instructions. It's performed first to make sure they
        end up in the same GPR. Also tries to allocate all values
        involved in copies (inserted by the ra\_split pass) to the same
        register, so that the copies may be eliminated.

    -   **ra\_init** - allocates gpr arrays (if indirect addressing is
        used), and remaining values.

-   **post\_scheduler** - ALU scheduler, handles VLIW packing and
    performs the final register allocation for local values inside ALU
    clauses. Eliminates all coalesced copies (if src and dst of the copy
    are allocated to the same register).

-   **ra\_checker** - optional debugging pass that tries to catch basic
    errors of the scheduler or regalloc,

-   **bc\_finalize** - propagates the regalloc information from values
    in node::src and node::dst vectors to the bytecode fields, converts
    control flow structure (region/depart/repeat) to the target
    instructions (JUMP/ELSE/POP,
    LOOP\_START/LOOP\_END/LOOP\_CONTINUE/LOOP\_BREAK).

-   **bc\_builder** - builds final bytecode,

* * * * *

References
----------

[1] ["Tree-Based Code Optimization. A Thesis Proposal", Carl
McConnell](http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.38.4210&rep=rep1&type=pdf)

[2] ["Effective Representation of Aliases and Indirect Memory Operations
in SSA Form", Fred Chow, Sun Chan, Shin-Ming Liu, Raymond Lo, Mark
Streich](http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.33.6974&rep=rep1&type=pdf)

[3] ["Global Code Motion. Global Value Numbering.", Cliff
Click](http://www.cs.washington.edu/education/courses/cse501/06wi/reading/click-pldi95.pdf)

[4] ["Register Allocation for Programs in SSA Form", Sebastian
Hack](http://digbib.ubka.uni-karlsruhe.de/volltexte/documents/6532)

[5] ["An extension to the SSA representation for predicated code",
Francois de
Ferriere](http://www.cdl.uni-saarland.de/ssasem/talks/Francois.de.Ferriere.pdf)

[6] ["Improvements to the Psi-SSA Representation", F. de
Ferriere](http://www.scopesconf.org/scopes-07/presentations/3_Presentation.pdf)
