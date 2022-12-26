/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 2022 Stephen Pridham

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
#ifndef RENDERER_BINDING_CACHE_H_
#define RENDERER_BINDING_CACHE_H_

#include <nvrhi/nvrhi.h>

class BindingCache
{
public:
	BindingCache();

	void                    Init( nvrhi::IDevice* _device );
	void                    Clear();

	nvrhi::BindingSetHandle GetCachedBindingSet( const nvrhi::BindingSetDesc& desc, nvrhi::IBindingLayout* layout );
	nvrhi::BindingSetHandle GetOrCreateBindingSet( const nvrhi::BindingSetDesc& desc, nvrhi::IBindingLayout* layout );

private:
	nvrhi::IDevice*                 device;
	idList<nvrhi::BindingSetHandle> bindingSets;
	idHashIndex						bindingHash;
	idSysMutex                      mutex;
};

class SamplerCache
{
public:

	SamplerCache() {}

	void                    Init( nvrhi::IDevice* _device );
	void                    Clear();

	nvrhi::SamplerHandle    GetOrCreateSampler( nvrhi::SamplerDesc samplerDesc );

private:

	nvrhi::IDevice* device;
	idList<nvrhi::SamplerHandle>	samplers;
	idHashIndex						samplerHash;
	idSysMutex                      mutex;
};

#endif