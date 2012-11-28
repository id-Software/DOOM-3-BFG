/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 BFG Edition GPL Source Code ("Doom 3 BFG Edition Source Code").

Doom 3 BFG Edition Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 BFG Edition Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 BFG Edition Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 BFG Edition Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 BFG Edition Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#ifndef __PRECOMPILED_H__
#define __PRECOMPILED_H__

#include "sys/sys_defines.h"
#include "sys/sys_builddefines.h"
#include "sys/sys_includes.h"
#include "sys/sys_assert.h"
#include "sys/sys_types.h"
#include "sys/sys_intrinsics.h"
#include "sys/sys_threading.h"

//-----------------------------------------------------

#define ID_TIME_T int64 // Signed because -1 means "File not found" and we don't want that to compare > than any other time

// non-portable system services
#include "../sys/sys_public.h"

// id lib
#include "../idlib/Lib.h"

#include "sys/sys_filesystem.h"

// framework
#include "../framework/BuildVersion.h"
#include "../framework/Licensee.h"
#include "../framework/CmdSystem.h"
#include "../framework/CVarSystem.h"
#include "../framework/Common.h"
#include "../framework/File.h"
#include "../framework/File_Manifest.h"
#include "../framework/File_SaveGame.h"
#include "../framework/File_Resource.h"
#include "../framework/FileSystem.h"
#include "../framework/UsercmdGen.h"
#include "../framework/Serializer.h"
#include "../framework/PlayerProfile.h"

// decls
#include "../framework/TokenParser.h"
#include "../framework/DeclManager.h"
#include "../framework/DeclTable.h"
#include "../framework/DeclSkin.h"
#include "../framework/DeclEntityDef.h"
#include "../framework/DeclFX.h"
#include "../framework/DeclParticle.h"
#include "../framework/DeclAF.h"
#include "../framework/DeclPDA.h"

// We have expression parsing and evaluation code in multiple places:
// materials, sound shaders, and guis. We should unify them.
const int MAX_EXPRESSION_OPS = 4096;
const int MAX_EXPRESSION_REGISTERS = 4096;

// renderer
#include "../renderer/OpenGL/qgl.h"
#include "../renderer/Cinematic.h"
#include "../renderer/Material.h"
#include "../renderer/BufferObject.h"
#include "../renderer/VertexCache.h"
#include "../renderer/Model.h"
#include "../renderer/ModelManager.h"
#include "../renderer/RenderSystem.h"
#include "../renderer/RenderWorld.h"

// sound engine
#include "../sound/sound.h"

// user interfaces
#include "../ui/ListGUI.h"
#include "../ui/UserInterface.h"

#include "../swf/SWF.h"

// collision detection system
#include "../cm/CollisionModel.h"

// AAS files and manager
#include "../aas/AASFile.h"
#include "../aas/AASFileManager.h"

// game
#include "../d3xp/Game.h"

// Session / Network
#include "../sys/LightweightCompression.h"
#include "../sys/Snapshot.h"
#include "../sys/PacketProcessor.h"
#include "../sys/SnapshotProcessor.h"

#include "../sys/sys_savegame.h"
#include "../sys/sys_session_savegames.h"
#include "../sys/sys_profile.h"
#include "../sys/sys_localuser.h"
#include "../sys/sys_signin.h"
#include "../sys/sys_stats_misc.h"
#include "../sys/sys_stats.h"
#include "../sys/sys_session.h"
#include "../sys/sys_achievements.h"

//-----------------------------------------------------

#ifndef _D3SDK

#ifdef GAME_DLL

#include "../d3xp/Game_local.h"

#else

#include "../framework/DemoChecksum.h"

// framework
#include "../framework/Compressor.h"
#include "../framework/EventLoop.h"
#include "../framework/KeyInput.h"
#include "../framework/EditField.h"
#include "../framework/DebugGraph.h"
#include "../framework/Console.h"
#include "../framework/DemoFile.h"
#include "../framework/Common_dialog.h"

#endif /* !GAME_DLL */

#endif /* !_D3SDK */

//-----------------------------------------------------

#undef min
#undef max
#include <algorithm>	// for min / max / swap

#endif /* !__PRECOMPILED_H__ */
