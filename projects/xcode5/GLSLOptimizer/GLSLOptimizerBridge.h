//
//  GLSLOptimizerBridge.h
//  glsl_optimizer_lib
//
//  Created by Ryan Walklin on 2/06/2015.
//
//

@class GLSLShader;

#import <Foundation/Foundation.h>

typedef NS_ENUM(NSUInteger, GLSLOptShaderType) {
    GLSLOptShaderTypeVertex = 0, // kGlslOptShaderVertex
    GLSLOptShaderTypeFragment // kGlslOptShaderFragment
}; // glslopt_shader_type

// Options flags for glsl_optimize
typedef NS_ENUM(NSUInteger, GLSLOptOptions) {
    GLSLOptOptionsSkipProcessor = 0, //kGlslOptionSkipPreprocessor = (1<<0), // Skip preprocessing shader source. Saves some time if you know you don't need it.
    GLSLOptOptionsNotFullShader // kGlslOptionNotFullShader = (1<<1), // Passed shader is not the full shader source. This makes some optimizations weaker.
}; // glslopt_options

// Optimizer target language
typedef NS_ENUM(NSUInteger, GLSLOptTarget) {
    GLSLOptTargetOpenGL = 0, // kGlslTargetOpenGL = 0,
    GLSLOptTargetOpenGLES20 = 1, // kGlslTargetOpenGLES20 = 1,
    GLSLOptTargetOpenGLES30 = 2,// kGlslTargetOpenGLES30 = 2,
    GLSLOptTargetMetal = 3// kGlslTargetMetal = 3,
}; // glslopt_target

// Type info
typedef NS_ENUM(NSUInteger, GLSLOptBasicType) {
    GLSLOptBasicTypeFloat = 0, // kGlslTypeFloat = 0,
    GLSLOptBasicTypeInt, // kGlslTypeInt,
    GLSLOptBasicTypeBool, // kGlslTypeBool,
    GLSLOptBasicTypeTex2D, // kGlslTypeTex2D,
    GLSLOptBasicTypeTex3D, // kGlslTypeTex3D,
    GLSLOptBasicTypeTexCube, // kGlslTypeTexCube,
    GLSLOptBasicTypeOther, // kGlslTypeOther,
    GLSLOptBasicTypeCount // kGlslTypeCount
}; // glslopt_basic_type

typedef NS_ENUM(NSUInteger, GLSLOptPrecision) {
    GLSLOptPrecisionHigh = 0, // kGlslPrecHigh = 0,
    GLSLOptPrecisionMedium, // kGlslPrecMedium,
    GLSLOptPrecisionLow, // kGlslPrecLow,
    GLSLOptPrecisionCount // kGlslPrecCount
}; // glslopt_precision

@interface GLSLShaderVariableDescription : NSObject

@property UInt32 index;
@property NSString *name;
@property GLSLOptBasicType type;
@property GLSLOptPrecision prec;
@property UInt32 vecSize;
@property UInt32 matSize;
@property SInt32 arraySize;
@property UInt32 location;

-(UInt32)elementCount;
-(UInt32)elementSize;
-(UInt32)rawSize;

@end

@interface GLSLShader: NSObject

-(BOOL)status;

-(NSString *)output;
-(NSString *)rawOutput;
-(NSString *)log;
-(UInt32)inputCount;
-(GLSLShaderVariableDescription *)inputDescription:(UInt32)index;

-(UInt32)uniformCount;
-(UInt32)uniformTotalSize;
-(GLSLShaderVariableDescription *)uniformDescription:(UInt32)index;

-(UInt32)textureCount;
-(GLSLShaderVariableDescription *)textureDescription:(UInt32)index;

@end

@interface GLSLOptimizer: NSObject

-(id)init:(GLSLOptTarget)target;

-(void)setMaxUnrollIterations:(UInt32)iterations;

-(GLSLShader *)optimize:(GLSLOptShaderType)shaderType shaderSource:(NSString *)shaderSource options:(NSUInteger)options;

@end



/*

int glslopt_shader_get_uniform_count (glslopt_shader* shader);
int glslopt_shader_get_uniform_total_size (glslopt_shader* shader);
void glslopt_shader_get_uniform_desc (glslopt_shader* shader, int index, const char** outName, glslopt_basic_type* outType, glslopt_precision* outPrec, int* outVecSize, int* outMatSize, int* outArraySize, int* outLocation);
int glslopt_shader_get_texture_count (glslopt_shader* shader);
void glslopt_shader_get_texture_desc (glslopt_shader* shader, int index, const char** outName, glslopt_basic_type* outType, glslopt_precision* outPrec, int* outVecSize, int* outMatSize, int* outArraySize, int* outLocation);

// Get *very* approximate shader stats:
// Number of math, texture and flow control instructions.
void glslopt_shader_get_stats (glslopt_shader* shader, int* approxMath, int* approxTex, int* approxFlow);

*/

