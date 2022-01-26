cmake_minimum_required(VERSION 3.7.2)

OPTION( ANGIE_ENGINE_WITH_SAMPLES "Angie engine with samples" ON )

include_directories( Engine )
add_definitions( -DANGIE_STATIC_LIBRARY )

# Engine
add_subdirectory( Engine )

# Samples
if (ANGIE_ENGINE_WITH_SAMPLES)

add_subdirectory( Samples/01_Simple )
set_target_properties( 01_Simple PROPERTIES FOLDER "Samples" )

add_subdirectory( Samples/02_CharacterController)
set_target_properties( 02_CharacterController PROPERTIES FOLDER "Samples" )

add_subdirectory( Samples/03_Triggers)
set_target_properties( 03_Triggers PROPERTIES FOLDER "Samples" )

add_subdirectory( Samples/04_Terrain )
set_target_properties( 04_Terrain PROPERTIES FOLDER "Samples" )

add_subdirectory( Samples/Airplane )
set_target_properties( Airplane PROPERTIES FOLDER "Samples" )

add_subdirectory( Samples/Billboard )
set_target_properties( Billboard PROPERTIES FOLDER "Samples" )

add_subdirectory( Samples/Blade )
set_target_properties( Blade PROPERTIES FOLDER "Samples" )

add_subdirectory( Samples/Physics )
set_target_properties( Physics PROPERTIES FOLDER "Samples" )

add_subdirectory( Samples/Planet )
set_target_properties( Planet PROPERTIES FOLDER "Samples" )

add_subdirectory( Samples/Portals2 )
set_target_properties( Portals PROPERTIES FOLDER "Samples" )

add_subdirectory( Samples/Project1 )
set_target_properties( Project1 PROPERTIES FOLDER "Samples" )

add_subdirectory( Samples/Quake1 )
set_target_properties( Quake1 PROPERTIES FOLDER "Samples" )

add_subdirectory( Samples/Shooter3D )
set_target_properties( Shooter3D PROPERTIES FOLDER "Samples" )

add_subdirectory( Samples/Sponza2 )
set_target_properties( Sponza PROPERTIES FOLDER "Samples" )

add_subdirectory( Samples/VirtualTexture )
set_target_properties( VirtualTexture PROPERTIES FOLDER "Samples" )

add_subdirectory( Samples/WitchhammerRPG )
set_target_properties( WitchhammerRPG PROPERTIES FOLDER "Samples" )

add_subdirectory( Samples/CruelMaze )
set_target_properties( CruelMaze PROPERTIES FOLDER "Samples" )

# Set StartUp project
#set_property( DIRECTORY PROPERTY VS_STARTUP_PROJECT Sponza )
endif()
