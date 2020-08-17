# Angie Engine

Game engine

Developed for Quake-style 3D games, but it is possible to create games of other genres.

## A bit of history
The beginning was laid a long time ago, the development of the latest version of the engine
started from scratch in the fall of 2018. Before that, there were other versions that were not very successful.

## Fun fact
One of the engine versions was called Unity and was developed in 2002. It was a clone of Quake 3 Arena (only level renderer). At that time, I was inspired by Carmack's code, the sources of Quake1 and Doom2 were already available.

The name Angie Engine was chosen by the name of heroine Angela "Angie" McAlister after watching the series Under The Dome. I liked her name, and it is in tune with the word Engine, and I thought, why not?

# Features

## Technical information
* C++  
* Windows7+, Linux  
* 64 bit only
* SSE2

## Actor-Component based world architecture
* Reflection
* Garbage collector
* World consist of Actors, Actors consist of Components, Components share resources.
   
## Different geometry types
* Static Mesh
* Procedural Mesh
* Skinned Mesh
* Surface

## Animation
* Procedural via ProceduralMesh
* Skinned via SkinnedMesh and animation controllers
* Component

## Dynamic and static lighting with shadows
* Directional Light
* Point Light
* Spot Light
* Diffuse per-pixel lightmaps
* Diffuse per-vertex light
* Dynamic per-pixel lighting
* Cascaded shadow maps (PSSM+PCF)
* Photometric profiles

## Material system
* Material graph with automatic shader generation
* Material instancing
* Different material types:
HUD;
Unlit;
Base Light (non-physically based);
PBR (physically based).
* Parallax mapping
* Tessellation Flat/PN with displacement mapping

## Forward+ clustered renderer based on OpenGL4.5 core
* Modern Framegraph architecture
* Depth pre-pass
* Wireframe and normals renderer
* Debug renderer (points, lines, polygons)
* Batching
* SRGB
* Premultiplied alpha
* Virtual texturing

## Postprocess effects
* Bloom
* Dynamic exposure
* Tonemapping
* Color grading (LUT, procedural)
* Vegnette
* FXAA
* HBAO
* SSLR
* Motion Blur (per-object, per-bone)

## Advanced culling system
* Frustum Culling
* Portal Culling
* Potentially Visible Set
* BSP

## Asset importing and resource management
* 3D modelling formats (GLTF, LWO)
* Different image formats (PNG, PSD, PNM, PIC, JPG, BMP, TGA, HDR, EXR)
* Fonts (TTF)
* Photometric profiles (IES)

## Audio
* Decoders ogg, mp3, wav
* Streaming support
* 2D and 3D
* Unlimited audio sources

## Fast scene raycasting
* Used speedup structures such as BSP, BVH, Portals

## Physics
* Powered by BulletPhysics engine
* Different collision shapes such as:
       Sphere.
       Box.
       Cylinder.
       Capsule.
       Convex.
       Triangle Soup
* Physics raycasting, sphere casting, convex casting
* Dynamic bodies simmulation
* Kinematic bodies
* Soft bodies (experimental)
* Triggers
* Convex decomposition

## Input system
* Devices
      Mouse,
      Keyboard,
      Joystick/Gamepad
* Axes mapping
* Actions mapping

## 2D
* Widget system GUI
* Canvas (low level 2D rendering api)
* Materials
* Blending

## AI
* Tiled navigation mesh for path finding

## Misc
* Powerful memory allocation system
* Classic console with commands and variables
* Multiple players
* Multiple viewports (up to 16 viewports per frame) with blending
* Multiple worlds and levels
* World pausing without additional user extra-code
* Documentation in doxygen format
* MIT license
   
## Used third-party libraries
* stb, cgltw, lwo2, tinyexr, fastlz, miniz, glew, sdl2, bullet, vhacd, recastnavigation, imgui, iesna, clipper, glutess
* Dynamic loaded: openal, libmpg123

## Some architectural solutions
* STL-less.
* Not using exceptions.
* Code style C with classes.
* Minimum use of third-party libraries.

## Planned features
* Particle System
* Global Illumination
* Toksvig
* Decals
* Area lights
* Directional lightmaps (radiocity?)
* Built-in lightmapper
* LODs
* Terrain
* Water
* Vegetation
* Imposters
* Subsurface scattering
* Postprocess materials
* Postprocess techinques: DOF, Air distortion
* More parallel computations
* SDF fonts
* Lens flares, godrays
* Network, steam integration
* Antialiasing (MSAA, TXAA)
* Multisampling for debug renderer
* Text and icons for debug renderer
* Occlusion culling imprevements: Software Occlusion Culling, HiZ, or someting like this
* Navmesh: dynamic obstacles, areas, area connections, crowd
* Audio: effects
* Physics: soft body, constraints, ragdoll, character controller
* Material graph editor
* Improve shadow mapping


## Contributing
Contributions are very welcome.


## Airplane minigame demo
[![Airplane minigame](http://img.youtube.com/vi/T9h7byyu_eQ/0.jpg)](https://youtu.be/T9h7byyu_eQ "Airplane minigame")
