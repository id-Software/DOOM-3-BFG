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

#ifndef __DECLFX_H__
#define __DECLFX_H__

/*
===============================================================================

	idDeclFX

===============================================================================
*/

enum
{
	FX_LIGHT,
	FX_PARTICLE,
	FX_DECAL,
	FX_MODEL,
	FX_SOUND,
	FX_SHAKE,
	FX_ATTACHLIGHT,
	FX_ATTACHENTITY,
	FX_LAUNCH,
	FX_SHOCKWAVE
};

//
// single fx structure
//
typedef struct
{
	int						type;
	int						sibling;

	idStr					data;
	idStr					name;
	idStr					fire;

	float					delay;
	float					duration;
	float					restart;
	float					size;
	float					fadeInTime;
	float					fadeOutTime;
	float					shakeTime;
	float					shakeAmplitude;
	float					shakeDistance;
	float					shakeImpulse;
	float					lightRadius;
	float					rotate;
	float					random1;
	float					random2;

	idVec3					lightColor;
	idVec3					offset;
	idMat3					axis;

	bool					soundStarted;
	bool					shakeStarted;
	bool					shakeFalloff;
	bool					shakeIgnoreMaster;
	bool					bindParticles;
	bool					explicitAxis;
	bool					noshadows;
	bool					particleTrackVelocity;
	bool					trackOrigin;
} idFXSingleAction;

//
// grouped fx structures
//
class idDeclFX : public idDecl
{
public:
	virtual size_t			Size() const;
	virtual const char* 	DefaultDefinition() const;
	virtual bool			Parse( const char* text, const int textLength, bool allowBinaryVersion );
	virtual void			FreeData();
	virtual void			Print() const;
	virtual void			List() const;

	idList<idFXSingleAction, TAG_FX>events;
	idStr					joint;

private:
	void					ParseSingleFXAction( idLexer& src, idFXSingleAction& FXAction );
};

#endif /* !__DECLFX_H__ */
