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

// NOTE: Some code in this file is based on Dear ImGui

#include <World/Public/Resource/FontAtlas.h>
#include <Core/Public/Logger.h>
#include <Core/Public/IntrusiveLinkedListMacro.h>
#include <Core/Public/Base85.h>
#include <Core/Public/Compress.h>
#include <Core/Public/CriticalError.h>

// Padding between glyphs within texture in pixels. Defaults to 1. If your rendering method doesn't rely on bilinear filtering you may set this to 0.
static const int TexGlyphPadding = 1;

static const int TabSize = 4;

static const bool bTexNoPowerOfTwoHeight = false;

static const float DefaultFontSize = 13;

// Replacement character if a glyph isn't found
static const SWideChar FallbackChar = (SWideChar)'?';

static EGlyphRange GGlyphRange = GLYPH_RANGE_DEFAULT;

#ifndef STB_RECT_PACK_IMPLEMENTATION
#define STBRP_STATIC
#define STBRP_ASSERT(x)     AN_ASSERT(x)
#define STBRP_SORT          qsort
#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"
#endif

#ifndef STB_TRUETYPE_IMPLEMENTATION
#define STBTT_malloc(x,u)   ((void)(u), GZoneMemory.Alloc(x))
#define STBTT_free(x,u)     ((void)(u), GZoneMemory.Free(x))
#define STBTT_assert(x)     AN_ASSERT(x)
#define STBTT_fmod(x,y)     Math::FMod(x,y)
#define STBTT_sqrt(x)       StdSqrt(x)
#define STBTT_pow(x,y)      StdPow(x,y)
#define STBTT_fabs(x)       Math::Abs(x)
#define STBTT_ifloor(x)     (int)Math::Floor(x)
#define STBTT_iceil(x)      (int)Math::Ceil(x)
#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#endif

// ProggyClean.ttf
// Copyright (c) 2004, 2005 Tristan Grimmer
// MIT license (see License.txt in http://www.upperbounds.net/download/ProggyClean.ttf.zip)
// Download and more information at http://upperbounds.net
static const size_t ProggyClean_Size = 5961;
static const uint64_t ProggyClean_Data[5968] =
{
    0xc71c70593ddd0178, 0x900900b16667af75, 0xf009001e25e01620, 0x9209e5dc025c303e, 0x453448c48a2b48a2, 0x579b0f04902c8a27,
    0x561d1e4765229048, 0x14518aa569963a64, 0x6862a2562e28e547, 0x2a955d8e5852e525, 0xb64b628e28930a27, 0x56992e1f2a8e1f4a,
    0xe91610fa5d912568, 0x177bba7bba7bbbb9, 0xcceeec73bb02c8fe, 0x00202dfafd5dfabb, 0xbee7ba186c0f0568, 0x0025e8d9fe9e6bd5,
    0xe1f1c0fbbd9f0bb4, 0xd7f600b3c2c75653, 0x86ed1b9c0f7bb9f0, 0xc7c3ac5f847ce1e0, 0x02f617c6f8ea1c2e, 0xc32323bd67801d14,
    0xefc03f4395f47f07, 0xe689f047839f86bd, 0xe87df8bbf8bbe86d, 0xd47845fcf47c723d, 0x3b1d37afe1cfc7c3, 0x16fcbccff7f86079,
    0xfcf87c739cbfc140, 0xc12fefc6de0d7429, 0xfbe91f1f0c4fba27, 0xf127a3f8065edfc4, 0xe57a333c9d4e17b9, 0xfc5ee3f8073f13ea,
    0xa8ea723d3a9fef9d, 0x37cfe01f3c303a85, 0x4ff8fc6e7e0b8f83, 0xbb00bf8dd4e4fcdf, 0xa4fc3f0e6ff68039, 0x50047e57b3cefdbf,
    0x0ad8c61fbfcf6cf3, 0x086d5dfce3bff1b1, 0xe44ae0bc3782f29f, 0x37a0970420cff2c3, 0xcef7849821866e3a, 0x47c9742dfc137841,
    0x397e1f1175b6fac4, 0x9c6de135807fa2dc, 0x833d1b1eb63ee016, 0x00150bc357dd05af, 0xf21fff57b6242179, 0xefe47d82bc7c5402,
    0xf0bc87fd1ee97f07, 0x77e8b7ccbd6e40d3, 0x48009e89f7c43bc2, 0xc340aaf62608fdf0, 0xa3581c0063e06980, 0xbc11ae48271a6cf7,
    0x712aa7c77e24f01f, 0x17705fbe37863b5f, 0x8089eff63824e2c4, 0x397befbc914382c5, 0x5491398f869ca17f, 0xfe81d8e0a0cf85de,
    0x8c1cfe49039bee40, 0x58f33e0f1f1d5f77, 0xb2926f1ff16f87b0, 0xb531e3f3ffe09a8c, 0xe3475d2fa015fd8f, 0x07dfc06d5f0e8f9f,
    0xb69eb4eac17a0e68, 0xed57f67ded3bb23e, 0xb3b7671ace9d3b1f, 0xf0a870bd30b179df, 0x161fa6d1a68ea6ed, 0xdddf35ef8b47c52b,
    0xed2debf372f9ac7c, 0x24fa693d3cb6772d, 0xad1be4ef7933b26f, 0x60f6d9bd6f1f5aab, 0xe53dd94e6edbd7db, 0x68ef6e5f5325d4f2,
    0x5ed3bdda677f6d1f, 0xe97dfc744e3a70e9, 0x75f73b2fd2d5d2ae, 0x75e5d435d9bceb5e, 0xa3fa63bd3ebae95d, 0x3333c676b19d5fd3,
    0xde66b7e677b999de, 0x6637acf2eb38359a, 0xf6dfefd9d5fd9aef, 0x9aee7338e6376dc0, 0x3ee6cbb9cdce7973, 0x333de686f3fbf73b,
    0xbfcdcff3bdf99def, 0xd55bfe3b774aff32, 0x74f46fbbaffbb17d, 0xc16b73d73cf68cf6, 0x5e7fc56c2d4f05fe, 0xd5be1797c2dcf7f0,
    0xd4bde91ef677bd0b, 0xc7bea1efa9afad7b, 0x45bed170fbec3efb, 0x7e2d0f16b7bfe12f, 0xf9fc964725a4c97e, 0xfa5f5e97bb4b29d2,
    0x687b2d0f65c1b2e1, 0x5f9707f2d97cb5b9, 0xaed62bdd1593fe5d, 0x3cac0f9592e57e78, 0xb45aaeef95a5f2bf, 0xfd595fab45eacaea,
    0xeecddc37fd83ff41, 0x5e7942f35bccd737, 0xfdac0fb5adf2f1f9, 0x18de06a781fac0de, 0xceeb27f07a78345c, 0xdaa572beb2ab7bad,
    0xfd6abf5b5ead1d51, 0xbbc3767c369a1bf3, 0xa6c169bcbc6fd71b, 0x57e6ef79b7bd36a7, 0xd0c750fcf2db2cb7, 0xc5f5b83adf5e86a7,
    0xedb23db686dbcd6d, 0xdafedfefb7f7b6fa, 0x1dadfb78bf6ecbed, 0xd8ef3fe3b2bc76fb, 0x647cee767737c773, 0x63f57aecf6bbdbe7,
    0x450d82ac8f4ebe43, 0x36e03304a1530c98, 0xd5872c31610b0f98, 0x76c0b601b0758650, 0x707dc0f706bf09d8, 0x61c00e127c16fc3f,
    0x7c30f033849c34f8, 0x8be0f7e027c08f06, 0x572ee503698065f0, 0x456d04cf67e02cf0, 0xd5679e47c7c2f4f7, 0x501e226f1c3e1afd,
    0x0bc31b5585b175f4, 0x27e369e7228b1840, 0xf80e3baf67622fa2, 0xea7218fa37c84dda, 0x2dc8fd11cb95c1da, 0x00e2fc5e6af1dfc2,
    0x2fa3beed0ccc2a03, 0x73e4c5648a7bdc37, 0x724c77022c7b0128, 0xee378174176636c6, 0x0682fb008345a0d9, 0x038f1884285479a6,
    0xe3de340e3c70568d, 0x7e6d4eeacfae8460, 0x720b06659b188943, 0x0f718c4caf7928bf, 0x6b1f047818de2afb, 0x5bbe63d3a8d65cb1,
    0x278ba7a7886a8fa3, 0xd1ce57399736e892, 0x1a9bc34cc3d8f15d, 0xc41c1630705eaf04, 0xc036d088d4058dc8, 0x80fa24041f9e46d4,
    0xc03d47494e9131d2, 0x25d291fd59e30464, 0x11739e25b122eb91, 0x6ac1f4cf9080b5b1, 0x62f750702adc1e19, 0xf4a28c8223092675,
    0x41df693c08e0945f, 0xe9cba6325947a1cd, 0x16b5711c7211adf8, 0x1218122d0c091d18, 0xca6948b31a04167e, 0x0b2cf8e5ef096368,
    0x3550c7d05ef8aa22, 0x42f3ac20d4c326a2, 0x52df03b40ce02369, 0xef8db020ac0d1693, 0x3458e01ce093dc9b, 0xdb2128a0997cc3d4,
    0x21e4725f4cdeb45a, 0x437a4433a26019d2, 0x937245037a7907a6, 0xa31e88528f318668, 0x293c2514e3748c33, 0xdcb2147678ea470b,
    0x53cd6dbc8d58ac8f, 0xa9703e351e187057, 0x53283c8b3e45431f, 0x36901aff26f0074a, 0x91523971d097560a, 0x4d869a60085f5290,
    0x6246275bd55f72e4, 0xa43a84a40da10922, 0x94635a1dbc25adb6, 0x72979c46ab489dab, 0x9891880ca6851a80, 0x5964f095e50381a6,
    0xcdc2c4d8a1429225, 0x1cab7cc22c0317e8, 0xdf359b7a65ed2e07, 0x73b7a7acef81f2aa, 0x835d8d596d16b6b5, 0x5159237792389086,
    0xa502c8a5f465dd85, 0x38acae9a92cfaa1e, 0xb72d8937f84c3c61, 0x77f16a5cf1354f69, 0xdf328df1857f429b, 0x0962fa52188278e7,
    0x46a3e7d8b79cfd4d, 0xbf7beb626868cb9c, 0xb63e3258d2625901, 0x2f1bd19fc6cfeafa, 0xf6f8b190f9e44701, 0x36c82adf6981f190,
    0xd0686470092e700b, 0x70728a53fe698445, 0x3324ef5ae6ecb14f, 0x49e41d0b52d4b592, 0x74987cfc35e71b66, 0x43dc744e9f1cb924,
    0x5c71217b122e675f, 0xd464309dc8a7a698, 0xb86d2aa86c724df2, 0x79079a4451a641e3, 0xc3ba71c4c6903b89, 0xfaa2d355e4e3020a,
    0x8052558f34cf2c79, 0xfbe538d4bab035df, 0x137f1f5063d60cf4, 0xb0b879edf153dedf, 0x4bd5e54893cf3e91, 0x299b9666eb753d02,
    0xcb24f3167f9fd40d, 0xa5987e95e135f086, 0x0cbc99aa786893ed, 0x06348d6adea432df, 0x0e0f538c72883efa, 0x31a26bd689ed9842,
    0xce2263b213a8a3d0, 0x4e3c6094475059ef, 0xa911b8d93523347a, 0xd12128c0b178fe28, 0x5e199e192c86e7ea, 0x481f580811727315,
    0x8a58751ae2089631, 0xf4503510c950b0aa, 0x8d718acbe71975e2, 0x47d3c73cd8bde947, 0xaedc5ea6358ed191, 0x4bff814f25162f18,
    0x386fa75571956c3d, 0x5701f999700f988d, 0x24f353ec11ec2fc3, 0xfe215e796f181381, 0xc49d3ace49927727, 0x60b293b9bc53f806,
    0xdf7267fd61539d9f, 0xdc808b8d6290264b, 0x7b98dd36f12bb326, 0xa923ce90f4caed56, 0xc4d7426e192828f8, 0x97a8218d6c93e78b,
    0xa6d605bc7a19284b, 0x53d781a463097367, 0xb1460473c78dbe23, 0x95427e44ca2f098b, 0xaec4fbc4d64efa27, 0x47ff5ccb7397f692,
    0x79b370319fe1d658, 0x4c63a22f8248b98a, 0xa27f3855e7d17dd2, 0x8e764467eb723f62, 0x2e349a57072bb4e7, 0x8765431c474ea835,
    0x33079de835c93689, 0x9999b13bd011815b, 0x470918e7ba9af929, 0x8f413e24d334f51d, 0x8f38de2ceb115fa2, 0x96915f8c0cfaccf0,
    0xbccdd895f8c42466, 0x38ca52bde8a0a5d8, 0xb66cdd5b8a91384d, 0xa45e65772a53f2e6, 0x09ec558a8d5bc8cb, 0xb665462e64aee319,
    0xc634d2d9c9b7f294, 0x843fea7353d31bd2, 0xe55d9d278545a6f3, 0x097539ac3280e043, 0x09a5db670c654e25, 0xe8d23e8548d19801,
    0xb58a4234fa3eb234, 0x6e144a328cc9651b, 0x78300920eb1c6932, 0xa6788ac3c6011688, 0x9abd94a2bf2bb2e6, 0x489524b8c4d4cc15,
    0xe26bd2202022a53e, 0x3a29fa403c4e21ba, 0x999ca6494390e6f9, 0xe9273896fdd4b392, 0x769dfc928b24e4e9, 0xa9a78998fb13d5c6,
    0x50bcb9ad6631742e, 0x5e9c83756789f5ca, 0x15248ce8026b7044, 0x027b1a12b674ba35, 0xb953694fa3a2271a, 0x0698c1e3514b9dcf,
    0x6b83f128a7a00823, 0x1de84a3b7e0a0a69, 0xdae9f191ae9ac31a, 0x906a65744a2bbbe5, 0x4e0b903db11591eb, 0x843cd23fe9e5b082,
    0x28f50747a8879aa7, 0x839a002237a41eeb, 0x624833808f0f35bc, 0x53c080ef433b1191, 0xfdee407e39a77309, 0x0c8677d6c05b2e58,
    0xbfccaed402418215, 0x734df1d33374f70c, 0x20975f8a8e1ad232, 0xf145e79097551be4, 0xc6bdfd692039273c, 0x63d76c6c08f2a738,
    0xb45048d979449fb0, 0xe0e6e758f10ee963, 0x46ad9630abaf3c69, 0x6a9aa0acb50c89c2, 0x7d96853e458ccb4c, 0xe6cd971e7eaca230,
    0xf3651abc7d041aea, 0x5249af00ceb61a8a, 0x711fcce653b80ab9, 0xe373a410407dc567, 0x34799e414da06012, 0x55a12528dca93324,
    0xee410db70414b1ba, 0xaa2dbc4affe5039c, 0x42127a889ed10a7a, 0x509e5142e599374b, 0x6fca5219057a0da9, 0x6874e75c969a930a,
    0xe5a6cf25fca32ad9, 0xd4b8c6a4f4a929e4, 0xd7eb3201899967a2, 0x04d6f1553c7f7f52, 0x520af5f9069ef0e8, 0x1e517bdb9bd6ec69,
    0xe2968ef361a99452, 0xbf7c0aff88d71f78, 0x795301d746855d99, 0xe464dfa02ff0bd33, 0x7c5457975f20f8fb, 0x394aad32a794d322,
    0x8c62932bb0d01134, 0xb9c12b77e5483477, 0x9c3d5f25352d5e46, 0xd6a0a4809620dc1a, 0x4d9f8dfcc20dde02, 0xb964cb6b013d73ac,
    0x242bc88d5935611d, 0x2733486a711a64a5, 0xc4325fdd748325c8, 0xf6a644c92bb72f98, 0x4b192e7e427f3541, 0x379004989e8a5499,
    0x40ed662094b19c61, 0x50ed112cf95ada9f, 0xac66b496d6b241c7, 0xe4edad2b2310a3df, 0x25ee668d950c9b67, 0xc8597819b8153e0f,
    0x859b541ac12e32f5, 0x961024cc4ba35a9f, 0x5feb349d5e8dbe99, 0x8b25ba82ca966b91, 0xa4975115074f702e, 0x0b587d06e7eca53a,
    0xafd1debe72c1b9da, 0xa8c2a2b8af8c48a4, 0x0f9422ff5bc046af, 0x852826c440a07091, 0x12f8398eabd0e2c2, 0x9bbad54c22ca93e3,
    0x8204b90987af00f2, 0xd1131599cf2358a8, 0xc71ece98ecb868c4, 0xa1b234fe2cea93b2, 0x841f4cbc107d84ec, 0x3cfe020dba3c5e58,
    0xe24859881426fbf0, 0xe854de8c95df411b, 0x336bac1e795dc2bb, 0x1f4e5b5ebfc2e3ec, 0xfe9e56d9b0dec483, 0x6c44199b8924f5e6,
    0xa02fdc6d5d253cb4, 0xf969dcf99978f918, 0x66f444ffe788b9f3, 0x0b80f40c696620d6, 0x2c354a8bd5fcbce7, 0xedac19d12bad5cbc,
    0x343266ab6a095ac8, 0xa442b38c0d24f3f2, 0x9a838e8c89374845, 0x2173b9897f7564ac, 0xd2efea8cc37850a2, 0xecbcf8d597dfadce,
    0x1324f4d3c8df2c85, 0x9d3128fb52836889, 0x5c44e3f033b6be96, 0xddd59bfa33c08d56, 0x73a36855f50efcea, 0x55856ab30472e3fd,
    0x2a421564525250bc, 0x56cbc7da4f07d307, 0x663aa29dfe6dbd8b, 0xb02cbb9ab9318c50, 0xfb4f368c356eb215, 0xa052f6fc6b7d914c,
    0x31cae5b5caed8f29, 0x75118d978773b0a2, 0x5cde9e71735076fb, 0x4e4eb99284d6235c, 0x7a7550344ebd25bd, 0x4ac1faabb7147f21,
    0xc8dc344f68b28c35, 0x3e6924c2ad6b4f70, 0xac613355b30d9b8c, 0xd56877fbd90e34f7, 0xccac3e54403288f0, 0x330cc90be7f312e4,
    0xc2163fb33475a891, 0x5e9aad5335f132e3, 0xbf289acf5ac4efcb, 0xbced6ca7f570a47f, 0xac140fa9cb902d68, 0x98aa8bc0be748572,
    0x5d5ac748de4db511, 0x2479f64d8202a38b, 0xe627f15048082759, 0xf712e1fc83f89c7a, 0xba06d990b0f69073, 0x67590d09997822c7,
    0x979beb3246b86287, 0x12833815c4ad7324, 0x5cd2ff7e7f2dd5e6, 0x5476b1a3d6732c19, 0x24e659262ca2bef6, 0x4a4634a0699ecfdc,
    0x255603d7876d8f0b, 0x8747d9b83a387266, 0xfbd7912bdb94775b, 0xc2b6647e88b6ab90, 0x40bf813c276e4cfc, 0x928d409919b8a219,
    0x8af6b896671fc0e6, 0x92bf56e14a2a3794, 0x793dc9838758b2ee, 0xdbe8ee721840869a, 0xa48511b485466088, 0x79e077186afa2c4e,
    0xe5d2fe694b18c504, 0x4eab09ef1532ce8c, 0x304a495031889a88, 0x11ed284b5ad6cf38, 0xba2ac725949ecbc3, 0x44d6a63bf04bca99,
    0xedefc5a4babd762f, 0xde1dbc6515fbf16e, 0x0e46ff01629b333e, 0x98143edd645befc2, 0x493740b49c7114a8, 0x62f8d70af20b2f6a,
    0xa2910e3b9090b692, 0xe8988d7285d19fdf, 0xed0eca292f7d991d, 0xba69b8da5e86a643, 0x8f495cecfef8ce41, 0x5f2605f11d84ee64,
    0x5e767e4722d5d246, 0x7b8c5de49422ec85, 0x46b95aafe3c4c37b, 0x80f2cb27f10bf454, 0x1423fa88141d40fa, 0xe8e05ecd3c8bd0ea,
    0xa00b9557d7c082c1, 0x221306a6ca45ee5a, 0x0f666be9ccd8d41a, 0xecd3fa6771997b64, 0xd78a9b984626d929, 0x6b25cf50d7d415fe,
    0xa38f1cbbbd157fc3, 0x9700538fa6f265b0, 0x2044fc0d494e275d, 0x11a4f63243ada9d5, 0xd49035610b25ab3c, 0x0d38f94ae2981ee3,
    0x78d71027610d35c0, 0x04bc21a85e46aded, 0x3bd8755a16823e2d, 0x5e94305b900d4e19, 0xadc0f730c3377b3d, 0xa7b035756eade745,
    0x4b2286674798c3ad, 0x2e70fe5d8c878d96, 0x1ce93685f4e9e6a9, 0xd8c117517a3d6058, 0xd76ff712a16ed55d, 0xc75cedd47d9443d4,
    0x5a2ea7741b4b259b, 0x66662138d3d67a92, 0x6d0af04cfb670978, 0x60a053eb8b31c365, 0x338e7246e52bdbf4, 0xa78e7f90d1c02633,
    0x40d47297291b1f6f, 0xaf215f6fb0b6af1f, 0x13ab390ac23324e6, 0xbf215754223e1999, 0x9597079211995c32, 0x054eb9c320ccf026,
    0x6972815c2589d6d8, 0x9f53f4aa8c0104f7, 0xf8829a0c4e86579b, 0x6a877d499e1df525, 0x9f8249b1eea305b7, 0xce78454863563ad3,
    0x59e85bf8c2c7f232, 0xdda46b3d1f03b237, 0xbb50c6a6f5c0cf85, 0x7c5ebcdfe2947d32, 0xfb0d21a36c4216fc, 0x1c32bbc6e438da07,
    0xedc4184871b73898, 0x0e9ac47f5aadf1aa, 0xcb1aa5e01988ea52, 0xabc50db0c6c5e1e6, 0x72735b8a5576395d, 0xfe93990d01fceade,
    0xbd5c22b550495656, 0x863b07cd2f76b0a7, 0xcad34f11cfaf31f1, 0x24c3224d07993334, 0xb313115a5a294be3, 0x44f38d1822014664,
    0x51bc851053d3d6e7, 0x3bce34759da8296c, 0x66144d4b12578bd6, 0xbf793bcdd63dca0d, 0x7c9363b9469e1154, 0x29a06e2681f85d5c,
    0xc9124a30730a6d9c, 0x5902d207d246c7d6, 0xa034b3e6a34660b5, 0xac84485f5b87c1de, 0xc2b385b55a13cebc, 0x40eb5fea678e715e,
    0xafe10185c76bfbaf, 0xbf750bc2af7e4a00, 0x79afd1a4934eeda3, 0x9c5e76feb37b2364, 0x1c52d08bb8f51333, 0x41e006859bb5263e,
    0x392f6189851a0764, 0x2136693394c0e64e, 0x3587c8eaac75270f, 0x3a6e3e461bb23310, 0x9c28d4428dfcd33c, 0x779264f84db1d1f2,
    0x203e74a14c68372e, 0xebf603deda796067, 0xa4315e2fdd0bd7f5, 0x479ad21af22b1bfa, 0xeed0a18ad46a0d2c, 0x332ddda23fec554d,
    0x233199cedccc5bf3, 0x3bee323d806fa155, 0xada93dea82c19e39, 0x092e2f5d926d4dc6, 0x2ea93651a6440d90, 0x5cc8f119484a6a8c,
    0xad8e8e54e7d1d76f, 0xa46661c7a48b4b8b, 0x808a545b661bba88, 0x311d4acd251fbdc8, 0x67d69471f4986e73, 0x73778d73bb2752b2,
    0x7d02b2df189ebe39, 0x397267d235acb7b9, 0xb5c774c32edd3099, 0x1ac1b0c5a8a7c174, 0xc96591d2d7963225, 0xd68dce9f6c6ab04c,
    0x42caac45b8a62178, 0xc2a5fb52d8b5bd57, 0x6baa4dfa72f0719f, 0x27603a85e475dd7b, 0x3a46b06f4146a762, 0x77bf36cefc2946cf,
    0x71c0d260a87878bd, 0x51f0d42e390d89a1, 0xe39d72e17ae68de5, 0x7fac1b1365050142, 0x7a8a24aa20d1f9e3, 0xc14ee62c906ba490,
    0x384d635e3afb164f, 0x88d6f04cfaf16f15, 0xe22d77e4d9b53997, 0xb9af10e86981cbf7, 0x13ae4db33da03f89, 0x67a8da398687d4d7,
    0xd4d5b93d7c1b529a, 0x12362750f4398689, 0x56791639a274ba83, 0x654c43ae64933f55, 0xe7a8fe035b050949, 0xd712a96eba1e56bf,
    0x05e824c4958ca935, 0xc8bbf3ae3b1f948d, 0x509733d41dcde8f7, 0x46af98d8c902bcd7, 0x65a96557c91ab73d, 0x2c89c9506ae6d5f0,
    0x3043da2bb0bd1f88, 0xc702982df68f1e1b, 0x85fb10ef389b7da2, 0xa3ce0a68f0472bb0, 0xb66ba283bf03a8e8, 0xa28bc2985bed1b81,
    0x43bed195b76b1b7d, 0xdfb473609300a67c, 0x9c112d3901b79c14, 0x19c21fb867087ee1, 0x1df6891e00b0dbee, 0xe7087ee19c2133e2,
    0xc377316b7fc7e81a, 0x73dee85ff6061757, 0x3bdd13e1d199e8fa, 0xbdf4e47c7e3d1f4f, 0x67bd3e1b1ecf8723, 0x3e1e4fa7ddeebecf,
    0x7b13e191b1fb617c, 0xff1abb5fefe9acf6, 0xec37402e1fb003bb, 0x0c28c0ce1470d381, 0x8e3e3833bc7c09c3, 0x30f5f8bddef046ff,
    0xf83da3e30e50c59c, 0x127fbc6eccfe1f6f, 0x37bf1db0af8c3f9f, 0xfc53a7d77fd77c12, 0x5e0a103cc8280ce8, 0x2a1350150723642c,
    0xa8c9a12682d466a2, 0x476a2a6829a1b515, 0x85d44ea12a0750d3, 0x66859a266819a3a6, 0x43cd17341cd0dba3, 0x1a980bac307751f3,
    0xd0c9f34c16866845, 0xe63b7fcb053da98a, 0x4c6c789d051e074c, 0x8d9b839e266163c7, 0xb8f0f30b981cc447, 0x1f9885e74c1e86e8,
    0x852c09618b9b987d, 0x155cfcc0aca47865, 0x0eba632df4c5d450, 0x7ac2a8153b1e20c0, 0x2dd9cc26c236823c, 0x76f9c61b615b0430,
    0x8f82ed5a607701d8, 0x0ebf05dd64789dc1, 0xd3037e1bb5f1e377, 0x9c5ec38f894c3eec, 0x33c4df809f07ddc3, 0x5e7883c36fc00f7a,
    0x5a01683d461821ef, 0xa316845a1f517a88, 0x15a396865a296825, 0xe47ea356855a2568, 0x3401a2d68ca835a2, 0x68f5a2aa0aa1d688,
    0x0b68cda13688da03, 0x3c19fc2ae15b421a, 0x177865c0ef879f09, 0xf027f08be067829e, 0xb7845c39f875f017, 0x5ff81ff803f809e1,
    0x85df0d3c21fc12f8, 0xfc12f00bc37fc31f, 0x97f08bc25f803e02, 0xc04b0d708ff04ff0, 0x07305fe10704b801, 0xf06bc1bfc33fc06f,
    0x1f821c29f83df0af, 0xe3cc0df0eff0ebc0, 0xde037843f02be1f7, 0x02f82ff833f02384, 0x1c84b1a7c84b0a7c, 0x81df84961aff5cc3,
    0x85193960ce225853, 0x9658f384dc30f073, 0x70b3e177e067c02f, 0x1b628f073e14fe1d, 0x535be0b707bc18fa, 0x91c1e4e07a389ece,
    0xfa5dfbfbfbe7fd85, 0xd2d70fa6587d0d70, 0x8eb87d20e1f40387, 0x9870fa5587d0ac3e, 0x0f0fa01e1f4fdc3e, 0xd0ea21e1f42387d2,
    0x5f1a5d7c690fa7ed, 0x7c6975f1a5d7c697, 0xf1a5d7c6975f1a5d, 0xc6975f1a5d7c6975, 0x1a5d7c6975f1a5d7, 0x236391c1d350f75f,
    0x524fa7b38ece23a3, 0x15b2342b6468556f, 0xb646856c8d0ad91a, 0xc8d0ad91a15b2342, 0x1a15b2342b646856, 0x42b646856c8d0ad9,
    0x5b99a55b99a15b23, 0x7c6956f8d2adf1a5, 0x8d2adf1a55be34ab, 0xa55be34ab7c6956f, 0xab7c6956f8d2adf1, 0x6f8d2adf1a55be34,
    0x62f94801ffc010f5, 0x410d314b0f2b00f7,
};

// A work of art lies ahead! (. = white layer, X = black layer, others are blank)
// The white texels on the top left are the ones we'll use everywhere to render filled shapes.
const int FONT_ATLAS_DEFAULT_TEX_DATA_W_HALF = 108;
const int FONT_ATLAS_DEFAULT_TEX_DATA_H = 27;
const unsigned int FONT_ATLAS_DEFAULT_TEX_DATA_ID = 0x80000000;
static const char FONT_ATLAS_DEFAULT_TEX_DATA_PIXELS[FONT_ATLAS_DEFAULT_TEX_DATA_W_HALF * FONT_ATLAS_DEFAULT_TEX_DATA_H + 1] =
{
    "..-         -XXXXXXX-    X    -           X           -XXXXXXX          -          XXXXXXX-     XX          "
    "..-         -X.....X-   X.X   -          X.X          -X.....X          -          X.....X-    X..X         "
    "---         -XXX.XXX-  X...X  -         X...X         -X....X           -           X....X-    X..X         "
    "X           -  X.X  - X.....X -        X.....X        -X...X            -            X...X-    X..X         "
    "XX          -  X.X  -X.......X-       X.......X       -X..X.X           -           X.X..X-    X..X         "
    "X.X         -  X.X  -XXXX.XXXX-       XXXX.XXXX       -X.X X.X          -          X.X X.X-    X..XXX       "
    "X..X        -  X.X  -   X.X   -          X.X          -XX   X.X         -         X.X   XX-    X..X..XXX    "
    "X...X       -  X.X  -   X.X   -    XX    X.X    XX    -      X.X        -        X.X      -    X..X..X..XX  "
    "X....X      -  X.X  -   X.X   -   X.X    X.X    X.X   -       X.X       -       X.X       -    X..X..X..X.X "
    "X.....X     -  X.X  -   X.X   -  X..X    X.X    X..X  -        X.X      -      X.X        -XXX X..X..X..X..X"
    "X......X    -  X.X  -   X.X   - X...XXXXXX.XXXXXX...X -         X.X   XX-XX   X.X         -X..XX........X..X"
    "X.......X   -  X.X  -   X.X   -X.....................X-          X.X X.X-X.X X.X          -X...X...........X"
    "X........X  -  X.X  -   X.X   - X...XXXXXX.XXXXXX...X -           X.X..X-X..X.X           - X..............X"
    "X.........X -XXX.XXX-   X.X   -  X..X    X.X    X..X  -            X...X-X...X            -  X.............X"
    "X..........X-X.....X-   X.X   -   X.X    X.X    X.X   -           X....X-X....X           -  X.............X"
    "X......XXXXX-XXXXXXX-   X.X   -    XX    X.X    XX    -          X.....X-X.....X          -   X............X"
    "X...X..X    ---------   X.X   -          X.X          -          XXXXXXX-XXXXXXX          -   X...........X "
    "X..X X..X   -       -XXXX.XXXX-       XXXX.XXXX       -------------------------------------    X..........X "
    "X.X  X..X   -       -X.......X-       X.......X       -    XX           XX    -           -    X..........X "
    "XX    X..X  -       - X.....X -        X.....X        -   X.X           X.X   -           -     X........X  "
    "      X..X          -  X...X  -         X...X         -  X..X           X..X  -           -     X........X  "
    "       XX           -   X.X   -          X.X          - X...XXXXXXXXXXXXX...X -           -     XXXXXXXXXX  "
    "------------        -    X    -           X           -X.....................X-           ------------------"
    "                    ----------------------------------- X...XXXXXXXXXXXXX...X -                             "
    "                                                      -  X..X           X..X  -                             "
    "                                                      -   X.X           X.X   -                             "
    "                                                      -    XX           XX    -                             "
};

static const Float2 CursorTexData[][3] =
{
    // Pos ........ Size ......... Offset ......
    { Float2( 0,3 ),  Float2( 12,19 ), Float2( 0, 0 ) },  // DRAW_CURSOR_ARROW
    { Float2( 13,0 ), Float2( 7,16 ),  Float2( 1, 8 ) },  // DRAW_CURSOR_TEXT_INPUT
    { Float2( 31,0 ), Float2( 23,23 ), Float2( 11,11 ) }, // DRAW_CURSOR_RESIZE_ALL
    { Float2( 21,0 ), Float2( 9,23 ),  Float2( 4,11 ) },  // DRAW_CURSOR_RESIZE_NS
    { Float2( 55,18 ),Float2( 23, 9 ), Float2( 11, 4 ) }, // DRAW_CURSOR_RESIZE_EW
    { Float2( 73,0 ), Float2( 17,17 ), Float2( 8, 8 ) },  // DRAW_CURSOR_RESIZE_NESW
    { Float2( 55,0 ), Float2( 17,17 ), Float2( 8, 8 ) },  // DRAW_CURSOR_RESIZE_NWSE
    { Float2( 91,0 ), Float2( 17,22 ), Float2( 5, 0 ) },  // DRAW_CURSOR_RESIZE_HAND
};

static const unsigned short * GetGlyphRangesDefault() {
    static const SWideChar ranges[] =
    {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0,
    };
    return &ranges[0];
}

static const unsigned short * GetGlyphRangesKorean() {
    static const SWideChar ranges[] =
    {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x3131, 0x3163, // Korean alphabets
        0xAC00, 0xD79D, // Korean characters
        0,
    };
    return &ranges[0];
}

static void UnpackAccumulativeOffsetsIntoRanges( int base_codepoint, const short* accumulative_offsets, int accumulative_offsets_count, SWideChar* out_ranges )
{
    for ( int n = 0; n < accumulative_offsets_count; n++, out_ranges += 2 )
    {
        out_ranges[0] = out_ranges[1] = (SWideChar)(base_codepoint + accumulative_offsets[n]);
        base_codepoint += accumulative_offsets[n];
    }
    out_ranges[0] = 0;
}

static const unsigned short * GetGlyphRangesJapanese() {
    // 1946 common ideograms code points for Japanese
    // Sourced from http://theinstructionlimit.com/common-kanji-character-ranges-for-xna-spritefont-rendering
    // FIXME: Source a list of the revised 2136 Joyo Kanji list from 2010 and rebuild this.
    static const short accumulative_offsets_from_0x4E00[] =
    {
        0,1,2,4,1,1,1,1,2,1,6,2,2,1,8,5,7,11,1,2,10,10,8,2,4,20,2,11,8,2,1,2,1,6,2,1,7,5,3,7,1,1,13,7,9,1,4,6,1,2,1,10,1,1,9,2,2,4,5,6,14,1,1,9,3,18,
        5,4,2,2,10,7,1,1,1,3,2,4,3,23,2,10,12,2,14,2,4,13,1,6,10,3,1,7,13,6,4,13,5,2,3,17,2,2,5,7,6,4,1,7,14,16,6,13,9,15,1,1,7,16,4,7,1,19,9,2,7,15,
        2,6,5,13,25,4,14,13,11,25,1,1,1,2,1,2,2,3,10,11,3,3,1,1,4,4,2,1,4,9,1,4,3,5,5,2,7,12,11,15,7,16,4,5,16,2,1,1,6,3,3,1,1,2,7,6,6,7,1,4,7,6,1,1,
        2,1,12,3,3,9,5,8,1,11,1,2,3,18,20,4,1,3,6,1,7,3,5,5,7,2,2,12,3,1,4,2,3,2,3,11,8,7,4,17,1,9,25,1,1,4,2,2,4,1,2,7,1,1,1,3,1,2,6,16,1,2,1,1,3,12,
        20,2,5,20,8,7,6,2,1,1,1,1,6,2,1,2,10,1,1,6,1,3,1,2,1,4,1,12,4,1,3,1,1,1,1,1,10,4,7,5,13,1,15,1,1,30,11,9,1,15,38,14,1,32,17,20,1,9,31,2,21,9,
        4,49,22,2,1,13,1,11,45,35,43,55,12,19,83,1,3,2,3,13,2,1,7,3,18,3,13,8,1,8,18,5,3,7,25,24,9,24,40,3,17,24,2,1,6,2,3,16,15,6,7,3,12,1,9,7,3,3,
        3,15,21,5,16,4,5,12,11,11,3,6,3,2,31,3,2,1,1,23,6,6,1,4,2,6,5,2,1,1,3,3,22,2,6,2,3,17,3,2,4,5,1,9,5,1,1,6,15,12,3,17,2,14,2,8,1,23,16,4,2,23,
        8,15,23,20,12,25,19,47,11,21,65,46,4,3,1,5,6,1,2,5,26,2,1,1,3,11,1,1,1,2,1,2,3,1,1,10,2,3,1,1,1,3,6,3,2,2,6,6,9,2,2,2,6,2,5,10,2,4,1,2,1,2,2,
        3,1,1,3,1,2,9,23,9,2,1,1,1,1,5,3,2,1,10,9,6,1,10,2,31,25,3,7,5,40,1,15,6,17,7,27,180,1,3,2,2,1,1,1,6,3,10,7,1,3,6,17,8,6,2,2,1,3,5,5,8,16,14,
        15,1,1,4,1,2,1,1,1,3,2,7,5,6,2,5,10,1,4,2,9,1,1,11,6,1,44,1,3,7,9,5,1,3,1,1,10,7,1,10,4,2,7,21,15,7,2,5,1,8,3,4,1,3,1,6,1,4,2,1,4,10,8,1,4,5,
        1,5,10,2,7,1,10,1,1,3,4,11,10,29,4,7,3,5,2,3,33,5,2,19,3,1,4,2,6,31,11,1,3,3,3,1,8,10,9,12,11,12,8,3,14,8,6,11,1,4,41,3,1,2,7,13,1,5,6,2,6,12,
        12,22,5,9,4,8,9,9,34,6,24,1,1,20,9,9,3,4,1,7,2,2,2,6,2,28,5,3,6,1,4,6,7,4,2,1,4,2,13,6,4,4,3,1,8,8,3,2,1,5,1,2,2,3,1,11,11,7,3,6,10,8,6,16,16,
        22,7,12,6,21,5,4,6,6,3,6,1,3,2,1,2,8,29,1,10,1,6,13,6,6,19,31,1,13,4,4,22,17,26,33,10,4,15,12,25,6,67,10,2,3,1,6,10,2,6,2,9,1,9,4,4,1,2,16,2,
        5,9,2,3,8,1,8,3,9,4,8,6,4,8,11,3,2,1,1,3,26,1,7,5,1,11,1,5,3,5,2,13,6,39,5,1,5,2,11,6,10,5,1,15,5,3,6,19,21,22,2,4,1,6,1,8,1,4,8,2,4,2,2,9,2,
        1,1,1,4,3,6,3,12,7,1,14,2,4,10,2,13,1,17,7,3,2,1,3,2,13,7,14,12,3,1,29,2,8,9,15,14,9,14,1,3,1,6,5,9,11,3,38,43,20,7,7,8,5,15,12,19,15,81,8,7,
        1,5,73,13,37,28,8,8,1,15,18,20,165,28,1,6,11,8,4,14,7,15,1,3,3,6,4,1,7,14,1,1,11,30,1,5,1,4,14,1,4,2,7,52,2,6,29,3,1,9,1,21,3,5,1,26,3,11,14,
        11,1,17,5,1,2,1,3,2,8,1,2,9,12,1,1,2,3,8,3,24,12,7,7,5,17,3,3,3,1,23,10,4,4,6,3,1,16,17,22,3,10,21,16,16,6,4,10,2,1,1,2,8,8,6,5,3,3,3,39,25,
        15,1,1,16,6,7,25,15,6,6,12,1,22,13,1,4,9,5,12,2,9,1,12,28,8,3,5,10,22,60,1,2,40,4,61,63,4,1,13,12,1,4,31,12,1,14,89,5,16,6,29,14,2,5,49,18,18,
        5,29,33,47,1,17,1,19,12,2,9,7,39,12,3,7,12,39,3,1,46,4,12,3,8,9,5,31,15,18,3,2,2,66,19,13,17,5,3,46,124,13,57,34,2,5,4,5,8,1,1,1,4,3,1,17,5,
        3,5,3,1,8,5,6,3,27,3,26,7,12,7,2,17,3,7,18,78,16,4,36,1,2,1,6,2,1,39,17,7,4,13,4,4,4,1,10,4,2,4,6,3,10,1,19,1,26,2,4,33,2,73,47,7,3,8,2,4,15,
        18,1,29,2,41,14,1,21,16,41,7,39,25,13,44,2,2,10,1,13,7,1,7,3,5,20,4,8,2,49,1,10,6,1,6,7,10,7,11,16,3,12,20,4,10,3,1,2,11,2,28,9,2,4,7,2,15,1,
        27,1,28,17,4,5,10,7,3,24,10,11,6,26,3,2,7,2,2,49,16,10,16,15,4,5,27,61,30,14,38,22,2,7,5,1,3,12,23,24,17,17,3,3,2,4,1,6,2,7,5,1,1,5,1,1,9,4,
        1,3,6,1,8,2,8,4,14,3,5,11,4,1,3,32,1,19,4,1,13,11,5,2,1,8,6,8,1,6,5,13,3,23,11,5,3,16,3,9,10,1,24,3,198,52,4,2,2,5,14,5,4,22,5,20,4,11,6,41,
        1,5,2,2,11,5,2,28,35,8,22,3,18,3,10,7,5,3,4,1,5,3,8,9,3,6,2,16,22,4,5,5,3,3,18,23,2,6,23,5,27,8,1,33,2,12,43,16,5,2,3,6,1,20,4,2,9,7,1,11,2,
        10,3,14,31,9,3,25,18,20,2,5,5,26,14,1,11,17,12,40,19,9,6,31,83,2,7,9,19,78,12,14,21,76,12,113,79,34,4,1,1,61,18,85,10,2,2,13,31,11,50,6,33,159,
        179,6,6,7,4,4,2,4,2,5,8,7,20,32,22,1,3,10,6,7,28,5,10,9,2,77,19,13,2,5,1,4,4,7,4,13,3,9,31,17,3,26,2,6,6,5,4,1,7,11,3,4,2,1,6,2,20,4,1,9,2,6,
        3,7,1,1,1,20,2,3,1,6,2,3,6,2,4,8,1,5,13,8,4,11,23,1,10,6,2,1,3,21,2,2,4,24,31,4,10,10,2,5,192,15,4,16,7,9,51,1,2,1,1,5,1,1,2,1,3,5,3,1,3,4,1,
        3,1,3,3,9,8,1,2,2,2,4,4,18,12,92,2,10,4,3,14,5,25,16,42,4,14,4,2,21,5,126,30,31,2,1,5,13,3,22,5,6,6,20,12,1,14,12,87,3,19,1,8,2,9,9,3,3,23,2,
        3,7,6,3,1,2,3,9,1,3,1,6,3,2,1,3,11,3,1,6,10,3,2,3,1,2,1,5,1,1,11,3,6,4,1,7,2,1,2,5,5,34,4,14,18,4,19,7,5,8,2,6,79,1,5,2,14,8,2,9,2,1,36,28,16,
        4,1,1,1,2,12,6,42,39,16,23,7,15,15,3,2,12,7,21,64,6,9,28,8,12,3,3,41,59,24,51,55,57,294,9,9,2,6,2,15,1,2,13,38,90,9,9,9,3,11,7,1,1,1,5,6,3,2,
        1,2,2,3,8,1,4,4,1,5,7,1,4,3,20,4,9,1,1,1,5,5,17,1,5,2,6,2,4,1,4,5,7,3,18,11,11,32,7,5,4,7,11,127,8,4,3,3,1,10,1,1,6,21,14,1,16,1,7,1,3,6,9,65,
        51,4,3,13,3,10,1,1,12,9,21,110,3,19,24,1,1,10,62,4,1,29,42,78,28,20,18,82,6,3,15,6,84,58,253,15,155,264,15,21,9,14,7,58,40,39,
    };
    static SWideChar base_ranges[] = // not zero-terminated
    {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x3000, 0x30FF, // CJK Symbols and Punctuations, Hiragana, Katakana
        0x31F0, 0x31FF, // Katakana Phonetic Extensions
        0xFF00, 0xFFEF  // Half-width characters
    };
    static SWideChar full_ranges[AN_ARRAY_SIZE( base_ranges ) + AN_ARRAY_SIZE( accumulative_offsets_from_0x4E00 )*2 + 1] = { 0 };
    if ( !full_ranges[0] )
    {
        memcpy( full_ranges, base_ranges, sizeof( base_ranges ) );
        UnpackAccumulativeOffsetsIntoRanges( 0x4E00, accumulative_offsets_from_0x4E00, AN_ARRAY_SIZE( accumulative_offsets_from_0x4E00 ), full_ranges + AN_ARRAY_SIZE( base_ranges ) );
    }
    return &full_ranges[0];
}

static const unsigned short * GetGlyphRangesChineseFull() {
    static const SWideChar ranges[] =
    {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x2000, 0x206F, // General Punctuation
        0x3000, 0x30FF, // CJK Symbols and Punctuations, Hiragana, Katakana
        0x31F0, 0x31FF, // Katakana Phonetic Extensions
        0xFF00, 0xFFEF, // Half-width characters
        0x4e00, 0x9FAF, // CJK Ideograms
        0,
    };
    return &ranges[0];
}

static const unsigned short * GetGlyphRangesChineseSimplifiedCommon() {
    // Store 2500 regularly used characters for Simplified Chinese.
    // Sourced from https://zh.wiktionary.org/wiki/%E9%99%84%E5%BD%95:%E7%8E%B0%E4%BB%A3%E6%B1%89%E8%AF%AD%E5%B8%B8%E7%94%A8%E5%AD%97%E8%A1%A8
    // This table covers 97.97% of all characters used during the month in July, 1987.
    static const short accumulative_offsets_from_0x4E00[] =
    {
        0,1,2,4,1,1,1,1,2,1,3,2,1,2,2,1,1,1,1,1,5,2,1,2,3,3,3,2,2,4,1,1,1,2,1,5,2,3,1,2,1,2,1,1,2,1,1,2,2,1,4,1,1,1,1,5,10,1,2,19,2,1,2,1,2,1,2,1,2,
        1,5,1,6,3,2,1,2,2,1,1,1,4,8,5,1,1,4,1,1,3,1,2,1,5,1,2,1,1,1,10,1,1,5,2,4,6,1,4,2,2,2,12,2,1,1,6,1,1,1,4,1,1,4,6,5,1,4,2,2,4,10,7,1,1,4,2,4,
        2,1,4,3,6,10,12,5,7,2,14,2,9,1,1,6,7,10,4,7,13,1,5,4,8,4,1,1,2,28,5,6,1,1,5,2,5,20,2,2,9,8,11,2,9,17,1,8,6,8,27,4,6,9,20,11,27,6,68,2,2,1,1,
        1,2,1,2,2,7,6,11,3,3,1,1,3,1,2,1,1,1,1,1,3,1,1,8,3,4,1,5,7,2,1,4,4,8,4,2,1,2,1,1,4,5,6,3,6,2,12,3,1,3,9,2,4,3,4,1,5,3,3,1,3,7,1,5,1,1,1,1,2,
        3,4,5,2,3,2,6,1,1,2,1,7,1,7,3,4,5,15,2,2,1,5,3,22,19,2,1,1,1,1,2,5,1,1,1,6,1,1,12,8,2,9,18,22,4,1,1,5,1,16,1,2,7,10,15,1,1,6,2,4,1,2,4,1,6,
        1,1,3,2,4,1,6,4,5,1,2,1,1,2,1,10,3,1,3,2,1,9,3,2,5,7,2,19,4,3,6,1,1,1,1,1,4,3,2,1,1,1,2,5,3,1,1,1,2,2,1,1,2,1,1,2,1,3,1,1,1,3,7,1,4,1,1,2,1,
        1,2,1,2,4,4,3,8,1,1,1,2,1,3,5,1,3,1,3,4,6,2,2,14,4,6,6,11,9,1,15,3,1,28,5,2,5,5,3,1,3,4,5,4,6,14,3,2,3,5,21,2,7,20,10,1,2,19,2,4,28,28,2,3,
        2,1,14,4,1,26,28,42,12,40,3,52,79,5,14,17,3,2,2,11,3,4,6,3,1,8,2,23,4,5,8,10,4,2,7,3,5,1,1,6,3,1,2,2,2,5,28,1,1,7,7,20,5,3,29,3,17,26,1,8,4,
        27,3,6,11,23,5,3,4,6,13,24,16,6,5,10,25,35,7,3,2,3,3,14,3,6,2,6,1,4,2,3,8,2,1,1,3,3,3,4,1,1,13,2,2,4,5,2,1,14,14,1,2,2,1,4,5,2,3,1,14,3,12,
        3,17,2,16,5,1,2,1,8,9,3,19,4,2,2,4,17,25,21,20,28,75,1,10,29,103,4,1,2,1,1,4,2,4,1,2,3,24,2,2,2,1,1,2,1,3,8,1,1,1,2,1,1,3,1,1,1,6,1,5,3,1,1,
        1,3,4,1,1,5,2,1,5,6,13,9,16,1,1,1,1,3,2,3,2,4,5,2,5,2,2,3,7,13,7,2,2,1,1,1,1,2,3,3,2,1,6,4,9,2,1,14,2,14,2,1,18,3,4,14,4,11,41,15,23,15,23,
        176,1,3,4,1,1,1,1,5,3,1,2,3,7,3,1,1,2,1,2,4,4,6,2,4,1,9,7,1,10,5,8,16,29,1,1,2,2,3,1,3,5,2,4,5,4,1,1,2,2,3,3,7,1,6,10,1,17,1,44,4,6,2,1,1,6,
        5,4,2,10,1,6,9,2,8,1,24,1,2,13,7,8,8,2,1,4,1,3,1,3,3,5,2,5,10,9,4,9,12,2,1,6,1,10,1,1,7,7,4,10,8,3,1,13,4,3,1,6,1,3,5,2,1,2,17,16,5,2,16,6,
        1,4,2,1,3,3,6,8,5,11,11,1,3,3,2,4,6,10,9,5,7,4,7,4,7,1,1,4,2,1,3,6,8,7,1,6,11,5,5,3,24,9,4,2,7,13,5,1,8,82,16,61,1,1,1,4,2,2,16,10,3,8,1,1,
        6,4,2,1,3,1,1,1,4,3,8,4,2,2,1,1,1,1,1,6,3,5,1,1,4,6,9,2,1,1,1,2,1,7,2,1,6,1,5,4,4,3,1,8,1,3,3,1,3,2,2,2,2,3,1,6,1,2,1,2,1,3,7,1,8,2,1,2,1,5,
        2,5,3,5,10,1,2,1,1,3,2,5,11,3,9,3,5,1,1,5,9,1,2,1,5,7,9,9,8,1,3,3,3,6,8,2,3,2,1,1,32,6,1,2,15,9,3,7,13,1,3,10,13,2,14,1,13,10,2,1,3,10,4,15,
        2,15,15,10,1,3,9,6,9,32,25,26,47,7,3,2,3,1,6,3,4,3,2,8,5,4,1,9,4,2,2,19,10,6,2,3,8,1,2,2,4,2,1,9,4,4,4,6,4,8,9,2,3,1,1,1,1,3,5,5,1,3,8,4,6,
        2,1,4,12,1,5,3,7,13,2,5,8,1,6,1,2,5,14,6,1,5,2,4,8,15,5,1,23,6,62,2,10,1,1,8,1,2,2,10,4,2,2,9,2,1,1,3,2,3,1,5,3,3,2,1,3,8,1,1,1,11,3,1,1,4,
        3,7,1,14,1,2,3,12,5,2,5,1,6,7,5,7,14,11,1,3,1,8,9,12,2,1,11,8,4,4,2,6,10,9,13,1,1,3,1,5,1,3,2,4,4,1,18,2,3,14,11,4,29,4,2,7,1,3,13,9,2,2,5,
        3,5,20,7,16,8,5,72,34,6,4,22,12,12,28,45,36,9,7,39,9,191,1,1,1,4,11,8,4,9,2,3,22,1,1,1,1,4,17,1,7,7,1,11,31,10,2,4,8,2,3,2,1,4,2,16,4,32,2,
        3,19,13,4,9,1,5,2,14,8,1,1,3,6,19,6,5,1,16,6,2,10,8,5,1,2,3,1,5,5,1,11,6,6,1,3,3,2,6,3,8,1,1,4,10,7,5,7,7,5,8,9,2,1,3,4,1,1,3,1,3,3,2,6,16,
        1,4,6,3,1,10,6,1,3,15,2,9,2,10,25,13,9,16,6,2,2,10,11,4,3,9,1,2,6,6,5,4,30,40,1,10,7,12,14,33,6,3,6,7,3,1,3,1,11,14,4,9,5,12,11,49,18,51,31,
        140,31,2,2,1,5,1,8,1,10,1,4,4,3,24,1,10,1,3,6,6,16,3,4,5,2,1,4,2,57,10,6,22,2,22,3,7,22,6,10,11,36,18,16,33,36,2,5,5,1,1,1,4,10,1,4,13,2,7,
        5,2,9,3,4,1,7,43,3,7,3,9,14,7,9,1,11,1,1,3,7,4,18,13,1,14,1,3,6,10,73,2,2,30,6,1,11,18,19,13,22,3,46,42,37,89,7,3,16,34,2,2,3,9,1,7,1,1,1,2,
        2,4,10,7,3,10,3,9,5,28,9,2,6,13,7,3,1,3,10,2,7,2,11,3,6,21,54,85,2,1,4,2,2,1,39,3,21,2,2,5,1,1,1,4,1,1,3,4,15,1,3,2,4,4,2,3,8,2,20,1,8,7,13,
        4,1,26,6,2,9,34,4,21,52,10,4,4,1,5,12,2,11,1,7,2,30,12,44,2,30,1,1,3,6,16,9,17,39,82,2,2,24,7,1,7,3,16,9,14,44,2,1,2,1,2,3,5,2,4,1,6,7,5,3,
        2,6,1,11,5,11,2,1,18,19,8,1,3,24,29,2,1,3,5,2,2,1,13,6,5,1,46,11,3,5,1,1,5,8,2,10,6,12,6,3,7,11,2,4,16,13,2,5,1,1,2,2,5,2,28,5,2,23,10,8,4,
        4,22,39,95,38,8,14,9,5,1,13,5,4,3,13,12,11,1,9,1,27,37,2,5,4,4,63,211,95,2,2,2,1,3,5,2,1,1,2,2,1,1,1,3,2,4,1,2,1,1,5,2,2,1,1,2,3,1,3,1,1,1,
        3,1,4,2,1,3,6,1,1,3,7,15,5,3,2,5,3,9,11,4,2,22,1,6,3,8,7,1,4,28,4,16,3,3,25,4,4,27,27,1,4,1,2,2,7,1,3,5,2,28,8,2,14,1,8,6,16,25,3,3,3,14,3,
        3,1,1,2,1,4,6,3,8,4,1,1,1,2,3,6,10,6,2,3,18,3,2,5,5,4,3,1,5,2,5,4,23,7,6,12,6,4,17,11,9,5,1,1,10,5,12,1,1,11,26,33,7,3,6,1,17,7,1,5,12,1,11,
        2,4,1,8,14,17,23,1,2,1,7,8,16,11,9,6,5,2,6,4,16,2,8,14,1,11,8,9,1,1,1,9,25,4,11,19,7,2,15,2,12,8,52,7,5,19,2,16,4,36,8,1,16,8,24,26,4,6,2,9,
        5,4,36,3,28,12,25,15,37,27,17,12,59,38,5,32,127,1,2,9,17,14,4,1,2,1,1,8,11,50,4,14,2,19,16,4,17,5,4,5,26,12,45,2,23,45,104,30,12,8,3,10,2,2,
        3,3,1,4,20,7,2,9,6,15,2,20,1,3,16,4,11,15,6,134,2,5,59,1,2,2,2,1,9,17,3,26,137,10,211,59,1,2,4,1,4,1,1,1,2,6,2,3,1,1,2,3,2,3,1,3,4,4,2,3,3,
        1,4,3,1,7,2,2,3,1,2,1,3,3,3,2,2,3,2,1,3,14,6,1,3,2,9,6,15,27,9,34,145,1,1,2,1,1,1,1,2,1,1,1,1,2,2,2,3,1,2,1,1,1,2,3,5,8,3,5,2,4,1,3,2,2,2,12,
        4,1,1,1,10,4,5,1,20,4,16,1,15,9,5,12,2,9,2,5,4,2,26,19,7,1,26,4,30,12,15,42,1,6,8,172,1,1,4,2,1,1,11,2,2,4,2,1,2,1,10,8,1,2,1,4,5,1,2,5,1,8,
        4,1,3,4,2,1,6,2,1,3,4,1,2,1,1,1,1,12,5,7,2,4,3,1,1,1,3,3,6,1,2,2,3,3,3,2,1,2,12,14,11,6,6,4,12,2,8,1,7,10,1,35,7,4,13,15,4,3,23,21,28,52,5,
        26,5,6,1,7,10,2,7,53,3,2,1,1,1,2,163,532,1,10,11,1,3,3,4,8,2,8,6,2,2,23,22,4,2,2,4,2,1,3,1,3,3,5,9,8,2,1,2,8,1,10,2,12,21,20,15,105,2,3,1,1,
        3,2,3,1,1,2,5,1,4,15,11,19,1,1,1,1,5,4,5,1,1,2,5,3,5,12,1,2,5,1,11,1,1,15,9,1,4,5,3,26,8,2,1,3,1,1,15,19,2,12,1,2,5,2,7,2,19,2,20,6,26,7,5,
        2,2,7,34,21,13,70,2,128,1,1,2,1,1,2,1,1,3,2,2,2,15,1,4,1,3,4,42,10,6,1,49,85,8,1,2,1,1,4,4,2,3,6,1,5,7,4,3,211,4,1,2,1,2,5,1,2,4,2,2,6,5,6,
        10,3,4,48,100,6,2,16,296,5,27,387,2,2,3,7,16,8,5,38,15,39,21,9,10,3,7,59,13,27,21,47,5,21,6
    };
    static SWideChar base_ranges[] = // not zero-terminated
    {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x2000, 0x206F, // General Punctuation
        0x3000, 0x30FF, // CJK Symbols and Punctuations, Hiragana, Katakana
        0x31F0, 0x31FF, // Katakana Phonetic Extensions
        0xFF00, 0xFFEF  // Half-width characters
    };
    static SWideChar full_ranges[AN_ARRAY_SIZE( base_ranges ) + AN_ARRAY_SIZE( accumulative_offsets_from_0x4E00 ) * 2 + 1] = { 0 };
    if ( !full_ranges[0] )
    {
        memcpy( full_ranges, base_ranges, sizeof( base_ranges ) );
        UnpackAccumulativeOffsetsIntoRanges( 0x4E00, accumulative_offsets_from_0x4E00, AN_ARRAY_SIZE( accumulative_offsets_from_0x4E00 ), full_ranges + AN_ARRAY_SIZE( base_ranges ) );
    }
    return &full_ranges[0];
}

static const unsigned short * GetGlyphRangesCyrillic() {
    static const SWideChar ranges[] =
    {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x0400, 0x052F, // Cyrillic + Cyrillic Supplement
        0x2DE0, 0x2DFF, // Cyrillic Extended-A
        0xA640, 0xA69F, // Cyrillic Extended-B
        0,
    };
    return &ranges[0];
}

static const unsigned short * GetGlyphRangesThai() {
    static const SWideChar ranges[] =
    {
        0x0020, 0x00FF, // Basic Latin
        0x2010, 0x205E, // Punctuations
        0x0E00, 0x0E7F, // Thai
        0,
    };
    return &ranges[0];
}

static const unsigned short * GetGlyphRangesVietnamese() {
    static const SWideChar ranges[] =
    {
        0x0020, 0x00FF, // Basic Latin
        0x0102, 0x0103,
        0x0110, 0x0111,
        0x0128, 0x0129,
        0x0168, 0x0169,
        0x01A0, 0x01A1,
        0x01AF, 0x01B0,
        0x1EA0, 0x1EF9,
        0,
    };
    return &ranges[0];
}

// 2 value per range, values are inclusive, zero-terminated list
static const unsigned short * GetGlyphRange( EGlyphRange _GlyphRange ) {
    switch ( _GlyphRange ) {
    case GLYPH_RANGE_KOREAN:
        return GetGlyphRangesKorean();
    case GLYPH_RANGE_JAPANESE:
        return GetGlyphRangesJapanese();
    case GLYPH_RANGE_CHINESE_FULL:
        return GetGlyphRangesChineseFull();
    case GLYPH_RANGE_CHINESE_SIMPLIFIED_COMMON:
        return GetGlyphRangesChineseSimplifiedCommon();
    case GLYPH_RANGE_CYRILLIC:
        return GetGlyphRangesCyrillic();
    case GLYPH_RANGE_THAI:
        return GetGlyphRangesThai();
    case GLYPH_RANGE_VIETNAMESE:
        return GetGlyphRangesVietnamese();
    case GLYPH_RANGE_DEFAULT:
    default:
        break;
    }
    return GetGlyphRangesDefault();
}

AN_CLASS_META( AFont )

AFont::AFont() {

}

AFont::~AFont() {
    Purge();
}

void AFont::InitializeFromMemoryTTF( const void * _SysMem, size_t _SizeInBytes, SFontCreateInfo const * _CreateInfo ) {
    Purge();

    SFontCreateInfo defaultCreateInfo;

    if ( !_CreateInfo ) {
        _CreateInfo = &defaultCreateInfo;

        defaultCreateInfo.SizePixels = DefaultFontSize;
        defaultCreateInfo.GlyphRange = GGlyphRange;
        defaultCreateInfo.OversampleH = 3; // FIXME: 2 may be a better default?
        defaultCreateInfo.OversampleV = 1;
        defaultCreateInfo.bPixelSnapH = false;
        defaultCreateInfo.GlyphExtraSpacing = Float2( 0.0f, 0.0f );
        defaultCreateInfo.GlyphOffset = Float2( 0.0f, 0.0f );
        defaultCreateInfo.GlyphMinAdvanceX = 0.0f;
        defaultCreateInfo.GlyphMaxAdvanceX = Math::MaxValue< float >();
        defaultCreateInfo.RasterizerMultiply = 1.0f;
    }

    if ( !Build( _SysMem, _SizeInBytes, _CreateInfo ) ) {
        return;
    }

    // Create atlas texture
    AtlasTexture = NewObject< ATexture >();
    AtlasTexture->Initialize2D( TEXTURE_PF_R8, 1, TexWidth, TexHeight );
    AtlasTexture->WriteTextureData2D( 0, 0, TexWidth, TexHeight, 0, TexPixelsAlpha8 );
}

void AFont::InitializeFromMemoryCompressedTTF( const void * _SysMem, size_t _SizeInBytes, SFontCreateInfo const * _CreateInfo ) {
    byte * decompressedData;
    size_t decompressedSize;
    if ( !Core::ZDecompressToHeap( (byte const *)_SysMem, _SizeInBytes, &decompressedData, &decompressedSize ) ) {
        CriticalError( "InitializeFromMemoryCompressedTTF: failed to decompress source data\n" );
    }
    InitializeFromMemoryTTF( decompressedData, decompressedSize, _CreateInfo );
    GHeapMemory.Free( decompressedData );
}

void AFont::InitializeFromMemoryCompressedBase85TTF( const char * _SysMem, SFontCreateInfo const * _CreateInfo ) {
    size_t compressedTTFSize = Core::DecodeBase85( (byte const *)_SysMem, nullptr );
    void * compressedTTF = GHunkMemory.Alloc( compressedTTFSize );
    Core::DecodeBase85( (byte const *)_SysMem, (byte *)compressedTTF );
    InitializeFromMemoryCompressedTTF( compressedTTF, compressedTTFSize, _CreateInfo );
    GHunkMemory.ClearLastHunk();
}

void AFont::LoadInternalResource( const char * _Path ) {
    if ( !Core::Stricmp( _Path, "/Default/Fonts/Default" ) ) {

        // Load embedded ProggyClean.ttf, disable oversampling
        SFontCreateInfo createInfo = {};
        createInfo.FontNum = 0;
        createInfo.SizePixels = DefaultFontSize;
        createInfo.GlyphRange = GGlyphRange;
        createInfo.OversampleH = 1;
        createInfo.OversampleV = 1;
        createInfo.bPixelSnapH = true;
        createInfo.GlyphExtraSpacing = Float2( 0.0f, 0.0f );
        createInfo.GlyphOffset = Float2( 0.0f, 0.0f );
        createInfo.GlyphMinAdvanceX = 0.0f;
        createInfo.GlyphMaxAdvanceX = Math::MaxValue< float >();
        createInfo.RasterizerMultiply = 1.0f;

        InitializeFromMemoryCompressedTTF( ProggyClean_Data, ProggyClean_Size, &createInfo );
        //InitializeFromMemoryCompressedBase85TTF( GetDefaultCompressedFontDataTTFBase85(), &createInfo );
        DrawOffset.Y = 1.0f;
        return;
    }

    GLogger.Printf( "Unknown internal font %s\n", _Path );

    LoadInternalResource( "/Default/Fonts/Default" );
}

bool AFont::LoadResource( AString const & _Path ) {
    Purge();

    int i = _Path.FindExtWithoutDot();
    const char * n = &_Path[i];

    AString fn = _Path;
    fn.StripExt();

    AFileStream f;
    if ( !f.OpenRead( fn.CStr() ) )
        return false;

    size_t sizeInBytes = f.SizeInBytes();
    if ( !sizeInBytes ) {
        return false;
    }

    int hunkMark = GHunkMemory.SetHunkMark();

    void * data = GHunkMemory.Alloc( sizeInBytes );
    f.ReadBuffer( data, sizeInBytes );
    if ( f.GetReadBytesCount() != sizeInBytes ) {
        GHunkMemory.ClearToMark( hunkMark );
        return false;
    }

    uint32_t sizePixels;
    sizePixels = Math::ToInt< uint32_t >( n );
    if ( sizePixels < 8 ) {
        sizePixels = 8;
    }

    SFontCreateInfo createInfo = {};
    createInfo.FontNum = 0;
    createInfo.SizePixels = sizePixels;
    createInfo.GlyphRange = GGlyphRange;
    createInfo.OversampleH = 3; // FIXME: 2 may be a better default?
    createInfo.OversampleV = 1;
    createInfo.bPixelSnapH = false;
    createInfo.GlyphExtraSpacing = Float2( 0.0f, 0.0f );
    createInfo.GlyphOffset = Float2( 0.0f, 0.0f );
    createInfo.GlyphMinAdvanceX = 0.0f;
    createInfo.GlyphMaxAdvanceX = Math::MaxValue< float >();
    createInfo.RasterizerMultiply = 1.0f;

    InitializeFromMemoryTTF( data, sizeInBytes, &createInfo );

    GHunkMemory.ClearToMark( hunkMark );

    return true;
}

void AFont::Purge() {
    TexWidth = 0;
    TexHeight = 0;
    TexUvScale = Float2( 0.0f, 0.0f );
    TexUvWhitePixel = Float2( 0.0f, 0.0f );
    FallbackAdvanceX = 0.0f;
    FallbackGlyph = NULL;
    Glyphs.Free();
    WideCharAdvanceX.Free();
    WideCharToGlyph.Free();
    CustomRects.Free();
    AtlasTexture = nullptr;
    GHeapMemory.Free( TexPixelsAlpha8 );
    TexPixelsAlpha8 = nullptr;
}

Float2 AFont::CalcTextSizeA( float _Size, float _MaxWidth, float _WrapWidth, const char * _TextBegin, const char * _TextEnd, const char** _Remaining ) const {
    if ( !IsValid() ) {
        return Float2::Zero();
    }

    if ( !_TextEnd )
        _TextEnd = _TextBegin + strlen( _TextBegin ); // FIXME-OPT: Need to avoid this.

    const float line_height = _Size;
    const float scale = _Size / FontSize;

    Float2 text_size = Float2( 0, 0 );
    float line_width = 0.0f;

    const bool word_wrap_enabled = (_WrapWidth > 0.0f);
    const char* word_wrap_eol = NULL;

    const char* s = _TextBegin;
    while ( s < _TextEnd )
    {
        if ( word_wrap_enabled )
        {
            // Calculate how far we can render. Requires two passes on the string data but keeps the code simple and not intrusive for what's essentially an uncommon feature.
            if ( !word_wrap_eol )
            {
                word_wrap_eol = CalcWordWrapPositionA( scale, s, _TextEnd, _WrapWidth - line_width );
                if ( word_wrap_eol == s ) // Wrap_width is too small to fit anything. Force displaying 1 character to minimize the height discontinuity.
                    word_wrap_eol++;    // +1 may not be a character start point in UTF-8 but it's ok because we use s >= word_wrap_eol below
            }

            if ( s >= word_wrap_eol )
            {
                if ( text_size.X < line_width )
                    text_size.X = line_width;
                text_size.Y += line_height;
                line_width = 0.0f;
                word_wrap_eol = NULL;

                // Wrapping skips upcoming blanks
                while ( s < _TextEnd )
                {
                    const char c = *s;
                    if ( Core::CharIsBlank( c ) ) { s++; } else if ( c == '\n' ) { s++; break; } else { break; }
                }
                continue;
            }
        }

        // Decode and advance source
        const char* prev_s = s;
        SWideChar c = (SWideChar)*s;
        if ( c < 0x80 )
        {
            s += 1;
        } else
        {
            s += Core::WideCharDecodeUTF8( s, _TextEnd, c );
            if ( c == 0 ) // Malformed UTF-8?
                break;
        }

        if ( c < 32 )
        {
            if ( c == '\n' )
            {
                text_size.X = Math::Max( text_size.X, line_width );
                text_size.Y += line_height;
                line_width = 0.0f;
                continue;
            }
            if ( c == '\r' )
                continue;
        }

        const float char_width = ((int)c < WideCharAdvanceX.Size() ? WideCharAdvanceX.ToPtr()[c] : FallbackAdvanceX) * scale;
        if ( line_width + char_width >= _MaxWidth )
        {
            s = prev_s;
            break;
        }

        line_width += char_width;
    }

    if ( text_size.X < line_width )
        text_size.X = line_width;

    if ( line_width > 0 || text_size.Y == 0.0f )
        text_size.Y += line_height;

    if ( _Remaining )
        *_Remaining = s;

    return text_size;
}

const char * AFont::CalcWordWrapPositionA( float _Scale, const char * _Text, const char * _TextEnd, float _WrapWidth ) const {
    if ( !IsValid() ) {
        return _Text;
    }

    // Simple word-wrapping for English, not full-featured. Please submit failing cases!
    // FIXME: Much possible improvements (don't cut things like "word !", "word!!!" but cut within "word,,,,", more sensible support for punctuations, support for Unicode punctuations, etc.)

    // For references, possible wrap point marked with ^
    //  "aaa bbb, ccc,ddd. eee   fff. ggg!"
    //      ^    ^    ^   ^   ^__    ^    ^

    // List of hardcoded separators: .,;!?'"

    // Skip extra blanks after a line returns (that includes not counting them in width computation)
    // e.g. "Hello    world" --> "Hello" "World"

    // Cut words that cannot possibly fit within one line.
    // e.g.: "The tropical fish" with ~5 characters worth of width --> "The tr" "opical" "fish"

    float line_width = 0.0f;
    float word_width = 0.0f;
    float blank_width = 0.0f;
    _WrapWidth /= _Scale; // We work with unscaled widths to avoid scaling every characters

    const char* word_end = _Text;
    const char* prev_word_end = NULL;
    bool inside_word = true;

    const char* s = _Text;
    while ( s < _TextEnd )
    {
        SWideChar c = (SWideChar)*s;
        const char* next_s;
        if ( c < 0x80 )
            next_s = s + 1;
        else
            next_s = s + Core::WideCharDecodeUTF8( s, _TextEnd, c );
        if ( c == 0 )
            break;

        if ( c < 32 )
        {
            if ( c == '\n' )
            {
                line_width = word_width = blank_width = 0.0f;
                inside_word = true;
                s = next_s;
                continue;
            }
            if ( c == '\r' )
            {
                s = next_s;
                continue;
            }
        }

        const float char_width = ((int)c < WideCharAdvanceX.Size() ? WideCharAdvanceX.ToPtr()[c] : FallbackAdvanceX);
        if ( Core::WideCharIsBlank( c ) )
        {
            if ( inside_word )
            {
                line_width += blank_width;
                blank_width = 0.0f;
                word_end = s;
            }
            blank_width += char_width;
            inside_word = false;
        } else
        {
            word_width += char_width;
            if ( inside_word )
            {
                word_end = next_s;
            } else
            {
                prev_word_end = word_end;
                line_width += word_width + blank_width;
                word_width = blank_width = 0.0f;
            }

            // Allow wrapping after punctuation.
            inside_word = !(c == '.' || c == ',' || c == ';' || c == '!' || c == '?' || c == '\"');
        }

        // We ignore blank width at the end of the line (they can be skipped)
        if ( line_width + word_width > _WrapWidth )
        {
            // Words that cannot possibly fit within an entire line will be cut anywhere.
            if ( word_width < _WrapWidth )
                s = prev_word_end ? prev_word_end : word_end;
            break;
        }

        s = next_s;
    }

    return s;
}

SWideChar const * AFont::CalcWordWrapPositionW( float _Scale, SWideChar const * _Text, SWideChar const * _TextEnd, float _WrapWidth ) const {
    if ( !IsValid() ) {
        return _Text;
    }

    // Simple word-wrapping for English, not full-featured. Please submit failing cases!
    // FIXME: Much possible improvements (don't cut things like "word !", "word!!!" but cut within "word,,,,", more sensible support for punctuations, support for Unicode punctuations, etc.)

    // For references, possible wrap point marked with ^
    //  "aaa bbb, ccc,ddd. eee   fff. ggg!"
    //      ^    ^    ^   ^   ^__    ^    ^

    // List of hardcoded separators: .,;!?'"

    // Skip extra blanks after a line returns (that includes not counting them in width computation)
    // e.g. "Hello    world" --> "Hello" "World"

    // Cut words that cannot possibly fit within one line.
    // e.g.: "The tropical fish" with ~5 characters worth of width --> "The tr" "opical" "fish"

    float line_width = 0.0f;
    float word_width = 0.0f;
    float blank_width = 0.0f;
    _WrapWidth /= _Scale; // We work with unscaled widths to avoid scaling every characters

    SWideChar const * word_end = _Text;
    SWideChar const * prev_word_end = NULL;
    bool inside_word = true;

    SWideChar const * s = _Text;
    while ( s < _TextEnd )
    {
        SWideChar c = *s;

        if ( c == 0 )
            break;

        SWideChar const * next_s = s + 1;

        if ( c < 32 )
        {
            if ( c == '\n' )
            {
                line_width = word_width = blank_width = 0.0f;
                inside_word = true;
                s = next_s;
                continue;
            }
            if ( c == '\r' )
            {
                s = next_s;
                continue;
            }
        }

        const float char_width = ((int)c < WideCharAdvanceX.Size() ? WideCharAdvanceX.ToPtr()[c] : FallbackAdvanceX);
        if ( Core::WideCharIsBlank( c ) )
        {
            if ( inside_word )
            {
                line_width += blank_width;
                blank_width = 0.0f;
                word_end = s;
            }
            blank_width += char_width;
            inside_word = false;
        } else
        {
            word_width += char_width;
            if ( inside_word )
            {
                word_end = next_s;
            } else
            {
                prev_word_end = word_end;
                line_width += word_width + blank_width;
                word_width = blank_width = 0.0f;
            }

            // Allow wrapping after punctuation.
            inside_word = !(c == '.' || c == ',' || c == ';' || c == '!' || c == '?' || c == '\"');
        }

        // We ignore blank width at the end of the line (they can be skipped)
        if ( line_width + word_width >= _WrapWidth )
        {
            // Words that cannot possibly fit within an entire line will be cut anywhere.
            if ( word_width < _WrapWidth )
                s = prev_word_end ? prev_word_end : word_end;
            break;
        }

        s = next_s;
    }

    return s;
}

void AFont::SetDrawOffset( Float2 const & _Offset ) {
    DrawOffset = _Offset;
}

bool AFont::GetMouseCursorTexData( EDrawCursor cursor_type, Float2* out_offset, Float2* out_size, Float2 out_uv_border[2], Float2 out_uv_fill[2] ) const {
    if ( cursor_type < DRAW_CURSOR_ARROW || cursor_type > DRAW_CURSOR_RESIZE_HAND )
        return false;

    SFontCustomRect const & r = CustomRects[0];
    Float2 pos = CursorTexData[cursor_type][0] + Float2( (float)r.X, (float)r.Y );
    Float2 size = CursorTexData[cursor_type][1];
    *out_size = size;
    *out_offset = CursorTexData[cursor_type][2];
    out_uv_border[0] = (pos)* TexUvScale;
    out_uv_border[1] = (pos + size) * TexUvScale;
    pos.X += FONT_ATLAS_DEFAULT_TEX_DATA_W_HALF + 1;
    out_uv_fill[0] = (pos)* TexUvScale;
    out_uv_fill[1] = (pos + size) * TexUvScale;
    return true;
}

void AFont::SetGlyphRanges( EGlyphRange _GlyphRange ) {
    GGlyphRange = _GlyphRange;
}

static void ImFontAtlasBuildMultiplyCalcLookupTable( unsigned char out_table[256], float in_brighten_factor )
{
    for ( unsigned int i = 0; i < 256; i++ )
    {
        unsigned int value = (unsigned int)(i * in_brighten_factor);
        out_table[i] = value > 255 ? 255 : (value & 0xFF);
    }
}

static void ImFontAtlasBuildMultiplyRectAlpha8( const unsigned char table[256], unsigned char* pixels, int x, int y, int w, int h, int stride )
{
    unsigned char* data = pixels + x + y * stride;
    for ( int j = h; j > 0; j--, data += stride )
        for ( int i = 0; i < w; i++ )
            data[i] = table[data[i]];
}

// Helper: ImBoolVector
// Store 1-bit per value. Note that Resize() currently clears the whole vector.
struct ImBoolVector
{
    TPodArray<int>   Storage;
    ImBoolVector() { }
    void            Resize( int sz ) { Storage.ResizeInvalidate( (sz + 31) >> 5 ); Storage.ZeroMem(); }
    void            Clear() { Storage.Clear(); }
    bool            GetBit( int n ) const { int off = (n >> 5); int mask = 1 << (n & 31); return (Storage[off] & mask) != 0; }
    void            SetBit( int n, bool v ) { int off = (n >> 5); int mask = 1 << (n & 31); if ( v ) Storage[off] |= mask; else Storage[off] &= ~mask; }
};

bool AFont::Build( const void * _SysMem, size_t _SizeInBytes, SFontCreateInfo const * _CreateInfo ) {
    AN_ASSERT( _SysMem != NULL && _SizeInBytes > 0 );
    AN_ASSERT( _CreateInfo->SizePixels > 0.0f );

    SFontCreateInfo const & cfg = *_CreateInfo;
    FontSize = cfg.SizePixels;

    AddCustomRect( FONT_ATLAS_DEFAULT_TEX_DATA_ID, FONT_ATLAS_DEFAULT_TEX_DATA_W_HALF*2+1, FONT_ATLAS_DEFAULT_TEX_DATA_H );

    const int fontOffset = stbtt_GetFontOffsetForIndex( (const unsigned char*)_SysMem, cfg.FontNum );
    if ( fontOffset < 0 ) {
        GLogger.Printf( "AFont::Build: Invalid font\n" );
        return false;
    }

    stbtt_fontinfo fontInfo = {};
    if ( !stbtt_InitFont( &fontInfo, (const unsigned char*)_SysMem, fontOffset ) ) {
        GLogger.Printf( "AFont::Build: Invalid font\n" );
        return false;
    }

    SWideChar const * glyphRanges = GetGlyphRange( cfg.GlyphRange );

    // Measure highest codepoints
    int glyphsHighest = 0;
    for ( SWideChar const * range = glyphRanges; range[0] && range[1]; range += 2 ) {
        glyphsHighest = Math::Max( glyphsHighest, (int)range[1] );
    }

    // 2. For every requested codepoint, check for their presence in the font data, and handle redundancy or overlaps between source fonts to avoid unused glyphs.
    int totalGlyphsCount = 0;

    ImBoolVector glyphsSet;
    glyphsSet.Resize( glyphsHighest + 1 );

    for ( SWideChar const * range = glyphRanges; range[0] && range[1]; range += 2 ) {
        for ( unsigned int codepoint = range[0]; codepoint <= range[1]; codepoint++ ) {
            if ( glyphsSet.GetBit( codepoint ) ) {
                GLogger.Printf( "Warning: duplicated glyph\n" );
                continue;
            }

            if ( !stbtt_FindGlyphIndex( &fontInfo, codepoint ) ) {   // It is actually in the font?
                continue;
            }

            // Add to avail set/counters
            glyphsSet.SetBit( codepoint, true );
            totalGlyphsCount++;
        }
    }

    if ( totalGlyphsCount == 0 ) {
        GLogger.Printf( "AFont::Build: total glyph count = 0\n" );
        return false;
    }

    // 3. Unpack our bit map into a flat list (we now have all the Unicode points that we know are requested _and_ available _and_ not overlapping another)
    TPodArray< int > glyphsList;
    glyphsList.Reserve( totalGlyphsCount );
    const int* it_begin = glyphsSet.Storage.begin();
    const int* it_end = glyphsSet.Storage.end();
    for ( const int* it = it_begin; it < it_end; it++ )
        if ( int entries_32 = *it )
            for ( int bit_n = 0; bit_n < 32; bit_n++ )
                if ( entries_32 & (1u << bit_n) )
                    glyphsList.Append( (int)((it - it_begin) << 5) + bit_n );
    AN_ASSERT( glyphsList.Size() == totalGlyphsCount );

    // Allocate packing character data and flag packed characters buffer as non-packed (x0=y0=x1=y1=0)
    // (We technically don't need to zero-clear rects, but let's do it for the sake of sanity)

    // Rectangle to pack. We first fill in their size and the packer will give us their position.
    TPodArray< stbrp_rect > rects;
    rects.Resize( totalGlyphsCount );
    rects.ZeroMem();

    // Output glyphs
    TPodArray< stbtt_packedchar > packedChars;
    packedChars.Resize( totalGlyphsCount );
    packedChars.ZeroMem();

    // 4. Gather glyphs sizes so we can pack them in our virtual canvas.

    // Convert our ranges in the format stb_truetype wants
    stbtt_pack_range packRange = {};          // Hold the list of codepoints to pack (essentially points to Codepoints.Data)
    packRange.font_size = cfg.SizePixels;
    packRange.first_unicode_codepoint_in_range = 0;
    packRange.array_of_unicode_codepoints = glyphsList.ToPtr();
    packRange.num_chars = glyphsList.Size();
    packRange.chardata_for_range = packedChars.ToPtr();
    packRange.h_oversample = (unsigned char)cfg.OversampleH;
    packRange.v_oversample = (unsigned char)cfg.OversampleV;

    // Gather the sizes of all rectangles we will need to pack (this loop is based on stbtt_PackFontRangesGatherRects)
    const float scale = (cfg.SizePixels > 0) ? stbtt_ScaleForPixelHeight( &fontInfo, cfg.SizePixels ) : stbtt_ScaleForMappingEmToPixels( &fontInfo, -cfg.SizePixels );
    const int padding = TexGlyphPadding;
    int area = 0;
    for ( int glyph_i = 0; glyph_i < glyphsList.Size(); glyph_i++ ) {
        int x0, y0, x1, y1;
        const int glyph_index_in_font = stbtt_FindGlyphIndex( &fontInfo, glyphsList[glyph_i] );
        AN_ASSERT( glyph_index_in_font != 0 );
        stbtt_GetGlyphBitmapBoxSubpixel( &fontInfo, glyph_index_in_font, scale * cfg.OversampleH, scale * cfg.OversampleV, 0, 0, &x0, &y0, &x1, &y1 );
        rects[glyph_i].w = (stbrp_coord)(x1 - x0 + padding + cfg.OversampleH - 1);
        rects[glyph_i].h = (stbrp_coord)(y1 - y0 + padding + cfg.OversampleV - 1);
        area += rects[glyph_i].w * rects[glyph_i].h;
    }

    // We need a width for the skyline algorithm, any width!
    // The exact width doesn't really matter much, but some API/GPU have texture size limitations and increasing width can decrease height.
    const int surface_sqrt = (int)StdSqrt( (float)area ) + 1;
    TexWidth = (surface_sqrt >= 4096*0.7f) ? 4096 : (surface_sqrt >= 2048*0.7f) ? 2048 : (surface_sqrt >= 1024*0.7f) ? 1024 : 512;
    TexHeight = 0;

    // 5. Start packing
    // Pack our extra data rectangles first, so it will be on the upper-left corner of our texture (UV will have small values).
    const int TEX_HEIGHT_MAX = 1024 * 32;
    stbtt_pack_context spc = {};
    stbtt_PackBegin( &spc, NULL, TexWidth, TEX_HEIGHT_MAX, 0, TexGlyphPadding, NULL );

    stbrp_context* pack_context = (stbrp_context *)spc.pack_info;
    AN_ASSERT( pack_context != NULL );

    AN_ASSERT( CustomRects.Size() >= 1 ); // We expect at least the default custom rects to be registered, else something went wrong.

    TPodArray< stbrp_rect > pack_rects;
    pack_rects.Resize( CustomRects.Size() );
    pack_rects.ZeroMem();
    for ( int i = 0; i < CustomRects.Size(); i++ ) {
        pack_rects[i].w = CustomRects[i].Width;
        pack_rects[i].h = CustomRects[i].Height;
    }
    stbrp_pack_rects( pack_context, pack_rects.ToPtr(), pack_rects.Size() );
    for ( int i = 0; i < pack_rects.Size(); i++ ) {
        if ( pack_rects[i].was_packed ) {
            CustomRects[i].X = pack_rects[i].x;
            CustomRects[i].Y = pack_rects[i].y;
            AN_ASSERT( pack_rects[i].w == CustomRects[i].Width && pack_rects[i].h == CustomRects[i].Height );
            TexHeight = Math::Max( TexHeight, pack_rects[i].y + pack_rects[i].h );
        }
    }

    // 6. Pack each source font. No rendering yet, we are working with rectangles in an infinitely tall texture at this point.
    stbrp_pack_rects( (stbrp_context*)spc.pack_info, rects.ToPtr(), totalGlyphsCount );

    // Extend texture height and mark missing glyphs as non-packed so we won't render them.
    // FIXME: We are not handling packing failure here (would happen if we got off TEX_HEIGHT_MAX or if a single if larger than TexWidth?)
    for ( int glyph_i = 0; glyph_i < totalGlyphsCount; glyph_i++ )
        if ( rects[glyph_i].was_packed )
            TexHeight = Math::Max( TexHeight, rects[glyph_i].y + rects[glyph_i].h );

    // 7. Allocate texture
    TexHeight = bTexNoPowerOfTwoHeight ? (TexHeight + 1) : Math::ToGreaterPowerOfTwo( TexHeight );
    TexUvScale = Float2( 1.0f / TexWidth, 1.0f / TexHeight );
    TexPixelsAlpha8 = (unsigned char*)GHeapMemory.ClearedAlloc( TexWidth * TexHeight );
    spc.pixels = TexPixelsAlpha8;
    spc.height = TexHeight;

    // 8. Render/rasterize font characters into the texture
    stbtt_PackFontRangesRenderIntoRects( &spc, &fontInfo, &packRange, 1, rects.ToPtr() );

    // Apply multiply operator
    if ( cfg.RasterizerMultiply != 1.0f ) {
        unsigned char multiply_table[256];
        ImFontAtlasBuildMultiplyCalcLookupTable( multiply_table, cfg.RasterizerMultiply );
        stbrp_rect* r = &rects[0];
        for ( int glyph_i = 0; glyph_i < totalGlyphsCount; glyph_i++, r++ ) {
            if ( r->was_packed ) {
                ImFontAtlasBuildMultiplyRectAlpha8( multiply_table, TexPixelsAlpha8, r->x, r->y, r->w, r->h, TexWidth );
            }
        }
    }

    // End packing
    stbtt_PackEnd( &spc );

    // 9. Setup AFont and glyphs for runtime
    const float font_scale = stbtt_ScaleForPixelHeight( &fontInfo, cfg.SizePixels );
    int unscaled_ascent, unscaled_descent, unscaled_line_gap;
    stbtt_GetFontVMetrics( &fontInfo, &unscaled_ascent, &unscaled_descent, &unscaled_line_gap );

    const float ascent = Math::Floor( unscaled_ascent * font_scale + ((unscaled_ascent > 0.0f) ? +1 : -1) );
    //const float descent = Math::Floor( unscaled_descent * font_scale + ((unscaled_descent > 0.0f) ? +1 : -1) );

    const float font_off_x = cfg.GlyphOffset.X;
    const float font_off_y = cfg.GlyphOffset.Y + Math::Round( ascent );

    for ( int glyph_i = 0; glyph_i < totalGlyphsCount; glyph_i++ ) {
        const int codepoint = glyphsList[glyph_i];
        const stbtt_packedchar& pc = packedChars[glyph_i];

        const float char_advance_x_org = pc.xadvance;
        const float char_advance_x_mod = Math::Clamp( char_advance_x_org, cfg.GlyphMinAdvanceX, cfg.GlyphMaxAdvanceX );
        float char_off_x = font_off_x;
        if ( char_advance_x_org != char_advance_x_mod ) {
            char_off_x += cfg.bPixelSnapH ? Math::Floor( (char_advance_x_mod - char_advance_x_org) * 0.5f ) : (char_advance_x_mod - char_advance_x_org) * 0.5f;
        }

        // Register glyph
        stbtt_aligned_quad q;
        float dummy_x = 0.0f, dummy_y = 0.0f;
        stbtt_GetPackedQuad( packedChars.ToPtr(), TexWidth, TexHeight, glyph_i, &dummy_x, &dummy_y, &q, 0 );
        AddGlyph( cfg, (SWideChar)codepoint, q.x0 + char_off_x, q.y0 + font_off_y, q.x1 + char_off_x, q.y1 + font_off_y, q.s0, q.t0, q.s1, q.t1, char_advance_x_mod );
    }

    // Render into our custom data block
    {
        SFontCustomRect & r = CustomRects[0];
        AN_ASSERT( r.Id == FONT_ATLAS_DEFAULT_TEX_DATA_ID );

        // Render/copy pixels
        AN_ASSERT( r.Width == FONT_ATLAS_DEFAULT_TEX_DATA_W_HALF * 2 + 1 && r.Height == FONT_ATLAS_DEFAULT_TEX_DATA_H );
        for ( int y = 0, n = 0; y < FONT_ATLAS_DEFAULT_TEX_DATA_H; y++ ) {
            for ( int x = 0; x < FONT_ATLAS_DEFAULT_TEX_DATA_W_HALF; x++, n++ ) {
                const int offset0 = (int)(r.X + x) + (int)(r.Y + y) * TexWidth;
                const int offset1 = offset0 + FONT_ATLAS_DEFAULT_TEX_DATA_W_HALF + 1;
                TexPixelsAlpha8[offset0] = FONT_ATLAS_DEFAULT_TEX_DATA_PIXELS[n] == '.' ? 0xFF : 0x00;
                TexPixelsAlpha8[offset1] = FONT_ATLAS_DEFAULT_TEX_DATA_PIXELS[n] == 'X' ? 0xFF : 0x00;
            }
        }

        TexUvWhitePixel = Float2( (r.X + 0.5f) * TexUvScale.X, (r.Y + 0.5f) * TexUvScale.Y );
    }

    // Register custom rectangle glyphs
    for ( int i = 0; i < CustomRects.Size(); i++ ) {
        SFontCustomRect const & r = CustomRects[i];
        if ( r.Id >= 0x110000 ) {
            continue;
        }

        Float2 uv0 = Float2( (float)r.X * TexUvScale.X, (float)r.Y * TexUvScale.Y );
        Float2 uv1 = Float2( (float)(r.X + r.Width) * TexUvScale.X, (float)(r.Y + r.Height) * TexUvScale.Y );

        AddGlyph( cfg,
            (SWideChar)r.Id,
                  r.GlyphOffset.X, r.GlyphOffset.Y,
                  r.GlyphOffset.X + r.Width, r.GlyphOffset.Y + r.Height,
                  uv0.X, uv0.Y, uv1.X, uv1.Y,
                  r.GlyphAdvanceX );
    }

    AN_ASSERT( Glyphs.Size() < 0xFFFF ); // -1 is reserved

    int widecharCount = 0;
    for ( int i = 0; i != Glyphs.Size(); i++ ) {
        widecharCount = Math::Max( widecharCount, (int)Glyphs[i].Codepoint );
    }
    widecharCount = widecharCount + 1;

    WideCharAdvanceX.Resize( widecharCount );
    WideCharToGlyph.Resize( widecharCount );

    for ( int n = 0 ; n < widecharCount ; n++ ) {
        WideCharAdvanceX[n] = -1.0f;
        WideCharToGlyph[n] = 0xffff;
    }

    for ( int i = 0; i < Glyphs.Size(); i++ ) {
        int codepoint = (int)Glyphs[i].Codepoint;
        WideCharAdvanceX[codepoint] = Glyphs[i].AdvanceX;
        WideCharToGlyph[codepoint] = (unsigned short)i;

        // Ensure there is no TAB codepoint
        AN_ASSERT( codepoint != '\t' );
    }

    // Create a glyph to handle TAB
    const int space = ' ';
    if ( space < WideCharToGlyph.Size() && WideCharToGlyph[space] != 0xffff ) {
        const int codepoint = '\t';
        if ( codepoint < widecharCount ) {
            SFontGlyph tabGlyph = Glyphs[WideCharToGlyph[space]];
            tabGlyph.Codepoint = codepoint;
            tabGlyph.AdvanceX *= TabSize;
            Glyphs.Append( tabGlyph );
            WideCharAdvanceX[codepoint] = tabGlyph.AdvanceX;
            WideCharToGlyph[codepoint] = (unsigned short)(Glyphs.Size() - 1);
        } else {
            GLogger.Printf( "AFont::Build: Warning: couldn't create TAB glyph\n" );
        }
    }

    unsigned short fallbackGlyphNum = 0;
    if ( FallbackChar < WideCharToGlyph.Size() && WideCharToGlyph[FallbackChar] != 0xffff ) {
        fallbackGlyphNum = WideCharToGlyph[FallbackChar];
    } else {
        GLogger.Printf( "AFont::Build: Warning: fallback character not found\n" );
    }

    FallbackGlyph = &Glyphs[fallbackGlyphNum];
    FallbackAdvanceX = FallbackGlyph->AdvanceX;

    for ( int i = 0; i < widecharCount ; i++ ) {
        if ( WideCharAdvanceX[i] < 0.0f ) {
            WideCharAdvanceX[i] = FallbackAdvanceX;
        }
        if ( WideCharToGlyph[i] == 0xffff ) {
            WideCharToGlyph[i] = fallbackGlyphNum;
        }
    }

    return true;
}

// x0/y0/x1/y1 are offset from the character upper-left layout position, in pixels. Therefore x0/y0 are often fairly close to zero.
// Not to be mistaken with texture coordinates, which are held by u0/v0/u1/v1 in normalized format (0.0..1.0 on each texture axis).
void AFont::AddGlyph( SFontCreateInfo const & cfg, SWideChar codepoint, float x0, float y0, float x1, float y1, float u0, float v0, float u1, float v1, float advance_x )
{
    Glyphs.Resize( Glyphs.Size() + 1 );
    SFontGlyph & glyph = Glyphs.Last();
    glyph.Codepoint = (SWideChar)codepoint;
    glyph.X0 = x0;
    glyph.Y0 = y0;
    glyph.X1 = x1;
    glyph.Y1 = y1;
    glyph.U0 = u0;
    glyph.V0 = v0;
    glyph.U1 = u1;
    glyph.V1 = v1;
    glyph.AdvanceX = advance_x + cfg.GlyphExtraSpacing.X;  // Bake spacing into AdvanceX

    if ( cfg.bPixelSnapH )
        glyph.AdvanceX = Math::Round( glyph.AdvanceX );
}

int AFont::AddCustomRect( unsigned int id, int width, int height ) {
    // Id needs to be >= 0x110000. Id >= 0x80000000 are reserved for internal use
    AN_ASSERT( id >= 0x110000 );
    AN_ASSERT( width > 0 && width <= 0xFFFF );
    AN_ASSERT( height > 0 && height <= 0xFFFF );
    SFontCustomRect r;
    r.Id = id;
    r.Width = (unsigned short)width;
    r.Height = (unsigned short)height;
    r.X = r.Y = 0xFFFF;
    r.GlyphAdvanceX = 0.0f;
    r.GlyphOffset = Float2( 0, 0 );
    CustomRects.Append( r );
    return CustomRects.Size() - 1; // Return index
}

// Example: BinaryToCompressedC( "Common/Fonts/ProggyClean.ttf", "Common/Fonts/ProggyClean.c", "ProggyClean" );
bool BinaryToCompressedC( const char * _SourceFile, const char * _DestFile, const char * _SymName, bool _EncodeBase85 ) {
    AFileStream source, dest;

    if ( !source.OpenRead( _SourceFile ) ) {
        GLogger.Printf( "Failed to open %s\n", _SourceFile );
        return false;
    }

    if ( !dest.OpenWrite( _DestFile ) ) {
        GLogger.Printf( "Failed to open %s\n", _DestFile );
        return false;
    }

    size_t decompressedSize = source.SizeInBytes();
    byte * decompressedData = (byte *)GHeapMemory.Alloc( decompressedSize );
    source.ReadBuffer( decompressedData, decompressedSize );

    size_t compressedSize = Core::ZMaxCompressedSize( decompressedSize );
    byte * compressedData = (byte *)GHeapMemory.Alloc( compressedSize );
    Core::ZCompress( compressedData, &compressedSize, (byte *)decompressedData, decompressedSize, ZLIB_UBER_COMPRESSION );

    if ( _EncodeBase85 ) {
        dest.Printf( "static const char %s_Data_Base85[%d+1] =\n    \"", _SymName, (int)((compressedSize+3)/4)*5 );
        char prev_c = 0;
        for ( size_t i = 0; i < compressedSize; i += 4 ) {
            uint32_t d = *(uint32_t *)(compressedData + i);
            for ( int j = 0; j < 5; j++, d /= 85 ) {
                unsigned int x = (d % 85) + 35;
                char c = (x>='\\') ? x+1 : x;
                dest.Printf( (c == '?' && prev_c == '?') ? "\\%c" : "%c", c );
                prev_c = c;
            }
            if ( (i % 112) == 112-4 )
                dest.Printf( "\"\n    \"" );
        }
        dest.Printf( "\";\n\n" );
    } else {
        dest.Printf( "static const size_t %s_Size = %d;\n", _SymName, (int)compressedSize );
        dest.Printf( "static const uint64_t %s_Data[%d] =\n{", _SymName, Align( compressedSize, 8 ) );
        int column = 0;
        for ( size_t i = 0; i < compressedSize; i += 8 ) {
            uint64_t d = *(uint64_t *)(compressedData + i);
            if ( (column++ % 6) == 0 ) {
                dest.Printf( "\n    0x%08x%08x, ", Math::INT64_HIGH_INT( d ), Math::INT64_LOW_INT( d ) );
            } else {
                dest.Printf( "0x%08x%08x, ", Math::INT64_HIGH_INT( d ), Math::INT64_LOW_INT( d ) );
            }
        }
        dest.Printf( "\n};\n\n" );
    }

    GHeapMemory.Free( compressedData );
    GHeapMemory.Free( decompressedData );

    return true;
}
