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

// Based on tutorial https://habr.com/ru/post/416163/

#define PARALLAX_TECHNIQUE_DISABLED 0
#define PARALLAX_TECHNIQUE_POM      1   // Parallax occlusion mapping
#define PARALLAX_TECHNIQUE_RPM      2   // Relief Parallax Mapping

#ifndef COMPUTE_TBN
#undef PARALLAX_TECHNIQUE
#define PARALLAX_TECHNIQUE PARALLAX_TECHNIQUE_DISABLED
#endif

#define PARALLAX_MIN_LAYERS 8.0
#define PARALLAX_MAX_LAYERS 32.0

#if PARALLAX_TECHNIQUE == PARALLAX_TECHNIQUE_DISABLED

#define InitParallaxTechnique()
#define ParallaxMapping(displacementMap, texCoord, bSelfShadowing) (texCoord)
#define GetParallaxSelfShadow(LightDir) 1.0

#else

mat3  InTBN;
vec3  InParallaxViewDir;
sampler2D InDisplacementMap;
float InParallaxDepth = 0;
vec2  InParallaxTexCoord = vec2( 0.0 );
vec2  InParallaxTexCoordSource = vec2( 0.0 );
bool  InParallaxSelfShadowing = false;
float InParallaxDisplacementScale = 0.05;

void InitParallaxTechnique()
{
    InTBN = transpose( mat3( VS_T, VS_B, VS_N ) );
    InParallaxViewDir = InTBN * normalize( InViewspaceToEyeVec - VS_Position );
}

float GetParallaxSelfShadow( vec3 LightDir ) {
    if ( !InParallaxSelfShadowing ) {
        return 1.0;
    }

    if ( InParallaxDisplacementScale < 0.001 ) {
        return 1.0;
    }

    vec3 tangentLightDir = InTBN * LightDir;

    float shadowMultiplier = 0.0;

    // расчет будем делать только для поверхностей, 
    // освещенных используемым источником
    float alignFactor = dot( vec3( 0.0, 0.0, 1.0 ), tangentLightDir );
    if ( alignFactor > 0.0 ) {

        // слои глубины
        const float minLayers = 16.0;
        const float maxLayers = 32.0;
        float numLayers = mix( maxLayers, minLayers, abs( alignFactor ) );

        // шаг слоя глубины
        float depthStep = InParallaxDepth / numLayers;
        if ( depthStep < 0.001 ) {
            return 1;
        }

        // шаг смещения текстурных координат
        //vec2 deltaTexCoords = InParallaxDisplacementScale * tangentLightDir.xy/(tangentLightDir.z * numLayers);
        vec2 deltaTexCoords = max( 0.001, InParallaxDisplacementScale/(tangentLightDir.z * numLayers) ) * tangentLightDir.xy;

        // счетчик точек, оказавшихся под поверхностью
        int numSamplesUnderSurface = 0;

        // поднимаемся на глубину слоя и смещаем 
        // текстурные координаты вдоль вектора L
        float currentLayerDepth = InParallaxDepth - depthStep;
        vec2 currentTexCoords = InParallaxTexCoordSource + deltaTexCoords;

        float currentDepthValue = 1.0-texture( InDisplacementMap, currentTexCoords ).r;

        // номер текущего шага
        float stepIndex = 1.0;
        // повторяем, пока не выйдем за слой нулевой глубины…
        while ( currentLayerDepth > 0.0 ) {
            // если нашли точку под поверхностью, то увеличим счетчик и 
            // рассчитаем очередной частичный и полный коэффициенты
            if ( currentDepthValue < currentLayerDepth ) {
                numSamplesUnderSurface++;
                float currentShadowMultiplier = (currentLayerDepth - currentDepthValue)*(1.0 - stepIndex/numLayers);

                shadowMultiplier = max( shadowMultiplier, currentShadowMultiplier );
            }
            stepIndex++;
            currentLayerDepth -= depthStep;
            currentTexCoords += deltaTexCoords;
            currentDepthValue = 1.0-texture( InDisplacementMap, currentTexCoords ).r;
        }
        // если точек под поверхностью не было, то точка 
        // считается освещенной и коэффициент оставим 1
        if ( numSamplesUnderSurface < 1 )
            shadowMultiplier = 1.0;
        else
            shadowMultiplier = 1.0 - shadowMultiplier;
    } else {
        return 1;
    }

    return pow( shadowMultiplier, 4.0 );
}

vec2 ParallaxMapping( sampler2D displacementMap, vec2 texCoord, float DisplacementScale, bool bSelfShadowing )
{
    InParallaxTexCoordSource = texCoord;
    InParallaxDisplacementScale = DisplacementScale;
    InParallaxSelfShadowing = bSelfShadowing;
    InDisplacementMap = displacementMap;
    InParallaxDepth = 0;
    InParallaxTexCoord = texCoord;

    if ( DisplacementScale < 0.001 ) {
        return texCoord;
    }

    //
    // Steep Parallax Mapping
    //

    // количество слоев глубины
    //const float numLayers = 10;
    float numLayers = mix( PARALLAX_MAX_LAYERS, PARALLAX_MIN_LAYERS, abs( dot( vec3( 0.0, 0.0, 1.0 ), InParallaxViewDir ) ) );
    // размер каждого слоя
    float layerDepth = max( 1.0 / numLayers, 0.001 );
    // глубина текущего слоя
    float currentLayerDepth = 0.0;
    // величина шага смещения текстурных координат на каждом слое
    // расчитывается на основе вектора P
    vec2 P = InParallaxViewDir.xy/* / InParallaxViewDir.z*/ * InParallaxDisplacementScale;
    vec2 deltaTexCoords = P / numLayers;

    // начальная инициализация
    vec2  currentTexCoords = texCoord;
    float currentDepthMapValue = 1.0-texture( displacementMap, currentTexCoords ).r;

    while ( currentLayerDepth < currentDepthMapValue )
    {
        // смещаем текстурные координаты вдоль вектора P
        currentTexCoords -= deltaTexCoords;
        // делаем выборку из карты глубин в текущих текстурных координатах 
        currentDepthMapValue = 1.0-texture( displacementMap, currentTexCoords ).r;
        // рассчитываем глубину следующего слоя
        currentLayerDepth += layerDepth;
    }

#if ( PARALLAX_TECHNIQUE == PARALLAX_TECHNIQUE_POM )

    //
    // Parallax occlusion mapping
    //

    // находим текстурные координаты перед найденной точкой пересечения,
    // т.е. делаем "шаг назад"
    vec2 prevTexCoords = currentTexCoords + deltaTexCoords;

    // находим значения глубин до и после нахождения пересечения 
    // для использования в линейной интерполяции
    float afterDepth = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = (1.0-texture( displacementMap, prevTexCoords ).r) - currentLayerDepth + layerDepth;

    // интерполяция текстурных координат 
    float weight = afterDepth / (afterDepth - beforeDepth);
    vec2 finalTexCoords = mix( currentTexCoords, prevTexCoords, weight );

    InParallaxDepth = mix( afterDepth, beforeDepth, weight );
    InParallaxTexCoord = finalTexCoords;

    return finalTexCoords;

#else//#elif ( PARALLAX_TECHNIQUE == PARALLAX_TECHNIQUE_RPM )

    //
    // Relief Parallax Mapping
    //

    // уполовиниваем смещение текстурных координат и размер слоя глубины
    deltaTexCoords *= 0.5;
    layerDepth *= 0.5;
    // сместимся в обратном направлении от точки, найденной в Steep PM
    currentTexCoords += deltaTexCoords;
    currentLayerDepth -= layerDepth;

    // установим максимум итераций поиска…
    const int reliefSteps = 5;
    int currentStep = reliefSteps;
    while ( currentStep > 0 ) {
        currentDepthMapValue = 1.0-texture( displacementMap, currentTexCoords ).r;
        deltaTexCoords *= 0.5;
        layerDepth *= 0.5;
        // если выборка глубины больше текущей глубины слоя, 
        // то уходим в левую половину интервала
        if ( currentDepthMapValue > currentLayerDepth ) {
            currentTexCoords -= deltaTexCoords;
            currentLayerDepth += layerDepth;
        }
        // иначе уходим в правую половину интервала
        else {
            currentTexCoords += deltaTexCoords;
            currentLayerDepth -= layerDepth;
        }
        currentStep--;
    }

    InParallaxDepth = currentDepthMapValue;
    InParallaxTexCoord = currentTexCoords;

    return currentTexCoords;
#endif
}

#endif
