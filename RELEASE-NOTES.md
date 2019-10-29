    ____   ____   ____                           _____  ____   ______ ______
   / __ \ / __ ) / __ \ ____   ____   ____ ___  |__  / / __ ) / ____// ____/
  / /_/ // __  |/ / / // __ \ / __ \ / __ `__ \  /_ < / __  |/ /_   / / __  
 / _, _// /_/ // /_/ // /_/ // /_/ // / / / / /___/ // /_/ // __/  / /_/ /  
/_/ |_|/_____//_____/ \____/ \____//_/ /_/ /_//____//_____//_/     \____/   
_________________________________________


RBDOOM-3-BFG Release Notes - https://github.com/RobertBeckebans/RBDOOM-3-BFG

Thank you for downloading RBDOOM-3-BFG.



_______________________________________

TBD in early 2020 - RBDOOM-3-BFG 1.3.0
_______________________________

- Finish Vulkan renderer backend

- Fix GPU Skinning with Vulkan

- Fix the lighting with stencil shadows with Vulkan

- Port HDR and Shadow Mapping to Vulkan



_______________________________________

October 2019 - RBDOOM-3-BFG 1.2.0
_______________________________

This is a maintenance release without Vulkan support even though it contains a lot Vulkan specific code.

- Experimental Work in Progress Vulkan renderer backend based on Dustin Land's Doom 3 BFG Vulkan port
  This renderer backend only supports ambient lighting and no light interactions but it also comes with a big renderer code cleanup.

  The changes also fixed several rendering bugs that occured in OpenGL:
  
- Fixed black dots when HDR and SSAO is enabled

- Hit/damage markers show a white background #422

- Refactored and simplified OpenGL renderer backend to match Vulkan backend

- Renamed the .vertex and .pixel shader files to .hlsl
  This allows better editing of the shaders within Visual Studio including Intellisense support.

- Integrated libbinkdec for video playback as a slim alternative to FFmpeg (thanks to Daniel Gibson)

- Added in-engine Flash debugging tools and new console variables.
  These tools help to analyse the id Tech view of Flash and what SWF tags are supported and how they are interpreted
  by id Tech's own ActionScript 2 interpreter
	- swf_exportAtlas
	- swf_exportSWF
	- swf_exportJSON
	- swf_show : Draws the bounding box of instanced Flash sprites in red and their names

- Added Steel Storm 2 Engine render demo fixes

- Merged LordHavoc's image compression progress bar which shows up in the map loading screen
  when loading and compressing new images from mods
  
- Added instructions how to use these Doom 3 port with the GOG installer

- Many smaller compiler related fixes like VS 2017 and VS 2019 support.
  If it fails to compile with GCC then it should at least build with Clang on Linux


_______________________________________

11 April 2016 - RBDOOM-3-BFG 1.1.0 preview 3
_______________________________

- True 64 bit HDR lighting with adaptive filmic tone mapping and gamma-correct rendering in linear RGB space

- Enhanced Subpixel Morphological Antialiasing
	For more information see "Anti-Aliasing Methods in CryENGINE 3" and the docs at http://www.iryoku.com/smaa/

- Filmic post process effects like Technicolor color grading and film grain

- Fixed issues with Mesa drivers and allowed them to use shadow mapping

- Defaulted fs_resourceLoadPriority to 0 so it is not necessary anymore to specify when running a modification

- Additional ambient render pass to make the game less dark similar to the Quake 4 r_forceAmbient technique

- Screen Space Ambient Occlusion http://graphics.cs.williams.edu/papers/SAOHPG12/

- Did some fine tuning to the Half-Lambert lighting curve so bump mapping doesn't loose too much details


_______________________________________

7 March 2015 - RBDOOM-3-BFG 1.0.3
_______________________________

- SDL 2 is the default for Linux

- CMake options like -DUSE_SYSTEM_LIBJPEG to aid Linux distribution package maintainers

- SDL gamepad support and other SDL input improvements like support for more mouse buttons

- Mac OS X support (experimental)

- XAudio 2, Windows 8 SDK fixes

- Better Mesa support (not including advanced shadow mapping)

- Added back dmap and aas compilers (mapping tools)

- Cinematic sequences can be skipped

- Localization support for other languages than English

- Improved modding support through loading of custom models and animations (see section 12 MODIFICATIONS in the README)



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

- Added soft shadow mapping

- Added PNG image support

- Replaced QGL with GLEW

- base/renderprogs/ can be baked into the executable using Premake/Lua

- Replaced Visual Studio solution with CMake

- Linux port using SDL/SDL2 and OpenAL

- 64 bit support for Windows and Linux

- Sourcecode cleanup using Artistic Style 2.03 C++ beautifier and by fixing tons of warnings using Clang compiler

- Fast compile times using precompiled header support

- Tons of renderer bugfixes and Cg -> GLSL converter that supports ES 2.00, ES 3.30 additional to GLSL 1.50

- Netcode fixes to allow multiplayer sessions to friends with +connect <ip of friend> (manual port forwarding required)






