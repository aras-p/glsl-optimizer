#extension GL_ARB_uniform_buffer_object : enable
float xll_saturate( float x) {
  return clamp( x, 0.0, 1.0);
}
vec2 xll_saturate( vec2 x) {
  return clamp( x, 0.0, 1.0);
}
vec3 xll_saturate( vec3 x) {
  return clamp( x, 0.0, 1.0);
}
vec4 xll_saturate( vec4 x) {
  return clamp( x, 0.0, 1.0);
}
mat2 xll_saturate(mat2 m) {
  return mat2( clamp(m[0], 0.0, 1.0), clamp(m[1], 0.0, 1.0));
}
mat3 xll_saturate(mat3 m) {
  return mat3( clamp(m[0], 0.0, 1.0), clamp(m[1], 0.0, 1.0), clamp(m[2], 0.0, 1.0));
}
mat4 xll_saturate(mat4 m) {
  return mat4( clamp(m[0], 0.0, 1.0), clamp(m[1], 0.0, 1.0), clamp(m[2], 0.0, 1.0), clamp(m[3], 0.0, 1.0));
}
layout(column_major) uniform;
uniform PER_BATCH {
    vec4 SSAO_params;
    vec4 SSAO_VOParams;
    vec4 PS_NearFarClipDist;
};
struct pixout_cl {
    vec4 Color;
};
struct vert2fragSSAO {
    vec4 HPosition;
    vec4 ScreenTC;
};
layout (binding = 0 ) uniform sampler2D depthTargetSampler;
layout (binding = 4 ) uniform sampler2D sRotSampler4x4_VO;
layout (binding = 1 ) uniform sampler2D sceneDepthSamplerAO;
float GetUnnormalizedLinearDownscaledDepth( in vec4 rawDepth );
float GetUnnormalizedLinearDownscaledDepth( in sampler2D depthSmp, in vec2 screenCoord );
float SSAO_GetDownscaledDepth( in sampler2D depthSampler, in vec2 uv );
vec3 GetSampleCoords( in int iSampleID, in mat2 mTransMat );
float GetLinearDepth( in float fDevDepth, in bool bDrawNearAdjust, in bool bScaled );
float GetLinearDepth( in sampler2D depthSampler, in vec2 ScreenTC );
vec3 ComputeVO( in vec4 vScreenTC );
pixout_cl Deferred_SSAO_VOJitter_PS( in vert2fragSSAO IN );
float GetUnnormalizedLinearDownscaledDepth( in vec4 rawDepth ) {
    return rawDepth.x ;
}
float GetUnnormalizedLinearDownscaledDepth( in sampler2D depthSmp, in vec2 screenCoord ) {
    vec4 data;
    data = texture2D( depthSmp, screenCoord);
    return GetUnnormalizedLinearDownscaledDepth( data);
}
float SSAO_GetDownscaledDepth( in sampler2D depthSampler, in vec2 uv ) {
    float ret;
    ret = GetUnnormalizedLinearDownscaledDepth( depthSampler, uv);
    return ret;
}
vec3 GetSampleCoords( in int iSampleID, in mat2 mTransMat ) {
    vec2 arrKernel[60];
    arrKernel[0] = vec2( 0.494679, 0.340907);
    arrKernel[1] = vec2( 0.340907, -0.494679);
    arrKernel[2] = vec2( -0.494679, -0.340907);
    arrKernel[3] = vec2( -0.340907, 0.494679);
    arrKernel[4] = vec2( 0.000000, 0.000000);
    arrKernel[5] = vec2( 0.000000, 0.000000);
    arrKernel[6] = vec2( 0.000000, 0.000000);
    arrKernel[7] = vec2( 0.000000, 0.000000);
    arrKernel[8] = vec2( 0.000000, 0.000000);
    arrKernel[9] = vec2( 0.000000, 0.000000);
    arrKernel[10] = vec2( 0.000000, 0.000000);
    arrKernel[11] = vec2( 0.000000, 0.000000);
    arrKernel[12] = vec2( 0.000000, 0.000000);
    arrKernel[13] = vec2( 0.000000, 0.000000);
    arrKernel[14] = vec2( 0.000000, 0.000000);
    arrKernel[15] = vec2( 0.000000, 0.000000);
    arrKernel[16] = vec2( 0.000000, 0.000000);
    arrKernel[17] = vec2( 0.000000, 0.000000);
    arrKernel[18] = vec2( 0.000000, 0.000000);
    arrKernel[19] = vec2( 0.000000, 0.000000);
    arrKernel[20] = vec2( 0.000000, 0.000000);
    arrKernel[21] = vec2( 0.000000, 0.000000);
    arrKernel[22] = vec2( 0.000000, 0.000000);
    arrKernel[23] = vec2( 0.000000, 0.000000);
    arrKernel[24] = vec2( 0.000000, 0.000000);
    arrKernel[25] = vec2( 0.000000, 0.000000);
    arrKernel[26] = vec2( 0.000000, 0.000000);
    arrKernel[27] = vec2( 0.000000, 0.000000);
    arrKernel[28] = vec2( 0.000000, 0.000000);
    arrKernel[29] = vec2( 0.000000, 0.000000);
    arrKernel[30] = vec2( 0.000000, 0.000000);
    arrKernel[31] = vec2( 0.000000, 0.000000);
    arrKernel[32] = vec2( 0.000000, 0.000000);
    arrKernel[33] = vec2( 0.000000, 0.000000);
    arrKernel[34] = vec2( 0.000000, 0.000000);
    arrKernel[35] = vec2( 0.000000, 0.000000);
    arrKernel[36] = vec2( 0.000000, 0.000000);
    arrKernel[37] = vec2( 0.000000, 0.000000);
    arrKernel[38] = vec2( 0.000000, 0.000000);
    arrKernel[39] = vec2( 0.000000, 0.000000);
    arrKernel[40] = vec2( 0.000000, 0.000000);
    arrKernel[41] = vec2( 0.000000, 0.000000);
    arrKernel[42] = vec2( 0.000000, 0.000000);
    arrKernel[43] = vec2( 0.000000, 0.000000);
    arrKernel[44] = vec2( 0.000000, 0.000000);
    arrKernel[45] = vec2( 0.000000, 0.000000);
    arrKernel[46] = vec2( 0.000000, 0.000000);
    arrKernel[47] = vec2( 0.000000, 0.000000);
    arrKernel[48] = vec2( 0.000000, 0.000000);
    arrKernel[49] = vec2( 0.000000, 0.000000);
    arrKernel[50] = vec2( 0.000000, 0.000000);
    arrKernel[51] = vec2( 0.000000, 0.000000);
    arrKernel[52] = vec2( 0.000000, 0.000000);
    arrKernel[53] = vec2( 0.000000, 0.000000);
    arrKernel[54] = vec2( 0.000000, 0.000000);
    arrKernel[55] = vec2( 0.000000, 0.000000);
    arrKernel[56] = vec2( 0.000000, 0.000000);
    arrKernel[57] = vec2( 0.000000, 0.000000);
    arrKernel[58] = vec2( 0.000000, 0.000000);
    arrKernel[59] = vec2( 0.000000, 0.000000);
    vec3 vSampleCoords;
    vSampleCoords.xy  = ( mTransMat * arrKernel[ iSampleID ] );
    vSampleCoords.z  = (1.00000 - dot( vSampleCoords.xy , vSampleCoords.xy ));
    return vSampleCoords;
}
float GetLinearDepth( in float fDevDepth, in bool bDrawNearAdjust, in bool bScaled ) {
    if ( bScaled ){
        return (fDevDepth * PS_NearFarClipDist.y );
    }
    return fDevDepth;
}
float GetLinearDepth( in sampler2D depthSampler, in vec2 ScreenTC ) {
    float fDepth;
    fDepth = texture2D( depthSampler, ScreenTC.xy ).x ;
    return GetLinearDepth( fDepth, true, false);
}
vec3 ComputeVO( in vec4 vScreenTC ) {
    vec4 vJitteringData;
    float fJitterRadius;
    mat2 mTransMat;
    int iKernelOffset = 0;
    int iSamplePackets = 1;
    float fCenterDepth;
    vec2 vTSRadius;
    float fDSRadius;
    vec4 fNearDepthRatio;
    vec4 vSkyAccess;
    vec4 vZSum;
    int iSamplePacket = 0;
    vec3 vIrrSample;
    vec4 arrSampledDepth;
    vec4 arrSampleZ;
    vec4 vTransformedDepth;
    vec4 fObscurance;
    vec4 vTransCoef;
    vec4 vFadeOut;
    float fAO;
    float fVariance;
    vJitteringData = texture2D( sRotSampler4x4_VO, vScreenTC.zw );
    fJitterRadius = vJitteringData.z ;
    mTransMat = mat2( vJitteringData.y , vJitteringData.x , ( -vJitteringData.x  ), vJitteringData.y );
    fCenterDepth = GetLinearDepth( depthTargetSampler, vScreenTC.xy );
    vTSRadius = SSAO_VOParams.xy ;
    fDSRadius = (max( fCenterDepth, 0.00100000) * fJitterRadius);
    fNearDepthRatio = vec4( 1.00000);
    vSkyAccess = vec4( 0.000000);
    vZSum = vec4( 0.000000);
    for ( ; (iSamplePacket < iSamplePackets); ( iSamplePacket++ )) {
        vIrrSample = GetSampleCoords( (iKernelOffset + (iSamplePacket * 4)), mTransMat);
        vIrrSample.xy  *= vTSRadius;
        arrSampledDepth.x  = SSAO_GetDownscaledDepth( sceneDepthSamplerAO, (vScreenTC.xy  + vIrrSample.xy ));
        arrSampleZ.x  = vIrrSample.z ;
        vIrrSample = GetSampleCoords( ((iKernelOffset + (iSamplePacket * 4)) + 1), mTransMat);
        vIrrSample.xy  *= vTSRadius;
        arrSampledDepth.y  = SSAO_GetDownscaledDepth( sceneDepthSamplerAO, (vScreenTC.xy  + vIrrSample.xy ));
        arrSampleZ.y  = vIrrSample.z ;
        vIrrSample = GetSampleCoords( ((iKernelOffset + (iSamplePacket * 4)) + 2), mTransMat);
        vIrrSample.xy  *= vTSRadius;
        arrSampledDepth.z  = SSAO_GetDownscaledDepth( sceneDepthSamplerAO, (vScreenTC.xy  + vIrrSample.xy ));
        arrSampleZ.z  = vIrrSample.z ;
        vIrrSample = GetSampleCoords( ((iKernelOffset + (iSamplePacket * 4)) + 3), mTransMat);
        vIrrSample.xy  *= vTSRadius;
        arrSampledDepth.w  = SSAO_GetDownscaledDepth( sceneDepthSamplerAO, (vScreenTC.xy  + vIrrSample.xy ));
        arrSampleZ.w  = vIrrSample.z ;
        vTransformedDepth = ((arrSampledDepth - fCenterDepth) / fDSRadius);
        fObscurance = max( (min( arrSampleZ, vTransformedDepth) + arrSampleZ), vec4( 0.000000));
        vTransCoef = xll_saturate( ((0.200000 * (vTransformedDepth + arrSampleZ)) + 1.00000) );
        vFadeOut = (vTransCoef * vTransCoef);
        vSkyAccess += mix( arrSampleZ, fObscurance, vFadeOut);
        vZSum += arrSampleZ;
        if ( (iSamplePackets > 1) ){
            fNearDepthRatio = min( fNearDepthRatio, ((arrSampledDepth * PS_NearFarClipDist.y ) - 1.20000));
        }
        else{
            fNearDepthRatio = ((arrSampledDepth * PS_NearFarClipDist.y ) - 1.20000);
        }
    }
    fNearDepthRatio.xy  = min( fNearDepthRatio.xy , fNearDepthRatio.zw );
    fNearDepthRatio.x  = min( fNearDepthRatio.x , fNearDepthRatio.y );
    vSkyAccess = mix( vZSum, vSkyAccess, vec4( xll_saturate( (fNearDepthRatio.x  + 0.700000) )));
    fAO = dot( vSkyAccess, vec4( (1.00000 / dot( vZSum, vec4( 1.00000)))));
    fVariance = (1.00000 - fAO);
    fAO += (fVariance * float( (abs( fVariance ) < 0.0500000) ));
    fAO *= SSAO_params.y ;
    fAO = (xll_saturate( (1.00000 - fAO) ) * SSAO_params.x );
    return vec3( xll_saturate( (1.00000 - fAO) ), fCenterDepth, xll_saturate( fNearDepthRatio.x  ));
}
pixout_cl Deferred_SSAO_VOJitter_PS( in vert2fragSSAO IN ) {
    pixout_cl OUT;
    vec2 vAODepth;
    OUT = pixout_cl( vec4( 0.000000, 0.000000, 0.000000, 0.000000));
    vAODepth = ComputeVO( IN.ScreenTC).xy ;
    OUT.Color = vec4( vAODepth.x );
    return OUT;
}
layout(location = 9 ) varying vec4 xlv_TEXCOORD0;
void main() {
    pixout_cl xl_retval;
    vert2fragSSAO xlt_IN;
    xlt_IN.HPosition = vec4(0.0);
    xlt_IN.ScreenTC = vec4( xlv_TEXCOORD0);
    xl_retval = Deferred_SSAO_VOJitter_PS( xlt_IN);
    gl_FragData[0] = vec4( xl_retval.Color);
}
