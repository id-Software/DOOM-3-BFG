```
    ____   ____   ____                           _____  ____   ______ ______
   / __ \ / __ ) / __ \ ____   ____   ____ ___  |__  / / __ ) / ____// ____/
  / /_/ // __  |/ / / // __ \ / __ \ / __ `__ \  /_ < / __  |/ /_   / / __  
 / _, _// /_/ // /_/ // /_/ // /_/ // / / / / /___/ // /_/ // __/  / /_/ /  
/_/ |_|/_____//_____/ \____/ \____//_/ /_/ /_//____//_____//_/     \____/   
_______________________________________________________________________
```


RBDOOM-3-BFG Release Notes - https://github.com/RobertBeckebans/RBDOOM-3-BFG

Thank you for downloading RBDOOM-3-BFG.


_______________________________________

TBD Q1/2021 - RBDOOM-3-BFG 1.3.0 - Changes since RBDOOM-3-BFG 1.2.0
_______________________________

<img src="https://i.imgur.com/iQjLKzx.png">

# RBDOOM-3-BFG 1.3.0 adds PBR rendering and TrenchBroom mapping support

## Physically Based Rendering

Implementing Physically Based Rendering (PBR) in Doom 3 is a challenge and comes with a few compromises because the Doom 3 content was designed to work with the hardware constraints in 2004 and that even meant to run on a Geforce 3.

The light rigs aren't made for PBR but it is possible to achieve good PBR lighting even with the old content by tweaking the light formulars with a few good magic constants. However I also want to support the modding scene to allow them to create brand new PBR materials made with Substance Designer/Painter or other modern tools so multiple rendering paths have been implemented.

## Extra Ambient Pass using Image Based Lighting

From an artistic point of view PBR allows artists to create textures that are based on real world measured color values and they look more or less the same in any renderer that follows the PBR guidelines and formulars.
RBDOOM-3-BFG only supports the PBR roughness/metal workflow.

The main goal is that the new content looks the same in RBDOOM-3-BFG as in Blender 2.9 with Cycles or Eevee.

To achieve the typical PBR look from an artistic point of view it also means to that it is necessary to add indirect lighting. Doom 3 and even Doom 3 BFG had no indirect lighting.
RBDOOM-3-BFG 1.2.0 already had indirect lighting similar to Quake 4 which used a fixed light direction.
This new version distributes automatically environment probes through the maps using the BSP area bounds.

The environment probes use an octahedron encoding and the specular mipmaps are convolved using the Split Sum Approximation by Brian Karris [Epic 2013].

For artists this basically means if you increase the roughness in your material then you increase the mip map level of the environment probe it samples light from and it gets blurier.

<img src="https://i.imgur.com/KkPSWrc.png" width="384"> <img src="https://i.imgur.com/8vEJCDR.png" width="384">

<img src="https://i.imgur.com/67k9QXG.png" width="384"> <img src="https://i.imgur.com/gfBG0Gm.png" width="384">

Left: No extra ambient pass. Ambient is pure black like in original Doom 3. Right: Extra ambient pass with r_forceAmbient 0.2 using local environment probe for irradiance and radiance lighting.

<img src="https://i.imgur.com/ZEI4i87.png" width="384"> <img src="https://i.imgur.com/FC82oOM.png" width="384">

Some examples that show additional environment lighting on all assets.

<img src="https://i.imgur.com/0OOgwC9.png" width="384"> <img src="https://i.imgur.com/qL0DgcZ.png" width="384">

<img src="https://i.imgur.com/5Pcomzu.png" width="384"> <img src="https://i.imgur.com/IczOpHI.png" width="384">

## PBR Texture format

In Doom 3 a classic simple materials looks like this:
```
textures/base_wall/snpanel2rust
{
  qer_editorimage		textures/base_wall/snpanel2rust.tga
	
  bumpmap           textures/base_wall/snpanel2_local.tga
  diffusemap        textures/base_wall/snpanel2rust_d.tga
  specularmap       textures/base_wall/snpanel2rust_s.tga
}
```

It's usually rendered with Blinn-Phong specular with a fixed specular exponent.
Specularmaps are more or less Gloss maps.

In RBDOOM-3-BFG it uses the PBR GGX Cook-Torrence formular and you can vary the roughness along the material like in other modern engines and you usually define a texture with the _rmao suffix.

RMAO Image Channels             | Description
:-----------------------------  | :------------------------------------------------
Red                             | Roughness
Green                           | Metalness
Blue                            | Ambient Occlusion

Example material:
```
models/mapobjects/materialorb/orb
{
  qer_editorimage   models/mapobjects/pbr/materialorb/substance/metal04_basecolor.png
  
  basecolormap      models/mapobjects/pbr/materialorb/substance/metal04_basecolor.png
  normalmap         models/mapobjects/pbr/materialorb/substance/metal04_normal.png
  specularmap       models/mapobjects/pbr/materialorb/substance/metal04_rmao.png
}
```

The engine will also look for _rmao.[png/tga] overrides for old materials and if it finds them it will render using a better PBR path. Oldschool specularmaps also go through a GGX pipeline but the roughness is estimated from the glossmap.

The Ambient Occlusion will be mixed with the Screen Space Ambient Occlusion and will only affect indirect lighting contributed by the environment probes.

## Filmic Post Processing

TL;DR If you enable it then you play DOOM 3 BFG with the optics of a Zack Snyder movie.

This release adds chromatic abberation and filmic dithering using Blue Noise.
The effect is heavy and is usually aimed in Film production to mix real camera footage with CG generated content.

Dithering demonstration: left side is quantized to 3 bits for each color channel. Right side is also only 3 bits but dithered with chromatic Blue Noise. The interesting fact about the dithering here is shown in the upper debug bands.
The first top band is the original signal. The second shows just 8 blocks and if you dither the those blocks with Blue Noise then it is close to the original signal which is surprising.

<img src="https://i.imgur.com/QJv2wH2.png" width="384"> <img src="https://i.imgur.com/MaXqld4.png" width="384">

## TrenchBroom

This release also adds experimental support for TrenchBroom. The goal of the TrenchBroom support is to make it easier to create new maps. It doesn't allow to create bezier patches so you won't be able to edit existing Doom 3 maps.
However you can copy paste brushes and entities into your new map using the Doom 3 (Valve) configuration.

TODO


## Changelog

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

[TRENCHBROOM]

* idMapFile and dmap were changed to support the Valve 220 .map format to aid mapping with TrenchBroom

* Added exportFGD [models] console command which exports all def/*.def entityDef declarations to base/exported/_tb/ as Forge Game Data files. TrenchBroom has native support to read those files https://developer.valvesoftware.com/wiki/FGD.
Using the models argument will also export all needed models by entity declarations to base/_tb/ as Wavefront OBJ files.

* To make it easier getting static models from Blender/Maya/3D Studio Max into TrenchBroom and the engine Wavefront OBJ model support has been ported from IcedTech

[MISCELLANEOUS]

* com_showFPS bigger than 1 uses ImGui to show more detailed renderer stats like the original console prints with r_speeds

* Removed 32bit support: FFmpeg and OpenAL libraries are only distributed as Win64 versions, 32bit CMakes files are gone

* Added Blue Noise based Filmic Dithering by Timothy Lottes and Chromatic Aberration

* Added Contrast Adaptive Sharpening (AMD) by Justin Marshall (IcedTech)

* Improved Shadow Mapping quality with Vogel Disk Sampling by Panos Karabelas and using dithering the result with Blue Noise magic by Alan Wolfe

* Improved Screen Space Ambient Occlusion performance by enhancing the quality with Blue Noise and skipping the expensive extra bilateral filtering pass

* Updated idRenderLog to support RenderDoc and Nvidia's Nsight and only issue OpenGL or Vulkan debug commands if the debug extensions are detected. Reference: https://devblogs.nvidia.com/best-practices-gpu-performance-events/

* Artistic Style C++ beautifier configuration has slightly changed to work closer to Clang Format's behaviour

* Updated documentation regarding modding support in the README

* Bumped the static vertex cache limit of 31 MB to roughly ~ 128 MB to help with some custom models and maps by the Doom 3 community

* Added support for Mikkelsen tangent space standard for new assets (thanks to Stephen Pridham)

* Renamed r_useFilmicPostProcessEffects to r_useFilmicPostProcessing

* Replaced Motion Blur System Option with Filmic VFX (r_useFilmicPostProcessing)

* Windows builds still require OpenGL 4.5 but they run in compatibility profile instead of core profile



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






