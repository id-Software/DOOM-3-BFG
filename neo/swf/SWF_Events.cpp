/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2016-2017 Dustin Land

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

/*
===================
idSWF::HitTest
===================
*/
idSWFScriptObject* idSWF::HitTest( idSWFSpriteInstance* spriteInstance, const swfRenderState_t& renderState, int x, int y, idSWFScriptObject* parentObject )
{

	if( spriteInstance->parent != NULL )
	{
		swfDisplayEntry_t* thisDisplayEntry = spriteInstance->parent->FindDisplayEntry( spriteInstance->depth );
		if( thisDisplayEntry->cxf.mul.w + thisDisplayEntry->cxf.add.w < 0.001f )
		{
			return NULL;
		}
	}

	if( !spriteInstance->isVisible )
	{
		return NULL;
	}

	if( spriteInstance->scriptObject->HasValidProperty( "onRelease" )
			|| spriteInstance->scriptObject->HasValidProperty( "onPress" )
			|| spriteInstance->scriptObject->HasValidProperty( "onRollOver" )
			|| spriteInstance->scriptObject->HasValidProperty( "onRollOut" )
			|| spriteInstance->scriptObject->HasValidProperty( "onDrag" )
	  )
	{
		parentObject = spriteInstance->scriptObject;
	}

	// rather than returning the first object we find, we actually want to return the last object we find
	idSWFScriptObject* returnObject = NULL;

	float xOffset = spriteInstance->xOffset;
	float yOffset = spriteInstance->yOffset;

	for( int i = 0; i < spriteInstance->displayList.Num(); i++ )
	{
		const swfDisplayEntry_t& display = spriteInstance->displayList[i];
		idSWFDictionaryEntry* entry = FindDictionaryEntry( display.characterID );
		if( entry == NULL )
		{
			continue;
		}
		swfRenderState_t renderState2;
		renderState2.matrix = display.matrix.Multiply( renderState.matrix );
		renderState2.ratio = display.ratio;

		if( entry->type == SWF_DICT_SPRITE )
		{
			idSWFScriptObject* object = HitTest( display.spriteInstance, renderState2, x, y, parentObject );
			if( object != NULL && object->Get( "_visible" ).ToBool() )
			{
				returnObject = object;
			}
		}
		else if( entry->type == SWF_DICT_SHAPE && ( parentObject != NULL ) )
		{
			idSWFShape* shape = entry->shape;
			for( int j = 0; j < shape->fillDraws.Num(); j++ )
			{
				const idSWFShapeDrawFill& fill = shape->fillDraws[j];
				for( int k = 0; k < fill.indices.Num(); k += 3 )
				{
					idVec2 xy1 = renderState2.matrix.Transform( fill.startVerts[fill.indices[k + 0]] );
					idVec2 xy2 = renderState2.matrix.Transform( fill.startVerts[fill.indices[k + 1]] );
					idVec2 xy3 = renderState2.matrix.Transform( fill.startVerts[fill.indices[k + 2]] );

					idMat3 edgeEquations;
					edgeEquations[0].Set( xy1.x + xOffset, xy1.y + yOffset, 1.0f );
					edgeEquations[1].Set( xy2.x + xOffset, xy2.y + yOffset, 1.0f );
					edgeEquations[2].Set( xy3.x + xOffset, xy3.y + yOffset, 1.0f );
					edgeEquations.InverseSelf();

					idVec3 p( x, y, 1.0f );
					idVec3 signs = p * edgeEquations;

					bool bx = signs.x > 0;
					bool by = signs.y > 0;
					bool bz = signs.z > 0;
					if( bx == by && bx == bz )
					{
						// point inside
						returnObject = parentObject;
					}
				}
			}
		}
		else if( entry->type == SWF_DICT_MORPH )
		{
			// FIXME: this should be roughly the same as SWF_DICT_SHAPE
		}
		else if( entry->type == SWF_DICT_TEXT )
		{
			// FIXME: this should be roughly the same as SWF_DICT_SHAPE
		}
		else if( entry->type == SWF_DICT_EDITTEXT )
		{
			idSWFScriptObject* editObject = NULL;

			if( display.textInstance->scriptObject.HasProperty( "onRelease" ) || display.textInstance->scriptObject.HasProperty( "onPress" ) )
			{
				// if the edit box itself can be clicked, then we want to return it when it's clicked on
				editObject = &display.textInstance->scriptObject;
			}
			else if( parentObject != NULL )
			{
				// otherwise, we want to return the parent object
				editObject = parentObject;
			}

			if( editObject == NULL )
			{
				continue;
			}

			if( display.textInstance->text.IsEmpty() )
			{
				continue;
			}

			const idSWFEditText* shape = entry->edittext;
			const idSWFEditText* text = display.textInstance->GetEditText();
			float textLength = display.textInstance->GetTextLength();

			float lengthDiff = fabs( shape->bounds.br.x - shape->bounds.tl.x ) - textLength;

			idVec3 tl;
			idVec3 tr;
			idVec3 br;
			idVec3 bl;

			float topOffset = 0.0f;

			if( text->align == SWF_ET_ALIGN_LEFT )
			{
				tl.ToVec2() = renderState2.matrix.Transform( idVec2( shape->bounds.tl.x  + xOffset, shape->bounds.tl.y + topOffset + yOffset ) );
				tr.ToVec2() = renderState2.matrix.Transform( idVec2( shape->bounds.br.x - lengthDiff + xOffset, shape->bounds.tl.y + topOffset + yOffset ) );
				br.ToVec2() = renderState2.matrix.Transform( idVec2( shape->bounds.br.x - lengthDiff + xOffset, shape->bounds.br.y + topOffset + yOffset ) );
				bl.ToVec2() = renderState2.matrix.Transform( idVec2( shape->bounds.tl.x + xOffset, shape->bounds.br.y + topOffset + yOffset ) );
			}
			else if( text->align == SWF_ET_ALIGN_RIGHT )
			{
				tl.ToVec2() = renderState2.matrix.Transform( idVec2( shape->bounds.tl.x + lengthDiff + xOffset, shape->bounds.tl.y + topOffset + yOffset ) );
				tr.ToVec2() = renderState2.matrix.Transform( idVec2( shape->bounds.br.x + xOffset, shape->bounds.tl.y + topOffset + yOffset ) );
				br.ToVec2() = renderState2.matrix.Transform( idVec2( shape->bounds.br.x + xOffset, shape->bounds.br.y + topOffset + yOffset ) );
				bl.ToVec2() = renderState2.matrix.Transform( idVec2( shape->bounds.tl.x + lengthDiff + xOffset, shape->bounds.br.y + topOffset + yOffset ) );
			}
			else if( text->align == SWF_ET_ALIGN_CENTER )
			{
				float middle = ( ( shape->bounds.br.x + xOffset ) + ( shape->bounds.tl.x + xOffset ) ) / 2.0f;
				tl.ToVec2() = renderState2.matrix.Transform( idVec2( middle - ( textLength / 2.0f ), shape->bounds.tl.y + topOffset + yOffset ) );
				tr.ToVec2() = renderState2.matrix.Transform( idVec2( middle + ( textLength / 2.0f ), shape->bounds.tl.y + topOffset + yOffset ) );
				br.ToVec2() = renderState2.matrix.Transform( idVec2( middle + ( textLength / 2.0f ), shape->bounds.br.y + topOffset + yOffset ) );
				bl.ToVec2() = renderState2.matrix.Transform( idVec2( middle - ( textLength / 2.0f ), shape->bounds.br.y + topOffset + yOffset ) );
			}
			else
			{
				tl.ToVec2() = renderState2.matrix.Transform( idVec2( shape->bounds.tl.x + xOffset, shape->bounds.tl.y + topOffset + yOffset ) );
				tr.ToVec2() = renderState2.matrix.Transform( idVec2( shape->bounds.br.x + xOffset, shape->bounds.tl.y + topOffset + yOffset ) );
				br.ToVec2() = renderState2.matrix.Transform( idVec2( shape->bounds.br.x + xOffset, shape->bounds.br.y + topOffset + yOffset ) );
				bl.ToVec2() = renderState2.matrix.Transform( idVec2( shape->bounds.tl.x + xOffset, shape->bounds.br.y + topOffset + yOffset ) );
			}

			tl.z = 1.0f;
			tr.z = 1.0f;
			br.z = 1.0f;
			bl.z = 1.0f;

			idMat3 edgeEquations;
			edgeEquations[0] = tl;
			edgeEquations[1] = tr;
			edgeEquations[2] = br;
			edgeEquations.InverseSelf();

			idVec3 p( x, y, 1.0f );
			idVec3 signs = p * edgeEquations;

			bool bx = signs.x > 0;
			bool by = signs.y > 0;
			bool bz = signs.z > 0;
			if( bx == by && bx == bz )
			{
				// point inside top right triangle
				returnObject = editObject;
			}

			edgeEquations[0] = tl;
			edgeEquations[1] = br;
			edgeEquations[2] = bl;
			edgeEquations.InverseSelf();
			signs = p * edgeEquations;

			bx = signs.x > 0;
			by = signs.y > 0;
			bz = signs.z > 0;
			if( bx == by && bx == bz )
			{
				// point inside bottom left triangle
				returnObject = editObject;
			}
		}
	}
	return returnObject;
}

/*
===================
idSWF::HandleEvent
===================
*/
bool idSWF::HandleEvent( const sysEvent_t* event )
{
	if( !IsLoaded() || !IsActive() || ( !inhibitControl && useInhibtControl ) )
	{
		return false;
	}
	if( event->evType == SE_KEY )
	{
		if( event->evValue == K_MOUSE1 )
		{
			mouseEnabled = true;
			idSWFScriptVar var;
			if( event->evValue2 )
			{

				idSWFScriptVar waitInput = globals->Get( "waitInput" );
				if( waitInput.IsFunction() )
				{
					useMouse = false;
					idSWFParmList waitParms;
					waitParms.Append( event->evValue );
					waitInput.GetFunction()->Call( NULL, waitParms );
					waitParms.Clear();
				}
				else
				{
					useMouse = true;
				}

				idSWFScriptObject* hitObject = HitTest( mainspriteInstance, swfRenderState_t(), mouseX, mouseY, NULL );
				if( hitObject != NULL )
				{
					mouseObject = hitObject;
					mouseObject->AddRef();

					var = hitObject->Get( "onPress" );
					if( var.IsFunction() )
					{
						idSWFParmList parms;
						parms.Append( event->inputDevice );
						var.GetFunction()->Call( hitObject, parms );
						parms.Clear();
						return true;
					}

					var = hitObject->Get( "onDrag" );
					if( var.IsFunction() )
					{
						idSWFParmList parms;
						parms.Append( mouseX );
						parms.Append( mouseY );
						parms.Append( true );
						var.GetFunction()->Call( hitObject, parms );
						parms.Clear();
						return true;
					}
				}

				idSWFParmList parms;
				parms.Append( hitObject );
				Invoke( "setHitObject", parms );

			}
			else
			{
				if( mouseObject )
				{
					var = mouseObject->Get( "onRelease" );
					if( var.IsFunction() )
					{
						idSWFParmList parms;
						parms.Append( mouseObject ); // FIXME: Remove this
						var.GetFunction()->Call( mouseObject, parms );
					}
					mouseObject->Release();
					mouseObject = NULL;
				}
				if( hoverObject )
				{
					hoverObject->Release();
					hoverObject = NULL;
				}

				if( var.IsFunction() )
				{
					return true;
				}
			}

			return false;
		}
		const char* keyName = idKeyInput::KeyNumToString( ( keyNum_t )event->evValue );
		idSWFScriptVar var = shortcutKeys->Get( keyName );
		// anything more than 32 levels of indirection we can be pretty sure is an infinite loop
		for( int runaway = 0; runaway < 32; runaway++ )
		{
			idSWFParmList eventParms;
			eventParms.Clear();
			eventParms.Append( event->inputDevice );
			if( var.IsString() )
			{
				// alias to another key
				var = shortcutKeys->Get( var.ToString() );
				continue;
			}
			else if( var.IsObject() )
			{
				// if this object is a sprite, send fake mouse events to it
				idSWFScriptObject* object = var.GetObject();
				// make sure we don't send an onRelease event unless we have already sent that object an onPress
				bool wasPressed = object->Get( "_pressed" ).ToBool();
				object->Set( "_pressed", event->evValue2 );
				if( event->evValue2 )
				{
					var = object->Get( "onPress" );
				}
				else if( wasPressed )
				{
					var = object->Get( "onRelease" );
				}
				if( var.IsFunction() )
				{
					var.GetFunction()->Call( object, eventParms );
					return true;
				}
			}
			else if( var.IsFunction() )
			{
				if( event->evValue2 )
				{
					// anonymous functions only respond to key down events
					var.GetFunction()->Call( NULL, eventParms );
					return true;
				}
				return false;
			}

			idSWFScriptVar useFunction = globals->Get( "useFunction" );
			if( useFunction.IsFunction() && event->evValue2 )
			{
				const char* action = idKeyInput::GetBinding( event->evValue );
				if( idStr::Cmp( "_use", action ) == 0 )
				{
					useFunction.GetFunction()->Call( NULL, idSWFParmList() );
				}
			}

			idSWFScriptVar waitInput = globals->Get( "waitInput" );
			if( waitInput.IsFunction() )
			{
				useMouse = false;
				if( event->evValue2 )
				{
					idSWFParmList waitParms;
					waitParms.Append( event->evValue );
					waitInput.GetFunction()->Call( NULL, waitParms );
				}
			}
			else
			{
				useMouse = true;
			}

			idSWFScriptVar focusWindow = globals->Get( "focusWindow" );
			if( focusWindow.IsObject() )
			{
				idSWFScriptVar onKey = focusWindow.GetObject()->Get( "onKey" );
				if( onKey.IsFunction() )
				{

					// make sure we don't send an onRelease event unless we have already sent that object an onPress
					idSWFScriptObject* object = focusWindow.GetObject();
					bool wasPressed = object->Get( "_kpressed" ).ToBool();
					object->Set( "_kpressed", event->evValue2 );
					if( event->evValue2 || wasPressed )
					{
						idSWFParmList parms;
						parms.Append( event->evValue );
						parms.Append( event->evValue2 );
						onKey.GetFunction()->Call( focusWindow.GetObject(), parms ).ToBool();
						return true;
					}
					else if( event->evValue == K_LSHIFT || event->evValue == K_RSHIFT )
					{
						idSWFParmList parms;
						parms.Append( event->evValue );
						parms.Append( event->evValue2 );
						onKey.GetFunction()->Call( focusWindow.GetObject(), parms ).ToBool();
					}
				}
			}
			return false;
		}
		idLib::Warning( "Circular reference in %s shortcutKeys.%s", filename.c_str(), keyName );
	}
	else if( event->evType == SE_CHAR )
	{
		idSWFScriptVar focusWindow = globals->Get( "focusWindow" );
		if( focusWindow.IsObject() )
		{
			idSWFScriptVar onChar = focusWindow.GetObject()->Get( "onChar" );
			if( onChar.IsFunction() )
			{
				idSWFParmList parms;
				parms.Append( event->evValue );
				parms.Append( idKeyInput::KeyNumToString( ( keyNum_t )event->evValue ) );
				onChar.GetFunction()->Call( focusWindow.GetObject(), parms ).ToBool();
				return true;
			}
		}
	}
	else if( event->evType == SE_MOUSE_ABSOLUTE || event->evType == SE_MOUSE )
	{
		mouseEnabled = true;
		isMouseInClientArea = true;

		// Mouse position in screen space needs to be converted to SWF space
		if( event->evType == SE_MOUSE_ABSOLUTE )
		{
			const float pixelAspect = renderSystem->GetPixelAspect();
			const float sysWidth = renderSystem->GetWidth() * ( pixelAspect > 1.0f ? pixelAspect : 1.0f );
			const float sysHeight = renderSystem->GetHeight() / ( pixelAspect < 1.0f ? pixelAspect : 1.0f );
			float scale = swfScale * sysHeight / ( float )frameHeight;
			float invScale = 1.0f / scale;
			float tx = 0.5f * ( sysWidth - ( frameWidth * scale ) );
			float ty = 0.5f * ( sysHeight - ( frameHeight * scale ) );

			mouseX = idMath::Ftoi( ( static_cast<float>( event->evValue ) - tx ) * invScale );
			mouseY = idMath::Ftoi( ( static_cast<float>( event->evValue2 ) - ty ) * invScale );
		}
		else
		{

			mouseX += event->evValue;
			mouseY += event->evValue2;

			mouseX = Max( Min( mouseX, idMath::Ftoi( frameWidth + renderBorder ) ), idMath::Ftoi( 0.0f - renderBorder ) );
			mouseY = Max( Min( mouseY, idMath::Ftoi( frameHeight ) ), 0 );
		}

		bool retVal = false;

		idSWFScriptObject* hitObject = HitTest( mainspriteInstance, swfRenderState_t(), mouseX, mouseY, NULL );
		if( hitObject != NULL )
		{
			hasHitObject = true;
		}
		else
		{
			hasHitObject = false;
		}

		if( hitObject != hoverObject )
		{
			// First check to see if we should call onRollOut on our previous hoverObject
			if( hoverObject != NULL )
			{
				idSWFScriptVar var = hoverObject->Get( "onRollOut" );
				if( var.IsFunction() )
				{
					var.GetFunction()->Call( hoverObject, idSWFParmList() );
					retVal = true;
				}
				hoverObject->Release();
				hoverObject = NULL;
			}
			// Then call onRollOver on our hitObject
			if( hitObject != NULL )
			{
				hoverObject = hitObject;
				hoverObject->AddRef();
				idSWFScriptVar var = hitObject->Get( "onRollOver" );
				if( var.IsFunction() )
				{
					var.GetFunction()->Call( hitObject, idSWFParmList() );
					retVal = true;
				}
			}
		}
		if( mouseObject != NULL )
		{
			idSWFScriptVar var = mouseObject->Get( "onDrag" );
			if( var.IsFunction() )
			{
				idSWFParmList parms;
				parms.Append( mouseX );
				parms.Append( mouseY );
				parms.Append( false );
				var.GetFunction()->Call( mouseObject, parms );
				return true;
			}
		}
		return retVal;
	}
	else if( event->evType == SE_MOUSE_LEAVE )
	{
		isMouseInClientArea = false;
	}
	else if( event->evType == SE_JOYSTICK )
	{
		idSWFParmList parms;
		parms.Append( event->evValue );
		parms.Append( event->evValue2 / 32.0f );
		Invoke( "onJoystick", parms );
	}
	return false;
}
