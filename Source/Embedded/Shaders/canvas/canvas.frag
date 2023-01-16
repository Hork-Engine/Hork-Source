/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2023 Alexander Samusev.

This file is part of the Hork Engine Source Code.

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

layout( location = 0 ) out vec4 FS_FragColor;

layout( location = 0 ) noperspective in vec2 VS_TexCoord;
layout( location = 1 ) noperspective in vec2 VS_Position;

layout( binding = 0 ) uniform sampler2D Smp_Source;

layout( binding = 1, std140 ) uniform UniformBuffer1
{
	vec4 innerCol;
	vec4 outerCol;
	
	mat3 scissorMat;
	mat3 paintMat;
	
	vec2 scissorExt;
	vec2 scissorScale;
	
	vec2 extent;
	float radius;
	float feather;
	
	float strokeMult;
	float strokeThr;
	int texType;
	int type;
};

float sdroundrect(vec2 pt, vec2 ext, float rad) {
	vec2 ext2 = ext - vec2(rad,rad);
	vec2 d = abs(pt) - ext2;
	return min(max(d.x,d.y),0.0) + length(max(d,0.0)) - rad;
}

// Scissoring
float scissorMask(vec2 p) {
	vec2 sc = (abs((scissorMat * vec3(p,1.0)).xy) - scissorExt);
	sc = vec2(0.5,0.5) - sc * scissorScale;
	return clamp(sc.x,0.0,1.0) * clamp(sc.y,0.0,1.0);
}
#ifdef EDGE_AA
// Stroke - from [0..1] to clipped pyramid, where the slope is 1px.
float strokeMask() {
	return min(1.0, (1.0-abs(VS_TexCoord.x*2.0-1.0))*strokeMult) * min(1.0, VS_TexCoord.y);
}
#endif

vec4 linearize(in vec4 c)
{
    c.x = (c.x <= 0.04045f) ? c.x / 12.92f : pow((c.x + 0.055f) / 1.055f, 2.4f);
	c.y = (c.y <= 0.04045f) ? c.y / 12.92f : pow((c.y + 0.055f) / 1.055f, 2.4f);
	c.z = (c.z <= 0.04045f) ? c.z / 12.92f : pow((c.z + 0.055f) / 1.055f, 2.4f);	
	return c;
}

vec3 linearize(in vec3 c)
{
    c.x = (c.x <= 0.04045f) ? c.x / 12.92f : pow((c.x + 0.055f) / 1.055f, 2.4f);
	c.y = (c.y <= 0.04045f) ? c.y / 12.92f : pow((c.y + 0.055f) / 1.055f, 2.4f);
	c.z = (c.z <= 0.04045f) ? c.z / 12.92f : pow((c.z + 0.055f) / 1.055f, 2.4f);	
	return c;
}

void main()
{
	float scissor = scissorMask(VS_Position);
#ifdef EDGE_AA
	float strokeAlpha = strokeMask();
	if (strokeAlpha < strokeThr) discard;
#else
	float strokeAlpha = 1.0;
#endif
	if (type == 0)  // Gradient
	{
		// Calculate gradient color using box gradient
		vec2 pt = (paintMat * vec3(VS_Position,1.0)).xy;
		float d = clamp((sdroundrect(pt, extent, radius) + feather*0.5) / feather, 0.0, 1.0);
		
		// Perform mixing in sRGB space for correct gradient
        vec4 color = mix(innerCol, outerCol, d);

		// Linearize
		color = linearize(color);
		
		// Premultiply
		color.rgb *= color.a;

		// Combine alpha
		color *= strokeAlpha * scissor;
		
		FS_FragColor = color;
	}
	else if (type == 1) // Image
	{
		// Calculate color fron texture
		vec2 pt = (paintMat * vec3(VS_Position,1.0)).xy / extent;

		vec4 color = texture(Smp_Source, pt);
		if (texType == 1)
			color = vec4(color.xyz*color.w,color.w);
			
		// Apply color tint and alpha.		
		vec4 c0 = linearize(innerCol);
		c0.rgb *= innerCol.a;		
		color *= c0;
		
		// Combine alpha
		color *= strokeAlpha * scissor;
		
		FS_FragColor = color;
	}
	else if (type == 2) // Stencil fill
	{
		FS_FragColor = vec4(1,1,1,1);
	}
	else // Textured tris
	{
		vec4 color = texture(Smp_Source, VS_TexCoord);
		if (texType == 1)
			color = vec4(color.xyz*color.w,color.w);
		
		vec4 c0 = linearize(innerCol);
		c0.rgb *= innerCol.a;
		color *= c0;
		
		// Combine alpha
		color *= scissor;
		
		FS_FragColor = color;
	}
}
