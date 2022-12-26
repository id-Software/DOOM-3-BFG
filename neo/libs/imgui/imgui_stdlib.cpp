// dear imgui: wrappers for C++ standard library (STL) types (std::string, etc.)
// This is also an example of how you may wrap your own similar types.

// Changelog:
// - v0.10: Initial version. Added InputText() / InputTextMultiline() calls with std::string

#include "imgui.h"
#include "imgui_stdlib.h"

// RB: replaced std::string with idStr
#include "sys/sys_defines.h"
#include "sys/sys_builddefines.h"
#include "sys/sys_includes.h"
#include "sys/sys_assert.h"
#include "sys/sys_types.h"
#include "sys/sys_intrinsics.h"
#include "sys/sys_threading.h"

#include "CmdArgs.h"
#include "containers/Sort.h"
#include "Str.h"

struct InputTextCallback_UserData
{
	idStr*            Str;
	ImGuiInputTextCallback  ChainCallback;
	void*                   ChainCallbackUserData;
};

static int InputTextCallback( ImGuiInputTextCallbackData* data )
{
	InputTextCallback_UserData* user_data = ( InputTextCallback_UserData* )data->UserData;
	if( data->EventFlag == ImGuiInputTextFlags_CallbackResize )
	{
		// Resize string callback
		// If for some reason we refuse the new length (BufTextLen) and/or capacity (BufSize) we need to set them back to what we want.
		idStr* str = user_data->Str;
		IM_ASSERT( data->Buf == str->c_str() );
		str->ReAllocate( data->BufTextLen, true );
		data->Buf = ( char* )str->c_str();
	}
	else if( user_data->ChainCallback )
	{
		// Forward to user callback, if any
		data->UserData = user_data->ChainCallbackUserData;
		return user_data->ChainCallback( data );
	}
	return 0;
}

bool ImGui::InputText( const char* label, idStr* str, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data )
{
	IM_ASSERT( ( flags & ImGuiInputTextFlags_CallbackResize ) == 0 );
	flags |= ImGuiInputTextFlags_CallbackResize;

	InputTextCallback_UserData cb_user_data;
	cb_user_data.Str = str;
	cb_user_data.ChainCallback = callback;
	cb_user_data.ChainCallbackUserData = user_data;
	return InputText( label, ( char* )str->c_str(), str->Size() + 1, flags, InputTextCallback, &cb_user_data );
}

bool ImGui::InputTextMultiline( const char* label, idStr* str, const ImVec2& size, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data )
{
	IM_ASSERT( ( flags & ImGuiInputTextFlags_CallbackResize ) == 0 );
	flags |= ImGuiInputTextFlags_CallbackResize;

	InputTextCallback_UserData cb_user_data;
	cb_user_data.Str = str;
	cb_user_data.ChainCallback = callback;
	cb_user_data.ChainCallbackUserData = user_data;
	return InputTextMultiline( label, ( char* )str->c_str(), str->Size() + 1, size, flags, InputTextCallback, &cb_user_data );
}

bool ImGui::InputTextWithHint( const char* label, const char* hint, idStr* str, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data )
{
	IM_ASSERT( ( flags & ImGuiInputTextFlags_CallbackResize ) == 0 );
	flags |= ImGuiInputTextFlags_CallbackResize;

	InputTextCallback_UserData cb_user_data;
	cb_user_data.Str = str;
	cb_user_data.ChainCallback = callback;
	cb_user_data.ChainCallbackUserData = user_data;
	return InputTextWithHint( label, hint, ( char* )str->c_str(), str->Size() + 1, flags, InputTextCallback, &cb_user_data );
}
