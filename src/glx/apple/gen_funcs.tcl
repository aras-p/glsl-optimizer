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

# This is going to need to be updated for future OpenGL versions:
#    cat specs/gl.tm  | grep -v '^#' | awk -F, '{sub(/[ \t]+/, ""); print "    "$1 " \"" $4 "\""}'
#    then change void from "*" to "void"
#
# TextureComponentCount is GLenum in SL for everything
# It is GLint in mesa, but is GLenum for glTexImage3DEXT
array set typemap {
    AccumOp "GLenum"
    AlphaFunction "GLenum"
    AttribMask "GLbitfield"
    BeginMode "GLenum"
    BinormalPointerTypeEXT "GLenum"
    BlendEquationMode "GLenum"
    BlendEquationModeEXT "GLenum"
    BlendFuncSeparateParameterEXT "GLenum"
    BlendingFactorDest "GLenum"
    BlendingFactorSrc "GLenum"
    Boolean "GLboolean"
    BooleanPointer "GLboolean*"
    Char "GLchar"
    CharPointer "GLchar*"
    CheckedFloat32 "GLfloat"
    CheckedInt32 "GLint"
    ClampColorTargetARB "GLenum"
    ClampColorModeARB "GLenum"
    ClampedColorF "GLclampf"
    ClampedFloat32 "GLclampf"
    ClampedFloat64 "GLclampd"
    ClampedStencilValue "GLint"
    ClearBufferMask "GLbitfield"
    ClientAttribMask "GLbitfield"
    ClipPlaneName "GLenum"
    ColorB "GLbyte"
    ColorD "GLdouble"
    ColorF "GLfloat"
    ColorI "GLint"
    ColorIndexValueD "GLdouble"
    ColorIndexValueF "GLfloat"
    ColorIndexValueI "GLint"
    ColorIndexValueS "GLshort"
    ColorIndexValueUB "GLubyte"
    ColorMaterialParameter "GLenum"
    ColorPointerType "GLenum"
    ColorS "GLshort"
    ColorTableParameterPName "GLenum"
    ColorTableParameterPNameSGI "GLenum"
    ColorTableTarget "GLenum"
    ColorTableTargetSGI "GLenum"
    ColorUB "GLubyte"
    ColorUI "GLuint"
    ColorUS "GLushort"
    CombinerBiasNV "GLenum"
    CombinerComponentUsageNV "GLenum"
    CombinerMappingNV "GLenum"
    CombinerParameterNV "GLenum"
    CombinerPortionNV "GLenum"
    CombinerRegisterNV "GLenum"
    CombinerScaleNV "GLenum"
    CombinerStageNV "GLenum"
    CombinerVariableNV "GLenum"
    CompressedTextureARB "GLvoid"
    ControlPointNV "GLvoid"
    ControlPointTypeNV "GLenum"
    ConvolutionParameter "GLenum"
    ConvolutionParameterEXT "GLenum"
    ConvolutionTarget "GLenum"
    ConvolutionTargetEXT "GLenum"
    CoordD "GLdouble"
    CoordF "GLfloat"
    CoordI "GLint"
    CoordS "GLshort"
    CullFaceMode "GLenum"
    CullParameterEXT "GLenum"
    DepthFunction "GLenum"
    DrawBufferMode "GLenum"
    DrawBufferName "GLint"
    DrawElementsType "GLenum"
    ElementPointerTypeATI "GLenum"
    EnableCap "GLenum"
    ErrorCode "GLenum"
    EvalMapsModeNV "GLenum"
    EvalTargetNV "GLenum"
    FeedbackElement "GLfloat"
    FeedbackType "GLenum"
    FenceNV "GLuint"
    FenceConditionNV "GLenum"
    FenceParameterNameNV "GLenum"
    FfdMaskSGIX "GLbitfield"
    FfdTargetSGIX "GLenum"
    Float32 "GLfloat"
    Float32Pointer "GLfloat*"
    Float64 "GLdouble"
    Float64Pointer "GLdouble*"
    FogParameter "GLenum"
    FogPointerTypeEXT "GLenum"
    FogPointerTypeIBM "GLenum"
    FragmentLightModelParameterSGIX "GLenum"
    FragmentLightNameSGIX "GLenum"
    FragmentLightParameterSGIX "GLenum"
    FramebufferAttachment "GLenum"
    FramebufferTarget "GLenum"
    FrontFaceDirection "GLenum"
    FunctionPointer "_GLfuncptr"
    GetColorTableParameterPName "GLenum"
    GetColorTableParameterPNameSGI "GLenum"
    GetConvolutionParameterPName "GLenum"
    GetHistogramParameterPName "GLenum"
    GetHistogramParameterPNameEXT "GLenum"
    GetMapQuery "GLenum"
    GetMinmaxParameterPName "GLenum"
    GetMinmaxParameterPNameEXT "GLenum"
    GetPName "GLenum"
    GetPointervPName "GLenum"
    GetTextureParameter "GLenum"
    HintMode "GLenum"
    HintTarget "GLenum"
    HintTargetPGI "GLenum"
    HistogramTarget "GLenum"
    HistogramTargetEXT "GLenum"
    IglooFunctionSelectSGIX "GLenum"
    IglooParameterSGIX "GLvoid"
    ImageTransformPNameHP "GLenum"
    ImageTransformTargetHP "GLenum"
    IndexFunctionEXT "GLenum"
    IndexMaterialParameterEXT "GLenum"
    IndexPointerType "GLenum"
    Int16 "GLshort"
    Int32 "GLint"
    Int8 "GLbyte"
    InterleavedArrayFormat "GLenum"
    LightEnvParameterSGIX "GLenum"
    LightModelParameter "GLenum"
    LightName "GLenum"
    LightParameter "GLenum"
    LightTextureModeEXT "GLenum"
    LightTexturePNameEXT "GLenum"
    LineStipple "GLushort"
    List "GLuint"
    ListMode "GLenum"
    ListNameType "GLenum"
    ListParameterName "GLenum"
    LogicOp "GLenum"
    MapAttribParameterNV "GLenum"
    MapParameterNV "GLenum"
    MapTarget "GLenum"
    MapTargetNV "GLenum"
    MapTypeNV "GLenum"
    MaskedColorIndexValueF "GLfloat"
    MaskedColorIndexValueI "GLuint"
    MaskedStencilValue "GLuint"
    MaterialFace "GLenum"
    MaterialParameter "GLenum"
    MatrixIndexPointerTypeARB "GLenum"
    MatrixMode "GLenum"
    MatrixTransformNV "GLenum"
    MeshMode1 "GLenum"
    MeshMode2 "GLenum"
    MinmaxTarget "GLenum"
    MinmaxTargetEXT "GLenum"
    NormalPointerType "GLenum"
    NurbsCallback "GLenum"
    NurbsObj "GLUnurbs*"
    NurbsProperty "GLenum"
    NurbsTrim "GLenum"
    OcclusionQueryParameterNameNV "GLenum"
    PixelCopyType "GLenum"
    PixelFormat "GLenum"
    PixelInternalFormat "GLenum"
    PixelMap "GLenum"
    PixelStoreParameter "GLenum"
    PixelTexGenModeSGIX "GLenum"
    PixelTexGenParameterNameSGIS "GLenum"
    PixelTransferParameter "GLenum"
    PixelTransformPNameEXT "GLenum"
    PixelTransformTargetEXT "GLenum"
    PixelType "GLenum"
    PointParameterNameARB "GLenum"
    PolygonMode "GLenum"
    ProgramNV "GLuint"
    ProgramCharacterNV "GLubyte"
    ProgramParameterNV "GLenum"
    ProgramParameterPName "GLenum"
    QuadricCallback "GLenum"
    QuadricDrawStyle "GLenum"
    QuadricNormal "GLenum"
    QuadricObj "GLUquadric*"
    QuadricOrientation "GLenum"
    ReadBufferMode "GLenum"
    RenderbufferTarget "GLenum"
    RenderingMode "GLenum"
    ReplacementCodeSUN "GLuint"
    ReplacementCodeTypeSUN "GLenum"
    SamplePassARB "GLenum"
    SamplePatternEXT "GLenum"
    SamplePatternSGIS "GLenum"
    SecondaryColorPointerTypeIBM "GLenum"
    SelectName "GLuint"
    SeparableTarget "GLenum"
    SeparableTargetEXT "GLenum"
    ShadingModel "GLenum"
    SizeI "GLsizei"
    SpriteParameterNameSGIX "GLenum"
    StencilFunction "GLenum"
    StencilFaceDirection "GLenum"
    StencilOp "GLenum"
    StencilValue "GLint"
    String "const GLubyte *"
    StringName "GLenum"
    TangentPointerTypeEXT "GLenum"
    TessCallback "GLenum"
    TessContour "GLenum"
    TessProperty "GLenum"
    TesselatorObj "GLUtesselator*"
    TexCoordPointerType "GLenum"
    Texture "GLuint"
    TextureComponentCount "GLint"
    TextureCoordName "GLenum"
    TextureEnvParameter "GLenum"
    TextureEnvTarget "GLenum"
    TextureFilterSGIS "GLenum"
    TextureGenParameter "GLenum"
    TextureNormalModeEXT "GLenum"
    TextureParameterName "GLenum"
    TextureTarget "GLenum"
    TextureUnit "GLenum"
    UInt16 "GLushort"
    UInt32 "GLuint"
    UInt8 "GLubyte"
    VertexAttribEnum "GLenum"
    VertexAttribEnumNV "GLenum"
    VertexAttribPointerTypeNV "GLenum"
    VertexPointerType "GLenum"
    VertexWeightPointerTypeEXT "GLenum"
    Void "GLvoid"
    VoidPointer "GLvoid*"
    ConstVoidPointer "GLvoid* const"
    WeightPointerTypeARB "GLenum"
    WinCoord "GLint"
    void "void"
    ArrayObjectPNameATI "GLenum"
    ArrayObjectUsageATI "GLenum"
    ConstFloat32 "GLfloat"
    ConstInt32 "GLint"
    ConstUInt32 "GLuint"
    ConstVoid "GLvoid"
    DataTypeEXT "GLenum"
    FragmentOpATI "GLenum"
    GetTexBumpParameterATI "GLenum"
    GetVariantValueEXT "GLenum"
    ParameterRangeEXT "GLenum"
    PreserveModeATI "GLenum"
    ProgramFormatARB "GLenum"
    ProgramTargetARB "GLenum"
    ProgramTarget "GLenum"
    ProgramPropertyARB "GLenum"
    ProgramStringPropertyARB "GLenum"
    ScalarType "GLenum"
    SwizzleOpATI "GLenum"
    TexBumpParameterATI "GLenum"
    VariantCapEXT "GLenum"
    VertexAttribPointerPropertyARB "GLenum"
    VertexAttribPointerTypeARB "GLenum"
    VertexAttribPropertyARB "GLenum"
    VertexShaderCoordOutEXT "GLenum"
    VertexShaderOpEXT "GLenum"
    VertexShaderParameterEXT "GLenum"
    VertexShaderStorageTypeEXT "GLenum"
    VertexShaderTextureUnitParameter "GLenum"
    VertexShaderWriteMaskEXT "GLenum"
    VertexStreamATI "GLenum"
    PNTrianglesPNameATI "GLenum"
    BufferOffset "GLintptr"
    BufferSize "GLsizeiptr"
    BufferAccessARB "GLenum"
    BufferOffsetARB "GLintptrARB"
    BufferPNameARB "GLenum"
    BufferPointerNameARB "GLenum"
    BufferSizeARB "GLsizeiptrARB"
    BufferTargetARB "GLenum"
    BufferUsageARB "GLenum"
    ObjectTypeAPPLE "GLenum"
    VertexArrayPNameAPPLE "GLenum"
    DrawBufferModeATI "GLenum"
    Half16NV "GLhalfNV"
    PixelDataRangeTargetNV "GLenum"
    TypeEnum "GLenum"
    GLbitfield "GLbitfield"
    GLenum "GLenum"
    Int64 "GLint64"
    UInt64 "GLuint64"
    handleARB "GLhandleARB"
    charARB "GLcharARB"
    charPointerARB "GLcharARB*"
    sync "GLsync"
    Int64EXT "GLint64EXT"
    UInt64EXT "GLuint64EXT"
    FramebufferAttachment "GLenum"
    FramebufferAttachmentParameterName "GLenum"
    Framebuffer "GLuint"
    FramebufferStatus "GLenum"
    FramebufferTarget "GLenum"
    GetFramebufferParameter "GLenum"
    Intptr "GLintptr"
    ProgramFormat "GLenum"
    ProgramProperty "GLenum"
    ProgramStringProperty "GLenum"
    ProgramTarget "GLenum"
    Renderbuffer "GLuint"
    RenderbufferParameterName "GLenum"
    Sizeiptr "GLsizeiptr"
    TextureInternalFormat "GLenum"
    VertexBufferObjectAccess "GLenum"
    VertexBufferObjectParameter "GLenum"
    VertexBufferObjectUsage "GLenum"
    BufferAccessMask "GLbitfield"
    GetMultisamplePNameNV "GLenum"
    SampleMaskNV "GLbitfield"
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
	
	if {"array" eq [lindex $p 3]} {
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

