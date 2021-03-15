```
    ____   ____   ____                           _____  ____   ______ ______
   / __ \ / __ ) / __ \ ____   ____   ____ ___  |__  / / __ ) / ____// ____/
  / /_/ // __  |/ / / // __ \ / __ \ / __ `__ \  /_ < / __  |/ /_   / / __  
 / _, _// /_/ // /_/ // /_/ // /_/ // / / / / /___/ // /_/ // __/  / /_/ /  
/_/ |_|/_____//_____/ \____/ \____//_/ /_/ /_//____//_____//_/     \____/   
_______________________________________________________________________
```

RBDOOM-3-BFG Readme - https://github.com/RobertBeckebans/RBDOOM-3-BFG

<img src="https://i.imgur.com/jEmcriO.png">


# Table of Contents

This file contains the following sections:

1. [About the Port](#about)
2. [Renderer Features Explained](#render)
3. [".plan"](#plan)
4. [May or may not ".plan"](#plan2)
5. [General Notes](#notes)
6. [License](#license)
7. [Getting the Source Code ](#source)
8. [Compiling on Windows](#compile_windows)
9. [Compiling on Linux](#compile_linux)
10. [Installation, Getting the Game Data, Running the Game](#installation)
11. [New Console Variables](#console)
12. [Known Issues](#issues)
13. [Bug Reports](#reports)
14. [FAQ](#faq)
	



---
# About the Port <a name="about"></a>

`RBDOOM-3-BFG is a modernization effort of DOOM-3-BFG.`

RBDOOM-3-BFG is based on DOOM-3-BFG and the goal of this port is to bring DOOM-3-BFG up to latest technology in 2019 making it closer to Doom 2016 while still remaining a DOOM 3 port regarding the gameplay. 

I started this project in 2012 and focused on making this code being future proof so other cool projects can build interesting things on top of it without the need to fix a lot of stuff first. Over 40 people all over the world contributed cool patches. Some results are:

## Gaming Related
* Soft shadows using PCF hardware shadow mapping.
	The implementation uses sampler2DArrayShadow and PCF which usually
	requires Direct3D 10.1 however it is in the OpenGL 3.2 core so it should
	be widely supported.
	All 3 light types are supported which means parallel lights (sun) use
	scene independent cascaded shadow mapping.
	The implementation is very fast with single taps (400 fps average per
	scene on a GTX 660 ti OC) however I defaulted it to 12 taps using a Poisson disc algorithm so the shadows look
	really good which should give you stable 100 fps on todays hardware (2014).
* Changed light interaction shaders to use Half-Lambert lighting like in Half-Life 2 to 
	make the game less dark. https://developer.valvesoftware.com/wiki/Half_Lambert
* True internal 64 bit HDR lighting with filmic ACES tone mapping and gamma-correct rendering in linear RGB space
* Enhanced Subpixel Morphological Antialiasing as a cheap alternative for MSAA. For more information see "Anti-Aliasing Methods in CryENGINE 3" and the docs at http://www.iryoku.com/smaa/
* Filmic post process effects like Chromatic Aberration and Dithering
* Additional ambient render pass to make the game less dark similar to the Quake 4 r_forceAmbient technique
* Screen Space Ambient Occlusion http://graphics.cs.williams.edu/papers/SAOHPG12/
* Fixed Bink video playback through libbinkdec (thanks to Daniel Gibson) or FFmpeg (thanks to Carl Kenner)
* Cinematic sequences can be skipped (thanks to Biel Bestué de Luna)
* Netcode fixes to allow multiplayer sessions to friends with +connect <ip of friend> (manual port forwarding required)

## Programming and Code Quality
* Flexible build system using CMake allowing to add optional features like Vulkan rendering, FFmpeg for videos or OpenAL for sound
* Linux support for both GCC and Clang with a proper SDL 2 OS layer including gamepad support
* Win64 support and Visual Studio support up to VS 2019
* ~~MacOS X support~~ (might be outdated due to Apple's latest API restrictions)
* OpenAL Soft sound backend primarily developed for Linux but works on Windows as well
* Sourcecode cleanup using the Artistic Style C++ beautifier to ensure the Git diffs and logs are clean
* Fixed tons of warnings using the Clang compiler
* Fast compile times using precompiled header support

## Modding Support
* RBDOOM-3-BFG allows mod editing and has many tiny fixes so custom content can be put into mod directories and the engine accepts it like vanilla Doom 3. DOOM 3 BFG wasn't designed for actual development or modding support. Many things like reading anything outside of the packed resource files was not supported. I also fixed many things in the renderer like r_showTris.
* PNG image support
* Collada .DAE model support in addition to .ase and .lwo for static map models
* Wavefront OBJ model support
* Added back dmap and aas compilers (mapping tools, thanks to Pat Raynor)
* Added in-engine Flash debugging tools and new console variables.
  These tools help to analyse the id Tech view of Flash and what SWF tags are supported and how they are interpreted
  by id Tech's own ActionScript 2 interpreter
	- postLoadExportFlashAtlas
    - postLoadExportFlashToSWF
    - postLoadExportFlashToJSON
	- swf_show : Draws the bounding box of instanced Flash sprites in red and their names

* Added Steel Storm 2 Engine render demo fixes

* Merged LordHavoc's image compression progress bar which shows up in the map loading screen
  when loading and compressing new images from mods
* Added support for Mikkelsen tangent space standard for new assets (thanks to Stephen Pridham)
	
If you want to start mod from a directory, you should first specify your mod directory adding the following command to the launcher:

"+set fs_game modDirectoryName"

so it would end up looking like: RBDoom3BFG +set fs_game modDirectoryName

IMPORTANT: RBDOOM-3-BFG does not support old Doom 3 modifications that include sourcecode modifications in binary form (.dll)
You can fork RBDOOM-3-BFG and create a new renamed binary that includes all required C++ game code modifications. 

---
# Renderer Features Explained <a name="render"></a>

## HDR
Left: Wrong original Blinn-Phong in sRGB gamma space. Right: Gamma correct HDR rendering in  linear RBG + ACES Film Tonemapping Try it yourself, it is hard to see the effect in a single Screenshot.

<img src="https://i.imgur.com/z5GRIFa.png" width="384"> <img src="https://i.imgur.com/4WJGNzX.png" width="384">

<img src="https://i.imgur.com/6R42aoG.png" width="384"> <img src="https://i.imgur.com/A7VLpRM.png" width="384">

## Extra Ambient Pass

Left: No extra ambient pass. Ambient is pure black. Right: Extra ambient pass with r_forceAmbient 0.2 

<img src="https://i.imgur.com/JZZNmSn.png" width="384"> <img src="https://i.imgur.com/LMtVJ4m.png" width="384">

## Screen Space Ambient Occlusion
Left: No HDR and no SSAO. Right: r_useSSAO 1 darkens the corners of the scene and also removes too much ambient added by the extra ambient pass

<img src="https://i.imgur.com/Zz5OZGJ.png" width="384"> <img src="https://i.imgur.com/qUNH5th.png" width="384">

<img src="https://i.imgur.com/AP2tBVd.png" width="384"> <img src="https://i.imgur.com/dJ1dY4X.png" width="384">

## Filmic Post Processing
Left: No post processing except HDR tone mapping. Right: r_useFilmicPostProcessing 1 adds Technicolor color grading and film grain

<img src="https://i.imgur.com/MwUyLXt.png" width="384"> <img src="https://i.imgur.com/1nZPlNk.png" width="384">

---
# ".plan" <a name=".plan"></a>

* If you want to see what is planned or in progress in a Trello style manner look here: [RBDOOM-3-BFG projects](https://github.com/RobertBeckebans/RBDOOM-3-BFG/projects)
* ~~Fix GPU Skinning with Vulkan~~
* ~~Fix the lighting with stencil shadows with Vulkan~~
* ~~Finish adapting the Vulkan renderer backend based on Dustin Land's vkDOOM3~~
* ~~Use ImGui to show renderer stats like com_showFPS 1 and more detailed renderer stats~~
* ~~Get ImGui to run with Vulkan~~
* Visualize the Vulkan memory fragmentation based on AMD's VMA library with ImGui
* Port all the RBDOOM-3-BFG specific extras like HDR, Shadow Mapping and Post Processing to Vulkan
* ~~Finish PBR texture support~~
* Finish partly integrated IBL lighting and [Light probe interpolation using tetrahedral tessellations](https://gdcvault.com/play/1015312/Light-Probe-Interpolation-Using-Tetrahedral)
* Generate IBL probes on the GPU through compute shaders
* Add Screen Space Reflections
* Evaluate Blender 2.8 as a complete replacement for D3Radiant and update the scripts for it
* Add glTF 2.0 support for static models and animations because it is the new JPG of 3D ;)
* Improve SMAA with Temporal Anti-Aliasing (SMAA T2x)

---
# May or may not ".plan" <a name=".plan2"></a>
* Add a DX12 Ultimate renderer backend
* Add [Volumetric Lighting](http://www.alexandre-pestana.com/volumetric-lights/)
* Give Mara's and McGuire's DeepGBuffer Screen Space Global Illumination a second try with Christoph Schieds' A-SVGF realtime denoising because A-SVGF works really well in Q2RTX
* Port Virtual Texturing support from [IcedTech](https://github.com/jmarshall23/IcedTech)
* Slim texture loading routines based on stb_image
* Add texture compression based on [Basis Universal GPU Texture and Texture Video Compression Codec](https://github.com/binomialLLC/basis_universal)
* Scrap complex and complicated multipass forward shading with a simpler forward+ solution
* Rip & Tear renderer backend with modern approaches by [The-Forge](https://github.com/ConfettiFX/The-Forge)
* Replace collision detection and physics with PhysX 4.1

---
# General Notes <a name="notes"></a>

A short summary of the file layout:

Directory                       | Description
:-----------------------------  | :------------------------------------------------
RBDOOM-3-BFG/base/              | Doom 3 BFG media directory ( models, textures, sounds, maps, etc. )
RBDOOM-3-BFG/neo/               | RBDOOM-3-BFG source code ( renderer, game code for multiple games, OS layer, etc. )
RBDOOM-3-BFG/build/             | Build folder for CMake
RBDOOM-3-BFG/tools/blender/     | Blender scripts for level mapping
RBDOOM-3-BFG/tools/trenchbroom  | TrenchBroom level editor for mapping

This release does not contain any game data, the game data is still
covered by the original EULA and must be obeyed as usual.

You must patch the game to the latest version.

You can purchase Doom 3 BFG Edition from GOG (DRM Free):
https://www.gog.com/game/doom_3_bfg_edition

Or the game can be purchased from Steam (with DRM):
http://store.steampowered.com/app/208200/


## Steam
The Doom 3 BFG Edition GPL Source Code release does not include functionality for integrating with 
Steam.  This includes roaming profiles, achievements, leaderboards, matchmaking, the overlay, or
any other Steam features.

## Bink Video playback
The RBDOOM-3-BFG Edition GPL Source Code release includes functionality for rendering Bink Videos through FFmpeg or libbinkdec.

## Back End Rendering of Stencil Shadows
The RBOOM-3-BFG Edition GPL Source Code release includes functionality enabling rendering
of stencil shadows via the "depth fail" method, a functionality commonly known as "Carmack's Reverse".

This method was patented by Creative Labs and has finally expired on 2019-10-13.
(see https://patents.google.com/patent/US6384822B1/en for expiration status)

---
# License <a name="license"></a>
See docs/COPYING.txt for the GNU GENERAL PUBLIC LICENSE

ADDITIONAL TERMS:  The Doom 3 BFG Edition GPL Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU GPL which accompanied the Doom 3 BFG Edition GPL Source Code.  If not, please request a copy in writing from id Software at id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

See docs/CODE_LICENSE_EXCEPTIONS.md for all parts of the code The parts that are not covered by the GPL.

---
# Getting the Source Code <a name="source"></a>

This project's GitHub.net Git repository can be checked out through Git with the following instruction set: 

	> git clone --recursive https://github.com/RobertBeckebans/RBDOOM-3-BFG.git

The paramter --recursive is only required if you want to build RBDOOM-3-BFG with Vulkan support.

Existing repositories can be updated manually:

	> git submodule init
	> git submodule update




---
# Compiling on Windows <a name="compile_windows"></a>

1. Download and install the Visual Studio 2017 Community Edition.

2. Download the DirectX SDK (June 2010) here: (Only for Windows 7 builds)
	http://www.microsoft.com/en-us/download/details.aspx?id=6812

3. Download and install the latest CMake.

4. Generate the VS2017 projects using CMake by doubleclicking a matching configuration .bat file in the neo/ folder.

5. Use the VS2017 solution to compile what you need:
	RBDOOM-3-BFG/build/RBDoom3BFG.sln
	

## Optional if you want to use FFmpeg

6. Download ffmpeg-4.2.2-win64-shared.zip from ffmpeg.zeranoe.com/builds/win64/shared

7. Extract the FFmpeg DLLs to your current build directory under RBDOOM-3-BFG/build/


---
# Compiling on Linux <a name="compile_linux"></a>

1. You need the following dependencies in order to compile RBDoom3BFG with all features:

	On Debian or Ubuntu:

		> apt-get install cmake libsdl2-dev libopenal-dev libavcodec-dev libavformat-dev libavutil-dev libswscale-dev
	
	On Fedora
		
		> yum install cmake SDL-devel openal-devel ffmpeg-devel
	
	On ArchLinux 
	
		> sudo pacman -S sdl2 cmake openal ffmpeg

	On openSUSE
	
		> sudo zypper install cmake libSDL2-devel openal-soft-devel

	You don't need FFmpeg to be installed. You can also turn it off by add -DFFMPEG=OFF to the CMake options.
	It is enabled by default because the bundled libbinkdec is slow during development if compiled for Debug mode.

2. Generate the Makefiles using CMake:

	> cd neo/
	> ./cmake-eclipse-linux-profile.sh
	
3. Compile RBDOOM-3-BFG targets with

	> cd ../build
	> make

---
# Installation, Getting the Game Data, Running the Game <a name="installation"></a>

If you use the prebuilt `Win64` binaries then simply extract them to your
`C:\Program Files (x86)\Steam\SteamApps\common\Doom 3 BFG Edition\` directory and run RBDoom3BFG.exe.

---
## The following instructions are primarily intented for `Linux` users and all hackers on other operating systems.

To play the game, you need the game data from a legal copy of the game.

Currently this requires a Windows installer, whether that be the GOG installer or by using Steam for Windows.

Note: the original DVD release of Doom 3 BFG contains encrypted data that is decoded by Steam on install.

On Linux and OSX the easiest way to install is with SteamCMD: https://developer.valvesoftware.com/wiki/SteamCMD
See the description on https://developer.valvesoftware.com/wiki/SteamCMD#Linux (OS X is directly below that) on how to install SteamCMD on your system. You won't have to create a new user.

Then you can download Doom 3 BFG with

> ./steamcmd.sh +@sSteamCmdForcePlatformType windows +login <YOUR_STEAM_LOGIN_NAME> +force_install_dir ./doom3bfg/ +app_update 208200 validate +quit

(replace <YOUR_STEAM_LOGIN_NAME> with your steam login name)
When it's done you should have the normal windows installation of Doom 3 BFG in ./doom3bfg/ and the needed files in ./doom3bfg/base/
That number is the "AppID" of Doom 3 BFG; if you wanna use this to get the data of other games you own, you can look up the AppID at https://steamdb.info/

NOTE that we've previously recommended using download_depot in the Steam console to install the game data. That turned out to be unreliable and result in broken, unusable game data. So use SteamCMD instead, as described above.

Alternatively with the GOG installer, you can use Wine to install the game. See https://winehq.org/download for details on how to install wine for Linux and Mac.

Once Wine is installed and configured on your system install Doom 3 BFG edition using the downloaded installers from gog.com:

> wine setup_doom_3_bfg_1.14_\(13452\)_\(g\).exe

(there will be several .exe files from GOG, make sure all of them are in the same directory)

Once this is complete, by default you can find your Doom 3 BFG "base/" directory at ".wine/drive_c/GOG\ Games/DOOM\ 3\ BFG/base".

Note that you may want to add the following line to the bottom of the default.cfg in whatever "base/" directory you use:

> set sys_lang "english"

This will ensure the game and its menus are in english and don't default to something else. Alternatives include:

set sys_lang "english"
set sys_lang "french"
set sys_lang "german"
set sys_lang "italian"
set sys_lang "japanese"
set sys_lang "spanish"

Anyway:

1. Install Doom 3 BFG in Steam (Windows version) or SteamCMD, make sure it's getting
   updated/patched.

2. Create your own Doom 3 BFG directory, e.g. /path/to/Doom3BFG/

3. Copy the game-data's base dir from Steam or GOG to that directory
   (e.g. /path/to/Doom3BFG/), it's in
	/your/path/to/Steam/steamapps/common/DOOM 3 BFG Edition/base/
	or, if you used SteamCMD or GOG installer with Wine, in the path you used above.

4. Copy your RBDoom3BFG executable that you created in 5) or 6) and the FFmpeg DLLs to your own 
   Doom 3 BFG directory (/path/to/Doom3BFG).
   
   Your own Doom 3 BFG directory now should look like:
	/path/to/Doom3BFG/
	 ->	RBDoom3BFG (or RBDoom3BFG.exe on Windows)
	 -> avcodec-58.dll
	 -> avdevice-58.dll
	 -> avfilter-7.dll
	 -> avformat-58.dll
	 -> avutil-56.dll
	 -> postproc-55.dll
	 -> swresample-5.dll
	 -> swscale-5.dll
	 ->	base/
		 ->	classicmusic/
		 ->	_common.crc
		 ->	(etc)

5. Run the game by executing the RBDoom3BFG executable.

6. Enjoy

7. If you run into bugs, please report them, see [Bug Reports](#reports)

---
# New Console Variables and Commands <a name="console"></a>


## Gaming Related
Name                                   | Description
:--------------------------------------| :------------------------------------------------
r_antiAliasing                         | Different Anti-Aliasing modes
r_useShadowMapping [0 or 1]            | Use soft shadow mapping instead of hard stencil shadows
r_hdrAutoExposure [0 or 1]             | Adaptive tonemapping with HDR. This allows to have very bright or very dark scenes but the camera will adapt to it so the scene won't loose details
r_exposure [0 .. 1]                    | Default 0.5, Controls brightness and affects HDR exposure key. This is what you change in the video brightness options
r_useSSAO [0 .. 1]                     | Use Screen Space Ambient Occlusion to darken the corners in the scene
r_useFilmicPostProcessing [0, 1]       | Apply several post process effects to mimic a filmic look

## Modding Support
Name                              | Description
:--------------------------------------| :------------------------------------------------
exportScriptEvents                     | Command: Generates a new script/doom_events.script that reflects all registered class events in the idClass C++ system. The gamecode still needs to be extended to add the original comments of the events
exportFGD `[models]`                             | Command: Exports all entity defs to exported/_tb/*.fgd for usage in TrenchBroom
exportEntityDefsToBlender              | Command: Exports all entity and model defs to exported/entities.json for usage in Blender
postLoadExportModels                   | Cvar: Export models after loading to OBJ model format. Set it to 1 before loading a map.
exportMapToOBJ                         | Command: Convert .map file to .obj/.mtl
postLoadExportFlashAtlas               | Cvar: Set to 1 at startup to dump the Flash images to exported/swf/
postLoadExportFlashToSWF               | Cvar: Set to 1 at startup to dump the Flash .bswf files as .swf (WIP)
postLoadExportFlashToJSON              | Cvar: Set to 1 at startup to dump the Flash .bswf files as .json. Can be reimported into the engine and imported into Blender for inspection
swf_show                               | Cvar: Draws the bounding box of instanced Flash sprites in red and their names
dmap mapfile                           | Command: Compiles a .map to its corresponding BSP .proc, Collision .cm files and Area Awareness System (AI navigation) .aas files. Just type dmap to list all options
dmap `[glfile]` mapfile                    | DMap option that exports the BSP areas and portals to .obj for debugging purposes
convertMapToJSON mapfile               | Command: Convert .map file to new .json map format with polygons instead of brushes. This was easy because the original .map format is only an array of entities and each entity has a simple dictionary for its values. This JSON format contains all level data and can be imported and exported to Blender without loosing any data. The new DMap can also compile map files with the .json suffix like regular maps. 
<img src="https://i.imgur.com/2k9IvJC.png" width="384"> <img src="https://i.imgur.com/MnUVKcl.png" width="384">




---
# Known Issues <a name="issues"></a>

* Some lights cause shadow acne with shadow mapping
* Some shadows might almost disappear due to the shadow filtering
* [HDR] HDR does not work with old-school stencil shadows
* [HDR] MSAA anti-aliasing modes don't work with HDR: Use SMAA
* ~~[HDR] HDR causes problems with the grabber gun~~
* ~~[HDR] HDR darkens the screen when you get hit by an enemy~~
* [Vulkan] Shadow Mapping is not supported yet
* [Vulkan] HDR is not supported yet
* [Vulkan] Post processing and SMAA is not supported yet

---
# Bug Reports <a name="reports"></a>

The best way for telling about a bug is by submitting a bug report at our GitHub bug tracker page:

	https://github.com/RobertBeckebans/RBDOOM-3-BFG/issues?state=open

If you want to report an issue with the game, you should make sure that your report includes all information useful to characterize and reproduce the bug.

* Search on Google
* Include the computer's hardware and software description ( CPU, RAM, 3D Card, distribution, kernel etc. )
* If appropriate, send a console log, a screenshot, an strace ..
* If you are sending a console log, make sure to enable developer output:

> RBDoom3BFG.exe +set developer 1 +set logfile 2

You can find your qconsole.log on Windows in C:\Users\<your user name>\Saved Games\id Software\RBDOOM 3 BFG\base\
	
---
# FAQ <a name="faq"></a>

**Q**: Why bother with DOOM-3-BFG in 2019 / 2020?
**A**: It is fun, period. Doom 3 was an impressive milestone in total game development in 2004. In 2011 id Software added lot stuff from the development of Rage like its own Flash SWF and ActionScript 2 interpreter, proper support for gamepads and widescreens. It also combines the gamecode for D3 and its missionpacks and runs it in a seperate thread and it has many multithreaded rendering optimizations. DOOM-3-BFG is one of the most transparent games available where you can open all files and inspect how the game was built.

**Q**: Can I use this engine to make a commercial game?
**A**: You can but don't bother me to give you free support and you probably should use Unreal Engine 4. I am a full time game developer and usually don't have time for any free support.

**Q**: How do I know what code you've changed?
**A**: Apart from the Git diffs, you can look for `// RB` in the source code. Many other contributors commented their changes in the same way. I enforced the usage of Astyle in this project which also makes it alot easier to compare it against other ports of DOOM-3-BFG. Simply format the other ports with Astyle like I do in neo/astyle-code.bat and you can compare the code easily in WinMerge or KDiff3.
