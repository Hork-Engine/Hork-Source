cmake_minimum_required(VERSION 3.7.2)

OPTION( ANGIE_ENGINE_WITH_SAMPLES "Angie engine with samples" ON )

include_directories( Engine )
add_definitions( -DANGIE_STATIC_LIBRARY )

# Engine
add_subdirectory( Engine )

# Samples
if (ANGIE_ENGINE_WITH_SAMPLES)

add_subdirectory( Samples/Airplane )
set_target_properties( Airplane PROPERTIES FOLDER "Samples" )

add_subdirectory( Samples/Blade )
set_target_properties( Blade PROPERTIES FOLDER "Samples" )

add_subdirectory( Samples/Physics )
set_target_properties( Physics PROPERTIES FOLDER "Samples" )

add_subdirectory( Samples/Planet )
set_target_properties( Planet PROPERTIES FOLDER "Samples" )

add_subdirectory( Samples/Portals )
set_target_properties( Portals PROPERTIES FOLDER "Samples" )

add_subdirectory( Samples/Quake1 )
set_target_properties( Quake1 PROPERTIES FOLDER "Samples" )

add_subdirectory( Samples/Shooter3D )
set_target_properties( Shooter3D PROPERTIES FOLDER "Samples" )

add_subdirectory( Samples/Sponza )
set_target_properties( Sponza PROPERTIES FOLDER "Samples" )

add_subdirectory( Samples/VirtualTexture )
set_target_properties( VirtualTexture PROPERTIES FOLDER "Samples" )

add_subdirectory( Samples/WitchhammerRPG )
set_target_properties( WitchhammerRPG PROPERTIES FOLDER "Samples" )

# Set StartUp project
#set_property( DIRECTORY PROPERTY VS_STARTUP_PROJECT Sponza )
endif()
