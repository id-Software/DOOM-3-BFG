/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 2014 Robert Beckebans

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

#ifndef __FRAMEBUFFER_H__
#define __FRAMEBUFFER_H__

class Framebuffer
{
public:

	Framebuffer( const char* name, int width, int height );
	
	static void				Init();
	static void				Shutdown();
	
	// deletes OpenGL object but leaves structure intact for reloading
	void					PurgeFramebuffer();
	
	void					Bind();
	static void				BindNull();
	
	void					AddColorBuffer( int format, int index );
	void					AddDepthBuffer( int format );
	
	void					AttachImage2D( int target, const idImage* image, int index );
	void					AttachImage3D( const idImage* image );
	void					AttachImageDepth( const idImage* image );
	void					AttachImageDepthLayer( const idImage* image, int layer );
	
	// check for OpenGL errors
	void					Check();
	uint32_t				GetFramebuffer() const
	{
		return frameBuffer;
	}
	
private:
	idStr					fboName;
	
	// FBO object
	uint32_t				frameBuffer;
	
	uint32_t				colorBuffers[16];
	int						colorFormat;
	
	uint32_t				depthBuffer;
	int						depthFormat;
	
	uint32_t				stencilBuffer;
	int						stencilFormat;
	
	int						width;
	int						height;
	
	static idList<Framebuffer*>	framebuffers;
};

struct globalFramebuffers_t
{
	Framebuffer*				shadowFBO;
};
extern globalFramebuffers_t globalFramebuffers;


#endif // __FRAMEBUFFER_H__