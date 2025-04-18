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

	enum class ArhiverOp : uint8_t
	{
		Key,
		Value,
		BeginSeq,
		EndSeq,
		BeginMap,
		EndMap,
		Null,
	};

	class IArchiver
	{
	public:
		virtual ~IArchiver() = default;

		virtual ArchiveMode GetMode() const = 0;

		// Templated interface for serialization
		template<typename T>
		IArchiver& operator<<(T& value)
		{
			Write(value);
			return *this;
		}

		template<typename T>
		IArchiver& operator>>(ArchiveField<T>& field)
		{
			Read(field.key, field.value);
			return *this;
		}

		IArchiver& operator<<(const char* key)
		{
			WriteKey(key);
			return *this;
		}
		// Templated interface for serialization
		IArchiver& operator<<(ArhiverOp op)
		{
			switch (op)
			{
			case ArhiverOp::BeginSeq:
				BeginArrayWrite();
				break;
			case ArhiverOp::EndSeq:
				EndArrayWrite();
				break;
			case ArhiverOp::BeginMap:
				BeginMap();
				break;
			case ArhiverOp::EndMap:
				EndMap();
				break;
			case ArhiverOp::Null:
				WriteNull();
				break;
			case ArhiverOp::Key:
			case ArhiverOp::Value:
			default:
				break;
			}

			return *this;
		}

	protected:
		virtual void Save() = 0;

		virtual void WriteNull() = 0;

		virtual void BeginMap() = 0;
		virtual void EndMap() = 0;

		virtual void BeginArrayWrite() = 0;
		virtual void EndArrayWrite() = 0;

		// TODO: Array and IData Obj
		virtual void WriteKey(const char* key) = 0;

		virtual void Write(const int32_t& value) = 0;
		virtual void Write(const uint32_t& value) = 0;
		virtual void Write(const float& value) = 0;
		virtual void Write(const std::string& value) = 0;
		virtual void Write(const bool& value) = 0;
		virtual void Write(const phx::UUID& value) = 0;
		virtual void Write(const DirectX::XMFLOAT2& value) = 0;
		virtual void Write(const DirectX::XMFLOAT3& value) = 0;
		virtual void Write(const DirectX::XMFLOAT4& value) = 0;

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

	private:
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
