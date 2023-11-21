
#include <PhxEngine/Renderer/RenderGraph/RenderGraph.h>

using namespace PhxEngine::Renderer;

PhxEngine::Renderer::RenderGraphBuilder::RenderGraphBuilder()
{
	this->m_prologuePass = this->m_graphAllocator->Construct<RgSentinelPass>("Prologue");
	this->m_epiloguePass = this->m_graphAllocator->Construct<RgSentinelPass>("Epilogue");
	this->m_renderPasses.emplace_back(this->m_prologuePass);
}
void PhxEngine::Renderer::RenderGraphBuilder::Execute()
{
	this->m_renderPasses.emplace_back(this->m_epiloguePass);
	this->Setup();
	// Realize Resources
	this->ExecuteInternal();
}

void PhxEngine::Renderer::RenderGraphBuilder::Setup()
{
	this->m_adjacencyLists.resize(this->m_renderPasses.size());

	for (size_t i = 0; i < this->m_renderPasses.size(); ++i)
	{
		RgPass* node = this->m_renderPasses[i];

		// Skip nodes that don't have dependencies
		if (!node->HasAnyDependencies())
		{
			continue;
		}

		std::vector<size_t>& indices = this->m_adjacencyLists[i];

		// Reverse iterate the render passes here, because often or not, the adjacency list should be built upon
		// the latest changes to the render passes since those pass are more likely to change the resource we are writing to from other passes
		// if we were to iterate from 0 to RenderPasses.size(), it'd often break the algorithm and creates an incorrect adjacency list
		for (size_t j = indices.size(); j-- != 0;)
		{
			if (i == j)
			{
				continue;
			}

			RgPass* neigbour = this->m_renderPasses[j];
			for (const RgResourceHandle& writeResources : node->GetWrites())
			{
				// TODO: See if a hashset would be faster - or consider using resource names.
				for (const RgResourceHandle& neigbourReads : node->GetReads())
				{
					indices.push_back(j);
				}
			}
		}
	}

	// Topological Sort
}

void PhxEngine::Renderer::RenderGraphBuilder::ExecuteInternal()
{
}