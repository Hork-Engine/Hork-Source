/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2020 Alexander Samusev.

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

#ifndef TONEMAPPING_H
#define TONEMAPPING_H

// Convert linear RGB into a CIE xyY (xy = chroma, Y = luminance).
vec3 RGB2xyY( in vec3 rgb ) {
	const mat3 RGB2XYZ = mat3
    (
		0.4124, 0.3576, 0.1805,
		0.2126, 0.7152, 0.0722,
		0.0193, 0.1192, 0.9505
    );

    vec3 XYZ = RGB2XYZ * rgb;

    // XYZ to xyY
    return vec3( XYZ.xy / max( XYZ.x + XYZ.y + XYZ.z, 1e-10 ), XYZ.y );
}

// Convert a CIE xyY value into linear RGB.
vec3 xyY2RGB( in vec3 xyY ) {
	const mat3 XYZ2RGB = mat3
    (
		3.2406, -1.5372, -0.4986,
		-0.9689, 1.8758, 0.0415,
		0.0557, -0.2040, 1.0570
	);

	// xyY to XYZ
	float z_div_y = xyY.z / max( xyY.y, 1e-10 );
	return XYZ2RGB * vec3( z_div_y * xyY.x, xyY.z, z_div_y * (1.0 - xyY.x - xyY.y) );
}

vec3 ToneLinear( in vec3 Color, in float Exposure ) {
	/*
	vec3 xyY = RGB2xyY( Color );

	xyY.z *= Exposure;

	return xyY2RGB( xyY );
	*/
    return Color * Exposure;
}

vec3 ToneReinhard( in vec3 Color, in float Exposure, in float WhitePoint ) {
	vec3 xyY = RGB2xyY( Color );

    float Lp = xyY.z * Exposure;
    xyY.z = Lp * (1.0f + Lp / (WhitePoint * WhitePoint)) / (1.0f + Lp);

    return xyY2RGB( xyY );
}

vec3 ACESFilm( in vec3 Color, in float Exposure ) {
	float a = 2.51f;
	float b = 0.03f;
	float c = 2.43f;
	float d = 0.59f;
	float e = 0.14f;
	vec3 x = Color * Exposure;
    return clamp( (x*(a*x+b))/(x*(c*x+d)+e), vec3(0), vec3(1) );
}

#endif // TONEMAPPING_H
