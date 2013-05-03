#extension GL_EXT_Cafe : enable
#extension GL_ARB_uniform_buffer_object : enable
#extension GL_ARB_shader_texture_lod : require
vec4 xll_tex2Dlod(sampler2D s, vec4 coord) {
   return texture2DLod( s, coord.xy, coord.w);
}
vec4 xll_texCUBElod(samplerCube s, vec4 coord) {
  return textureCubeLod( s, coord.xyz, coord.w);
}
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
uniform PER_LIGHT {
    vec4 LDiffuses0;
    vec4 WorldLightsPos0;
    vec4 ShadowChanMasks0;
    vec4 LDiffuses1;
    vec4 WorldLightsPos1;
    vec4 ShadowChanMasks1;
    vec4 LDiffuses2;
    vec4 WorldLightsPos2;
    vec4 ShadowChanMasks2;
    vec4 LDiffuses3;
    vec4 WorldLightsPos3;
    vec4 ShadowChanMasks3;
};
uniform PER_INSTANCE {
    vec4 Ambient;
    float NumInstructions;
    vec4 AvgFogVolumeContrib;
    vec4 AlphaTest;
    vec4 SnowVolumeParams[2];
};
uniform PER_MATERIAL {
    vec4 MatDifColor;
    vec4 MatSpecColor;
    vec4 MatEmissiveColor;
    vec4 padding_4_3;
    vec3 _0FresnelScale_1FresnelPower_2FresnelBias_3;
    vec4 padding_4_5;
    vec4 padding_4_6;
    vec3 _0_1_2SoftIntersectionFactor_3;
};
uniform PER_FRAME {
    vec4 padding_3_0;
    vec4 padding_3_1;
    vec4 g_PS_SunLightDir;
};
struct pixout {
    vec4 Color;
};
struct fragInput {
    vec4 baseTC;
    vec4 basesectorTC;
    vec4 bumpTC;
    vec4 terrainDetailTC;
    vec4 vTangent;
    vec4 vBinormal;
    vec4 vNormal;
    vec4 vView;
    vec4 screenProj;
    vec4 projTC;
    vec4 Color;
    vec4 Color1;
    vec4 VisTerrainCol;
    vec4 SunRefl;
    vec4 Ambient;
    vec4 AlphaTest;
    vec3 DistAtten;
    vec4 AvgFogVolumeContrib;
    vec4 OutdoorAOInfo;
    vec4 blendVector;
    float viewFade;
    vec4 irisTC;
    vec3 viewTS;
    vec3 offsetuv;
};
struct fragPassCustom {
    vec3 vTangent;
    vec3 vBinormal;
    float fSpecMultiplier;
    vec2 vSurfaceRoughness;
};
struct fragPass {
    fragInput IN;
    bool bCustomComposition;
    bool bRenormalizeNormal;
    bool bForceRenormalizeNormal;
    bool bDontUseBump;
    bool bDetailBumpMapping;
    bool bDetailBumpMappingMasking;
    bool bVertexColors;
    bool bAlphaGlow;
    bool bHemisphereLighting;
    bool bDontUseEmissive;
    bool bRefractionMap;
    bool bSkinIrradiancePass;
    bool bDisableInShadowShading;
    bool bDisableAlphaTestD3D10;
    bool bDeferredSpecularShading;
    bool bSkipMaterial;
    int nReflectionMapping;
    int nMicroDetailQuality;
    float fBumpHeightScale;
    float fHeightBias;
    float fSelfShadowStrength;
    vec2 vDetailBumpTilling;
    vec2 vDetailBlendAmount;
    float fDetailBumpScale;
    float fLod;
    float blendFac;
    int nQuality;
    mat3 mTangentToWS;
    vec3 vView;
    vec3 vNormal;
    vec3 vNormalDiffuse;
    vec3 vReflVec;
    vec3 cBumpMap;
    vec4 cDiffuseMap;
    vec4 cGlossMap;
    vec4 cEnvironment;
    vec4 cShadowOcclMap;
    vec4 cSnowMap;
    float fNdotE;
    float fGloss;
    float fAlpha;
    float fAlphaTestRef;
    vec4 cNormalMapRT;
    vec4 cDiffuseAccRT;
    vec4 cSpecularAccRT;
    float fSceneDepthRT;
    vec2 vSceneUVsRT;
    float fFresnel_Bias;
    float fFresnel_Scale;
    float fFresnel_Pow;
    float fFresnel;
    float fAlphaGlow_Multiplier;
    vec3 cAmbientAcc;
    vec3 cDiffuseAcc;
    vec3 cBackDiffuseAcc;
    vec3 cSpecularAcc;
    vec4 vNormalNoise;
    vec4 blendVector;
    float blendFacH;
    float viewFade;
    vec4 irisTC;
    vec3 offsetuv;
    fragPassCustom pCustom;
};
struct fragLightPassCustom {
    vec4 Dummy;
};
struct fragLightPass {
    int nType;
    vec3 cDiffuse;
    vec3 cSpecular;
    vec3 cFilter;
    vec3 vLight;
    float fNdotL;
    float fFallOff;
    float fOcclShadow;
    vec3 cOut;
    fragLightPassCustom pCustom;
};
struct vert2FragGeneral {
    vec4 HPosition;
    vec4 baseTC;
    vec4 vTangent;
    vec4 vBinormal;
    vec4 vView;
    vec4 screenProj;
    vec4 projTC;
    vec4 AvgFogVolumeContrib;
    vec4 Ambient;
};
layout (binding = 1 ) uniform sampler2D bumpMapSampler;
layout (binding = 2 ) uniform sampler2D detailMapSampler;
layout (binding = 0 ) uniform sampler2D diffuseMapSampler;
layout (binding = 3 ) uniform sampler2D envMapSampler;
layout (binding = 4 ) uniform samplerCube envMapSamplerCUBE;
layout (binding = 5 ) uniform sampler2D sceneSnowMapSampler;
void brb_frag_unify_parameters( inout fragPass pPass );
void brb_frag_unify( inout fragPass pPass, in vert2FragGeneral IN );
int GetShaderQuality(  );
void frag_quality_setup( inout fragPass pPass );
void frag_hdr_setup( inout fragPass pPass, inout vec4 cOut );
void ComputeGlobalFogPS( inout vec3 cOut, in float fDist );
void frag_fog_setup( inout fragPass pPass, inout vec4 cOut );
void brb_frag_custom_ambient( inout fragPass pPass, inout vec3 cAmbient );
void frag_ambient( inout fragPass pPass, in vec3 vNormal );
void brb_frag_final_composition( inout fragPass pPass, inout vec3 cFinal );
void brb_frag_custom_end( inout fragPass pPass, inout vec3 cFinal );
void brb_frag_custom_begin( inout fragPass pPass );
vec3 GetBumpFromTextureColor( in vec4 color );
vec2 EXPAND( in vec2 a );
vec3 GetNormalMapFromTextureColor( in vec4 color );
vec3 GetNormalMap( in sampler2D bumpMap, in vec2 bumpTC );
vec4 DecodeRGBK( in vec4 Color, in float fMultiplier, in bool bUsePPP );
vec4 GetEnvironmentCMap( in samplerCube envMap, in vec3 envTC, in float fGloss );
vec4 GetEnvironment2DMap( in sampler2D envMap, in vec2 envTC );
vec4 EXPAND( in vec4 a );
vec4 DVColor( in vec2 d );
void DebugOutput( out vec4 Color, in vec4 baseTC );
vec4 brb_frag_shared_output( inout fragPass pPass );
void HDROutput( out pixout OUT, in vec4 Color, in float fDepth );
pixout brb_BeamPS( in vert2FragGeneral IN );
void brb_frag_unify_parameters( inout fragPass pPass ) {
    pPass.bRenormalizeNormal = true;
    pPass.bForceRenormalizeNormal = true;
    pPass.bHemisphereLighting = true;
    pPass.bDisableInShadowShading = true;
    pPass.bDeferredSpecularShading = true;
    pPass.fFresnel_Bias = _0FresnelScale_1FresnelPower_2FresnelBias_3.z ;
    pPass.fFresnel_Scale = _0FresnelScale_1FresnelPower_2FresnelBias_3.x ;
    pPass.fFresnel_Pow = _0FresnelScale_1FresnelPower_2FresnelBias_3.y ;
}
void brb_frag_unify( inout fragPass pPass, in vert2FragGeneral IN ) {
    pPass.nQuality = GetShaderQuality( );
    pPass.fFresnel_Bias = 1.00000;
    pPass.fFresnel_Scale = 0.000000;
    pPass.fFresnel_Pow = 4.00000;
    pPass.IN.baseTC = IN.baseTC;
    pPass.IN.bumpTC = pPass.IN.baseTC;
    pPass.IN.Ambient = IN.Ambient;
    pPass.IN.vTangent = vec4( 1.00000, 0.000000, 0.000000, 1.00000);
    pPass.IN.vBinormal = vec4( 0.000000, 1.00000, 0.000000, 1.00000);
    pPass.IN.vNormal.xyz  = vec3( vec4( 0.000000, 0.000000, 1.00000, 1.00000));
    pPass.IN.vTangent = IN.vTangent;
    pPass.IN.vBinormal = IN.vBinormal;
    pPass.IN.vNormal.xyz  = (cross( IN.vTangent.xyz , IN.vBinormal.xyz ) * IN.vTangent.w );
    pPass.IN.screenProj = IN.screenProj;
    pPass.IN.vView = IN.vView;
    pPass.IN.projTC = IN.projTC;
    pPass.IN.AvgFogVolumeContrib = IN.AvgFogVolumeContrib;
    brb_frag_unify_parameters( pPass);
}
int GetShaderQuality(  ) {
    int nQuality;
    nQuality = 1;
    return nQuality;
}
void frag_quality_setup( inout fragPass pPass ) {
    pPass.nQuality = GetShaderQuality( );
    if ( (pPass.nQuality == 0) ){
        pPass.bRenormalizeNormal = pPass.bForceRenormalizeNormal;
        pPass.nReflectionMapping = 0;
        pPass.bDetailBumpMapping = false;
    }
}
void frag_hdr_setup( inout fragPass pPass, inout vec4 cOut ) {
    if ( ( !pPass.bSkinIrradiancePass ) ){
        cOut.xyz  *= g_PS_SunLightDir.w ;
    }
}
void ComputeGlobalFogPS( inout vec3 cOut, in float fDist ) {
    cOut.xyz  = mix( vec3( 0.000000, 0.000000, 0.000000), cOut.xyz , vec3( fDist));
}
void frag_fog_setup( inout fragPass pPass, inout vec4 cOut ) {
    ComputeGlobalFogPS( cOut.xyz , pPass.IN.vView.w );
    cOut.xyz  = (pPass.IN.AvgFogVolumeContrib.xyz  + (cOut.xyz  * pPass.IN.AvgFogVolumeContrib.w ));
    cOut.w  *= pPass.IN.vView.w ;
}
void brb_frag_custom_ambient( inout fragPass pPass, inout vec3 cAmbient ) {
    pPass.cAmbientAcc.xyz  += cAmbient.xyz ;
}
void frag_ambient( inout fragPass pPass, in vec3 vNormal ) {
    vec3 amb;
    float fBlendFactor;
    amb = pPass.IN.Ambient.xyz ;
    if ( pPass.bHemisphereLighting ){
        fBlendFactor = ((vNormal.z  * 0.250000) + 0.750000);
        amb.xyz  *= fBlendFactor;
    }
    brb_frag_custom_ambient( pPass, amb);
}
void brb_frag_final_composition( inout fragPass pPass, inout vec3 cFinal ) {
    vec3 cDiffuse;
    vec3 cSpecularCol;
    cDiffuse = pPass.cDiffuseMap.xyz ;
    if ( (pPass.bDontUseEmissive == false) ){
        cDiffuse.xyz  *= MatEmissiveColor.xyz ;
    }
    cSpecularCol = MatSpecColor.xyz ;
    cFinal.xyz  = cDiffuse;
}
void brb_frag_custom_end( inout fragPass pPass, inout vec3 cFinal ) {
}
void brb_frag_custom_begin( inout fragPass pPass ) {
    vec4 baseTC;
    vec4 bumpTC;
    float fAlpha;
    float geoSoftIntersectionFactor;
    float depth;
    float softIntersect;
    baseTC = pPass.IN.baseTC;
    bumpTC = pPass.IN.bumpTC;
    pPass.cGlossMap = vec4( 1.00000);
    fAlpha = pPass.cDiffuseMap.w ;
    if ( pPass.bVertexColors ){
        fAlpha *= pPass.IN.Color.w ;
    }
    else{
        if ( pPass.bAlphaGlow ){
            fAlpha = pPass.IN.Color.w ;
        }
    }
    pPass.fAlpha = (fAlpha * pPass.IN.Ambient.w );
    geoSoftIntersectionFactor = _0_1_2SoftIntersectionFactor_3.z ;
    depth = (pPass.fSceneDepthRT - pPass.IN.screenProj.w );
    softIntersect = xll_saturate( (geoSoftIntersectionFactor * min( depth, pPass.IN.screenProj.w )) );
    pPass.fAlpha *= softIntersect;
}
vec3 GetBumpFromTextureColor( in vec4 color ) {
    vec3 bump;
    bump.xy  = color.yx ;
    bump.z  = 1.00000;
    return bump;
}
vec2 EXPAND( in vec2 a ) {
    return ((a * 2.00000) - 1.00000);
}
vec3 GetNormalMapFromTextureColor( in vec4 color ) {
    vec3 normal;
    normal.xy  = GetBumpFromTextureColor( color).xy ;
    normal.xy  = EXPAND( normal.xy );
    normal.z  = sqrt( xll_saturate( (1.00000 + dot( normal.xy , ( -normal.xy  ))) ) );
    return normal;
}
vec3 GetNormalMap( in sampler2D bumpMap, in vec2 bumpTC ) {
    return GetNormalMapFromTextureColor( texture2D( bumpMap, bumpTC.xy ));
}
vec4 DecodeRGBK( in vec4 Color, in float fMultiplier, in bool bUsePPP ) {
    if ( bUsePPP ){
        Color.xyz  *= ((Color.w  * Color.w ) * fMultiplier);
    }
    else{
        Color.xyz  *= (Color.w  * fMultiplier);
    }
    return Color;
}
vec4 GetEnvironmentCMap( in samplerCube envMap, in vec3 envTC, in float fGloss ) {
    float numCMapMips = 6.00000;
    float fGlossinessLod;
    vec4 envColor;
    fGlossinessLod = (numCMapMips - (fGloss * numCMapMips));
    envColor = DecodeRGBK( xll_texCUBElod( envMap, vec4( envTC, fGlossinessLod)), 16.0000, false);
    return envColor;
}
vec4 GetEnvironment2DMap( in sampler2D envMap, in vec2 envTC ) {
    vec4 envColor;
    envColor = texture2D( envMap, envTC.xy );
    return envColor;
}
vec4 EXPAND( in vec4 a ) {
    return ((a * 2.00000) - 1.00000);
}
vec4 DVColor( in vec2 d ) {
    float Reso = 512.000;
    float TargetDeriv;
    float HalfTD;
    float TwoTD;
    vec4 dd = vec4( 0.000000, 0.000000, 0.000000, 1.00000);
    TargetDeriv = (1.00000 / Reso);
    HalfTD = (TargetDeriv * 0.500000);
    TwoTD = (TargetDeriv * 2.00000);
    if ( (d.x  > TwoTD) ){
        dd.x  = 1.00000;
    }
    if ( (d.y  > TwoTD) ){
        dd.y  = 1.00000;
    }
    if ( (d.x  < HalfTD) ){
        dd.z  = 1.00000;
    }
    return dd;
}
void DebugOutput( out vec4 Color, in vec4 baseTC ) {
    Color = vec4( 0.000000);
    Color = DVColor( abs( dFdy( baseTC.xy  ) ));
}
vec4 brb_frag_shared_output( inout fragPass pPass ) {
    vec4 cOut;
    vec2 microDetailBaseTC;
    float dispAmount;
    float blendLayer2Tiling = 0.000000;
    float blendHeightScale = 0.000000;
    vec2 vDetailTC;
    vec4 cDetailMap;
    vec2 vDetailN;
    vec2 vBlendAmount;
    vec3 vAmbientNormal;
    cOut = vec4( 0.000000);
    frag_quality_setup( pPass);
    microDetailBaseTC = pPass.IN.baseTC.xy ;
    dispAmount = pPass.fBumpHeightScale;
    pPass.vView = normalize( ( -pPass.IN.vView.xyz  ) );
    pPass.mTangentToWS = mat3( pPass.IN.vTangent.xyz , pPass.IN.vBinormal.xyz , pPass.IN.vNormal.xyz );
    pPass.cDiffuseMap = texture2D( diffuseMapSampler, pPass.IN.baseTC.xy );
    if ( ( !pPass.bDontUseBump ) ){
        pPass.cBumpMap = GetNormalMap( bumpMapSampler, pPass.IN.bumpTC.xy );
    }
    else{
        pPass.cBumpMap = vec3( 0.000000, 0.000000, 1.00000);
    }
    pPass.vNormalDiffuse = pPass.cBumpMap;
    vDetailTC = (pPass.vDetailBumpTilling * pPass.IN.baseTC.xy );
    cDetailMap = texture2D( detailMapSampler, vDetailTC);
    if ( pPass.bDetailBumpMapping ){
        if ( pPass.bDetailBumpMappingMasking ){
            pPass.fDetailBumpScale *= pPass.cDiffuseMap.w ;
            pPass.vDetailBlendAmount *= pPass.cDiffuseMap.w ;
        }
        vDetailN = EXPAND( cDetailMap).xy ;
        pPass.cBumpMap.xy  += (vDetailN.xy  * pPass.fDetailBumpScale);
    }
    pPass.fAlpha = (pPass.cDiffuseMap.w  * pPass.IN.Ambient.w );
    DebugOutput( cOut, pPass.IN.baseTC);
    return cOut;
    pPass.vNormal = ( pPass.mTangentToWS * pPass.cBumpMap.xyz  );
    if ( pPass.bRenormalizeNormal ){
        pPass.vNormal = normalize( pPass.vNormal );
    }
    pPass.fNdotE = dot( pPass.vView.xyz , pPass.vNormal.xyz );
    pPass.fGloss = MatSpecColor.w ;
    pPass.vReflVec = (((2.00000 * pPass.fNdotE) * pPass.vNormal.xyz ) - pPass.vView.xyz );
    pPass.fFresnel = (pPass.fFresnel_Bias + (pPass.fFresnel_Scale * pow( (1.00100 - xll_saturate( pPass.fNdotE )), pPass.fFresnel_Pow)));
    brb_frag_custom_begin( pPass);
    if ( bool( SnowVolumeParams[ 0 ].x  ) ){
        pPass.cSnowMap = xll_tex2Dlod( sceneSnowMapSampler, vec4( (pPass.IN.screenProj.xy  / pPass.IN.screenProj.w ), 0.000000, 0.000000));
        pPass.fGloss = mix( pPass.fGloss, 0.300000, pPass.cSnowMap.w );
    }
    if ( pPass.bDetailBumpMapping ){
        vBlendAmount = vec2( (mix( 0.500000, float( cDetailMap.zw ), float( pPass.vDetailBlendAmount)) * 2.00000));
        pPass.cDiffuseMap.xyz  *= vBlendAmount.x ;
        pPass.cGlossMap.xyz  *= vBlendAmount.y ;
    }
    if ( (pPass.nReflectionMapping > 0) ){
        if ( (pPass.nReflectionMapping == 1) ){
            pPass.cEnvironment.xyz  = vec3( GetEnvironmentCMap( envMapSamplerCUBE, pPass.vReflVec.xyz , pPass.fGloss));
        }
        else{
            if ( (pPass.nReflectionMapping == 2) ){
                pPass.cEnvironment.xyz  = vec3( GetEnvironment2DMap( envMapSampler, pPass.vReflVec.xy ));
            }
        }
    }
    vAmbientNormal = pPass.vNormal.xyz ;
    frag_ambient( pPass, vAmbientNormal);
    if ( (pPass.bCustomComposition == false) ){
        brb_frag_final_composition( pPass, cOut.xyz );
    }
    brb_frag_custom_end( pPass, cOut.xyz );
    if ( pPass.bAlphaGlow ){
        cOut.xyz  += ((pPass.cDiffuseMap.w  * pPass.cDiffuseMap.xyz ) * pPass.fAlphaGlow_Multiplier);
    }
    cOut.w  = pPass.fAlpha;
    frag_fog_setup( pPass, cOut);
    frag_hdr_setup( pPass, cOut);
    return cOut;
}
void HDROutput( out pixout OUT, in vec4 Color, in float fDepth ) {
    OUT.Color = Color;
}
pixout brb_BeamPS( in vert2FragGeneral IN ) {
    pixout OUT;
    fragPass pPass;
    vec4 cFinal;
    OUT = pixout( vec4( 0.000000, 0.000000, 0.000000, 0.000000));
    pPass = fragPass( fragInput( vec4( 0.000000, 0.000000, 0.000000, 0.000000), vec4( 0.000000, 0.000000, 0.000000, 0.000000), vec4( 0.000000, 0.000000, 0.000000, 0.000000), vec4( 0.000000, 0.000000, 0.000000, 0.000000), vec4( 0.000000, 0.000000, 0.000000, 0.000000), vec4( 0.000000, 0.000000, 0.000000, 0.000000), vec4( 0.000000, 0.000000, 0.000000, 0.000000), vec4( 0.000000, 0.000000, 0.000000, 0.000000), vec4( 0.000000, 0.000000, 0.000000, 0.000000), vec4( 0.000000, 0.000000, 0.000000, 0.000000), vec4( 0.000000, 0.000000, 0.000000, 0.000000), vec4( 0.000000, 0.000000, 0.000000, 0.000000), vec4( 0.000000, 0.000000, 0.000000, 0.000000), vec4( 0.000000, 0.000000, 0.000000, 0.000000), vec4( 0.000000, 0.000000, 0.000000, 0.000000), vec4( 0.000000, 0.000000, 0.000000, 0.000000), vec3( 0.000000, 0.000000, 0.000000), vec4( 0.000000, 0.000000, 0.000000, 0.000000), vec4( 0.000000, 0.000000, 0.000000, 0.000000), vec4( 0.000000, 0.000000, 0.000000, 0.000000), 0.000000, vec4( 0.000000, 0.000000, 0.000000, 0.000000), vec3( 0.000000, 0.000000, 0.000000), vec3( 0.000000, 0.000000, 0.000000)), false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, 0, 0, 0.000000, 0.000000, 0.000000, vec2( 0.000000, 0.000000), vec2( 0.000000, 0.000000), 0.000000, 0.000000, 0.000000, 0, mat3( 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000), vec3( 0.000000, 0.000000, 0.000000), vec3( 0.000000, 0.000000, 0.000000), vec3( 0.000000, 0.000000, 0.000000), vec3( 0.000000, 0.000000, 0.000000), vec3( 0.000000, 0.000000, 0.000000), vec4( 0.000000, 0.000000, 0.000000, 0.000000), vec4( 0.000000, 0.000000, 0.000000, 0.000000), vec4( 0.000000, 0.000000, 0.000000, 0.000000), vec4( 0.000000, 0.000000, 0.000000, 0.000000), vec4( 0.000000, 0.000000, 0.000000, 0.000000), 0.000000, 0.000000, 0.000000, 0.000000, vec4( 0.000000, 0.000000, 0.000000, 0.000000), vec4( 0.000000, 0.000000, 0.000000, 0.000000), vec4( 0.000000, 0.000000, 0.000000, 0.000000), 0.000000, vec2( 0.000000, 0.000000), 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, vec3( 0.000000, 0.000000, 0.000000), vec3( 0.000000, 0.000000, 0.000000), vec3( 0.000000, 0.000000, 0.000000), vec3( 0.000000, 0.000000, 0.000000), vec4( 0.000000, 0.000000, 0.000000, 0.000000), vec4( 0.000000, 0.000000, 0.000000, 0.000000), 0.000000, 0.000000, vec4( 0.000000, 0.000000, 0.000000, 0.000000), vec3( 0.000000, 0.000000, 0.000000), fragPassCustom( vec3( 0.000000, 0.000000, 0.000000), vec3( 0.000000, 0.000000, 0.000000), 0.000000, vec2( 0.000000, 0.000000)));
    brb_frag_unify( pPass, IN);
    cFinal = brb_frag_shared_output( pPass);
    HDROutput( OUT, cFinal, 1.00000);
    return OUT;
}
layout(location = 9 ) varying vec4 xlv_TEXCOORD0;
layout(location = 10 ) varying vec4 xlv_TEXCOORD1;
layout(location = 11 ) varying vec4 xlv_TEXCOORD2;
layout(location = 12 ) varying vec4 xlv_TEXCOORD3;
layout(location = 13 ) varying vec4 xlv_TEXCOORD4;
layout(location = 14 ) varying vec4 xlv_TEXCOORD5;
layout(location = 15 ) varying vec4 xlv_TEXCOORD6;
layout(location = 16 ) varying vec4 xlv_TEXCOORD7;
void main() {
    pixout xl_retval;
    vert2FragGeneral xlt_IN;
    xlt_IN.HPosition = vec4(0.0);
    xlt_IN.baseTC = vec4( xlv_TEXCOORD0);
    xlt_IN.vTangent = vec4( xlv_TEXCOORD1);
    xlt_IN.vBinormal = vec4( xlv_TEXCOORD2);
    xlt_IN.vView = vec4( xlv_TEXCOORD3);
    xlt_IN.screenProj = vec4( xlv_TEXCOORD4);
    xlt_IN.projTC = vec4( xlv_TEXCOORD5);
    xlt_IN.AvgFogVolumeContrib = vec4( xlv_TEXCOORD6);
    xlt_IN.Ambient = vec4( xlv_TEXCOORD7);
    xl_retval = brb_BeamPS( xlt_IN);
    gl_FragData[0] = vec4( xl_retval.Color);
}
