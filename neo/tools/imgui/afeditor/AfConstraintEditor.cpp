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

#include "AfConstraintEditor.h"

#include "../imgui/BFGimgui.h"

static const char* constraintTypeNames[] =
{
	"Invalid",
	"Fixed",
	"Ball and Socket Joint",
	"Universal Joint",
	"Hinge",
	"Slider",
	"Spring"
};

static const char* anchorTypeNames[] =
{
	"joint",
	"coorindates"
};

static const char* jointPointTypeNames[] =
{
	"bone",
	"angles"
};

static const char* afVecTypeNames[] =
{
	"coordinates",
	"joint",
	"bone center",
	"bone direction"
};

static const char* constraintLimitTypeNames[] =
{
	"none",
	"cone",
	"pyramid"
};

namespace ImGuiTools
{

static bool BodyItemGetter( void* data, int index, const char** out );
static bool ConstraintLimitTypeGetter( void* data, int index, const char** out );

AfConstraintEditor::AfConstraintEditor( idDeclAF* newDecl, idDeclAF_Constraint* newConstraint )
	: decl( newDecl )
	, constraint( newConstraint )
	, anchorType( 0 )
	, shaft1Type( 0 )
	, shaft2Type( 0 )
	, limitOrientationType( 0 )
	, body1( 0 )
	, body2( 0 )
	, jointAnchor1( 0 )
	, jointAnchor2( 0 )
	, jointShaft1( 0 )
{
	InitJointLists();
}

AfConstraintEditor::~AfConstraintEditor()
{
}

bool AfConstraintEditor::Do()
{
	ImGui::PushID( constraint->name.c_str() );

	bool changed = false;

	ImGui::InputText( "##rename", &renameConstraint );

	ImGui::SameLine();

	if( ImGui::Button( "Rename" ) )
	{
		if( renameConstraint.Allocated() > 0 )
		{
			decl->RenameConstraint( constraint->name, renameConstraint.c_str() );
			renameConstraint.Clear();
		}
		changed = true;
	}

	if( ImGui::CollapsingHeader( "General" ) )
	{
		ImGui::Columns( 2, "constraintGeneral2" );
		changed = ImGui::Combo( "Type##General", ( int* )&constraint->type, constraintTypeNames, ARRAY_COUNT( constraintTypeNames ) ) || changed;
		changed = ImGui::DragFloat( "Friction##Friction", &constraint->friction ) || changed;

		if( constraint->type == DECLAF_CONSTRAINT_SPRING )
		{
			changed = ImGui::DragFloat( "Stretch", &constraint->stretch ) || changed;
			ImGui::SameLine();
			HelpMarker( "Spring constant when the spring is stretched" );
			changed = ImGui::DragFloat( "Compress", &constraint->compress, 0.001f, 0.0f, 1.0f ) || changed;
			ImGui::SameLine();
			HelpMarker( "Spring constant when the spring is compressed" );
			changed = ImGui::DragFloat( "Damping", &constraint->damping, 0.001f, 0.0f, 1.0f ) || changed;
			ImGui::SameLine();
			HelpMarker( "Spring damping" );
			changed = ImGui::DragFloat(	"Length", &constraint->restLength, 0.001f, 0.0f, 1.0f ) || changed;
			ImGui::SameLine();
			HelpMarker( "Rest length of the spring" );
			changed = ImGui::DragFloat(	"Min Length", &constraint->minLength, 0.001f, 0.0f, 1.0f ) || changed;
			ImGui::SameLine();
			HelpMarker( "Minimum length of the spring" );
			changed = ImGui::DragFloat(	"Max Length", &constraint->maxLength, 0.001f, 0.0f, 1.0f ) || changed;
			ImGui::SameLine();
			HelpMarker( "Maximum length of the spring" );
		}

		ImGui::NextColumn();
		if( ImGui::BeginCombo( "Body 1", constraint->body1.c_str() ) )
		{
			for( int n = 0; n < decl->bodies.Num(); n++ )
			{
				bool isSelected = ( constraint->body1 == decl->bodies[n]->name );
				if( ImGui::Selectable( decl->bodies[n]->name, isSelected ) )
				{
					constraint->body1 = decl->bodies[n]->name;
					changed = true;
				}
				if( isSelected )
				{
					ImGui::SetItemDefaultFocus();    // Set the initial focus when opening the combo (scrolling + for keyboard navigation support in the upcoming navigation branch)
				}
			}
			ImGui::EndCombo();
		}

		if( ImGui::BeginCombo( "Body 2", constraint->body2.c_str() ) )
		{
			for( int n = 0; n < decl->bodies.Num(); n++ )
			{
				bool isSelected = ( constraint->body2 == decl->bodies[n]->name );
				if( ImGui::Selectable( decl->bodies[n]->name, isSelected ) )
				{
					constraint->body2 = decl->bodies[n]->name;
					changed = true;
				}
				if( isSelected )
				{
					ImGui::SetItemDefaultFocus();    // Set the initial focus when opening the combo (scrolling + for keyboard navigation support in the upcoming navigation branch)
				}
			}
			ImGui::EndCombo();
		}

		ImGui::Columns( 1 );
	}

	if( ImGui::CollapsingHeader( "Anchor" ) )
	{
		ImGui::PushID( "Anchor1" );
		ImGui::Columns( 2, "anchorColumn2" );
		changed = ImGui::Combo( "Type##anchor", ( int* )&constraint->anchor.type, afVecTypeNames, ARRAY_COUNT( afVecTypeNames ) ) || changed;
		ImGui::NextColumn();
		changed = InputAfVector( &constraint->anchor ) || changed;
		ImGui::Columns( 1 );
		ImGui::PopID();
	}

	if( constraint->type == DECLAF_CONSTRAINT_SPRING )
	{
		if( ImGui::CollapsingHeader( "Anchor2" ) )
		{
			ImGui::PushID( "Anchor2" );
			ImGui::Columns( 2, "anchorColumn2" );
			changed = ImGui::Combo( "Type##anchor", ( int* )&constraint->anchor2.type, afVecTypeNames, ARRAY_COUNT( afVecTypeNames ) ) || changed;
			ImGui::NextColumn();
			changed = InputAfVector( &constraint->anchor2 ) || changed;
			ImGui::Columns( 1 );
			ImGui::PopID();
		}
	}

	if( constraint->type == DECLAF_CONSTRAINT_UNIVERSALJOINT )
	{
		if( ImGui::CollapsingHeader( "Shaft 1" ) )
		{
			ImGui::PushID( "Shaft 1" );
			ImGui::Columns( 2, "shaft12" );
			changed = ImGui::Combo( "Type", ( int* )&constraint->shaft[0].type, afVecTypeNames, ARRAY_COUNT( afVecTypeNames ) ) || changed;
			ImGui::NextColumn();
			changed = InputAfVector( &constraint->shaft[0] ) || changed;
			ImGui::Columns( 1 );
			ImGui::PopID();
		}

		if( ImGui::CollapsingHeader( "Shaft 2" ) )
		{
			ImGui::PushID( "Shaft 2" );
			ImGui::Columns( 2, "shaft22" );
			changed = ImGui::Combo( "Type", ( int* )&constraint->shaft[1].type, afVecTypeNames, ARRAY_COUNT( afVecTypeNames ) ) || changed;
			ImGui::NextColumn();
			changed = InputAfVector( &constraint->shaft[1] ) || changed;
			ImGui::Columns( 1 );
			ImGui::PopID();
		}
	}

	if( constraint->type == DECLAF_CONSTRAINT_HINGE )
	{
		if( ImGui::CollapsingHeader( "Limit Orientation" ) )
		{
			ImGui::DragFloat3( "Limit Angles", &constraint->limitAngles[0] );
			ImGui::SameLine();
			HelpMarker( "Specifies a V-shaped limit."
						"\nThe first angle specifies the center of the V-shaped limit."
						"\nThe angle which specifies the width of the V-shaped limit follows."
						"\nThe V-shape is attached to body2. Next a shaft is specified which"
						"\nis attached to body1 and is constrained to always stay within the V-shape."
						"\nThe orientation of this shaft is specified with the third angle." );
		}
	}
	else
	{
		if( ImGui::CollapsingHeader( "Limit Type" ) )
		{
			ImGui::PushID( "LimitType" );
			ImGui::Columns( 2, "limit2" );
			changed = ImGui::Combo( "Type", ( int* )&constraint->limit, ConstraintLimitTypeGetter, nullptr, ARRAY_COUNT( constraintLimitTypeNames ) - 1 ) || changed;
			ImGui::NextColumn();
			switch( constraint->limit )
			{
				case idDeclAF_Constraint::LIMIT_NONE:
					break;
				case idDeclAF_Constraint::LIMIT_CONE:
					changed = ImGui::DragFloat3( "Cone##Limit", ( float* )&constraint->limitAngles[0], 0, -359.0f, 360.0f ) || changed;
					break;
				case idDeclAF_Constraint::LIMIT_PYRAMID:
					changed = ImGui::DragFloat3( "Pyramid##Limit", ( float* )&constraint->limitAngles[0], 0, -359.0f, 360.0f ) || changed;
					break;
			}
			ImGui::Columns( 1 );
			ImGui::PopID();
		}

		if( ImGui::CollapsingHeader( "Limit Orientation" ) )
		{
			ImGui::PushID( "LimitO" );
			ImGui::Columns( 2, "limitOrientation2" );
			changed = ImGui::Combo( "Type", ( int* )&constraint->limitAxis.type, afVecTypeNames, ARRAY_COUNT( afVecTypeNames ) ) || changed;
			ImGui::NextColumn();
			changed = InputAfVector( &constraint->limitAxis ) || changed;
			ImGui::Columns( 1 );
			ImGui::PopID();
		}
	}

	ImGui::PopID();

	return changed;
}

bool AfConstraintEditor::InputAfVector( idAFVector* afVec )
{
	bool changed = false;

	switch( afVec->type )
	{
		case idAFVector::VEC_COORDS:
			changed = changed || ImGui::DragFloat3( "Coordinates", ( float* )&afVec->ToVec3() );
			break;
		case idAFVector::VEC_JOINT:
			if( ImGui::BeginCombo( "Joint", afVec->joint1.c_str() ) )
			{
				for( int n = 0; n < joints.Num(); n++ )
				{
					bool is_selected = ( afVec->joint1 == joints[n] );
					if( ImGui::Selectable( joints[n], is_selected ) )
					{
						afVec->joint1 = joints[n];
						changed = true;
					}
					if( is_selected )
					{
						ImGui::SetItemDefaultFocus();    // Set the initial focus when opening the combo (scrolling + for keyboard navigation support in the upcoming navigation branch)
					}
				}
				ImGui::EndCombo();
			}
			break;
		case idAFVector::VEC_BONECENTER:
		case idAFVector::VEC_BONEDIR:
			if( ImGui::BeginCombo( "Joint 1", afVec->joint1.c_str() ) )
			{
				for( int n = 0; n < joints.Num(); n++ )
				{
					bool is_selected = ( afVec->joint1 == joints[n] );
					if( ImGui::Selectable( joints[n], is_selected ) )
					{
						afVec->joint1 = joints[n];
						changed = true;
					}
					if( is_selected )
					{
						ImGui::SetItemDefaultFocus();    // Set the initial focus when opening the combo (scrolling + for keyboard navigation support in the upcoming navigation branch)
					}
				}
				ImGui::EndCombo();
			}
			if( ImGui::BeginCombo( "Joint 2", afVec->joint2.c_str() ) )
			{
				for( int n = 0; n < joints.Num(); n++ )
				{
					bool is_selected = ( afVec->joint2 == joints[n] );
					if( ImGui::Selectable( joints[n], is_selected ) )
					{
						afVec->joint2 = joints[n];
						changed = true;
					}
					if( is_selected )
					{
						ImGui::SetItemDefaultFocus();    // Set the initial focus when opening the combo (scrolling + for keyboard navigation support in the upcoming navigation branch)
					}
				}
				ImGui::EndCombo();
			}
			break;
		default:
			ImGui::Text( "*Unknown Type*" );
	}

	return changed;
}

void AfConstraintEditor::InitJointLists()
{
	const idRenderModel* model = gameEdit->ANIM_GetModelFromName( decl->model.c_str() );
	if( !model )
	{
		return;
	}

	joints.Clear();

	const int numJoints = model->NumJoints();
	for( int i = 0; i < numJoints; i++ )
	{
		const char* jointName = model->GetJointName( ( jointHandle_t )i );
		joints[joints.Append( jointName )].ToLower();
	}
}

static bool BodyItemGetter( void* data, int index, const char** out )
{
	idDeclAF* decl = reinterpret_cast<idDeclAF*>( data );
	if( index < 0 || index >= decl->bodies.Num() )
	{
		return false;
	}
	idDeclAF_Body* body = decl->bodies[index];
	*out = body->name.c_str();
	return true;
}

bool ConstraintLimitTypeGetter( void* data, int index, const char** out )
{
	index += 1;

	if( index < 0 || index > ARRAY_COUNT( constraintLimitTypeNames ) )
	{
		*out = constraintLimitTypeNames[0];
		return false;
	}

	*out = constraintLimitTypeNames[index];
	return true;
}

}
