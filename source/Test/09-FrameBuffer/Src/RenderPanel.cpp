#include "RenderPanel.hpp"

#include "Application.hpp"
#include "MainFrame.hpp"

#include <Buffer/GeometryBuffers.hpp>
#include <Buffer/StagingBuffer.hpp>
#include <Buffer/UniformBuffer.hpp>
#include <Buffer/VertexBuffer.hpp>
#include <Command/Queue.hpp>
#include <Core/BackBuffer.hpp>
#include <Core/Connection.hpp>
#include <Core/Device.hpp>
#include <Core/Renderer.hpp>
#include <Core/SwapChain.hpp>
#include <Descriptor/DescriptorSet.hpp>
#include <Descriptor/DescriptorSetLayout.hpp>
#include <Descriptor/DescriptorSetLayoutBinding.hpp>
#include <Descriptor/DescriptorSetPool.hpp>
#include <Image/Texture.hpp>
#include <Pipeline/DepthStencilState.hpp>
#include <Pipeline/MultisampleState.hpp>
#include <Pipeline/Scissor.hpp>
#include <Pipeline/VertexLayout.hpp>
#include <Pipeline/Viewport.hpp>
#include <RenderPass/FrameBuffer.hpp>
#include <RenderPass/RenderPass.hpp>
#include <RenderPass/RenderPassState.hpp>
#include <RenderPass/RenderSubpass.hpp>
#include <RenderPass/RenderSubpassState.hpp>
#include <Shader/ShaderProgram.hpp>
#include <Sync/ImageMemoryBarrier.hpp>

#include <Utils/Transform.hpp>

#include <FileUtils.hpp>

#include <chrono>

namespace vkapp
{
	namespace
	{
		enum class Ids
		{
			RenderTimer = 42
		}	Ids;

		static int const TimerTimeMs = 10;
	}

	RenderPanel::RenderPanel( wxWindow * parent
		, wxSize const & size
		, renderer::Renderer const & renderer )
		: wxPanel{ parent, wxID_ANY, wxDefaultPosition, size }
		, m_timer{ new wxTimer{ this, int( Ids::RenderTimer ) } }
		, m_offscreenVertexData
		{
			{
				{ -200.0f, -200.0f, 0.0f, 1.0f },
				{ -0.1f, -0.1f },
			},
			{
				{ -200.0f, 200.0f, 0.0f, 1.0f },
				{ -0.1f, 1.1f },
			},
			{
				{ 200.0f, -200.0f, 0.0f, 1.0f },
				{ 1.1f, -0.1f },
			},
			{
				{ 200.0f, 200.0f, 0.0f, 1.0f },
				{ 1.1f, 1.1f },
			},
		}
		, m_offscreenIndexData
		{
			0, 1, 2, 2, 1, 3,
		}
		, m_mainVertexData
		{
			{ { -1.0, -1.0, 0.0, 1.0 }, { 0.0, 0.0 } },
			{ { -1.0, +1.0, 0.0, 1.0 }, { 0.0, 1.0 } },
			{ { +1.0, -1.0, 0.0, 1.0 }, { 1.0, 0.0 } },
			{ { +1.0, +1.0, 0.0, 1.0 }, { 1.0, 1.0 } },
		}
	{
		try
		{
			doCreateDevice( renderer );
			std::cout << "Logical device created." << std::endl;
			doCreateSwapChain();
			std::cout << "Swap chain created." << std::endl;
			doCreateStagingBuffer();
			std::cout << "Staging buffer created." << std::endl;
			doCreateTexture();
			std::cout << "Truck texture created." << std::endl;
			doCreateUniformBuffer();
			std::cout << "Uniform buffer created." << std::endl;
			doCreateOffscreenDescriptorSet();
			std::cout << "Offscreen descriptor set created." << std::endl;
			doCreateOffscreenRenderPass();
			std::cout << "Offscreen render pass created." << std::endl;
			doCreateFrameBuffer();
			std::cout << "Frame buffer created." << std::endl;
			doCreateOffscreenVertexBuffer();
			std::cout << "Offscreen vertex buffer created." << std::endl;
			doCreateOffscreenPipeline();
			std::cout << "Offscreen pipeline created." << std::endl;
			doPrepareOffscreenFrame();
			doCreateMainDescriptorSet();
			std::cout << "Main descriptor set created." << std::endl;
			doCreateMainRenderPass();
			std::cout << "Main render pass created." << std::endl;
			doCreateMainVertexBuffer();
			std::cout << "Main vertex buffer created." << std::endl;
			doCreateMainPipeline();
			std::cout << "Main pipeline created." << std::endl;
			doPrepareMainFrames();

			m_timer->Start( TimerTimeMs );

			Connect( int( Ids::RenderTimer )
				, wxEVT_TIMER
				, wxTimerEventHandler( RenderPanel::onTimer )
				, nullptr
				, this );
			Connect( wxID_ANY
				, wxEVT_SIZE
				, wxSizeEventHandler( RenderPanel::onSize )
				, nullptr
				, this );
		}
		catch ( std::exception & )
		{
			doCleanup();
			throw;
		}
	}

	RenderPanel::~RenderPanel()
	{
		doCleanup();
	}

	void RenderPanel::doCleanup()
	{
		delete m_timer;

		if ( m_device )
		{
			m_device->waitIdle();
		}

		m_updateCommandBuffer.reset();
		m_commandBuffer.reset();
		m_commandBuffers.clear();
		m_frameBuffers.clear();
		m_sampler.reset();
		m_texture.reset();
		m_stagingBuffer.reset();

		m_matrixUbo.reset();
		m_mainDescriptorSet.reset();
		m_mainDescriptorPool.reset();
		m_mainDescriptorLayout.reset();
		m_mainPipeline.reset();
		m_mainPipelineLayout.reset();
		m_mainProgram.reset();
		m_mainVertexBuffer.reset();
		m_mainGeometryBuffers.reset();
		m_mainRenderPass.reset();

		m_offscreenDescriptorSet.reset();
		m_offscreenDescriptorPool.reset();
		m_offscreenDescriptorLayout.reset();
		m_offscreenPipeline.reset();
		m_offscreenPipelineLayout.reset();
		m_offscreenProgram.reset();
		m_offscreenIndexBuffer.reset();
		m_offscreenVertexBuffer.reset();
		m_offscreenGeometryBuffers.reset();
		m_offscreenRenderPass.reset();

		m_frameBuffer.reset();
		m_renderTargetColour.reset();

		m_swapChain.reset();
		m_device.reset();
	}

	void RenderPanel::doUpdateProjection()
	{
		auto size = m_swapChain->getDimensions();
		float halfWidth = static_cast< float >( size.x ) * 0.5f;
		float halfHeight = static_cast< float >( size.y ) * 0.5f;
		m_matrixUbo->getData( 0u ) = m_device->ortho( -halfWidth
			, halfWidth
			, -halfHeight
			, halfHeight
			, -1.0f
			, 1.0f );
		m_stagingBuffer->copyUniformData( *m_updateCommandBuffer
			, m_matrixUbo->getDatas()
			, *m_matrixUbo
			, renderer::PipelineStageFlag::eVertexShader );
	}

	void RenderPanel::doCreateDevice( renderer::Renderer const & renderer )
	{
		m_device = renderer.createDevice( common::makeConnection( this, renderer ) );
	}

	void RenderPanel::doCreateSwapChain()
	{
		wxSize size{ GetClientSize() };
		m_swapChain = m_device->createSwapChain( { size.x, size.y } );
		m_swapChain->setClearColour( { 1.0f, 0.8f, 0.4f, 0.0f } );
		m_swapChainReset = m_swapChain->onReset.connect( [this]()
		{
			doCreateFrameBuffer();
			doPrepareOffscreenFrame();
			doCreateMainDescriptorSet();
			doPrepareMainFrames();
		} );
		m_updateCommandBuffer = m_device->getGraphicsCommandPool().createCommandBuffer();
	}

	void RenderPanel::doCreateTexture()
	{
		std::string shadersFolder = common::getPath( common::getExecutableDirectory() ) / "share" / AppName / "Shaders";
		auto image = common::loadImage( shadersFolder / "texture.png" );
		m_texture = m_device->createTexture();
		m_texture->setImage( image.format, image.size );
		m_sampler = m_device->createSampler( renderer::WrapMode::eClampToEdge
			, renderer::WrapMode::eClampToEdge
			, renderer::WrapMode::eClampToEdge
			, renderer::Filter::eLinear
			, renderer::Filter::eLinear );
		m_stagingBuffer->copyTextureData( m_swapChain->getDefaultResources().getCommandBuffer()
			, image.data
			, *m_texture );
	}

	void RenderPanel::doCreateUniformBuffer()
	{
		m_matrixUbo = std::make_unique< renderer::UniformBuffer< renderer::Mat4 > >( *m_device
			, 1u
			, renderer::BufferTarget::eTransferDst
			, renderer::MemoryPropertyFlag::eDeviceLocal );
	}

	void RenderPanel::doCreateStagingBuffer()
	{
		m_stagingBuffer = std::make_unique< renderer::StagingBuffer >( *m_device
			, 0u
			, 10000000u );
	}

	void RenderPanel::doCreateOffscreenDescriptorSet()
	{
		std::vector< renderer::DescriptorSetLayoutBinding > bindings
		{
			renderer::DescriptorSetLayoutBinding{ 0u, renderer::DescriptorType::eCombinedImageSampler, renderer::ShaderStageFlag::eFragment },
			renderer::DescriptorSetLayoutBinding{ 1u, renderer::DescriptorType::eUniformBuffer, renderer::ShaderStageFlag::eVertex },
		};
		m_offscreenDescriptorLayout = m_device->createDescriptorSetLayout( std::move( bindings ) );
		m_offscreenDescriptorPool = m_offscreenDescriptorLayout->createPool( 1u );
		m_offscreenDescriptorSet = m_offscreenDescriptorPool->createDescriptorSet();
		m_offscreenDescriptorSet->createBinding( m_offscreenDescriptorLayout->getBinding( 0u )
			, m_texture->getView()
			, *m_sampler );
		m_offscreenDescriptorSet->createBinding( m_offscreenDescriptorLayout->getBinding( 1u )
			, *m_matrixUbo
			, 0u );
		m_offscreenDescriptorSet->update();
	}

	void RenderPanel::doCreateOffscreenRenderPass()
	{
		std::vector< renderer::PixelFormat > formats{ { renderer::PixelFormat::eR8G8B8A8 } };
		renderer::RenderSubpassPtrArray subpasses;
		subpasses.emplace_back( m_device->createRenderSubpass( formats
			, { renderer::PipelineStageFlag::eColourAttachmentOutput, renderer::AccessFlag::eColourAttachmentWrite } ) );
		m_offscreenRenderPass = m_device->createRenderPass( formats
			, subpasses
			, renderer::RenderPassState{ renderer::PipelineStageFlag::eColourAttachmentOutput
				, renderer::AccessFlag::eColourAttachmentWrite
				, { renderer::ImageLayout::eColourAttachmentOptimal } }
			, renderer::RenderPassState{ renderer::PipelineStageFlag::eColourAttachmentOutput
				, renderer::AccessFlag::eColourAttachmentWrite
				, { renderer::ImageLayout::eColourAttachmentOptimal } } );
	}

	void RenderPanel::doCreateFrameBuffer()
	{
		auto size = GetClientSize();
		m_renderTargetColour = m_device->createTexture();
		m_renderTargetColour->setImage( renderer::PixelFormat::eR8G8B8A8
			, { size.GetWidth(), size.GetHeight() }
			, renderer::ImageUsageFlag::eColourAttachment | renderer::ImageUsageFlag::eSampled );

		m_frameBuffer = m_offscreenRenderPass->createFrameBuffer( { size.GetWidth(), size.GetHeight() }
			, { *m_renderTargetColour } );
	}

	void RenderPanel::doCreateOffscreenVertexBuffer()
	{
		m_offscreenVertexLayout = renderer::makeLayout< TexturedVertexData >( *m_device, 0 );
		m_offscreenVertexLayout->createAttribute< renderer::Vec4 >( 0u
			, uint32_t( offsetof( TexturedVertexData, position ) ) );
		m_offscreenVertexLayout->createAttribute< renderer::Vec2 >( 1u
			, uint32_t( offsetof( TexturedVertexData, uv ) ) );

		m_offscreenVertexBuffer = renderer::makeVertexBuffer< TexturedVertexData >( *m_device
			, uint32_t( m_offscreenVertexData.size() )
			, renderer::BufferTarget::eTransferDst
			, renderer::MemoryPropertyFlag::eDeviceLocal );
		m_stagingBuffer->copyVertexData( m_swapChain->getDefaultResources().getCommandBuffer()
			, m_offscreenVertexData
			, *m_offscreenVertexBuffer
			, renderer::PipelineStageFlag::eVertexInput );

		m_offscreenIndexBuffer = renderer::makeBuffer< uint16_t >( *m_device
			, uint32_t( m_offscreenIndexData.size() )
			, renderer::BufferTarget::eTransferDst
			, renderer::MemoryPropertyFlag::eDeviceLocal );
		m_stagingBuffer->copyBufferData( m_swapChain->getDefaultResources().getCommandBuffer()
			, m_offscreenIndexData
			, *m_offscreenIndexBuffer );

		m_offscreenGeometryBuffers = m_device->createGeometryBuffers( *m_offscreenVertexBuffer
			, 0u
			, *m_offscreenVertexLayout
			, m_offscreenIndexBuffer->getBuffer()
			, 0u
			, renderer::IndexType::eUInt16 );
	}

	void RenderPanel::doCreateOffscreenPipeline()
	{
		m_offscreenPipelineLayout = m_device->createPipelineLayout( *m_offscreenDescriptorLayout );
		wxSize size{ GetClientSize() };
		std::string shadersFolder = common::getPath( common::getExecutableDirectory() ) / "share" / AppName / "Shaders";
		m_offscreenProgram = m_device->createShaderProgram();

		if ( m_offscreenProgram->isSPIRVSupported() )
		{
			if ( !wxFileExists( shadersFolder / "offscreen_vert.spv" )
				|| !wxFileExists( shadersFolder / "offscreen_frag.spv" ) )
			{
				throw std::runtime_error{ "Shader files are missing" };
			}

			m_offscreenProgram->createModule( common::dumpBinaryFile( shadersFolder / "offscreen_vert.spv" )
				, renderer::ShaderStageFlag::eVertex );
			m_offscreenProgram->createModule( common::dumpBinaryFile( shadersFolder / "offscreen_frag.spv" )
				, renderer::ShaderStageFlag::eFragment );
			m_offscreenProgram->link();
		}
		else
		{
			if ( !wxFileExists( shadersFolder / "offscreen.vert" )
				|| !wxFileExists( shadersFolder / "offscreen.frag" ) )
			{
				throw std::runtime_error{ "Shader files are missing" };
			}

			m_offscreenProgram->createModule( common::dumpTextFile( shadersFolder / "offscreen.vert" )
				, renderer::ShaderStageFlag::eVertex );
			m_offscreenProgram->createModule( common::dumpTextFile( shadersFolder / "offscreen.frag" )
				, renderer::ShaderStageFlag::eFragment );
			m_offscreenProgram->link();
		}

		m_offscreenPipeline = m_device->createPipeline( *m_offscreenPipelineLayout
			, *m_offscreenProgram
			, { *m_offscreenVertexLayout }
			, *m_offscreenRenderPass
			, renderer::PrimitiveTopology::eTriangleList
			, renderer::RasterisationState{ 0, false, false, renderer::PolygonMode::eFill, renderer::CullModeFlag::eNone } );
		m_offscreenPipeline->multisampleState( renderer::MultisampleState{} );
		m_offscreenPipeline->depthStencilState( renderer::DepthStencilState{} );
		m_offscreenPipeline->finish();
	}

	void RenderPanel::doCreateMainDescriptorSet()
	{
		std::vector< renderer::DescriptorSetLayoutBinding > bindings
		{
			renderer::DescriptorSetLayoutBinding{ 0u, renderer::DescriptorType::eCombinedImageSampler, renderer::ShaderStageFlag::eFragment },
		};
		m_mainDescriptorLayout = m_device->createDescriptorSetLayout( std::move( bindings ) );
		m_mainDescriptorPool = m_mainDescriptorLayout->createPool( 1u );
		m_mainDescriptorSet = m_mainDescriptorPool->createDescriptorSet();
		m_mainDescriptorSet->createBinding( m_mainDescriptorLayout->getBinding( 0u )
			, m_renderTargetColour->getView()
			, *m_sampler );
		m_mainDescriptorSet->update();
	}

	void RenderPanel::doCreateMainRenderPass()
	{
		std::vector< renderer::PixelFormat > formats{ { m_swapChain->getFormat() } };
		renderer::RenderSubpassPtrArray subpasses;
		subpasses.emplace_back( m_device->createRenderSubpass( formats
			, { renderer::PipelineStageFlag::eColourAttachmentOutput, renderer::AccessFlag::eColourAttachmentWrite } ) );
		m_mainRenderPass = m_device->createRenderPass( formats
			, subpasses
			, renderer::RenderPassState{ renderer::PipelineStageFlag::eColourAttachmentOutput
				, renderer::AccessFlag::eColourAttachmentWrite
				, { renderer::ImageLayout::eColourAttachmentOptimal } }
			, renderer::RenderPassState{ renderer::PipelineStageFlag::eColourAttachmentOutput
				, renderer::AccessFlag::eColourAttachmentWrite
				, { renderer::ImageLayout::eColourAttachmentOptimal } } );
	}

	void RenderPanel::doPrepareOffscreenFrame()
	{
		doUpdateProjection();
		m_commandBuffer = m_device->getGraphicsCommandPool().createCommandBuffer();
		wxSize size{ GetClientSize() };
		auto & commandBuffer = *m_commandBuffer;
		auto & frameBuffer = *m_frameBuffer;

		if ( commandBuffer.begin( renderer::CommandBufferUsageFlag::eSimultaneousUse ) )
		{
			auto dimensions = m_swapChain->getDimensions();
			commandBuffer.memoryBarrier( renderer::PipelineStageFlag::eColourAttachmentOutput
				, renderer::PipelineStageFlag::eColourAttachmentOutput
				, m_renderTargetColour->makeColourAttachment() );
			commandBuffer.beginRenderPass( *m_offscreenRenderPass
				, frameBuffer
				, { renderer::ClearValue{ m_swapChain->getClearColour() } }
				, renderer::SubpassContents::eInline );
			commandBuffer.bindPipeline( *m_offscreenPipeline );
			commandBuffer.setViewport( { uint32_t( dimensions.x )
				, uint32_t( dimensions.y )
				, 0
				, 0 } );
			commandBuffer.setScissor( { 0
				, 0
				, uint32_t( dimensions.x )
				, uint32_t( dimensions.y ) } );
			commandBuffer.bindGeometryBuffers( *m_offscreenGeometryBuffers );
			commandBuffer.bindDescriptorSet( *m_offscreenDescriptorSet
				, *m_offscreenPipelineLayout );
			commandBuffer.drawIndexed( uint32_t( m_offscreenIndexData.size() )
				, 1u
				, 0u
				, 0u
				, 0u );
			commandBuffer.endRenderPass();
			commandBuffer.memoryBarrier( renderer::PipelineStageFlag::eColourAttachmentOutput
				, renderer::PipelineStageFlag::eBottomOfPipe
				, m_renderTargetColour->makeShaderInputResource() );

			auto res = commandBuffer.end();

			if ( !res )
			{
				std::stringstream stream;
				stream << "Command buffers recording failed.";
				throw std::runtime_error{ stream.str() };
			}
		}
	}

	void RenderPanel::doCreateMainVertexBuffer()
	{
		m_mainVertexLayout = renderer::makeLayout< TexturedVertexData >( *m_device, 0 );
		m_mainVertexLayout->createAttribute< renderer::Vec4 >( 0u
			, uint32_t( offsetof( TexturedVertexData, position ) ) );
		m_mainVertexLayout->createAttribute< renderer::Vec2 >( 1u
			, uint32_t( offsetof( TexturedVertexData, uv ) ) );

		m_mainVertexBuffer = renderer::makeVertexBuffer< TexturedVertexData >( *m_device
			, uint32_t( m_mainVertexData.size() )
			, renderer::BufferTarget::eTransferDst
			, renderer::MemoryPropertyFlag::eDeviceLocal );
		m_stagingBuffer->copyVertexData( m_swapChain->getDefaultResources().getCommandBuffer()
			, m_mainVertexData
			, *m_mainVertexBuffer
			, renderer::PipelineStageFlag::eVertexInput );

		m_mainGeometryBuffers = m_device->createGeometryBuffers( *m_mainVertexBuffer
			, 0u
			, *m_mainVertexLayout );
	}

	void RenderPanel::doCreateMainPipeline()
	{
		m_mainPipelineLayout = m_device->createPipelineLayout( *m_mainDescriptorLayout );
		wxSize size{ GetClientSize() };
		std::string shadersFolder = common::getPath( common::getExecutableDirectory() ) / "share" / AppName / "Shaders";
		m_mainProgram = m_device->createShaderProgram();

		if ( m_mainProgram->isSPIRVSupported() )
		{
			if ( !wxFileExists( shadersFolder / "main_vert.spv" )
				|| !wxFileExists( shadersFolder / "main_frag.spv" ) )
			{
				throw std::runtime_error{ "Shader files are missing" };
			}

			m_mainProgram->createModule( common::dumpBinaryFile( shadersFolder / "main_vert.spv" )
				, renderer::ShaderStageFlag::eVertex );
			m_mainProgram->createModule( common::dumpBinaryFile( shadersFolder / "main_frag.spv" )
				, renderer::ShaderStageFlag::eFragment );
			m_mainProgram->link();
		}
		else
		{
			if ( !wxFileExists( shadersFolder / "main.vert" )
				|| !wxFileExists( shadersFolder / "main.frag" ) )
			{
				throw std::runtime_error{ "Shader files are missing" };
			}

			m_mainProgram->createModule( common::dumpTextFile( shadersFolder / "main.vert" )
				, renderer::ShaderStageFlag::eVertex );
			m_mainProgram->createModule( common::dumpTextFile( shadersFolder / "main.frag" )
				, renderer::ShaderStageFlag::eFragment );
			m_mainProgram->link();
		}

		m_mainPipeline = m_device->createPipeline( *m_mainPipelineLayout
			, *m_mainProgram
			, { *m_mainVertexLayout }
			, *m_mainRenderPass
			, renderer::PrimitiveTopology::eTriangleStrip );
		m_mainPipeline->multisampleState( renderer::MultisampleState{} );
		m_mainPipeline->finish();
	}

	void RenderPanel::doPrepareMainFrames()
	{
		m_frameBuffers = m_swapChain->createFrameBuffers( *m_mainRenderPass );
		m_commandBuffers = m_swapChain->createCommandBuffers();

		for ( size_t i = 0u; i < m_frameBuffers.size(); ++i )
		{
			auto & frameBuffer = *m_frameBuffers[i];
			auto & commandBuffer = *m_commandBuffers[i];

			wxSize size{ GetClientSize() };

			if ( commandBuffer.begin( renderer::CommandBufferUsageFlag::eSimultaneousUse ) )
			{
				auto dimensions = m_swapChain->getDimensions();
				m_swapChain->preRenderCommands( i, commandBuffer );
				commandBuffer.beginRenderPass( *m_mainRenderPass
					, frameBuffer
					, { renderer::ClearValue{ m_swapChain->getClearColour() } }
					, renderer::SubpassContents::eInline );
				commandBuffer.bindPipeline( *m_mainPipeline );
				commandBuffer.setViewport( { uint32_t( dimensions.x )
					, uint32_t( dimensions.y )
					, 0
					, 0 } );
				commandBuffer.setScissor( { 0
					, 0
					, uint32_t( dimensions.x )
					, uint32_t( dimensions.y ) } );
				commandBuffer.bindGeometryBuffers( *m_mainGeometryBuffers );
				commandBuffer.bindDescriptorSet( *m_mainDescriptorSet
					, *m_mainPipelineLayout );
				commandBuffer.draw( 4u, 1u, 0u, 0u );
				commandBuffer.endRenderPass();
				m_swapChain->postRenderCommands( i, commandBuffer );

				auto res = commandBuffer.end();

				if ( !res )
				{
					std::stringstream stream;
					stream << "Command buffers recording failed.";
					throw std::runtime_error{ stream.str() };
				}
			}
		}
	}

	void RenderPanel::doDraw()
	{
		auto resources = m_swapChain->getResources();

		if ( resources )
		{
			auto before = std::chrono::high_resolution_clock::now();
			auto & queue = m_device->getGraphicsQueue();
			auto res = queue.submit( *m_commandBuffer
				, nullptr );

			if ( res )
			{
				auto res = queue.submit( *m_commandBuffers[resources->getBackBuffer()]
					, resources->getImageAvailableSemaphore()
					, renderer::PipelineStageFlag::eColourAttachmentOutput
					, resources->getRenderingFinishedSemaphore()
					, &resources->getFence() );
				m_swapChain->present( *resources );
			}

			auto after = std::chrono::high_resolution_clock::now();
			wxGetApp().updateFps( std::chrono::duration_cast< std::chrono::microseconds >( after - before ) );
		}
		else
		{
			m_timer->Stop();
		}
	}

	void RenderPanel::doResetSwapChain()
	{
		m_device->waitIdle();
		wxSize size{ GetClientSize() };
		m_swapChain->reset( { size.GetWidth(), size.GetHeight() } );
	}

	void RenderPanel::onTimer( wxTimerEvent & event )
	{
		if ( event.GetId() == int( Ids::RenderTimer ) )
		{
			doDraw();
		}
	}

	void RenderPanel::onSize( wxSizeEvent & event )
	{
		m_timer->Stop();
		doResetSwapChain();
		m_timer->Start( TimerTimeMs );
		event.Skip();
	}
}