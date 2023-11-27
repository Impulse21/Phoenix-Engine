
#include <PhxEngine/Renderer/RenderGraph/RenderGraph.h>

#include <PhxEngine/Core/Log.h>

#include <algorithm>
#include <stdexcept>

using namespace PhxEngine::Renderer;

PhxEngine::Renderer::RgBuilder::RgBuilder(Core::IAllocator* allocoator)
	: m_graphAllocator(allocoator)
{
	this->m_prologuePass = this->m_graphAllocator->Construct<RgSentinelPass>("Prologue");
	this->m_epiloguePass = this->m_graphAllocator->Construct<RgSentinelPass>("Epilogue");
	this->m_renderPasses.emplace_back(this->m_prologuePass);
}
void PhxEngine::Renderer::RgBuilder::Execute()
{
	this->m_renderPasses.emplace_back(this->m_epiloguePass);
	this->Setup();
	// TODO: Realize Resources

	for (auto& level : this->m_compiledDependencyLevels)
	{
		level.Execute();
	}
}

void PhxEngine::Renderer::RgBuilder::Setup()
{
	// https://poniesandlight.co.uk/reflect/island_rendergraph_1/
	// https://levelup.gitconnected.com/organizing-gpu-work-with-directed-acyclic-graphs-f3fd5f2c2af3
	// TODO: Validate graph
	// The graph is implicily created topologcly sorted because of the Pass Results. 
	// The dependent graph would already be added prior to it's dependencies.
	Phx::FlexArray<int> distance(this->m_renderPasses.size(), 0);

	for (int u = this->m_renderPasses.size() - 1; u --> 0 ; )
	{
		for (RgReference reference : m_renderPasses[u]->GetReads())
		{
			if (reference.Type != RgReferenceType::PassResult)
			{
				continue;
			}

			if (distance[reference.Index] < distance[u] + 1)
			{
				distance[reference.Index] = distance[u] + 1;
			}
		}
	}

	this->m_compiledDependencyLevels.resize(*std::ranges::max_element(distance) + 1);
	for (int i = this->m_renderPasses.size() - 1; i --> 0; )
	{
		int level = distance[i];
		this->m_compiledDependencyLevels[level].AddPass(this->m_renderPasses[i]);
	}

	std::reverse(this->m_compiledDependencyLevels.begin(), this->m_compiledDependencyLevels.end());

#if 0

	this->m_adjacencyLists.resize(this->m_renderPasses.size());


	for (size_t i = 0; i < this->m_renderPasses.size(); ++i)
	{
		RgPass* node = this->m_renderPasses[i];

		// Skip nodes that don't have dependencies
		if (!node->HasAnyDependencies())
		{
			continue;
		}

		Phx::FlexArray<size_t>& indices = this->m_adjacencyLists[i];

		// Reverse iterate the render passes here, because often or not, the adjacency list should be built upon
		// the latest changes to the render passes since those pass are more likely to change the resource we are writing to from other passes
		// if we were to iterate from 0 to RenderPasses.size(), it'd often break the algorithm and creates an incorrect adjacency list
		for (size_t j = this->m_renderPasses.size(); j-- != 0;)
		{
			if (i == j)
			{
				continue;
			}

			RgPass* neigbour = this->m_renderPasses[j];
			bool found = false;
			for (const RgReference& neibourReadRef : node->GetReads())
			{
				if (neibourReadRef.Type == RgReferenceType::PassResult && neibourReadRef.Index == i)
				{
					indices.push_back(j);
					found = true;
					break;
				}
			}
		}
	}

	// Topological Sort
	Phx::FlexArray<bool> visited(this->m_renderPasses.size(), false);
	Phx::FlexArray<bool> onStack(this->m_renderPasses.size(), false);

	this->m_topologicalSortedPasses.reserve(this->m_renderPasses.size());
	for (size_t i = 0; i < this->m_renderPasses.size(); i++)
	{
		if (!visited[i])
		{
			this->DepthFirstSearchRec(i, visited, onStack);
		}
	}

	// reverse the sorted passes to get the correct topology
	std::reverse(this->m_topologicalSortedPasses.begin(), this->m_topologicalSortedPasses.end());

	// Calculate Dependency Levels
	Phx::FlexArray<int> distance(this->m_topologicalSortedPasses.size(), 0);

	for (int u = 0; u < this->m_topologicalSortedPasses.size(); u++)
	{
		for (auto v : this->m_adjacencyLists[u])
		{
			if (distance[v] < distance[u] + 1)
			{
				distance[v] = distance[u] + 1;
			}
		}
	}

	this->m_compiledDependencyLevels.resize(*std::ranges::max_element(distance) + 1);
	for (size_t i = 0; i < this->m_topologicalSortedPasses.size(); i++)
	{
		int level = distance[i];
		this->m_compiledDependencyLevels[level].AddPass(this->m_topologicalSortedPasses[i]);
	}
#endif
}

void PhxEngine::Renderer::RgBuilder::DepthFirstSearchRec(size_t n, Phx::FlexArray<bool>& visited, std::stack<size_t>& stack) const
{
	visited[n] = true;

	for (auto neigbour : this->m_adjacencyLists[n])
	{
		if (!visited[n])
		{
			this->DepthFirstSearchRec(neigbour, visited, stack);
		}
	}

	stack.push(n);
}

void PhxEngine::Renderer::RgBuilder::DepthFirstSearchRec(size_t n, Phx::FlexArray<bool>& visited, Phx::FlexArray<bool>& onStack)
{
	visited[n] = true;
	onStack[n] = true;

	// We found the bottom, add to list
	if (!this->m_adjacencyLists[n].empty())
	{
		for (auto neigbour : this->m_adjacencyLists[n])
		{
			if (visited[neigbour] && onStack[neigbour])
			{
				assert(false, "circular Dependacy Found in Graph");
				throw std::runtime_error("circular Dependacy Found in Graph");
			}

			if (!visited[neigbour])
			{
				this->DepthFirstSearchRec(neigbour, visited, onStack);
			}
		}
	}

	this->m_topologicalSortedPasses.push_back(this->m_renderPasses[n]);
	onStack[n] = false;
}

void PhxEngine::Renderer::RgDependencyLevel::AddPass(RgPass* pass)
{
	this->m_passes.push_back(pass);
	this->m_reads.insert(this->m_reads.end(), pass->GetReads().begin(), pass->GetReads().end());
	this->m_writes.insert(this->m_writes.end(), pass->GetWrites().begin(), pass->GetWrites().end());
}

void PhxEngine::Renderer::RgDependencyLevel::Execute()
{
	PHX_LOG_CORE_INFO("Executing Dependency Level");
	RgRegistry reg;
	for (auto& p : this->m_passes)
	{
		p->Execute(reg, nullptr);
	}
}
