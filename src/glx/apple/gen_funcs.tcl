package require Tcl 8.5

#input is specs/gl.spec

set license {
/*
 Copyright (c) 2008, 2009 Apple Inc.
 
 Permission is hereby granted, free of charge, to any person
 obtaining a copy of this software and associated documentation files
 (the "Software"), to deal in the Software without restriction,
 including without limitation the rights to use, copy, modify, merge,
 publish, distribute, sublicense, and/or sell copies of the Software,
 and to permit persons to whom the Software is furnished to do so,
 subject to the following conditions:
 
 The above copyright notice and this permission notice shall be
 included in all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT.  IN NO EVENT SHALL THE ABOVE LISTED COPYRIGHT
 HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 DEALINGS IN THE SOFTWARE.
 
 Except as contained in this notice, the name(s) of the above
 copyright holders shall not be used in advertising or otherwise to
 promote the sale, use or other dealings in this Software without
 prior written authorization.
*/
}


proc extension name {
    global extensions

    set extensions($name) 1
}

proc alias {from to} {
    global aliases
    
    set aliases($from) $to
}

proc promoted name {
    global promoted

    set promoted($name) 1
}

proc noop name {
    global noop
    
    set noop($name) 1
}

set dir [file dirname [info script]]

source [file join $dir GL_extensions]
source [file join $dir GL_aliases]
source [file join $dir GL_promoted]
source [file join $dir GL_noop]

proc is-extension-supported? name {
    global extensions

    return [info exists extensions($name)]
}

#This is going to need to be updated for OpenGL >= 2.1 in SnowLeopard.
array set typemap {
    void void
    List GLuint
    Mode GLenum
    CheckedFloat32 GLfloat
    CheckedInt32 GLint
    Float32 GLfloat
    Int32 GLint
    Int64 GLint64EXT
    UInt64 GLuint64EXT
    Float64 GLdouble
    ListMode GLuint
    SizeI GLsizei
    ListNameType GLenum
    Void void
    BeginMode GLenum
    CoordF GLfloat
    UInt8 GLubyte
    Boolean GLboolean
    ColorIndexValueD GLdouble
    ColorB GLbyte
    ColorD GLdouble
    ColorF GLfloat
    ColorI GLint
    ColorS GLshort
    ColorUB GLubyte
    ColorUI GLuint
    ColorUS GLushort
    ColorIndexValueF GLfloat
    ColorIndexValueI GLint
    ColorIndexValueS GLshort
    Int8 GLbyte
    CoordD GLdouble
    Int16 GLshort
    CoordI GLint
    CoordS GLshort
    ClipPlaneName GLenum
    MaterialFace GLenum
    ColorMaterialParameter GLenum
    CullFaceMode GLenum
    FogParameter GLenum
    FrontFaceDirection GLenum
    HintTarget GLenum
    HintMode GLenum
    LightName GLenum
    LightParameter GLenum
    LightModelParameter GLenum
    LineStipple GLushort
    MaterialParameter GLenum
    PolygonMode GLenum
    WinCoord GLint
    ShadingModel GLenum
    TextureTarget GLenum
    TextureParameterName GLenum
    TextureComponentCount GLenum
    PixelFormat GLenum
    PixelType GLenum
    TextureEnvTarget GLenum
    TextureEnvParameter GLenum
    TextureCoordName GLenum
    TextureGenParameter GLenum
    FeedbackType GLenum
    FeedbackElement GLfloat
    SelectName GLuint
    RenderingMode GLenum
    DrawBufferMode GLenum
    ClearBufferMask GLbitfield
    MaskedColorIndexValueF GLfloat
    ClampedColorF GLclampf
    StencilValue GLint
    ClampedFloat64 GLclampd
    MaskedStencilValue GLuint
    MaskedColorIndexValueI GLuint
    AccumOp GLenum
    EnableCap GLenum
    AttribMask GLbitfield
    MapTarget GLenum
    MeshMode1 GLenum
    MeshMode2 GLenum
    AlphaFunction GLenum
    ClampedFloat32 GLclampf
    BlendingFactorSrc GLenum
    BlendingFactorDest GLenum
    LogicOp GLenum
    StencilFunction GLenum
    ClampedStencilValue GLint
    MaskedStencilValue GLuint
    StencilOp GLenum
    DepthFunction GLenum
    PixelTransferParameter GLenum
    PixelStoreParameter GLenum
    PixelMap GLenum
    UInt32 GLuint
    UInt16 GLushort
    ReadBufferMode GLenum
    PixelCopyType GLenum
    GetPName GLenum
    ErrorCode GLenum
    GetMapQuery GLenum
    String "const GLubyte *"
    StringName GLenum
    GetTextureParameter GLenum
    MatrixMode GLenum
    ColorPointerType GLenum
    DrawElementsType GLenum
    GetPointervPName GLenum
    VoidPointer "void *"
    IndexPointerType GLenum
    InterleavedArrayFormat GLenum
    NormalPointerType GLenum
    TexCoordPointerType GLenum
    VertexPointerType GLenum
    PixelInternalFormat GLenum
    Texture GLuint
    ColorIndexValueUB GLubyte
    ClientAttribMask GLbitfield
    BlendEquationMode GLenum
    ColorTableTarget GLenum
    ColorTableParameterPName GLenum
    GetColorTableParameterPName GLenum
    ConvolutionTarget GLenum
    ConvolutionParameter GLenum
    GetConvolutionParameterPName GLenum
    SeparableTarget GLenum
    HistogramTarget GLenum
    GetHistogramParameterPName GLenum
    MinmaxTarget GLenum
    GetMinmaxParameterPName GLenum
    TextureTarget GLenum
    TextureUnit GLenum
    CompressedTextureARB "void"
    BlendFuncSeparateParameterEXT GLenum
    FogPointerTypeEXT GLenum
    PointParameterNameARB GLenum
    GLenum GLenum
    BufferTargetARB GLenum
    ConstUInt32 "const GLuint"
    BufferSize GLsizeiptr
    ConstVoid "const GLvoid"
    BufferUsageARB GLenum
    BufferOffset GLintptr
    BufferAccessARB GLenum
    BufferPNameARB GLenum
    BufferPointerNameARB GLenum
    BlendEquationModeEXT GLenum
    DrawBufferModeATI GLenum
    StencilFaceDirection GLenum
    Char GLchar
    VertexAttribPropertyARB GLenum
    VertexAttribPointerPropertyARB GLenum
    CharPointer "GLchar *"
    VertexAttribPointerTypeARB GLenum
    ClampColorTargetARB unknown3.0
    ClampColorModeARB unknown3.0
    VertexAttribEnum GLenum
    VertexAttribEnumNV GLenum
    DrawBufferName unknown3.0
    WeightPointerTypeARB GLenum
    ProgramTargetARB GLenum
    ProgramFormatARB GLenum
    ProgramStringPropertyARB GLenum
    BufferSizeARB GLsizeiptrARB
    BufferOffsetARB GLintptrARB
    handleARB GLhandleARB
    charPointerARB "GLcharARB *"
    charARB GLcharARB
    RenderbufferTarget GLenum
    FramebufferTarget GLenum
    FramebufferAttachment GLenum
    BinormalPointerTypeEXT GLenum
    HintTargetPGI GLenum
    ProgramParameterPName GLenum
    ProgramPropertyARB GLenum
    ElementPointerTypeATI GLenum
    FenceNV GLuint
    FenceConditionNV GLenum
    ObjectTypeAPPLE GLenum
    VertexArrayPNameAPPLE GLenum
    SeparableTargetEXT GLenum
    ColorTableTargetSGI GLenum
    ColorTableParameterPNameSGI GLenum
    CombinerOutputNV GLenum
    CombinerStageNV GLenum
    CombinerPortionNV GLenum
    CombinerRegisterNV GLenum
    CombinerScaleNV GLenum
    CombinerBiasNV GLenum
    CombinerComponentUsageNV GLenum
    CombinerMappingNV GLenum
    CombinerParameterNV GLenum
    CombinerVariableNV GLenum
    ConvolutionParameterEXT GLenum
    ConvolutionTargetEXT GLenum
    CullParameterEXT GLenum
    FenceParameterNameNV GLenum
    FragmentLightModelParameterSGIX GLenum
    FragmentLightNameSGIX GLenum
    FragmentLightParameterSGIX GLenum
    GetColorTableParameterPNameSGI GLenum
    GetHistogramParameterPNameEXT GLenum
    GetMinmaxParameterPNameEXT GLenum
    HistogramTargetEXT GLenum
    LightEnvParameterSGIX GLenum
    MinmaxTargetEXT GLenum
    PNTrianglesPNameATI GLenum
    ProgramCharacterNV GLubyte
    SamplePatternEXT GLenum
    SamplePatternSGIS GLenum
    TypeEnum GLenum
}

proc psplit s {
    set r [list]
    set token ""

    foreach c [split $s ""] {
	if {[string is space -strict $c]} {
	    if {[string length $token]} {
		lappend r $token
	    }
	    set token ""
	} else {
	    append token $c
	}
    }

    if {[string length $token]} {
	lappend r $token
    }

    return $r
}

proc is-extension? str {
    #Check if the trailing name of the function is NV, or EXT, and so on.
    
    if {[string is upper [string index $str end]]
	&& [string is upper [string index $str end-1]]} {
	return 1
    } 

    return 0
}


proc parse {data arvar} {
    upvar 1 $arvar ar

    set state ""
    set name ""

    foreach line [split $data \n] {
	if {"attr" eq $state} {
	    if {[string match "\t*" $line]} {
		set plist [psplit [lindex [split $line "#"] 0]]
		#puts PLIST:$plist
		set master $ar($name)
		set param [dict get $master parameters]

		switch -- [llength $plist] {
		    1 {
			dict set master [lindex $plist 0] ""
		    }

		    2 {
			#standard key, value pair
			set key [lindex $plist 0]
			set value [lindex $plist 1]

			dict set master $key $value
		    }

		    default {
			set key [lindex $plist 0]

			#puts PLIST:$plist

			if {"param" eq $key} {
			    lappend param [list [lindex $plist 1] [lindex $plist 2] [lindex $plist 3] [lindex $plist 4]]
			} else {
			    dict set master $key [lrange $plist 1 end]
			}
		    }		    
		}
		
		dict set master parameters $param

		set ar($name) $master
	    } else {
		set state ""
	    }
	} elseif {[regexp {^([A-Z_a-z0-9]+)\((.*)\)\S*} $line all name argv] > 0} {
	    #puts "MATCH:$name ARGV:$argv"
	    
	    #Trim the spaces in the elements.
	    set newargv [list]
	    foreach a [split $argv ,] {
		lappend newargv [string trim $a]
	    }
	    

	    set d [dict create name $name arguments $newargv \
		       parameters [dict create]]
	    set ar($name) $d
	    set state attr
	} 
    }
}

#This returns true if the category is valid for an extension.
proc is-valid-category? c {
    set clist [list display-list drawing drawing-control feedback framebuf misc modeling pixel-op pixel-rw state-req xform VERSION_1_0 VERSION_1_0_DEPRECATED VERSION_1_1 VERSION_1_1_DEPRECATED VERSION_1_2 VERSION_1_2_DEPRECATED VERSION_1_3 VERSION_1_3_DEPRECATED VERSION_1_4 VERSION_1_4_DEPRECATED VERSION_1_5 VERSION_2_0 VERSION_2_1 VERSION_3_0 VERSION_3_0_DEPRECATED VERSION_3_1 VERSION_3_2]

    set result [expr {$c in $clist}]


    if {!$result} {
	set result [is-extension-supported? $c]
    }

    return $result
}

proc translate-parameters {func parameters} {
    global typemap

    set newparams [list]

    foreach p $parameters {
	set var [lindex $p 0]
	
	set ptype [lindex $p 1]
	
	if {![info exists typemap($ptype)]} {
	    set ::missingTypes($ptype) $func
	    continue
	}
	
	set type $typemap($ptype)
	
	#In the gl.spec file is MultiDrawArrays first and count
	#are really 'in' so we make them const.
	#The gl.spec notes this problem.
	if {("MultiDrawArrays" eq $func) && ("first" eq $var)} {
	    set final_type "const $type *"
	} elseif {("MultiDrawArrays" eq $func) && ("count" eq $var)} {
	    set final_type "const $type *"
	} elseif {"array" eq [lindex $p 3]} {
	    if {"in" eq [lindex $p 2]} {
		set final_type "const $type *"
	    } else {
		set final_type "$type *"
	    }
	} else {
	    set final_type $type
	}
	    
	lappend newparams [list $final_type $var]
    }
 
    return $newparams
}

proc api-new-entry {info func} {
    global typemap

    set master [dict create]
    set rettype [dict get $info return]
    
    if {![info exists typemap($rettype)]} {
	set ::missingTypes($rettype) $func
    } else {
	dict set master return $typemap($rettype)
    }
    
    dict set master parameters [translate-parameters $func \
				    [dict get $info parameters]]

    return $master
}

proc main {argc argv} {
    global extensions typemap aliases promoted noop

    set fd [open [lindex $argv 0] r]
    set data [read $fd]
    close $fd
  
    array set ar {}
    
    parse $data ar
    
    array set newapi {}
    array set missingTypes {}

    foreach {key value} [array get ar] {
	puts "KEY:$key VALUE:$value"

	set category [dict get $value category]
	
	#Invalidate any of the extensions and things not in the spec we support.
	set valid [is-valid-category? $category]
	puts VALID:$valid
	
	if {!$valid} {
	   continue
	}

	puts "VALID:$key"

	if {"BlitFramebuffer" eq $key} {
	    #This was promoted to an ARB extension after Leopard it seems.
	    set key BlitFramebufferEXT
	}

	if {"ARB_framebuffer_object" eq $category} {
	    #This wasn't an ARB extension in Leopard either.
	    if {![string match *EXT $key]} {
		append key EXT
	    }
	}

	set newapi($key) [api-new-entry $value $key]
    }

    #Now iterate and support as many aliases as we can for promoted functions
    #based on if the newapi contains the function.
    foreach {func value} [array get ar] {
	if {![info exists promoted([dict get $value category])]} {
	    continue
	}

	if {[dict exists $value alias]} {
	    #We have an alias.  Let's see if we have the implementation.
	    set alias [dict get $value alias]

	    if {[info exists newapi($alias)] && ![info exists newapi($func)]} {
		#We have an implementing function available.
		puts "HAVE:$key ALIAS:$alias"

		set master [api-new-entry $value $func]
		dict set master alias_for $alias
		set newapi($func) $master		
	    }
	}
    } 

    parray noop

    #Now handle the no-op compatibility categories.
    foreach {func value} [array get ar] {
	if {[info exists noop([dict get $value category])]} {
	    if {[info exists newapi($func)]} {
		puts stderr "$func shouldn't be a noop"
		exit 1
	    }
	    
	    set master [api-new-entry $value $func]
	    dict set master noop 1
	    set newapi($func) $master
	}
    }

    

    parray newapi

    if {[array size ::missingTypes]} {
	parray ::missingTypes
	return 1
    }

    foreach {from to} [array get aliases] {
	set d $newapi($to)
	dict set d alias_for $to
	set newapi($from) $d
    }

    #Iterate the nm output and set each symbol in an associative array.
    array set validapi {}

    foreach line [split [exec nm -j -g /System/Library/Frameworks/OpenGL.framework/Libraries/libGL.dylib] \n] {
	set fn [string trim $line]

	#Only match the _gl functions.
	if {[string match _gl* $fn]} {
	    set finalfn [string range $fn 3 end]
	    puts FINALFN:$finalfn
	    set validapi($finalfn) $finalfn
	}
    }

    puts "Correcting the API functions to match the OpenGL framework."
    #parray validapi
    
    #Iterate the newapi and unset any members that the
    #libGL.dylib doesn't support, assuming they aren't no-ops.
    foreach fn [array names newapi] {
	if {![info exists validapi($fn)]} {
	    puts "WARNING: libGL.dylib lacks support for: $fn"

	    if {[dict exists $newapi($fn) noop] 
		&& [dict get $newapi($fn) noop]} {
		#This is no-op, so we should skip it.
		continue
	    }

	    #Is the function an alias for another in libGL?
	    if {[dict exists $newapi($fn) alias_for]} {
		set alias [dict get $newapi($fn) alias_for]

		if {![info exists validapi($alias)]} {
		    puts "WARNING: alias function doesn't exist for $fn."
		    puts "The alias is $alias."
		    puts "unsetting $fn"		    
		    unset newapi($fn)
		} 
	    } else {
		puts "unsetting $fn"
		unset newapi($fn)
	    }
	}
    }

    
    #Now print a warning about any symbols that libGL supports that we don't.
    foreach fn [array names validapi] {
	if {![info exists newapi($fn)]} {
	    puts "AppleSGLX is missing $fn"
	}
    }

    puts "NOW GENERATING:[lindex $::argv 1]"
    set fd [open [lindex $::argv 1] w]

    set sorted [lsort -dictionary [array names newapi]]

    foreach f $sorted {
	set attr $newapi($f)
	set pstr ""
	foreach p [dict get $attr parameters] {
	    append pstr "[lindex $p 0] [lindex $p 1], "
	}
	set pstr [string trimright $pstr ", "]
	puts $fd "[dict get $attr return] gl[set f]($pstr); "
    }

    close $fd

    if {$::argc == 3} {
	puts "NOW GENERATING:[lindex $::argv 2]"
	#Dump the array as a serialized list.
	set fd [open [lindex $::argv 2] w]
	puts $fd [array get newapi]
	close $fd
    }

    return 0
}
exit [main $::argc $::argv]

