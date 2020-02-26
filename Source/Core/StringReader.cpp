#include "Pargon/Containers/Blueprint.h"
#include "Pargon/Containers/Buffer.h"
#include "Pargon/Containers/String.h"
#include "Pargon/Serialization/StringReader.h"
#include "Pargon/Serialization/BlueprintReader.h"

#include <rapidjson/document.h>
#include <regex>

using namespace Pargon;

StringReader::StringReader(StringView text) :
	_data(text.begin()),
	_length(text.Length()),
	_index(0)
{
}

void StringReader::ReportError(StringView message)
{
	auto line = 1;
	auto column = 1;

	for (auto i = 0; i < _index; ++i)
	{
		++column;

		if (_data[i] == '\n')
		{
			++line;
			column = 1;
		}
	}

	_hasFailed = true;
	_errors.Add({ line, column, message });
}

auto StringReader::ViewTo(int count) const -> StringView
{
	if (_index + count > _length)
		return {};

	return { _data + _index, static_cast<int>(count) };
}

auto StringReader::ViewNext() const -> char
{
	return AtEnd() ? '\0' : _data[_index];
}

auto StringReader::ViewNext(int index) const -> char
{
	if (_index + index >= _length)
		return '\0';

	return _data[_index + index];
}

auto StringReader::ViewRemaining() const -> StringView
{
	return { _data + _index, static_cast<int>(_length - _index) };
}

auto StringReader::ViewPrevious(int count) const -> StringView
{
	if (_index < count)
		return {};

	return { _data + _index - count, static_cast<int>(count) };
}

auto StringReader::ReadTo(int count) -> StringView
{
	if (_index + count > _length)
		return {};

	_index += count;
	return { _data + _index - count, static_cast<int>(count) };
}

auto StringReader::ReadNext() -> char
{
	return AtEnd() ? '\0' : _data[_index++];
}

auto StringReader::Advance(int count) -> bool
{
	return MoveTo(_index + count);
}

auto StringReader::Retreat(int count) -> bool
{
	return MoveTo(_index - count);
}

auto StringReader::MoveTo(int index) -> bool
{
	if (index > _length)
		return false;

	_index = index;
	return true;
}

void StringReader::Reset()
{
	_index = 0;
}

auto StringReader::AdvanceTo(StringView string, bool ignoreCase) -> int
{
	auto index = IndexOf(ViewRemaining(), string, ignoreCase);
	if (index != String::InvalidIndex)
		_index += index;

	return index;
}

auto StringReader::AdvanceToWhitespace() -> int
{
	return AdvanceToAny(" \t\r\n", false);
}

auto StringReader::AdvanceToAny(StringView characters, bool ignoreCase) -> int
{
	auto index = IndexOfAny(ViewRemaining(), characters);
	if (index != String::InvalidIndex)
		_index += index;

	return index;
}

auto StringReader::AdvanceToExpression(StringView pattern, bool ignoreCase) -> int
{
	std::cmatch match;
	std::regex::flag_type flags = ignoreCase ? std::regex_constants::icase | std::regex_constants::ECMAScript : std::regex_constants::ECMAScript;
	auto regex = std::regex(pattern.begin(), pattern.end(), flags);

	if (!std::regex_search(_data + _index, _data + _length, match, regex))
		return -1;

	auto position = static_cast<int>(match.position(0));
	_index += position;
	return position;
}

auto StringReader::AdvancePast(StringView string, bool ignoreCase) -> int
{
	auto index = IndexOfOther(ViewRemaining(), string, ignoreCase);
	if (index != String::InvalidIndex)
		_index += index;

	return index;
}

auto StringReader::AdvancePastWhitespace() -> int
{
	return AdvancePastAny(" \t\r\n", false);
}

auto StringReader::AdvancePastAny(StringView characters, bool ignoreCase) -> int
{
	auto index = IndexOfAnyOther(ViewRemaining(), characters, ignoreCase);
	if (index == String::InvalidIndex)
		index = _length - _index;

	_index += index;
	return index;
}

auto StringReader::AdvancePastExpression(StringView pattern, bool ignoreCase) -> int
{
	std::cmatch match;
	std::regex::flag_type flags = ignoreCase ? std::regex_constants::icase | std::regex_constants::ECMAScript : std::regex_constants::ECMAScript;
	auto regex = std::regex(pattern.begin(), pattern.end(), flags);

	if (!std::regex_search(_data + _index, _data + _length, match, regex, std::regex_constants::match_continuous))
		return -1;

	auto length = static_cast<int>(match.length(0));
	_index += length;
	return length;
}

auto StringReader::RetreatTo(StringView string, bool ignoreCase) -> int
{
	auto index = LastIndexOf({ _data, _index }, string, ignoreCase);
	if (index != String::InvalidIndex)
		_index = index;

	return index;
}

auto StringReader::RetreatToWhitespace() -> int
{
	return RetreatToAny(" \t\r\n", false);
}

auto StringReader::RetreatToAny(StringView characters, bool ignoreCase) -> int
{
	auto index = LastIndexOfAny({ _data, _index }, characters);
	if (index != String::InvalidIndex)
		_index = index;

	return index;
}

auto StringReader::RetreatToExpression(StringView pattern, bool ignoreCase) -> int
{
	std::cmatch match;
	std::regex::flag_type flags = ignoreCase ? std::regex_constants::icase | std::regex_constants::ECMAScript : std::regex_constants::ECMAScript;
	auto regex = std::regex(pattern.begin(), pattern.end(), flags);

	if (!std::regex_search(_data, _data + _index, match, regex))
		return -1;

	auto position = static_cast<int>(match.position(0));
	_index = position;
	return position;
}

auto StringReader::RetreatPast(StringView string, bool ignoreCase) -> int
{
	auto index = LastIndexOfOther({ _data, _index }, string, ignoreCase);
	if (index != String::InvalidIndex)
		_index = index;

	return index;
}

auto StringReader::RetreatPastWhitespace() -> int
{
	return RetreatPastAny(" \t\r\n", false);
}

auto StringReader::RetreatPastAny(StringView characters, bool ignoreCase) -> int
{
	auto index = LastIndexOfAnyOther({ _data, _index }, characters, ignoreCase);
	if (index != String::InvalidIndex)
		_index = index;

	return index;
}

auto StringReader::RetreatPastExpression(StringView pattern, bool ignoreCase) -> int
{
	std::cmatch match;
	std::regex::flag_type flags = ignoreCase ? std::regex_constants::icase | std::regex_constants::ECMAScript : std::regex_constants::ECMAScript;
	auto regex = std::regex(pattern.begin(), pattern.end(), flags);

	if (!std::regex_search(_data, _data + _index, match, regex, std::regex_constants::match_continuous))
		return -1;

	auto length = static_cast<int>(match.length(0));
	_index = length;
	return length;
}

namespace
{
	template<typename T>
	auto ReadNumber(StringReader& reader, T& number, const char* format) -> bool
	{
		int count;
		auto success = sscanf(reader.ViewRemaining().begin(), format, &number, &count);
		if (success != 1)
			return false;

		reader.Advance(count);
		return true;
	}
}

void StringReader::Read_(char& character, StringView format)
{
	if (!AtEnd())
		character = ReadNext();
	else
		ReportError("not a char");
}

void StringReader::Read_(wchar_t& character, StringView format)
{
	auto number = static_cast<int>(character);
	Read_(number, format);
	character = static_cast<wchar_t>(number);
}

void StringReader::Read_(char16_t& character, StringView format)
{
	auto number = static_cast<std::uint_least16_t>(character);
	Read_(number, format);
	character = static_cast<char16_t>(number);
}

void StringReader::Read_(char32_t& character, StringView format)
{
	auto number = static_cast<std::uint_least32_t>(character);
	Read_(number, format);
	character = static_cast<char32_t>(number);
}

void StringReader::Read_(bool& boolean, StringView format)
{
	if (StartsWith(ViewRemaining(), "true", true))
		boolean = true;
	else if (StartsWith(ViewRemaining(), "false", true))
		boolean = false;
	else
		ReportError("not a bool");
}

void StringReader::Read_(signed char& number, StringView format)
{
	if (!ReadNumber(*this, number, "%hhi%n"))
		ReportError("not a number");
}

void StringReader::Read_(short& number, StringView format)
{
	if (!ReadNumber(*this, number, "%hi%n"))
		ReportError("not a number");
}

void StringReader::Read_(int& number, StringView format)
{
	if (!ReadNumber(*this, number, "%i%n"))
		ReportError("not a number");
}

void StringReader::Read_(long& number, StringView format)
{
	if (!ReadNumber(*this, number, "%li%n"))
		ReportError("not a number");
}

void StringReader::Read_(long long& number, StringView format)
{
	if (!ReadNumber(*this, number, "%lli%n"))
		ReportError("not a number");
}

void StringReader::Read_(unsigned char& number, StringView format)
{
	if (!ReadNumber(*this, number, "%hhu%n"))
		ReportError("not a number");
}

void StringReader::Read_(unsigned short& number, StringView format)
{
	if (!ReadNumber(*this, number, "%hu%n"))
		ReportError("not a number");
}

void StringReader::Read_(unsigned int& number, StringView format)
{
	if (!ReadNumber(*this, number, "%u%n"))
		ReportError("not a number");
}

void StringReader::Read_(unsigned long& number, StringView format)
{
	if (!ReadNumber(*this, number, "%lu%n"))
		ReportError("not a number");
}

void StringReader::Read_(unsigned long long& number, StringView format)
{
	if (!ReadNumber(*this, number, "%llu%n"))
		ReportError("not a number");
}

void StringReader::Read_(float& number, StringView format)
{
	if (!ReadNumber(*this, number, "%f%n"))
		ReportError("not a number");
}

void StringReader::Read_(double& number, StringView format)
{
	if (!ReadNumber(*this, number, "%lf%n"))
		ReportError("not a number");
}

void StringReader::Read_(long double& number, StringView format)
{
	if (!ReadNumber(*this, number, "%Lf%n"))
		ReportError("not a number");
}

void StringReader::Read_(String& s, StringView format)
{
	StringView view;
	Read_(view, format);

	if (!_hasFailed)
		s = view.GetString();
}

void StringReader::Read_(StringView& s, StringView format)
{
	if (format.IsEmpty())
	{
		s = ViewRemaining();
	}
	else
	{
		auto length = AdvancePastExpression(format, true);
		if (length == String::InvalidIndex)
			ReportError("input doesn't match specification");
		else
			s = ViewPrevious(length);
	}
}

void StringReader::Read_(Text& text, StringView format)
{
	StringView view;
	Read_(view, format);

	if (!_hasFailed)
		text = Text(view.GetString());
}

void StringReader::Read_(Buffer& buffer, StringView format)
{
	auto string = ViewRemaining();
	buffer = FromBase64(string);
}

namespace
{
	auto ReadPonString(StringReader& reader, Blueprint& blueprint) -> bool
	{
		StringView string;

		if (!reader.Parse("\"{|(?:[^\"\\\\]|\\\\.)*}\"", string))
		{
			reader.ReportError("expected a string");
			return false;
		}

		auto escaped = Unescaped(string);
		blueprint.SetToString(escaped);
		return true;
	}

	auto ReadPonBoolNullOrNumber(StringReader& reader, Blueprint& blueprint) -> bool
	{
		auto count = reader.AdvanceToAny("\r\n\t ]}", false);
		auto value = reader.ViewPrevious(count).GetString();

		if (Pargon::Equals(value, "true", true))
		{
			blueprint.SetToBoolean(true);
		}
		else if (Pargon::Equals(value, "false", true))
		{
			blueprint.SetToBoolean(false);
		}
		else if (Pargon::Equals(value, "invalid", true))
		{
			blueprint.SetToInvalid();
		}
		else if (Pargon::Equals(value, "null", true))
		{
			blueprint.SetToNull();
		}
		else if (Pargon::IndexOf(value, ".", false) != String::InvalidIndex)
		{
			double number;
			StringReader numberReader(value);
			numberReader.Read(number, {});

			if (numberReader.HasFailed())
			{
				reader.ReportError("expected a number");
				return false;
			}

			blueprint.SetToFloatingPoint(number);
		}
		else
		{
			long long number;
			StringReader numberReader(value);
			numberReader.Read(number, {});

			if (numberReader.HasFailed())
			{
				reader.ReportError("expected a number");
				return false;
			}

			blueprint.SetToInteger(number);
		}

		return true;
	}

	auto ReadPonObject(StringReader& reader, Blueprint& blueprint) -> bool;

	auto ReadPonArray(StringReader& reader, Blueprint& blueprint) -> bool
	{
		reader.AdvancePastWhitespace();

		auto character = reader.ReadNext();
		if (character != '[')
		{
			reader.Retreat(1);
			reader.ReportError("expected '['"_sv );
			return false;
		}

		auto& array = blueprint.SetToArray();

		while (true)
		{
			reader.AdvancePastWhitespace();

			character = reader.ViewNext();
			if (character == ']')
				break;

			auto& child = array.Children.Increment();

			if (character == '[')
			{
				if (!ReadPonArray(reader, child))
					return false;
			}
			else if (character == '{')
			{
				if (!ReadPonObject(reader, child))
					return false;
			}
			else if (character == '\"')
			{
				if (!ReadPonString(reader, child))
					return false;
			}
			else
			{
				if (!ReadPonBoolNullOrNumber(reader, child))
					return false;
			}
		}

		return reader.Advance(1);
	}

	auto ReadPonObject(StringReader& reader, Blueprint& blueprint) -> bool
	{
		reader.AdvancePastWhitespace();
		char character = reader.ReadNext();
		if (character != '{')
		{
			reader.ReportError("expected '{'");
			return false;
		}

		auto& object = blueprint.SetToObject();

		while (true)
		{
			reader.AdvancePastWhitespace();

			character = reader.ViewNext();
			if (character == '}')
				break;

			auto count = reader.AdvanceToAny(" \t\r\n=:[{", false);
			auto name = reader.ViewPrevious(count);
			auto& child = object.Children.AddOrSet(name, {});

			reader.AdvancePastWhitespace();

			character = reader.ViewNext();
			if (character == '=')
			{
				reader.Advance(1);
				reader.AdvancePastWhitespace();

				character = reader.ViewNext();
				if (character == '\"')
				{
					if (!ReadPonString(reader, child))
						return false;
				}
				else
				{
					if (!ReadPonBoolNullOrNumber(reader, child))
						return false;
				}
			}
			else if (character == '[')
			{
				if (!ReadPonArray(reader, child))
					return false;
			}
			else if (character == '{')
			{
				if (!ReadPonObject(reader, child))
					return false;
			}
			else
			{
				reader.ReportError("expected '}'");
				return false;
			}
		}

		return reader.Advance(1);
	}

	auto ReadPon(StringReader& reader, Blueprint& blueprint) -> bool
	{
		auto success = true;
	
		reader.AdvancePastWhitespace();
	
		if (reader.ViewNext() == '[')
			success = ReadPonArray(reader, blueprint);
		else if (reader.ViewNext() == '{')
			success = ReadPonObject(reader, blueprint);
		else if (reader.ViewNext() == '\"')
			success = ReadPonString(reader, blueprint);
		else
			success = ReadPonBoolNullOrNumber(reader, blueprint);
	
		return success;
	}

	struct ReadStream
	{
		using Ch = char;

		StringReader& Reader;

		auto PutBegin() -> Ch* { return nullptr; }
		auto PutEnd(Ch* begin) -> size_t { return 0; }
		void Put(Ch c) {}
		void Flush() {}
		auto Peek() const -> Ch { return Reader.ViewNext(); }
		auto Take() const -> Ch { return Reader.ReadNext(); }
		auto Tell() -> size_t { return Reader.Index(); }
	};

	void ReadJsonArray(Blueprint& blueprint, const rapidjson::Value& value);

	void ReadJsonObject(Blueprint& blueprint, const rapidjson::Value& value)
	{
		auto& object = blueprint.SetToObject();

		for (auto iterator = value.MemberBegin(); iterator != value.MemberEnd(); iterator++)
		{
			auto name = StringView{ iterator->name.GetString(), static_cast<int>(iterator->name.GetStringLength()) };
			auto& child = object.Children.AddOrGet(name, {});

			if (iterator->value.IsArray())
				ReadJsonArray(child, iterator->value);
			else if (iterator->value.IsObject())
				ReadJsonObject(child, iterator->value);
			else if (iterator->value.IsBool())
				child.SetToBoolean(iterator->value.IsTrue());
			else if (iterator->value.IsDouble())
				child.SetToFloatingPoint(iterator->value.GetDouble());
			else if (iterator->value.IsUint())
				child.SetToInteger(iterator->value.GetUint());
			else if (iterator->value.IsInt())
				child.SetToInteger(iterator->value.GetInt());
			else if (iterator->value.IsNull())
				child.SetToNull();
			else if (iterator->value.IsString())
				child.SetToString({ iterator->value.GetString(), static_cast<int>(iterator->value.GetStringLength()) });
			else
				child.SetToInvalid();
		}
	}

	void ReadJsonArray(Blueprint& blueprint, const rapidjson::Value& value)
	{
		auto& array = blueprint.SetToArray();

		for (auto iterator = value.Begin(); iterator != value.End(); iterator++)
		{
			auto& child = array.Children.Increment();

			if (iterator->IsArray())
				ReadJsonArray(child, *iterator);
			else if (iterator->IsObject())
				ReadJsonObject(child, *iterator);
			else if (iterator->IsBool())
				child.SetToBoolean(iterator->IsTrue());
			else if (iterator->IsDouble())
				child.SetToFloatingPoint(iterator->GetDouble());
			else if (iterator->IsUint())
				child.SetToInteger(iterator->GetUint());
			else if (iterator->IsInt())
				child.SetToInteger(iterator->GetInt());
			else if (iterator->IsNull())
				child.SetToNull();
			else if (iterator->IsString())
				child.SetToString({ iterator->GetString(), static_cast<int>(iterator->GetStringLength()) });
			else
				child.SetToInvalid();
		}
	}

	auto ReadJson(StringReader& reader, Blueprint& blueprint) -> bool
	{
		rapidjson::Document document;
		auto stream = ReadStream{ reader };
		document.ParseStream(stream);

		if (document.IsObject())
			ReadJsonObject(blueprint, document);
		else if (document.IsArray())
			ReadJsonArray(blueprint, document);
		else
			return false;

		return true;
	}
}

void StringReader::Read_(Blueprint& blueprint, StringView format)
{
	auto success = true;
	auto location = Index();

	if (Equals(format, "json", true))
		success = ReadJson(*this, blueprint);
	else
		success = ReadPon(*this, blueprint);

	if (!success)
		MoveTo(location);
}
