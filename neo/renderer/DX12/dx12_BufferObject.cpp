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

// DX12 Implementation for buffer objects.

#pragma hdrstop
#include "../../idlib/precompiled.h"
#include "../tr_local.h"

idCVar r_showBuffers("r_showBuffers", "0", CVAR_INTEGER, "");

extern DX12Renderer dxRenderer;
//static const GLenum bufferUsage = GL_STATIC_DRAW_ARB;
//static const GLenum bufferUsage = GL_DYNAMIC_DRAW_ARB;

/*
==================
IsWriteCombined
==================
*/
bool IsWriteCombined(void* base) {
	MEMORY_BASIC_INFORMATION info;
	SIZE_T size = VirtualQueryEx(GetCurrentProcess(), base, &info, sizeof(info));
	if (size == 0) {
		DWORD error = GetLastError();
		error = error;
		return false;
	}
	bool isWriteCombined = ((info.AllocationProtect & PAGE_WRITECOMBINE) != 0);
	return isWriteCombined;
}


/*
================================================================================================

	Buffer Objects

================================================================================================
*/

#ifdef ID_WIN_X86_SSE2_INTRIN

void CopyBuffer(byte* dst, const byte* src, int numBytes) {
	assert_16_byte_aligned(dst);
	assert_16_byte_aligned(src);

	int i = 0;
	for (; i + 128 <= numBytes; i += 128) {
		__m128i d0 = _mm_load_si128((__m128i*) & src[i + 0 * 16]);
		__m128i d1 = _mm_load_si128((__m128i*) & src[i + 1 * 16]);
		__m128i d2 = _mm_load_si128((__m128i*) & src[i + 2 * 16]);
		__m128i d3 = _mm_load_si128((__m128i*) & src[i + 3 * 16]);
		__m128i d4 = _mm_load_si128((__m128i*) & src[i + 4 * 16]);
		__m128i d5 = _mm_load_si128((__m128i*) & src[i + 5 * 16]);
		__m128i d6 = _mm_load_si128((__m128i*) & src[i + 6 * 16]);
		__m128i d7 = _mm_load_si128((__m128i*) & src[i + 7 * 16]);
		_mm_stream_si128((__m128i*) & dst[i + 0 * 16], d0);
		_mm_stream_si128((__m128i*) & dst[i + 1 * 16], d1);
		_mm_stream_si128((__m128i*) & dst[i + 2 * 16], d2);
		_mm_stream_si128((__m128i*) & dst[i + 3 * 16], d3);
		_mm_stream_si128((__m128i*) & dst[i + 4 * 16], d4);
		_mm_stream_si128((__m128i*) & dst[i + 5 * 16], d5);
		_mm_stream_si128((__m128i*) & dst[i + 6 * 16], d6);
		_mm_stream_si128((__m128i*) & dst[i + 7 * 16], d7);
	}
	for (; i + 16 <= numBytes; i += 16) {
		__m128i d = _mm_load_si128((__m128i*) & src[i]);
		_mm_stream_si128((__m128i*) & dst[i], d);
	}
	for (; i + 4 <= numBytes; i += 4) {
		*(uint32*)&dst[i] = *(const uint32*)&src[i];
	}
	for (; i < numBytes; i++) {
		dst[i] = src[i];
	}
	_mm_sfence();
}

#else

void CopyBuffer(byte* dst, const byte* src, int numBytes) {
	assert_16_byte_aligned(dst);
	assert_16_byte_aligned(src);
	memcpy(dst, src, numBytes);
}

#endif

/*
================================================================================================

	idVertexBuffer

================================================================================================
*/

/*
========================
idVertexBuffer::idVertexBuffer
========================
*/
idVertexBuffer::idVertexBuffer() {
	size = 0;
	offsetInOtherBuffer = OWNS_BUFFER_FLAG;
	apiObject = NULL;
	SetUnmapped();
}

/*
========================
idVertexBuffer::~idVertexBuffer
========================
*/
idVertexBuffer::~idVertexBuffer() {
	FreeBufferObject();
}

void idVertexBuffer::FreeBufferObject() {
	if (apiObject != NULL) {
		dxRenderer.FreeVertexBuffer(reinterpret_cast<DX12VertexBuffer*>(apiObject));
		delete apiObject;
	}
}

/*
========================
idVertexBuffer::Reference
========================
*/
void idVertexBuffer::Reference(const idVertexBuffer& other) {
	assert(IsMapped() == false);
	//assert( other.IsMapped() == false );	// this happens when building idTriangles while at the same time setting up idDrawVerts
	assert(other.GetAPIObject() != NULL);
	assert(other.GetSize() > 0);

	FreeBufferObject();
	size = other.GetSize();						// this strips the MAPPED_FLAG
	offsetInOtherBuffer = other.GetOffset();	// this strips the OWNS_BUFFER_FLAG
	apiObject = other.apiObject;
	assert(OwnsBuffer() == false);
}

/*
========================
idVertexBuffer::Reference
========================
*/
void idVertexBuffer::Reference(const idVertexBuffer& other, int refOffset, int refSize) {
	assert(IsMapped() == false);
	//assert( other.IsMapped() == false );	// this happens when building idTriangles while at the same time setting up idDrawVerts
	assert(other.GetAPIObject() != NULL);
	assert(refOffset >= 0);
	assert(refSize >= 0);
	assert(refOffset + refSize <= other.GetSize());

	FreeBufferObject();
	size = refSize;
	offsetInOtherBuffer = other.GetOffset() + refOffset;
	apiObject = other.apiObject;
	assert(OwnsBuffer() == false);
}

/*
========================
idVertexBuffer::Update
========================
*/
void idVertexBuffer::Update(const void* data, int updateSize) const {
	assert(apiObject != NULL);
	assert(IsMapped() == false);
	assert_16_byte_aligned(data);
	assert((GetOffset() & 15) == 0);

	if (updateSize > size) {
		idLib::FatalError("idVertexBuffer::Update: size overrun, %i > %i\n", updateSize, GetSize());
	}

	int numBytes = (updateSize + 15) & ~15;

	void* buffer = MapBuffer(BM_WRITE);
	CopyBuffer((byte*)buffer + GetOffset(), (byte*)data, numBytes);
	UnmapBuffer();
}

/*
========================
idVertexBuffer::MapBuffer
========================
*/
void* idVertexBuffer::MapBuffer(bufferMapType_t mapType) const {
	assert(apiObject != NULL);
	assert(IsMapped() == false);

	UINT8* buffer = NULL;
	D3D12_RANGE readRange = { 0, 0 };
	DX12VertexBuffer* bufferObject = reinterpret_cast<DX12VertexBuffer*>(apiObject);

	if (mapType == BM_READ || mapType == BM_WRITE) { // TODO: Can we make a read only one?
		HRESULT hr = bufferObject->vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&buffer));
		if (FAILED(bufferObject)) {
			common->Warning("Could not load vertex buffer.");
			buffer = NULL;
		}
		else {
			buffer = (byte*)buffer + GetOffset();
		}
	}
	else {
		assert(false);
	}

	SetMapped();

	if (buffer == NULL) {
		idLib::FatalError("idVertexBuffer::MapBuffer: failed");
	}
	return buffer;
}

/*
========================
idVertexBuffer::UnmapBuffer
========================
*/
void idVertexBuffer::UnmapBuffer() const {
	assert(apiObject != NULL);
	assert(IsMapped());

	reinterpret_cast<DX12VertexBuffer*>(apiObject)->vertexBuffer->Unmap(0, nullptr);

	SetUnmapped();
}

/*
========================
idVertexBuffer::ClearWithoutFreeing
========================
*/
void idVertexBuffer::ClearWithoutFreeing() {
	size = 0;
	offsetInOtherBuffer = OWNS_BUFFER_FLAG;
	apiObject = NULL;
}

/*
========================
idVertexBuffer::AllocBufferObject
========================
*/
bool idVertexBuffer::AllocBufferObject(const void* data, int allocSize) {
	assert(apiObject == NULL);
	assert_16_byte_aligned(data);

	if (allocSize <= 0) {
		idLib::Error("idVertexBuffer::AllocBufferObject: allocSize = %i", allocSize);
	}

	size = allocSize;

	bool allocationFailed = false;

	const int numBytes = GetAllocedSize();
	apiObject = dxRenderer.AllocVertexBuffer(new DX12VertexBuffer(), numBytes);


	if (r_showBuffers.GetBool()) {
		idLib::Printf("index buffer alloc %p, api %p (%i bytes)\n", this, GetAPIObject(), GetSize());
	}

	//// copy the data
	if (data != NULL) {
		Update(data, allocSize);
	}

	return !allocationFailed;
}

/*
================================================================================================

	idIndexBuffer

================================================================================================
*/

/*
========================
idIndexBuffer::idIndexBuffer
========================
*/
idIndexBuffer::idIndexBuffer() {
	size = 0;
	offsetInOtherBuffer = OWNS_BUFFER_FLAG;
	apiObject = NULL;
	SetUnmapped();
}

/*
========================
idIndexBuffer::~idIndexBuffer
========================
*/
idIndexBuffer::~idIndexBuffer() {
	FreeBufferObject();
}

/*
========================
idIndexBuffer::AllocBufferObject
========================
*/
bool idIndexBuffer::AllocBufferObject(const void* data, int allocSize) {
	assert(apiObject == NULL);
	assert_16_byte_aligned(data);

	if (allocSize <= 0) {
		idLib::Error("idIndexBuffer::AllocBufferObject: allocSize = %i", allocSize);
	}

	size = allocSize;

	bool allocationFailed = false;

	const int numBytes = GetAllocedSize();

	apiObject = dxRenderer.AllocIndexBuffer(new DX12IndexBuffer(), numBytes);

	if (r_showBuffers.GetBool()) {
		idLib::Printf("index buffer alloc %p, api %p (%i bytes)\n", this, GetAPIObject(), GetSize());
	}

	//GLenum err = qglGetError();
	//if (err == GL_OUT_OF_MEMORY) {
	//	idLib::Warning("idIndexBuffer:AllocBufferObject: allocation failed");
	//	allocationFailed = true;
	//}


	//// copy the data
	if (data != NULL) {
		Update(data, allocSize);
	}

	return !allocationFailed;
}

/*
========================
idIndexBuffer::FreeBufferObject
========================
*/
void idIndexBuffer::FreeBufferObject() {
	if (IsMapped()) {
		UnmapBuffer();
	}

	// if this is a sub-allocation inside a larger buffer, don't actually free anything.
	if (OwnsBuffer() == false) {
		ClearWithoutFreeing();
		return;
	}

	if (apiObject == NULL) {
		return;
	}

	if (r_showBuffers.GetBool()) {
		idLib::Printf("index buffer free %p, api %p (%i bytes)\n", this, GetAPIObject(), GetSize());
	}

	dxRenderer.FreeIndexBuffer(static_cast<DX12IndexBuffer*>(apiObject));
	delete apiObject;

	ClearWithoutFreeing();
}

/*
========================
idIndexBuffer::Reference
========================
*/
void idIndexBuffer::Reference(const idIndexBuffer& other) {
	assert(IsMapped() == false);
	//assert( other.IsMapped() == false );	// this happens when building idTriangles while at the same time setting up triIndex_t
	assert(other.GetAPIObject() != NULL);
	assert(other.GetSize() > 0);

	FreeBufferObject();
	size = other.GetSize();						// this strips the MAPPED_FLAG
	offsetInOtherBuffer = other.GetOffset();	// this strips the OWNS_BUFFER_FLAG
	apiObject = other.apiObject;
	assert(OwnsBuffer() == false);
}

/*
========================
idIndexBuffer::Reference
========================
*/
void idIndexBuffer::Reference(const idIndexBuffer& other, int refOffset, int refSize) {
	assert(IsMapped() == false);
	//assert( other.IsMapped() == false );	// this happens when building idTriangles while at the same time setting up triIndex_t
	assert(other.GetAPIObject() != NULL);
	assert(refOffset >= 0);
	assert(refSize >= 0);
	assert(refOffset + refSize <= other.GetSize());

	FreeBufferObject();
	size = refSize;
	offsetInOtherBuffer = other.GetOffset() + refOffset;
	apiObject = other.apiObject;
	assert(OwnsBuffer() == false);
}

/*
========================
idIndexBuffer::Update
========================
*/
void idIndexBuffer::Update(const void* data, int updateSize) const {

	assert(apiObject != NULL);
	assert(IsMapped() == false);
	assert_16_byte_aligned(data);
	assert((GetOffset() & 15) == 0);

	if (updateSize > size) {
		idLib::FatalError("idIndexBuffer::Update: size overrun, %i > %i\n", updateSize, GetSize());
	}

	int numBytes = (updateSize + 15) & ~15;
	void* buffer = MapBuffer(BM_WRITE);
	CopyBuffer((byte*)buffer + GetOffset(), (byte*)data, numBytes);
	UnmapBuffer();
}

/*
========================
idIndexBuffer::MapBuffer
========================
*/
void* idIndexBuffer::MapBuffer(bufferMapType_t mapType) const {
	assert(apiObject != NULL);
	assert(IsMapped() == false);

	UINT8* buffer = NULL;
	D3D12_RANGE readRange = { 0, 0 };
	DX12IndexBuffer* bufferObject = reinterpret_cast<DX12IndexBuffer*>(apiObject);

	if (mapType == BM_READ || mapType == BM_WRITE) { // TODO: Can we make a read only one?
		HRESULT hr = bufferObject->indexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&buffer));
		if (FAILED(bufferObject)) {
			common->Warning("Could not load index buffer.");
			buffer = NULL;
		}
		else {
			buffer = (byte*)buffer + GetOffset();
		}
	}
	else {
		assert(false);
	}

	SetMapped();

	if (buffer == NULL) {
		idLib::FatalError("idIndexBuffer::MapBuffer: failed");
	}
	return buffer;
}

/*
========================
idIndexBuffer::UnmapBuffer
========================
*/
void idIndexBuffer::UnmapBuffer() const {
	assert(apiObject != NULL);
	assert(IsMapped());

	reinterpret_cast<DX12IndexBuffer*>(apiObject)->indexBuffer->Unmap(0, nullptr);

	SetUnmapped();
}

/*
========================
idIndexBuffer::ClearWithoutFreeing
========================
*/
void idIndexBuffer::ClearWithoutFreeing() {
	size = 0;
	offsetInOtherBuffer = OWNS_BUFFER_FLAG;
	apiObject = NULL;
}

/*
================================================================================================

	idJointBuffer

================================================================================================
*/

/*
========================
idJointBuffer::idJointBuffer
========================
*/
idJointBuffer::idJointBuffer() {
	numJoints = 0;
	offsetInOtherBuffer = OWNS_BUFFER_FLAG;
	apiObject = NULL;
	SetUnmapped();
}

/*
========================
idJointBuffer::~idJointBuffer
========================
*/
idJointBuffer::~idJointBuffer() {
	FreeBufferObject();
}

/*
========================
idJointBuffer::AllocBufferObject
========================
*/
bool idJointBuffer::AllocBufferObject(const float* joints, int numAllocJoints) {
	assert(apiObject == NULL);
	assert_16_byte_aligned(joints);

	if (numAllocJoints <= 0) {
		idLib::Error("idJointBuffer::AllocBufferObject: joints = %i", numAllocJoints);
	}

	numJoints = numAllocJoints;

	bool allocationFailed = false;

	const int numBytes = GetAllocedSize();

	// TODO: Alloc buffer object.
	/*GLuint buffer = 0;
	qglGenBuffersARB(1, &buffer);
	qglBindBufferARB(GL_UNIFORM_BUFFER, buffer);
	qglBufferDataARB(GL_UNIFORM_BUFFER, numBytes, NULL, GL_STREAM_DRAW_ARB);
	qglBindBufferARB(GL_UNIFORM_BUFFER, 0);
	apiObject = reinterpret_cast<void*>(buffer);*/

	// TODO: Find if this is the appropriate buffer
	apiObject = dxRenderer.AllocJointBuffer(new DX12JointBuffer(), numBytes);

	if (r_showBuffers.GetBool()) {
		idLib::Printf("joint buffer alloc %p, api %p (%i joints)\n", this, GetAPIObject(), GetNumJoints());
	}

	// copy the data
	if (joints != NULL) {
		Update(joints, numAllocJoints);
	}

	return !allocationFailed;
}

/*
========================
idJointBuffer::FreeBufferObject
========================
*/
void idJointBuffer::FreeBufferObject() {
	if (IsMapped()) {
		UnmapBuffer();
	}

	// if this is a sub-allocation inside a larger buffer, don't actually free anything.
	if (OwnsBuffer() == false) {
		ClearWithoutFreeing();
		return;
	}

	if (apiObject == NULL) {
		return;
	}

	if (r_showBuffers.GetBool()) {
		idLib::Printf("joint buffer free %p, api %p (%i joints)\n", this, GetAPIObject(), GetNumJoints());
	}

	// TODO: Implement clear buffer object.
	/*GLuint buffer = reinterpret_cast<GLuint> (apiObject);
	qglBindBufferARB(GL_UNIFORM_BUFFER, 0);
	qglDeleteBuffersARB(1, &buffer);*/

	dxRenderer.FreeJointBuffer(static_cast<DX12JointBuffer*>(apiObject));
	delete apiObject;

	ClearWithoutFreeing();
}

/*
========================
idJointBuffer::Reference
========================
*/
void idJointBuffer::Reference(const idJointBuffer& other) {
	assert(IsMapped() == false);
	assert(other.IsMapped() == false);
	assert(other.GetAPIObject() != NULL);
	assert(other.GetNumJoints() > 0);

	FreeBufferObject();
	numJoints = other.GetNumJoints();			// this strips the MAPPED_FLAG
	offsetInOtherBuffer = other.GetOffset();	// this strips the OWNS_BUFFER_FLAG
	apiObject = other.apiObject;
	assert(OwnsBuffer() == false);
}

/*
========================
idJointBuffer::Reference
========================
*/
void idJointBuffer::Reference(const idJointBuffer& other, int jointRefOffset, int numRefJoints) {
	assert(IsMapped() == false);
	assert(other.IsMapped() == false);
	assert(other.GetAPIObject() != NULL);
	assert(jointRefOffset >= 0);
	assert(numRefJoints >= 0);
	assert(jointRefOffset + numRefJoints * sizeof(idJointMat) <= other.GetNumJoints() * sizeof(idJointMat));
	assert_16_byte_aligned(numRefJoints * 3 * 4 * sizeof(float));

	FreeBufferObject();
	numJoints = numRefJoints;
	offsetInOtherBuffer = other.GetOffset() + jointRefOffset;
	apiObject = other.apiObject;
	assert(OwnsBuffer() == false);
}

/*
========================
idJointBuffer::Update
========================
*/
void idJointBuffer::Update(const float* joints, int numUpdateJoints) const {
	assert(apiObject != NULL);
	assert(IsMapped() == false);
	assert_16_byte_aligned(joints);
	assert((GetOffset() & 15) == 0);

	if (numUpdateJoints > numJoints) {
		idLib::FatalError("idJointBuffer::Update: size overrun, %i > %i\n", numUpdateJoints, numJoints);
	}

	const int numBytes = numUpdateJoints * 3 * 4 * sizeof(float);

	float* buffer = MapBuffer(BM_WRITE);
	CopyBuffer((byte*)buffer + GetOffset(), (byte*)joints, numBytes);
	UnmapBuffer();
}

/*
========================
idJointBuffer::MapBuffer
========================
*/
float* idJointBuffer::MapBuffer(bufferMapType_t mapType) const {
	assert(IsMapped() == false);
	assert(mapType == BM_WRITE);
	assert(apiObject != NULL);

	int numBytes = GetAllocedSize();

	void* buffer = NULL;
	D3D12_RANGE readRange = { 0, 0 };
	DX12JointBuffer* bufferObject = reinterpret_cast<DX12JointBuffer*>(apiObject);

	if (mapType == BM_READ || mapType == BM_WRITE) { // TODO: Can we make a read only one?
		HRESULT hr = bufferObject->jointBuffer->Map(0, &readRange, reinterpret_cast<void**>(&buffer));
		if (FAILED(bufferObject)) {
			common->Warning("Could not load joint buffer.");
			buffer = NULL;
		}
		else {
			buffer = (byte*)buffer + GetOffset();
		}
	}
	else {
		assert(false);
	}

	SetMapped();

	if (buffer == NULL) {
		idLib::FatalError("idJointBuffer::MapBuffer: failed");
	}
	return (float*)buffer;
}

/*
========================
idJointBuffer::UnmapBuffer
========================
*/
void idJointBuffer::UnmapBuffer() const {
	assert(apiObject != NULL);
	assert(IsMapped());

	reinterpret_cast<DX12JointBuffer*>(apiObject)->jointBuffer->Unmap(0, nullptr);

	SetUnmapped();
}

/*
========================
idJointBuffer::ClearWithoutFreeing
========================
*/
void idJointBuffer::ClearWithoutFreeing() {
	numJoints = 0;
	offsetInOtherBuffer = OWNS_BUFFER_FLAG;
	apiObject = NULL;
}

/*
========================
idJointBuffer::Swap
========================
*/
void idJointBuffer::Swap(idJointBuffer& other) {
	// Make sure the ownership of the buffer is not transferred to an unintended place.
	assert(other.OwnsBuffer() == OwnsBuffer());

	SwapValues(other.numJoints, numJoints);
	SwapValues(other.offsetInOtherBuffer, offsetInOtherBuffer);
	SwapValues(other.apiObject, apiObject);
}


void UnbindBufferObjects() {
	//TODO: Implement
}