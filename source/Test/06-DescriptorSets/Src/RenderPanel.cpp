#include "RenderPanel.hpp"

#include "Application.hpp"
#include "MainFrame.hpp"

#include <Renderer/Core/BackBuffer.hpp>
#include <Renderer/Core/Connection.hpp>
#include <Renderer/Descriptor/DescriptorSet.hpp>
#include <Renderer/Descriptor/DescriptorSetLayout.hpp>
#include <Renderer/Descriptor/DescriptorSetLayoutBinding.hpp>
#include <Renderer/Descriptor/DescriptorSetPool.hpp>
#include <Renderer/Core/Device.hpp>
#include <Renderer/RenderPass/FrameBuffer.hpp>
#include <Renderer/Buffer/GeometryBuffers.hpp>
#include <Renderer/Sync/ImageMemoryBarrier.hpp>
#include <Renderer/Pipeline/MultisampleState.hpp>
#include <Renderer/Command/Queue.hpp>
#include <Renderer/RenderPass/RenderPass.hpp>
#include <Renderer/RenderPass/RenderPassState.hpp>
#include <Renderer/RenderPass/RenderSubpass.hpp>
#include <Renderer/RenderPass/RenderSubpassState.hpp>
#include <Renderer/Core/Renderer.hpp>
#include <Renderer/Pipeline/Scissor.hpp>
#include <Renderer/Shader/ShaderProgram.hpp>
#include <Renderer/Buffer/StagingBuffer.hpp>
#include <Renderer/Core/SwapChain.hpp>
#include <Renderer/Image/Texture.hpp>
#include <Renderer/Buffer/UniformBuffer.hpp>
#include <Renderer/Buffer/VertexBuffer.hpp>
#include <Renderer/Pipeline/VertexLayout.hpp>
#include <Renderer/Pipeline/Viewport.hpp>

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
		, m_vertexData
		{
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
				}
			}
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
			doCreateDescriptorSet();
			std::cout << "Descriptor set created." << std::endl;
			doCreateRenderPass();
			std::cout << "Render pass created." << std::endl;
			doCreateVertexBuffer();
			std::cout << "Vertex buffer created." << std::endl;
			doCreatePipeline();
			std::cout << "Pipeline created." << std::endl;
			doPrepareFrames();

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
			delete m_timer;

			if ( m_device )
			{
				m_device->waitIdle();
			}

			m_commandBuffers.clear();
			m_frameBuffers.clear();
			m_uniformBuffer.reset();
			m_descriptorSet.reset();
			m_descriptorPool.reset();
			m_descriptorLayout.reset();
			m_sampler.reset();
			m_texture.reset();
			m_stagingBuffer.reset();
			m_pipeline.reset();
			m_pipelineLayout.reset();
			m_program.reset();
			m_vertexBuffer.reset();
			m_geometryBuffers.reset();
			m_renderPass.reset();
			m_swapChain.reset();
			m_device.reset();

			throw;
		}
	}

	RenderPanel::~RenderPanel()
	{
		delete m_timer;
		m_device->waitIdle();
		m_commandBuffers.clear();
		m_frameBuffers.clear();
		m_uniformBuffer.reset();
		m_descriptorSet.reset();
		m_descriptorPool.reset();
		m_descriptorLayout.reset();
		m_sampler.reset();
		m_texture.reset();
		m_stagingBuffer.reset();
		m_pipeline.reset();
		m_pipelineLayout.reset();
		m_program.reset();
		m_vertexBuffer.reset();
		m_geometryBuffers.reset();
		m_renderPass.reset();
		m_swapChain.reset();
		m_device.reset();
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
			auto size = m_swapChain->getDimensions();
			float halfWidth = static_cast< float >( size.x ) * 0.5f;
			float halfHeight = static_cast< float >( size.y ) * 0.5f;
			m_uniformBuffer->getData( 0u ) = utils::ortho( -halfWidth
				, halfWidth
				, -halfHeight
				, halfHeight
				, -1.0f
				, 1.0f );
			m_stagingBuffer->copyUniformData( m_swapChain->getDefaultResources().getCommandBuffer()
				, m_uniformBuffer->getDatas()
				, *m_uniformBuffer
				, renderer::PipelineStageFlag::eVertexShader );
			doPrepareFrames();
		} );
	}

	void RenderPanel::doCreateTexture()
	{
		std::string shadersFolder = common::getPath( common::getExecutableDirectory() ) / "share" / AppName / "Shaders";

		if ( !wxFileExists( shadersFolder / "texture.png" ) )
		{
			throw std::runtime_error{ "Couldn't find truck texture." };
		}

		wxImage image{ shadersFolder / "texture.png", wxBITMAP_TYPE_PNG };

		if ( image.IsOk() )
		{
			uint8_t * data = image.GetData();
			uint32_t size = image.GetSize().x * image.GetSize().y;
			renderer::ByteArray buffer( size * 4 );

			if ( image.HasAlpha() )
			{
				uint8_t * alpha = image.GetData();
				auto it = buffer.begin();

				for ( uint32_t i{ 0u }; i < size; ++i )
				{
					*it++ = *data++;
					*it++ = *data++;
					*it++ = *data++;
					*it++ = *alpha++;
				}
			}
			else
			{
				auto it = buffer.begin();

				for ( uint32_t i{ 0u }; i < size; ++i )
				{
					*it++ = *data++;
					*it++ = *data++;
					*it++ = *data++;
					*it++ = 0xFF;
				}
			}

			m_texture = m_device->createTexture();
			m_texture->setImage( renderer::PixelFormat::eR8G8B8A8
				, { image.GetSize().x, image.GetSize().y } );
			m_sampler = m_device->createSampler( renderer::WrapMode::eClampToEdge
				, renderer::WrapMode::eClampToEdge
				, renderer::WrapMode::eClampToEdge
				, renderer::Filter::eLinear
				, renderer::Filter::eLinear );
			m_stagingBuffer->copyTextureData( m_swapChain->getDefaultResources().getCommandBuffer()
				, buffer
				, *m_texture );
		}
		else
		{
			throw std::runtime_error{ "Failed to load truck texture image" };
		}
	}

	void RenderPanel::doCreateUniformBuffer()
	{
		m_uniformBuffer = std::make_unique< renderer::UniformBuffer< renderer::Mat4 > >( *m_device
			, 1u
			, renderer::BufferTarget::eTransferDst
			, renderer::MemoryPropertyFlag::eDeviceLocal );
		auto size = m_swapChain->getDimensions();
		float halfWidth = static_cast< float >( size.x ) * 0.5f;
		float halfHeight = static_cast< float >( size.y ) * 0.5f;
		m_uniformBuffer->getData( 0u ) = utils::ortho( -halfWidth
			, halfWidth
			, -halfHeight
			, halfHeight
			, -1.0f
			, 1.0f );
		m_stagingBuffer->copyUniformData( m_swapChain->getDefaultResources().getCommandBuffer()
			, m_uniformBuffer->getDatas()
			, *m_uniformBuffer
			, renderer::PipelineStageFlag::eVertexShader );
	}

	void RenderPanel::doCreateDescriptorSet()
	{
		std::vector< renderer::DescriptorSetLayoutBinding > bindings
		{
			renderer::DescriptorSetLayoutBinding{ 0u, renderer::DescriptorType::eCombinedImageSampler, renderer::ShaderStageFlag::eFragment },
			renderer::DescriptorSetLayoutBinding{ 1u, renderer::DescriptorType::eUniformBuffer, renderer::ShaderStageFlag::eVertex }
		};
		m_descriptorLayout = m_device->createDescriptorSetLayout( std::move( bindings ) );
		m_descriptorPool = m_descriptorLayout->createPool( 1u );
		m_descriptorSet = m_descriptorPool->createDescriptorSet();
		m_descriptorSet->createBinding( m_descriptorLayout->getBinding( 0u )
			, m_texture->getView()
			, *m_sampler );
		m_descriptorSet->createBinding( m_descriptorLayout->getBinding( 1u )
			, *m_uniformBuffer
			, 0u );
		m_descriptorSet->update();
	}

	void RenderPanel::doCreateRenderPass()
	{
		std::vector< renderer::PixelFormat > formats{ { m_swapChain->getFormat() } };
		renderer::RenderSubpassPtrArray subpasses;
		subpasses.emplace_back( m_device->createRenderSubpass( formats
			, { renderer::PipelineStageFlag::eColourAttachmentOutput, renderer::AccessFlag::eColourAttachmentWrite } ) );
		m_renderPass = m_device->createRenderPass( formats
			, subpasses
			, renderer::RenderPassState{ renderer::PipelineStageFlag::eColourAttachmentOutput
				, renderer::AccessFlag::eColourAttachmentWrite
				, { renderer::ImageLayout::eColourAttachmentOptimal } }
			, renderer::RenderPassState{ renderer::PipelineStageFlag::eColourAttachmentOutput
				, renderer::AccessFlag::eColourAttachmentWrite
				, { renderer::ImageLayout::eColourAttachmentOptimal } } );
	}

	void RenderPanel::doCreateVertexBuffer()
	{
		m_vertexBuffer = renderer::makeVertexBuffer< TexturedVertexData >( *m_device
			, uint32_t( m_vertexData.size() )
			, renderer::BufferTarget::eTransferDst
			, renderer::MemoryPropertyFlag::eDeviceLocal );
		m_vertexLayout = renderer::makeLayout< TexturedVertexData >( *m_device, 0 );
		m_vertexLayout->createAttribute< renderer::Vec4 >( 0u
			, uint32_t( offsetof( TexturedVertexData, position ) ) );
		m_vertexLayout->createAttribute< renderer::Vec2 >( 1u
			, uint32_t( offsetof( TexturedVertexData, uv ) ) );
		m_stagingBuffer->copyVertexData( m_swapChain->getDefaultResources().getCommandBuffer()
			, m_vertexData
			, *m_vertexBuffer
			, renderer::PipelineStageFlag::eVertexInput );
		m_geometryBuffers = m_device->createGeometryBuffers( *m_vertexBuffer
			, 0u
			, *m_vertexLayout );
	}

	void RenderPanel::doCreateStagingBuffer()
	{
		m_stagingBuffer = std::make_unique< renderer::StagingBuffer >( *m_device
			, 0u
			, 10000000u );
	}

	void RenderPanel::doCreatePipeline()
	{
		m_pipelineLayout = m_device->createPipelineLayout( *m_descriptorLayout );
		wxSize size{ GetClientSize() };
		std::string shadersFolder = common::getPath( common::getExecutableDirectory() ) / "share" / AppName / "Shaders";

		if ( wxGetApp().getRendererName() != wxT( "gl" ) )
		{
			if ( !wxFileExists( shadersFolder / "vert.spv" )
				|| !wxFileExists( shadersFolder / "frag.spv" ) )
			{
				throw std::runtime_error{ "Shader files are missing" };
			}

			m_program = m_device->createShaderProgram();
			m_program->createModule( common::dumpBinaryFile( shadersFolder / "vert.spv" )
				, renderer::ShaderStageFlag::eVertex );
			m_program->createModule( common::dumpBinaryFile( shadersFolder / "frag.spv" )
				, renderer::ShaderStageFlag::eFragment );
			m_program->link();
		}
		else
		{
			if ( !wxFileExists( shadersFolder / "shader.vert" )
				|| !wxFileExists( shadersFolder / "shader.frag" ) )
			{
				throw std::runtime_error{ "Shader files are missing" };
			}

			m_program = m_device->createShaderProgram();
			m_program->createModule( common::dumpTextFile( shadersFolder / "shader.vert" )
				, renderer::ShaderStageFlag::eVertex );
			m_program->createModule( common::dumpTextFile( shadersFolder / "shader.frag" )
				, renderer::ShaderStageFlag::eFragment );
			m_program->link();
		}

		m_pipeline = m_device->createPipeline( *m_pipelineLayout
			, *m_program
			, { *m_vertexLayout }
			, *m_renderPass
			, renderer::PrimitiveTopology::eTriangleStrip );
		m_pipeline->multisampleState( renderer::MultisampleState{} );
		m_pipeline->finish();
	}

	void RenderPanel::doPrepareFrames()
	{
		m_frameBuffers = m_swapChain->createFrameBuffers( *m_renderPass );
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
				commandBuffer.beginRenderPass( *m_renderPass
					, frameBuffer
					, { renderer::ClearValue{ m_swapChain->getClearColour() } }
					, renderer::SubpassContents::eInline );
				commandBuffer.bindPipeline( *m_pipeline );
				commandBuffer.setViewport( { uint32_t( dimensions.x )
					, uint32_t( dimensions.y )
					, 0
					, 0 } );
				commandBuffer.setScissor( { 0
					, 0
					, uint32_t( dimensions.x )
					, uint32_t( dimensions.y ) } );
				commandBuffer.bindGeometryBuffers( *m_geometryBuffers );
				commandBuffer.bindDescriptorSet( *m_descriptorSet
					, *m_pipelineLayout );
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
			auto res = queue.submit( *m_commandBuffers[resources->getBackBuffer()]
				, resources->getImageAvailableSemaphore()
				, renderer::PipelineStageFlag::eColourAttachmentOutput
				, resources->getRenderingFinishedSemaphore()
				, &resources->getFence() );
			m_swapChain->present( *resources );

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
