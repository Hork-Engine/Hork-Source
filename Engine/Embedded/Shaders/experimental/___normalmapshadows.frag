
#define float4 vec4
#define float3 vec3
#define float2 vec2
bool	EnableShadows=true;
float	HeightScale=0.22;
float	SampleCount=32;
float	ShadowHardness=50;
float LodBias = 1;
float GetParallaxSelfShadow/*GetNormalMappingShadows*/( vec3 LightDir )
{
    vec3 tangentLightDir = InTBN * LightDir;
	tangentLightDir.y = -tangentLightDir.y;


	float	samplecount = SampleCount;// * max(1.0 - abs(tangentLightDir.z), 1.0/SampleCount);
	float	invsamplecount = 1.0 / samplecount;

	float	hardness = HeightScale * ShadowHardness;

	float	shadow = 0.0;
	float3	normal;
	float2	uvddx = dFdx(InParallaxTexCoordSource) * LodBias;
	float2	uvddy = dFdy(InParallaxTexCoordSource) * LodBias;

	normal = textureGrad(tslot_2, InParallaxTexCoordSource, uvddx, uvddy).xyz;
	normal = normal * 2.0 - 1.0;

	float2	dir = tangentLightDir.xy * float2(1.0,-1.0) * HeightScale;

	//can be used for adaptive number of steps
	//float2	uvwidth = fwidth(dir.xy);
	//uvwidth.x = max(uvwidth.x, uvwidth.y);

	//lighting with flat normals (from vertex or depth generated)
	float	lighting = saturate(dot(tangentLightDir, normal));

	float	step = invsamplecount;
	float	pos = step;

	//randomization of start sample
	float2	noise = vec2(0.0);//frac(vPos.xy*0.5);
	noise.x = noise.x + noise.y*0.5;
	pos = step - step * noise.x;

	//do not compute on back faces/pixels
	pos = (-lighting >= 0.0) ? 1.001 : pos;

	if (EnableShadows == false) pos = 1.001;

	float	slope = -lighting;
	float	maxslope = 0.0;
	while (pos <= 1.0)
	{
		float3	tmpNormal = textureGrad(tslot_2, InParallaxTexCoordSource + dir * pos, uvddx, uvddy).xyz;
		tmpNormal = tmpNormal * 2.0 - 1.0;

		//to save memory bandwidth, you may reconstruct Z if normal have 2 channels (bad precision, sphere transform decoding is better, but slower)
		//tmpNormal.z = sqrt(1.0-dot(tmpNormal.xy, tmpNormal.xy));

		float	tmpLighting = dot(tangentLightDir, tmpNormal);

		float	shadowed = -tmpLighting;

		//for g-buffer normals of deferred render insert here depth comparison to occlude objects, abstract code example:
		//float2	cropminmax = saturate(1.0 - (depth - tmpDepth) * float2(4000.0, -600.0));
		//cropminmax.x = cropminmax.x * cropminmax.y;
		//shadowed *= cropminmax.x;

		slope += shadowed;

		//if (slope > 0.0) //cheap, but not correct, suitable for hard shadow with early exit
		if (slope > maxslope) //more suitable for calculating soft shadows by distance or/and angle
		{
			shadow += hardness * (1.0-pos);
		}
		maxslope = max(maxslope, slope);

		pos += step;
	}

	shadow = saturate(1.0 - shadow * invsamplecount);
    return shadow;
}













// Parallax mapping from Dark Places

uniform mediump vec4 OffsetMapping_ScaleSteps;
uniform mediump float OffsetMapping_Bias;
#ifdef USEOFFSETMAPPING_LOD
uniform mediump float OffsetMapping_LodDistance;
#endif
vec2 OffsetMapping(vec2 TexCoord, vec2 dPdx, vec2 dPdy)
{
	float i;
	// distance-based LOD
#ifdef USEOFFSETMAPPING_LOD
	//mediump float LODFactor = min(1.0, OffsetMapping_LodDistance / EyeVectorFogDepth.z);
	//mediump vec4 ScaleSteps = vec4(OffsetMapping_ScaleSteps.x, OffsetMapping_ScaleSteps.y * LODFactor, OffsetMapping_ScaleSteps.z / LODFactor, OffsetMapping_ScaleSteps.w * LODFactor);
	mediump float GuessLODFactor = min(1.0, OffsetMapping_LodDistance / EyeVectorFogDepth.z);
#ifdef USEOFFSETMAPPING_RELIEFMAPPING
	// stupid workaround because 1-step and 2-step reliefmapping is void
	mediump float LODSteps = max(3.0, ceil(GuessLODFactor * OffsetMapping_ScaleSteps.y));
#else
	mediump float LODSteps = ceil(GuessLODFactor * OffsetMapping_ScaleSteps.y);
#endif
	mediump float LODFactor = LODSteps / OffsetMapping_ScaleSteps.y;
	mediump vec4 ScaleSteps = vec4(OffsetMapping_ScaleSteps.x, LODSteps, 1.0 / LODSteps, OffsetMapping_ScaleSteps.w * LODFactor);
#else
	#define ScaleSteps OffsetMapping_ScaleSteps
#endif
#ifdef USEOFFSETMAPPING_RELIEFMAPPING
	float f;
	// 14 sample relief mapping: linear search and then binary search
	// this basically steps forward a small amount repeatedly until it finds
	// itself inside solid, then jitters forward and back using decreasing
	// amounts to find the impact
	//vec3 OffsetVector = vec3(EyeVectorFogDepth.xy * ((1.0 / EyeVectorFogDepth.z) * ScaleSteps.x) * vec2(-1, 1), -1);
	//vec3 OffsetVector = vec3(normalize(EyeVectorFogDepth.xy) * ScaleSteps.x * vec2(-1, 1), -1);
	vec3 OffsetVector = vec3(normalize(EyeVectorFogDepth.xyz).xy * ScaleSteps.x * vec2(-1, 1), -1);
	vec3 RT = vec3(vec2(TexCoord.xy - OffsetVector.xy*OffsetMapping_Bias), 1);
	OffsetVector *= ScaleSteps.z;
	for(i = 1.0; i < ScaleSteps.y; ++i)
		RT += OffsetVector *  step(textureGrad(Texture_Normal, RT.xy, dPdx, dPdy).a, RT.z);
	for(i = 0.0, f = 1.0; i < ScaleSteps.w; ++i, f *= 0.5)
		RT += OffsetVector * (step(textureGrad(Texture_Normal, RT.xy, dPdx, dPdy).a, RT.z) * f - 0.5 * f);
	return RT.xy;
#else
	// 2 sample offset mapping (only 2 samples because of ATI Radeon 9500-9800/X300 limits)
	//vec2 OffsetVector = vec2(EyeVectorFogDepth.xy * ((1.0 / EyeVectorFogDepth.z) * ScaleSteps.x) * vec2(-1, 1));
	//vec2 OffsetVector = vec2(normalize(EyeVectorFogDepth.xy) * ScaleSteps.x * vec2(-1, 1));
	vec2 OffsetVector = vec2(normalize(EyeVectorFogDepth.xyz).xy * ScaleSteps.x * vec2(-1, 1));
	OffsetVector *= ScaleSteps.z;
	for(i = 0.0; i < ScaleSteps.y; ++i)
		TexCoord += OffsetVector * ((1.0 - OffsetMapping_Bias) - textureGrad(Texture_Normal, TexCoord, dPdx, dPdy).a);
	return TexCoord;
#endif
}
