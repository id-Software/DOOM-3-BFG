#ifndef RENDERER_PIPELINECACHE_H_
#define RENDERER_PIPELINECACHE_H_


struct PipelineKey
{
	uint64 state;
	int program;
	bool mirrored;

	Framebuffer* framebuffer;
};

inline bool operator==( const PipelineKey& lhs, const PipelineKey& rhs )
{
	return lhs.state == rhs.state &&
		   lhs.program == rhs.program &&
		   lhs.mirrored == rhs.mirrored &&
		   lhs.framebuffer == rhs.framebuffer;
}

template<>
struct std::hash<PipelineKey>
{
	std::size_t operator()( const PipelineKey& key ) const noexcept
	{
		std::size_t h = 0;
		nvrhi::hash_combine( h, key.state );
		nvrhi::hash_combine( h, key.program );
		nvrhi::hash_combine( h, key.mirrored );
		nvrhi::hash_combine( h, key.framebuffer );
		return h;
	}
};

class PipelineCache
{
public:

	PipelineCache( );

	void Init( nvrhi::DeviceHandle deviceHandle );

	void Clear( );

	nvrhi::GraphicsPipelineHandle GetOrCreatePipeline( const PipelineKey& key );

private:

	nvrhi::DeviceHandle												device;
	idHashIndex														pipelineHash;
	idList<std::pair<PipelineKey, nvrhi::GraphicsPipelineHandle>>	pipelines;
};


#endif