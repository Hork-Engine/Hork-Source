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

#include <Hork/Core/CoreApplication.h>

#include <Hork/Core/IO.h>
#include <Hork/Core/Parse.h>
#include <Hork/Core/Logger.h>
#include <Hork/Core/Platform.h>
#include <Hork/Image/RawImage.h>
#include <Hork/Image/Image.h>
#include <Hork/Image/ImageEncoders.h>
#include <Hork/Resources/Resource_Texture.h>

HK_NAMESPACE_BEGIN

const char* occlusion = nullptr;
const char* roughness = nullptr;
const char* metallic = nullptr;
const char* displacement = nullptr;
const char* emission = nullptr;
int occlusionChannel = 0;
int roughnessChannel = 0;
int metallicChannel = 0;
int displacementChannel = 0;
int emissionChannel = 0;
const char* normal = nullptr;
bool generateMipmaps = false;
bool convertFromDirectXNormalMap = false;
bool compressNormals = true;
ImageMipmapConfig mipmapConfig;
NormalRoughnessImportSettings::ROUGHNESS_BAKE roughnessBake = NormalRoughnessImportSettings::ROUGHNESS_BAKE_vMF;
bool antialiasing = false;
NORMAL_MAP_PACK normalMapPack = NORMAL_MAP_PACK_RGBA_BC1_COMPATIBLE;
enum PBR_PRESET
{
    UNDEFINED,
    ORMX,
    ORMX_BC1,
    ORMX_BC3,
    ORMX_BC7,

    // TODO:
    //OCCLUSION_BC4,
    //ROUGHNESS_BC4,
    //METALLIC_BC4,
    //DISPLACEMENT_BC4,
};
PBR_PRESET pbrPreset = UNDEFINED;
RawImageResampleParams resampleParams;
bool resample = false;
const char* outputPBRMap = nullptr;
const char* outputNormalMap = nullptr;

bool IsCompressionRequired()
{
    return pbrPreset == ORMX_BC1 || pbrPreset == ORMX_BC3 || pbrPreset == ORMX_BC7;
}

RawImage CreateORMX()
{
    const char* inputImages[4] = {occlusion, roughness, metallic, displacement ? displacement : emission};
    const char* uniqueImages[4];
    int imageRemapping[4];
    int uniqueImageCount = 0;

    for (int i = 0, j; i < 4; ++i)
    {
        for (j = 0; j < uniqueImageCount; ++j)
        {
            if (!inputImages[i] && !uniqueImages[j])
            {
                imageRemapping[i] = j;
                break;
            }

            if (inputImages[i] && uniqueImages[j] && !Core::Strcmp(inputImages[i], uniqueImages[j]))
            {
                imageRemapping[i] = j;
                break;
            }
        }
        if (j == uniqueImageCount)
        {
            uniqueImages[uniqueImageCount] = inputImages[i];
            imageRemapping[i] = uniqueImageCount;
            uniqueImageCount++;
        }
    }

    if (uniqueImageCount == 0)
        return {};

    RawImage images[4];
    uint32_t width = 0;
    uint32_t height = 0;

    for (int i = 0; i < uniqueImageCount; ++i)
    {
        if (uniqueImages[i])
        {
            LOG("Loading {}...\n", uniqueImages[i]);
            images[i] = CreateRawImage(uniqueImages[i], RAW_IMAGE_FORMAT_RGBA8);
            if (!images[i])
            {
                LOG("Failed to load {}\n", uniqueImages[i]);
                return {};
            }

            if (resample)
                images[i] = ResampleRawImage(images[i], resampleParams);

            width = images[i].GetWidth();
            height = images[i].GetHeight();
        }
    }

    for (int i = 0; i < uniqueImageCount; ++i)
    {
        if (uniqueImages[i])
        {
            if (images[i].GetWidth() != width || images[i].GetHeight() != height)
            {
                LOG("The source images have different sizes. Use the -resample option to configure resampling parameters.\n");
                return {};
            }
        }
    }

    const uint32_t blockSize = IsCompressionRequired() ? 4 : 1;

    resampleParams.ScaledWidth = generateMipmaps ? Math::Max<uint32_t>(Math::ToClosestPowerOfTwo(width), blockSize) : Align(width, blockSize);
    resampleParams.ScaledHeight = generateMipmaps ? Math::Max<uint32_t>(Math::ToClosestPowerOfTwo(height), blockSize) : Align(height, blockSize);

    if (resampleParams.ScaledWidth != width || resampleParams.ScaledHeight != height)
    {
        for (int i = 0; i < uniqueImageCount; ++i)
        {
            if (uniqueImages[i])
                images[i] = ResampleRawImage(images[i], resampleParams);
        }

        width = resampleParams.ScaledWidth;
        height = resampleParams.ScaledHeight;
    }

    RawImage combinedImage;
    combinedImage.Reset(width, height, RAW_IMAGE_FORMAT_RGBA8);

    int srcChannels[4] = {occlusionChannel, roughnessChannel, metallicChannel, displacement ? displacementChannel : emissionChannel};

    // Set default values
    combinedImage.Clear({1, 1, 0, 1});

    for (int i = 0; i < 4; ++i)
    {
        auto const& source = images[imageRemapping[i]];
        if (source)
        {
            CopyImageChannel<uint8_t>(reinterpret_cast<uint8_t const*>(source.GetData()),
                reinterpret_cast<uint8_t*>(combinedImage.GetData()),
                combinedImage.GetWidth(),
                combinedImage.GetHeight(),
                4,
                4,
                srcChannels[i],
                i);
        }
    }

    return combinedImage;
}

static void NormalizeVectors(Float3* pVectors, size_t Count)
{
    Float3* end = pVectors + Count;
    while (pVectors < end)
    {
        *pVectors = *pVectors * 2.0f - 1.0f;
        pVectors->NormalizeSelf();
        ++pVectors;
    }
}

RawImage CreateNormalMap()
{
    LOG("Loading {}...\n", normal);
    RawImage rawImage = CreateRawImage(normal, RAW_IMAGE_FORMAT_RGB32_FLOAT);
    if (!rawImage)
    {
        LOG("Failed to load {}\n", normal);
        return {};
    }

    if (convertFromDirectXNormalMap)
        rawImage.InvertGreen();

    RawImageResampleParams normalMapResampleParams = resampleParams;

    // Override resample filter for normal maps
    // TODO: Check what filter is better for normal maps
    normalMapResampleParams.HorizontalFilter = normalMapResampleParams.VerticalFilter = IMAGE_RESAMPLE_FILTER_TRIANGLE;

    // NOTE: Currently all compression methods have blockSize = 4
    const uint32_t blockSize = compressNormals ? 4 : 1;

    if (resample)
    {
        normalMapResampleParams.ScaledWidth  = generateMipmaps ? Math::Max<uint32_t>(Math::ToClosestPowerOfTwo(resampleParams.ScaledWidth), blockSize) : Align(resampleParams.ScaledWidth, blockSize);
        normalMapResampleParams.ScaledHeight = generateMipmaps ? Math::Max<uint32_t>(Math::ToClosestPowerOfTwo(resampleParams.ScaledHeight), blockSize) : Align(resampleParams.ScaledHeight, blockSize);
    }
    else
    {
        normalMapResampleParams.ScaledWidth  = generateMipmaps ? Math::Max<uint32_t>(Math::ToClosestPowerOfTwo(rawImage.GetWidth()), blockSize) : Align(rawImage.GetWidth(), blockSize);
        normalMapResampleParams.ScaledHeight = generateMipmaps ? Math::Max<uint32_t>(Math::ToClosestPowerOfTwo(rawImage.GetHeight()), blockSize) : Align(rawImage.GetHeight(), blockSize);
    }

    if (normalMapResampleParams.ScaledWidth != rawImage.GetWidth() || normalMapResampleParams.ScaledHeight != rawImage.GetHeight())
    {
        rawImage = ResampleRawImage(rawImage, normalMapResampleParams);
    }

    NormalizeVectors((Float3*)rawImage.GetData(), rawImage.GetWidth() * rawImage.GetHeight());

    return rawImage;
}

NORMAL_MAP_PACK GetNormalMapPack(StringView name)
{
    if (!name.Icmp("RGBA_BC1_COMPATIBLE"))
        return NORMAL_MAP_PACK_RGBA_BC1_COMPATIBLE;
    if (!name.Icmp("RG_BC5_COMPATIBLE"))
        return NORMAL_MAP_PACK_RG_BC5_COMPATIBLE;
    if (!name.Icmp("SPHEREMAP_BC5_COMPATIBLE"))
        return NORMAL_MAP_PACK_SPHEREMAP_BC5_COMPATIBLE;
    if (!name.Icmp("STEREOGRAPHIC_BC5_COMPATIBLE"))
        return NORMAL_MAP_PACK_STEREOGRAPHIC_BC5_COMPATIBLE;
    if (!name.Icmp("PARABOLOID_BC5_COMPATIBLE"))
        return NORMAL_MAP_PACK_PARABOLOID_BC5_COMPATIBLE;
    if (!name.Icmp("RGBA_BC3_COMPATIBLE"))
        return NORMAL_MAP_PACK_RGBA_BC3_COMPATIBLE;
    LOG("GetNormalMapPack: unknown {}\n, return RGBA_BC1_COMPATIBLE\n", name);
    return NORMAL_MAP_PACK_RGBA_BC1_COMPATIBLE;
}

bool ImportImage(ImageStorage& storage, StringView fileName)
{
    LOG("Importing texture {}...\n", fileName);

    File file = File::sOpenWrite(fileName);
    if (!file)
    {
        LOG("Failed to open \"{}\"\n", fileName);
        return false;
    }

    AssetUtils::CreateTexture(file, storage);
    return true;
}

int RunApplication()
{
    Core::SetEnableConsoleOutput(true);

    const char* help = R"(
    -h                      -- Help

    -occlusion <filename> <channel_index>   -- Source file for ambient occlusion map (default = 1)
    -roughness <filename> <channel_index>   -- Source file for roughness map (default = 1)
    -metallic  <filename> <channel_index>   -- Source file for metallic map (default = 0)
    -displacement_to_alpha <filename> <channel_index>   -- Source file for displacement
    -emission_to_alpha     <filename> <channel_index>   -- Source file for emission

    -normal <source file> <dest file>   -- Source and destination files for normal map

    -a <type>               -- Apply normal map antialiasing:
                                vMF
                                Toksvig
                               NOTE If antialiasing is enabled, then the roughness and normal maps should have the same size

    -dx_normal              -- Convert from DirectX normal map

    -normal_pack <type>     -- Normal map packing type:
                                RGBA_BC1_COMPATIBLE
                                RG_BC5_COMPATIBLE
                                SPHEREMAP_BC5_COMPATIBLE
                                STEREOGRAPHIC_BC5_COMPATIBLE
                                PARABOLOID_BC5_COMPATIBLE
                                RGBA_BC3_COMPATIBLE

    -no_compress_normals    -- Don't compress normal map. If not specified, will be used BC1_UNORM, BC3_UNORM or BC5_UNORM - depends on packing type.

    -resample <width> <height>                  -- Scale input textures
    -resample_edge_mode  <mode_h> <mode_v>      -- Use edge mode for resampling (clamp/reflect/wrap/zero)
    -resample_orm_filter <filter_h> <filter_v>  -- Use filter for resampling (box/triangle/cubicspline/catmullrom/mitchell)

    -pbr_preset <preset> <filename>    -- Output PBR preset:
                                ORMX     (R - occlusion, G - roughness, B - metallic, A - optional map)
                                    Output format RGBA8_UNORM
                                ORMX_BC1 (R - occlusion, G - roughness, B - metallic)
                                    Output format BC1_UNORM
                                ORMX_BC3 (R - occlusion, G - roughness, B - metallic, A - optional map)
                                    Output format BC3_UNORM
                                ORMX_BC7 (R - occlusion, G - roughness, B - metallic, A - optional map)
                                    Output format BC7_UNORM

    Notes about emission:
            To convert the emission in full RGB color use TextureImporter.
            BC1 compression is preferred because it requires less memory.
            Keep emission color in sRGB color space.

    Mipmap generation:
        Don't specify if you don't want to generate mipmaps

        -mip_edge_mode <mode>   -- Mipmap resampling edge mode:
                                        clamp
                                        reflect
                                        wrap
                                        zero

        -mip_filter <filter>    -- Mipmap resampling filter:
                                        box
                                        triangle
                                        cubicspline
                                        catmullrom
                                        mitchell
    )";

    auto& args = CoreApplication::sArgs();
    int i;

    i = args.Find("-h");
    if (i != -1)
    {
        LOG(help);
        return 0;
    }

    i = args.Find("-pbr_preset");
    if (i != -1 && i + 2 < args.Count())
    {
        if (!Core::Stricmp(args.At(i + 1), "ORMX"))
            pbrPreset = ORMX;
        else if (!Core::Stricmp(args.At(i + 1), "ORMX_BC1"))
            pbrPreset = ORMX_BC1;
        else if (!Core::Stricmp(args.At(i + 1), "ORMX_BC3"))
            pbrPreset = ORMX_BC3;
        else if (!Core::Stricmp(args.At(i + 1), "ORMX_BC7"))
            pbrPreset = ORMX_BC7;
        else
        {
            LOG("Unknown PBR preset: {}\n", args.At(i + 1));
            return -1;
        }

        outputPBRMap = args.At(i + 2);
    }    

    i = args.Find("-occlusion");
    if (i != -1 && i + 2 < args.Count())
    {
        occlusion = args.At(i + 1);
        occlusionChannel = Core::ParseInt32(args.At(i + 2));
        if (occlusionChannel < 0 || occlusionChannel >= 4)
        {
            LOG("Invalid channel index {}\n", occlusionChannel);
            return -1;
        }
    }

    i = args.Find("-roughness");
    if (i != -1 && i + 2 < args.Count())
    {
        roughness = args.At(i + 1);
        roughnessChannel = Core::ParseInt32(args.At(i + 2));
        if (roughnessChannel < 0 || roughnessChannel >= 4)
        {
            LOG("Invalid channel index {}\n", roughnessChannel);
            return -1;
        }
    }

    i = args.Find("-metallic");
    if (i != -1 && i + 2 < args.Count())
    {
        metallic = args.At(i + 1);
        metallicChannel = Core::ParseInt32(args.At(i + 2));
        if (metallicChannel < 0 || metallicChannel >= 4)
        {
            LOG("Invalid channel index {}\n", metallicChannel);
            return -1;
        }
    }

    i = args.Find("-displacement_to_alpha");
    if (i != -1 && i + 2 < args.Count())
    {
        if (pbrPreset == ORMX_BC1)
        {
            LOG("Warning: The specified displacement will be ignored for the ORMX_BC1 preset\n");
        }
        else
        {
            displacement = args.At(i + 1);
            displacementChannel = Core::ParseInt32(args.At(i + 2));
            if (displacementChannel < 0 || displacementChannel >= 4)
            {
                LOG("Invalid channel index {}\n", displacementChannel);
                return -1;
            }
        }        
    }
    else
    {
        i = args.Find("-emission_to_alpha");
        if (i != -1 && i + 2 < args.Count())
        {
            if (pbrPreset == ORMX_BC1)
            {
                LOG("Warning: The specified emission will be ignored for the ORMX_BC1 preset\n");
            }
            else
            {
                emission = args.At(i + 1);
                emissionChannel = Core::ParseInt32(args.At(i + 2));
                if (emissionChannel < 0 || emissionChannel >= 4)
                {
                    LOG("Invalid channel index {}\n", emissionChannel);
                    return -1;
                }
            }        
        }
    }

    i = args.Find("-normal");
    if (i != -1 && i + 2 < args.Count())
    {
        normal = args.At(i + 1);
        outputNormalMap = args.At(i + 2);
    }

    i = args.Find("-a");
    if (i != -1 && i + 1 < args.Count())
    {
        if (!normal)
        {
            LOG("Warning: The normal map is not specified! Antialiasing will be ignored\n");
        }
        else
        {
            if (!Core::Stricmp(args.At(i + 1), "vMF"))
            {
                roughnessBake = NormalRoughnessImportSettings::ROUGHNESS_BAKE_vMF;
                antialiasing = true;
            }
            else if (!Core::Stricmp(args.At(i + 1), "Toksvig"))
            {
                roughnessBake = NormalRoughnessImportSettings::ROUGHNESS_BAKE_TOKSVIG;
                antialiasing = true;
            }
        }
    }

    i = args.Find("-dx_normal");
    if (i != -1)
    {
        convertFromDirectXNormalMap = true;
    }

    i = args.Find("-no_compress_normals");
    if (i != -1)
    {
        compressNormals = false;
    }

    i = args.Find("-normal_pack");
    if (i != -1 && i + 1 < args.Count())
    {
        normalMapPack = GetNormalMapPack(args.At(i + 1));
    }

    i = args.Find("-mip_edge_mode");
    if (i != -1 && i + 1 < args.Count())
    {
        mipmapConfig.EdgeMode = GetResampleEdgeMode(args.At(i + 1));
        generateMipmaps = true;
    }

    i = args.Find("-mip_filter");
    if (i != -1 && i + 1 < args.Count())
    {
        mipmapConfig.Filter = GetResampleFilter(args.At(i + 1));
        generateMipmaps = true;
    }

    i = args.Find("-resample");
    if (i != -1 && i + 2 < args.Count())
    {
        resampleParams.ScaledWidth = Core::ParseUInt32(args.At(i + 1));
        resampleParams.ScaledHeight = Core::ParseUInt32(args.At(i + 2));
        if (resampleParams.ScaledWidth == 0 || resampleParams.ScaledHeight == 0)
        {
            LOG("Invalid resample size {} x {}\n", resampleParams.ScaledWidth, resampleParams.ScaledHeight);
            return -1;
        }
        if (resampleParams.ScaledWidth > 4096 || resampleParams.ScaledHeight > 4096)
        {
            LOG("Resulting texture size is too large {} x {}\n", resampleParams.ScaledWidth, resampleParams.ScaledHeight);
            return -1;
        }
        resample = true;

        if (IsCompressionRequired())
        {
            const uint32_t blockSize = 4;

            resampleParams.ScaledWidth  = generateMipmaps ? Math::Max<uint32_t>(Math::ToClosestPowerOfTwo(resampleParams.ScaledWidth), blockSize) : Align(resampleParams.ScaledWidth, blockSize);
            resampleParams.ScaledHeight = generateMipmaps ? Math::Max<uint32_t>(Math::ToClosestPowerOfTwo(resampleParams.ScaledHeight), blockSize) : Align(resampleParams.ScaledHeight, blockSize);
        }
    }

    i = args.Find("-resample_edge_mode");
    if (i != -1 && i + 2 < args.Count())
    {
        resampleParams.HorizontalEdgeMode = GetResampleEdgeMode(args.At(i + 1));
        resampleParams.VerticalEdgeMode = GetResampleEdgeMode(args.At(i + 2));
    }

    i = args.Find("-resample_orm_filter");
    if (i != -1 && i + 2 < args.Count())
    {
        resampleParams.HorizontalFilter = GetResampleFilter(args.At(i + 1));
        resampleParams.VerticalFilter = GetResampleFilter(args.At(i + 2));
    }

    RawImage ormx;
    RawImage normalmap;

    switch (pbrPreset)
    {
    case ORMX:
    case ORMX_BC1:
    case ORMX_BC3:
    case ORMX_BC7:
        ormx = CreateORMX();
        if (!ormx)
            return -1;
        break;
    }

    if (normal)
    {
        normalmap = CreateNormalMap();
        if (!normalmap)
            return -1;
    }

    // Ensure normal map and ormx are the same size
    if (antialiasing && ormx && normalmap && (ormx.GetWidth() != normalmap.GetWidth() || ormx.GetHeight() != normalmap.GetHeight()))
    {
        LOG("Can't apply antialiasing. ORMX and NormalMap must be the same size.\n");
        return -1;
    }

    ImageStorage uncompressedORMX;
    if (ormx)
    {
        ImageStorageDesc desc;
        desc.Type       = TEXTURE_2D;
        desc.Format     = TEXTURE_FORMAT_RGBA8_UNORM;
        desc.Width      = ormx.GetWidth();
        desc.Height     = ormx.GetHeight();
        desc.SliceCount = 1;
        desc.NumMipmaps = generateMipmaps ? CalcNumMips(desc.Format, desc.Width, desc.Height) : 1;
        desc.Flags      = IMAGE_STORAGE_NO_ALPHA;

        uncompressedORMX = ImageStorage(desc);
        ImageSubresource subresource = uncompressedORMX.GetSubresource({0, 0});

        subresource.Write(0, 0, desc.Width, desc.Height, ormx.GetData());

        if (generateMipmaps)
            uncompressedORMX.GenerateMipmaps(mipmapConfig);

        if (antialiasing && normalmap)
        {
            RawImage averageNormals;
            
            // Update roughness
            for (uint32_t level = 1; level < uncompressedORMX.GetDesc().NumMipmaps; ++level)
            {
                ImageSubresource levelSubresource = uncompressedORMX.GetSubresource({0, level});        

                RawImageResampleParams normalMapResample;
                normalMapResample.Flags              = RAW_IMAGE_RESAMPLE_FLAG_DEFAULT;
                normalMapResample.HorizontalEdgeMode = mipmapConfig.EdgeMode;
                normalMapResample.VerticalEdgeMode   = mipmapConfig.EdgeMode;
                normalMapResample.HorizontalFilter   = IMAGE_RESAMPLE_FILTER_TRIANGLE;
                normalMapResample.VerticalFilter     = IMAGE_RESAMPLE_FILTER_TRIANGLE;
                normalMapResample.ScaledWidth        = levelSubresource.GetWidth();
                normalMapResample.ScaledHeight       = levelSubresource.GetHeight();

                averageNormals = ResampleRawImage(level == 1 ? normalmap : averageNormals, normalMapResample);

                uint8_t* roughnessData = reinterpret_cast<uint8_t*>(levelSubresource.GetData()) + 1; // Roughness in the second channel of ORMX map

                uint32_t pixCount = levelSubresource.GetWidth() * levelSubresource.GetHeight();
                for (uint32_t index = 0; index < pixCount; index++)
                {
                    Float3 n = reinterpret_cast<Float3*>(averageNormals.GetData())[index];

                    float r2 = n.LengthSqr();
                    if (r2 > 1e-8f && r2 < 1.0f)
                    {
                        if (roughnessBake == NormalRoughnessImportSettings::ROUGHNESS_BAKE_vMF)
                        {
                            // vMF
                            // Equation from http://graphicrants.blogspot.com/2018/05/normal-map-filtering-using-vmf-part-3.html
                            float variance = 2.0f * Math::RSqrt(r2) * (1.0f - r2) / (3.0f - r2);

                            float roughnessVal = *roughnessData / 255.0f;

                            *roughnessData = Math::Round(Math::Saturate(Math::Sqrt(roughnessVal * roughnessVal + variance)) * 255.0f);
                        }
                        else
                        {
                            auto RoughnessToSpecPower = [](float Roughness) -> float
                                {
                                    return 2.0f / (Roughness * Roughness) - 2.0f;
                                };
                            auto SpecPowerToRoughness = [](float Spec) -> float
                                {
                                    return sqrt(2.0f / (Spec + 2.0f));
                                };

                            // Toksvig
                            // https://blog.selfshadow.com/2011/07/22/specular-showdown/
                            float r         = sqrt(r2);
                            float specPower = RoughnessToSpecPower(Math::Max<uint8_t>(*roughnessData, 1) / 255.0f);
                            float ft        = r / Math::Lerp(specPower, 1.0f, r);

                            *roughnessData = Math::Round(Math::Saturate(SpecPowerToRoughness(ft * specPower)) * 255.0f);
                        }
                    }

                    roughnessData += 4;
                }
            }
        }
    }

    // Compress ORMX
    if (pbrPreset == ORMX_BC1 || pbrPreset == ORMX_BC3 || pbrPreset == ORMX_BC7)
    {
        ImageStorageDesc desc;
        desc.Type       = TEXTURE_2D;

        switch (pbrPreset)
        {
        case ORMX_BC1:
            desc.Format = TEXTURE_FORMAT_BC1_UNORM;
            break;
        case ORMX_BC3:
            desc.Format = TEXTURE_FORMAT_BC3_UNORM;
            break;
        case ORMX_BC7:
            desc.Format = TEXTURE_FORMAT_BC7_UNORM;
            break;
        }

        desc.Width      = uncompressedORMX.GetDesc().Width;
        desc.Height     = uncompressedORMX.GetDesc().Height;
        desc.SliceCount = 1;
        desc.NumMipmaps = generateMipmaps ? CalcNumMips(desc.Format, desc.Width, desc.Height) : 1;
        desc.Flags      = IMAGE_STORAGE_NO_ALPHA;

        ImageStorage compressedORMX = ImageStorage(desc);

        for (uint32_t level = 0; level < desc.NumMipmaps; ++level)
        {
            ImageSubresource src = uncompressedORMX.GetSubresource({0, level});
            ImageSubresource dst = compressedORMX.GetSubresource({0, level});

            HK_ASSERT(src.GetWidth() == dst.GetWidth() && src.GetHeight() == dst.GetHeight());

            switch (pbrPreset)
            {
            case ORMX_BC1:
                TextureBlockCompression::CompressBC1(src.GetData(), dst.GetData(), dst.GetWidth(), dst.GetHeight());
                break;
            case ORMX_BC3:
                TextureBlockCompression::CompressBC3(src.GetData(), dst.GetData(), dst.GetWidth(), dst.GetHeight());
                break;
            case ORMX_BC7:
                TextureBlockCompression::CompressBC7(src.GetData(), dst.GetData(), dst.GetWidth(), dst.GetHeight());
                break;
            }            
        }

        if (!ImportImage(compressedORMX, outputPBRMap))
            return -1;
    }
    else
    {
        if (!ImportImage(uncompressedORMX, outputPBRMap))
            return -1;
    }

    if (normal)
    {
        ImageStorage normalMapStorage = CreateNormalMap(reinterpret_cast<Float3 const*>(normalmap.GetData()), normalmap.GetWidth(), normalmap.GetHeight(), normalMapPack, compressNormals, generateMipmaps,
            mipmapConfig.EdgeMode, IMAGE_RESAMPLE_FILTER_TRIANGLE);

        if (!ImportImage(normalMapStorage, outputNormalMap))
            return -1;
    }        
    
    return 0;
}

HK_NAMESPACE_END

using ApplicationClass = Hk::CoreApplication;

alignas(alignof(ApplicationClass)) static char AppData[sizeof(ApplicationClass)];

int main(int argc, char* argv[])
{
    using namespace Hk;

#if defined(HK_DEBUG) && defined(HK_COMPILER_MSVC)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

#ifdef HK_OS_WIN32
    ArgumentPack args;
#else
    ArgumentPack args(argc, argv);
#endif

    ApplicationClass* app = new (AppData) ApplicationClass(args);
    int exitCode = RunApplication();
    app->~ApplicationClass();
    return exitCode;
}
