#pragma once

#include "Pargon/Containers/Array.h"
#include "Pargon/Containers/Buffer.h"
#include "Pargon/Containers/List.h"
#include "Pargon/Containers/Sequence.h"
#include "Pargon/Containers/String.h"
#include "Pargon/Containers/Text.h"

#include <type_traits>
#include <utility>

namespace Pargon
{
	class Serializer;

	template<typename... Names>
	constexpr auto SetEnumNames(Names&&... names) -> Array<StringView, sizeof...(Names)>
	{
		return {{ std::forward<Names>(names)... }};
	}

	template<typename EnumType> Array<StringView, 0> EnumNames;

	template<typename T>
	struct FormatArgument
	{
		StringView Name;
		T& Value;
	};

	template<typename T> auto NamedArgument(StringView Name, T& value) -> FormatArgument<T>;

	class SerializationTraits
	{
	private:
		static_assert(std::numeric_limits<unsigned char>::digits == 8, "char is not 8 bits");
		static_assert(std::numeric_limits<float>::is_iec559, "float is not IEE 754");
		static_assert(std::numeric_limits<double>::is_iec559, "double is not IEE 754");
		static_assert(sizeof(float) == 4, "float size is not 4 bytes");
		static_assert(sizeof(double) == 8, "double size is not 8 bytes.");

		template<typename T>
		struct Primitive
		{
		private:
			static_assert(std::is_arithmetic<T>::value, "T must be an integral or floating point type");

			template<typename, typename> struct Normalized;
			template<typename U> struct Normalized<bool, U> { using Type = int8_t; };
			template<typename U> struct Normalized<char, U> { using Type = int8_t; };
			template<typename U> struct Normalized<char16_t, U> { using Type = int16_t; };
			template<typename U> struct Normalized<char32_t, U> { using Type = int32_t; };
			template<typename U> struct Normalized<wchar_t, U> { using Type = int32_t; };
			template<typename U> struct Normalized<signed char, U> { using Type = int8_t; };
			template<typename U> struct Normalized<short, U> { using Type = int16_t; };
			template<typename U> struct Normalized<int, U> { using Type = int32_t; };
			template<typename U> struct Normalized<long, U> { using Type = int32_t; };
			template<typename U> struct Normalized<long long, U> { using Type = int64_t; };
			template<typename U> struct Normalized<unsigned char, U> { using Type = uint8_t; };
			template<typename U> struct Normalized<unsigned short, U> { using Type = uint16_t; };
			template<typename U> struct Normalized<unsigned int, U> { using Type = uint32_t; };
			template<typename U> struct Normalized<unsigned long, U> { using Type = uint32_t; };
			template<typename U> struct Normalized<unsigned long long, U> { using Type = uint64_t; };
			template<typename U> struct Normalized<float, U> { using Type = float; };
			template<typename U> struct Normalized<double, U> { using Type = double; };
			template<typename U> struct Normalized<long double, U> { using Type = double; };

		public:
			using Type = typename Normalized<T, T>::Type;
			static constexpr std::size_t Size = sizeof(Type);
		};

		template<typename T>
		class Enum
		{
		private:
			template<typename U> struct HasNonEmptyNames : std::integral_constant<bool, !EnumNames<T>.IsEmpty()> {};

		public:
			static constexpr bool HasNames = HasNonEmptyNames<T>::value;
		};

		template<typename T> struct Buffer : std::integral_constant<bool, std::is_convertible_v<T, BufferView>> {};
		template<typename T> struct String : std::integral_constant<bool, std::is_convertible_v<T, StringView>> {};
		template<typename T> struct Text : std::integral_constant<bool, std::is_convertible_v<T, TextView>> {};

		template<typename T> struct Sequence : std::false_type {};
		template<template<typename> typename SequenceType, typename ItemType> struct Sequence<SequenceType<ItemType>> : std::integral_constant<bool, std::is_convertible_v<SequenceType<ItemType>, SequenceView<ItemType>>> { using type = ItemType; };
		template<template<typename, int> typename SequenceType, typename ItemType, int N> struct Sequence<SequenceType<ItemType, N>> : std::integral_constant<bool, std::is_convertible_v<SequenceType<ItemType, N>, SequenceView<ItemType>>> { using type = ItemType; };

		template<typename T>
		class Class
		{
		private:
			template<typename U> using void_t = void;
			template<typename U, typename V, typename = void> struct HasSerializeMethod : std::false_type {};
			template<typename U, typename V, typename = void> struct HasSerializeFunction : std::false_type {};
			template<typename U, typename V> struct HasSerializeMethod<U, V, void_t<decltype(std::declval<U>().Serialize(std::declval<V&>()))>> : std::true_type {};
			template<typename U, typename V> struct HasSerializeFunction<U, V, void_t<decltype(Serialize(std::declval<U&>(), std::declval<V&>()))>> : std::true_type {};

		public:
			static constexpr bool CanSerializeAsMethod = HasSerializeMethod<T, Serializer>::value;
			static constexpr bool CanSerializeAsFunction = HasSerializeFunction<T, Serializer>::value;
			static constexpr bool CanSerialize = CanSerializeAsMethod || CanSerializeAsFunction;
		};

		template<typename T> struct FormatArgumentTest : std::false_type {};
		template<typename T> struct FormatArgumentTest<FormatArgument<T>> : std::true_type {};

	public:
		template<typename T> using NormalizedType = typename Primitive<T>::Type;
		template<typename T> static constexpr std::size_t NormalizedSize = sizeof(NormalizedType<T>);

		template<typename T> static constexpr bool HasNames = Enum<T>::HasNames;

		template<typename T> static constexpr bool CanSerializeAsMethod = Class<T>::CanSerializeAsMethod;
		template<typename T> static constexpr bool CanSerializeAsFunction = Class<T>::CanSerializeAsFunction;
		template<typename T> static constexpr bool CanViewAsBuffer = Buffer<T>::value;
		template<typename T> static constexpr bool CanViewAsString = String<T>::value;
		template<typename T> static constexpr bool CanViewAsText = Text<T>::value;
		template<typename T> static constexpr bool CanViewAsSequence = Sequence<T>::value;

		template<typename T> using SequenceType = typename Sequence<T>::type;

		template<typename T> static constexpr bool IsFormatArgument = FormatArgumentTest<T>::value;
	};

	struct FormatToken
	{
		static constexpr int NoParameter = -1;
		static constexpr int NamedParameter = -2;

		int ParameterIndex;
		StringView ParameterName;
		StringView Specification;
	};

	struct StringFormat
	{
		List<FormatToken> Tokens;
	};

	struct TextFormat
	{
		List<FormatToken> Tokens;
	};

	auto ParseFormatString(StringView format) -> StringFormat;
	auto ParseFormatText(TextView format) -> TextFormat;
}

template<typename T>
auto Pargon::NamedArgument(StringView Name, T& value) -> FormatArgument<T>
{
	return { name, value };
}
