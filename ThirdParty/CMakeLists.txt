#message(STATUS "Vulkan-Headers.................................")
#add_subdirectory_with_folder("ThirdParty" Vulkan-Headers)
#install_thirdparty_includes(Vulkan-Headers/include/vulkan/ vulkan)

message(STATUS "GLSLANG.................................")
set(BUILD_SHARED_LIBS OFF)
set(GLSLANG_TESTS OFF)
set(GLSLANG_ENABLE_INSTALL OFF)
set(ENABLE_GLSLANG_BINARIES OFF)
set(ENABLE_GLSLANG_JS OFF)
set(ENABLE_HLSL OFF)
set(ENABLE_EXCEPTIONS OFF)
set(ENABLE_RTTI OFF)
set(ENABLE_OPT OFF)
add_subdirectory_with_folder("ThirdParty/glslang" glslang)
install_thirdparty_includes(glslang/ glslang)

message(STATUS "SDL3.................................")
set(SDL_TEST   OFF)
set(SDL_RENDER OFF )
set(SDL_RENDER_D3D OFF)
set(SDL_DIRECTX OFF)
set(SDL2_DISABLE_SDL2MAIN   ON)
set(SDL2_DISABLE_INSTALL    ON)
set(SDL2_DISABLE_UNINSTALL  ON)
set(SDL_LIBC  ON)
add_subdirectory_with_folder("ThirdParty" SDL3)
install_thirdparty_includes(SDL3/include/ SDL)

message(STATUS "MIMALLOC.................................")
set(MI_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(MI_BUILD_OBJECT OFF CACHE BOOL "" FORCE)
set(MI_OVERRIDE OFF CACHE BOOL "" FORCE)
set(MI_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory_with_folder("ThirdParty" mimalloc)
install_thirdparty_includes(mimalloc/include/ mimalloc)

message(STATUS "EASTL.................................")
add_subdirectory_with_folder("ThirdParty" EABase)
add_subdirectory_with_folder("ThirdParty" EASTL)
install_thirdparty_includes(EABase/include/Common/EABase/ EABase)
install_thirdparty_includes(EASTL/include/EASTL/ EASTL)

message(STATUS "FMT.................................")
add_subdirectory(fmt)
install_thirdparty_includes(fmt/include/fmt/ fmt)

message(STATUS "STB.................................")
install_thirdparty_includes(stb/ stb)

message(STATUS "JOLT PHYSICS.................................")
set(TARGET_HELLO_WORLD OFF CACHE BOOL "" FORCE)
set(TARGET_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(TARGET_PERFORMANCE_TEST OFF CACHE BOOL "" FORCE)
set(TARGET_SAMPLES OFF CACHE BOOL "" FORCE)
set(TARGET_VIEWER OFF CACHE BOOL "" FORCE)
set(USE_STATIC_MSVC_RUNTIME_LIBRARY OFF)
add_subdirectory_with_folder("ThirdParty/JoltPhysics" JoltPhysics/Build)
install_thirdparty_includes(JoltPhysics/Jolt/ Jolt)

message(STATUS "HACD.................................")
add_subdirectory_with_folder("ThirdParty" HACD)
install_thirdparty_includes(HACD/ HACD)

message(STATUS "VHACD.................................")
add_subdirectory_with_folder("ThirdParty" VHACD)
install_thirdparty_includes(VHACD/ VHACD)

message(STATUS "RECAST NAVIGATION.................................")
add_subdirectory_with_folder("ThirdParty" recastnavigation)
install_thirdparty_includes(recastnavigation/DebugUtils/Include/ Detour)
install_thirdparty_includes(recastnavigation/Detour/Include/ Detour)
install_thirdparty_includes(recastnavigation/DetourCrowd/Include/ Detour)
install_thirdparty_includes(recastnavigation/DetourTileCache/Include/ Detour)
install_thirdparty_includes(recastnavigation/Recast/Include/ Recast)

message(STATUS "muFFT.................................")
add_subdirectory_with_folder("ThirdParty/muFFT" muFFT)
install_thirdparty_includes(muFFT/ muFFT)

message(STATUS "MINIAUDIO.................................")
add_subdirectory_with_folder("ThirdParty" miniaudio)
install_thirdparty_includes(miniaudio/ miniaudio)

message(STATUS "MINIZ.................................")
add_subdirectory_with_folder("ThirdParty" miniz)
install_thirdparty_includes(miniz/ miniz)

message(STATUS "FASTLZ.................................")
add_subdirectory_with_folder("ThirdParty" fastlz)
install_thirdparty_includes(fastlz/ fastlz)

message(STATUS "GLUTESS.................................")
add_subdirectory_with_folder("ThirdParty" glutess)
install_thirdparty_includes(glutess/ glutess)

message(STATUS "CLIPPER.................................")
add_subdirectory_with_folder("ThirdParty" clipper)
install_thirdparty_includes(clipper/ clipper)

message(STATUS "UFBX.................................")
add_subdirectory_with_folder("ThirdParty" ufbx)
install_thirdparty_includes(ufbx/ ufbx)

message(STATUS "CGLTF.................................")
add_subdirectory_with_folder("ThirdParty" cgltf)
install_thirdparty_includes(cgltf/ cgltf)

message(STATUS "FAST_OBJ.................................")
install_thirdparty_includes(fast_obj/ fast_obj)

message(STATUS "FAST_FLOAT.................................")
install_thirdparty_includes(fast_float/ fast_float)

message(STATUS "OZZ.................................")
add_subdirectory_with_folder("ThirdParty/Ozz" ozz)
install_thirdparty_includes(ozz/include/ozz ozz)

message(STATUS "MIKKTSPACE.................................")
add_subdirectory_with_folder("ThirdParty" MikkTSpace)
install_thirdparty_includes(MikkTSpace/ MikkTSpace)

message(STATUS "bc6h_enc.................................")
install_thirdparty_includes(bc6h_enc/ bc6h_enc)

message(STATUS "BC7ENC.................................")
add_subdirectory_with_folder("ThirdParty" bc7enc_rdo)
install_thirdparty_includes(bc7enc_rdo/ bc7enc_rdo)

message(STATUS "BCDEC.................................")
add_subdirectory_with_folder("ThirdParty" bcdec)
install_thirdparty_includes(bcdec/ bcdec)

message(STATUS "LUNASVG.................................")
add_subdirectory_with_folder("ThirdParty" lunasvg)
install_thirdparty_includes(lunasvg/ lunasvg)

message(STATUS "WEBP.................................")
add_subdirectory_with_folder("ThirdParty" WebP)
install_thirdparty_includes(WebP/src/webp/ webp)

message(STATUS "TINYEXR.................................")
add_subdirectory_with_folder("ThirdParty" tinyexr)
install_thirdparty_includes(tinyexr/ tinyexr)

message(STATUS "PL_MPEG.................................")
install_thirdparty_includes(pl_mpeg/ pl_mpeg)

message(STATUS "OPTICK.................................")
add_subdirectory_with_folder("ThirdParty" optick)
install_thirdparty_includes(optick/src/ optick)

message(STATUS "nanovg.................................")
install_thirdparty_includes(nanovg/ nanovg)
