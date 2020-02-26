#pragma once

#include "Pargon/Containers/String.h"
#include "Pargon/Serialization/Serialization.h"
#include "Pargon/Serialization/StringWriter.h"

#include <type_traits>
#include <utility>

namespace Pargon
{
	template<typename ItemType, int N> class Array;
	class Blueprint;
	class Buffer;
	template<typename ItemType> class List;
	template<typename KeyType, typename ItemType> class Map;

	class StringReader
	{
	public:
		struct Error
		{
			int Line;
			int Column;
			String Message;
		};

		template<typename T> static constexpr auto CanRead() -> bool;

		StringReader(StringView text);

		auto Index() const -> int;
		auto Remaining() const -> int;
		auto AtEnd() const -> bool;

		auto HasFailed() const -> bool;
		void ReportError(StringView message);

		auto ViewTo(int count) const -> StringView;
		auto ViewNext() const -> char;
		auto ViewNext(int index) const -> char;
		auto ViewRemaining() const -> StringView;
		auto ViewPrevious(int count) const -> StringView;

		auto ReadTo(int count) -> StringView;
		auto ReadNext() -> char;

		bool Advance(int count);
		bool Retreat(int count);
		bool MoveTo(int index);
		void Reset();

		auto AdvanceTo(StringView string, bool ignoreCase) -> int;
		auto AdvanceToWhitespace() -> int;
		auto AdvanceToAny(StringView characters, bool ignoreCase) -> int;
		auto AdvanceToExpression(StringView pattern, bool ignoreCase) -> int;

		auto AdvancePast(StringView string, bool ignoreCase) -> int;
		auto AdvancePastWhitespace() -> int;
		auto AdvancePastAny(StringView characters, bool ignoreCase) -> int;
		auto AdvancePastExpression(StringView pattern, bool ignoreCase) -> int;

		auto RetreatTo(StringView string, bool ignoreCase) -> int;
		auto RetreatToWhitespace() -> int;
		auto RetreatToAny(StringView characters, bool ignoreCase) -> int;
		auto RetreatToExpression(StringView pattern, bool ignoreCase) -> int;

		auto RetreatPast(StringView string, bool ignoreCase) -> int;
		auto RetreatPastWhitespace() -> int;
		auto RetreatPastAny(StringView characters, bool ignoreCase) -> int;
		auto RetreatPastExpression(StringView pattern, bool ignoreCase) -> int;

		template<typename T> auto Read(T& value, StringView format) -> bool;
		template<typename... Ts> auto Parse(StringView format, Ts&... inputs) -> bool;
		template<typename... Ts> auto Parse(StringFormat format, Ts&... inputs) -> bool;

	private:
		friend class Serializer;

		struct Traits
		{
			template<typename U> using void_t = void;
			template<typename U, typename V, typename = void> struct HasFromStringMethod : std::false_type {};
			template<typename U, typename V, typename = void> struct HasFromStringFunction : std::false_type {};
			template<typename U, typename V> struct HasFromStringMethod<U, V, void_t<decltype(static_cast<void>(0), static_cast<void>(0), 0, std::declval<U>().FromString(std::declval<V&>(), std::declval<StringView>()))>> : std::true_type {};
			template<typename U, typename V> struct HasFromStringFunction<U, V, void_t<decltype(static_cast<void>(0), static_cast<void>(0), 0, FromString(std::declval<U&>(), std::declval<V&>(), std::declval<StringView>()))>> : std::true_type {};

			template<typename T> static constexpr bool CanReadAsMethod = HasFromStringMethod<T, StringReader>::value;
			template<typename T> static constexpr bool CanReadAsFunction = HasFromStringFunction<T, StringReader>::value;
		};

		const char* _data;
		int _length;
		int _index;

		bool _hasFailed = false;
		List<Error> _errors;

		void Read_(char& character, StringView format);
		void Read_(wchar_t& character, StringView format);
		void Read_(char16_t& character, StringView format);
		void Read_(char32_t& character, StringView format);
		void Read_(bool& boolean, StringView format);
		void Read_(signed char& number, StringView format);
		void Read_(short& number, StringView format);
		void Read_(int& number, StringView format);
		void Read_(long& number, StringView format);
		void Read_(long long& number, StringView format);
		void Read_(unsigned char& number, StringView format);
		void Read_(unsigned short& number, StringView format);
		void Read_(unsigned int& number, StringView format);
		void Read_(unsigned long& number, StringView format);
		void Read_(unsigned long long& number, StringView format);
		void Read_(float& number, StringView format);
		void Read_(double& number, StringView format);
		void Read_(long double& number, StringView format);

		void Read_(String& string, StringView format);
		void Read_(StringView& string, StringView format);
		void Read_(Text& text, StringView format);
		void Read_(Buffer& buffer, StringView format);
		void Read_(Blueprint& blueprint, StringView format);

		template<typename ItemType, int N> void Read_(Array<ItemType, N>& array, StringView format);
		template<typename ItemType> void Read_(List<ItemType>& list, StringView format);
		template<typename KeyType, typename ItemType> void Read_(Map<KeyType, ItemType>& map, StringView format);
		template<typename ClassType> void Read_(ClassType& item, StringView format);

		template<typename EnumType> void ReadEnum(EnumType& value, StringView Format);

		template<typename T> void Serialize(T&& value);
		template<typename T> void Serialize(StringView name, T&& value);
		template<typename T> void Serialize(StringView name, T&& value, const T& defaultValue);

		static void ReadNamedParameter(StringReader& reader, const FormatToken& token)
		{
			reader.ReportError(FormatString("no parameter with name {}", token.ParameterName));
		}

		template<typename U, typename... Us>
		static void ReadNamedParameter(StringReader& reader, const FormatToken& token, U& parameter, Us&... parameters)
		{
			constexpr auto isFormatArgument = SerializationTraits::IsFormatArgument<U>;

			if constexpr (isFormatArgument)
			{
				if (Equals(parameter.Name, token.ParameterName))
					reader.Read(parameter.Value, token.Specification);
				else
					ReadNamedParameter(reader, token, parameters...);
			}
			else
			{
				ReadNamedParameter(reader, token, parameters...);
			}
		}

		template<int N>
		static void ReadIndexedParameter(StringReader& reader, const FormatToken& token)
		{
			reader.ReportError(FormatString("no parameter with index {}", token.ParameterIndex));
		}

		template<int N, typename U, typename... Us>
		static void ReadIndexedParameter(StringReader& reader, const FormatToken& token, U& parameter, Us&... parameters)
		{
			if (token.ParameterIndex == N)
				reader.Read(parameter, token.Specification);
			else
				ReadIndexedParameter<N + 1>(reader, token, parameters...);
		}
	};

	template<typename T> auto ReadFromString(StringView string) -> T;
	template<typename... Ts> auto ParseString(StringView format, Ts&... inputs) -> bool;
	template<typename... Ts> auto ParseString(StringFormat format, Ts&... inputs) -> bool;
}

template<typename T>
constexpr auto Pargon::StringReader::CanRead() -> bool
{
	constexpr auto _canReadAsMethod = Traits::CanReadAsMethod<T>;
	constexpr auto _canReadAsFunction = Traits::CanReadAsFunction<T>;
	constexpr auto _canSerializeAsMethod = SerializationTraits::CanSerializeAsMethod<T>;
	constexpr auto _canSerializeAsFunction = SerializationTraits::CanSerializeAsFunction<T>;
	constexpr auto _canReadAsEnum = std::is_enum<T>::value && SerializationTraits::HasNames<T>;

	return _canReadAsMethod || _canReadAsFunction || _canSerializeAsMethod || _canSerializeAsFunction || _canReadAsEnum;
}

inline
auto Pargon::StringReader::Index() const -> int
{
	return _index;
}

inline
auto Pargon::StringReader::Remaining() const -> int
{
	return _length - _index;
}

inline
auto Pargon::StringReader::AtEnd() const -> bool
{
	return Remaining() == 0;
}

inline
auto Pargon::StringReader::HasFailed() const -> bool
{
	return _hasFailed;
}

template<typename T>
auto Pargon::StringReader::Read(T& item, StringView format) -> bool
{
	auto errors = _errors.Count();
	Read_(item, format);
	return errors == _errors.Count();
}

template<typename... Ts>
auto Pargon::StringReader::Parse(StringView format, Ts&... inputs) -> bool
{
	auto tokens = ParseFormatString(format);
	return Parse(tokens, inputs...);
}

template<typename... Ts>
auto Pargon::StringReader::Parse(StringFormat format, Ts&... inputs) -> bool
{
	auto errors = _errors.Count();

	StringView temporary;

	for (auto& token : format.Tokens)
	{
		if (token.ParameterIndex == FormatToken::NoParameter)
			Read(temporary, token.Specification);
		else if (token.ParameterIndex == FormatToken::NamedParameter)
			ReadNamedParameter(*this, token, inputs...);
		else
			ReadIndexedParameter<0>(*this, token, inputs...);
	}

	return errors == _errors.Count();
}

template<typename ItemType, int N>
void Pargon::StringReader::Read_(Array<ItemType, N>& array, StringView format)
{
	if (ViewNext() != '[')
	{
		ReportError("expected '['");
		return;
	}

	auto start = Index();
	Advance(1);
	AdvancePastWhitespace();

	Array<ItemType, N> placeholder;

	auto first = true;
	for (auto& child : placeholder)
	{
		if (!first)
		{
			if (ViewNext() != ',')
			{
				ReportError("expected ','");
				MoveTo(start);
				return;
			}

			Advance(1);
			AdvancePastWhitespace();
		}

		Read_(child, format);

		if (HasFailed())
		{
			MoveTo(start);
			return;
		}

		AdvancePastWhitespace();
		first = false;
	}

	if (ViewNext() != ']')
	{
		ReportError("expected ']'");
		MoveTo(start);
		return;
	}

	array = std::move(placeholder);
}

template<typename ItemType>
void Pargon::StringReader::Read_(List<ItemType>& list, StringView format)
{
	if (ViewNext() != '[')
	{
		Results.ReportError(_index, "expected '['");
		return;
	}

	auto start = Index();
	Advance(1);
	AdvancePastWhitespace();

	List<ItemType> placeholder;

	auto first = true;
	while (ViewNext() != ']')
	{
		if (!first)
		{
			if (ViewNext() != ',')
			{
				Results.ReportError(_index, "expected ','");
				MoveTo(start);
				return;
			}

			Advance(1);
			AdvancePastWhitespace();
		}

		auto& child = placeholder.Increment();

		if (!Read_(child, format))
		{
			MoveTo(start);
			return;
		}

		AdvancePastWhitespace();
		first = false;
	}

	list = std::move(placeholder);
}

template<typename KeyType, typename ItemType>
void Pargon::StringReader::Read_(Map<KeyType, ItemType>& map, StringView format)
{
	if (ViewNext() != '{')
	{
		Results.ReportError(_index, "expected '{'");
		return;
	}

	auto start = Index();
	Advance(1);
	AdvancePastWhitespace();

	Map<KeyType, ItemType> placeholder;

	auto first = true;
	while (ViewNext() != '}')
	{
		if (!first)
		{
			if (ViewNext() != ',')
			{
				Results.ReportError(_index, "expected ','");
				MoveTo(start);
				return;
			}

			Advance(1);
			AdvancePastWhitespace();
		}

		KeyType key;
		if (!Read_(key, format))
		{
			MoveTo(start);
			return;
		}

		if (ViewNext() != ':')
		{
			Results.ReportError(_index, "expected ':'");
			MoveTo(start);
			return;
		}

		Advance(1);
		AdvancePastWhitespace();

		auto& child = placeholder.AddOrGet(key, {});

		if (!Read_(child, format))
		{
			MoveTo(start);
			return;
		}

		AdvancePastWhitespace();
		first = false;
	}

	map = std::move(placeholder);
}

template<typename ClassType>
void Pargon::StringReader::Read_(ClassType& item, StringView format)
{
	static_assert(CanRead<ClassType>(), "ClassType does not support reading from a String");

	constexpr auto readAsMethod = Traits::CanReadAsMethod<ClassType>;
	constexpr auto readAsFunction = Traits::CanReadAsFunction<ClassType>;
	constexpr auto serializeAsMethod = SerializationTraits::CanSerializeAsMethod<ClassType>;
	constexpr auto serializeAsFunction = SerializationTraits::CanSerializeAsFunction<ClassType>;
	constexpr auto readAsEnum = std::is_enum<ClassType>::value && SerializationTraits::HasNames<ClassType>;

	if constexpr (readAsMethod)
		item.FromString(*this, format);
	else if constexpr (readAsFunction)
		FromString(item, *this, format);
	else if constexpr (serializeAsMethod)
		item.Serialize(*this, format);
	else if constexpr (serializeAsFunction)
		Serialize(item, *this, format);
	else if constexpr (readAsEnum)
		ReadEnum(item, format);
}

template<typename EnumType>
void Pargon::StringReader::ReadEnum(EnumType& value, StringView Format)
{
	int i;
	for (i = 0; i < EnumNames<EnumType>.Count(); i++)
	{
		auto name = EnumNames<EnumType>.Item(i);

		if (Equals(ViewRemaining(), name, true))
		{
			value = static_cast<EnumType>(i);
			Advance(name.Length());
			break;
		}
	}

	if (i == EnumNames<EnumType>.Count())
		ReportError("could not read as enum");
}

template<typename T>
void Pargon::StringReader::Serialize(T&& value)
{
	Read(value, {});
}

template<typename T>
void Pargon::StringReader::Serialize(StringView name, T&& value)
{
	Serialize(std::forward<T>(value));
}

template<typename T>
void Pargon::StringReader::Serialize(StringView name, T&& value, const T& defaultValue)
{
	Serialize(std::forward<T>(value));
}

template<typename T>
auto Pargon::ReadFromString(StringView string) -> T
{
	T item;
	StringReader reader(string);
	reader.Read(item, {});
	return item;
}

template<typename... Ts>
auto Pargon::ParseString(StringView format, Ts&... inputs) -> bool
{
	StringReader reader;
	reader.Parse(format, inputs...);
	return !reader.HasFailed();
}

template<typename... Ts>
auto Pargon::ParseString(StringFormat format, Ts&... inputs) -> bool
{
	StringReader reader;
	reader.Parse(format, inputs...);
	return !reader.HasFailed();
}