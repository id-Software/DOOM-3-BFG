cmake_minimum_required(VERSION 2.8)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

project(RBDoom3)

option(USE_MFC_TOOLS
		"Compile the built-in MFC based tools" OFF)
		
option(MONOLITH
		"Embed game logic into main executable" ON)
		
#option(STANDALONE
#		"Skip Doom 3 base/ folder" ON)


if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
	add_definitions(-pipe)
	#add_definitions(-Wall)
	add_definitions(-mmmx -msse -msse2)
	if(WIN32)
		# require msvcr70.dll or newer for _aligned_malloc etc
		# I think it is from Visual C++ .NET 2002, so it should be available on any remotely modern system.
		add_definitions(-D__MSVCRT_VERSION__=0x0700)
		
		find_package(DirectX REQUIRED)
		include_directories(libs/mingw-hacks)
		include_directories(${DirectX_INCLUDE_DIR})
		add_definitions(-DUSE_XINPUT)
	endif()	
	
	#if(NOT ANDROID)
		add_definitions(-DUSE_EXCEPTIONS)
	#endif()
	
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -D_DEBUG -O0 -ggdb  -Wno-pragmas -fno-strict-aliasing -Wno-unused-variable -Wno-unused-but-set-variable -Wno-switch")
	#set(CMAKE_C_FLAGS_DEBUGALL "${CMAKE_C_FLAGS_DEBUGALL} -g -ggdb -D_DEBUG -Wno-pragmas -fno-strict-aliasing -Wno-unused-variable -Wno-unused-but-set-variable -Wno-switch")
	#set(CMAKE_C_FLAGS_PROFILE "${CMAKE_C_FLAGS_PROFILE} -g -ggdb -D_DEBUG -O1 -fno-omit-frame-pointer -Wunknown-pragmas -fno-strict-aliasing  -Wno-unused-variable -Wno-unused-but-set-variable -Wno-switch")
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -ffast-math -fno-unsafe-math-optimizations -fomit-frame-pointer -Wno-pragmas -fno-strict-aliasing  -Wno-unused-variable -Wno-unused-but-set-variable -Wno-switch")
	set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} -g -O3 -ffast-math -fno-unsafe-math-optimizations -fomit-frame-pointer -Wno-pragmas -fno-strict-aliasing  -Wno-unused-variable -Wno-unused-but-set-variable -Wno-switch")
	set(CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL} -Os -ffast-math -fno-unsafe-math-optimizations -fomit-frame-pointer -Wno-pragmas -fno-strict-aliasing -Wno-unused-variable -Wno-unused-but-set-variable -Wno-switch")
	
	set(CMAKE_CXX_FLAGS_DEBUG ${CMAKE_C_FLAGS_DEBUG})
	#set(CMAKE_CXX_FLAGS_DEBUGALL ${CMAKE_C_FLAGS_DEBUGALL})
	#set(CMAKE_CXX_FLAGS_PROFILE ${CMAKE_C_FLAGS_PROFILE})
	set(CMAKE_CXX_FLAGS_RELEASE ${CMAKE_C_FLAGS_RELEASE})
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO ${CMAKE_C_FLAGS_RELWITHDEBINFO})
	set(CMAKE_CXX_FLAGS_MINSIZEREL ${CMAKE_C_FLAGS_MINSIZEREL})

elseif(MSVC)
	
	# Multiprocessor support for compiling
	# /MP
	
	# /MT  use multithreaded, static runtime library
	#      (=> binary won't need MSVCP100D.dll or similar)
	# /MTd the same with debug symbols
	# /MD and /MDd - the same with dynamic runtime lib
	#     (=> MSVCP100D.dll or similar is needed to run binary)

	# Inline Function Expansion: Any Suitable (/Ob2)
	# "/Ob2",
			
	# enable Intrinsic Functions
	# "/Oi",
			
	# Omit Frame Pointers
	# "/Oy",
	
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /D _DEBUG /MP /MTd")
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MP /Oi /Oy /MT")
	set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} /MP /Oi /MTd")
	set(CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL} /MP /Oi /Oy /MT")
	
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MP /MTd")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MP /Oi /Oy /MT")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /MP /MTd")
	set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} /MP /Oi /Oy /MT")

	add_definitions(-DWIN32 
					-D_WINDOWS
					-D_CRT_SECURE_NO_DEPRECATE
					-D_CRT_NONSTDC_NO_DEPRECATE
					-D_CRT_SECURE_NO_WARNINGS
					-D_MBCS
					#-DUSE_OPENAL
					-DUSE_EXCEPTIONS)
					
	if(NOT CMAKE_CL_64)
		add_definitions(-D_USE_32BIT_TIME_T)
	endif()
					
	if(USE_MFC_TOOLS)
		add_definitions(-D_AFXDLL)
		
		# 0: Use Standard Windows Libraries
		# 1: Use MFC in a Static Library
		# 2: Use MFC in a Shared DLL 
		set(CMAKE_MFC_FLAG 2)
	endif()
endif()

#if(STANDALONE)
#	add_definitions(-DSTANDALONE)
#endif()

add_subdirectory(idlib)

file(GLOB AAS_INCLUDES aas/*.h)
file(GLOB AAS_SOURCES aas/*.cpp)

file(GLOB_RECURSE CM_INCLUDES cm/*.h)
file(GLOB_RECURSE CM_SOURCES cm/*.cpp)

file(GLOB FRAMEWORK_INCLUDES framework/*.h)
file(GLOB FRAMEWORK_SOURCES framework/*.cpp)

file(GLOB FRAMEWORK_ASYNC_INCLUDES framework/async/*.h)
file(GLOB FRAMEWORK_ASYNC_SOURCES framework/async/*.cpp)

file(GLOB_RECURSE RENDERER_INCLUDES renderer/*.h)
	# renderer/AutoRender.h
	# renderer/AutoRenderBink.h
	# renderer/BinaryImage.h
	# renderer/BinaryImageData.h
	# renderer/BoundsTrack.h
	# renderer/BufferObject.h
	# renderer/Cinematic.h
	# renderer/Font.h
	# renderer/GLMatrix.h
	# renderer/GLState.h
	# renderer/GuiModel.h
	# renderer/Image.h
	# renderer/ImageOpts.h
	# renderer/Interaction.h
	# renderer/Material.h
	# renderer/Model.h
	# renderer/ModelDecal.h
	# renderer/ModelManager.h
	# renderer/ModelOverlay.h
	# renderer/Model_ase.h
	# #renderer/Model_ColladaHelper.h
	# #renderer/Model_ColladaParser.h
	# renderer/Model_local.h
	# renderer/Model_lwo.h
	# renderer/Model_ma.h
	# renderer/Model_md3.h
	# renderer/RenderLog.h
	# renderer/RenderProgs.h
	# renderer/RenderSystem.h
	# renderer/RenderTexture.h
	# renderer/RenderWorld.h
	# renderer/RenderWorld_local.h
	# renderer/ResolutionScale.h
	# renderer/ScreenRect.h
	# renderer/simplex.h
	# renderer/tr_local.h
	# renderer/VertexCache.h)


file(GLOB_RECURSE RENDERER_SOURCES renderer/*.cpp)
	# renderer/AutoRender.cpp
	# renderer/Cinematic.cpp
	# renderer/Framebuffer.cpp
	# renderer/GLShader.cpp
	# renderer/GuiModel.cpp
	# renderer/Image_files.cpp
	# renderer/Image_init.cpp
	# renderer/Image_load.cpp
	# renderer/Image_process.cpp
	# renderer/Image_program.cpp
	# renderer/Interaction.cpp
	# renderer/Material.cpp
	# renderer/Model.cpp
	# renderer/ModelDecal.cpp
	# renderer/ModelManager.cpp
	# renderer/ModelOverlay.cpp
	# renderer/Model_ase.cpp
	# renderer/Model_beam.cpp
	# #renderer/Model_ColladaParser.cpp
	# renderer/Model_liquid.cpp
	# renderer/Model_lwo.cpp
	# renderer/Model_ma.cpp
	# renderer/Model_md3.cpp
	# renderer/Model_md5.cpp
	# renderer/Model_prt.cpp
	# renderer/Model_sprite.cpp
	# renderer/RenderEntity.cpp
	# renderer/RenderSystem.cpp
	# renderer/RenderSystem_init.cpp
	# renderer/RenderWorld.cpp
	# renderer/RenderWorld_demo.cpp
	# renderer/RenderWorld_load.cpp
	# renderer/RenderWorld_portals.cpp
	# renderer/VertexCache.cpp
	# renderer/draw_arb.cpp
	# renderer/draw_arb2.cpp
	# renderer/draw_glsl.cpp
	# renderer/draw_common.cpp
	# renderer/draw_exp.cpp
	# renderer/draw_nv10.cpp
	# renderer/draw_nv20.cpp
	# renderer/draw_r200.cpp
	# renderer/tr_backend.cpp
	# renderer/tr_deform.cpp
	# renderer/tr_font.cpp
	# renderer/tr_guisurf.cpp
	# renderer/tr_light.cpp
	# renderer/tr_lightrun.cpp
	# renderer/tr_main.cpp
	# renderer/tr_math.cpp
	# renderer/tr_orderIndexes.cpp
	# renderer/tr_polytope.cpp
	# renderer/tr_render.cpp
	# renderer/tr_rendertools.cpp
	# renderer/tr_shadowbounds.cpp
	# renderer/tr_stencilshadow.cpp
	# renderer/tr_subview.cpp
	# renderer/tr_trace.cpp
	# renderer/tr_trisurf.cpp
	# renderer/tr_turboshadow.cpp)


file(GLOB IRRXML_INCLUDES libs/irrxml/src/*.h)
file(GLOB IRRXML_SOURCES libs/irrxml/src/*.cpp)
	
set(JPEG_INCLUDES
	libs/jpeg-6/jchuff.h
	libs/jpeg-6/jconfig.h
	libs/jpeg-6/jdct.h
	libs/jpeg-6/jdhuff.h
	libs/jpeg-6/jerror.h
	libs/jpeg-6/jinclude.h
	libs/jpeg-6/jmemsys.h
	libs/jpeg-6/jmorecfg.h
	libs/jpeg-6/jpegint.h
	libs/jpeg-6/jpeglib.h
	libs/jpeg-6/jversion.h)
	
set(JPEG_SOURCES
	libs/jpeg-6/jcapimin.cpp
	libs/jpeg-6/jcapistd.cpp
	libs/jpeg-6/jccoefct.cpp
	libs/jpeg-6/jccolor.cpp
	libs/jpeg-6/jcdctmgr.cpp
	libs/jpeg-6/jchuff.cpp
	libs/jpeg-6/jcinit.cpp
	libs/jpeg-6/jcmainct.cpp
	libs/jpeg-6/jcmarker.cpp
	libs/jpeg-6/jcmaster.cpp
	libs/jpeg-6/jcomapi.cpp
	libs/jpeg-6/jcparam.cpp
	libs/jpeg-6/jcphuff.cpp
	libs/jpeg-6/jcprepct.cpp
	libs/jpeg-6/jcsample.cpp
	libs/jpeg-6/jctrans.cpp
	libs/jpeg-6/jdapimin.cpp
	libs/jpeg-6/jdapistd.cpp
	libs/jpeg-6/jdatadst.cpp
	libs/jpeg-6/jdatasrc.cpp
	libs/jpeg-6/jdcoefct.cpp
	libs/jpeg-6/jdcolor.cpp
	libs/jpeg-6/jddctmgr.cpp
	libs/jpeg-6/jdhuff.cpp
	libs/jpeg-6/jdinput.cpp
	libs/jpeg-6/jdmainct.cpp
	libs/jpeg-6/jdmarker.cpp
	libs/jpeg-6/jdmaster.cpp
	libs/jpeg-6/jdmerge.cpp
	libs/jpeg-6/jdphuff.cpp
	libs/jpeg-6/jdpostct.cpp
	libs/jpeg-6/jdsample.cpp
	libs/jpeg-6/jdtrans.cpp
	libs/jpeg-6/jerror.cpp
	libs/jpeg-6/jfdctflt.cpp
	libs/jpeg-6/jfdctfst.cpp
	libs/jpeg-6/jfdctint.cpp
	libs/jpeg-6/jidctflt.cpp
	libs/jpeg-6/jidctfst.cpp
	libs/jpeg-6/jidctint.cpp
	libs/jpeg-6/jidctred.cpp
	#libs/jpeg-6/jload.cpp
	libs/jpeg-6/jmemmgr.cpp
	libs/jpeg-6/jmemnobs.cpp
	#libs/jpeg-6/jpegtran.cpp
	libs/jpeg-6/jquant1.cpp
	libs/jpeg-6/jquant2.cpp
	libs/jpeg-6/jutils.cpp)

file(GLOB PNG_INCLUDES libs/png/*.h)
file(GLOB PNG_SOURCES libs/png/*.c)

file(GLOB ZLIB_INCLUDES libs/zlib/*.h)
file(GLOB ZLIB_SOURCES libs/zlib/*.c)
	
set(GLEW_INCLUDES
	libs/glew/include/GL/glew.h)
	
if(WIN32)
	set(GLEW_INCLUDES ${GLEW_INCLUDES} libs/glew/include/GL/wglew.h)
else()
	set(GLEW_INCLUDES ${GLEW_INCLUDES} libs/glew/include/GL/glxew.h)
endif()
	
set(GLEW_SOURCES
	libs/glew/src/glew.c)
	
set(FREETYPE_SOURCES
	libs/freetype/src/autofit/autofit.c
	libs/freetype/src/bdf/bdf.c
	libs/freetype/src/cff/cff.c
	libs/freetype/src/base/ftbase.c
	libs/freetype/src/base/ftbitmap.c
	libs/freetype/src/cache/ftcache.c
	libs/freetype/src/base/ftdebug.c
	libs/freetype/src/base/ftgasp.c
	libs/freetype/src/base/ftglyph.c
	libs/freetype/src/gzip/ftgzip.c
	libs/freetype/src/base/ftinit.c
	libs/freetype/src/lzw/ftlzw.c
	libs/freetype/src/base/ftstroke.c
	libs/freetype/src/base/ftsystem.c
	libs/freetype/src/smooth/smooth.c
	libs/freetype/src/base/ftbbox.c
	libs/freetype/src/base/ftmm.c
	libs/freetype/src/base/ftpfr.c
	libs/freetype/src/base/ftsynth.c
	libs/freetype/src/base/fttype1.c
	libs/freetype/src/base/ftwinfnt.c
	libs/freetype/src/pcf/pcf.c
	libs/freetype/src/pfr/pfr.c
	libs/freetype/src/psaux/psaux.c
	libs/freetype/src/pshinter/pshinter.c
	libs/freetype/src/psnames/psmodule.c
	libs/freetype/src/raster/raster.c
	libs/freetype/src/sfnt/sfnt.c
	libs/freetype/src/truetype/truetype.c
	libs/freetype/src/type1/type1.c
	libs/freetype/src/cid/type1cid.c
	libs/freetype/src/type42/type42.c
	libs/freetype/src/winfonts/winfnt.c)
	
	
set(SOUND_INCLUDES
	sound/snd_local.h
	sound/sound.h
	sound/SoundVoice.h
	sound/WaveFile.h)
	
set(SOUND_SOURCES
	#sound/snd_cache.cpp
	#sound/snd_decoder.cpp
	#sound/snd_efxfile.cpp
	sound/snd_emitter.cpp
	sound/snd_shader.cpp
	sound/snd_system.cpp
	sound/snd_world.cpp
	sound/SoundVoice.cpp
	sound/WaveFile.cpp
	)
	
set(XAUDIO2_INCLUDES
	sound/XAudio2/XA2_SoundHardware.h
	sound/XAudio2/XA2_SoundSample.h
	sound/XAudio2/XA2_SoundVoice.h)
	
set(XAUDIO2_SOURCES
	sound/XAudio2/XA2_SoundHardware.cpp
	sound/XAudio2/XA2_SoundSample.cpp
	sound/XAudio2/XA2_SoundVoice.cpp)

set(STUBAUDIO_INCLUDES
	sound/stub/SoundStub.h)

set(STUBAUDIO_SOURCES
	sound/stub/SoundSample.cpp)
	
set(OGGVORBIS_INCLUDES
	libs/oggvorbis/ogg/ogg.h
	libs/oggvorbis/ogg/os_types.h)
	
set(OGGVORBIS_SOURCES
	libs/oggvorbis/oggsrc/bitwise.c
	libs/oggvorbis/oggsrc/framing.c
	
	libs/oggvorbis/vorbissrc/mdct.c
	libs/oggvorbis/vorbissrc/smallft.c
	libs/oggvorbis/vorbissrc/block.c
	libs/oggvorbis/vorbissrc/envelope.c
	libs/oggvorbis/vorbissrc/windowvb.c
	libs/oggvorbis/vorbissrc/lsp.c
	libs/oggvorbis/vorbissrc/lpc.c
	libs/oggvorbis/vorbissrc/analysis.c
	libs/oggvorbis/vorbissrc/synthesis.c
	libs/oggvorbis/vorbissrc/psy.c
	libs/oggvorbis/vorbissrc/info.c
	libs/oggvorbis/vorbissrc/floor1.c
	libs/oggvorbis/vorbissrc/floor0.c
	libs/oggvorbis/vorbissrc/res0.c
	libs/oggvorbis/vorbissrc/mapping0.c
	libs/oggvorbis/vorbissrc/registry.c
	libs/oggvorbis/vorbissrc/codebook.c
	libs/oggvorbis/vorbissrc/sharedbook.c
	libs/oggvorbis/vorbissrc/lookup.c
	libs/oggvorbis/vorbissrc/bitrate.c
	libs/oggvorbis/vorbissrc/vorbisfile.c)

file(GLOB SYS_INCLUDES sys/*.h)
file(GLOB SYS_SOURCES sys/*.cpp)

file(GLOB UI_INCLUDES ui/*.h)
file(GLOB UI_SOURCES ui/*.cpp)

file(GLOB SWF_INCLUDES swf/*.h)
file(GLOB SWF_SOURCES swf/*.cpp)


set(GAMED3XP_INCLUDES
	d3xp/Achievements.h
	d3xp/Actor.h
	d3xp/AF.h
	d3xp/AFEntity.h
	d3xp/AimAssist.h
	d3xp/BrittleFracture.h
	d3xp/Camera.h
	#d3xp/EndLevel.h
	d3xp/Entity.h
	d3xp/Fx.h
	d3xp/Game.h
	d3xp/GameEdit.h
	d3xp/Game_local.h
	d3xp/Grabber.h
	d3xp/IK.h
	d3xp/Item.h
	d3xp/Leaderboards.h
	d3xp/Light.h
	d3xp/Misc.h
	d3xp/Moveable.h
	d3xp/Mover.h
	d3xp/MultiplayerGame.h
	d3xp/Player.h
	d3xp/PlayerIcon.h
	d3xp/PlayerView.h
	d3xp/PredictedValue.h
	d3xp/Projectile.h
	d3xp/Pvs.h
	d3xp/SecurityCamera.h
	d3xp/SmokeParticles.h
	d3xp/Sound.h
	d3xp/Target.h
	d3xp/Trigger.h
	d3xp/Weapon.h
	d3xp/WorldSpawn.h)
	
set(GAMED3XP_SOURCES
	d3xp/Achievements.cpp
	d3xp/Actor.cpp
	d3xp/AF.cpp
	d3xp/AFEntity.cpp
	d3xp/AimAssist.cpp
	d3xp/BrittleFracture.cpp
	d3xp/Camera.cpp
	#d3xp/EndLevel.cpp
	d3xp/Entity.cpp
	d3xp/Fx.cpp
	d3xp/GameEdit.cpp
	d3xp/Game_local.cpp
	d3xp/Game_network.cpp
	d3xp/Grabber.cpp
	d3xp/IK.cpp
	d3xp/Item.cpp
	d3xp/Leaderboards.cpp
	d3xp/Light.cpp
	d3xp/Misc.cpp
	d3xp/Moveable.cpp
	d3xp/Mover.cpp
	d3xp/MultiplayerGame.cpp
	d3xp/Player.cpp
	d3xp/PlayerIcon.cpp
	d3xp/PlayerView.cpp
	d3xp/precompiled.cpp
	d3xp/Projectile.cpp
	d3xp/Pvs.cpp
	d3xp/SecurityCamera.cpp
	d3xp/SmokeParticles.cpp
	d3xp/Sound.cpp
	d3xp/Target.cpp
	d3xp/Trigger.cpp
	d3xp/Weapon.cpp
	d3xp/WorldSpawn.cpp)

file(GLOB GAMED3XP_AI_INCLUDES d3xp/ai/*.h)
file(GLOB GAMED3XP_AI_SOURCES d3xp/ai/*.cpp)

file(GLOB GAMED3XP_ANIM_INCLUDES d3xp/anim/*.h)
file(GLOB GAMED3XP_ANIM_SOURCES d3xp/anim/*.cpp)

file(GLOB GAMED3XP_GAMESYS_INCLUDES d3xp/gamesys/*.h)
set(GAMED3XP_GAMESYS_SOURCES
	#d3xp/gamesys/Callbacks.cpp
	d3xp/gamesys/Class.cpp
	#d3xp/gamesys/DebugGraph.cpp
	d3xp/gamesys/Event.cpp
	d3xp/gamesys/SaveGame.cpp
	d3xp/gamesys/SysCmds.cpp
	d3xp/gamesys/SysCvar.cpp
	#d3xp/gamesys/TypeInfo.cpp
	)

file(GLOB GAMED3XP_MENUS_INCLUDES d3xp/menus/*.h)
file(GLOB GAMED3XP_MENUS_SOURCES d3xp/menus/*.cpp)
	
file(GLOB GAMED3XP_PHYSICS_INCLUDES d3xp/physics/*.h)
file(GLOB GAMED3XP_PHYSICS_SOURCES d3xp/physics/*.cpp)

file(GLOB GAMED3XP_SCRIPT_INCLUDES d3xp/script/*.h)
file(GLOB GAMED3XP_SCRIPT_SOURCES d3xp/script/*.cpp)


file(GLOB COMPILER_INCLUDES tools/compilers/*.h)

file(GLOB COMPILER_AAS_INCLUDES tools/compilers/aas/*.h)
file(GLOB COMPILER_AAS_SOURCES tools/compilers/aas/*.cpp)

file(GLOB COMPILER_DMAP_INCLUDES tools/compilers/dmap/*.h)
file(GLOB COMPILER_DMAP_SOURCES tools/compilers/dmap/*.cpp)

file(GLOB COMPILER_RENDERBUMP_INCLUDES tools/compilers/renderbump/*.h)
file(GLOB COMPILER_RENDERBUMP_SOURCES tools/compilers/renderbump/*.cpp)

file(GLOB COMPILER_ROQVQ_INCLUDES tools/compilers/roqvq/*.h)
file(GLOB COMPILER_ROQVQ_SOURCES tools/compilers/roqvq/*.cpp)

set(TOOLS_INCLUDES tools/edit_public.h)

file(GLOB TOOLS_COMAFX_INCLUDES tools/comafx/*.h)
file(GLOB TOOLS_COMAFX_SOURCES tools/comafx/*.cpp)

set(TOOLS_COMMON_INCLUDES
	tools/common/ColorButton.h
	tools/common/DialogHelpers.h
	tools/common/MaskEdit.h
	tools/common/OpenFileDialog.h
	tools/common/PropertyGrid.h
	tools/common/RegistryOptions.h
	#tools/common/RenderBumpFlatDialog.h
	tools/common/RollupPanel.h
	tools/common/SpinButton.h)

set(TOOLS_COMMON_SOURCES
	tools/common/AlphaPopup.cpp
	tools/common/ColorButton.cpp
	tools/common/MaskEdit.cpp
	tools/common/OpenFileDialog.cpp
	tools/common/PropertyGrid.cpp
	tools/common/RegistryOptions.cpp
	#tools/common/RenderBumpFlatDialog.cpp
	tools/common/RollupPanel.cpp
	tools/common/SpinButton.cpp)

file(GLOB TOOLS_COMMON_PROPTREE_INCLUDES tools/common/PropTree/*.h)
file(GLOB TOOLS_COMMON_PROPTREE_SOURCES tools/common/PropTree/*.cpp)

file(GLOB EDITOR_AF_INCLUDES tools/af/*.h)
file(GLOB EDITOR_AF_SOURCES tools/af/*.cpp)

set(EDITOR_DECL_INCLUDES
	tools/decl/DialogDeclBrowser.h
	tools/decl/DialogDeclEditor.h
	tools/decl/DialogDeclNew.h)
	
set(EDITOR_DECL_SOURCES
	tools/decl/DialogDeclBrowser.cpp
	tools/decl/DialogDeclEditor.cpp
	tools/decl/DialogDeclNew.cpp)
	
file(GLOB EDITOR_DEBUGGER_INCLUDES tools/debugger/*.h)
file(GLOB EDITOR_DEBUGGER_SOURCES tools/debugger/*.cpp)

file(GLOB EDITOR_GUIED_INCLUDES tools/guied/*.h)
set(EDITOR_GUIED_SOURCES
	tools/guied/GEApp.cpp
	tools/guied/GECheckInDlg.cpp
	tools/guied/GEDeleteModifier.cpp
	tools/guied/GEHideModifier.cpp
	tools/guied/GEInsertModifier.cpp
	tools/guied/GEItemPropsDlg.cpp
	tools/guied/GEItemScriptsDlg.cpp
	tools/guied/GEKeyValueModifier.cpp
	tools/guied/GEModifier.cpp
	tools/guied/GEModifierGroup.cpp
	tools/guied/GEModifierStack.cpp
	tools/guied/GEMoveModifier.cpp
	tools/guied/GENavigator.cpp
	tools/guied/GEOptions.cpp
	tools/guied/GEOptionsDlg.cpp
	tools/guied/GEProperties.cpp
	tools/guied/GEPropertyPage.cpp
	tools/guied/GESelectionMgr.cpp
	tools/guied/GESizeModifier.cpp
	tools/guied/GEStateModifier.cpp
	tools/guied/GEStatusBar.cpp
	tools/guied/GETransformer.cpp
	tools/guied/GEViewer.cpp
	tools/guied/GEWindowWrapper.cpp
	#tools/guied/GEWindowWrapper_stub.cpp
	tools/guied/GEWorkspace.cpp
	tools/guied/GEWorkspaceFile.cpp
	tools/guied/GEZOrderModifier.cpp
	tools/guied/guied.cpp)

file(GLOB EDITOR_MATERIALEDITOR_INCLUDES tools/materialeditor/*.h)
file(GLOB EDITOR_MATERIALEDITOR_SOURCES tools/materialeditor/*.cpp)

file(GLOB EDITOR_PARTICLE_INCLUDES tools/particle/*.h)
file(GLOB EDITOR_PARTICLE_SOURCES tools/particle/*.cpp)

file(GLOB EDITOR_PDA_INCLUDES tools/pda/*.h)
file(GLOB EDITOR_PDA_SOURCES tools/pda/*.cpp)

file(GLOB EDITOR_RADIANT_INCLUDES tools/radiant/*.h)
file(GLOB EDITOR_RADIANT_SOURCES tools/radiant/*.cpp)

file(GLOB EDITOR_SCRIPT_INCLUDES tools/script/*.h)
file(GLOB EDITOR_SCRIPT_SOURCES tools/script/*.cpp)

file(GLOB EDITOR_SOUND_INCLUDES tools/sound/*.h)
file(GLOB EDITOR_SOUND_SOURCES tools/sound/*.cpp)
	

file(GLOB DOOMCLASSIC_INCLUDES ../doomclassic/doom/*.h)
file(GLOB DOOMCLASSIC_SOURCES ../doomclassic/doom/*.cpp)

list(REMOVE_ITEM DOOMCLASSIC_SOURCES ../doomclassic/doom/i_net_ps3.cpp)
list(REMOVE_ITEM DOOMCLASSIC_SOURCES ../doomclassic/doom/i_net_xbox.cpp)
list(REMOVE_ITEM DOOMCLASSIC_SOURCES ../doomclassic/doom/i_video_ps3.cpp)
list(REMOVE_ITEM DOOMCLASSIC_SOURCES ../doomclassic/doom/i_video_xbox.cpp)

file(GLOB TIMIDITY_INCLUDES libs/timidity/*.h)
file(GLOB TIMIDITY_SOURCES libs/timidity/*.cpp)

file(GLOB_RECURSE WIN32_INCLUDES sys/win32/*.h)
#file(GLOB_RECURSE WIN32_SOURCES sys/win32/*.cpp)

set(WIN32_SOURCES
	sys/win32/win_achievements.cpp
	sys/win32/win_glimp.cpp
	sys/win32/win_input.cpp
	sys/win32/win_localuser.cpp
	sys/win32/win_main.cpp
	sys/win32/win_net.cpp
	sys/win32/win_qgl.cpp
	sys/win32/win_savegame.cpp
	sys/win32/win_session_local.cpp
	sys/win32/win_shared.cpp
	sys/win32/win_signin.cpp
	sys/win32/win_snd.cpp
	sys/win32/win_syscon.cpp
	sys/win32/win_taskkeyhook.cpp
	sys/win32/win_wndproc.cpp)
	
if(MSVC)
	list(APPEND WIN32_SOURCES sys/win32/win_cpu.cpp)
endif()

set(WIN32_RESOURCES
	# sys/win32/rc/res/BEVEL.BMP
	# sys/win32/rc/res/BITMAP2.BMP
	# sys/win32/rc/res/BMP00001.BMP
	# sys/win32/rc/res/BMP0002.BMP
	# sys/win32/rc/res/DEFTEX.WAL
	# sys/win32/rc/res/ENDCAP.BMP
	# sys/win32/rc/res/GetString.htm
	# sys/win32/rc/res/IBEVEL.BMP
	# sys/win32/rc/res/IENDCAP.BMP
	# sys/win32/rc/res/MEFileToolbar.bmp
	# sys/win32/rc/res/MEtoolbar.bmp
	# sys/win32/rc/res/MaterialEditor.ico
	# sys/win32/rc/res/PropTree.rc2
	# sys/win32/rc/res/Q.BMP
	# sys/win32/rc/res/RADIANT3.GIF
	# sys/win32/rc/res/Radiant.ico
	# sys/win32/rc/res/RadiantDoc.ico
	# sys/win32/rc/res/TOOLBAR1.BMP
	# sys/win32/rc/res/TOOLBAR2.BMP
	# sys/win32/rc/res/Toolbar.bmp
	# sys/win32/rc/res/VIEWDEFA.BMP
	# sys/win32/rc/res/VIEWOPPO.BMP
	# sys/win32/rc/res/bmp00002.bmp
	# sys/win32/rc/res/bmp00003.bmp
	# sys/win32/rc/res/bmp00004.bmp
	# sys/win32/rc/res/bmp00005.bmp
	# sys/win32/rc/res/cchsb.bmp
	# sys/win32/rc/res/ccrgb.bmp
	# sys/win32/rc/res/dbg_back.bmp
	# sys/win32/rc/res/dbg_breakpoint.ico
	# sys/win32/rc/res/dbg_current.ico
	# sys/win32/rc/res/dbg_currentline.ico
	# sys/win32/rc/res/dbg_empty.ico
	# sys/win32/rc/res/dbg_open.bmp
	# sys/win32/rc/res/dbg_toolbar.bmp
	sys/win32/rc/res/doom.ico
	# sys/win32/rc/res/fpoint.cur
	# sys/win32/rc/res/fxed_link.ico
	# sys/win32/rc/res/fxed_toolbar.bmp
	# sys/win32/rc/res/fxeditor.ico
	# sys/win32/rc/res/guied.ico
	# sys/win32/rc/res/guied_collapse.ico
	# sys/win32/rc/res/guied_expand.ico
	# sys/win32/rc/res/guied_hand.cur
	# sys/win32/rc/res/guied_nav_visible.ico
	# sys/win32/rc/res/guied_nav_visibledisabled.ico
	# sys/win32/rc/res/guied_scripts.ico
	# sys/win32/rc/res/guied_scripts_white.ico
	# sys/win32/rc/res/guied_viewer_toolbar.bmp
	# sys/win32/rc/res/icon2.ico
	# sys/win32/rc/res/logo_sm3dfx.bmp
	# sys/win32/rc/res/matedtree.bmp
	# sys/win32/rc/res/me_disabled_icon.ico
	# sys/win32/rc/res/me_enabled.ico
	# sys/win32/rc/res/me_off_icon.ico
	# sys/win32/rc/res/me_on_icon.ico
	# sys/win32/rc/res/qe3.ico
	# sys/win32/rc/res/shaderbar.bmp
	# sys/win32/rc/res/shaderdoc.ico
	# sys/win32/rc/res/shaderframe.ico
	# sys/win32/rc/res/spliter.cur
	)



#if(USE_MFC_TOOLS)
	list(APPEND WIN32_RESOURCES sys/win32/rc/doom.rc)
#else()
#	list(APPEND WIN32_RESOURCES sys/win32/rc/doom_nomfc.rc)
#endif()


file(GLOB POSIX_INCLUDES sys/posix/*.h)
file(GLOB POSIX_SOURCES sys/posix/*.cpp)

file(GLOB SDL_INCLUDES sys/sdl/*.h)
file(GLOB SDL_SOURCES sys/sdl/*.cpp)


source_group("aas" FILES ${AAS_INCLUDES})
source_group("aas" FILES ${AAS_SOURCES})

source_group("cm" FILES ${CM_INCLUDES})
source_group("cm" FILES ${CM_SOURCES})

source_group("framework" FILES ${FRAMEWORK_INCLUDES})
source_group("framework" FILES ${FRAMEWORK_SOURCES})

source_group("framework\\async" FILES ${FRAMEWORK_ASYNC_INCLUDES})
source_group("framework\\async" FILES ${FRAMEWORK_ASYNC_SOURCES})

source_group("renderer" FILES ${RENDERER_INCLUDES})
source_group("renderer" FILES ${RENDERER_SOURCES})

source_group("libs\\irrxml" FILES ${IRRXML_INCLUDES})
source_group("libs\\irrxml" FILES ${IRRXML_SOURCES})

source_group("libs\\jpeg-6" FILES ${JPEG_INCLUDES})
source_group("libs\\jpeg-6" FILES ${JPEG_SOURCES})

source_group("libs\\png" FILES ${PNG_INCLUDES})
source_group("libs\\png" FILES ${PNG_SOURCES})

source_group("libs\\zlib" FILES ${ZLIB_INCLUDES})
source_group("libs\\zlib" FILES ${ZLIB_SOURCES})

source_group("libs\\glew" FILES ${GLEW_INCLUDES})
source_group("libs\\glew" FILES ${GLEW_SOURCES})

source_group("libs\\freetype" FILES ${FREETYPE_SOURCES})

source_group("sound" FILES ${SOUND_INCLUDES})
source_group("sound" FILES ${SOUND_SOURCES})

source_group("sound\\XAudio2" FILES ${XAUDIO2_INCLUDES})
source_group("sound\\XAudio2" FILES ${XAUDIO2_SOURCES})

source_group("sound\\stub" FILES ${STUBAUDIO_INCLUDES})
source_group("sound\\stub" FILES ${STUBAUDIO_SOURCES})

source_group("libs\\oggvorbis" FILES ${OGGVORBIS_INCLUDES})
source_group("libs\\oggvorbis" FILES ${OGGVORBIS_SOURCES})

source_group("ui" FILES ${UI_INCLUDES})
source_group("ui" FILES ${UI_SOURCES})

source_group("swf" FILES ${SWF_INCLUDES})
source_group("swf" FILES ${SWF_SOURCES})

source_group("sys" FILES ${SYS_INCLUDES})
source_group("sys" FILES ${SYS_SOURCES})

source_group("doomclassic" FILES ${DOOMCLASSIC_INCLUDES})
source_group("doomclassic" FILES ${DOOMCLASSIC_SOURCES})

source_group("libs\\timidity" FILES ${TIMIDITY_INCLUDES})
source_group("libs\\timidity" FILES ${TIMIDITY_SOURCES})


source_group("game-d3xp" FILES ${GAMED3XP_INCLUDES})
source_group("game-d3xp" FILES ${GAMED3XP_SOURCES})

source_group("game-d3xp\\ai" FILES ${GAMED3XP_AI_INCLUDES})
source_group("game-d3xp\\ai" FILES ${GAMED3XP_AI_SOURCES})

source_group("game-d3xp\\anim" FILES ${GAMED3XP_ANIM_INCLUDES})
source_group("game-d3xp\\anim" FILES ${GAMED3XP_ANIM_SOURCES})

source_group("game-d3xp\\gamesys" FILES ${GAMED3XP_GAMESYS_INCLUDES})
source_group("game-d3xp\\gamesys" FILES ${GAMED3XP_GAMESYS_SOURCES})

source_group("game-d3xp\\menus" FILES ${GAMED3XP_MENUS_INCLUDES})
source_group("game-d3xp\\menus" FILES ${GAMED3XP_MENUS_SOURCES})

source_group("game-d3xp\\physics" FILES ${GAMED3XP_PHYSICS_INCLUDES})
source_group("game-d3xp\\physics" FILES ${GAMED3XP_PHYSICS_SOURCES})

source_group("game-d3xp\\script" FILES ${GAMED3XP_SCRIPT_INCLUDES})
source_group("game-d3xp\\script" FILES ${GAMED3XP_SCRIPT_SOURCES})




source_group("sys\\win32" FILES ${WIN32_INCLUDES})
source_group("sys\\win32" FILES ${WIN32_SOURCES})
source_group("sys\\win32\\Resources" FILES ${WIN32_RESOURCES})

source_group("sys\\posix" FILES ${POSIX_INCLUDES})
source_group("sys\\posix" FILES ${POSIX_SOURCES})

source_group("sys\\sdl" FILES ${SDL_INCLUDES})
source_group("sys\\sdl" FILES ${SDL_SOURCES})


source_group("tools\\compilers" FILES ${COMPILER_INCLUDES})

source_group("tools\\compilers\\aas" FILES ${COMPILER_AAS_INCLUDES})
source_group("tools\\compilers\\aas" FILES ${COMPILER_AAS_SOURCES})

source_group("tools\\compilers\\dmap" FILES ${COMPILER_DMAP_INCLUDES})
source_group("tools\\compilers\\dmap" FILES ${COMPILER_DMAP_SOURCES})

source_group("tools\\compilers\\renderbump" FILES ${COMPILER_RENDERBUMP_INCLUDES})
source_group("tools\\compilers\\renderbump" FILES ${COMPILER_RENDERBUMP_SOURCES})

source_group("tools\\compilers\\roqvq" FILES ${COMPILER_ROQVQ_INCLUDES})
source_group("tools\\compilers\\roqvq" FILES ${COMPILER_ROQVQ_SOURCES})

source_group("tools" FILES ${TOOLS_INCLUDES})

source_group("tools\\comafx" FILES ${TOOLS_COMAFX_INCLUDES})
source_group("tools\\comafx" FILES ${TOOLS_COMAFX_SOURCES})

source_group("tools\\common" FILES ${TOOLS_COMMON_INCLUDES})
source_group("tools\\common" FILES ${TOOLS_COMMON_SOURCES})

source_group("tools\\common\\proptree" FILES ${TOOLS_COMMON_PROPTREE_INCLUDES})
source_group("tools\\common\\proptree" FILES ${TOOLS_COMMON_PROPTREE_SOURCES})

source_group("tools\\af" FILES ${EDITOR_AF_INCLUDES})
source_group("tools\\af" FILES ${EDITOR_AF_SOURCES})

source_group("tools\\debugger" FILES ${EDITOR_DEBUGGER_INCLUDES})
source_group("tools\\debugger" FILES ${EDITOR_DEBUGGER_SOURCES})

source_group("tools\\decl" FILES ${EDITOR_DECL_INCLUDES})
source_group("tools\\decl" FILES ${EDITOR_DECL_SOURCES})

source_group("tools\\guied" FILES ${EDITOR_GUIED_INCLUDES})
source_group("tools\\guied" FILES ${EDITOR_GUIED_SOURCES})

source_group("tools\\materialeditor" FILES ${EDITOR_MATERIALEDITOR_INCLUDES})
source_group("tools\\materialeditor" FILES ${EDITOR_MATERIALEDITOR_SOURCES})

source_group("tools\\particle" FILES ${EDITOR_PARTICLE_INCLUDES})
source_group("tools\\particle" FILES ${EDITOR_PARTICLE_SOURCES})

source_group("tools\\pda" FILES ${EDITOR_PDA_INCLUDES})
source_group("tools\\pda" FILES ${EDITOR_PDA_SOURCES})

source_group("tools\\radiant" FILES ${EDITOR_RADIANT_INCLUDES})
source_group("tools\\radiant" FILES ${EDITOR_RADIANT_SOURCES})

source_group("tools\\script" FILES ${EDITOR_SCRIPT_INCLUDES})
source_group("tools\\script" FILES ${EDITOR_SCRIPT_SOURCES})

source_group("tools\\sound" FILES ${EDITOR_SOUND_INCLUDES})
source_group("tools\\sound" FILES ${EDITOR_SOUND_SOURCES})


set(RBDOOM3_SOURCES
			${AAS_INCLUDES} ${AAS_SOURCES}
			${CM_INCLUDES} ${CM_SOURCES}
			${FRAMEWORK_INCLUDES} ${FRAMEWORK_SOURCES}
			${FRAMEWORK_ASYNC_INCLUDES} ${FRAMEWORK_ASYNC_SOURCES} 
			${RENDERER_INCLUDES} ${RENDERER_SOURCES}
			#${IRRXML_INCLUDES} ${IRRXML_SOURCES}
			${JPEG_INCLUDES} ${JPEG_SOURCES}
			#${PNG_INCLUDES} ${PNG_SOURCES}
			${ZLIB_INCLUDES} ${ZLIB_SOURCES}
			#${GLEW_INCLUDES} ${GLEW_SOURCES}
			#${FREETYPE_SOURCES}
			${SOUND_INCLUDES} ${SOUND_SOURCES}
			#${OGGVORBIS_INCLUDES} ${OGGVORBIS_SOURCES}
			${UI_INCLUDES} ${UI_SOURCES}
			${SWF_INCLUDES} ${SWF_SOURCES}
			
			#${COMPILER_INCLUDES}
			#${COMPILER_AAS_INCLUDES} ${COMPILER_AAS_SOURCES}
			#${COMPILER_DMAP_INCLUDES} ${COMPILER_DMAP_SOURCES}
			#${COMPILER_RENDERBUMP_INCLUDES} ${COMPILER_RENDERBUMP_SOURCES}
			#${COMPILER_ROQVQ_INCLUDES} ${COMPILER_ROQVQ_SOURCES}
			)

add_definitions(-D__DOOM__
				#-DGLEW_STATIC
				#-DBUILD_FREETYPE
				#-DFT2_BUILD_LIBRARY
				)
				
if(MONOLITH)
	list(APPEND RBDOOM3_SOURCES
		${GAMED3XP_INCLUDES} ${GAMED3XP_SOURCES}
		${GAMED3XP_AI_INCLUDES} ${GAMED3XP_AI_SOURCES}
		${GAMED3XP_ANIM_INCLUDES} ${GAMED3XP_ANIM_SOURCES}
		${GAMED3XP_GAMESYS_INCLUDES} ${GAMED3XP_GAMESYS_SOURCES}
		${GAMED3XP_MENUS_INCLUDES} ${GAMED3XP_MENUS_SOURCES}
		${GAMED3XP_PHYSICS_INCLUDES} ${GAMED3XP_PHYSICS_SOURCES}
		${GAMED3XP_SCRIPT_INCLUDES} ${GAMED3XP_SCRIPT_SOURCES})
else()
	add_definitions(-D__DOOM_DLL__)
endif()

include_directories(
			.
			#libs/curl/include
			#libs/openal/include
			#libs/glew/include
			#libs/freetype/include
			libs/zlib)
			
#link_directories(
#			${CMAKE_CURRENT_SOURCE_DIR}/libs/curl/lib
#			${CMAKE_CURRENT_SOURCE_DIR}/libs/openal/lib)

if(MSVC)

	add_definitions(-DUSE_DOOMCLASSIC)

	if(MSVC_VERSION EQUAL 1700)
		set(DirectX_LIBRARIES
			dinput8
			dsound
			dxguid
			#dxerr
			Xinput
			xaudio2.lib
			)
		add_definitions(-DUSE_XINPUT)
	else()
		find_package(DirectX REQUIRED)
		include_directories(${DirectX_INCLUDE_DIR})
		add_definitions(-DUSE_XINPUT)
	endif()
	
	if(USE_MFC_TOOLS)
		
		include_directories(libs/atlmfc/include)
		if(CMAKE_CL_64)
			link_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/atlmfc/lib/amd64)
		else(CMAKE_CL_64)
			link_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/atlmfc/lib)
		endif(CMAKE_CL_64)
		
		add_definitions(-DUSE_MFC_TOOLS)
		
		list(APPEND RBDOOM3_SOURCES
			${TOOLS_INCLUDES}
			${TOOLS_COMAFX_INCLUDES} ${TOOLS_COMAFX_SOURCES}
			${TOOLS_COMMON_INCLUDES} ${TOOLS_COMMON_SOURCES}
			${TOOLS_COMMON_PROPTREE_INCLUDES} ${TOOLS_COMMON_PROPTREE_SOURCES}
			
			${EDITOR_AF_INCLUDES} ${EDITOR_AF_SOURCES}
			${EDITOR_DEBUGGER_INCLUDES} ${EDITOR_DEBUGGER_SOURCES}
			${EDITOR_DECL_INCLUDES} ${EDITOR_DECL_SOURCES}
			${EDITOR_GUIED_INCLUDES} ${EDITOR_GUIED_SOURCES}
			${EDITOR_MATERIALEDITOR_INCLUDES} ${EDITOR_MATERIALEDITOR_SOURCES}
			${EDITOR_PARTICLE_INCLUDES} ${EDITOR_PARTICLE_SOURCES}
			${EDITOR_PDA_INCLUDES} ${EDITOR_PDA_SOURCES}
			${EDITOR_RADIANT_INCLUDES} ${EDITOR_RADIANT_SOURCES}
			${EDITOR_SCRIPT_INCLUDES} ${EDITOR_SCRIPT_SOURCES}
			${EDITOR_SOUND_INCLUDES} ${EDITOR_SOUND_SOURCES})
	endif()
	
	list(APPEND RBDOOM3_SOURCES
		${DOOMCLASSIC_INCLUDES} ${DOOMCLASSIC_SOURCES}
		${TIMIDITY_INCLUDES} ${TIMIDITY_SOURCES}
		
		${SYS_INCLUDES} ${SYS_SOURCES}
		${XAUDIO2_INCLUDES} ${XAUDIO2_SOURCES}
		${WIN32_INCLUDES} ${WIN32_SOURCES} ${WIN32_RESOURCES})
		

	list(REMOVE_DUPLICATES RBDOOM3_SOURCES)
	
	add_executable(RBDoom3 WIN32 ${RBDOOM3_SOURCES})
		
	#add_custom_command(TARGET RBDoom3
    #               PRE_BUILD
	#			   COMMAND nmake ARGS /f Makefile.vc6 CFG=release
	#			   WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/libs/curl/lib
    #               COMMENT "Compiling libcURL")
		
	add_dependencies(RBDoom3 idlib)
	target_link_libraries(RBDoom3
		idlib
		${DirectX_LIBRARIES}
		#libcurl 
		#openal32 
		opengl32
		glu32
		dbghelp
		#eaxguid
		iphlpapi
		winmm
		wsock32.lib
		)
else()

	list(APPEND RBDOOM3_SOURCES
		${SYS_INCLUDES} ${SYS_SOURCES}
		${STUBAUDIO_INCLUDES} ${STUBAUDIO_SOURCES})
	
	if(WIN32)
		add_definitions(-DUSE_DOOMCLASSIC)
	
		list(APPEND RBDOOM3_SOURCES
			${DOOMCLASSIC_INCLUDES} ${DOOMCLASSIC_SOURCES}
			${TIMIDITY_INCLUDES} ${TIMIDITY_SOURCES}
			${WIN32_INCLUDES} ${WIN32_SOURCES} ${WIN32_RESOURCES})  
	
		#list(REMOVE_ITEM WIN32_SOURCES sys/win32/win_cpu.cpp)
		list(APPEND WIN32_SOURCES sys/sdl/sdl_cpu.cpp)
	endif()
	
	if(UNIX)
	
		find_package(SDL REQUIRED)
		include_directories(${SDL_INCLUDE_DIR})
		set(SDLx_LIBRARY ${SDL_LIBRARY})
	
		list(APPEND RBDOOM3_SOURCES
			${POSIX_INCLUDES} ${POSIX_SOURCES}
			${SDL_INCLUDES} ${SDL_SOURCES}
			sys/linux/linux_main.cpp)
	endif()

	list(REMOVE_DUPLICATES RBDOOM3_SOURCES)
	
	add_executable(RBDoom3 WIN32 ${RBDOOM3_SOURCES})

	add_dependencies(RBDoom3 idlib)
	
	if(MINGW)
		include_directories(libs/sdl2/include)
		
		link_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/sdl2/libmingw32)
	endif()
	
	if(WIN32)
	
		target_link_libraries(RBDoom3
			idlib
			${DirectX_LIBRARIES}
			#libcurl 
			#openal32 
			opengl32
			glu32
			dbghelp
			#eaxguid
			iphlpapi
			winmm
			wsock32.lib
			SDL2
			)
	  
	endif()
	
	if(UNIX)	
		target_link_libraries(RBDoom3
			idlib
			GL
			pthread
			${SDLx_LIBRARY}
			)
	endif()
	
endif()
