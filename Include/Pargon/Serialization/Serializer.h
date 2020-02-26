#pragma once

#include "Pargon/Serialization/BlueprintReader.h"
#include "Pargon/Serialization/BlueprintWriter.h"
#include "Pargon/Serialization/BufferReader.h"
#include "Pargon/Serialization/BufferWriter.h"
#include "Pargon/Serialization/StringReader.h"
#include "Pargon/Serialization/StringWriter.h"

#include <variant>

namespace Pargon
{
	class Serializer
	{
	public:
		Serializer(BufferReader& reader);
		Serializer(BufferWriter& writer);
		Serializer(StringReader& reader);
		Serializer(StringWriter& writer);
		Serializer(BlueprintReader& reader);
		Serializer(BlueprintWriter& writer);

		template<typename T> void Serialize(T&& value);
		template<typename T> void Serialize(StringView name, T&& value);
		template<typename T> void Serialize(StringView name, T&& value, const T& defaultValue);

	private:
		using Variant = std::variant<
			std::reference_wrapper<BufferReader>,
			std::reference_wrapper<BufferWriter>,
			std::reference_wrapper<StringReader>,
			std::reference_wrapper<StringWriter>,
			std::reference_wrapper<BlueprintReader>,
			std::reference_wrapper<BlueprintWriter>
		>;

		Variant _serializer;
	};
}

inline
Pargon::Serializer::Serializer(BufferReader& reader) :
	_serializer(reader)
{
}

inline
Pargon::Serializer::Serializer(BufferWriter& writer) :
	_serializer(writer)
{
}

inline
Pargon::Serializer::Serializer(StringReader& reader) :
	_serializer(reader)
{
}

inline
Pargon::Serializer::Serializer(StringWriter& writer) :
	_serializer(writer)
{
}

inline
Pargon::Serializer::Serializer(BlueprintReader& reader) :
	_serializer(reader)
{
}

inline
Pargon::Serializer::Serializer(BlueprintWriter& writer) :
	_serializer(writer)
{
}

template<typename T>
void Pargon::Serializer::Serialize(T&& value)
{
	std::visit([&](auto&& variant)
	{
		variant.get().Serialize(std::forward<T>(value));
	}, _serializer);
}

template<typename T>
void Pargon::Serializer::Serialize(StringView name, T&& value)
{
	std::visit([&](auto&& variant)
	{
		variant.get().Serialize(name, std::forward<T>(value));
	}, _serializer);
}

template<typename T>
void Pargon::Serializer::Serialize(StringView name, T&& value, const T& defaultValue)
{
	std::visit([&](auto&& variant)
	{
		variant.get().Serialize(name, std::forward<T>(value), defaultValue);
	}, _serializer);
}
