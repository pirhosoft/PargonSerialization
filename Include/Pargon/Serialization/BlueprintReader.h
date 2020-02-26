#pragma once

#include "Pargon/Containers/Blueprint.h"
#include "Pargon/Serialization/StringReader.h"

#include <memory>

namespace Pargon
{
	template<typename T, int N> class Array;
	class Blueprint;
	class BlueprintReader;
	class Buffer;
	class BufferView;
	template<typename T> class List;
	template<typename KeyType, typename T> class Map;
	class String;
	class StringView;

	class BlueprintReader
	{
	public:
		struct Error
		{
			String Path;
			String Message;
		};

		template<typename T> static constexpr auto CanRead() -> bool;

		BlueprintReader(const Blueprint& blueprint);

		auto GetBlueprint() const -> const Blueprint&;
		auto ExtractBlueprint() const -> Blueprint;

		auto HasFailed() const -> bool;
		void ReportError(StringView message);

		template<typename T> void Read(T& value);
		template<typename T> auto ReadChild(int index, T& value) -> bool;
		template<typename T> auto ReadChild(StringView name, T& value) -> bool;

		auto MoveDown(StringView child) -> bool;
		auto MoveDown(int index) -> bool;
		auto MoveUp() -> bool;
		auto MoveNext() -> bool;
		auto FirstChild() -> bool;
		auto NextChild() -> bool;
		auto ChildCount() -> int;

	private:
		friend class Serializer;

		class Traits
		{
		private:
			template<typename U> using void_t = void;
			template<typename U, typename V, typename = void> struct HasFromBlueprintMethod : std::false_type {};
			template<typename U, typename V, typename = void> struct HasFromBlueprintFunction : std::false_type {};
			template<typename U, typename V> struct HasFromBlueprintMethod<U, V, void_t<decltype(static_cast<void>(0), static_cast<void>(0), 0, std::declval<U>().FromBlueprint(std::declval<V&>()))>> : std::true_type {};
			template<typename U, typename V> struct HasFromBlueprintFunction<U, V, void_t<decltype(static_cast<void>(0), static_cast<void>(0), 0, FromBlueprint(std::declval<U&>(), std::declval<V&>()))>> : std::true_type {};

		public:
			template<typename T> static constexpr bool CanReadAsMethod = HasFromBlueprintMethod<T, BlueprintReader>::value;
			template<typename T> static constexpr bool CanReadAsFunction = HasFromBlueprintFunction<T, BlueprintReader>::value;
		};

		struct BlueprintTreeNode
		{
			const Blueprint* Node;
			int Index;
		};

		List<BlueprintTreeNode> _tree;

		const Blueprint* _current;
		int _currentIndex;

		bool _hasFailed = false;
		List<Error> _errors;

		auto Read_(char& character) -> bool;
		auto Read_(wchar_t& character) -> bool;
		auto Read_(char16_t& character) -> bool;
		auto Read_(char32_t& character) -> bool;
		auto Read_(bool& boolean) -> bool;
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

		auto Read_(String& string) -> bool;
		auto Read_(StringView& string) -> bool;
		auto Read_(Buffer& buffer) -> bool;
		auto Read_(Blueprint& blueprint) -> bool;

		template<typename ItemType, int N> auto Read_(Array<ItemType, N>& array) -> bool;
		template<typename ItemType> auto Read_(List<ItemType>& list) -> bool;
		template<typename KeyType, typename ItemType> auto Read_(Map<KeyType, ItemType>& map) -> bool;
		template<typename ClassType> auto Read_(ClassType& item) -> bool;

		template<typename T> auto ReadString(T& value) -> bool;
		template<typename I> auto ReadArray(I iterator) -> bool;
		template<typename I> auto ReadObject(I iterator) -> bool;

		template<typename T> void Serialize(T&& value);
		template<typename T> void Serialize(StringView name, T&& value);
		template<typename T> void Serialize(StringView name, T&& value, const T& defaultValue);
	};
}

template<typename T>
constexpr auto Pargon::BlueprintReader::CanRead() -> bool
{
	constexpr auto _canReadAsMethod = Traits::CanReadAsMethod<T>;
	constexpr auto _canReadAsFunction = Traits::CanReadAsFunction<T>;
	constexpr auto _canSerializeAsMethod = SerializationTraits::CanSerializeAsMethod<T>;
	constexpr auto _canSerializeAsFunction = SerializationTraits::CanSerializeAsFunction<T>;
	constexpr auto _canReadAsEnum = std::is_enum<T>::value && SerializationTraits::HasNames<T>;

	return _canReadAsMethod || _canReadAsFunction || _canSerializeAsMethod || _canSerializeAsFunction || _canReadAsEnum;
}

inline
auto Pargon::BlueprintReader::GetBlueprint() const -> const Blueprint&
{
	return *_current;
}

inline
auto Pargon::BlueprintReader::ExtractBlueprint() const -> Blueprint
{
	return std::move(*_current);
}

inline
auto Pargon::BlueprintReader::HasFailed() const -> bool
{
	return _hasFailed;
}

template<typename T>
void Pargon::BlueprintReader::Read(T& value)
{
	Read_(value);
}

template<typename ItemType, int N>
auto Pargon::BlueprintReader::Read_(Array<ItemType, N>& array) -> bool
{
	return ReadArray([&](int index)
	{
		if (index >= N)
			return false;

		return Read(array.Item(index));
	});
}

template<typename ItemType>
auto Pargon::BlueprintReader::Read_(List<ItemType>& list) -> bool
{
	list.Clear();

	return ReadArray([&](int index)
	{
		auto& item = list.Increment();
		return Read(item);
	});
}

template<typename KeyType, typename ItemType>
auto Pargon::BlueprintReader::Read_(Map<KeyType, ItemType>& map) -> bool
{
	return ReadObject([&](StringView name)
	{
		auto key = ReadFromString<KeyType>(name);
		auto& item = map.AddOrGet(key, {});
		return Read(item);
	});
}

template<typename ClassType>
auto Pargon::BlueprintReader::Read_(ClassType& item) -> bool
{
	constexpr auto readAsMethod = Traits::CanReadAsMethod<ClassType>;
	constexpr auto readAsFunction = Traits::CanReadAsFunction<ClassType>;
	constexpr auto serializeAsMethod = SerializationTraits::CanSerializeAsMethod<ClassType>;
	constexpr auto serializeAsFunction = SerializationTraits::CanSerializeAsFunction<ClassType>;
	constexpr auto readAsString = StringReader::CanRead<ClassType>();

	static_assert(readAsMethod || readAsFunction || serializeAsMethod || serializeAsFunction || readAsString, "ClassType does not support reading from a Blueprint");

	if constexpr (readAsMethod)
		item.FromBlueprint(*this);
	else if constexpr (readAsFunction)
		FromBlueprint(item, *this);
	else if constexpr (serializeAsMethod)
		item.Serialize(*this);
	else if constexpr (serializeAsFunction)
		Serialize(item, *this);
	else if constexpr (readAsString)
		ReadString(item);

	return false;
}

template<typename T>
auto Pargon::BlueprintReader::ReadString(T& value) -> bool
{
	auto string = _current->AsStringView();
	auto reader = StringReader(string);
	return reader.Read(value, "");
}

template<typename I>
auto Pargon::BlueprintReader::ReadArray(I iterator) -> bool
{
	auto array = _current->AsArray();
	if (array == nullptr)
		return false;

	auto& children = array->Children;
	for (auto i = 0; i < children.Count(); i++)
	{
		auto previous = _current;
		_current = std::addressof(children.Item(i));
		iterator(i);
		_current = previous;
	}

	return true;
}

template<typename I>
auto Pargon::BlueprintReader::ReadObject(I iterator) -> bool
{
	auto object = _current->AsObject();
	if (object == nullptr)
		return false;

	for (auto& key : object->Children.Keys())
	{
		auto& item = object->Children.ItemWithKey(key);
		auto previous = _current;
		_current = std::addressof(item);
		iterator(key);
		_current = previous;
	}

	return true;
}

template<typename T>
auto Pargon::BlueprintReader::ReadChild(int index, T& value) -> bool
{
	auto object = _current->AsArray();
	if (object == nullptr)
		return false;

	if (index >= object->Children.Count())
		return false;

	if (MoveDown(index))
	{
		Read_(value);
		MoveUp();
	}

	return true;
}

template<typename T>
auto Pargon::BlueprintReader::ReadChild(StringView name, T& value) -> bool
{
	auto object = _current->AsObject();
	if (object == nullptr)
		return false;

	if (MoveDown(name))
	{
		Read_(value);
		MoveUp();
	}

	return true;
}

template<typename T>
void Pargon::BlueprintReader::Serialize(T&& value)
{
	Read(value);
}

template<typename T>
void Pargon::BlueprintReader::Serialize(StringView name, T&& value)
{
	Serialize(std::forward<T>(value));
}

template<typename T>
void Pargon::BlueprintReader::Serialize(StringView name, T&& value, const T& defaultValue)
{
	Serialize(std::forward<T>(value));
}
