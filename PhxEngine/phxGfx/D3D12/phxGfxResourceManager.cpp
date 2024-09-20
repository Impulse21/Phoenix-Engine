#include "pch.h"
#include "phxGfxResourceManager.h"

void phx::gfx::D3D12ResourceManager::RunGrabageCollection(uint64_t completedFrame)
{
	while (!this->m_deleteQueue.empty())
	{
		DeleteItem& deleteItem = this->m_deleteQueue.front();
		if (deleteItem.Frame + kBufferCount < completedFrame)
		{
			deleteItem.DeleteFn();
			this->m_deleteQueue.pop_front();
		}
		else
		{
			break;
		}
	}
}
