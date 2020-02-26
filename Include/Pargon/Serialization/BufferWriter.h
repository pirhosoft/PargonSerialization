#pragma once

#include "Pargon/Containers/Buffer.h"
#include "Pargon/Serialization/Serialization.h"

#include <type_traits>
#include <utility>

namespace Pargon
{
	class Blueprint;
	template<typename KeyType, typename T> class Map;
	template<typename T> class SequenceView;
	class StringView;
	class TextView;

	class BufferWriter
	{
	public:
		template<typename T> static constexpr auto CanWrite() -> bool;

		auto Size() const -> int;
		auto Endian() const -> Endian;
		void SetEndian(Pargon::Endian endian);

		auto GetBuffer() const -> BufferView;
		auto ExtractBuffer() -> Buffer;

		void Align(size_t size);
		void WriteBytes(BufferView data, bool correctEndian);

		void WriteBit(bool bit);
		void WriteBits(int count, long long bits);
		void WriteSignedBits(int count, long long bits);
		void Realign(bool bit);

		template<typename T> void Write(const T& value);

	private:
		friend class Serializer;

		class Traits
		{
		private:
			template<typename U> using void_t = void;
			template<typename U, typename V, typename = void> struct HasToBufferMethod : std::false_type {};
			template<typename U, typename V, typename = void> struct HasToBufferFunction : std::false_type {};
			template<typename U, typename V> struct HasToBufferMethod<U, V, void_t<decltype(static_cast<void>(0), 0, std::declval<const U>().ToBuffer(std::declval<V&>()))>> : std::true_type {};
			template<typename U, typename V> struct HasToBufferFunction<U, V, void_t<decltype(static_cast<void>(0), 0, ToBuffer(std::declval<U>(), std::declval<V&>()))>> : std::true_type {};

		public:
			template<typename T> static constexpr bool CanWriteAsMethod = HasToBufferMethod<T, BufferWriter>::value;
			template<typename T> static constexpr bool CanWriteAsFunction = HasToBufferFunction<T, BufferWriter>::value;
		};

		Pargon::Endian _endian;
		int _bitIndex = 7;
		Buffer _buffer;

		void Write_(char character);
		void Write_(wchar_t character);
		void Write_(char16_t character);
		void Write_(char32_t character);
		void Write_(bool boolean);
		void Write_(signed char number);
		void Write_(short number);
		void Write_(int number);
		void Write_(long number);
		void Write_(long long number);
		void Write_(unsigned char number);
		void Write_(unsigned short number);
		void Write_(unsigned int number);
		void Write_(unsigned long number);
		void Write_(unsigned long long number);
		void Write_(float number);
		void Write_(double number);
		void Write_(long double number);

		void Write_(const Blueprint& blueprint);
		template<typename KeyType, typename ItemType> void Write_(const Map<KeyType, ItemType>& map);
		template<typename ClassType> void Write_(const ClassType& item);

		void WriteBuffer(BufferView buffer);
		void WriteString(StringView string);
		void WriteText(TextView string);
		template<typename ItemType> void WriteSequence(SequenceView<ItemType> sequence);

		template<typename T> void Serialize(T&& value);
		template<typename T> void Serialize(StringView name, T&& value);
		template<typename T> void Serialize(StringView name, T&& value, const T& defaultValue);
	};
}

template<typename T>
constexpr auto Pargon::BufferWriter::CanWrite() -> bool
{
	constexpr auto _canWriteAsMethod = Traits::CanWriteAsMethod<T>;
	constexpr auto _canWriteAsFunction = Traits::CanWriteAsFunction<T>;
	constexpr auto _canSerializeAsMethod = SerializationTraits::CanSerializeAsMethod<T>;
	constexpr auto _canSerializeAsFunction = SerializationTraits::CanSerializeAsFunction<T>;
	constexpr auto _canWriteAsEnum = std::is_enum<T>::value && SerializationTraits::HasNames<T>;
	constexpr auto _canWriteAsBuffer = SerializationTraits::CanViewAsBuffer<T>;
	constexpr auto _canWriteAsString = SerializationTraits::CanViewAsString<T>;
	constexpr auto _canWriteAsText = SerializationTraits::CanViewAsText<T>;
	constexpr auto _canWriteAsSequence = SerializationTraits::CanViewAsSequence<T>;
	constexpr auto _canWriteAsData = std::is_class<T>::value && std::is_standard_layout<T>::value;

	return _canWriteAsMethod || _canWriteAsFunction || _canSerializeAsMethod || _canSerializeAsFunction || _canWriteAsEnum || _canWriteAsBuffer || _canWriteAsString || _canWriteAsText || _canWriteAsSequence || _canWriteAsData;
}

inline
auto Pargon::BufferWriter::Size() const -> int
{
	return _buffer.Size();
}

inline
auto Pargon::BufferWriter::Endian() const -> Pargon::Endian
{
	return _endian;
}

inline
void Pargon::BufferWriter::SetEndian(Pargon::Endian endian)
{
	_endian = endian;
}

inline
auto Pargon::BufferWriter::GetBuffer() const -> BufferView
{
	return { _buffer };
}

inline
auto Pargon::BufferWriter::ExtractBuffer() -> Buffer
{
	auto buffer = std::move(_buffer);
	return buffer;
}

template<typename T>
void Pargon::BufferWriter::Write(const T& item)
{
	Write_(item);
}

template<typename KeyType, typename ItemType>
void Pargon::BufferWriter::Write_(const Map<KeyType, ItemType>& map)
{
	Write_(map.Count());

	for (auto i = 0; i < map.Count(); i++)
	{
		Write_(map.GetKey(i));
		Write_(map.ItemAtIndex(i));
	}
}

template<typename ClassType>
void Pargon::BufferWriter::Write_(const ClassType& item)
{
	constexpr auto writeAsMethod = Traits::CanWriteAsMethod<ClassType>;
	constexpr auto writeAsFunction = Traits::CanWriteAsFunction<ClassType>;
	constexpr auto serializeAsMethod = SerializationTraits::CanSerializeAsMethod<ClassType>;
	constexpr auto serializeAsFunction = SerializationTraits::CanSerializeAsFunction<ClassType>;
	constexpr auto writeAsEnum = std::is_enum<ClassType>::value && SerializationTraits::HasNames<ClassType>;
	constexpr auto writeAsBuffer = SerializationTraits::CanViewAsBuffer<ClassType>;
	constexpr auto writeAsString = SerializationTraits::CanViewAsString<ClassType>;
	constexpr auto writeAsText = SerializationTraits::CanViewAsText<ClassType>;
	constexpr auto writeAsSequence = SerializationTraits::CanViewAsSequence<ClassType>;
	constexpr auto writeAsData = std::is_class<ClassType>::value && std::is_standard_layout<ClassType>::value;

	static_assert(writeAsMethod || writeAsFunction || serializeAsMethod || serializeAsFunction || writeAsEnum || writeAsBuffer || writeAsString || writeAsText || writeAsSequence || writeAsData, "ClassType does not support writing to a Buffer");

	if constexpr (writeAsMethod)
		item.ToBuffer(*this);
	else if constexpr (writeAsFunction)
		ToBuffer(item, *this);
	else if constexpr (serializeAsMethod)
		item.Serialize(*this);
	else if constexpr (serializeAsFunction)
		Serialize(item, *this);
	else if constexpr (writeAsEnum)
		Write_(static_cast<int>(item));
	else if constexpr (writeAsBuffer)
		WriteBuffer(item);
	else if constexpr (writeAsString)
		WriteString(item);
	else if constexpr (writeAsText)
		WriteText(item);
	else if constexpr (writeAsSequence)
		WriteSequence<SerializationTraits::SequenceType<ClassType>>(item);
	else if constexpr (writeAsData)
		WriteBytes(BufferView(item), false);
}

template<typename ItemType>
void Pargon::BufferWriter::WriteSequence(SequenceView<ItemType> sequence)
{
	Write_(sequence.Count());

	for (auto& item : sequence)
		Write_(item);
}

template<typename T>
void Pargon::BufferWriter::Serialize(T&& value)
{
	Write(value);
}

template<typename T>
void Pargon::BufferWriter::Serialize(StringView name, T&& value)
{
	Serialize(std::forward<T>(value));
}

template<typename T>
void Pargon::BufferWriter::Serialize(StringView name, T&& value, const T& defaultValue)
{
	Serialize(std::forward<T>(value));
}
