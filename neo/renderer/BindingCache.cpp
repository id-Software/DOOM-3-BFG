#include "precompiled.h"
#pragma hdrstop

#include "BindingCache.h"

BindingCache::BindingCache()
	: device( nullptr )
	, bindingSets()
	, bindingHash()
	, mutex()
{
}

void BindingCache::Init( nvrhi::IDevice* _device )
{
	device = _device;
}

nvrhi::BindingSetHandle BindingCache::GetCachedBindingSet( const nvrhi::BindingSetDesc& desc, nvrhi::IBindingLayout* layout )
{
	size_t hash = 0;
	nvrhi::hash_combine( hash, desc );
	nvrhi::hash_combine( hash, layout );

	mutex.Lock();

	nvrhi::BindingSetHandle result = nullptr;
	for( int i = bindingHash.First( hash ); i != -1; i = bindingHash.Next( i ) )
	{
		nvrhi::BindingSetHandle bindingSet = bindingSets[i];
		if( *bindingSet->getDesc() == desc )
		{
			result = bindingSet;
			break;
		}
	}

	mutex.Unlock();

	if( result )
	{
		assert( result->getDesc() && *result->getDesc() == desc );
	}

	return result;
}

nvrhi::BindingSetHandle BindingCache::GetOrCreateBindingSet( const nvrhi::BindingSetDesc& desc, nvrhi::IBindingLayout* layout )
{
	size_t hash = 0;
	nvrhi::hash_combine( hash, desc );
	nvrhi::hash_combine( hash, layout );

	mutex.Lock();

	nvrhi::BindingSetHandle result = nullptr;
	for( int i = bindingHash.First( hash ); i != -1; i = bindingHash.Next( i ) )
	{
		nvrhi::BindingSetHandle bindingSet = bindingSets[i];
		if( *bindingSet->getDesc() == desc )
		{
			result = bindingSet;
			break;
		}
	}

	mutex.Unlock();

	if( !result )
	{
		mutex.Lock();

		result = device->createBindingSet( desc, layout );

		int entryIndex = bindingSets.Append( result );
		bindingHash.Add( hash, entryIndex );

		mutex.Unlock();
	}

	if( result )
	{
		assert( result->getDesc() && *result->getDesc() == desc );
	}

	return result;
}

void BindingCache::Clear()
{
	// RB FIXME void StaticDescriptorHeap::releaseDescriptors(DescriptorIndex baseIndex, uint32_t count)
	// will try to gain a conflicting mutex lock and cause an abort signal

	mutex.Lock();
	for( int i = 0; i < bindingSets.Num(); i++ )
	{
		bindingSets[i].Reset();
	}
	bindingSets.Clear();
	bindingHash.Clear();
	mutex.Unlock();
}

void SamplerCache::Init( nvrhi::IDevice* _device )
{
	device = _device;
}

void SamplerCache::Clear()
{
	mutex.Lock();
	samplers.Clear();
	samplerHash.Clear();
	mutex.Unlock();
}

nvrhi::SamplerHandle SamplerCache::GetOrCreateSampler( nvrhi::SamplerDesc desc )
{
#if 1
	size_t hash = std::hash<nvrhi::SamplerDesc> {}( desc );

	mutex.Lock();

	nvrhi::SamplerHandle result = nullptr;
	for( int i = samplerHash.First( hash ); i != -1; i = samplerHash.Next( i ) )
	{
		nvrhi::SamplerHandle sampler = samplers[i];
		if( sampler->getDesc() == desc )
		{
			result = sampler;
			break;
		}
	}

	mutex.Unlock();

	if( !result )
	{
		mutex.Lock();

		result = device->createSampler( desc );

		int entryIndex = samplers.Append( result );
		samplerHash.Add( hash, entryIndex );

		mutex.Unlock();
	}

	if( result )
	{
		assert( result->getDesc() == desc );
	}

	return result;
#else
	return device->createSampler( desc );
#endif
}
