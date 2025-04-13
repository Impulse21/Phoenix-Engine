#pragma once

#include <string>
#include <PhxCore\UUID.h>
#include <PhxCore\Span.h>

namespace DirectX
{
	struct XMFLOAT2;
	struct XMFLOAT3;
	struct XMFLOAT4;
}

namespace phx::data
{
	template<typename T>
	struct ArchiveField
	{
		const char* key;
		T& value;

		ArchiveField(const char* key, T& value) : key(key), value(value) {}
	};

	struct IDataObj;
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
		IArchiver& operator<<(const ArchiveField<T>& field)
		{
			Write(field.key, field.value);
			return *this;
		}

		template<typename T>
		IArchiver& operator>>(ArchiveField<T>& field)
		{
			Read(field.key, field.value);
			return *this;
		}

	protected:
		virtual void Save() = 0;

		// TODO: Array and IData Obj
		virtual void Write(const char* key, const int32_t& value) = 0;
		virtual void Write(const char* key, const uint32_t& value) = 0;
		virtual void Write(const char* key, const float& value) = 0;
		virtual void Write(const char* key, const std::string& value) = 0;
		virtual void Write(const char* key, const bool& value) = 0;
		virtual void Write(const char* key, const phx::UUID& value) = 0;
		virtual void Write(const char* key, const DirectX::XMFLOAT2& value) = 0;
		virtual void Write(const char* key, const DirectX::XMFLOAT3& value) = 0;
		virtual void Write(const char* key, const DirectX::XMFLOAT4& value) = 0;

		// TOOD: IS there a better way to do this?
		virtual void Write(const char* key, const phx::Span<int32_t> value) = 0;
		virtual void Write(const char* key, const phx::Span<uint32_t> value) = 0;
		virtual void Write(const char* key, const phx::Span<float> value) = 0;
		virtual void Write(const char* key, const phx::Span<std::string> value) = 0;
		virtual void Write(const char* key, const phx::Span<bool> value) = 0;
		virtual void Write(const char* key, const phx::Span<DirectX::XMFLOAT2> value) = 0;
		virtual void Write(const char* key, const phx::Span<DirectX::XMFLOAT3> value) = 0;
		virtual void Write(const char* key, const phx::Span<DirectX::XMFLOAT4> value) = 0;
		// virtual void Write(const std::string& key, phx::Span<IDataObj> value) = 0;

		virtual void Read(const char* key, int32_t& value) = 0;
		virtual void Read(const char* key, uint32_t& value) = 0;
		virtual void Read(const char* key, float& value) = 0;
		virtual void Read(const char* key, bool& value) = 0;
		virtual void Read(const char* key, phx::UUID& value) = 0;
		virtual void Read(const char* key, DirectX::XMFLOAT2& value) = 0;
		virtual void Read(const char* key, DirectX::XMFLOAT3& value) = 0;
		virtual void Read(const char* key, DirectX::XMFLOAT4& value) = 0;
		// virtual void Read(const std::string& key, IDataObj& value) = 0;

	};

	// TRYING MIXIN
#if false
	template<typename Derived>
	class ArchiverMixin : public IArchiver
	{
	public:
		template<typename T>
		void Write(const std::string& key, const std::vector<T>& vec)
		{
			static_cast<Derived*>(this)->WriteSequence(key, vec);
		}

		template<typename T>
		void Read(const std::string& key, std::vector<T>& vec)
		{
			static_cast<Derived*>(this)->ReadSequence(key, vec);
		}
	};
#endif
}
