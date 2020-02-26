#include "Pargon/Containers/Buffer.h"
#include "Pargon/Containers/String.h"
#include "Pargon/Serialization/BlueprintWriter.h"

using namespace Pargon;


void BlueprintWriter::Write_(char character)
{
	Write_(static_cast<short>(character));
}

void BlueprintWriter::Write_(wchar_t character)
{
	Write_(static_cast<long>(character));
}

void BlueprintWriter::Write_(char16_t character)
{
	Write_(static_cast<uint16_t>(character));
}

void BlueprintWriter::Write_(char32_t character)
{
	Write_(static_cast<uint32_t>(character));
}

void BlueprintWriter::Write_(bool boolean)
{
	_current->SetToBoolean(boolean);
}

void BlueprintWriter::Write_(signed char number)
{
	_current->SetToInteger(number);
}

void BlueprintWriter::Write_(short number)
{
	_current->SetToInteger(number);
}

void BlueprintWriter::Write_(int number)
{
	_current->SetToInteger(number);
}

void BlueprintWriter::Write_(long number)
{
	_current->SetToInteger(number);
}

void BlueprintWriter::Write_(long long number)
{
	_current->SetToInteger(number);
}

void BlueprintWriter::Write_(unsigned char number)
{
	_current->SetToInteger(number);
}

void BlueprintWriter::Write_(unsigned short number)
{
	_current->SetToInteger(number);
}

void BlueprintWriter::Write_(unsigned int number)
{
	_current->SetToInteger(number);
}

void BlueprintWriter::Write_(unsigned long number)
{
	_current->SetToInteger(number);
}

void BlueprintWriter::Write_(unsigned long long number)
{
	_current->SetToInteger(number);
}

void BlueprintWriter::Write_(float number)
{
	_current->SetToFloatingPoint(number);
}

void BlueprintWriter::Write_(double number)
{
	_current->SetToFloatingPoint(number);
}

void BlueprintWriter::Write_(long double number)
{
	_current->SetToFloatingPoint(static_cast<Blueprint::FloatingPoint>(number));
}

void BlueprintWriter::Write_(const Blueprint& blueprint)
{
	*_current = blueprint;
}

void BlueprintWriter::WriteString(StringView string)
{
	_current->SetToString(string);
}

void BlueprintWriter::MoveDown(int index)
{
	if (!_current->IsArray())
		_current->SetToArray();

	auto array = _current->AsArray();
	array->Children.EnsureCount(index + 1, {});
	auto& child = array->Children.Item(index);

	_tree.Add({ _current, _currentIndex });
	_current = std::addressof(child);
}

void BlueprintWriter::MoveDown(StringView name)
{
	if (!_current->IsObject())
		_current->SetToObject();

	auto object = _current->AsObject();
	auto& child = object->Children.AddOrGet(name, {});

	_tree.Add({ _current, _currentIndex });
	_current = std::addressof(child);
}

auto BlueprintWriter::MoveUp() -> bool
{
	if (_tree.IsEmpty())
		return false;

	auto& parent = _tree.Last();

	_current = parent.Node;
	_currentIndex = parent.Index;

	_tree.RemoveLast();

	return true;
}

auto BlueprintWriter::MoveNext() -> bool
{
	if (_tree.IsEmpty())
		return false;

	auto parent = _tree.Last();
	auto object = parent.Node->AsObject();
	auto array = parent.Node->AsArray();

	if (object != nullptr)
	{
		if (_currentIndex >= object->Children.LastIndex())
			return false;

		_currentIndex++;
		_current = std::addressof(object->Children.ItemAtIndex(_currentIndex));
	}
	else if (array != nullptr)
	{
		if (_currentIndex >= array->Children.LastIndex())
			return false;

		_currentIndex++;
		_current = std::addressof(array->Children.Item(_currentIndex));
	}
	else
	{
		return false;
	}

	return true;
}
