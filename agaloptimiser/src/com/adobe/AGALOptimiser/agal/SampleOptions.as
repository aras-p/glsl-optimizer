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

import com.adobe.AGALOptimiser.error.Enforcer;
import com.adobe.AGALOptimiser.error.ErrorMessages;
import com.adobe.AGALOptimiser.nsinternal;
import com.adobe.AGALOptimiser.utils.SerializationFlags;
import com.adobe.AGALOptimiser.utils.SerializationUtils;

use namespace nsinternal;

[ExcludeClass]
public final class SampleOptions
{
    private       static const SAMPLER_FILTER_SHIFT    : uint = 28;
    private       static const SAMPLER_MIPMAP_SHIFT    : uint = 24;
    private       static const SAMPLER_WRAPPING_SHIFT  : uint = 20;
    private       static const SAMPLER_DIMENSION_SHIFT : uint = 12;

    nsinternal static const SAMPLE_FILTER_NEAREST   : uint = (0 << SAMPLER_FILTER_SHIFT);
    nsinternal static const SAMPLE_FILTER_LINEAR    : uint = (1 << SAMPLER_FILTER_SHIFT);

    nsinternal static const SAMPLE_MIP_DISABLE      : uint = (0 << SAMPLER_MIPMAP_SHIFT);
    nsinternal static const SAMPLE_MIP_NEAREST      : uint = (1 << SAMPLER_MIPMAP_SHIFT);
    nsinternal static const SAMPLE_MIP_LINEAR       : uint = (2 << SAMPLER_MIPMAP_SHIFT);

    nsinternal static const SAMPLE_WRAPPING_CLAMP   : uint = (0 << SAMPLER_WRAPPING_SHIFT);
    nsinternal static const SAMPLE_WRAPPING_REPEAT  : uint = (1 << SAMPLER_WRAPPING_SHIFT);

    nsinternal static const SAMPLE_DIMENSION_2D     : uint = (0 << SAMPLER_DIMENSION_SHIFT);
    nsinternal static const SAMPLE_DIMENSION_CUBE   : uint = (1 << SAMPLER_DIMENSION_SHIFT);
    nsinternal static const SAMPLE_DIMENSION_3D     : uint = (2 << SAMPLER_DIMENSION_SHIFT);

    nsinternal static const SAMPLE_FILTER_MASK      : uint = (0xf << SAMPLER_FILTER_SHIFT);
    nsinternal static const SAMPLE_MIP_MASK         : uint = (0xf << SAMPLER_MIPMAP_SHIFT);
    nsinternal static const SAMPLE_WRAPPING_MASK    : uint = (0xf << SAMPLER_WRAPPING_SHIFT);
    nsinternal static const SAMPLE_DIMENSION_MASK   : uint = (0xf << SAMPLER_DIMENSION_SHIFT);

    private var options_ : uint = 0;

    public function SampleOptions(options : uint)
    {
        options_ = options;
    }

    nsinternal function get code() : uint
    {
        return options_;
    }

    nsinternal function clone() : SampleOptions
    {
        return new SampleOptions(options_);
    }

    nsinternal function serialize(indentLevel : uint = 0,
                                     flags : uint = 0) : String
    {
        var s : String = SerializationUtils.indentString(indentLevel);

       	s += "<"

        if ((options_ & SAMPLE_FILTER_MASK) == SAMPLE_FILTER_NEAREST) 
            s += Constants.SAMPLE_FILTER_NEAREST_TAG + " ";
        else if ((options_ & SAMPLE_FILTER_MASK) == SAMPLE_FILTER_LINEAR) 
            s += Constants.SAMPLE_FILTER_LINEAR_TAG + " ";
        else
            throw new Error(ErrorMessages.UNSUPPORTED_SAMPLING_FLAGS);
        
        if ((options_ & SAMPLE_MIP_MASK) == SAMPLE_MIP_NEAREST) 
            s += Constants.SAMPLE_MIP_NEAREST_TAG + " ";
        else if ((options_ & SAMPLE_MIP_MASK) == SAMPLE_MIP_DISABLE) 
            s += Constants.SAMPLE_MIP_DISABLE_TAG + " ";
        else if ((options_ & SAMPLE_MIP_MASK) == SAMPLE_MIP_LINEAR) 
            s += Constants.SAMPLE_MIP_LINEAR_TAG + " ";
        else
            throw new Error(ErrorMessages.UNSUPPORTED_SAMPLING_FLAGS);

        if ((options_ & SAMPLE_WRAPPING_MASK) == SAMPLE_WRAPPING_CLAMP) 
            s += Constants.SAMPLE_WRAPPING_CLAMP_TAG + " ";
        else if ((options_ & SAMPLE_WRAPPING_MASK) == SAMPLE_WRAPPING_REPEAT) 
            s += Constants.SAMPLE_WRAPPING_REPEAT_TAG + " ";
        else
            throw new Error(ErrorMessages.UNSUPPORTED_SAMPLING_FLAGS);
        
        if ((options_ & SAMPLE_DIMENSION_MASK) == SAMPLE_DIMENSION_2D) 
        {
           	s += Constants.SAMPLE_DIMENSION_2D_TAG;
        }
        else if ((options_ & SAMPLE_DIMENSION_MASK) == SAMPLE_DIMENSION_CUBE) 
            s += Constants.SAMPLE_DIMENSION_CUBE_TAG;
        else if ((options_ & SAMPLE_DIMENSION_MASK) == SAMPLE_DIMENSION_3D) 
        {
        	s += Constants.SAMPLE_DIMENSION_3D_TAG;
        }
        else
            throw new Error(ErrorMessages.UNSUPPORTED_SAMPLING_FLAGS);

        s += ">";

        return s;
    }

    nsinternal static function isSampleOption(option : String) : Boolean
    {
        var b : Boolean = 
            option == Constants.SAMPLE_FILTER_NEAREST_TAG ||
            option == Constants.SAMPLE_FILTER_LINEAR_TAG ||
            option == Constants.SAMPLE_MIP_DISABLE_TAG ||
            option == Constants.SAMPLE_MIP_NEAREST_TAG ||
            option == Constants.SAMPLE_MIP_LINEAR_TAG ||
            option == Constants.SAMPLE_WRAPPING_CLAMP_TAG ||
            option == Constants.SAMPLE_WRAPPING_REPEAT_TAG ||
			option == Constants.SAMPLE_WRAPPING_WRAP_TAG ||
            option == Constants.SAMPLE_DIMENSION_2D_TAG ||
            option == Constants.SAMPLE_DIMENSION_CUBE_TAG ||
            option == Constants.SAMPLE_DIMENSION_3D_TAG;
        
        return b;
    }

    nsinternal static function sampleOptionToValue(option : String) : uint
    {
        if (option == Constants.SAMPLE_FILTER_NEAREST_TAG)
            return SAMPLE_FILTER_NEAREST;
        if (option == Constants.SAMPLE_FILTER_LINEAR_TAG)
            return SAMPLE_FILTER_LINEAR;
        if (option == Constants.SAMPLE_MIP_NEAREST_TAG)
            return SAMPLE_MIP_NEAREST;
        if (option == Constants.SAMPLE_MIP_DISABLE_TAG)
            return SAMPLE_MIP_DISABLE;
        if (option == Constants.SAMPLE_MIP_LINEAR_TAG)
            return SAMPLE_MIP_LINEAR;
        if (option == Constants.SAMPLE_WRAPPING_CLAMP_TAG)
            return SAMPLE_WRAPPING_CLAMP;
        if (option == Constants.SAMPLE_WRAPPING_REPEAT_TAG)
            return SAMPLE_WRAPPING_REPEAT;
		if (option == Constants.SAMPLE_WRAPPING_WRAP_TAG)
			return SAMPLE_WRAPPING_REPEAT;
        if (option == Constants.SAMPLE_DIMENSION_2D_TAG)
            return SAMPLE_DIMENSION_2D;
        if (option == Constants.SAMPLE_DIMENSION_CUBE_TAG)
            return SAMPLE_DIMENSION_CUBE;
        if (option == Constants.SAMPLE_DIMENSION_3D_TAG)
            return SAMPLE_DIMENSION_3D;

        Enforcer.check(false, ErrorMessages.INVALID_OPTION);

        return SAMPLE_FILTER_NEAREST;
    }
} // SampleCategory
} // com.adobe.AGALOptimiser.asm
