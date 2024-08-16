# Hork Engine

Game engine

Developed for Quake-style 3D games, but it is possible to create games of other genres.

Samples can be found here:\
https://github.com/Hork-Engine/Hork-Samples

Old version of the engine:\
https://github.com/Hork-Engine/Hork-Source-Archive

# Features

## Technical information
* C++  
* Windows7+, Linux  
* 64 bit only
* Data oriented design
* With no exceptions

## GameObject-Component based world architecture
* The world has a modular structure - it's easy to add new features
* Composition instead of inheritance, objects are extended by different components
* Handles instead of pointers
* ECS (Entity-Component-System) can be easily integrated into an existing model
* Asynchronous loading of resources

## Rendering
* Forward+ clustered renderer based on OpenGL4.6 core
* Modern Framegraph architecture
* Material graph with automatic shader generation, material instancing
* Physically Based Rendering
* Antialiasing (FXAA, SMAA), specular antialiasing (Toksvig, vMF baked to Roughness)
* Screen space reflections
* Postprocessing effects (Bloom, Dynamic exposure, Tonemapping, Color grading LUT/procedural, Vegnette)
* Motion Blur (per-object, per-bone)
* Horizon-based Ambient Occlusion (HBAO)
* Tessellation Flat/PN with displacement mapping
* Parallax mapping

## Physics
* Simulation of rigid bodies, collision detection, triggers, character controller, water buoyancy calculations.

## Audio
* 2D and spatialized 3D / HRTF
* Unlimited audio sources
* OGG, FLAC, MP3, WAV
* Audio resampling

## Input system
* Keyboard, Mouse, Game controllers
* Mapping to Axes and Actions
   
## Animation
* Skeletal animation, inverse kinematics, sockets

## Navigation
* Tiled navigation mesh
* Path finding
* Off-mesh links
* Dynamic obstacles

## UI
* Customizable UI
* Antialiased vector rendering, HTML5 - like canvas API
* TTF fonts

## Asset processing
* All types of block compression and decompression: BC1-7
* Image resampling
* sRGB / Premultiplied alpha aware
* WEBP, PNG, PSD, PNM, PIC, JPG, BMP, TGA, HDR, EXR
* 3D formats: GLTF 2.0, OBJ
* IES profiles
* SVG rasterization
* Various geometry processing: convex decomposition, triangulation, polygon clipping
   
## Used third-party libraries
* bc7enc
* bcdec
* cgltf
* Clipper
* EASTL
* FastLZ
* fast_float
* fast_obj
* {fmt}
* GLEW
* glslang
* glutess
* HACD
* Jolt Physics
* libwebp
* lunasvg (freetype, plutovg as part of lunasvg)
* MikkTSpace
* mimalloc
* miniaudio
* Miniz
* muFFT
* nanovg
* Optick
* ozz-animation
* Recast & Detour
* SDL2
* stb
* tinyexr
* ufbx
* V-HACD
* FXAA
* SMAA
* half.c

See [details](ThirdParty.md).


## Contributing
Contributions are very welcome.


## Airplane minigame demo
[![Airplane minigame](http://img.youtube.com/vi/T9h7byyu_eQ/0.jpg)](https://youtu.be/T9h7byyu_eQ "Airplane minigame")
