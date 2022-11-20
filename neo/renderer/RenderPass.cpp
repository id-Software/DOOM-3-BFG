#include "precompiled.h"
#pragma hdrstop

#include "RenderPass.h"
#include "Passes/CommonPasses.h"

#include <nvrhi/utils.h>

static triIndex_t quadPicIndexes[6] = { 3, 0, 2, 2, 0, 1 };

#define MAX_VERTS 8000
#define MAX_INDEXES 120000

idDrawVert* BasicTriangle::AllocVerts( int vertCount, triIndex_t* tempIndexes, int indexCount )
{
	if( numIndexes + indexCount > MAX_INDEXES )
	{
		static int warningFrame = 0;
		if( warningFrame != tr.frameCount )
		{
			warningFrame = tr.frameCount;
			idLib::Warning( "idGuiModel::AllocTris: MAX_INDEXES exceeded" );
		}
		return NULL;
	}

	if( numVerts + vertCount > MAX_VERTS )
	{
		static int warningFrame = 0;
		if( warningFrame != tr.frameCount )
		{
			warningFrame = tr.frameCount;
			idLib::Warning( "idGuiModel::AllocTris: MAX_VERTS exceeded" );
		}
		return NULL;
	}


	int startVert = numVerts;
	int startIndex = numIndexes;

	numVerts += vertCount;
	numIndexes += indexCount;

	if( ( startIndex & 1 ) || ( indexCount & 1 ) )
	{
		// slow for write combined memory!
		// this should be very rare, since quads are always an even index count
		for( int i = 0; i < indexCount; i++ )
		{
			indexPointer[startIndex + i] = startVert + tempIndexes[i];
		}
	}
	else
	{
		for( int i = 0; i < indexCount; i += 2 )
		{
			WriteIndexPair( indexPointer + startIndex + i, startVert + tempIndexes[i], startVert + tempIndexes[i + 1] );
		}
	}

	return vertexPointer + startVert;
}

bool BasicTriangle::Init()
{
	int v = renderProgManager.FindShader( "vertbuffershaders.hlsl", SHADER_STAGE_VERTEX );
	int f = renderProgManager.FindShader( "vertbuffershaders.hlsl", SHADER_STAGE_FRAGMENT );

	vertexShader = renderProgManager.GetShader( v );
	pixelShader = renderProgManager.GetShader( f );

	if( !vertexShader || !pixelShader )
	{
		return false;
	}

	commandList = GetDevice()->createCommandList();

	nvrhi::VertexAttributeDesc attributes[] =
	{
		nvrhi::VertexAttributeDesc()
		.setName( "POSITION" )
		.setFormat( nvrhi::Format::RGB32_FLOAT )
		.setOffset( offsetof( idDrawVert, xyz ) )
		.setElementStride( sizeof( idDrawVert ) ),
		//nvrhi::VertexAttributeDesc()
		//	.setName( "NORMAL" )
		//	.setFormat( nvrhi::Format::RGBA8_UINT )
		//	.setOffset( offsetof( idDrawVert, normal ) )
		//	.setElementStride( sizeof( idDrawVert ) ),
		//nvrhi::VertexAttributeDesc()
		//	.setName( "COLOR" )
		//	.setFormat( nvrhi::Format::RGBA8_UINT )
		//	.setOffset( offsetof( idDrawVert, color ) )
		//	.setElementStride( sizeof( idDrawVert ) ),
		//nvrhi::VertexAttributeDesc()
		//	.setName( "COLOR2" )
		//	.setFormat( nvrhi::Format::RGBA8_UINT )
		//	.setOffset( offsetof( idDrawVert, color2 ) )
		//	.setElementStride( sizeof( idDrawVert ) ),
		nvrhi::VertexAttributeDesc()
		.setName( "UV" )
		.setFormat( nvrhi::Format::RG16_FLOAT )
		.setOffset( offsetof( idDrawVert, st ) )
		.setElementStride( sizeof( idDrawVert ) ),
		//nvrhi::VertexAttributeDesc()
		//	.setName( "TANGENT" )
		//	.setFormat( nvrhi::Format::RGBA8_UINT )
		//	.setOffset( offsetof( idDrawVert, tangent ) )
		//	.setElementStride( sizeof( idDrawVert ) ),
	};

	inputLayout = GetDevice()->createInputLayout( attributes, uint32_t( std::size( attributes ) ), vertexShader );

	material = declManager->FindMaterial( "guis/rml/shell/textures/invader" );

	commandList->open();
	for( int i = 0; i < material->GetNumStages(); i++ )
	{
		material->GetStage( i )->texture.image->FinalizeImage( true, commandList );
	}
	commandList->close();
	GetDevice()->executeCommandList( commandList );
	GetDevice()->runGarbageCollection();

	nvrhi::ITexture* texture = ( nvrhi::ITexture* )material->GetStage( 0 )->texture.image->GetTextureID();

	CommonRenderPasses commonPasses;
	commonPasses.Init( GetDevice() );

	idUniformBuffer& ubo = renderProgManager.BindingParamUbo();

	nvrhi::BindingSetDesc bindingSetDesc;
	bindingSetDesc.bindings =
	{
		nvrhi::BindingSetItem::ConstantBuffer( 0, ubo.GetAPIObject(), nvrhi::BufferRange( ubo.GetOffset(), ubo.GetSize() ) ),
		nvrhi::BindingSetItem::Texture_SRV( 0, texture ),
		nvrhi::BindingSetItem::Sampler( 0, commonPasses.m_AnisotropicWrapSampler )
	};

	if( !nvrhi::utils::CreateBindingSetAndLayout( GetDevice(), nvrhi::ShaderType::All, 0, bindingSetDesc, bindingLayout, bindingSet ) )
	{
		common->Error( "Couldn't create the binding set or layout" );
		return false;
	}

	return true;
}

void BasicTriangle::BackBufferResizing()
{
	pipeline = nullptr;
}

void BasicTriangle::Animate( float fElapsedTimeSeconds )
{
}

void BasicTriangle::RenderFrontend()
{
	vertexBlock = vertexCache.AllocVertex( NULL, MAX_VERTS, sizeof( idDrawVert ), commandList );
	indexBlock = vertexCache.AllocIndex( NULL, MAX_INDEXES, sizeof( triIndex_t ), commandList );

	vertexPointer = ( idDrawVert* )vertexCache.MappedVertexBuffer( vertexBlock );
	indexPointer = ( triIndex_t* )vertexCache.MappedIndexBuffer( indexBlock );
	numVerts = 0;
	numIndexes = 0;

	idDrawVert* verts = AllocVerts( 4, quadPicIndexes, 6 );

	uint32_t currentColorNativeBytesOrder = LittleLong( PackColor( idVec4( 255, 255, 255, 255 ) ) );

	float x = 0.f;
	float y = 0.f;
	float w = renderSystem->GetWidth();
	float h = renderSystem->GetHeight();
	float s1 = 0.0f, t1 = 0.0f, s2 = 1.0f, t2 = 1.0f;

	idVec4 topLeft( x, y, s1, t1 );
	idVec4 topRight( x + w, y, s2, t1 );
	idVec4 bottomRight( x + w, y + h, s2, t2 );
	idVec4 bottomLeft( x, y + h, s1, t2 );

	float z = 0.0f;

	ALIGNTYPE16 idDrawVert localVerts[4];

	localVerts[0].Clear();
	localVerts[0].xyz[0] = x;
	localVerts[0].xyz[1] = y;
	localVerts[0].xyz[2] = 0.f;
	localVerts[0].SetTexCoord( s1, t1 );
	localVerts[0].SetNativeOrderColor( currentColorNativeBytesOrder );
	localVerts[0].ClearColor2();

	localVerts[1].Clear();
	localVerts[1].xyz[0] = x + w;
	localVerts[1].xyz[1] = y;
	localVerts[1].xyz[2] = 0.f;
	localVerts[1].SetTexCoord( s2, t1 );
	localVerts[1].SetNativeOrderColor( currentColorNativeBytesOrder );
	localVerts[1].ClearColor2();

	localVerts[2].Clear();
	localVerts[2].xyz[0] = x + w;
	localVerts[2].xyz[1] = y + h;
	localVerts[2].xyz[2] = 0.f;
	localVerts[2].SetTexCoord( s2, t2 );
	localVerts[2].SetNativeOrderColor( currentColorNativeBytesOrder );
	localVerts[2].ClearColor2();

	localVerts[3].Clear();
	localVerts[3].xyz[0] = x;
	localVerts[3].xyz[1] = y + h;
	localVerts[3].xyz[2] = 0.f;
	localVerts[3].SetTexCoord( s1, t2 );
	localVerts[3].SetNativeOrderColor( currentColorNativeBytesOrder );
	localVerts[3].ClearColor2();

	WriteDrawVerts16( verts, localVerts, 4 );

	for( int i = 0; i < 6; i += 2 )
	{
		WriteIndexPair( indexPointer + i, quadPicIndexes[i], quadPicIndexes[i + 1] );
	}
}

void BasicTriangle::Render( nvrhi::IFramebuffer* framebuffer )
{
	if( !pipeline )
	{
		nvrhi::GraphicsPipelineDesc psoDesc;
		psoDesc.VS = vertexShader;
		psoDesc.PS = pixelShader;
		psoDesc.inputLayout = inputLayout;
		psoDesc.bindingLayouts = { bindingLayout };
		psoDesc.primType = nvrhi::PrimitiveType::TriangleList;
		psoDesc.renderState.depthStencilState.depthTestEnable = false;
		//psoDesc.renderState.rasterState.frontCounterClockwise = true;

		pipeline = GetDevice()->createGraphicsPipeline( psoDesc, framebuffer );
	}

	commandList->open();
	commandList->beginMarker( "Basic" );

	RenderFrontend();

	nvrhi::utils::ClearColorAttachment( commandList, framebuffer, 0, nvrhi::Color( 0.f ) );

	float w = framebuffer->getFramebufferInfo().width;
	float h = framebuffer->getFramebufferInfo().height;

	idRenderMatrix projectionMatrix;
	{
		// orthographic matrix
		float xScale = 1.0f / w;
		float yScale = -1.0f / h;  // flip y
		float zScale = 1.0f;

		float projMat[16];
		projMat[0 * 4 + 0] = 2.f * xScale;
		projMat[0 * 4 + 1] = 0.0f;
		projMat[0 * 4 + 2] = 0.0f;
		projMat[0 * 4 + 3] = 0.0f;

		projMat[1 * 4 + 0] = 0.0f;
		projMat[1 * 4 + 1] = 2.f * yScale;
		projMat[1 * 4 + 2] = 0.0f;
		projMat[1 * 4 + 3] = 0.0f;

		projMat[2 * 4 + 0] = 0.0f;
		projMat[2 * 4 + 1] = 0.0f;
		projMat[2 * 4 + 2] = zScale;
		projMat[2 * 4 + 3] = 0.0f;

		projMat[3 * 4 + 0] = -( w * xScale );
		projMat[3 * 4 + 1] = -( h * yScale );
		projMat[3 * 4 + 2] = 0.0f;
		projMat[3 * 4 + 3] = 1.0f;

		float projMatT[16];
		R_MatrixTranspose( projMat, projMatT );

		renderProgManager.SetRenderParm( renderParm_t::RENDERPARM_PROJMATRIX_X, &projMat[0] );
		renderProgManager.SetRenderParm( renderParm_t::RENDERPARM_PROJMATRIX_Y, &projMat[4] );
		renderProgManager.SetRenderParm( renderParm_t::RENDERPARM_PROJMATRIX_Z, &projMat[8] );
		renderProgManager.SetRenderParm( renderParm_t::RENDERPARM_PROJMATRIX_W, &projMat[12] );
	}

	renderProgManager.CommitConstantBuffer( commandList, true );

	idVertexBuffer* vertexBuffer;
	uint vertOffset = 0;
	{
		if( vertexCache.CacheIsStatic( vertexBlock ) )
		{
			vertexBuffer = &vertexCache.staticData.vertexBuffer;
		}
		else
		{
			const uint64 frameNum = ( int )( vertexBlock >> VERTCACHE_FRAME_SHIFT ) & VERTCACHE_FRAME_MASK;
			if( frameNum != ( ( vertexCache.currentFrame ) & VERTCACHE_FRAME_MASK ) )
			{
				idLib::Warning( "RB_DrawElementsWithCounters, vertexBuffer == NULL" );
				return;
			}
			vertexBuffer = &vertexCache.frameData[vertexCache.drawListNum].vertexBuffer;
		}
		vertOffset = ( uint )( vertexBlock >> VERTCACHE_OFFSET_SHIFT ) & VERTCACHE_OFFSET_MASK;
	}

	vertCacheHandle_t indexBlockTemp = indexBlock + ( ( int64 )( 0 * sizeof( triIndex_t ) ) << VERTCACHE_OFFSET_SHIFT );

	idIndexBuffer* indexBuffer;
	uint indexOffset = 0;
	if( vertexCache.CacheIsStatic( indexBlockTemp ) )
	{
		indexBuffer = &vertexCache.staticData.indexBuffer;
	}
	else
	{
		const uint64 frameNum = ( int )( indexBlockTemp >> VERTCACHE_FRAME_SHIFT ) & VERTCACHE_FRAME_MASK;
		if( frameNum != ( ( vertexCache.currentFrame ) & VERTCACHE_FRAME_MASK ) )
		{
			idLib::Warning( "RB_DrawElementsWithCounters, indexBuffer == NULL" );
			return;
		}
		indexBuffer = &vertexCache.frameData[vertexCache.drawListNum].indexBuffer;
	}
	indexOffset = ( uint )( indexBlock >> VERTCACHE_OFFSET_SHIFT ) & VERTCACHE_OFFSET_MASK;

	nvrhi::GraphicsState state;
	state.bindings = { bindingSet };
	state.indexBuffer = { indexBuffer->GetAPIObject(), nvrhi::Format::R16_UINT, indexOffset };
	state.vertexBuffers = { { vertexBuffer->GetAPIObject(), 0, vertOffset } };
	state.pipeline = pipeline;
	state.framebuffer = framebuffer;
	state.viewport.addViewportAndScissorRect( framebuffer->getFramebufferInfo().getViewport() );

	commandList->setGraphicsState( state );

	nvrhi::DrawArguments args;
	args.vertexCount = 6;
	commandList->drawIndexed( args );

	commandList->endMarker();
	commandList->close();

	GetDevice()->executeCommandList( commandList );
}