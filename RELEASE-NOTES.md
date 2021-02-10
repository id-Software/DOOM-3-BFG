```
    ____   ____   ____                           _____  ____   ______ ______
   / __ \ / __ ) / __ \ ____   ____   ____ ___  |__  / / __ ) / ____// ____/
  / /_/ // __  |/ / / // __ \ / __ \ / __ `__ \  /_ < / __  |/ /_   / / __  
 / _, _// /_/ // /_/ // /_/ // /_/ // / / / / /___/ // /_/ // __/  / /_/ /  
/_/ |_|/_____//_____/ \____/ \____//_/ /_/ /_//____//_____//_/     \____/   
_________________________________________
```


RBDOOM-3-BFG Release Notes - https://github.com/RobertBeckebans/RBDOOM-3-BFG

Thank you for downloading RBDOOM-3-BFG.


_______________________________________

TBD end 2020 - Changes since RBDOOM-3-BFG 1.2.0
_______________________________

RBDOOM-3-BFG 1.3.0 is mainly a Phyiscally Based Rendering Update.

Implementing Phyiscally Based Rendering (PBR) in Doom 3 is a challenge and comes with a few compromises because the Doom 3 content was designed to work with the hardware constraints in 2004 and that even meant to run on a Geforce 3.

The light rigs aren't made for PBR but it is possible to achieve a good PBR lighting even with the old content by tweaking the light formulars with a few good magic constants. However I also want to support the modding scene to allow them to create brand new PBR materials made with Substance Designer/Painter or other modern tools so multiple rendering paths have been implemented.

From an artistic point of view PBR allows artists to create textures that are based on real world measured color values and they look more or less the same in any renderer that follows the PBR guidelines and formulars.
RBDOOM-3-BFG only supports the PBR roughness/metal workflow.

The main goal is that the new content looks the same in RBDOOM-3-BFG as in Blender 2.8 with Cycles or Eevee.

[PBR]

* Changed light interaction shaders to use PBR GGX Cook-Torrance specular contribution. The material roughness is estimated by the old school Doom 3 glossmap if no PBR texture is provided

* PBR texture support ground work is done. Only a few bits are left to be enabled.

* Extended ambient pass to support Image Based Lighting using a single fixed cubemap at the moment (r_usePBR)

* Turned off Half-Lambert lighting hack in favor of Image Based Lighting

* Improved r_hdrDebug which shows F-stops like in Filament

* Turned off r_hdrAutomaticExposure by default because it caused too much flickering and needs further work. The new default values work well with all Doom 3 content

* r_lightScale (default 3) can be changed when HDR is enabled

* Light shaders were tweaked with magic constants that r_useHDR 1, r_forceAmbient 0.3, r_exposure 0.5 look as bright as in Doom 2016 or any other PBR renderer

* SSAO only affects the ambient pass and not the direct lighting and the ambient occlusion affects the specular indirect contribution depending on the roughness of a material. See moving Frostbite to PBR (Lagarde2014)

* Added HACK to look for PBR reflection maps with the suffix _rmao if a specular map was specified and ends with _s.tga. This allows to override the materials with PBR textures without touching the material .mtr files.

* Fixed ambient lights, 3D GUIs and fog being too bright in HDR mode

* Fixed darkening of the screen in HDR mode when you get hit by an enemy

* Fixed ellipse bug when using Grabber gun in HDR mode #407

[VULKAN]

* Fixed GPU Skinning with Vulkan

* Fixed the lighting with stencil shadows with Vulkan and added Carmarck's Rerverse optimization

* Added anisotropic filtering to Vulkan

* Added Git submodule glslang 7.10.2984 -> stable release Nov 15, 2018

* Vulkan version builds on Linux. Big thanks to Eric Womer for helping out with the SDL 2 part

* Fixed Bink video playback with Vulkan

* ImGui runs with Vulkan by skipping all Vulkan implementation details for it and rendering ImGui on a higher level like the Flash GUI

[MISCELLANEOUS]

* com_showFPS 1 uses ImGui to show more detailed renderer stats like the console prints with r_speeds

* Removed 32bit support: FFmpeg and OpenAL libraries are only distributed as Win64 versions, 32bit CMakes files are gone

* Added Blue Noise based Filmic Dithering by Timothy Lottes and Chromatic Aberration

* Added Contrast Adaptive Sharpening (AMD) by Justin Marshal (IcedTech)

* Improved Shadow Mapping quality with Vogel Disk Sampling by Panos Karabelas and using dithering the result with Blue Noise magic by Alan Wolfe

* Improved Screen Space Ambient Occlusion performance by enhancing the quality with Blue Noise and skipping the expensive extra bilateral filtering pass

* Updated idRenderLog to support RenderDoc and Nvidia's Nsight and only issue OpenGL or Vulkan debug commands if the debug extensions are detected. Reference: https://devblogs.nvidia.com/best-practices-gpu-performance-events/

* Artistic Style C++ beautifier configuration has slightly changed to work closer to Clang Format's behaviour

* Updated documentation regarding modding support in the README

* Bumped the static vertex cache limit of 31 MB to roughly ~ 128 MB to help with some custom models and maps by the Doom 3 community

* Renamed r_useFilmicPostProcessEffects to r_useFilmicPostProcessing

* Replaced Motion Blur System Option with Filmic VFX (r_useFilmicPostProcessing)



_______________________________________

October 2019 - RBDOOM-3-BFG 1.2.0
_______________________________

This is a maintenance release without Vulkan support even though it contains a lot Vulkan specific code.

* Experimental Work in Progress Vulkan renderer backend based on Dustin Land's Doom 3 BFG Vulkan port
  This renderer backend only supports ambient lighting and no light interactions but it also comes with a big renderer code cleanup.

  The changes also fixed several rendering bugs that occured in OpenGL:
  
* Fixed black dots when HDR and SSAO is enabled

* Hit/damage markers show a white background #422

* Refactored and simplified OpenGL renderer backend to match Vulkan backend

* Renamed the .vertex and .pixel shader files to .hlsl
  This allows better editing of the shaders within Visual Studio including Intellisense support.

* Integrated libbinkdec for video playback as a slim alternative to FFmpeg (thanks to Daniel Gibson)

* Added in-engine Flash debugging tools and new console variables.
  These tools help to analyse the id Tech view of Flash and what SWF tags are supported and how they are interpreted
  by id Tech's own ActionScript 2 interpreter
	- postLoadExportAtlas
	- postLoadExportSWF
	- postLoadExportJSON
	- swf_show : Draws the bounding box of instanced Flash sprites in red and their names

* Added Steel Storm 2 Engine render demo fixes

* Merged LordHavoc's image compression progress bar which shows up in the map loading screen
  when loading and compressing new images from mods
  
* Added instructions how to use these Doom 3 port with the GOG installer

* Many smaller compiler related fixes like VS 2017 and VS 2019 support.
  If it fails to compile with GCC then it should at least build with Clang on Linux


_______________________________________

11 April 2016 - RBDOOM-3-BFG 1.1.0 preview 3
_______________________________

* True 64 bit HDR lighting with adaptive filmic tone mapping and gamma-correct rendering in linear RGB space

* Enhanced Subpixel Morphological Antialiasing
	For more information see "Anti-Aliasing Methods in CryENGINE 3" and the docs at http://www.iryoku.com/smaa/
* Filmic post process effects like Technicolor color grading and film grain

* Fixed issues with Mesa drivers and allowed them to use shadow mapping

* Defaulted fs_resourceLoadPriority to 0 so it is not necessary anymore to specify when running a modification

* Additional ambient render pass to make the game less dark similar to the Quake 4 r_forceAmbient technique

* Screen Space Ambient Occlusion http://graphics.cs.williams.edu/papers/SAOHPG12/

* Did some fine tuning to the Half-Lambert lighting curve so bump mapping doesn't loose too much details


_______________________________________

7 March 2015 - RBDOOM-3-BFG 1.0.3
_______________________________

* SDL 2 is the default for Linux

* CMake options like -DUSE_SYSTEM_LIBJPEG to aid Linux distribution package maintainers

* SDL gamepad support and other SDL input improvements like support for more mouse buttons

* Mac OS X support (experimental)

* XAudio 2, Windows 8 SDK fixes

* Better Mesa support (not including advanced shadow mapping)

* Added back dmap and aas compilers (mapping tools)

* Cinematic sequences can be skipped

* Localization support for other languages than English

* Improved modding support through loading of custom models and animations (see section 12 MODIFICATIONS in the README)



_______________________________________

17 May 2014 - RBDOOM-3-BFG 1.0.2

RBDOOM-3-BFG-1.0.2-ATI-hotfix1-win32-20140517-git-952907f.7z
_______________________________

This release doubles the shadow mapping performance and allows the game to be played at solid 120 fps in 1080p with a Nvidia GTX 660 Ti.
It also adds a graphics option to enable/disable shadow mapping and fixes a few problems with FFmpeg playing Bink videos.



_______________________________________

11 May 2014 - RBDOOM-3-BFG 1.0.1

RBDOOM-3-BFG-win32-20140511-git-f950769.7z
_______________________________

This release improves the performance drastically and adds shadow mapping support for translucent surfaces like grates.


_______________________________________

10 May 2014 - RBDOOM-3-BFG 1.0

RBDOOM-3-BFG-win32-20140510-git-14f87fe.7z
_______________________________

* Added soft shadow mapping

* Added PNG image support

* Replaced QGL with GLEW

* base/renderprogs/ can be baked into the executable using Premake/Lua

* Replaced Visual Studio solution with CMake

* Linux port using SDL/SDL2 and OpenAL

* 64 bit support for Windows and Linux

* Sourcecode cleanup using Artistic Style 2.03 C++ beautifier and by fixing tons of warnings using Clang compiler

* Fast compile times using precompiled header support

* Tons of renderer bugfixes and Cg -> GLSL converter that supports ES 2.00, ES 3.30 additional to GLSL 1.50

* Netcode fixes to allow multiplayer sessions to friends with +connect <ip of friend> (manual port forwarding required)






