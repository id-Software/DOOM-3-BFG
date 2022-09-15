/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.
Copyright (C) 2022 Stephen Pridham

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "precompiled.h"
#pragma hdrstop

#include "AfPropertyEditor.h"

#include "../imgui/BFGimgui.h"

namespace ImGuiTools
{
AfPropertyEditor::AfPropertyEditor( idDeclAF* newDecl )
	: decl( newDecl )
	, contentWidget( MakePhysicsContentsSelector() )
	, clipMaskWidget( MakePhysicsContentsSelector() )
	, linearTolerance( 0 )
	, angularTolerance( 0 )
	, currentModel( 0 )
{
	contentWidget.UpdateWithBitFlags( decl->contents );
	clipMaskWidget.UpdateWithBitFlags( decl->clipMask );

	UpdateModelDefList();
}

AfPropertyEditor::~AfPropertyEditor()
{
}

bool AfPropertyEditor::Do()
{
	bool changed = false;

	if( ImGui::CollapsingHeader( "MD5" ) )
	{
		if( ImGui::Combo( "Models", &currentModel, StringListItemGetter, &modelDefs, modelDefs.Num() ) )
		{
			decl->model = modelDefs[currentModel];
			changed = true;
		}
		ImGui::InputText( "Skin", &decl->skin, ImGuiInputTextFlags_CharsNoBlank );
	}

	if( ImGui::CollapsingHeader( "Default Collision Detection" ) )
	{
		ImGui::Checkbox( "Self Collision", &decl->selfCollision );
		ImGui::ListBoxHeader( "Contents" );
		changed = DoMultiSelect( &contentWidget, &decl->contents ) || changed;
		ImGui::ListBoxFooter();

		ImGui::ListBoxHeader( "Clip Mask" );
		changed = DoMultiSelect( &clipMaskWidget, &decl->clipMask ) || changed;
		ImGui::ListBoxFooter();
	}

	if( ImGui::CollapsingHeader( "Default Friction" ) )
	{
		changed = ImGui::DragFloat( "Linear", &decl->defaultLinearFriction, 0.01f, -100000.f, 100000.f, "%.2f" ) || changed;
		changed = ImGui::DragFloat( "Angular", &decl->defaultAngularFriction, 0.01f, -100000.f, 100000.f, "%.2f" ) || changed;
		changed = ImGui::DragFloat( "Contact", &decl->defaultContactFriction, 0.01f, -100000.f, 100000.f, "%.2f" ) || changed;
		changed = ImGui::DragFloat( "Constraint", &decl->defaultConstraintFriction, 0.01f, -100000.f, 100000.f, "%.2f" ) || changed;
	}

	if( ImGui::CollapsingHeader( "Mass" ) )
	{
		changed = changed || ImGui::InputFloat( "Total Mass", &decl->totalMass, 1.0f, 50.0f, "%.0f" );
		ImGui::SameLine();
		HelpMarker( "The total mass of the articulated figure.\n"
					"If the total mass is set to a value greater than\n"
					"zero then the mass of each body is scaled such that\n"
					"the total mass of the articulated figure equals the given mass." );
	}

	if( ImGui::CollapsingHeader( "Suspend Speed" ) )
	{
		changed = changed || ImGui::InputFloat2( "Linear Velocity", ( float* )&decl->suspendVelocity, "%.0f" );
		changed = changed || ImGui::InputFloat2( "Linear Acceleration", ( float* )&decl->suspendAcceleration, "%.0f" );
	}

	// TODO(Stephen): Figure out what these properties are for.
	//if (ImGui::CollapsingHeader("Suspend Movement"))
	//{
	//	ImGui::InputInt("No Move Time", (int*)&decl->noMoveTime);
	//	ImGui::InputInt("Linear Tolerance", &linearTolerance);
	//	ImGui::InputInt("Angular Tolerance", &angularTolerance);
	//	ImGui::InputInt("Minimum Move Time", (int*)&decl->minMoveTime);
	//	ImGui::InputInt("Maximum Move Time", (int*)&decl->maxMoveTime);
	//}

	return changed;
}

void AfPropertyEditor::UpdateModelDefList()
{
	// update the model list.
	const int totalModelDefs = declManager->GetNumDecls( DECL_MODELDEF );
	modelDefs.AssureSize( totalModelDefs );
	for( int i = 0; i < totalModelDefs; i++ )
	{
		const idDecl* modelDef = declManager->DeclByIndex( DECL_MODELDEF, i, false );
		if( modelDef )
		{
			modelDefs[i] = modelDef->GetName();
			if( decl->model == modelDef->GetName() )
			{
				currentModel = i;
			}
		}
	}
}

}