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

package com.adobe.AGALOptimiser.agal {

import com.adobe.AGALOptimiser.nsinternal;

import flash.utils.Dictionary;

use namespace nsinternal;

// This class is used during a recursive cloning operation to maintain
// the topology of references within an object graph being cloned.  For example,
// as registers are added to the system, this object keeps the mapping of old to new,
// so that the clone of something with a reference to a register can be switched
// to referenced the new cloned one.

internal final class CloningContext
{
    private var program_               : Program    = null;
    private var registers_             : Dictionary = new Dictionary();
    private var procedures_            : Dictionary = new Dictionary();
    private var parameters_            : Dictionary = new Dictionary();
    private var constants_             : Dictionary = new Dictionary();
    private var outputs_               : Dictionary = new Dictionary();
    private var vertexBuffers_         : Dictionary = new Dictionary();
    private var textures_              : Dictionary = new Dictionary();
    private var interpolateds_         : Dictionary = new Dictionary();
    private var cloningAnOutput_       : Boolean    = false;
    private var cloningAnInterpolated_ : Boolean    = false;

    public function CloningContext()
    {
    }

    internal function get cloningAnInterpolated() : Boolean
    {
        return cloningAnInterpolated_;
    }

    internal function set cloningAnInterpolated(value : Boolean) : void
    {
        cloningAnInterpolated_ = value;
    }

    internal function get cloningAnOutput() : Boolean
    {
        return cloningAnOutput_;
    }

    internal function set cloningAnOutput(value : Boolean) : void
    {
        cloningAnOutput_ = value;
    }

    internal function get constants() : Dictionary
    {
        return constants_;
    }

    internal function get interpolateds() : Dictionary
    {
        return interpolateds_;
    }

    internal function get outputs() : Dictionary
    {
        return outputs_;
    }

    internal function get parameters() : Dictionary
    {
        return parameters_;
    }

    internal function get procedures() : Dictionary
    {
        return procedures_;
    }

    internal function get program() : Program
    {
        return program_;
    }

    internal function set program(value : Program) : void
    {
        program_ = value;
    }

    internal function get registerMap() : Dictionary
    {
        return registers_;
    }

    internal function get textures() : Dictionary
    {
        return textures_;
    }

    internal function get vertexBuffers() : Dictionary
    {
        return vertexBuffers_;
    }

} // CloningContext
} // com.adobe.AGALOptimiser.agal