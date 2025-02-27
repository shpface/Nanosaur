cmake_minimum_required(VERSION 3.13)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

#set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_STANDARD 20)

set(CMAKE_OSX_DEPLOYMENT_TARGET "10.11" CACHE STRING "Minimum OS X deployment version")

project(Nanosaur
	VERSION 1.4.1
	LANGUAGES C CXX
)

set(GAME_TARGET ${PROJECT_NAME})
set(GAME_ICNS cmake/${PROJECT_NAME}.icns)

#------------------------------------------------------------------------------
# GLOBAL OPTIONS (BEFORE ADDING SUBDIRECTORIES)
#------------------------------------------------------------------------------

if(MSVC)
	# This lets us detect that MSVC supports C++20 features via the __cplusplus macro
	# (See: https://docs.microsoft.com/en-us/cpp/build/reference/zc-cplusplus)
	add_compile_options(/Zc:__cplusplus)
	add_compile_definitions(UNICODE _UNICODE)

	set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Nanosaur)

endif()

#------------------------------------------------------------------------------
# DEPENDENCIES
#------------------------------------------------------------------------------

add_subdirectory(extern/Quesa)
add_subdirectory(extern/Pomme)

find_package(SDL2 REQUIRED COMPONENTS main)
find_package(OpenGL REQUIRED)

#------------------------------------------------------------------------------
# GAME SOURCES
#------------------------------------------------------------------------------

set(GAME_SRCDIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Write header file containing version info
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.h.in ${GAME_SRCDIR}/Headers/version.h)

set(GAME_CORE_SOURCES
	${GAME_SRCDIR}/System/Main.c
	${GAME_SRCDIR}/System/File.c
	${GAME_SRCDIR}/System/Collision.c
	${GAME_SRCDIR}/System/input.c
	${GAME_SRCDIR}/System/Misc.c
	${GAME_SRCDIR}/System/Objects.c
	${GAME_SRCDIR}/System/Objects2.c
	${GAME_SRCDIR}/System/Sprites.c
	${GAME_SRCDIR}/System/Sound.c
	${GAME_SRCDIR}/System/Windows.c
	${GAME_SRCDIR}/System/SelfRunDemo.c
	${GAME_SRCDIR}/Terrain/Terrain.c
	${GAME_SRCDIR}/Terrain/Terrain2.c
	${GAME_SRCDIR}/Player/MyGuy.c
	${GAME_SRCDIR}/Player/Player_Control.c
	${GAME_SRCDIR}/Player/Weapons.c
	${GAME_SRCDIR}/Enemies/Enemy.c
	${GAME_SRCDIR}/Enemies/Enemy_TriCer.c
	${GAME_SRCDIR}/Enemies/Enemy_Stego.c
	${GAME_SRCDIR}/Enemies/Enemy_Rex.c
	${GAME_SRCDIR}/Enemies/Enemy_Ptera.c
	${GAME_SRCDIR}/Enemies/Enemy_Spitter.c
	${GAME_SRCDIR}/Items/Items.c
	${GAME_SRCDIR}/Items/TimePortal.c
	${GAME_SRCDIR}/Items/Pickups.c
	${GAME_SRCDIR}/Items/Effects.c
	${GAME_SRCDIR}/Items/Traps.c
	${GAME_SRCDIR}/Items/Triggers.c
	${GAME_SRCDIR}/Skeleton/SkeletonObj.c
	${GAME_SRCDIR}/Skeleton/SkeletonJoints.c
	${GAME_SRCDIR}/Skeleton/SkeletonAnim.c
	${GAME_SRCDIR}/Skeleton/Bones.c
	${GAME_SRCDIR}/QD3D/EnvironmentMap.c
	${GAME_SRCDIR}/QD3D/Camera.c
	${GAME_SRCDIR}/QD3D/QD3D_Support.c
	${GAME_SRCDIR}/QD3D/QD3D_Geometry.c
	${GAME_SRCDIR}/QD3D/3DMF.c
	${GAME_SRCDIR}/QD3D/3DMath.c
	${GAME_SRCDIR}/Screens/Infobar.c
	${GAME_SRCDIR}/Screens/Title.c
	${GAME_SRCDIR}/Screens/MainMenu.c
	${GAME_SRCDIR}/Screens/HighScores.c
)

set(GAME_CORE_HEADERS
	${GAME_SRCDIR}/Headers/main.h
	${GAME_SRCDIR}/Headers/file.h
	${GAME_SRCDIR}/Headers/collision.h
	${GAME_SRCDIR}/Headers/input.h
	${GAME_SRCDIR}/Headers/misc.h
	${GAME_SRCDIR}/Headers/objects.h
	${GAME_SRCDIR}/Headers/3dmath.h
	${GAME_SRCDIR}/Headers/3dmf.h
	${GAME_SRCDIR}/Headers/bones.h
	${GAME_SRCDIR}/Headers/camera.h
	${GAME_SRCDIR}/Headers/effects.h
	${GAME_SRCDIR}/Headers/enemy.h
	${GAME_SRCDIR}/Headers/environmentmap.h
	${GAME_SRCDIR}/Headers/globals.h
	${GAME_SRCDIR}/Headers/highscores.h
	${GAME_SRCDIR}/Headers/infobar.h
	${GAME_SRCDIR}/Headers/items.h
	${GAME_SRCDIR}/Headers/mainmenu.h
	${GAME_SRCDIR}/Headers/mobjtypes.h
	${GAME_SRCDIR}/Headers/movie.h
	${GAME_SRCDIR}/Headers/myguy.h
	${GAME_SRCDIR}/Headers/mytraps.h
	${GAME_SRCDIR}/Headers/objtypes.h
	${GAME_SRCDIR}/Headers/pickups.h
	${GAME_SRCDIR}/Headers/player_control.h
	${GAME_SRCDIR}/Headers/qd3d_geometry.h
	${GAME_SRCDIR}/Headers/qd3d_support.h
	${GAME_SRCDIR}/Headers/selfrundemo.h
	${GAME_SRCDIR}/Headers/skeletonanim.h
	${GAME_SRCDIR}/Headers/skeletonjoints.h
	${GAME_SRCDIR}/Headers/skeletonobj.h
	${GAME_SRCDIR}/Headers/sound2.h
	${GAME_SRCDIR}/Headers/sprites.h
	${GAME_SRCDIR}/Headers/structs.h
	${GAME_SRCDIR}/Headers/terrain.h
	${GAME_SRCDIR}/Headers/timeportal.h
	${GAME_SRCDIR}/Headers/title.h
	${GAME_SRCDIR}/Headers/triggers.h
	${GAME_SRCDIR}/Headers/version.h
	${GAME_SRCDIR}/Headers/weapons.h
	${GAME_SRCDIR}/Headers/windows_nano.h
)

set(GAME_GLUE_SOURCES
	${GAME_SRCDIR}/Main.cpp
	${GAME_SRCDIR}/Glue/Backdrop.cpp
	${GAME_SRCDIR}/Glue/EdgePadding.cpp
	${GAME_SRCDIR}/Glue/FrustumCulling.c
	${GAME_SRCDIR}/Glue/GamePatches.cpp
	${GAME_SRCDIR}/Glue/GLBackdrop.cpp
	${GAME_SRCDIR}/Glue/GLFunctions.cpp
	${GAME_SRCDIR}/Glue/Patch3DMFModels.c
	${GAME_SRCDIR}/Glue/ProMode.c
	${GAME_SRCDIR}/Glue/SettingsScreen.cpp
	${GAME_SRCDIR}/Glue/Q3FSSpecStorage.cpp
	${GAME_SRCDIR}/Headers/Glue/GamePatches.h
	${GAME_SRCDIR}/Headers/Glue/GLBackdrop.h
	${GAME_SRCDIR}/Headers/Glue/GLFunctions.h
	${GAME_SRCDIR}/Headers/QD3DCompat/QD3D.h
	${GAME_SRCDIR}/Headers/QD3DCompat/QD3DCamera.h
	${GAME_SRCDIR}/Headers/QD3DCompat/QD3DDrawContext.h
	${GAME_SRCDIR}/Headers/QD3DCompat/QD3DErrors.h
	${GAME_SRCDIR}/Headers/QD3DCompat/QD3DGeometry.h
	${GAME_SRCDIR}/Headers/QD3DCompat/QD3DGroup.h
	${GAME_SRCDIR}/Headers/QD3DCompat/QD3DIO.h
	${GAME_SRCDIR}/Headers/QD3DCompat/QD3DLight.h
	${GAME_SRCDIR}/Headers/QD3DCompat/QD3DMath.h
	${GAME_SRCDIR}/Headers/QD3DCompat/QD3DPick.h
	${GAME_SRCDIR}/Headers/QD3DCompat/QD3DRenderer.h
	${GAME_SRCDIR}/Headers/QD3DCompat/QD3DSet.h
	${GAME_SRCDIR}/Headers/QD3DCompat/QD3DShader.h
	${GAME_SRCDIR}/Headers/QD3DCompat/QD3DStorage.h
	${GAME_SRCDIR}/Headers/QD3DCompat/QD3DStyle.h
	${GAME_SRCDIR}/Headers/QD3DCompat/QD3DTransform.h
	${GAME_SRCDIR}/Headers/QD3DCompat/QD3DView.h
)

set(GAME_ALL_SOURCES
	${GAME_CORE_SOURCES}
	${GAME_CORE_HEADERS}
	${GAME_GLUE_SOURCES}
)

#------------------------------------------------------------------------------
# SOURCE GROUPS
#------------------------------------------------------------------------------

source_group(TREE ${GAME_SRCDIR} PREFIX "" FILES ${GAME_ALL_SOURCES})

#------------------------------------------------------------------------------
# SOURCE FILE PROPERTIES
#------------------------------------------------------------------------------

if(NOT MSVC)
	set_source_files_properties(${GAME_CORE_SOURCES} PROPERTIES COMPILE_FLAGS "-include Pomme.h")
else()
	set_source_files_properties(${GAME_CORE_SOURCES} PROPERTIES COMPILE_FLAGS /FIPomme.h)
endif()

#------------------------------------------------------------------------------
# EXECUTABLE TARGET
#------------------------------------------------------------------------------

if(WIN32)
	# "WIN32" here is equivalent to /SUBSYSTEM:WINDOWS for MSVC
	add_executable(${GAME_TARGET} WIN32
		${GAME_ALL_SOURCES}
		"${CMAKE_CURRENT_SOURCE_DIR}/cmake/WindowsResources.rc"
	)
elseif(APPLE)
	add_executable(${GAME_TARGET} MACOSX_BUNDLE
		${GAME_ALL_SOURCES}
		${GAME_ICNS}
		${SDL2_LIBRARIES}
	)
else()
	add_executable(${GAME_TARGET} ${GAME_ALL_SOURCES})
endif()

target_include_directories(${GAME_TARGET} PRIVATE
	${SDL2_INCLUDE_DIRS}
	${OPENGL_INCLUDE_DIRS}
	extern/Pomme/src
	${GAME_SRCDIR}
	${GAME_SRCDIR}/Headers
	${GAME_SRCDIR}/Headers/Glue
	${GAME_SRCDIR}/Headers/QD3DCompat
)

target_link_libraries(${GAME_TARGET}
	Quesa_SDL
	Pomme
	${SDL2_LIBRARIES}
	${OPENGL_LIBRARIES}
)

#------------------------------------------------------------------------------
# DEFINES
#------------------------------------------------------------------------------

add_compile_definitions(
	"$<$<CONFIG:DEBUG>:_DEBUG>"
)

target_compile_definitions(Quesa_SDL PUBLIC
	QUESA_OS_SDL
	QUESA_APPLY_FOG_COLOR_IN_ALL_PASSES
)

target_compile_definitions(${GAME_TARGET} PRIVATE
	QUESA_OS_SDL
	SOURCE_PORT_ENHANCEMENTS
)

if(WIN32)
	# On Win32, we need NOGDI and NOUSER to be able to define some Mac functions
	# whose names are otherwise taken by Windows APIs.
	target_compile_definitions(${GAME_TARGET} PRIVATE WIN32_LEAN_AND_MEAN NOGDI NOUSER)
endif()

#------------------------------------------------------------------------------
# COMPILER OPTIONS
#------------------------------------------------------------------------------

if(NOT MSVC)
	target_compile_options(${GAME_TARGET} PRIVATE
		-fexceptions
		-Wall
		-Wextra
		-Wshadow
		-Wno-multichar
		-Wno-unknown-pragmas
		-Wno-unused-parameter
	)
else()
	target_compile_options(${GAME_TARGET} PRIVATE
		/EHa    # C++ may throw exceptions from extern "C" functions
		/W4
		/wd4068 # ignore unrecognized pragmas
		/wd4100 # unreferenced formal parameters
		/wd4201 # nonstandard extension (nameless struct)
		/wd4244 # conversion from double to float
		/wd4305 # truncation from double to float
	)
endif()

#------------------------------------------------------------------------------
# PLATFORM-SPECIFIC PACKAGING
#------------------------------------------------------------------------------

if(APPLE)
	set_target_properties(${GAME_TARGET} PROPERTIES
		# Set framework search path to (App bundle)/Contents/Frameworks so the game can use its embedded SDL2.framework
		XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "@executable_path/../Frameworks"

		# Explicitly turn off code signing, otherwise downloaded app will be quarantined forever
		XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY ""

		RESOURCE ${GAME_ICNS}
		MACOSX_BUNDLE_ICON_FILE ${PROJECT_NAME}.icns
		MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
		MACOSX_BUNDLE_COPYRIGHT "© 1998 Pangea Software, Inc."
	)

	# Copy stuff to app bundle contents
	set_source_files_properties(${GAME_ICNS} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
	set_source_files_properties(${SDL2_LIBRARIES} PROPERTIES MACOSX_PACKAGE_LOCATION "Frameworks")

	set(BUNDLE_CONTENTS_DIR "$<TARGET_FILE_DIR:${PROJECT_NAME}>/..")
	set(APP_PARENT_DIR "${BUNDLE_CONTENTS_DIR}/../..")

	add_custom_command(TARGET ${GAME_TARGET} POST_BUILD
		# Copy assets to app bundle
		COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/Data ${BUNDLE_CONTENTS_DIR}/Resources

		# Nuke SDL headers in embedded framework
		COMMAND rm -rf ${BUNDLE_CONTENTS_DIR}/Frameworks/SDL2.framework/Headers
		COMMAND rm -rf ${BUNDLE_CONTENTS_DIR}/Frameworks/SDL2.framework/Versions/Current/Headers

		# High-DPI support in Info.plist
#		plutil -replace NSHighResolutionCapable -bool true ${BUNDLE_CONTENTS_DIR}/Info.plist
	)
else()
	set(APP_PARENT_DIR "$<TARGET_FILE_DIR:${GAME_TARGET}>")

	# Copy assets besides executable
	add_custom_command(TARGET ${GAME_TARGET} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/Data ${APP_PARENT_DIR}/Data)
endif()

# Copy SDL2 DLLs to output folder on Windows
if(WIN32)
	foreach(DLL ${SDL2_DLLS})
		add_custom_command(TARGET ${GAME_TARGET} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different ${DLL} ${APP_PARENT_DIR})
	endforeach()

	set_target_properties(${GAME_TARGET} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
endif()

# Copy documentation to output folder
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/ReadMe.txt.in ${CMAKE_CURRENT_BINARY_DIR}/ReadMe.txt)
add_custom_command(TARGET ${GAME_TARGET} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_BINARY_DIR}/ReadMe.txt ${APP_PARENT_DIR}/ReadMe.txt
#	COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.md ${APP_PARENT_DIR}/License.txt
	COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/docs ${APP_PARENT_DIR}/Documentation
	COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/CHANGELOG.md ${APP_PARENT_DIR}/Documentation/CHANGELOG.md
)

