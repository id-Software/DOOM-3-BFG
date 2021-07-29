/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 2016 Daniel Gibson

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

#include "precompiled.h"
#pragma hdrstop

#include "../imgui/BFGimgui.h"
#include "../idlib/CmdArgs.h"

#include "lighteditor/LightEditor.h"


extern idCVar g_editEntityMode;

static bool releaseMouse = false;
#if 0 // currently this doesn't make too much sense
void ShowEditors_f( const idCmdArgs& args )
{
	showToolWindows = true;
}
#endif // 0

namespace ImGuiTools
{

// things in impl need to be used in at least one other file, but should generally not be touched
namespace impl
{

void SetReleaseToolMouse( bool doRelease )
{
	releaseMouse = doRelease;
}

} //namespace impl

bool AreEditorsActive()
{
	// FIXME: this is not exactly clean and must be changed if we ever support game dlls
	return g_editEntityMode.GetInteger() > 0;
}

bool ReleaseMouseForTools()
{
	return AreEditorsActive() && releaseMouse;
}

void DrawToolWindows()
{
#if 0
	ImGui::Begin( "Show Ingame Editors", &showToolWindows, 0 );

	ImGui::Checkbox( "Light", &LightEditor::showIt );
	ImGui::SameLine();
	ImGui::Checkbox( "Particle", &showParticlesEditor );
#endif // 0

	if( LightEditor::showIt )
	{
		LightEditor::Draw();
	}

	// TODO: other editor windows..
	//ImGui::End();
}

void LightEditorInit( const idDict* dict, idEntity* ent )
{
	if( dict == NULL || ent == NULL )
	{
		return;
	}

	// NOTE: we can't access idEntity (it's just a declaration), because it should
	// be game/mod specific. but we can at least check the spawnclass from the dict.
	idassert( idStr::Icmp( dict->GetString( "spawnclass" ), "idLight" ) == 0
			  && "LightEditorInit() must only be called with light entities or NULL!" );


	LightEditor::showIt = true;
	impl::SetReleaseToolMouse( true );

	LightEditor::ReInit( dict, ent );
}

} //namespace ImGuiTools
