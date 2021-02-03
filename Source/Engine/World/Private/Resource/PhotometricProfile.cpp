/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2021 Alexander Samusev.

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

#include <World/Public/Resource/PhotometricProfile.h>
#include <World/Public/Resource/Asset.h>
#include <World/Public/Resource/Texture.h>
#include <Core/Public/Logger.h>
#include <Core/Public/Image.h>
#include "iesna/iesna.h"

AN_CLASS_META( APhotometricProfile )

int APhotometricProfile::PhotometricProfileCounter = 0;

APhotometricProfile::APhotometricProfile() {
    Core::ZeroMem( Data, sizeof( Data ) );
}

APhotometricProfile::~APhotometricProfile() {
}

void APhotometricProfile::Initialize( const byte * InData, float InIntensity ) {
    Intensity = InIntensity;
    Core::Memcpy( Data, InData, sizeof( Data ) );
}

void APhotometricProfile::LoadInternalResource( const char * _Path ) {
    if ( !Core::Stricmp( _Path, "/Default/PhotometricProfile/Default" ) ) {
        Intensity = 1.0f;
        Core::Memset( Data, 0xff, sizeof( Data ) );
        return;
    }

    GLogger.Printf( "Unknown internal resource %s\n", _Path );

    LoadInternalResource( "/Default/PhotometricProfile/Default" );
}

static float SampleIESAvgVertical( IE_DATA const * Data, float VerticalAgnle ) {
    if ( Data->photo.num_horz_angles <= 0 ) {
        // No horizontal angles
        return 0;
    }

    if ( Data->photo.num_vert_angles < 2 ) {
        float intensity = 0;
        for ( int i = 0 ; i < Data->photo.num_horz_angles ; i++ ) {
            intensity += Data->photo.pcandela[i][0];
        }
        return intensity / Data->photo.num_horz_angles;
    }

    int vertialIndex;

    // Find vertical angle
    for ( vertialIndex = 0; vertialIndex < Data->photo.num_vert_angles - 1; vertialIndex++ ) {
        if ( VerticalAgnle >= Data->photo.vert_angles[vertialIndex]
             && VerticalAgnle < Data->photo.vert_angles[vertialIndex + 1] ) {
            break;
        }
    }

    if ( vertialIndex == Data->photo.num_vert_angles ) {
        // Angle not found
        return 0;
    }

    // Sum horizontal intensities at specified vertical angle
    float intensity0 = 0;
    float intensity1 = 0;
    for ( int i = 0 ; i < Data->photo.num_horz_angles ; i++ ) {
        intensity0 += Data->photo.pcandela[i][vertialIndex];
        intensity1 += Data->photo.pcandela[i][vertialIndex+1];
    }
    intensity0 /= Data->photo.num_horz_angles;
    intensity1 /= Data->photo.num_horz_angles;

    const float angleDelta = Data->photo.vert_angles[vertialIndex + 1] - Data->photo.vert_angles[vertialIndex];
    const float fract = angleDelta > 0.0f ? (VerticalAgnle - Data->photo.vert_angles[vertialIndex]) / angleDelta : 0.0f;

    return Math::Lerp( intensity0, intensity1, fract );
}

static float SampleIES( IE_DATA const * iesData, float x, float y ) {
    float z = 0;
    // Convert cartesian coordinate to polar coordinates
    const float distance = StdSqrt( x * x + y * y + z * z );

    const float angleV = /*Angl::Normalize360*/(Math::Degrees( std::acos( y / distance ) ));

    float fractH = 0;

    float * horizA, *horizB;

    if ( iesData->photo.num_horz_angles > 1 ) {
        int h = -1;

        //for ( int i = 0; i < iesData->photo.num_horz_angles; i++ ) {
        //    GLogger.Printf( "Horizontal %d : %f\n", i, iesData->photo.horz_angles[i] );
        //}

        // Find horizontal angle
        //const float angleH = Math::FMod( Math::Degrees( std::atan2( y, x ) + 180.0f ), 360.0f );
        //const float angleH = Math::FMod( Math::Degrees( std::atan2( y, x ) ), 360.0f );
        float angleH = /*Angl::Normalize360*/(Math::Degrees( std::atan2( z, x ) ));

        angleH = 75;

        for ( int i = 0; i < iesData->photo.num_horz_angles - 1; i++ ) {
            if ( angleH >= iesData->photo.horz_angles[i] && angleH < iesData->photo.horz_angles[i + 1] ) {
                h = i;
                break;
            }
        }

        if ( h == -1 ) {
            // Angle not found
            return 0;
        }

        const float angleDelta = iesData->photo.horz_angles[h + 1] - iesData->photo.horz_angles[h];

        fractH = angleDelta > 0.0f ? (angleH - iesData->photo.horz_angles[h]) / angleDelta : 0.0f;

        horizA = iesData->photo.pcandela[h];
        horizB = iesData->photo.pcandela[h+1];

    } else if ( iesData->photo.num_horz_angles == 1 ) {
        // Only one horizontal angle
        horizA = iesData->photo.pcandela[0];
        horizB = iesData->photo.pcandela[0];
    } else {
        // No horizontal angles
        return 0;
    }

    // Find vertical angle
    int v = -1;
    for ( int i = 0; i < iesData->photo.num_vert_angles - 1; i++ ) {
        if ( angleV >= iesData->photo.vert_angles[i] && angleV < iesData->photo.vert_angles[i + 1] ) {
            v = i;
            break;
        }
    }

    if ( v == -1 ) {
        // Angle not found
        return 0;
    }

    // Get the four samples to use for bilinear interpolation
    const float a = horizA[v];
    const float b = horizB[v];
    const float c = horizA[v + 1];
    const float d = horizB[v + 1];

    const float angleDelta = iesData->photo.vert_angles[v + 1] - iesData->photo.vert_angles[v];

    const float fractV = angleDelta > 0.0f ? (angleV - iesData->photo.vert_angles[v]) / angleDelta : 0.0f;

    const float candelas = Math::Lerp( Math::Lerp( a, b, fractH ), Math::Lerp( c, d, fractH ), fractV );

    const float attenuation = 1.0 / (distance * distance);

    return candelas * iesData->lamp.multiplier * iesData->elec.ball_factor * iesData->elec.blp_factor * attenuation;
}

static float SampleIESAvg( IE_DATA const * iesData, float x, float y ) {
    float z = 0;
    // Convert cartesian coordinate to polar coordinates
    const float distance = StdSqrt( x * x + y * y + z * z );

    const float angleV = /*Angl::Normalize360*/(Math::Degrees( std::acos( y / distance ) ));

    if ( iesData->photo.num_horz_angles <= 0 ) {
        // No horizontal angles
        return 0;
    }

    // Find vertical angle
    int v = -1;
    for ( int i = 0; i < iesData->photo.num_vert_angles - 1; i++ ) {
        if ( angleV >= iesData->photo.vert_angles[i] && angleV < iesData->photo.vert_angles[i + 1] ) {
            v = i;
            break;
        }
    }

    if ( v == -1 ) {
        // Angle not found
        return 0;
    }

    // Sum horizontal angles at specified vertical angle
    float sum_v0 = 0;
    float sum_v1 = 0;
    for ( int i = 0 ; i < iesData->photo.num_horz_angles ; i++ ) {
        sum_v0 += iesData->photo.pcandela[i][v];
        sum_v1 += iesData->photo.pcandela[i][v+1];
    }
    sum_v0 /= iesData->photo.num_horz_angles;
    sum_v1 /= iesData->photo.num_horz_angles;

    const float angleDelta = iesData->photo.vert_angles[v + 1] - iesData->photo.vert_angles[v];

    const float fractV = angleDelta > 0.0f ? (angleV - iesData->photo.vert_angles[v]) / angleDelta : 0.0f;

    const float candelas = Math::Lerp( sum_v0, sum_v1, fractV );

    const float attenuation = 1.0 / (distance * distance);

    return candelas * iesData->lamp.multiplier * iesData->elec.ball_factor * iesData->elec.blp_factor * attenuation;
}

static void TestIES( IE_DATA & PhotoData )
{
    int w = 512, h = 512;
    float scale = 1.0f;
    byte * data = (byte*)GHeapMemory.Alloc( w * h * 3 );
    for ( int y = 0 ; y < h ; y++ ) {
        for ( int x = 0 ; x < w ; x++ ) {
            float s = SampleIES( &PhotoData,
                ((float)x - w*0.5f - 0.5f) * scale, ((float)y - h*0.5f - 0.5f) * scale );

            //byte c = LinearToSRGB_UChar( s );
            byte c = Math::Saturate( s * 0.5f ) * 255;

            data[y*w*3 + x*3 + 0] = c;
            data[y*w*3 + x*3 + 1] = c;
            data[y*w*3 + x*3 + 2] = c;
        }
    }
    AFileStream f;
    f.OpenWrite( "ies.png" );
    WritePNG( f, w, h, 3, data, w*3 );
    for ( int y = 0 ; y < h ; y++ ) {
        for ( int x = 0 ; x < w ; x++ ) {
            float s = SampleIESAvg( &PhotoData,
                ((float)x - w*0.5f - 0.5f) * scale, ((float)y - h*0.5f - 0.5f) * scale );

            //byte c = LinearToSRGB_UChar( s );
            byte c = Math::Saturate( s * 0.5f ) * 255;

            data[y*w*3 + x*3 + 0] = c;
            data[y*w*3 + x*3 + 1] = c;
            data[y*w*3 + x*3 + 2] = c;
        }
    }
    f.OpenWrite( "ies_avg.png" );
    WritePNG( f, w, h, 3, data, w*3 );
    float smin = Math::MaxValue<float>(), smax = Math::MinValue< float >();
    float linear[256];
    w = 256;
    for ( int i = 0 ; i < w ; i++ )
    {
        //float LdotDir = Math::Clamp( (float)i / (w - 1) * 2.0f - 1.0f, -1.0f, 1.0f );
        //float angle2 = Math::Degrees( std::acos( LdotDir ) );

        float angle1 = (float)i/w * 180.0f;

        //GLogger.Printf( "Sample angle %f\n", angle2 );

        float s = SampleIESAvgVertical( &PhotoData, angle1 );

        if ( s > 0 ) {
            smin = Math::Min( smin, s );
        }

        smax = Math::Max( smax, s );

        linear[i] = s;
    }

    if ( smax < 1.0f ) {
        smax = 1;
    }
    const float norm = 1.0f / smax;
    for ( int i = 0 ; i < w ; i++ )
    {
        linear[i] = linear[i] * norm;

        //byte c = Math::Saturate( linear[i] ) * 255;
        byte c = LinearToSRGB_UChar( linear[i] * 255 );

        data[i*3+0] = c;
        data[i*3+1] = c;
        data[i*3+2] = c;
    }

    GHeapMemory.Free( data );
}

bool APhotometricProfile::LoadResource( IBinaryStream & Stream ) {
    const char * fn = Stream.GetFileName();

    int extOffset = Core::FindExt( fn );

    if ( !Core::Stricmp( &fn[extOffset], ".ies" ) ) {
        IE_Context context;
        IE_DATA photoData;

        context.calloc = [](size_t n, size_t sz)
        {
            return GZoneMemory.ClearedAlloc( n * sz );
        };
        context.free = []( void * p )
        {
            GZoneMemory.Free( p );
        };
        context.rewind = []( void * userData )
        {
            AFileStream * f = (AFileStream *)userData;
            f->Rewind();
        };
        context.fgets = []( char * pbuf, size_t size, void * userData )
        {
            AFileStream * f = (AFileStream *)userData;
            return f->Gets( pbuf, size );
        };
        context.userData = &Stream;

        if ( !IES_Load( &context, &photoData ) ) {
            return false;
        }

        Intensity = 0.0f;

        float unnormalizedData[PHOTOMETRIC_DATA_SIZE];

        for ( int i = 0 ; i < PHOTOMETRIC_DATA_SIZE ; i++ ) {
#if 1
            float LdotDir = Math::Clamp( (float)i / (PHOTOMETRIC_DATA_SIZE - 1) * 2.0f - 1.0f, -1.0f, 1.0f );
            float angle = Math::Degrees( std::acos( LdotDir ) );
#else
            float angle = (float)i / PHOTOMETRIC_DATA_SIZE * 180.0f;
#endif
            unnormalizedData[i] = SampleIESAvgVertical( &photoData, angle );

            Intensity = Math::Max( Intensity, unnormalizedData[i] );
        }

        const float normalizer = Intensity > 0.0f ? 1.0f / Intensity : 1.0f;
        for ( int i = 0 ; i < PHOTOMETRIC_DATA_SIZE ; i++ ) {
            //Data[i] = Math::Saturate( unnormalizedData[i] * normalizer ) * 255;
            Data[i] = Math::Pow(Math::Saturate( unnormalizedData[i] * normalizer ), 1.0f / 2.2f ) * 255;
            // TODO: Try sRGB
        }

        const float scale = photoData.lamp.multiplier * photoData.elec.ball_factor * photoData.elec.blp_factor;

        Intensity *= scale;

        TestIES( photoData );

        IES_Free( &context, &photoData );
    } else {

        uint32_t fileFormat;
        uint32_t fileVersion;

        fileFormat = Stream.ReadUInt32();

        if ( fileFormat != FMT_FILE_TYPE_PHOTOMETRIC_PROFILE ) {
            GLogger.Printf( "Expected file format %d\n", FMT_FILE_TYPE_PHOTOMETRIC_PROFILE );
            return false;
        }

        fileVersion = Stream.ReadUInt32();

        if ( fileVersion != FMT_VERSION_PHOTOMETRIC_PROFILE ) {
            GLogger.Printf( "Expected file version %d\n", FMT_VERSION_PHOTOMETRIC_PROFILE );
            return false;
        }

        AString guid;

        Stream.ReadObject( guid );

        Intensity = Stream.ReadFloat();
        Stream.ReadBuffer( Data, sizeof( Data ) );
    }

    return true;
}

void APhotometricProfile::WritePhotometricData( ATexture * ProfileTexture, int FrameIndex ) {
    if ( FrameNum == FrameIndex ) {
        // Already updated at this frame
        return;
    }
    FrameNum = FrameIndex;
    if ( ProfileTexture ) {
        ProfileTexture->WriteTextureData1DArray( 0, PHOTOMETRIC_DATA_SIZE, PhotometricProfileCounter, 0, Data );
        PhotometricProfileIndex = PhotometricProfileCounter;
        PhotometricProfileCounter = ( PhotometricProfileCounter + 1 ) & 0xff;
    }
}
