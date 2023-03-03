// The MIT License(MIT)
//
// Copyright(c) 2019 Vadim Slyusarev
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include "optick_common.h"

#if USE_OPTICK

#include <cstring>
#include <new>
#include <stdlib.h>
#include <atomic>

#include <array>
#include <list>
#include <string>
#include <sstream>
#include <unordered_set>
#include <unordered_map>
#include <vector>

namespace Optick
{
	class Memory
	{
		struct Header
		{
			uint64_t size;
		};

	#if defined(OPTICK_32BIT)
		static std::atomic<uint32_t> memAllocated;
	#else
		static std::atomic<uint64_t> memAllocated;
	#endif

		static void* (*allocate)(size_t);
		static void  (*deallocate)(void*);
		static void  (*initThread)(void);
	public:
		static OPTICK_INLINE void* Alloc(size_t size)
		{
			size_t totalSize = size + sizeof(Header);
			void *ptr = allocate(totalSize);
			OPTICK_VERIFY(ptr, "Can't allocate memory", return nullptr);

			Header* header = (Header*)ptr;
			header->size = totalSize;
			memAllocated += totalSize;

			return (uint8_t*)ptr + sizeof(Header);
		}

		static OPTICK_INLINE void Free(void* p)
		{
			if (p != nullptr)
			{
				uint8_t* basePtr = (uint8_t*)p - sizeof(Header);
				Header* header = (Header*)basePtr;
				memAllocated -= header->size;
				deallocate(basePtr);
			}
		}

		static OPTICK_INLINE size_t GetAllocatedSize()
		{
			return (size_t)memAllocated;
		}

		template<class T>
		static T* New()
		{
			return new (Memory::Alloc(sizeof(T))) T();
		}

		template<class T, class P1>
		static T* New(P1 p1)
		{
			return new (Memory::Alloc(sizeof(T))) T(p1);
		}

		template<class T, class P1, class P2>
		static T* New(P1 p1, P2 p2)
		{
			return new (Memory::Alloc(sizeof(T))) T(p1, p2);
		}

		template<class T>
		static void Delete(T* p)
		{
			if (p)
			{
				p->~T();
				Free(p);
			}
		}

		static void SetAllocator(AllocateFn allocateFn, DeallocateFn deallocateFn, InitThreadCb initThreadCb)
		{
			allocate = allocateFn;
			deallocate = deallocateFn;
			initThread = initThreadCb;
		}

		static void InitThread()
		{
			if (initThread != nullptr)
				initThread();
		}

		template<typename T> 
		struct Allocator : public std::allocator<T> 
		{
			Allocator() {}
			template<class U> 
			Allocator(const Allocator<U>&) {}
			template<typename U> struct rebind { typedef Allocator<U> other; };

			typename std::allocator<T>::value_type* allocate(typename std::allocator<T>::size_type n) 
			{
				return reinterpret_cast<typename std::allocator<T>::value_type*>(Memory::Alloc(n * sizeof(T)));
			}

			typename std::allocator<T>::value_type* allocate(typename std::allocator<T>::size_type n, const typename std::allocator<void>::value_type*)
			{
				return reinterpret_cast<typename std::allocator<T>::value_type*>(Memory::Alloc(n * sizeof(T)));
			}

			void deallocate(typename std::allocator<T>::value_type* p, typename std::allocator<T>::size_type)
			{
				Memory::Free(p);
			}
		};
	};

	// std::* section
	template <typename T, size_t _Size> class array  : public std::array<T, _Size>{};
	template <typename T> class vector : public std::vector<T, Memory::Allocator<T>>{};
	template <typename T> class list : public std::list<T, Memory::Allocator<T>>{};
	template <typename T> class unordered_set : public std::unordered_set<T, std::hash<T>, std::equal_to<T>, Memory::Allocator<T>>{};
	template <typename T, typename V> class unordered_map : public std::unordered_map<T, V, std::hash<T>, std::equal_to<T>, Memory::Allocator<std::pair<const T, V>>>{};
	
	using string = std::basic_string<char, std::char_traits<char>, Memory::Allocator<char>>;
	using wstring = std::basic_string<wchar_t, std::char_traits<wchar_t>, Memory::Allocator<wchar_t>>;
	
	using istringstream = std::basic_istringstream<char, std::char_traits<char>, Memory::Allocator<char>>;
	using ostringstream = std::basic_ostringstream<char, std::char_traits<char>, Memory::Allocator<char>>;
	using stringstream = std::basic_stringstream<char, std::char_traits<char>, Memory::Allocator<char>>;

	using fstream = std::basic_fstream<char, std::char_traits<char>>;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template<class T, uint32 SIZE>
	struct MemoryChunk
	{
		T data[SIZE];
		MemoryChunk* next;
		MemoryChunk* prev;

		MemoryChunk() : next(0), prev(0) {}

		~MemoryChunk()
		{
			MemoryChunk* chunk = this;
			while (chunk->next)
				chunk = chunk->next;

			while (chunk != this)
			{
				MemoryChunk* toDelete = chunk;
				chunk = toDelete->prev;
				Memory::Delete(toDelete);
			}

			if (prev != nullptr)
			{
				prev->next = nullptr;
				prev = nullptr;
			}
		}
	};
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template<class T, uint32 SIZE = 16>
	class MemoryPool
	{
		typedef MemoryChunk<T, SIZE> Chunk;
		Chunk* root;
		Chunk* chunk;
		uint32 index;

		OPTICK_INLINE void AddChunk()
		{
			index = 0;
			if (!chunk || !chunk->next)
			{
				Chunk* newChunk = Memory::New<Chunk>();
				if (chunk)
				{
					chunk->next = newChunk;
					newChunk->prev = chunk;
					chunk = newChunk;
				}
				else
				{
					root = chunk = newChunk;
				}
			}
			else
			{
				chunk = chunk->next;
			}
		}
	public:
		MemoryPool() : root(nullptr), chunk(nullptr), index(SIZE) {}

		OPTICK_INLINE T& Add()
		{
			if (index >= SIZE)
				AddChunk();

			return chunk->data[index++];
		}

		OPTICK_INLINE T& Add(const T& item)
		{
			return Add() = item;
		}

		OPTICK_INLINE T* AddRange(const T* items, size_t count, bool allowOverlap = true)
		{
			if (count == 0 || (count > SIZE && !allowOverlap))
				return nullptr;

			if (count >= (SIZE - index) && !allowOverlap)
			{
				AddChunk();
			}

			T* result = &chunk->data[index];

			while (count)
			{
				size_t numLeft = SIZE - index;
				size_t numCopy = numLeft < count ? numLeft : count;
				std::memcpy(&chunk->data[index], items, sizeof(T) * numCopy);

				count -= numCopy;
				items += numCopy;
				index += (uint32_t)numCopy;

				if (count)
					AddChunk();
			}

			return result;
		}


		OPTICK_INLINE T* TryAdd(int count)
		{
			if (index + count <= SIZE)
			{
				T* res = &chunk->data[index];
				index += count;
				return res;
			}

			return nullptr;
		}

		OPTICK_INLINE T* Back()
		{
			if (chunk && index > 0)
				return &chunk->data[index - 1];

			if (chunk && chunk->prev != nullptr)
				return &chunk->prev->data[SIZE - 1];

			return nullptr;
		}

		OPTICK_INLINE T* Front()
		{
			return !IsEmpty() ? &root->data[0] : nullptr;
		}

		OPTICK_INLINE size_t Size() const
		{
			if (root == nullptr)
				return 0;

			size_t count = 0;

			for (const Chunk* it = root; it != chunk; it = it->next)
				count += SIZE;

			return count + index;
		}

		OPTICK_INLINE bool IsEmpty() const
		{
			return (chunk == nullptr) || (chunk == root && index == 0);
		}

		OPTICK_INLINE void Clear(bool preserveMemory = true)
		{
			if (!preserveMemory)
			{
				if (root)
				{
					Memory::Delete(root);
					root = nullptr;
					chunk = nullptr;
					index = SIZE;
				}
			}
			else if (root)
			{
				index = 0;
				chunk = root;
			}
		}

		class const_iterator
		{
			void advance()
			{
				if (chunkIndex < SIZE - 1)
				{
					++chunkIndex;
				}
				else
				{
					chunkPtr = chunkPtr->next;
					chunkIndex = 0;
				}
			}
		public:
			typedef const_iterator self_type;
			typedef T value_type;
			typedef T& reference;
			typedef T* pointer;
			typedef int difference_type;
			const_iterator(const Chunk* ptr, size_t index) : chunkPtr(ptr), chunkIndex(index) { }
			self_type operator++()
			{
				self_type i = *this;
				advance();
				return i;
			}
			self_type operator++(int /*junk*/)
			{
				advance();
				return *this;
			}
			reference operator*() { return (reference)chunkPtr->data[chunkIndex]; }
			pointer operator->() { return &chunkPtr->data[chunkIndex]; }
			bool operator==(const self_type& rhs) const { return (chunkPtr == rhs.chunkPtr) && (chunkIndex == rhs.chunkIndex); }
			bool operator!=(const self_type& rhs) const { return (chunkPtr != rhs.chunkPtr) || (chunkIndex != rhs.chunkIndex); }
		private:
			const Chunk* chunkPtr;
			size_t chunkIndex;
		};

		const_iterator begin() const
		{
			return const_iterator(root, root ? 0 : SIZE);
		}

		const_iterator end() const
		{
			return const_iterator(chunk, index);
		}

		template<class Func>
		void ForEach(Func func) const
		{
			for (const Chunk* it = root; it != chunk; it = it->next)
				for (uint32 i = 0; i < SIZE; ++i)
					func(it->data[i]);

			if (chunk)
				for (uint32 i = 0; i < index; ++i)
					func(chunk->data[i]);
		}

		template<class Func>
		void ForEach(Func func)
		{
			for (Chunk* it = root; it != chunk; it = it->next)
				for (uint32 i = 0; i < SIZE; ++i)
					func(it->data[i]);

			if (chunk)
				for (uint32 i = 0; i < index; ++i)
					func(chunk->data[i]);
		}

		template<class Func>
		void ForEachChunk(Func func) const
		{
			for (const Chunk* it = root; it != chunk; it = it->next)
				func(it->data, SIZE);

			if (chunk)
				func(chunk->data, index);
		}

		void ToArray(T* destination) const
		{
			uint32 curIndex = 0;

			for (const Chunk* it = root; it != chunk; it = it->next)
			{
				memcpy(&destination[curIndex], it->data, sizeof(T) * SIZE);
				curIndex += SIZE;
			}

			if (chunk && index > 0)
			{
				memcpy(&destination[curIndex], chunk->data, sizeof(T) * index);
			}
		}
	};
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template<uint32 CHUNK_SIZE>
	class MemoryBuffer : private MemoryPool<uint8, CHUNK_SIZE>
	{
	public:
		template<class U>
		U* Add(U* data, size_t size, bool allowOverlap = true)
		{
			return (U*)(MemoryPool<uint8, CHUNK_SIZE>::AddRange((uint8*)data, size, allowOverlap));
		}

		template<class T>
		T* Add(const T& val, bool allowOverlap = true)
		{
			return static_cast<T*>(Add(&val, sizeof(T), allowOverlap));
		}

		void Clear(bool preserveMemory)
		{
			MemoryPool<uint8, CHUNK_SIZE>::Clear(preserveMemory);
		}
	};
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}

#endif //USE_OPTICK