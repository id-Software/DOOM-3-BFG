
#ifndef NEO_IMGUI_IMGUI_HOOKS_H_
#define NEO_IMGUI_IMGUI_HOOKS_H_

#include "../sys/sys_public.h"


namespace ImGuiHook
{

bool	Init( int windowWidth, int windowHeight );

bool	IsInitialized();

// tell imgui that the (game) window size has changed
void	NotifyDisplaySizeChanged( int width, int height );

// inject a sys event (keyboard, mouse, unicode character)
bool	InjectSysEvent( const sysEvent_t* keyEvent );

// inject the current mouse wheel delta for scrolling
bool	InjectMouseWheel( int delta );

// call this once per frame *before* calling ImGui::* commands to draw widgets etc
// (but ideally after getting all new events)
void	NewFrame();

// call this to enable custom ImGui windows which are not editors
bool	IsReadyToRender();

// call this once per frame (at the end) - it'll render all ImGui::* commands
// since NewFrame()
void	Render();

void	Destroy();

} //namespace ImGuiHook


#endif /* NEO_IMGUI_IMGUI_HOOKS_H_ */
