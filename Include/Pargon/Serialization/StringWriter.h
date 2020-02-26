#pragma once

#include "Pargon/Containers/String.h"
#include "Pargon/Serialization/Serialization.h"

namespace Pargon
{
	class Blueprint;
	class BufferView;
	template<typename KeyType, typename T> class Map;
	template<typename T> class SequenceView;
	class StringView;
	class Text;

	class StringWriter
	{
	public:
		template<typename T> static constexpr auto CanWrite() -> bool;

		auto GetString() const -> StringView;
		auto ExtractString() -> String;

		void Write(StringView string);
		template<typename T> void Write(const T& value, StringView format);
		template<typename... Ts> void Format(StringView format, const Ts&... inputs);
		template<typename... Ts> void Format(StringFormat format, const Ts&... inputs);

	private:
		friend class Serializer;

		struct Traits
		{
			template<typename U> using void_t = void;
			template<typename U, typename V, typename = void> struct HasToStringMethod : std::false_type {};
			template<typename U, typename V, typename = void> struct HasToStringFunction : std::false_type {};
			template<typename U, typename V> struct HasToStringMethod<U, V, void_t<decltype(static_cast<void>(0), 0, std::declval<const U>().ToString(std::declval<V&>(), std::declval<StringView>()))>> : std::true_type {};
			template<typename U, typename V> struct HasToStringFunction<U, V, void_t<decltype(static_cast<void>(0), 0, ToString(std::declval<U>(), std::declval<V&>(), std::declval<StringView>()))>> : std::true_type {};

			template<typename T> static constexpr bool CanWriteAsMethod = HasToStringMethod<T, StringWriter>::value;
			template<typename T> static constexpr bool CanWriteAsFunction = HasToStringFunction<T, StringWriter>::value;
		};

		String _string;

		void Write_(char character, StringView format);
		void Write_(wchar_t character, StringView format);
		void Write_(char16_t character, StringView format);
		void Write_(char32_t character, StringView format);
		void Write_(bool boolean, StringView format);
		void Write_(signed char number, StringView format);
		void Write_(short number, StringView format);
		void Write_(int number, StringView format);
		void Write_(long number, StringView format);
		void Write_(long long number, StringView format);
		void Write_(unsigned char number, StringView format);
		void Write_(unsigned short number, StringView format);
		void Write_(unsigned int number, StringView format);
		void Write_(unsigned long number, StringView format);
		void Write_(unsigned long long number, StringView format);
		void Write_(float number, StringView format);
		void Write_(double number, StringView format);
		void Write_(long double number, StringView format);

		void Write_(const Blueprint& blueprint, StringView format);

		template<typename KeyType, typename ItemType> void Write_(const Map<KeyType, ItemType>& map, StringView format);
		template<typename ClassType> void Write_(const ClassType& item, StringView format);

		void WriteBuffer(BufferView buffer, StringView format);
		void WriteString(StringView string, StringView format);
		void WriteText(TextView text, StringView format);
		template<typename EnumType> void WriteEnum(EnumType value, StringView format);
		template<typename ItemType> void WriteSequence(SequenceView<ItemType> sequence, StringView format);

		static void WriteNamedParameter(StringWriter& writer, const FormatToken& token) {}
		template<int N> static void WriteIndexedParameter(StringWriter& writer, const FormatToken& token) {}
		template<typename U, typename... Us> static void WriteNamedParameter(StringWriter& writer, const FormatToken& token, const U& parameter, const Us&... parameters);
		template<int N, typename U, typename... Us> static void WriteIndexedParameter(StringWriter& writer, const FormatToken& token, const U& parameter, const Us&... parameters);

		template<typename T> void Serialize(T&& value);
		template<typename T> void Serialize(StringView name, T&& value);
		template<typename T> void Serialize(StringView name, T&& value, const T& defaultValue);
	};

	template<typename T> auto WriteToString(const T& item, StringView format) -> String;
	template<typename... Ts> auto FormatString(StringView format, const Ts&... inputs) -> String;
	template<typename... Ts> auto FormatString(StringFormat format, const Ts&... inputs) -> String;
}

template<typename T>
constexpr auto Pargon::StringWriter::CanWrite() -> bool
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

	return _canWriteAsMethod || _canWriteAsFunction || _canSerializeAsMethod || _canSerializeAsFunction || _canWriteAsEnum || _canWriteAsBuffer || _canWriteAsString || _canWriteAsText || _canWriteAsSequence;
}

inline
auto Pargon::StringWriter::GetString() const -> StringView
{
	return _string;
}

inline
auto Pargon::StringWriter::ExtractString() -> String
{
	auto string = std::move(_string);
	return string;
}

inline
void Pargon::StringWriter::Write(StringView string)
{
	WriteString(string, "");
}

template<typename T>
void Pargon::StringWriter::Write(const T& item, StringView format)
{
	Write_(item, format);
}

template<typename... Ts>
void Pargon::StringWriter::Format(StringView format, const Ts&... inputs)
{
	auto tokens = ParseFormatString(format);
	Format(tokens, inputs...);
}

template<typename... Ts>
void Pargon::StringWriter::Format(StringFormat format, const Ts&... inputs)
{
	for (auto& token : format.Tokens)
	{
		if (token.ParameterIndex == FormatToken::NoParameter)
			WriteString(token.Specification, {});
		else if (token.ParameterIndex == FormatToken::NamedParameter)
			WriteNamedParameter(*this, token, inputs...);
		else
			WriteIndexedParameter<0>(*this, token, inputs...);
	}
}

template<typename KeyType, typename ItemType>
void Pargon::StringWriter::Write_(const Map<KeyType, ItemType>& map, StringView format)
{
	// formats
	// "{" -> "{ key: item, key: item }" (default)
	// "[" -> "[ key: item, key: item ]"
	// "-" -> "key: item, key: item"

	auto open = format.Length() > 0 ? format.Character(0) : '{';
	auto divider = ": "_sv;
	auto separator = ", "_sv;
	auto close = static_cast<char>(open + 2);

	if (open != '-') Write_(open, {});

	auto first = true;
	for (auto i = 0u; i < map.Count(); i++)
	{
		if (!first)
			WriteString(separator, {});

		Write_(map.GetKey(i), {});
		WriteString(divider, {});
		Write_(map.ItemAtIndex(i), {});
	}

	if (open != '-') Write_(close, {});
}

template<typename ClassType>
void Pargon::StringWriter::Write_(const ClassType& item, StringView format)
{
	static_assert(CanWrite<ClassType>(), "ClassType does not support writing to a String");

	constexpr auto writeAsMethod = Traits::CanWriteAsMethod<ClassType>;
	constexpr auto writeAsFunction = Traits::CanWriteAsFunction<ClassType>;
	constexpr auto serializeAsMethod = SerializationTraits::CanSerializeAsMethod<ClassType>;
	constexpr auto serializeAsFunction = SerializationTraits::CanSerializeAsFunction<ClassType>;
	constexpr auto writeAsEnum = std::is_enum<ClassType>::value && SerializationTraits::HasNames<ClassType>;
	constexpr auto viewAsBuffer = SerializationTraits::CanViewAsBuffer<ClassType>;
	constexpr auto viewAsString = SerializationTraits::CanViewAsString<ClassType>;
	constexpr auto viewAsText = SerializationTraits::CanViewAsText<ClassType>;
	constexpr auto viewAsSequence = SerializationTraits::CanViewAsSequence<ClassType>;

	if constexpr (writeAsMethod)
		item.ToString(*this, format);
	else if constexpr (writeAsFunction)
		ToString(item, *this, format);
	else if constexpr (serializeAsMethod)
		item.Serialize(*this, format);
	else if constexpr (serializeAsFunction)
		Serialize(item, *this, format);
	else if constexpr (writeAsEnum)
		WriteEnum(item, format);
	else if constexpr (viewAsString)
		WriteString(item, format);
	else if constexpr (viewAsText)
		WriteText(item, format);
	else if constexpr (viewAsBuffer)
		WriteBuffer(item, format);
	else if constexpr (viewAsSequence)
		WriteSequence<SerializationTraits::SequenceType<ClassType>>(item, format);
}

template<typename EnumType>
void Pargon::StringWriter::WriteEnum(EnumType value, StringView format)
{
	// formats
	// "n" -> write numeric value
	// "#" -> write hex value
	// empty -> write name (default)

	auto i = static_cast<int>(value);

	if (i < EnumNames<EnumType>.Count())
	{
		if (!format.IsEmpty())
			Write_(i, format);
		else
			WriteString(EnumNames<EnumType>.Item(i), {});
	}
}

template<typename ItemType>
void Pargon::StringWriter::WriteSequence(SequenceView<ItemType> sequence, StringView format)
{
	// formats
	// "{" -> "{ item, item }"
	// "[" -> "[ item, item ]" (default)
	// "-" -> "item, item"

	auto open = format.Length() > 0 ? format.Character(0) : '[';
	auto separator = ", "_sv;
	auto close = static_cast<char>(open + 2);

	if (open != '-') Write_(open, {});

	auto first = true;
	for (auto& item : sequence)
	{
		if (!first)
			WriteString(separator, {});

		Write_(item, {});
		first = false;
	}

	if (open != '-') Write_(close, {});
}

template<typename U, typename... Us>
void Pargon::StringWriter::WriteNamedParameter(StringWriter& writer, const FormatToken& token, const U& parameter, const Us&... parameters)
{
	constexpr auto isFormatArgument = SerializationTraits::IsFormatArgument<U>;

	if constexpr (isFormatArgument)
	{
		if (Equals(parameter.Name, token.ParameterName))
			writer.Write(parameter.Value, token.Specification);
		else
			WriteNamedParameter(writer, token, parameters...);
	}
	else
	{
		WriteNamedParameter(writer, token, parameters...);
	}
}

template<int N, typename U, typename... Us>
void Pargon::StringWriter::WriteIndexedParameter(StringWriter& writer, const FormatToken& token, const U& parameter, const Us&... parameters)
{
	if (token.ParameterIndex == N)
		writer.Write(parameter, token.Specification);
	else
		WriteIndexedParameter<N + 1>(writer, token, parameters...);
}

template<typename T>
void Pargon::StringWriter::Serialize(T&& value)
{
	Write_(value, {});
}

template<typename T>
void Pargon::StringWriter::Serialize(StringView name, T&& value)
{
	Serialize(std::forward<T>(value));
}

template<typename T>
void Pargon::StringWriter::Serialize(StringView name, T&& value, const T& defaultValue)
{
	Serialize(std::forward<T>(value));
}

template<typename T>
auto Pargon::WriteToString(const T& item, StringView format) -> String
{
	StringWriter writer;
	writer.Write(item, format);
	return writer.ExtractString();
}

template<typename... Ts>
auto Pargon::FormatString(StringView format, const Ts&... inputs) -> String
{
	StringWriter writer;
	writer.Format(format, inputs...);
	return writer.ExtractString();
}

template<typename... Ts>
auto Pargon::FormatString(StringFormat format, const Ts&... inputs) -> String
{
	StringWriter writer;
	writer.Format(format, inputs...);
	return writer.ExtractString();
}