/*
Copyright (c) 2012 Adobe Systems Incorporated

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

package com.adobe.AGALOptimiser.translator.transformations {

import com.adobe.AGALOptimiser.nsinternal;

use namespace nsinternal; 

[ExcludeClass]
public final class Constants
{
    nsinternal static const COLOR_TAG                                      : String = "color";
    nsinternal static const DEGREE_TAG                                     : String = "degree";
    nsinternal static const EDGES_TAG                                      : String = "edges";
    nsinternal static const ID_TAG                                         : String = "id";
    nsinternal static const INTERFERENCE_GRAPH_TAG                         : String = "interferenceGraph";
    nsinternal static const TRANSFORMATION_CONSTANT_OPERAND_NORMALIZER     : String = "CONORM";
    nsinternal static const TRANSFORMATION_CONSTANT_VECTORIZER             : String = "CVEC";
    nsinternal static const TRANSFORMATION_DEAD_CODE_ELIMINATION           : String = "DCE";
    nsinternal static const TRANSFORMATION_IDENTITY_MOVE_REMOVER           : String = "IMR";
    nsinternal static const TRANSFORMATION_MOVE_CHAIN_REDUCTION            : String = "MCR";
    nsinternal static const TRANSFORMATION_PATTERN_PEEPHOLE_OPTIMIZER      : String = "PEEP";
    nsinternal static const TRANSFORMATION_REGISTER_ASSIGNER               : String = "RASS";
    nsinternal static const TRANSFORMATION_SOURCE_TO_SINK_REORGANIZER      : String = "SO2SIREORG";
    nsinternal static const TRANSFORMATION_STANDARD_PEEPHOLE_OPTIMIZER     : String = "SPEEP"; 

} // Constants
} // com.adobe.AGALOptimiser.translator.transformations