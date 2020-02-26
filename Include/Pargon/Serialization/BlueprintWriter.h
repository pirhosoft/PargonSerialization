#pragma once

#include "Pargon/Containers/Blueprint.h"
#include "Pargon/Containers/String.h"
#include "Pargon/Serialization/Serialization.h"
#include "Pargon/Serialization/StringWriter.h"

namespace Pargon
{
	class BufferView;
	template<typename KeyType, typename T> class Map;
	template<typename T> class SequenceView;

	class BlueprintWriter
	{
	public:
		template<typename T> static constexpr auto CanWrite() -> bool;

		auto GetBlueprint() const -> const Blueprint&;
		auto ExtractBlueprint() -> Blueprint;

		void MoveDown(int index);
		void MoveDown(StringView child);
		auto MoveUp() -> bool;
		auto MoveNext() -> bool;

		template<typename T> void Write(const T& item);

	private:
		friend class Serializer;

		class Traits
		{
		private:
			template<typename U> using void_t = void;
			template<typename U, typename V, typename = void> struct HasToBlueprintMethod : std::false_type {};
			template<typename U, typename V, typename = void> struct HasToBlueprintFunction : std::false_type {};
			template<typename U, typename V> struct HasToBlueprintMethod<U, V, void_t<decltype(static_cast<void>(0), 0, std::declval<const U>().ToBlueprint(std::declval<V&>()))>> : std::true_type {};
			template<typename U, typename V> struct HasToBlueprintFunction<U, V, void_t<decltype(static_cast<void>(0), 0, ToBlueprint(std::declval<U>(), std::declval<V&>()))>> : std::true_type {};

		public:
			template<typename T> static constexpr bool CanWriteAsMethod = HasToBlueprintMethod<T, BlueprintWriter>::value;
			template<typename T> static constexpr bool CanWriteAsFunction = HasToBlueprintFunction<T, BlueprintWriter>::value;
		};

		struct BlueprintTreeNode
		{
			Blueprint* Node;
			int Index;
		};

		List<BlueprintTreeNode> _tree;

		Blueprint _blueprint;
		Blueprint* _current = std::addressof(_blueprint);
		int _currentIndex;

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

		void WriteString(StringView string);
		template<typename ItemType> void WriteSequence(SequenceView<ItemType> sequence);
		template<typename ClassType> void ConvertToString(const ClassType& item);

		template<typename T> void Serialize(T&& value);
		template<typename T> void Serialize(StringView name, T&& value);
		template<typename T> void Serialize(StringView name, T&& value, const T& defaultValue);
	};
}

template<typename T>
constexpr auto Pargon::BlueprintWriter::CanWrite() -> bool
{
	constexpr auto _canWriteAsMethod = Traits::CanWriteAsMethod<T>;
	constexpr auto _canWriteAsFunction = Traits::CanWriteAsFunction<T>;
	constexpr auto _canSerializeAsMethod = SerializationTraits::CanSerializeAsMethod<T>;
	constexpr auto _canSerializeAsFunction = SerializationTraits::CanSerializeAsFunction<T>;
	constexpr auto _canWriteAsSequence = SerializationTraits::CanViewAsSequence<T>;
	constexpr auto _canWriteAsString = StringWriter::CanWrite<T>();

	return _canWriteAsMethod || _canWriteAsFunction || _canSerializeAsMethod || _canSerializeAsFunction || _canWriteAsString || _canWriteAsSequence;
}

inline
auto Pargon::BlueprintWriter::GetBlueprint() const-> const Blueprint&
{
	return _blueprint;
}

inline
auto Pargon::BlueprintWriter::ExtractBlueprint() -> Blueprint
{
	_current = nullptr;
	auto blueprint = std::move(_blueprint);
	return blueprint;
}

template<typename T>
void Pargon::BlueprintWriter::Write(const T& item)
{
	Write_(item);
}

template<typename KeyType, typename ItemType>
void Pargon::BlueprintWriter::Write_(const Map<KeyType, ItemType>& map)
{
	_current->SetToObject();

	for (auto i = 0; i < map.Count(); i++)
	{
		auto key = WriteToString(map.GetKey(i));
		MoveDown(key);
		Write_(key, map.ItemAtIndex(i));
		MoveUp();
	}
}

template<typename ClassType>
void Pargon::BlueprintWriter::Write_(const ClassType& item)
{
	constexpr auto writeAsMethod = Traits::CanWriteAsMethod<ClassType>;
	constexpr auto writeAsFunction = Traits::CanWriteAsFunction<ClassType>;
	constexpr auto serializeAsMethod = SerializationTraits::CanSerializeAsMethod<ClassType>;
	constexpr auto serializeAsFunction = SerializationTraits::CanSerializeAsFunction<ClassType>;
	constexpr auto writeAsSequence = SerializationTraits::CanViewAsSequence<ClassType>;
	constexpr auto writeAsString = SerializationTraits::CanViewAsString<ClassType>;
	constexpr auto convertToString = StringWriter::CanWrite<ClassType>();

	if constexpr (writeAsMethod)
		item.ToBlueprint(*this);
	else if constexpr (writeAsFunction)
		ToBlueprint(item, *this);
	else if constexpr(serializeAsMethod)
		item.Serialize(*this);
	else if constexpr (serializeAsFunction)
		Serialize(item, *this);
	else if constexpr (writeAsSequence)
		WriteSequence<SerializationTraits::SequenceType<ClassType>>(item);
	else if constexpr (writeAsString)
		WriteString(item);
	else if constexpr (convertToString)
		WriteString(item);
}

template<typename ItemType>
void Pargon::BlueprintWriter::WriteSequence(SequenceView<ItemType> sequence)
{
	_current->SetToArray();

	auto index = 0;
	for (auto& item : sequence)
	{
		MoveDown(index++);
		Write_(item);
		MoveUp();
	}
}

template<typename ClassType>
void Pargon::BlueprintWriter::ConvertToString(const ClassType& item)
{
	auto string = WriteToString(item, {});
	WriteString(string);
}

template<typename T>
void Pargon::BlueprintWriter::Serialize(T&& value)
{
	Write(value);
}

template<typename T>
void Pargon::BlueprintWriter::Serialize(StringView name, T&& value)
{
	MoveDown(name);
	Serialize(std::forward<T>(value));
	MoveUp();
}

template<typename T>
void Pargon::BlueprintWriter::Serialize(StringView name, T&& value, const T& defaultValue)
{
	if (value != defaultValue)
		Serialize(name, std::forward<T>(value));
}
