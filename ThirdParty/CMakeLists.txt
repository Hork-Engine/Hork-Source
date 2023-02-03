message(STATUS "SDL2.................................")
set(SDL_RENDER OFF CACHE BOOL "" FORCE)
set(SDL_RENDER_D3D OFF CACHE BOOL "" FORCE)
set(SDL_DIRECTX OFF CACHE BOOL "" FORCE)
set(SDL2_DISABLE_SDL2MAIN   ON CACHE BOOL "" FORCE)
set(SDL2_DISABLE_INSTALL    ON CACHE BOOL "" FORCE)
set(SDL2_DISABLE_UNINSTALL  ON CACHE BOOL "" FORCE)
set(SDL_LIBC  ON CACHE BOOL "" FORCE)
add_subdirectory_with_folder("ThirdParty" SDL2)
install_thirdparty_includes(SDL2/include/ SDL)

message(STATUS "MIMALLOC.................................")
set(MI_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(MI_BUILD_OBJECT OFF CACHE BOOL "" FORCE)
set(MI_OVERRIDE OFF CACHE BOOL "" FORCE)
set(MI_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory_with_folder("ThirdParty" mimalloc)
install_thirdparty_includes(mimalloc/include/ mimalloc)

message(STATUS "EASTL.................................")
add_subdirectory_with_folder("ThirdParty" EASTL)
install_thirdparty_includes(EASTL/include/EASTL/ EASTL)
install_thirdparty_includes(EASTL/EABase/include/Common/EABase/ EABase)

message(STATUS "FMT.................................")
add_subdirectory(fmt)
install_thirdparty_includes(fmt/include/fmt/ fmt)

message(STATUS "STB.................................")
install_thirdparty_includes(stb/ stb)

message(STATUS "BULLET PHYSICS.................................")
set(USE_DOUBLE_PRECISION OFF CACHE BOOL "" FORCE)
set(USE_SOFT_BODY_MULTI_BODY_DYNAMICS_WORLD OFF CACHE BOOL "" FORCE)
set(BUILD_BULLET3 OFF CACHE BOOL "" FORCE)
add_subdirectory_with_folder("ThirdParty/BulletPhysics" bullet3)
install_thirdparty_includes(bullet3/src/ .)

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

message(STATUS "TINYEXR.................................")
add_subdirectory_with_folder("ThirdParty" tinyexr)
install_thirdparty_includes(tinyexr/ tinyexr)

message(STATUS "CGLTF.................................")
add_subdirectory_with_folder("ThirdParty" cgltf)
install_thirdparty_includes(cgltf/ cgltf)

message(STATUS "FAST_OBJ.................................")
add_subdirectory_with_folder("ThirdParty" fast_obj)
install_thirdparty_includes(fast_obj/ fast_obj)

message(STATUS "MIKKTSPACE.................................")
add_subdirectory_with_folder("ThirdParty" MikkTSpace)
install_thirdparty_includes(MikkTSpace/ MikkTSpace)

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

message(STATUS "ANGELSCRIPT.................................")
add_subdirectory_with_folder("ThirdParty" angelscript/projects/cmake)
add_subdirectory_with_folder("ThirdParty" angelscript_addon)
install_thirdparty_includes(angelscript/include/ angelscript)
install_thirdparty_includes(angelscript_addon/ angelscript)

message(STATUS "OPTICK.................................")
add_subdirectory_with_folder("ThirdParty" optick)
install_thirdparty_includes(optick/src/ optick)

message(STATUS "bc6h_enc.................................")
install_thirdparty_includes(bc6h_enc/ bc6h_enc)

message(STATUS "nanovg.................................")
install_thirdparty_includes(nanovg/ nanovg)
