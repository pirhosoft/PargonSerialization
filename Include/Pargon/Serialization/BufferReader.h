#pragma once

#include "Pargon/Containers/Buffer.h"
#include "Pargon/Containers/Map.h"
#include "Pargon/Containers/String.h"
#include "Pargon/Serialization/Serialization.h"

#include <type_traits>
#include <utility>

namespace Pargon
{
	template<typename T, int N> class Array;
	class Blueprint;
	class BufferReader;
	template<typename T> class List;
	template<typename KeyType, typename T> class Map;

	class BufferReader
	{
	public:
		struct Error
		{
			int Index;
			String Message;
		};

		template<typename T> static constexpr auto CanRead() -> bool;

		BufferReader(BufferView view);

		auto Endian() const -> Endian;
		void SetEndian(Pargon::Endian endian);

		auto Index() const -> int;
		auto Remaining() const -> int;
		auto AtEnd() const -> bool;

		auto HasFailed() const -> bool;
		void ReportError(StringView message);

		auto Advance(int count) -> bool;
		auto Retreat(int count) -> bool;
		auto MoveTo(int index) -> bool;

		auto ViewByte() -> uint8_t;
		auto ViewBytes(int count) -> BufferView;
		auto ReadByte() -> uint8_t;
		auto ReadBytes(int count) -> BufferView;
		auto CopyBytes(BufferReference into, bool correctEndian) -> bool;

		auto ReadBit() -> bool;
		auto ReadBits(int count) -> long long;
		auto ReadSignedBits(int count) -> long long;
		void Realign();

		template<typename T> auto Read() -> T;
		template<typename T> auto Read(T& value) -> bool;

	private:
		class Traits
		{
		private:
			template<typename U> using void_t = void;
			template<typename U, typename V, typename = void> struct HasFromBufferMethod : std::false_type {};
			template<typename U, typename V, typename = void> struct HasFromBufferFunction : std::false_type {};
			template<typename U, typename V> struct HasFromBufferMethod<U, V, void_t<decltype(static_cast<void>(0), static_cast<void>(0), 0, std::declval<U>().FromBuffer(std::declval<V&>()))>> : std::true_type {};
			template<typename U, typename V> struct HasFromBufferFunction<U, V, void_t<decltype(static_cast<void>(0), static_cast<void>(0), 0, FromBuffer(std::declval<U&>(), std::declval<V&>()))>> : std::true_type {};

		public:
			template<typename T> static constexpr bool CanReadAsMethod = HasFromBufferMethod<T, BufferReader>::value;
			template<typename T> static constexpr bool CanReadAsFunction = HasFromBufferFunction<T, BufferReader>::value;
		};

		const uint8_t* _data;
		int _length;
		Pargon::Endian _endian = NativeEndian;

		int _index = 0;
		int _bitIndex = 0;

		bool _hasFailed = false;
		List<Error> _errors;

		auto Read_(bool& boolean) -> bool;
		auto Read_(char& character) -> bool;
		auto Read_(wchar_t& character) -> bool;
		auto Read_(char16_t& character) -> bool;
		auto Read_(char32_t& character) -> bool;
		auto Read_(signed char& number) -> bool;
		auto Read_(short& number) -> bool;
		auto Read_(int& number) -> bool;
		auto Read_(long& number) -> bool;
		auto Read_(long long& number) -> bool;
		auto Read_(unsigned char& number) -> bool;
		auto Read_(unsigned short& number) -> bool;
		auto Read_(unsigned int& number) -> bool;
		auto Read_(unsigned long& number) -> bool;
		auto Read_(unsigned long long& number) -> bool;
		auto Read_(float& number) -> bool;
		auto Read_(double& number) -> bool;
		auto Read_(long double& number) -> bool;

		auto Read_(Buffer& buffer) -> bool;
		auto Read_(BufferView& buffer) -> bool;
		auto Read_(String& string) -> bool;
		auto Read_(StringView& string) -> bool;
		auto Read_(Text& text) -> bool;
		auto Read_(Blueprint& blueprint) -> bool;

		template<typename ItemType, int N> auto Read_(Array<ItemType, N>& array) -> bool;
		template<typename ItemType> auto Read_(List<ItemType>& list) -> bool;
		template<typename KeyType, typename ItemType> auto Read_(Map<KeyType, ItemType>& map) -> bool;
		template<typename ClassType> auto Read_(ClassType& item) -> bool;

		template<typename T> void Serialize(T&& value);
		template<typename T> void Serialize(StringView name, T&& value);
		template<typename T> void Serialize(StringView name, T&& value, const T& defaultValue);
	};
}

template<typename T>
constexpr auto Pargon::BufferReader::CanRead() -> bool
{
	constexpr auto _canReadAsMethod = Traits::CanReadAsMethod<T>;
	constexpr auto _canReadAsFunction = Traits::CanReadAsFunction<T>;
	constexpr auto _canSerializeAsMethod = SerializationTraits::CanSerializeAsMethod<T>;
	constexpr auto _canSerializeAsFunction = SerializationTraits::CanSerializeAsFunction<T>;
	constexpr auto _canReadAsEnum = std::is_enum<T>::value && SerializationTraits::EnumCount<T>;
	constexpr auto _canReadAsData = std::is_class<T>::value && std::is_standard_layout<T>::value;

	return _canReadAsMethod || _canReadAsFunction || _canSerializeAsMethod || _canSerializeAsFunction || _canReadAsEnum || _canReadAsData;
}

inline
auto Pargon::BufferReader::Endian() const -> Pargon::Endian
{
	return _endian;
}

inline
void Pargon::BufferReader::SetEndian(Pargon::Endian endian)
{
	_endian = endian;
}

inline
auto Pargon::BufferReader::Index() const -> int
{
	return _index;
}

inline
auto Pargon::BufferReader::Remaining() const -> int
{
	return _length - _index;
}

inline
auto Pargon::BufferReader::AtEnd() const -> bool
{
	return _index == _length;
}

inline
auto Pargon::BufferReader::HasFailed() const -> bool
{
	return _hasFailed;
}

template<typename T>
auto Pargon::BufferReader::Read() -> T
{
	static_assert(std::is_default_constructible_v<T>, "T must be default constructible");

	auto item = T{};

	if (!_hasFailed)
		Read_(item);

	return item;
}

template<typename T>
auto Pargon::BufferReader::Read(T& item) -> bool
{
	return !_hasFailed && Read_(item);
}

template<typename ItemType, int N>
auto Pargon::BufferReader::Read_(Array<ItemType, N>& array) -> bool
{
	Array<ItemType, N> placeholder;

	for (auto& child : placeholder)
	{
		if (!Read_(child))
			return false;
	}

	array = std::move(placeholder);
	return true;
}

template<typename ItemType>
auto Pargon::BufferReader::Read_(List<ItemType>& list) -> bool
{
	int count;
	if (!Read_(count))
		return false;

	List<ItemType> placeholder;

	for (auto i = 0; i < count; i++)
	{
		auto& item = placeholder.Increment();

		if (!Read_(item))
			return false;
	}

	list = std::move(placeholder);
	return true;
}

template<typename KeyType, typename ItemType>
auto Pargon::BufferReader::Read_(Map<KeyType, ItemType>& map) -> bool
{
	int count;
	if (!Read_(count))
		return false;

	Map<KeyType, ItemType> placeholder;

	for (auto i = 0; i < count; i++)
	{
		KeyType key;
		if (!Read_(key))
			return false;

		ItemType value;
		if (!Read_(value))
			return false;

		placeholder.AddOrSet(key, std::move(value));
	}

	map = std::move(placeholder);
	return true;
}

template<typename ClassType>
auto Pargon::BufferReader::Read_(ClassType& item) -> bool
{
	constexpr auto readAsMethod = Traits::CanReadAsMethod<ClassType>;
	constexpr auto readAsFunction = Traits::CanReadAsFunction<ClassType>;
	constexpr auto serializeAsMethod = SerializationTraits::CanSerializeAsMethod<ClassType>;
	constexpr auto serializeAsFunction = SerializationTraits::CanSerializeAsFunction<ClassType>;
	constexpr auto readAsEnum = std::is_enum<ClassType>::value && SerializationTraits::EnumCount<ClassType>;
	constexpr auto readAsData = std::is_class<ClassType>::value && std::is_standard_layout<ClassType>::value;

	static_assert(readAsMethod || readAsFunction || serializeAsMethod || serializeAsFunction || readAsEnum || readAsData, "ClassType does not support reading from a Buffer");

	if constexpr (readAsMethod)
		item.FromBuffer(*this);
	else if constexpr (readAsFunction)
		FromBuffer(item, *this);
	else if constexpr (serializeAsMethod)
		item.Serialize(*this);
	else if constexpr (serializeAsFunction)
		Serialize(item, *this);
	else if constexpr (readAsEnum)
		return CopyBytes({ reinterpret_cast<uint8_t*>(std::addressof(item)), sizeof(item) }, true);
	else if constexpr (readAsData)
		return CopyBytes({ reinterpret_cast<uint8_t*>(std::addressof(item)), sizeof(item) }, false);

	return !_hasFailed;
}

template<typename T>
void Pargon::BufferReader::Serialize(T&& value)
{
	Read(value);
}

template<typename T>
void Pargon::BufferReader::Serialize(StringView name, T&& value)
{
	Serialize(std::forward<T>(value));
}

template<typename T>
void Pargon::BufferReader::Serialize(StringView name, T&& value, const T& defaultValue)
{
	Serialize(std::forward<T>(value));
}