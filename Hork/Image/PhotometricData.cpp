/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2024 Alexander Samusev.

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

#include <Hork/Image/PhotometricData.h>
#include <Hork/Image/RawImage.h>
#include <Hork/Core/Parse.h>
#include <Hork/Core/Color.h>

HK_NAMESPACE_BEGIN

bool PhotometricData::ReadSamples(MutableArrayView<uint8_t> Samples, float& Intensity) const
{
    if (Samples.Size() != PHOTOMETRIC_DATA_SIZE)
    {
        LOG("PhotometricData::ReadSamples: wrong array size - should be {}\n", PHOTOMETRIC_DATA_SIZE);
        return false;
    }

    float unnormalizedData[PHOTOMETRIC_DATA_SIZE];

    Intensity = 0.0f;
    for (int i = 0; i < PHOTOMETRIC_DATA_SIZE; i++)
    {
#if 1
        float LdotDir = Math::Clamp((float)i / (PHOTOMETRIC_DATA_SIZE - 1) * 2.0f - 1.0f, -1.0f, 1.0f);
        float angle   = Math::Degrees(std::acos(LdotDir));
#else
        float angle = (float)i / PHOTOMETRIC_DATA_SIZE * 180.0f;
#endif
        unnormalizedData[i] = SampleAvgVertical(angle);

        Intensity = Math::Max(Intensity, unnormalizedData[i]);
    }

    const float normalizer = Intensity > 0.0f ? 1.0f / Intensity : 1.0f;
    for (int i = 0; i < PHOTOMETRIC_DATA_SIZE; i++)
    {
        // NOTE: Store in 2.2 gamma space for best results.
        Samples[i] = Math::Pow(Math::Saturate(unnormalizedData[i] * normalizer), 1.0f / 2.2f) * 255;
    }

    Intensity *= m_LampMultiplier * m_ElecBallFactor * m_ElecBlpFactor;

    return true;
}

float PhotometricData::SampleAvgVertical(float VerticalAgnle) const
{
    if (m_HorzAngles.IsEmpty())
    {
        // No horizontal angles
        return 0;
    }

    if (m_VertAngles.Size() < 2)
    {
        float intensity = 0;
        for (int i = 0; i < (int)m_HorzAngles.Size(); i++)
            intensity += m_Candela[i][0];
        return intensity / m_HorzAngles.Size();
    }

    int  vertialIndex;
    bool found = false;

    // Find vertical angle
    for (vertialIndex = 0; vertialIndex < (int)m_VertAngles.Size() - 1; vertialIndex++)
    {
        if (VerticalAgnle >= m_VertAngles[vertialIndex] && VerticalAgnle < m_VertAngles[vertialIndex + 1])
        {
            found = true;
            break;
        }
    }

    if (!found)
    {
        // Angle not found
        return 0;
    }

    // Sum horizontal intensities at specified vertical angle
    float intensity0 = 0;
    float intensity1 = 0;
    for (int i = 0; i < (int)m_HorzAngles.Size(); i++)
    {
        intensity0 += m_Candela[i][vertialIndex];
        intensity1 += m_Candela[i][vertialIndex + 1];
    }
    intensity0 /= m_HorzAngles.Size();
    intensity1 /= m_HorzAngles.Size();

    const float angleDelta = m_VertAngles[vertialIndex + 1] - m_VertAngles[vertialIndex];
    const float fract      = angleDelta > 0.0f ? (VerticalAgnle - m_VertAngles[vertialIndex]) / angleDelta : 0.0f;

    return Math::Lerp(intensity0, intensity1, fract);
}

float PhotometricData::Sample2D(float x, float y) const
{
    float z = 0;
    // Convert cartesian coordinate to polar coordinates
    const float distance = std::sqrt(x * x + y * y + z * z);

    const float angleV = /*Angl::Normalize360*/ (Math::Degrees(std::acos(y / distance)));

    float fractH = 0;

    Vector<float> const *horizA, *horizB;

    if (m_HorzAngles.Size() > 1)
    {
        int h = -1;

        // Find horizontal angle
        //const float angleH = Math::FMod( Math::Degrees( std::atan2( y, x ) + 180.0f ), 360.0f );
        //const float angleH = Math::FMod( Math::Degrees( std::atan2( y, x ) ), 360.0f );
        float angleH = /*Angl::Normalize360*/ (Math::Degrees(std::atan2(z, x)));

        //angleH = 75; // for debug

        for (int i = 0; i < (int)m_HorzAngles.Size() - 1; i++)
        {
            if (angleH >= m_HorzAngles[i] && angleH < m_HorzAngles[i + 1])
            {
                h = i;
                break;
            }
        }

        if (h == -1)
        {
            // Angle not found
            return 0;
        }

        const float angleDelta = m_HorzAngles[h + 1] - m_HorzAngles[h];

        fractH = angleDelta > 0.0f ? (angleH - m_HorzAngles[h]) / angleDelta : 0.0f;

        horizA = &m_Candela[h];
        horizB = &m_Candela[h + 1];
    }
    else if (m_HorzAngles.Size() == 1)
    {
        // Only one horizontal angle
        horizA = &m_Candela[0];
        horizB = &m_Candela[0];
    }
    else
    {
        // No horizontal angles
        return 0;
    }

    // Find vertical angle
    int v = -1;
    for (int i = 0; i < (int)m_VertAngles.Size() - 1; i++)
    {
        if (angleV >= m_VertAngles[i] && angleV < m_VertAngles[i + 1])
        {
            v = i;
            break;
        }
    }

    if (v == -1)
    {
        // Angle not found
        return 0;
    }

    // Get the four samples to use for bilinear interpolation
    const float a = (*horizA)[v];
    const float b = (*horizB)[v];
    const float c = (*horizA)[v + 1];
    const float d = (*horizB)[v + 1];

    const float angleDelta = m_VertAngles[v + 1] - m_VertAngles[v];

    const float fractV = angleDelta > 0.0f ? (angleV - m_VertAngles[v]) / angleDelta : 0.0f;

    const float candelas = Math::Lerp(Math::Lerp(a, b, fractH), Math::Lerp(c, d, fractH), fractV);

    const float attenuation = 1.0 / (distance * distance);

    return candelas * m_LampMultiplier * m_ElecBallFactor * m_ElecBlpFactor * attenuation;
}

float PhotometricData::SampleAvg(float x, float y) const
{
    float z = 0;
    // Convert cartesian coordinate to polar coordinates
    const float distance = std::sqrt(x * x + y * y + z * z);

    const float angleV = /*Angl::Normalize360*/ (Math::Degrees(std::acos(y / distance)));

    if (m_HorzAngles.IsEmpty())
    {
        // No horizontal angles
        return 0;
    }

    // Find vertical angle
    int v = -1;
    for (int i = 0; i < (int)m_VertAngles.Size() - 1; i++)
    {
        if (angleV >= m_VertAngles[i] && angleV < m_VertAngles[i + 1])
        {
            v = i;
            break;
        }
    }

    if (v == -1)
    {
        // Angle not found
        return 0;
    }

    // Sum horizontal angles at specified vertical angle
    float sum_v0 = 0;
    float sum_v1 = 0;
    for (int i = 0; i < m_HorzAngles.Size(); i++)
    {
        sum_v0 += m_Candela[i][v];
        sum_v1 += m_Candela[i][v + 1];
    }
    sum_v0 /= m_HorzAngles.Size();
    sum_v1 /= m_HorzAngles.Size();

    const float angleDelta = m_VertAngles[v + 1] - m_VertAngles[v];

    const float fractV = angleDelta > 0.0f ? (angleV - m_VertAngles[v]) / angleDelta : 0.0f;

    const float candelas = Math::Lerp(sum_v0, sum_v1, fractV);

    const float attenuation = 1.0 / (distance * distance);

    return candelas * m_LampMultiplier * m_ElecBallFactor * m_ElecBlpFactor * attenuation;
}

#if 0
void TestPhotometricData(PhotometricData const& photometricData)
{
    int      w = 512, h = 512;
    float    scale = 1.0f;
    HeapBlob dataBlob(w * h * 3);
    byte*    data = (byte*)dataBlob.GetData();
    for (int y = 0; y < h; y++)
    {
        for (int x = 0; x < w; x++)
        {
            float s = photometricData.Sample2D(((float)x - w * 0.5f - 0.5f) * scale, ((float)y - h * 0.5f - 0.5f) * scale);

            //byte c = LinearToSRGB_UChar( s );
            byte c = Math::Saturate(s * 0.5f) * 255;

            data[y * w * 3 + x * 3 + 0] = c;
            data[y * w * 3 + x * 3 + 1] = c;
            data[y * w * 3 + x * 3 + 2] = c;
        }
    }

    ImageWriteConfig wrt;
    wrt.Width = w;
    wrt.Height = h;
    wrt.NumChannels = 3;
    wrt.pData       = data;
    WriteImage("ies.png", wrt);

    for (int y = 0; y < h; y++)
    {
        for (int x = 0; x < w; x++)
        {
            float s = photometricData.SampleAvg(((float)x - w * 0.5f - 0.5f) * scale, ((float)y - h * 0.5f - 0.5f) * scale);

            //byte c = LinearToSRGB_UChar( s );
            byte c = Math::Saturate(s * 0.5f) * 255;

            data[y * w * 3 + x * 3 + 0] = c;
            data[y * w * 3 + x * 3 + 1] = c;
            data[y * w * 3 + x * 3 + 2] = c;
        }
    }

    WriteImage("ies_avg.png", wrt);

    float smin = Math::MaxValue<float>(), smax = Math::MinValue<float>();
    float linear[256];
    w = 256;
    for (int i = 0; i < w; i++)
    {
        //float LdotDir = Math::Clamp( (float)i / (w - 1) * 2.0f - 1.0f, -1.0f, 1.0f );
        //float angle2 = Math::Degrees( std::acos( LdotDir ) );

        float angle1 = (float)i / w * 180.0f;

        float s = photometricData.SampleAvgVertical(angle1);

        if (s > 0)
        {
            smin = Math::Min(smin, s);
        }

        smax = Math::Max(smax, s);

        linear[i] = s;
    }

    if (smax < 1.0f)
    {
        smax = 1;
    }
    const float norm = 1.0f / smax;
    for (int i = 0; i < w; i++)
    {
        linear[i] = linear[i] * norm;

        //byte c = Math::Saturate( linear[i] ) * 255;
        byte c = LinearToSRGB_UChar(linear[i] * 255);

        data[i * 3 + 0] = c;
        data[i * 3 + 1] = c;
        data[i * 3 + 2] = c;
    }
}
#endif

class IesParser
{
public:
    IesParser(StringView Text);

    PhotometricData Parse();

private:
    StringView NextLine();
    StringView NextToken();

    int         m_LineNum{};
    const char* m_Ptr{};
    const char* m_End{};
};

IesParser::IesParser(StringView Data) :
    m_Ptr(Data.Begin()), m_End(Data.End())
{}

PhotometricData IesParser::Parse()
{
    StringView text = NextLine();
    if (text.IsEmpty())
    {
        LOG("IesParser::Parse: Empty file\n");
        return {};
    }

    if (!text.Cmp("IESNA:LM-63-1995"))
    {
        // Format IESNA 95
    }
    else if (!text.Cmp("IESNA91"))
    {
        // Format = IESNA 91
    }
    else
    {
        // Format = IESNA_86
    }

    while (text.CmpN("TILT=", 5) && !text.IsEmpty())
        text = NextLine();

    if (text.IsEmpty())
    {
        LOG("IesParser::Parse: TILT= not found\n");
        return {};
    }

    int numVertAngles;
    int numHorzAngles;

    PhotometricData data;

    data.m_NumLamps       = Core::ParseUInt32(NextToken());
    data.m_LumensLamp     = Core::ParseFloat(NextToken());
    data.m_LampMultiplier = Core::ParseFloat(NextToken());

    numVertAngles = Core::ParseUInt32(NextToken());
    numHorzAngles = Core::ParseUInt32(NextToken());
    if (numVertAngles > 1024 || numHorzAngles > 1024)
    {
        LOG("IesParser::Parse: Invalid data\n");
        return {};
    }

    data.m_GonioType      = Core::ParseUInt32(NextToken());
    data.m_Units          = Core::ParseUInt32(NextToken());
    data.m_DimWidth       = Core::ParseFloat(NextToken());
    data.m_DimLength      = Core::ParseFloat(NextToken());
    data.m_DimHeight      = Core::ParseFloat(NextToken());
    data.m_ElecBallFactor = Core::ParseFloat(NextToken());
    data.m_ElecBlpFactor  = Core::ParseFloat(NextToken());
    data.m_ElecInputWatts = Core::ParseFloat(NextToken());

    data.m_VertAngles.Resize(numVertAngles);
    data.m_HorzAngles.Resize(numHorzAngles);

    for (float& angle : data.m_VertAngles)
        angle = Core::ParseFloat(NextToken());

    for (float& angle : data.m_HorzAngles)
        angle = Core::ParseFloat(NextToken());

    data.m_Candela.Resize(numHorzAngles);
    for (auto& vertAngles : data.m_Candela)
    {
        vertAngles.Resize(numVertAngles);
        for (float& angle : vertAngles)
            angle = Core::ParseFloat(NextToken());
    }

    return data;
}

StringView IesParser::NextLine()
{
    const char* lineStart = m_Ptr;
    const char* lineEnd   = m_End;

    // Find line bounds
    while (m_Ptr < m_End)
    {
        if (*m_Ptr == '\r' || *m_Ptr == '\n')
        {
            lineEnd = m_Ptr;
            if (m_Ptr + 1 < m_End && *m_Ptr == '\r' && *(m_Ptr + 1) == '\n')
                m_Ptr++;
            m_Ptr++;
            m_LineNum++;
            break;
        }
        m_Ptr++;
    }

    // Skip spaces at line start
    while (lineStart < lineEnd && (Core::CharIsBlank(*lineStart) || *lineStart == '\r'))
        lineStart++;

    // Skip spaces at line end
    while (lineEnd > lineStart && (Core::CharIsBlank(*(lineEnd - 1)) || *(lineEnd - 1) == '\r'))
        lineEnd--;

    return StringView(lineStart, lineEnd);
}

StringView IesParser::NextToken()
{
    while (m_Ptr < m_End && (Core::CharIsBlank(*m_Ptr) || *m_Ptr == '\n' || *m_Ptr == '\r'))
    {
        if (*m_Ptr == '\n')
            m_LineNum++;
        m_Ptr++;
    }

    const char* tokenStart = m_Ptr;

    while (!Core::CharIsBlank(*m_Ptr) && *m_Ptr != '\n' && *m_Ptr != '\r')
    {
        m_Ptr++;
    }

    return StringView(tokenStart, m_Ptr);
}

PhotometricData ParsePhotometricData(StringView Text)
{
    return IesParser(Text).Parse();
}

HK_NAMESPACE_END
