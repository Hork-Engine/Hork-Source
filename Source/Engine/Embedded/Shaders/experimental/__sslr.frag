/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

This file is part of the Angie Engine Source Code.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

float g_Roughness;
vec3 g_Normal;

const float minRayStep = 0.1;
const float maxSteps = 30;
const int numBinarySearchSteps = 5;

float SampleViewZ( vec2 TexCoord ) {
    //textureLod(ReflectionDepth, TexCoord, 2).x;

    return -textureLod(ReflectionDepth, TexCoord, 0).x;
}

float getDeltaDepth(const vec3 hit) {
    vec2 UV = ViewToUV(hit); 
    float z = SampleViewZ(UV);
	return z - hit.z;
}

vec3 hitCoord;
vec4 binarySearch(vec3 dir) {
	float ddepth;
	vec3 start = hitCoord;
	for (int i = 0; i < numBinarySearchSteps; i++) {
		dir *= 0.5;
		hitCoord -= dir;
		ddepth = getDeltaDepth(hitCoord);
		if (ddepth < 0.0) hitCoord += dir;
	}
	// Ugly discard of hits too far away
    if (abs(ddepth) > SSLRMaxDist) return vec4(0.0);

	return vec4(ViewToUV(hitCoord), 0.0, 1.0);
}

vec4 rayCast(vec3 dir) {
    const float RayMarchStep = SSLRSampleOffset;// 0.1;

    dir *= RayMarchStep;
    
	for (int i = 0; i < maxSteps; i++) {
		hitCoord += dir;
		if (getDeltaDepth(hitCoord) > 0.0) return binarySearch(dir);
	}
	return vec4(0.0);
}

vec2 BinarySearch(vec3 dir, vec3 hitCoord)
{
    vec2 UV;
 
    for(int i = 0; i < numBinarySearchSteps; i++)
    {

        UV = ViewToUV(hitCoord);
 
        float z = SampleViewZ(UV);

 
        float dDepth = (-hitCoord.z) - (-z);

        dir *= 0.5;
        if(dDepth > 0.0)
            hitCoord += dir;
        else
            hitCoord -= dir;    
    }

    UV = ViewToUV(hitCoord);
 
    return UV;
}

vec3 RayMarch(vec3 dir, vec3 hitCoord)
{
    const float RayMarchStep = SSLRSampleOffset;// 0.1;

    dir *= RayMarchStep;
 
    vec2 UV;

    for(int i = 0; i < maxSteps; i++)
    {
        hitCoord += dir;
 
        UV = ViewToUV(hitCoord);
 
        float z = SampleViewZ(UV);
   /*
        if ( abs( z - hitCoord.z ) < 0.1 ) {
        
            UV = BinarySearch(dir, hitCoord);
            
            return textureLod( ReflectionColor, UV, 0 ).rgb;
        }*/
        
        
        if(z < -1000.0)
        {
            return vec3(0,1,0);
            //continue;
        }
 
        float dDepth = (-hitCoord.z) - (-z);

        if((dir.z - dDepth) < 1.2)
        {
            if(dDepth <= 0.0)
            {   
                UV = BinarySearch(dir, hitCoord);
            
                return textureLod( ReflectionColor, UV, 0 ).rgb;
            }
        }
    }
 
    
    return vec3(0,0,0);//textureLod( ReflectionColor, UV, 0 ).rgb;
}

#define Scale vec3(.8, .8, .8)
#define K 19.19
vec3 hash(vec3 a)
{
    a = fract(a * Scale);
    a += dot(a, a.yxz + K);
    return fract((a.xxy + a.yxx)*a.zyx);
}

vec3 FetchLocalReflection( vec3 ReflectionDir, float Lod ) {
    vec2 UV;
    float L = SSLRSampleOffset;
    float LDelmiter = 0.1f;
    
    vec3 dir = ReflectionDir;// * max(minRayStep, -VS_Position.z);
    
    #if 0
    //float roughness = 0.5;
    //const float ssrJitter = 0.1;
    //dir = ReflectionDir * (1.0 - rand(InNormalizedScreenCoord) * ssrJitter * roughness) * 2.0;
    
    if ( InNormalizedScreenCoord.x > 0.5 ) {
        vec3 jitt = mix(vec3(0.0), vec3(hash((InverseViewMatrix*vec4(VS_Position,1.0)).xyz)), g_Roughness);
        dir = jitt + ReflectionDir;// * max(minRayStep, -VS_Position.z);
    }
    
    #endif

    // calc fragment position at prev frame
    vec3 positionPrevFrame = (ViewspaceReprojection * vec4( VS_Position, 1.0 )).xyz;
    
    for ( int i = 0; i < 10; i++ )
    {
        vec3 dir = VS_Position + dir * L;

        // calc UV for previous frame
        UV = ViewToUVReproj( dir );

        // read reflection depth
        float z = -textureLod( ReflectionDepth, UV, 0 ).x;

        // calc viewspace position at prev frame
        vec3 p = UVToView( UV, z );

        // calc distance between fragments
        L = distance( positionPrevFrame, p );
    }
    
    //L = saturate(L * SSLRMaxDist);

    float error = 1;// (1 - L);
    
    #if 0
    hitCoord = VS_Position;
    vec4 coords = rayCast(dir);
    UV = coords.xy;
    #endif
    #if 1
    vec3 reflectColor = textureLod( ReflectionColor, UV, 0 ).rgb * error;
    #endif
    #if 0
    vec3 reflectColor = RayMarch((/*vec3(jitt) + */ReflectionDir * max(minRayStep, -VS_Position.z)), VS_Position);
    #endif

    vec2 dCoords = smoothstep(0.2, 0.6, abs(vec2(0.5, 0.5) - UV.xy)); 
    float screenEdgefactor = clamp(1.0 - (dCoords.x + dCoords.y), 0.0, 1.0);
    const float reflectionSpecularFalloffExponent = 3.0;
    float ReflectionMultiplier = pow(1 - g_Roughness, reflectionSpecularFalloffExponent) * 
                screenEdgefactor * 
                saturate(-ReflectionDir.z);
    
    return clamp( reflectColor * (ReflectionMultiplier), 0.0, 20.0 );// * error;
}


































// #ADD THESE PROPERTIES TO THE OWNER OF THIS FILTER

const float reflectance = 1; // FIXME
const int samples = 5;


// **** **** **** ****

// #TWEAK THESE TO YOUR LIKING

const float stepSize = 0.01; // stride of cast rays (in blender units)
const int sampleSteps = 50; // maxmimum amount of steps that a ray can take
// const int samples = 4;       // amount of rays fired per pixel
/// replace with: vec4(R, G, B, 1.0)
const vec4 skyColor = vec4(0,0,0,1);//vec4(0.3, 0.5, 0.9, 1.0);

// **** **** **** ****

const float TAU = 6.28318530718;

float getDepth(vec2 coord){

    return -textureLod(ReflectionDepth, coord, 0).x; // FIXME sign
}

vec4 getColor(vec2 coord){
	return texture(ReflectionColor, coord);
}

vec2 getCoord(vec3 pos) {
  //vec3 norm = pos / pos.z;
  //vec2 view = vec2((norm.x * fovratio + 1.0) / 2.0, (norm.y * fovratio * aspectratio + 1.0) / 2.0);
  //return view;
  return ViewToUV(pos);
}

// **** **** **** ****

struct ShaderData {
	vec3 normal;
	vec3 view;

	float alpha;

	vec3 micronormal;
	vec3 light;

	float alpha2;

	float ndotv;
	float ndotl;

	// hdotl = hdotv = cos(theta_d)
	float hdotl;

	float fresnel;
	float brdf;

	float diffuse_f_view;
	float diffuse_f_light;
};

// **** **** **** ****

float fresnel(const ShaderData d){
	return reflectance + (1.0-reflectance) * pow(1.0-d.hdotl, 5.0);
}

float g1 (float ndotv, float alpha2) {
    float cos2 = ndotv * ndotv;
    float tan2 = (1.0f-cos2)/cos2;
    
    return 2.0f / (1.0f + sqrt(1.0f + alpha2 * tan2));
}

float brdf (const ShaderData d) {
	// D term is not computed because we are importance-sampling it exactly.

	float geometry_A = g1(d.ndotl, d.alpha2);
	float geometry_B = g1(d.ndotv, d.alpha2);

	// I'm not sure why, but it looks like we should not divide by this
	// float denom = 4.0 * d.ndotl * d.ndotv;
	// I get the ndotl, since it cancels out with the one in the rendering equation.
	// But the 4ndotv is a total mistery to me...
	// I need to go over the math again.

	return geometry_A * geometry_B;
}

/*
// Near blue noise.
// Rounded to 6 digits (arbitrary choice)
const float randomA[32] = float[](
	0.408960, 0.089650, 0.659157, 0.090182,
	0.692129, 0.321620, 0.438963, 0.868447,
	0.215370, 0.698281, 0.510065, 0.596903,
	0.282642, 0.270406, 0.087304, 0.894975,
	0.726160, 0.705759, 0.919466, 0.149936,
	0.237849, 0.838222, 0.871506, 0.463300,
	0.625321, 0.907557, 0.215592, 0.063410,
	0.471047, 0.379783, 0.088273, 0.485860
);

const float randomB[32] = float[](
	0.197508, 0.440571, 0.915642, 0.920050,
	0.698262, 0.438228, 0.531794, 0.398298,
	0.091603, 0.170917, 0.888277, 0.526257,
	0.744579, 0.267982, 0.795485, 0.884548,
	0.567740, 0.377212, 0.556393, 0.586749,
	0.525395, 0.637739, 0.135191, 0.088169,
	0.791195, 0.277727, 0.890907, 0.607043,
	0.362902, 0.855291, 0.308795, 0.698057
);
*/

// First 4 points are the standard rotated quad AA distribution
// Rest are halton sequence with bases 2 and 3
// Everything is rounded to 6 digits (arbitrary choice)
const float randomA[32] = float[](
	0.625000,0.375000,0.125000,0.875000,
	0.937500,0.031250,0.531250,0.281250,
	0.781250,0.156250,0.656250,0.406250,
	0.906250,0.093750,0.593750,0.343750,
	0.843750,0.218750,0.718750,0.468750,
	0.968750,0.015625,0.515625,0.265625,
	0.765625,0.140625,0.640625,0.390625,
	0.890625,0.078125,0.578125,0.328125
);

const float randomB[32] = float[](
	0.125000,0.875000,0.375000,0.625000,
	0.259259,0.592593,0.925926,0.074074,
	0.407407,0.740741,0.185185,0.518519,
	0.851852,0.296296,0.629630,0.962963,
	0.012346,0.345679,0.679012,0.123457,
	0.456790,0.790123,0.234568,0.567901,
	0.901235,0.049383,0.382716,0.716049,
	0.160494,0.493827,0.827160,0.271605
);

vec2 nth_random (int n) {
	// @@ This breaks if we ever want more than 32 samples
	return vec2(randomA[n], randomB[n]);
}

vec2 micronormal (const ShaderData d, vec2 r)  {
	float theta = acos(sqrt((1.0 - r.x) / ((d.alpha2 - 1.0) * r.x + 1.0)));
	float phi = r.y * TAU;

	return vec2(phi, theta);
}

//get a scalar random value from a 3d value
float rand(vec3 value){
    //make value smaller to avoid artefacts
    vec3 smallValue = sin(value);
    //get scalar value from 3d vector
    float random = dot(smallValue, vec3(12.9898, 78.233, 37.719));
    //make value more random by making it bigger and then taking teh factional part
    random = mod(sin(random) * 143758.5453, 1.0f);
    return random;
}

// PRECONDITION: Coords is a rotation matrix
vec3 generateMicronormal(const ShaderData d, int index, mat3 coords){
	
	vec2 r = mod(nth_random(index) * rand(d.view), 1.0);
	vec2 n_polar = micronormal(d, r);

// INVARIANT: n_tangentSpace is a unit vector
	vec3 n_tangentSpace = vec3(
		sin(n_polar.y) * cos(n_polar.x),
		sin(n_polar.y) * sin(n_polar.x),
		cos(n_polar.y)
	);
	
    vec3 n_coordSpace = coords * n_tangentSpace;

	return n_coordSpace;
}

// **** **** **** ****

vec3 raymarch ( vec3 position, vec3 direction ) {
	direction = normalize(direction);

	const float epsilon = 0.001;
	float jitterAmount = epsilon + rand(direction) * 0.01;

	vec3 e = position + direction;
	vec3 ep = e / e.z;
	vec3 pp = position / position.z;
	vec3 dp = ep - pp;
	vec3 dp0 = normalize(dp);
    
	for (int i = 0; i < sampleSteps; ++i) {
		float rayLen = jitterAmount + i * stepSize;
		vec3 sp = pp + dp0 * rayLen;
		float t = - cross(sp,position).z / cross(sp,direction).z;

		// @@ We should be able to compute both rayEnd and screenCoord at once
		vec3 rayEnd = position + direction * t;
		vec2 screenCoord = getCoord(rayEnd);
		//float delta = rayEnd.z - getDepth(screenCoord);
        float delta = -rayEnd.z - -getDepth(screenCoord);   // !!!!!!!
		if (delta > 0.0f && delta < 1.0f) {
			return getColor(screenCoord).xyz;
		}
	}

    return skyColor.xyz;
}

// **** **** **** ****

ShaderData make_shader_data () {
	ShaderData result;

	result.normal = g_Normal;
	// position is the shift from the camera to the current fragment.
	// In particular, it points away from the camera. We want the direction
	// from current the fragment to the camera, so we change the sign (-).
	result.view = -normalize(VS_Position);

	result.alpha = g_Roughness * g_Roughness;
	result.alpha2 = result.alpha * result.alpha;

	result.ndotv = dot(result.view, result.normal);

	return result;
}

void update_shader_data (inout ShaderData result, vec3 micronormal) {
	result.micronormal = micronormal;

	// glsl reflects vectors across a plane.
	// We want to reflect *against* a plane
	result.light = -reflect(result.view, micronormal);

	result.ndotl = dot(result.normal, result.light);
	result.hdotl = dot(result.micronormal, result.light);

	result.fresnel = fresnel(result);
	result.brdf = brdf(result);

	// @@ Probably dont need to store all of these
	float diffuse_f90 = 0.5f + 2.0f * g_Roughness * result.hdotl * result.hdotl; 
	float diffuse_f0 = 1.0f - reflectance;
	//setColor(result.diffuse_f90 >= 9.0 ? vec4(1.0,0.0,0.0,0.0) : vec4(0.0,1.0,0.0,0.0));
	result.diffuse_f_view = diffuse_f0 + (diffuse_f90 - diffuse_f0) * pow(1.0f - result.ndotv, 5.0f);
	result.diffuse_f_light = diffuse_f0 + (diffuse_f90 - diffuse_f0) * pow(1.0f - result.ndotl, 5.0f);
}

// Return a vec4, pack average fresnel into alpha channel
vec4 glossyReflection(ShaderData d){

	vec3 Z = d.normal;
	vec3 X = normalize(cross(d.normal, d.view));
	vec3 Y = cross(Z, X);

	mat3 coords = mat3(X, Y, Z);

	vec4 radiance = vec4(0.0);

	for(int i = 0; i < samples; i++){
		
		vec3 micronormal = generateMicronormal(d, i, coords);

		update_shader_data(d, micronormal);
		
		radiance += vec4(
			raymarch(VS_Position, d.light) * d.brdf * d.fresnel,
			d.diffuse_f_view * d.diffuse_f_light);
		
	}
	
	radiance /= float(samples);

	return radiance;
}

// **** **** **** ****

vec4 lerp(vec4 a, vec4 b, float t){
	return a + (b-a)*t;
}

vec4  FetchLocalReflection2(){
	ShaderData d = make_shader_data();

	return clamp( glossyReflection(d), 0.0, 100.0 );

	// setColor(lerp(d.direct, reflection, clamp(d.view.x*64.0, -1.0, 1.0)*0.5+0.5));
	//setColor(reflection + d.direct * reflection.w);
}
