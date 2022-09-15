/*
 * ImGui integration into Doom3BFG/OpenTechEngine.
 * Based on ImGui SDL and OpenGL3 examples.
 *  Copyright (c) 2014-2015 Omar Cornut and ImGui contributors
 *
 * Doom3-specific Code (and ImGui::DragXYZ(), based on ImGui::DragFloatN())
 *  Copyright (C) 2015 Daniel Gibson
 *
 * This file is under MIT License, like the original code from ImGui.
 */

#include "precompiled.h"
#pragma hdrstop

#include "BFGimgui.h"
#include "renderer/RenderCommon.h"
#include "renderer/RenderBackend.h"


idCVar imgui_showDemoWindow( "imgui_showDemoWindow", "0", CVAR_GUI | CVAR_BOOL, "show big ImGui demo window" );

// our custom ImGui functions from BFGimgui.h

// like DragFloat3(), but with "X: ", "Y: " or "Z: " prepended to each display_format, for vectors
// if !ignoreLabelWidth, it makes sure the label also fits into the current item width.
//    note that this screws up alignment with consecutive "value+label widgets" (like Drag* or ColorEdit*)
bool ImGui::DragVec3( const char* label, idVec3& v, float v_speed,
					  float v_min, float v_max, const char* display_format, float power, bool ignoreLabelWidth )
{
	bool value_changed = false;
	ImGui::BeginGroup();
	ImGui::PushID( label );

	ImGuiStyle& style = ImGui::GetStyle();
	float wholeWidth = ImGui::CalcItemWidth() - 2.0f * style.ItemSpacing.x;
	float spacing = style.ItemInnerSpacing.x;
	float labelWidth = ignoreLabelWidth ? 0.0f : ( ImGui::CalcTextSize( label, NULL, true ).x + spacing );
	float coordWidth = ( wholeWidth - labelWidth - 2.0f * spacing ) * ( 1.0f / 3.0f ); // width of one x/y/z dragfloat

	ImGui::PushItemWidth( coordWidth );
	for( int i = 0; i < 3; i++ )
	{
		ImGui::PushID( i );
		char format[64];
		idStr::snPrintf( format, sizeof( format ), "%c: %s", "XYZ"[i], display_format );
		value_changed |= ImGui::DragFloat( "##v", &v[i], v_speed, v_min, v_max, format, power );

		ImGui::PopID();
		ImGui::SameLine( 0.0f, spacing );
	}
	ImGui::PopItemWidth();
	ImGui::PopID();

	const char* labelEnd = strstr( label, "##" );
	ImGui::TextUnformatted( label, labelEnd );

	ImGui::EndGroup();

	return value_changed;
}

// shortcut for DragXYZ with ignorLabelWidth = false
// very similar, but adjusts width to width of label to make sure it's not cut off
// sometimes useful, but might not align with consecutive "value+label widgets" (like Drag* or ColorEdit*)
bool ImGui::DragVec3fitLabel( const char* label, idVec3& v, float v_speed,
							  float v_min, float v_max, const char* display_format, float power )
{
	return ImGui::DragVec3( label, v, v_speed, v_min, v_max, display_format, power, false );
}

// the ImGui hooks to integrate it into the engine



namespace ImGuiHook
{

namespace
{

bool	g_IsInit = false;
double	g_Time = 0.0f;
bool	g_MousePressed[5] = { false, false, false, false, false };
float	g_MouseWheel = 0.0f;
ImVec2	g_MousePos = ImVec2( -1.0f, -1.0f ); //{-1.0f, -1.0f};
ImVec2	g_DisplaySize = ImVec2( 0.0f, 0.0f ); //{0.0f, 0.0f};



bool g_haveNewFrame = false;

bool HandleKeyEvent( const sysEvent_t& keyEvent )
{
	assert( keyEvent.evType == SE_KEY );

	keyNum_t keyNum = static_cast<keyNum_t>( keyEvent.evValue );
	bool pressed = keyEvent.evValue2 > 0;

	ImGuiIO& io = ImGui::GetIO();

	if( keyNum < K_JOY1 )
	{
		// keyboard input as direct input scancodes
		io.KeysDown[keyNum] = pressed;

		io.KeyAlt = usercmdGen->KeyState( K_LALT ) == 1 || usercmdGen->KeyState( K_RALT ) == 1;
		io.KeyCtrl = usercmdGen->KeyState( K_LCTRL ) == 1 || usercmdGen->KeyState( K_RCTRL ) == 1;
		io.KeyShift = usercmdGen->KeyState( K_LSHIFT ) == 1 || usercmdGen->KeyState( K_RSHIFT ) == 1;

		return true;
	}
	else if( keyNum >= K_MOUSE1 && keyNum <= K_MOUSE5 )
	{
		int buttonIdx = keyNum - K_MOUSE1;

		// K_MOUSE* are contiguous, so they can be used as indexes into imgui's
		// g_MousePressed[] - imgui even uses the same order (left, right, middle, X1, X2)
		g_MousePressed[buttonIdx] = pressed;

		return true; // let's pretend we also handle mouse up events
	}

	return false;
}

// Gross hack. I'm sorry.
// sets the kb-layout specific keys in the keymap
void FillCharKeys( int* keyMap )
{
	// set scancodes as default values in case the real keys aren't found below
	keyMap[ImGuiKey_A] = K_A;
	keyMap[ImGuiKey_C] = K_C;
	keyMap[ImGuiKey_V] = K_V;
	keyMap[ImGuiKey_X] = K_X;
	keyMap[ImGuiKey_Y] = K_Y;
	keyMap[ImGuiKey_Z] = K_Z;

	// try all probable keys for whether they're ImGuiKey_A/C/V/X/Y/Z
	for( int k = K_1; k < K_RSHIFT; ++k )
	{
		const char* kn = idKeyInput::LocalizedKeyName( ( keyNum_t )k );
		if( kn[0] == '\0' || kn[1] != '\0' || kn[0] == '?' )
		{
			// if the key wasn't found or the name has more than one char,
			// it's not what we're looking for.
			continue;
		}
		switch( kn[0] )
		{
			case 'a': // fall-through
			case 'A':
				keyMap [ImGuiKey_A] = k;
				break;
			case 'c': // fall-through
			case 'C':
				keyMap [ImGuiKey_C] = k;
				break;

			case 'v': // fall-through
			case 'V':
				keyMap [ImGuiKey_V] = k;
				break;

			case 'x': // fall-through
			case 'X':
				keyMap [ImGuiKey_X] = k;
				break;

			case 'y': // fall-through
			case 'Y':
				keyMap [ImGuiKey_Y] = k;
				break;

			case 'z': // fall-through
			case 'Z':
				keyMap [ImGuiKey_Z] = k;
				break;
		}
	}
}

// Sys_GetClipboardData() expects that you Mem_Free() its returned data
// ImGui can't do that, of course, so copy it into a static buffer here,
// Mem_Free() and return the copy
const char* GetClipboardText( void* )
{
	char* txt = Sys_GetClipboardData();
	if( txt == NULL )
	{
		return NULL;
	}

	static idStr clipboardBuf;
	clipboardBuf = txt;

	Mem_Free( txt );

	return clipboardBuf.c_str();
}

void SetClipboardText( void*, const char* text )
{
	Sys_SetClipboardData( text );
}


bool ShowWindows()
{
	return ( ImGuiTools::AreEditorsActive() || imgui_showDemoWindow.GetBool() || com_showFPS.GetInteger() > 1 );
}

bool UseInput()
{
	return ImGuiTools::ReleaseMouseForTools();
}

} //anon namespace

bool Init( int windowWidth, int windowHeight )
{
	if( IsInitialized() )
	{
		Destroy();
	}

	IMGUI_CHECKVERSION();

	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	// Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array.
	io.KeyMap[ImGuiKey_Tab] = K_TAB;
	io.KeyMap[ImGuiKey_LeftArrow] = K_LEFTARROW;
	io.KeyMap[ImGuiKey_RightArrow] = K_RIGHTARROW;
	io.KeyMap[ImGuiKey_UpArrow] = K_UPARROW;
	io.KeyMap[ImGuiKey_DownArrow] = K_DOWNARROW;
	io.KeyMap[ImGuiKey_PageUp] = K_PGUP;
	io.KeyMap[ImGuiKey_PageDown] = K_PGDN;
	io.KeyMap[ImGuiKey_Home] = K_HOME;
	io.KeyMap[ImGuiKey_End] = K_END;
	io.KeyMap[ImGuiKey_Delete] = K_DEL;
	io.KeyMap[ImGuiKey_Backspace] = K_BACKSPACE;
	io.KeyMap[ImGuiKey_Enter] = K_ENTER;
	io.KeyMap[ImGuiKey_Escape] = K_ESCAPE;

	FillCharKeys( io.KeyMap );

	g_DisplaySize.x = windowWidth;
	g_DisplaySize.y = windowHeight;
	io.DisplaySize = g_DisplaySize;

	// RB: FIXME double check
	io.SetClipboardTextFn = SetClipboardText;
	io.GetClipboardTextFn = GetClipboardText;
	io.ClipboardUserData = NULL;

	// make it a bit prettier with rounded edges
	ImGuiStyle& style = ImGui::GetStyle();
	//style.ChildWindowRounding = 9.0f;
	//style.FrameRounding = 4.0f;
	//style.ScrollbarRounding = 4.0f;
	//style.GrabRounding = 4.0f;

	// Setup style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	g_IsInit = true;

	return true;
}

void NotifyDisplaySizeChanged( int width, int height )
{
	if( g_DisplaySize.x != width || g_DisplaySize.y != height )
	{
		g_DisplaySize = ImVec2( ( float )width, ( float )height );

		if( IsInitialized() )
		{
			Destroy();
			Init( width, height );

			// reuse the default ImGui font
			const idMaterial* image = declManager->FindMaterial( "_imguiFont" );

			ImGuiIO& io = ImGui::GetIO();

			byte* pixels = NULL;
			io.Fonts->GetTexDataAsRGBA32( &pixels, &width, &height );

			io.Fonts->TexID = ( void* )image;
		}
	}
}

// inject a sys event
bool InjectSysEvent( const sysEvent_t* event )
{
	if( IsInitialized() && UseInput() )
	{
		if( event == NULL )
		{
			assert( 0 ); // I think this shouldn't happen
			return false;
		}

		const sysEvent_t& ev = *event;

		switch( ev.evType )
		{
			case SE_KEY:
				return HandleKeyEvent( ev );

			case SE_MOUSE_ABSOLUTE:
				g_MousePos.x = ev.evValue;
				g_MousePos.y = ev.evValue2;
				return true;

			case SE_CHAR:
				if( ev.evValue < 0x10000 )
				{
					ImGui::GetIO().AddInputCharacter( ev.evValue );
					return true;
				}
				break;

			case SE_MOUSE_LEAVE:
				g_MousePos = ImVec2( -1.0f, -1.0f );
				return true;

			default:
				break;
		}
	}
	return false;
}

bool InjectMouseWheel( int delta )
{
	if( IsInitialized() && UseInput() && delta != 0 )
	{
		g_MouseWheel = ( delta > 0 ) ? 1 : -1;
		return true;
	}
	return false;
}

void NewFrame()
{
	if( !g_haveNewFrame && IsInitialized() && ShowWindows() )
	{
		ImGuiIO& io = ImGui::GetIO();

		// Setup display size (every frame to accommodate for window resizing)
		io.DisplaySize = g_DisplaySize;

		// Setup time step
		int	time = Sys_Milliseconds();
		double current_time = time * 0.001;
		io.DeltaTime = g_Time > 0.0 ? ( float )( current_time - g_Time ) : ( float )( 1.0f / 60.0f );

		if( io.DeltaTime <= 0.0F )
		{
			io.DeltaTime = ( 1.0f / 60.0f );
		}

		g_Time = current_time;

		// Setup inputs
		io.MousePos = g_MousePos;

		// If a mouse press event came, always pass it as "mouse held this frame",
		// so we don't miss click-release events that are shorter than 1 frame.
		for( int i = 0; i < 5; ++i )
		{
			io.MouseDown[i] = g_MousePressed[i] || usercmdGen->KeyState( K_MOUSE1 + i ) == 1;
			//g_MousePressed[i] = false;
		}

		io.MouseWheel = g_MouseWheel;
		g_MouseWheel = 0.0f;

		// Hide OS mouse cursor if ImGui is drawing it TODO: hide mousecursor?
		// ShowCursor(io.MouseDrawCursor ? 0 : 1);

		ImGui::GetIO().MouseDrawCursor = UseInput();

		// Start the frame
		ImGui::NewFrame();
		g_haveNewFrame = true;

		if( imgui_showDemoWindow.GetBool() && !ImGuiTools::ReleaseMouseForTools() )
		{
			ImGuiTools::impl::SetReleaseToolMouse( true );
		}
	}
}

bool IsReadyToRender()
{
	if( IsInitialized() && ShowWindows() )
	{
		if( !g_haveNewFrame )
		{
			// for screenshots etc, where we didn't go through idCommonLocal::Frame()
			// before idRenderSystemLocal::SwapCommandBuffers_FinishRendering()
			NewFrame();
		}

		return true;
	}

	return false;
}

void Render()
{
	if( IsInitialized() && ShowWindows() )
	{
		if( !g_haveNewFrame )
		{
			// for screenshots etc, where we didn't go through idCommonLocal::Frame()
			// before idRenderSystemLocal::SwapCommandBuffers_FinishRendering()
			NewFrame();
		}

		ImGuiTools::DrawToolWindows();

		if( imgui_showDemoWindow.GetBool() )
		{
			ImGui::ShowDemoWindow();
		}

		ImGui::Render();
		idRenderBackend::ImGui_RenderDrawLists( ImGui::GetDrawData() );
		g_haveNewFrame = false;
	}
}

void Destroy()
{
	if( IsInitialized() )
	{
		ImGui::DestroyContext();
		g_IsInit = false;
		g_haveNewFrame = false;
	}
}

bool IsInitialized()
{
	// checks if imgui is up and running
	return g_IsInit;
}

} //namespace ImGuiHook
