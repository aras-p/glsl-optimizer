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

package com.adobe.AGALOptimiser.type {
    
    import com.adobe.AGALOptimiser.error.Enforcer;
    import com.adobe.AGALOptimiser.nsinternal;
    
    use namespace nsinternal;
    
    [ExcludeClass]
    public class ArrayType extends Type
    {
        private var dimension_    : uint = undefined;
        private var elementType_  : Type = undefined;
        
        
        public function ArrayType(elementType : Type,
                                  dimension : uint = 0)
        {
            super();
            
            Enforcer.checkNull(elementType);
            if (dimension != 0)
                Enforcer.check(dimension > 1, "Dimension of array is at least 2");
            
            elementType_ = elementType;
            dimension_ = dimension;
        }
        
        override nsinternal function channelCount() : int
        {
            return elementType_.channelCount() * dimension_;
        }
        
        override nsinternal function clone() : Type
        {
            return new ArrayType(elementType_, dimension_);
        }
        
        nsinternal function get dimension() : int
        {
            return dimension_;
        }
        
        nsinternal function get elementType() : Type
        {
            return elementType_;
        }
        
        override nsinternal function equals(t : Type) : Boolean
        {
            return ((t != null) &&
                    (t is ArrayType) &&
                    (dimension_ == (t as ArrayType).dimension) &&
                    (elementType_.equals((t as ArrayType).elementType)));  
        }
        
        override nsinternal function getStage3DTypeFormatString() : String
        {
          return elementType_.getStage3DTypeFormatString();
        }
        
        override nsinternal function isArray() : Boolean
        {
            return true;
        }
             
        override nsinternal function serialize(indentLevel : int = 0,
                                                  flags : int = 0) : String
        {
            return "[" + dimension_ +  " x " + elementType_.serialize(0, flags) + "]";
        }
          
    } // ArrayType  
} // com.adobe.AGALOptimiser.type
