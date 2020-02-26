#include "Pargon/Containers/String.h"
#include "Pargon/Serialization/Serialization.h"

using namespace Pargon;

namespace
{
	auto GetView(const char* begin, const char* end) -> StringView
	{
		return { begin, static_cast<int>(end - begin) };
	}

	auto GetView(TextIterator begin, TextIterator end) -> StringView
	{
		return GetView(begin.Data(), end.Data());
	}

	template<typename IteratorType>
	auto FindLiteralEnd(IteratorType begin, IteratorType end) -> IteratorType
	{
		return std::find(begin, end, '{');
	}

	template<typename IteratorType>
	auto IsLiteralBrace(IteratorType begin, IteratorType end) -> bool
	{
		return (begin < end - 1) && (*begin == '{') && (*(begin + 1) == '{');
	}

	template<typename IteratorType>
	auto FindIdEnd(IteratorType begin, IteratorType end) -> IteratorType
	{
		auto brace = std::find(begin, end, '}');
		auto pipe = std::find(begin, end, '|');

		return pipe < brace ? pipe : brace;
	}

	template<typename IteratorType>
	auto FindSpecificationEnd(IteratorType begin, IteratorType end) -> IteratorType
	{
		auto cursor = begin;
		auto braces = 0;

		while (cursor < end)
		{
			if (*cursor == '{') ++braces;
			else if (*cursor == '}') --braces;

			if (braces < 0) break;
			else cursor++;
		}

		return cursor;
	}

	template<typename IteratorType>
	auto ParseInt(IteratorType begin, IteratorType end) -> int
	{
		auto value = 0;

		for (auto cursor = begin; cursor < end; cursor++)
		{
			value *= 10;
			value += *cursor - '0';
		}

		return value;
	}

	template<typename IteratorType>
	void ParseId(FormatToken& token, IteratorType begin, IteratorType end, int& nextIndex)
	{
		if (begin >= end)
			token.ParameterIndex = nextIndex++;
		else if (*begin == '-')
			token.ParameterIndex = FormatToken::NoParameter;
		else if (*begin >= '0' && *begin <= '9')
			token.ParameterIndex = ParseInt(begin, end);
		else
			token.ParameterIndex = FormatToken::NamedParameter;

		token.ParameterName = GetView(begin, end);
	}

	template<typename TokenType, typename IteratorType>
	void ParseSpecification(TokenType& token, IteratorType begin, IteratorType end)
	{
		token.Specification = GetView(begin, end);
	}

	template<typename FormatType, typename ViewType>
	auto ParseFormat(ViewType format) -> FormatType
	{
		FormatType tokens;

		auto nextIndex = 0;
		auto cursor = format.begin();
		auto end = format.end();

		while (cursor != end)
		{
			auto open = FindLiteralEnd(cursor, end);

			if (cursor != open)
			{
				auto& token = tokens.Tokens.Increment();
				token.ParameterIndex = FormatToken::NoParameter;
				token.Specification = GetView(cursor, open);

				cursor = open;
			}

			if (IsLiteralBrace(open, end))
			{
				auto& token = tokens.Tokens.Increment();
				token.ParameterIndex = FormatToken::NoParameter;
				token.Specification = GetView(cursor, cursor + 1);
				cursor += 2;
			}
			else if (open != end)
			{
				cursor = open + 1;

				auto id = FindIdEnd(cursor, end);
				auto specification = id == end || *id == '}' ? id : FindSpecificationEnd(id + 1, end);

				auto& token = tokens.Tokens.Increment();

				ParseId(token, cursor, id, nextIndex);

				if (*id == '|')
					ParseSpecification(token, id + 1, specification);

				cursor = specification + 1;
			}
		}

		return tokens;
	}
}

auto Pargon::ParseFormatString(StringView format) -> StringFormat
{
	return ParseFormat<StringFormat>(format);
}

auto Pargon::ParseFormatText(TextView format) -> TextFormat
{
	return ParseFormat<TextFormat>(format);
}
