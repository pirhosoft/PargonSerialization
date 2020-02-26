#include "Pargon/Containers/Blueprint.h"
#include "Pargon/Containers/Buffer.h"
#include "Pargon/Serialization/BlueprintWriter.h"
#include "Pargon/Serialization/StringWriter.h"

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>

using namespace Pargon;

namespace
{
	template<typename T>
	void WriteInteger(StringWriter& writer, T number, const char* format)
	{
		auto length = snprintf(nullptr, 0, format, number);
		auto output = std::make_unique<char[]>(length + 1);

		snprintf(output.get(), length + 1, format, number);

		writer.Write(StringView{ output.get(), length }, {});
	}

	template<typename T>
	void WriteRational(StringWriter& writer, T number, const char* format)
	{
		auto length = snprintf(nullptr, 0, format, number);
		auto output = std::make_unique<char[]>(length + 1);
		auto retreat = 0;

		snprintf(output.get(), length + 1, format, number);

		while (output[length - retreat - 1] == '0' && output[length - retreat - 2] != '.')
			retreat++;

		writer.Write(StringView{ output.get(), length - retreat }, {});
	}
}

void StringWriter::Write_(char character, StringView format)
{
	// formats
	// 'n' -> write as number
	// '#' -> write as a hex number
	// other -> write as a character (default)

	if (format.Length() > 0 && format.Character(0) == 'n')
		Write_(static_cast<int>(character), {});
	if (format.Length() > 0 && format.Character(0) == '#')
		Write_(static_cast<int>(character), format);
	else
		_string.Append(character);
}

void StringWriter::Write_(wchar_t character, StringView format)
{
	// formats
	// 'n' -> write as number (default)
	// '#' -> write as a hex number

	Write_(static_cast<int>(character), format);
}

void StringWriter::Write_(char16_t character, StringView format)
{
	// formats
	// 'n' -> write as number (default)
	// '#' -> write as a hex number

	Write_(static_cast<std::uint_least16_t>(character), format);
}

void StringWriter::Write_(char32_t character, StringView format)
{
	// formats
	// 'n' -> write as number (default)
	// '#' -> write as a hex number

	Write_(static_cast<std::uint_least32_t>(character), format);
}

void StringWriter::Write_(bool boolean, StringView format)
{
	// formats
	// "t" -> "true" or "false" (default)
	// "T" -> "True" or "False"
	// "TT" -> "TRUE" or "FALSE"

	if (Equals(format, "TT"))
		WriteString(boolean ? "TRUE"_sv : "FALSE"_sv, {});
	else if (format.Length() > 0 && format.Character(0) == 'T')
		WriteString(boolean ? "True"_sv : "False"_sv, {});
	else
		WriteString(boolean ? "true"_sv : "false"_sv, {});
}

void StringWriter::Write_(signed char number, StringView format)
{
	// formats
	// other -> write as number (default)

	WriteInteger(*this, number, "%hhi");
}

void StringWriter::Write_(short number, StringView format)
{
	// formats
	// other -> write as number (default)

	WriteInteger(*this, number, "%hi");
}

void StringWriter::Write_(int number, StringView format)
{
	// formats
	// other -> write as number (default)

	WriteInteger(*this, number, "%i");
}

void StringWriter::Write_(long number, StringView format)
{
	// formats
	// other -> write as number (default)

	WriteInteger(*this, number, "%li");
}

void StringWriter::Write_(long long number, StringView format)
{
	// formats
	// other -> write as number (default)

	WriteInteger(*this, number, "%lli");
}

void StringWriter::Write_(unsigned char number, StringView format)
{
	// formats
	// '#' -> write as a hex number
	// other -> write as number (default)

	auto hex = format.Length() > 0 && format.Character(0) == '#';
	WriteInteger(*this, number, hex ? "%hhX" : "%hhu");
}

void StringWriter::Write_(unsigned short number, StringView format)
{
	// formats
	// '#' -> write as a hex number
	// other -> write as number (default)

	auto hex = format.Length() > 0 && format.Character(0) == '#';
	WriteInteger(*this, number, hex ? "%hX" : "%hu");
}

void StringWriter::Write_(unsigned int number, StringView format)
{
	// formats
	// '#' -> write as a hex number
	// other -> write as number (default)

	auto hex = format.Length() > 0 && format.Character(0) == '#';
	WriteInteger(*this, number, hex ? "%X" : "%u");
}

void StringWriter::Write_(unsigned long number, StringView format)
{
	// formats
	// '#' -> write as a hex number
	// other -> write as number (default)

	auto hex = format.Length() > 0 && format.Character(0) == '#';
	WriteInteger(*this, number, hex ? "%lX" : "%lu");
}

void StringWriter::Write_(unsigned long long number, StringView format)
{
	// formats
	// '#' -> write as a hex number
	// other -> write as number (default)

	auto hex = format.Length() > 0 && format.Character(0) == '#';
	WriteInteger(*this, number, hex ? "%llX" : "%llu");
}

void StringWriter::Write_(float number, StringView format)
{
	// formats
	// other -> write as number (default)

	WriteRational(*this, static_cast<double>(number), "%f");
}

void StringWriter::Write_(double number, StringView format)
{
	// formats
	// other -> write as number (default)

	WriteRational(*this, number, "%f");
}

void StringWriter::Write_(long double number, StringView format)
{
	// formats
	// other -> write as number (default)

	WriteRational(*this, number, "%Lf");
}

namespace
{
	void WritePonValue(StringWriter& writer, const Blueprint& blueprint, int tabs, bool writeEqual, bool last)
	{
		if (writeEqual && !blueprint.IsArray() && !blueprint.IsObject())
			writer.Write(tabs ? "= "_sv : "="_sv, {});

		if (blueprint.IsInvalid())
		{
			writer.Write("invalid"_sv, {});
		}
		else if (blueprint.IsNull())
		{
			writer.Write("null"_sv, {});
		}
		else if (blueprint.IsBoolean())
		{
			writer.Write(blueprint.AsBoolean(), {});
		}
		else if (blueprint.IsInteger())
		{
			writer.Write(blueprint.AsInteger(), {});
		}
		else if (blueprint.IsFloatingPoint())
		{
			writer.Write(blueprint.AsFloatingPoint(), {});
		}
		else if (blueprint.IsString())
		{
			auto escaped = Escaped(blueprint.AsStringView());
			writer.Format("\"{}\"", escaped);
		}
		else if (blueprint.IsArray())
		{
			auto array = blueprint.AsArray();

			writer.Write('[', {});
			if (tabs && !array->Children.IsEmpty()) writer.Write('\n', {});

			auto count = 0;
			for (auto& child : array->Children)
			{
				count++;
				for (auto i = 0; i < tabs; i++)
					writer.Write('\t', {});

				WritePonValue(writer, child, tabs ? tabs + 1 : 0, false, count == array->Children.Count());

				if (tabs)
					writer.Write('\n', {});
			}

			if (tabs && !array->Children.IsEmpty())
			{
				for (auto i = 0; i < (tabs - 1); i++)
					writer.Write('\t', {});
			}

			writer.Write(']', {});
		}
		else if (blueprint.IsObject())
		{
			auto object = blueprint.AsObject();

			writer.Write('{', {});
			if (tabs && !object->Children.IsEmpty()) writer.Write('\n', {});

			auto count = 0;
			for (auto i = 0; i < object->Children.Count(); i++)
			{
				auto& key = object->Children.GetKey(i);
				auto& item = object->Children.ItemAtIndex(i);

				count++;
				for (auto i = 0; i < tabs; i++)
					writer.Write('\t', {});

				if (!key.IsEmpty())
				{
					writer.Write(key, {});
					if (tabs) writer.Write(' ', {});
				}

				WritePonValue(writer, item, tabs ? tabs + 1 : 0, true, count == object->Children.Count());

				if (tabs)
					writer.Write('\n', {});
			}

			if (tabs && !object->Children.IsEmpty())
			{
				for (auto i = 0; i < (tabs - 1); i++)
					writer.Write('\t', {});
			}

			writer.Write('}', {});
		}

		if (!tabs && !last)
			writer.Write(' ', {});
	}

	struct WriteStream
	{
		using Ch = char;

		auto PutBegin() -> Ch* { return nullptr; }
		auto PutEnd(Ch* begin) -> size_t { return 0; }
		void Put(Ch c) { Writer.Write(c, {}); }
		void Flush() {}

		StringWriter& Writer;
	};

	void WriteJsonArray(const Blueprint& blueprint, rapidjson::Value& value, rapidjson::MemoryPoolAllocator<>& allocator);

	void WriteJsonObject(const Blueprint& blueprint, rapidjson::Value& value, rapidjson::MemoryPoolAllocator<>& allocator)
	{
		auto object = blueprint.AsObject();

		for (auto i = 0; i < object->Children.Count(); i++)
		{
			auto& key = object->Children.GetKey(i);
			auto& child = object->Children.ItemAtIndex(i);
			auto name = rapidjson::GenericStringRef<char>(key.begin());

			if (child.IsInvalid())
			{
			}
			else if (child.IsNull())
			{
				value.AddMember(name, rapidjson::Value(rapidjson::Type::kNullType), allocator);
			}
			else if (child.IsBoolean())
			{
				value.AddMember(name, child.AsBoolean(), allocator);
			}
			else if (child.IsInteger())
			{
				value.AddMember(name, static_cast<int64_t>(child.AsInteger()), allocator);
			}
			else if (child.IsFloatingPoint())
			{
				value.AddMember(name, child.AsFloatingPoint(), allocator);
			}
			else if (child.IsString())
			{
				auto string = rapidjson::GenericStringRef<char>(child.AsStringView().begin());
				value.AddMember(name, string, allocator);
			}
			else if (child.IsArray())
			{
				auto member = rapidjson::Value(rapidjson::Type::kArrayType);

				WriteJsonArray(child, member, allocator);
				value.AddMember(name, std::move(member), allocator);
			}
			else if (child.IsObject())
			{
				auto member = rapidjson::Value(rapidjson::Type::kObjectType);

				WriteJsonObject(child, member, allocator);
				value.AddMember(name, std::move(member), allocator);
			}
		}
	}

	void WriteJsonArray(const Blueprint& blueprint, rapidjson::Value& value, rapidjson::MemoryPoolAllocator<>& allocator)
	{
		auto array = blueprint.AsArray();

		for (auto& child : array->Children)
		{
			if (child.IsInvalid())
			{
			}
			else if (child.IsNull())
			{
				value.PushBack(rapidjson::Value(rapidjson::Type::kNullType), allocator);
			}
			else if (child.IsBoolean())
			{
				value.PushBack(child.AsBoolean(), allocator);
			}
			else if (child.IsInteger())
			{
				value.PushBack(static_cast<int64_t>(child.AsInteger()), allocator);
			}
			else if (child.IsFloatingPoint())
			{
				value.PushBack(child.AsFloatingPoint(), allocator);
			}
			else if (child.IsString())
			{
				auto string = rapidjson::GenericStringRef<char>(child.AsStringView().begin());
				value.PushBack(string, allocator);
			}
			else if (child.IsArray())
			{
				auto member = rapidjson::Value(rapidjson::Type::kArrayType);
				WriteJsonArray(child, member, allocator);
				value.PushBack(std::move(member), allocator);
			}
			else if (child.IsObject())
			{
				auto member = rapidjson::Value(rapidjson::Type::kObjectType);
				WriteJsonObject(child, member, allocator);
				value.PushBack(std::move(member), allocator);
			}
		}
	}

	void WriteJsonValue(StringWriter& writer, const Blueprint& blueprint, bool prettyPrint)
	{
		if (blueprint.IsInvalid())
		{
			writer.Write("invalid"_sv, {});
		}
		else if (blueprint.IsNull())
		{
			writer.Write("null"_sv, {});
		}
		else if (blueprint.IsBoolean())
		{
			writer.Write(blueprint.AsBoolean(), {});
		}
		else if (blueprint.IsInteger())
		{
			writer.Write(static_cast<int64_t>(blueprint.AsInteger()), {});
		}
		else if (blueprint.IsFloatingPoint())
		{
			writer.Write(blueprint.AsFloatingPoint(), {});
		}
		else if (blueprint.IsString())
		{
			writer.Write(blueprint.AsStringView(), {});
		}
		else
		{
			rapidjson::Document document;

			if (blueprint.IsObject())
			{
				auto& value = document.SetObject();
				WriteJsonObject(blueprint, value, document.GetAllocator());
			}
			else if (blueprint.IsArray())
			{
				auto& value = document.SetArray();
				WriteJsonArray(blueprint, value, document.GetAllocator());
			}

			auto stream = WriteStream{ writer };

			if (prettyPrint)
			{
				rapidjson::PrettyWriter<WriteStream> output(stream);
				output.SetIndent('\t', 1);
				document.Accept(output);
			}
			else
			{
				rapidjson::Writer<WriteStream> output(stream);
				document.Accept(output);
			}
		}
	}
}

void StringWriter::Write_(const Blueprint& blueprint, StringView format)
{
	// formats
	// "pon" -> pretty printed pon (default)
	// "PON" -> compressed pon
	// "json" -> pretty printed json
	// "JSON" -> compressed json

	if (Equals(format, "JSON"))
		WriteJsonValue(*this, blueprint, false);
	else if (Equals(format, "json"))
		WriteJsonValue(*this, blueprint, true);
	if (Equals(format, "PON"))
		WritePonValue(*this, blueprint, 0, false, true);
	else
		WritePonValue(*this, blueprint, 1, false, true);
}

void StringWriter::WriteString(StringView string, StringView format)
{
	_string.Append(string);
}

void StringWriter::WriteText(TextView text, StringView format)
{
	_string.Append(text.GetString());
}

void StringWriter::WriteBuffer(BufferView buffer, StringView format)
{
	auto base64 = ToBase64(buffer);
	WriteString(base64, {});
}