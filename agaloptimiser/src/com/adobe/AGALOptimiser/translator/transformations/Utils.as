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

import com.adobe.AGALOptimiser.agal.AgalParser;
import com.adobe.AGALOptimiser.agal.ConstantValue;
import com.adobe.AGALOptimiser.agal.Program;
import com.adobe.AGALOptimiser.agal.Register;
import com.adobe.AGALOptimiser.agal.RegisterCategory;
import com.adobe.AGALOptimiser.nsinternal;
import com.adobe.AGALOptimiser.utils.SerializationFlags;

import flash.geom.Transform;

use namespace nsinternal;

public final class Utils
{
    public function Utils()
    {
    }
    
    public static function generateOptimizationMix1() : TransformationSequenceManager
    {
        var transformations : Vector.<TransformationManager> = new Vector.<TransformationManager>();
        var subordinate     : TransformationIterationManager = null;

        transformations.push(new TransformationSingletonManager(StandardPeepholeOptimizer));
        transformations.push(new TransformationSingletonManager(DeadCodeEliminator));
        transformations.push(new TransformationSingletonManager(IdentityMoveRemover));
        transformations.push(new TransformationSingletonManager(MoveChainReducer));
		//transformations.push(new TransformationSingletonManager(SourceToSinkReorganizer));
        //transformations.push(new TransformationSingletonManager(DeadCodeEliminator));
        
        subordinate = new TransformationIterationManager(new TransformationSequenceManager(transformations),
                                                         10);
        
        transformations.length = 0;
        transformations.push(new TransformationSingletonManager(DeadCodeEliminator));
        transformations.push(subordinate);
    
        return new TransformationSequenceManager(transformations);
    }
	
	public static function optimizeShader(shader:Object, isVS:Boolean ):Object
	{
		var s:String
		var test:AgalParser = new AgalParser();
		var agalsrc:String =
			(isVS ? "vertex" : "fragment") + "_program version (1)\n" +
			"registers\n"
			
		var glslType:String
		var agalType:String
		var swiz:String
		var arraySize:int
		var idx:int
		
		for(s in shader.varnames) {
			if(s.indexOf("hoisted_") != 0) {
				var loc:String = shader.varnames[s];
				glslType = shader.types[loc];
				swiz = "";
				arraySize = 1;
				if(glslType.indexOf("[") >= 0) {
					idx = glslType.indexOf("[");
					arraySize = int(glslType.slice(idx+1, glslType.length-1))
					glslType = glslType.slice(0, idx);
				}
				switch(glslType) {
					case "bool":
					case "int": 
					case "float": // agalType = "float"; swiz = ".x"; break;
					case "ivec2":
					case "ivec3":
					case "ivec4":
					case "vec2": 
					case "vec3": 
					case "vec4": {
						if(arraySize > 1)
							agalType = "[" + arraySize + " x float-4]";
						else
							agalType = "float-4";
						
						break;
					}
					case "mat3": agalType = "[3 x float-4]"; break;
					case "mat4": agalType = "[4 x float-4]"; break;
					case "sampler2D": agalType = "image-4"; break;
					case "samplerCube": agalType = "image-4"; break;
					default: throw "Unhandled glslType: " + glslType;
				}
				
				if(loc.indexOf("va") == 0 ) {
					agalsrc += agalType + " " + loc + swiz + " vertex(" + s + " " + loc.slice(2) + ")\n"
				} else if(loc.indexOf("vc") == 0 || loc.indexOf("fc") == 0) {
					agalsrc += agalType + " " + loc + swiz + " parameter(" + s + ")\n"
				} else if(loc.indexOf("fs") == 0) {
					agalsrc += agalType + " " + loc + " texture(" + s + " " + loc.slice(2) + ")\n"
				} else if(loc.indexOf("v") == 0 ) {
					agalsrc += "float-4 " + loc + " interpolated(" + s + ")\n"
				}
			}
		}
		for(s in shader.types) {
			glslType = shader.types[s];
			swiz = "";
			arraySize = 1;
			if(glslType.indexOf("[") >= 0) {
				idx = glslType.indexOf("[");
				arraySize = int(glslType.slice(idx+1, glslType.length-1))
				glslType = glslType.slice(0, idx);
			}
			switch(glslType) {
				case "bool":
				case "int": 
				case "float": // agalType = "float"; swiz = ".x"; break;
				case "ivec2":
				case "ivec3":
				case "ivec4":
				case "vec2": 
				case "vec3": 
				case "vec4": {
					if(arraySize > 1)
						agalType = "[" + arraySize + " x float-4]";
					else
						agalType = "float-4";
					
					break;
				}
				case "mat3": agalType = "[3 x float-4]"; break;
				case "mat4": agalType = "[4 x float-4]"; break;
				case "sampler2D": agalType = "image-4"; break;
				case "samplerCube": agalType = "image-4"; break;
				default: throw "Unhandled glslType: " + glslType;
			}
			
			if(s.indexOf("vt") == 0 || s.indexOf("ft") == 0) {
				agalsrc += agalType + " " + s + swiz + "\n"
			}
		}
		for(s in shader.consts) {
			agalsrc += "float-4 " + s + " const(" + shader.consts[s][0] + ", " + shader.consts[s][1] + ", "  + shader.consts[s][2] + ", "  + shader.consts[s][3] + ")\n"
		}
		
		if(isVS)
			agalsrc += "float-4 op output(gl_Position)\n"
		else
			agalsrc += "float-4 oc output(gl_FragColor)\n"
		
		agalsrc += "end_registers\nprocedure main\noperations\n"
		agalsrc += (shader.agalasm as String);
		agalsrc += "\nend_operations\nend_procedure\nend_"+(isVS ? "vertex" : "fragment")+"_program\n"

		var vp:Program = test.parse(agalsrc);
		vp.main = vp.procedures[0];
		var tsm:TransformationSequenceManager = Utils.generateOptimizationMix1();
		var i:int = tsm.transformProgram(vp);
		var ra:RegisterAssigner = new RegisterAssigner(vp);
		
		var newshader:Object = {};
		newshader["info"] = "";
		newshader["agalasm"] = vp.serialize(0, SerializationFlags.DISPLAY_AGAL_MINIASM_FORM);
		newshader["varnames"] = {}
		newshader["types"] = {}
		newshader["storage"] = {}
		newshader["consts"] = {}
		
		var unnamed:int = 0;
		
		for each(var reg:Register in vp.registers) {
			var agalRegType:String = RegisterCategory.registerCategoryToString(reg.category, SerializationFlags.DISPLAY_AGAL_MINIASM_FORM);
			var agalRegName:String = agalRegType + reg.baseNumber;
			var oldAgalRegName:String = shader["varnames"][reg.name];
			
			if(reg.name)
				newshader["varnames"][reg.name] = agalRegName;
			else
				newshader["varnames"]["unnamed_" + unnamed++] = agalRegName;
			
			var cv:ConstantValue = reg.valueInfo as ConstantValue;
			if(cv)
				newshader["consts"][agalRegName] = [cv.getChannelAsFloat(0), cv.getChannelAsFloat(1), cv.getChannelAsFloat(2), cv.getChannelAsFloat(3)];
			
			newshader["types"][agalRegName] = shader["types"][oldAgalRegName];
			newshader["storage"][agalRegName] = shader["storage"][oldAgalRegName];
		}

		return newshader;
	}
    
} // Utils
} // com.adobe.AGALOptimiser.translator.transformations