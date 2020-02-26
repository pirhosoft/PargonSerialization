#include "Pargon/Containers/Blueprint.h"
#include "Pargon/Containers/String.h"
#include "Pargon/Serialization/BufferWriter.h"

#include <algorithm>

using namespace Pargon;

void BufferWriter::Align(size_t size)
{
	auto padding = _buffer.Size() % size;
	if (padding > 0)
		_buffer.Append(0, static_cast<int>(size - padding));
}

void BufferWriter::WriteBytes(BufferView data, bool correctEndian)
{
	_buffer.Append(data, correctEndian && _endian != NativeEndian);
}

namespace
{
	auto GetBit(int index, bool bit) -> unsigned char
	{
		return bit ? 1 << index : 0;
	}
}

void BufferWriter::WriteBit(bool bit)
{
	if (_bitIndex == 7)
		Write_('\0');

	auto index = _buffer.Size() - 1;
	auto byteValue = _buffer.Byte(index);
	auto bitValue = GetBit(_bitIndex, bit);

	_buffer.SetByte(_buffer.Size() - 1, byteValue | bitValue);

	if (_bitIndex == 0)
		_bitIndex = 7;
	else
		_bitIndex--;
}

void BufferWriter::WriteBits(int count, long long bits)
{
	for (auto i = count; i > 0; i--)
	{
		auto bit = GetBit(i - 1, true);
		WriteBit((bits & bit) != 0);
	}
}

void BufferWriter::WriteSignedBits(int count, long long bits)
{
	WriteBit(bits < 0);
	WriteBits(count - 1, std::abs(bits));
}

void BufferWriter::Realign(bool bit)
{
	while (_bitIndex != 7)
		WriteBit(bit);
}

namespace
{
	template<typename T>
	void WriteRaw(BufferWriter& writer, const T& value)
	{
		auto normalized = static_cast<SerializationTraits::NormalizedType<T>>(value);
		writer.WriteBytes({ reinterpret_cast<const uint8_t*>(std::addressof(value)), sizeof(value) }, true);
	}
}

void BufferWriter::Write_(char character)
{
	WriteRaw(*this, character);
}

void BufferWriter::Write_(wchar_t character)
{
	WriteRaw(*this, character);
}

void BufferWriter::Write_(char16_t character)
{
	WriteRaw(*this, character);
}

void BufferWriter::Write_(char32_t character)
{
	WriteRaw(*this, character);
}

void BufferWriter::Write_(bool boolean)
{
	WriteRaw(*this, boolean);
}

void BufferWriter::Write_(signed char number)
{
	WriteRaw(*this, number);
}

void BufferWriter::Write_(short number)
{
	WriteRaw(*this, number);
}

void BufferWriter::Write_(int number)
{
	WriteRaw(*this, number);
}

void BufferWriter::Write_(long number)
{
	WriteRaw(*this, number);
}

void BufferWriter::Write_(long long number)
{
	WriteRaw(*this, number);
}

void BufferWriter::Write_(unsigned char number)
{
	WriteRaw(*this, number);
}

void BufferWriter::Write_(unsigned short number)
{
	WriteRaw(*this, number);
}

void BufferWriter::Write_(unsigned int number)
{
	WriteRaw(*this, number);
}

void BufferWriter::Write_(unsigned long number)
{
	WriteRaw(*this, number);
}

void BufferWriter::Write_(unsigned long long number)
{
	WriteRaw(*this, number);
}

void BufferWriter::Write_(float number)
{
	WriteRaw(*this, number);
}

void BufferWriter::Write_(double number)
{
	WriteRaw(*this, number);
}

void BufferWriter::Write_(long double number)
{
	WriteRaw(*this, number);
}

void BufferWriter::Write_(const Blueprint& blueprint)
{
	auto type = 0;

	if (blueprint.IsInvalid()) type = 0;
	if (blueprint.IsNull()) type = 1;
	if (blueprint.IsBoolean()) type = 2;
	if (blueprint.IsInteger()) type = 3;
	if (blueprint.IsFloatingPoint()) type = 4;
	if (blueprint.IsString()) type = 5;
	if (blueprint.IsArray()) type = 6;
	if (blueprint.IsObject()) type = 7;

	Write_(type);

	switch (type)
	{
		case 0: break;
		case 1: break;
		case 2:
		{
			Write_(blueprint.AsBoolean());
			break;
		}
		case 3:
		{
			Write_(blueprint.AsInteger());
			break;
		}
		case 4:
		{
			Write_(blueprint.AsFloatingPoint());
			break;
		}
		case 5:
		{
			WriteString(blueprint.AsStringView());
			break;
		}
		case 6:
		{
			auto array = blueprint.AsArray();
			WriteSequence<Blueprint>(array->Children);
			break;
		}
		case 7:
		{
			auto object = blueprint.AsObject();
			Write_(object->Children);
			break;
		}
	}
}

void BufferWriter::WriteBuffer(BufferView buffer)
{
	Write_(buffer.Size());
	WriteBytes(buffer, false);
}

void BufferWriter::WriteString(StringView string)
{
	Write_(string.Length());
	WriteBytes(string, false);
}

void BufferWriter::WriteText(TextView text)
{
	auto string = text.GetString();
	WriteString(string);
}
