/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.
Copyright (C) 2015 Daniel Gibson
Copyright (C) 2020-2021 Robert Beckebans

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

#include "LightEditor.h"

#include "../imgui/BFGimgui.h"

#include "renderer/Material.h"
#include "renderer/Image.h"

namespace ImGuiTools
{

void LightInfo::Defaults()
{
	pointLight = true;
	fallOff = 1.0f;
	strTexture = "";
	equalRadius = true;
	explicitStartEnd = false;
	lightStart.Zero();
	lightEnd.Zero();
	lightUp.Zero();
	lightRight.Zero();
	lightTarget.Zero();
	lightCenter.Zero();
	color[0] = color[1] = color[2] = 1.0f;

#if 0 // FIXME: unused, delete?
	fog = false;
	fogDensity.Zero();

	strobe = false;
	strobeSpeed = 0.0f;
	rotate = false;
	rotateSpeed = 0.0f;
#endif // 0

	lightRadius.Zero();
	castShadows = true;
	castSpecular = true;
	hasCenter = false;
	isParallel = false;
	lightStyle = -1;
}


void LightInfo::DefaultPoint()
{
	idVec3 oldColor = color;
	Defaults();
	color = oldColor;
	pointLight = true;
}

void LightInfo::DefaultProjected()
{
	idVec3 oldColor = color;
	Defaults();
	color = oldColor;
	pointLight = false;
	lightTarget[2] = -256;
	lightUp[1] = -128;
	lightRight[0] = -128;
}

void LightInfo::FromDict( const idDict* e )
{
	lightRadius.Zero();
	lightTarget.Zero();
	lightRight.Zero();
	lightUp.Zero();
	lightStart.Zero();
	lightEnd.Zero();
	lightCenter.Zero();

	castShadows = !e->GetBool( "noshadows" );
	castSpecular = !e->GetBool( "nospecular" );
	fallOff = e->GetFloat( "falloff" );
	strTexture = e->GetString( "texture" );

	isParallel = e->GetBool( "parallel" );

	if( !e->GetVector( "_color", "", color ) )
	{
		color[0] = color[1] = color[2] = 1.0f;
		// NOTE: like the game, imgui uses color values between 0.0 and 1.0
		//       even though it displays them as 0 to 255
	}

#if 0 // FIXME: unused, delete?
	if( e->GetVec4( "fog", "", fogDensity ) )
	{
		fog = true;
	}
	else
	{
		fog = false;
	}
#endif // 0

	if( e->GetVector( "light_right", "", lightRight ) )
	{
		// projected light
		pointLight = false;
		e->GetVector( "light_target", "", lightTarget );
		e->GetVector( "light_up", "", lightUp );
		if( e->GetVector( "light_start", "", lightStart ) )
		{
			// explicit start and end points
			explicitStartEnd = true;
			if( !e->GetVector( "light_end", "", lightEnd ) )
			{
				// no end, use target
				lightEnd = lightTarget;
			}
		}
		else
		{
			explicitStartEnd = false;
			// create a start a quarter of the way to the target
			lightStart = lightTarget * 0.25;
			lightEnd = lightTarget;
		}
	}
	else
	{
		pointLight = true;
		if( e->GetVector( "light_radius", "", lightRadius ) )
		{
			equalRadius = ( lightRadius.x == lightRadius.y && lightRadius.x == lightRadius.z );
		}
		else
		{
			float radius = e->GetFloat( "light" );
			if( radius == 0 )
			{
				radius = 300;
			}
			lightRadius[0] = lightRadius[1] = lightRadius[2] = radius;
			equalRadius = true;
		}
		if( e->GetVector( "light_center", "", lightCenter ) )
		{
			hasCenter = true;
		}
	}

	// RB: Quake 1 light styles
	lightStyle = e->GetInt( "style", -1 );
}

// the returned idDict is supposed to be used by idGameEdit::EntityChangeSpawnArgs()
// and thus will contain pairs with value "" if the key should be removed from entity
void LightInfo::ToDict( idDict* e )
{
	// idGameEdit::EntityChangeSpawnArgs() will delete key/value from entity,
	// if value is "" => use DELETE_VAL for readability
	static const char* DELETE_VAL = "";

	e->Set( "light", DELETE_VAL ); // we always use "light_radius" instead

	e->Set( "noshadows", ( !castShadows ) ? "1" : "0" );
	e->Set( "nospecular", ( !castSpecular ) ? "1" : "0" );

	e->SetFloat( "falloff", fallOff );

	if( strTexture.Length() > 0 )
	{
		e->Set( "texture", strTexture );
	}
	else
	{
		e->Set( "texture", DELETE_VAL );
	}

	e->Set( "_color", color.ToString( 4 ) ); // NOTE: e->SetVector() uses precision of 2, not enough for color

#if 0 // DG: I think this isn't used
	if( fog )
	{
		idVec4 tmp( fogDensity );
		tmp *= 1.0f / 255.0f; // TODO: why not just make fogdensity values between 0 and 1 to start with?
		e->SetVec4( "fog", tmp );
	}
#endif // 0

	if( pointLight )
	{
		if( !equalRadius )
		{
			e->SetVector( "light_radius", lightRadius );
		}
		else
		{
			idVec3 tmp( lightRadius[0] ); // x, y and z have the same value
			e->SetVector( "light_radius", tmp );
		}

		if( hasCenter )
		{
			e->SetVector( "light_center", lightCenter );
		}
		else
		{
			e->Set( "light_center", DELETE_VAL );
		}

		e->Set( "parallel", isParallel ? "1" : DELETE_VAL );

		// get rid of all the projected light specific stuff
		e->Set( "light_target", DELETE_VAL );
		e->Set( "light_up", DELETE_VAL );
		e->Set( "light_right", DELETE_VAL );
		e->Set( "light_start", DELETE_VAL );
		e->Set( "light_end", DELETE_VAL );
	}
	else
	{
		e->SetVector( "light_target", lightTarget );
		e->SetVector( "light_up", lightUp );
		e->SetVector( "light_right", lightRight );
		if( explicitStartEnd )
		{
			e->SetVector( "light_start", lightStart );
			e->SetVector( "light_end", lightEnd );
		}
		else
		{
			e->Set( "light_start", DELETE_VAL );
			e->Set( "light_end", DELETE_VAL );
		}

		// get rid of the pointlight specific stuff
		e->Set( "light_radius", DELETE_VAL );
		e->Set( "light_center", DELETE_VAL );
		e->Set( "parallel", DELETE_VAL );
	}

	// RB: Quake 1 light styles
	if( lightStyle != -1 )
	{
		e->SetInt( "style", lightStyle );
	}
	else
	{
		e->Set( "style", DELETE_VAL );
	}
}

LightInfo::LightInfo()
{
	Defaults();
}


// ########### LightEditor #############

// static
LightEditor LightEditor::TheLightEditor;

// static
bool LightEditor::showIt = false;

// static
void LightEditor::ReInit( const idDict* dict, idEntity* light )
{
	// TODO: if the lighteditor is currently shown, show a warning first about saving current changes to the last light?
	TheLightEditor.Init( dict, light );
}

// static
void LightEditor::Draw()
{
	TheLightEditor.DrawWindow();
}

void LightEditor::Init( const idDict* dict, idEntity* light )
{
	Reset();

	if( textureNames.Num() == 0 )
	{
		LoadLightTextures();
	}

	if( styleNames.Num() == 0 )
	{
		LoadLightStyles();
	}

	if( dict )
	{
		original.FromDict( dict );
		cur.FromDict( dict );

		gameEdit->EntityGetOrigin( light, entityPos );

		const char* name = dict->GetString( "name", NULL );
		if( name )
		{
			entityName = name;
			title.Format( "Light Editor: %s at (%s)", name, entityPos.ToString() );
		}
		else
		{
			//idassert( 0 && "LightEditor::Init(): Given entity has no 'name' property?!" );
			entityName = ""; // TODO: generate name or handle gracefully or something?
			title.Format( "Light Editor: <unnamed> light at (%s)", entityPos.ToString() );
		}

		currentTextureIndex = 0;
		currentTexture = NULL;
		if( original.strTexture.Length() > 0 )
		{
			const char* curTex = original.strTexture.c_str();
			for( int i = 0; i < textureNames.Num(); ++i )
			{
				if( textureNames[i] == curTex )
				{
					currentTextureIndex = i + 1; // remember, 0 is "<No Texture>"
					LoadCurrentTexture();
					break;
				}
			}
		}

		// RB: light styles
		if( original.lightStyle >= 0 )
		{
			currentStyleIndex = original.lightStyle + 1;
		}
	}

	this->lightEntity = light;
}

void LightEditor::Reset()
{
	title = "Light Editor: no Light selected!";
	entityPos.x = idMath::INFINITUM;
	entityPos.y = idMath::INFINITUM;
	entityPos.z = idMath::INFINITUM;

	original.Defaults();
	cur.Defaults();

	lightEntity = NULL;
	currentTextureIndex = 0;
	currentTexture = NULL;
	currentTextureMaterial = NULL;
	currentStyleIndex = 0;
}

namespace
{
class idSort_textureNames : public idSort_Quick< idStr, idSort_textureNames >
{
public:
	int Compare( const idStr& a, const idStr& b ) const
	{
		return a.Icmp( b );
	}
};
} //anon. namespace

void LightEditor::LoadLightTextures()
{
	textureNames.Clear();

	int count = declManager->GetNumDecls( DECL_MATERIAL );

	for( int i = 0; i < count; i++ )
	{
		// just get the name of the light material
		const idMaterial* mat = declManager->MaterialByIndex( i, false );

		idStr matName = mat->GetName();
		matName.ToLower(); // FIXME: why? (this is from old doom3 code)

		if( matName.Icmpn( "lights/", strlen( "lights/" ) ) == 0 || matName.Icmpn( "fogs/", strlen( "fogs/" ) ) == 0 )
		{
			// actually load the material
			const idMaterial* material = declManager->FindMaterial( matName, false );
			if( material != NULL )
			{
				// check if the material has textures or is just a leftover from the development
				idImage* editorImage = mat->GetLightEditorImage();
				if( !editorImage->IsLoaded() )
				{
					editorImage->DeferredLoadImage();
				}

				if( !editorImage->IsDefaulted() )
				{
					textureNames.Append( matName );
				}
			}
		}
	}

	textureNames.SortWithTemplate( idSort_textureNames() );
}

// static
bool LightEditor::TextureItemsGetter( void* data, int idx, const char** outText )
{
	LightEditor* self = static_cast<LightEditor*>( data );
	if( idx == 0 )
	{
		*outText = "<No Texture>";
		return true;
	}

	// as index 0 has special purpose, the "real" index is one less
	--idx;

	if( idx < 0 || idx >= self->textureNames.Num() )
	{
		*outText = "<Invalid Index!>";
		return false;
	}

	*outText = self->textureNames[idx].c_str();

	return true;
}

void LightEditor::LoadCurrentTexture()
{
	currentTexture = NULL;

	if( currentTextureIndex > 0 && cur.strTexture.Length() > 0 )
	{
		const idMaterial* mat = declManager->FindMaterial( cur.strTexture, false );
		if( mat != NULL )
		{
			currentTexture = mat->GetLightEditorImage();
			if( currentTexture )
			{
				// RB: create extra 2D material of the image for UI rendering

				// HACK that deserves being called a hack
				idStr uiName( "lighteditor/" );
				uiName += currentTexture->GetName();

				currentTextureMaterial = declManager->FindMaterial( uiName, true );
			}
		}
	}
}

void LightEditor::LoadLightStyles()
{
	styleNames.Clear();

	const idDeclEntityDef* decl = static_cast<const idDeclEntityDef*>( declManager->FindType( DECL_ENTITYDEF, "light", false ) );
	if( decl == NULL )
	{
		return;
	}

	int numStyles = decl->dict.GetInt( "num_styles", "0" );
	if( numStyles > 0 )
	{
		for( int i = 0; i < numStyles; i++ )
		{
			idStr style = decl->dict.GetString( va( "light_style%d", i ) );
			styleNames.Append( style );
		}
	}
	else
	{
		// RB: it's not defined in entityDef light so use predefined Quake 1 table
		for( int i = 0; i < 12; i++ )
		{
			idStr style( predef_lightstylesinfo[ i ] );
			styleNames.Append( style );
		}
	}
}

// static
bool LightEditor::StyleItemsGetter( void* data, int idx, const char** outText )
{
	LightEditor* self = static_cast<LightEditor*>( data );
	if( idx == 0 )
	{
		*outText = "<No Lightstyle>";
		return true;
	}

	// as index 0 has special purpose, the "real" index is one less
	--idx;

	if( idx < 0 || idx >= self->styleNames.Num() )
	{
		*outText = "<Invalid Index!>";
		return false;
	}

	*outText = self->styleNames[idx].c_str();

	return true;
}

void LightEditor::TempApplyChanges()
{
	if( lightEntity != NULL )
	{
		idDict d;
		cur.ToDict( &d );

		gameEdit->EntityChangeSpawnArgs( lightEntity, &d );
		gameEdit->EntityUpdateChangeableSpawnArgs( lightEntity, NULL );
	}
}

void LightEditor::SaveChanges()
{
	idDict d;
	cur.ToDict( &d );
	if( entityName[0] != '\0' )
	{
		gameEdit->MapCopyDictToEntity( entityName, &d );
	}
	else if( entityPos.x != idMath::INFINITUM )
	{
		entityName = gameEdit->GetUniqueEntityName( "light" );
		d.Set( "name", entityName );

		// RB: this is really HACKY
		gameEdit->MapCopyDictToEntityAtOrigin( entityPos, &d );
	}

	gameEdit->MapSave();
}

void LightEditor::CancelChanges()
{
	if( lightEntity != NULL )
	{
		idDict d;
		original.ToDict( &d );

		gameEdit->EntityChangeSpawnArgs( lightEntity, &d );
		gameEdit->EntityUpdateChangeableSpawnArgs( lightEntity, NULL );
	}
}

// a kinda ugly hack to get a float* (as used by imgui) from idVec3
static float* vecToArr( idVec3& v )
{
	return &v.x;
}

void LightEditor::DrawWindow()
{
	bool showWindow = showIt;
	if( ImGui::Begin( title, &showWindow ) ) //, ImGuiWindowFlags_ShowBorders ) )
	{
		bool changes = false;

		changes |= ImGui::Checkbox( "Cast Shadows", &cur.castShadows );
		//ImGui::SameLine();
		changes |= ImGui::Checkbox( "Cast Specular", &cur.castSpecular );

		ImGui::Spacing();

		changes |= ImGui::ColorEdit3( "Color", vecToArr( cur.color ) );

		// TODO: fog, fogDensity - probably unused!

		ImGui::Spacing();

		int lightSelectionRadioBtn = cur.pointLight ? 0 : 1;

		changes |= ImGui::RadioButton( "Point Light", &lightSelectionRadioBtn, 0 );
		ImGui::SameLine();
		changes |= ImGui::RadioButton( "Projected Light", &lightSelectionRadioBtn, 1 );

		cur.pointLight = ( lightSelectionRadioBtn == 0 );

		ImGui::Indent();

		ImGui::Spacing();

		if( lightSelectionRadioBtn == 0 )
		{
			ImGui::PushItemWidth( -1.0f ); // align end of Drag* with right window border

			changes |= ImGui::Checkbox( "Equilateral Radius", &cur.equalRadius );
			ImGui::Text( "Radius:" );
			ImGui::Indent();
			if( cur.equalRadius )
			{
				if( ImGui::DragFloat( "##radEquil", &cur.lightRadius.x, 1.0f, 0.0f, 10000.0f, "%.1f" ) )
				{
					cur.lightRadius.z = cur.lightRadius.y = cur.lightRadius.x;
					changes = true;
				}
			}
			else
			{
				changes |= ImGui::DragVec3( "##radXYZ", cur.lightRadius );
			}
			ImGui::Unindent();

			ImGui::Spacing();

			// TODO: this could as well be a slider or something, if 0/0.5/1 is too restricting

			ImGui::Text( "Fall-off:" );
			ImGui::SameLine();
#if 0
			ImGui::RadioButton( "0.0", &fallOffRadio, 0 );
			ImGui::SameLine();
			ImGui::RadioButton( "0.5", &fallOffRadio, 1 );
			ImGui::SameLine();
			ImGui::RadioButton( "1.0", &fallOffRadio, 2 );
#endif // 0

			// a slider is easier than radiobuttons.. does it really have to be radiobuttons?
			changes |= ImGui::SliderFloat( "##FallOff", &cur.fallOff, 0.0f, 1.0f, "%.1f" );

			ImGui::Spacing();

			changes |= ImGui::Checkbox( "Parallel", &cur.isParallel );

			ImGui::Spacing();

			changes |= ImGui::Checkbox( "Center", &cur.hasCenter );
			if( cur.hasCenter )
			{
				ImGui::Indent();
				changes |= ImGui::DragVec3( "##centerXYZ", cur.lightCenter, 1.0f, 0.0f, 10000.0f, "%.1f" );
				ImGui::Unindent();
			}
			ImGui::PopItemWidth(); // back to default alignment on right side
		}
		else if( lightSelectionRadioBtn == 1 )
		{
			changes |= ImGui::DragVec3( "Target", cur.lightTarget, 1.0f, 0.0f, 0.0f, "%.1f" );
			changes |= ImGui::DragVec3( "Right", cur.lightRight, 1.0f, 0.0f, 0.0f, "%.1f" );
			changes |= ImGui::DragVec3( "Up", cur.lightUp, 1.0f, 0.0f, 0.0f, "%.1f" );

			ImGui::Spacing();

			changes |= ImGui::Checkbox( "Explicit start/end points", &cur.explicitStartEnd );

			ImGui::Spacing();
			if( cur.explicitStartEnd )
			{
				changes |= ImGui::DragVec3( "Start", cur.lightStart, 1.0f, 0.0f, 0.0f, "%.1f" );
				changes |= ImGui::DragVec3( "End", cur.lightEnd, 1.0f, 0.0f, 0.0f, "%.1f" );
			}
		}

		ImGui::Unindent();

		if( ImGui::Combo( "Style", &currentStyleIndex, StyleItemsGetter, this, styleNames.Num() + 1 ) )
		{
			changes = true;

			// -1 because 0 is "<No Lightstyle>"
			cur.lightStyle = ( currentStyleIndex > 0 ) ? currentStyleIndex - 1 : -1;
		}

		ImGui::Spacing();

		if( ImGui::Combo( "Texture", &currentTextureIndex, TextureItemsGetter, this, textureNames.Num() + 1 ) )
		{
			changes = true;

			// -1 because 0 is "<No Texture>"
			cur.strTexture = ( currentTextureIndex > 0 ) ? textureNames[currentTextureIndex - 1] : "";
			LoadCurrentTexture();
		}

		if( currentTextureMaterial != nullptr && currentTexture != nullptr )
		{
			ImVec2 size( currentTexture->GetUploadWidth(), currentTexture->GetUploadHeight() );

#if !IMGUI_BFGUI
			ImGui::Image( ( void* )currentTexture->GetImGuiTextureID(), size, ImVec2( 0, 0 ), ImVec2( 1, 1 ),
#else
			ImGui::Image( ( void* )currentTextureMaterial, size, ImVec2( 0, 0 ), ImVec2( 1, 1 ),
#endif
						  ImColor( 255, 255, 255, 255 ), ImColor( 255, 255, 255, 128 ) );
		}

		// TODO: allow multiple lights selected at the same time + "apply different" button?
		//       then only the changed attribute (e.g. color) would be set to all lights,
		//       but they'd keep their other individual properties (eg radius)

		ImGui::Spacing();

		if( ImGui::Button( "Save to .map" ) )
		{
			SaveChanges();
			showWindow = false;
		}
		else if( ImGui::SameLine(), ImGui::Button( "Cancel" ) )
		{
			CancelChanges();
			showWindow = false;
		}
		else if( changes )
		{
			TempApplyChanges();
		}
	}
	ImGui::End();

	if( showIt && !showWindow )
	{
		// TODO: do the same as when pressing cancel?
		showIt = showWindow;
		impl::SetReleaseToolMouse( false );
	}
}

} //namespace ImGuiTools
