#pragma once
#include <string>
#include <PhxCore\UUID.h>

namespace DirectX
{
	struct XMFLOAT2;
	struct XMFLOAT3;
	struct XMFLOAT4;
}

namespace phx::data
{
	class IDataObj;
	enum class ArchiveMode
	{
		Read,
		Write
	};

	class IArchiver
	{
	public:
		virtual ~IArchiver() = default;

		virtual ArchiveMode GetMode() const = 0;

		// Templated interface for serialization
		template<typename T>
		IArchiver& operator<<(const std::pair<const char*, T>& pair)
		{
			Write(pair.first, pair.second);
			return *this;
		}

		template<typename T>
		IArchiver& operator>>(std::pair<const char*, T>& pair)
		{
			Read(pair.first, pair.second);
			return *this;
		}

	protected:
		virtual void Write(const char* key, const int32_t& value) = 0;
		virtual void Write(const char* key, const uint32_t& value) = 0;
		virtual void Write(const char* key, const float& value) = 0;
		virtual void Write(const char* key, const std::string& value) = 0;
		virtual void Write(const char* key, const bool& value) = 0;
		virtual void Write(const char* key, const phx::UUID& value) = 0;
		virtual void Write(const char* key, const DirectX::XMFLOAT2& value) = 0;
		virtual void Write(const char* key, const DirectX::XMFLOAT3& value) = 0;
		virtual void Write(const char* key, const DirectX::XMFLOAT4& value) = 0;
		// virtual void Write(const std::string& key, const IDataObj& value) = 0;

		virtual void Read(const char* key, int32_t& value) = 0;
		virtual void Read(const char* key, uint32_t& value) = 0;
		virtual void Read(const char* key, float& value) = 0;
		virtual void Read(const char* key, bool& value) = 0;
		virtual void Read(const char* key, phx::UUID& value) = 0;
		virtual void Read(const char* key, DirectX::XMFLOAT2& value) = 0;
		virtual void Read(const char* key, DirectX::XMFLOAT3& value) = 0;
		virtual void Read(const char* key, DirectX::XMFLOAT4& value) = 0;
		// virtual void Read(const std::string& key, IDataObj& value) = 0;

		// Add overloads here for other types you support
	};

}
