#include "Pargon/Containers/Blueprint.h"
#include "Pargon/Containers/String.h"
#include "Pargon/Serialization/BufferReader.h"
#include "Pargon/Serialization/BufferWriter.h"

#include <algorithm>

using namespace Pargon;

BufferReader::BufferReader(BufferView view) :
	_data(view.begin()),
	_length(view.Size())
{
}

void BufferReader::ReportError(StringView message)
{
	_hasFailed = true;
	_errors.Add({ _index, message });
}

auto BufferReader::Advance(int count) -> bool
{
	return MoveTo(_index + count);
}

auto BufferReader::Retreat(int count) -> bool
{
	return MoveTo(_index - count);
}

auto BufferReader::MoveTo(int index) -> bool
{
	if (_hasFailed)
		return false;

	if (index < 0 || index > _length)
	{
		ReportError("attempted to move past the end of the buffer");
		return false;
	}

	_bitIndex = 0;
	_index = index;
	return true;
}

auto BufferReader::ViewByte() -> uint8_t
{
	if (_hasFailed)
		return 0;

	if (_index == _length)
	{
		ReportError("attempted to view past the end of the buffer");
		return 0;
	}

	return _data[_index];
}

auto BufferReader::ViewBytes(int count) -> BufferView
{
	if (_hasFailed)
		return {};

	if (count < 0 || _index + count > _length)
	{
		ReportError("attempted to view past the end of the buffer");
		return {};
	}

	return { _data + _index, count };
}

auto BufferReader::ReadByte() -> uint8_t
{
	if (_hasFailed)
		return 0;

	if (_index == _length)
	{
		ReportError("attempted to read past the end of the buffer");
		return 0;
	}

	return _data[_index++];
}

auto BufferReader::ReadBytes(int count) -> BufferView
{
	if (_hasFailed)
		return {};

	if (count < 0 || _index + count > _length)
	{
		ReportError("attempted to read past the end of the buffer");
		return {};
	}

	_index += count;
	return { _data + _index - count, count };
}

auto BufferReader::CopyBytes(BufferReference into, bool correctEndian) -> bool
{
	if (_hasFailed)
		return false;

	auto data = static_cast<uint8_t*>(into.begin());
	auto size = into.Size();

	if ((_index + size) > _length)
	{
		ReportError("attempted to read past the end of the buffer");
		return false;
	}

	if (correctEndian && _endian != NativeEndian)
		std::reverse_copy(_data + _index, _data + _index + size, data);
	else
		std::copy(_data + _index, _data + _index + size, data);

	_index += size;
	return true;
}

namespace
{
	auto Mask(int start, int end)
	{
		auto count = end - start;
		auto mask = 255 >> (8 - count);
		return mask << (8 - end);
	}

	auto ExtractBits(unsigned char byte, int start, int end)
	{
		return (byte & Mask(start, end)) >> (8 - end);
	}
}

auto BufferReader::ReadBit() -> bool
{
	if (_hasFailed)
		return false;

	return ReadBits(1) != 0;
}

auto BufferReader::ReadBits(int count) -> long long
{
	if (_hasFailed)
		return 0;

	if (count < 0 || count > 64)
	{
		ReportError("a maximum of 64 bits can be read at once");
		return 0;
	}

	auto required = ((_bitIndex + count - 1) / 8) + 1;
	if (Remaining() < required)
	{
		ReportError("attempted to read past the end of the buffer");
		return 0;
	}

	auto byte = _data[_index];

	auto end = _bitIndex + count;
	auto result = 0ull;

	while (end >= 8)
	{
		result <<= 8 - _bitIndex;
		result |= ExtractBits(byte, _bitIndex, 8);
		end -= 8;

		_bitIndex = 0;
		byte = _data[++_index];
	}

	if (end > 0)
	{
		result <<= end;
		result |= ExtractBits(byte, _bitIndex, end);
		_bitIndex = end;
	}

	return result;
}

auto BufferReader::ReadSignedBits(int count) -> long long
{
	if (_hasFailed)
		return 0;

	if (count < 0 || count > 64)
	{
		ReportError("a maximum of 64 bits can be read at once");
		return 0;
	}

	union
	{
		long long Signed;
		unsigned long long Unsigned;
	} mask;

	auto negative = ReadBit();
	auto value = ReadBits(count - 1);

	if (negative)
	{
		mask.Unsigned = std::numeric_limits<unsigned long long>::max();
		mask.Unsigned <<= (count - 1);
		mask.Unsigned |= value;

		return mask.Signed;
	}

	return static_cast<long long>(value);
}

void BufferReader::Realign()
{
	if (!_hasFailed && _bitIndex > 0)
	{
		_bitIndex = 0;
		_index++;
	}
}

namespace
{
	template<typename T>
	auto ReadInto(BufferReader& reader, T& item) -> bool
	{
		auto value = static_cast<SerializationTraits::NormalizedType<T>>(item);
		if (reader.CopyBytes({ reinterpret_cast<uint8_t*>(std::addressof(value)), sizeof(value) }, true))
		{
			item = static_cast<T>(value);
			return true;
		}

		return false;
	}
}

auto BufferReader::Read_(bool& boolean) -> bool
{
	return ReadInto(*this, boolean);
}

auto BufferReader::Read_(char& character) -> bool
{
	return ReadInto(*this, character);
}

auto BufferReader::Read_(wchar_t& character) -> bool
{
	return ReadInto(*this, character);
}

auto BufferReader::Read_(char16_t& character) -> bool
{
	return ReadInto(*this, character);
}

auto BufferReader::Read_(char32_t& character) -> bool
{
	return ReadInto(*this, character);
}

auto BufferReader::Read_(signed char& number) -> bool
{
	return ReadInto(*this, number);
}

auto BufferReader::Read_(short& number) -> bool
{
	return ReadInto(*this, number);
}

auto BufferReader::Read_(int& number) -> bool
{
	return ReadInto(*this, number);
}

auto BufferReader::Read_(long& number) -> bool
{
	return ReadInto(*this, number);
}

auto BufferReader::Read_(long long& number) -> bool
{
	return ReadInto(*this, number);
}

auto BufferReader::Read_(unsigned char& number) -> bool
{
	return ReadInto(*this, number);
}

auto BufferReader::Read_(unsigned short& number) -> bool
{
	return ReadInto(*this, number);
}

auto BufferReader::Read_(unsigned int& number) -> bool
{
	return ReadInto(*this, number);
}

auto BufferReader::Read_(unsigned long& number) -> bool
{
	return ReadInto(*this, number);
}

auto BufferReader::Read_(unsigned long long& number) -> bool
{
	return ReadInto(*this, number);
}

auto BufferReader::Read_(float& number) -> bool
{
	return ReadInto(*this, number);
}

auto BufferReader::Read_(double& number) -> bool
{
	return ReadInto(*this, number);
}

auto BufferReader::Read_(long double& number) -> bool
{
	return ReadInto(*this, number);
}

auto BufferReader::Read_(Buffer& buffer) -> bool
{
	int size;
	if (!Read_(size))
		return false;

	return CopyBytes(buffer.GetReference(size), false);
}

auto BufferReader::Read_(BufferView& buffer) -> bool
{
	int size;
	if (!Read_(size))
		return false;

	buffer = ReadBytes(size);
	return !_hasFailed;
}

auto BufferReader::Read_(String& string) -> bool
{
	int length;
	if (!Read_(length))
		return false;

	string = ReadBytes(length);
	return !_hasFailed;
}

auto BufferReader::Read_(StringView& string) -> bool
{
	int length;
	if (!Read_(length))
		return false;

	string = ReadBytes(length);
	return !_hasFailed;
}

auto BufferReader::Read_(Text& text) -> bool
{
	int length;
	if (!Read_(length))
		return false;

	auto utf8 = ReadBytes(length);

	if (!_hasFailed)
	{
		text = Text(utf8, Encoding::Utf8);
		return true;
	}

	return false;
}

auto BufferReader::Read_(Blueprint& blueprint) -> bool
{
	int type;
	if (!Read_(type))
		return false;

	switch (type)
	{
	case 0:
	{
		blueprint.SetToInvalid();
		return true;
	}
	case 1:
	{
		blueprint.SetToNull();
		return true;
	}
	case 2:
	{
		bool value;
		if (!Read_(value))
			return false;

		blueprint.SetToBoolean(value);
		return true;
	}
	case 3:
	{
		long long value;
		if (!Read_(value))
			return false;

		blueprint.SetToInteger(value);
		return true;
	}
	case 4:
	{
		double value;
		if (!Read_(value))
			return false;

		blueprint.SetToFloatingPoint(value);
		return true;
	}
	case 5:
	{
		StringView value;
		if (!Read_(value))
			return false;

		blueprint.SetToString(value);
		return true;
	}
	case 6:
	{
		List<Blueprint> children;
		if (!Read_(children))
			return false;

		blueprint.SetToArray().Children = std::move(children);
		return true;
	}
	case 7:
	{
		Map<String, Blueprint> children;
		if (!Read_(children))
			return false;

		blueprint.SetToObject().Children = std::move(children);
		return true;
	}
	}

	ReportError("invalid blueprint type");
	return false;
}