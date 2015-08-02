//
//  GLSLOptimizerBridge.m
//  glsl_optimizer_lib
//
//  Created by Ryan Walklin on 2/06/2015.
//
//

#import <Foundation/Foundation.h>

#import "GLSLOptimizerBridge.h"

#import "glsl_optimizer.h"

@implementation GLSLShaderVariableDescription

-(UInt32)elementCount {
    return self.vecSize * self.matSize * (self.arraySize == -1 ? 1 : self.arraySize);
}

-(UInt32)elementSize {
    UInt32 elementSize = 0;
    
    switch (self.type) {
        case GLSLOptBasicTypeFloat:
            elementSize = sizeof(float);
            break;
        case GLSLOptBasicTypeInt:
            elementSize = sizeof(int);
            break;
        case GLSLOptBasicTypeBool:
            elementSize = sizeof(bool);
            break;
        case GLSLOptBasicTypeTex2D:
        case GLSLOptBasicTypeTex3D:
        case GLSLOptBasicTypeTexCube:
            break;
        default:
            break;
    }
    return elementSize;
}
-(UInt32)rawSize {
    return [self elementCount] * [self elementSize];
}

@end

typedef NS_ENUM(NSUInteger, GLSLShaderVariableType) {
    GLSLShaderVariableTypeInput = 0,
    GLSLShaderVariableTypeUniform,
    GLSLShaderVariableTypeTexture
};

@interface GLSLShader () {
    
@private
    glslopt_shader *_shader;
}

-(id)initWithShader: (glslopt_shader *)shader;

-(GLSLShaderVariableDescription *)shaderVariableDescription:(GLSLShaderVariableType)type index:(UInt32)index;

@end

@implementation GLSLShader: NSObject

-(id)initWithShader: (glslopt_shader *)shader {
    self = [super init];
    if (self) {
        _shader = shader;
    }
    return self;
}

-(BOOL)status {
    return glslopt_get_status(_shader) == true;
}

-(NSString *)output {
    return [NSString stringWithUTF8String: glslopt_get_output(_shader)];
    
}

-(NSString *)rawOutput {
    return [NSString stringWithUTF8String: glslopt_get_raw_output(_shader)];
}

-(NSString *)log {
    return [NSString stringWithUTF8String: glslopt_get_log (_shader)];
}

-(UInt32)inputCount {
    return UInt32(glslopt_shader_get_input_count(_shader));
}

-(GLSLShaderVariableDescription *)inputDescription:(UInt32)index {
    NSAssert(index < self.inputCount, @"index < inputCount");
    return [self shaderVariableDescription:GLSLShaderVariableTypeInput index:index];
}

-(UInt32)uniformCount {
    return UInt32(glslopt_shader_get_uniform_count(_shader));
}

-(UInt32)uniformTotalSize {
    return UInt32(glslopt_shader_get_uniform_total_size(_shader));
}

-(GLSLShaderVariableDescription *)uniformDescription:(UInt32)index {
    NSAssert(index < self.uniformCount, @"index < inputCount");
    return [self shaderVariableDescription:GLSLShaderVariableTypeUniform index:index];
}

-(UInt32)textureCount {
    return UInt32(glslopt_shader_get_texture_count(_shader));
}

-(GLSLShaderVariableDescription *)textureDescription:(UInt32)index {
    NSAssert(index < self.textureCount, @"index < inputCount");
    return [self shaderVariableDescription:GLSLShaderVariableTypeTexture index:index];
}

-(GLSLShaderVariableDescription *)shaderVariableDescription:(GLSLShaderVariableType)type index:(UInt32)index {

    const char *outName = nil;
    glslopt_basic_type outType;
    glslopt_precision outPrec;
    int outVecSize;
    int outMatSize;
    int outArraySize;
    int outLocation;
    
    switch (type) {
        case GLSLShaderVariableTypeInput:
            glslopt_shader_get_input_desc(_shader, index, &outName, &outType, &outPrec, &outVecSize, &outMatSize, &outArraySize, &outLocation);
            break;
        case GLSLShaderVariableTypeUniform:
            glslopt_shader_get_uniform_desc(_shader, index, &outName, &outType, &outPrec, &outVecSize, &outMatSize, &outArraySize, &outLocation);
            break;
        case GLSLShaderVariableTypeTexture:
            glslopt_shader_get_texture_desc(_shader, index, &outName, &outType, &outPrec, &outVecSize, &outMatSize, &outArraySize, &outLocation);
            break;
    }
    
    GLSLShaderVariableDescription *varDesc = [[GLSLShaderVariableDescription alloc] init];

    varDesc.name = [NSString stringWithUTF8String:outName];
    varDesc.type = GLSLOptBasicType(outType);
    varDesc.prec = GLSLOptPrecision(outPrec);
    varDesc.vecSize = SInt32(outVecSize);
    varDesc.matSize = SInt32(outMatSize);
    varDesc.arraySize = SInt32(outArraySize);
    varDesc.location = SInt32(outLocation);
    
    return varDesc;
}

/*
 // Get *very* approximate shader stats:
 // Number of math, texture and flow control instructions.
 void glslopt_shader_get_stats (glslopt_shader* shader, int* approxMath, int* approxTex, int* approxFlow);
 */

- (void)dealloc
{
    glslopt_shader_delete(_shader);
}

@end

@interface GLSLOptimizer () {
    @private
    glslopt_ctx *_ctx;
}

@end

@implementation GLSLOptimizer

-(id)init:(GLSLOptTarget)target {
    self = [super init];
    if (self) {
        _ctx = glslopt_initialize(glslopt_target(target));
    }
    return self;
}

-(void)dealloc {
    glslopt_cleanup(_ctx);
}

-(void)setMaxUnrollIterations:(UInt32)iterations {
    glslopt_set_max_unroll_iterations(_ctx, iterations);
}

-(GLSLShader *)optimize:(GLSLOptShaderType)shaderType shaderSource:(NSString *)shaderSource options:(NSUInteger)options {
    glslopt_shader* shader = glslopt_optimize(_ctx, glslopt_shader_type(shaderType), shaderSource.UTF8String, UInt32(options));
    
    GLSLShader *glslShader = [[GLSLShader alloc] initWithShader: shader];
    
    if ([glslShader status]) {
        return glslShader;
    } else {
        NSLog(@"%@", [glslShader log]);
    }
    return nil;
}

@end


