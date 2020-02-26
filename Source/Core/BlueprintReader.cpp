#include "Pargon/Containers/Buffer.h"
#include "Pargon/Containers/Blueprint.h"
#include "Pargon/Containers/String.h"
#include "Pargon/Serialization/BlueprintReader.h"

using namespace Pargon;

BlueprintReader::BlueprintReader(const Blueprint& blueprint) :
	_current(std::addressof(blueprint)),
	_currentIndex(0u)
{
}

void BlueprintReader::ReportError(StringView message)
{
	_hasFailed = true;
	_errors.Add({ ""_sv, message });
}

auto BlueprintReader::Read_(char& character) -> bool
{
	short number;
	if (!Read_(number))
		return false;

	character = static_cast<char>(number);
	return true;
}

auto BlueprintReader::Read_(wchar_t& character) -> bool
{
	short number;
	if (!Read_(number))
		return false;

	character = static_cast<wchar_t>(number);
	return true;
}

auto BlueprintReader::Read_(char16_t& character) -> bool
{
	short number;
	if (!Read_(number))
		return false;

	character = static_cast<uint16_t>(number);
	return true;
}

auto BlueprintReader::Read_(char32_t& character) -> bool
{
	short number;
	if (!Read_(number))
		return false;

	character = static_cast<uint32_t>(number);
	return true;
}

auto BlueprintReader::Read_(bool& boolean) -> bool
{
	if (_current->IsBoolean())
	{
		boolean = _current->AsBoolean();
		return true;
	}

	return false;
}

auto BlueprintReader::Read_(signed char& number) -> bool
{
	if (_current->IsInteger())
	{
		number = static_cast<signed char>(_current->AsInteger());
		return true;
	}

	return false;
}

auto BlueprintReader::Read_(short& number) -> bool
{
	if (_current->IsInteger())
	{
		number = static_cast<short>(_current->AsInteger());
		return true;
	}

	return false;
}

auto BlueprintReader::Read_(int& number) -> bool
{
	if (_current->IsInteger())
	{
		number = static_cast<int>(_current->AsInteger());
		return true;
	}

	return false;
}

auto BlueprintReader::Read_(long& number) -> bool
{
	if (_current->IsInteger())
	{
		number = static_cast<long>(_current->AsInteger());
		return true;
	}

	return false;
}

auto BlueprintReader::Read_(long long& number) -> bool
{
	if (_current->IsInteger())
	{
		number = static_cast<long long>(_current->AsInteger());
		return true;
	}

	return false;
}

auto BlueprintReader::Read_(unsigned char& number) -> bool
{
	if (_current->IsInteger())
	{
		number = static_cast<unsigned char>(_current->AsInteger());
		return true;
	}

	return false;
}

auto BlueprintReader::Read_(unsigned short& number) -> bool
{
	if (_current->IsInteger())
	{
		number = static_cast<unsigned short>(_current->AsInteger());
		return true;
	}

	return false;
}

auto BlueprintReader::Read_(unsigned int& number) -> bool
{
	if (_current->IsInteger())
	{
		number = static_cast<unsigned int>(_current->AsInteger());
		return true;
	}

	return false;
}

auto BlueprintReader::Read_(unsigned long& number) -> bool
{
	if (_current->IsInteger())
	{
		number = static_cast<unsigned long>(_current->AsInteger());
		return true;
	}

	return false;
}

auto BlueprintReader::Read_(unsigned long long& number) -> bool
{
	if (_current->IsInteger())
	{
		number = static_cast<unsigned long long>(_current->AsInteger());
		return true;
	}

	return false;
}

auto BlueprintReader::Read_(float& number) -> bool
{
	if (_current->IsFloatingPoint())
	{
		number = static_cast<float>(_current->AsFloatingPoint());
		return true;
	}

	return false;
}

auto BlueprintReader::Read_(double& number) -> bool
{
	if (_current->IsFloatingPoint())
	{
		number = static_cast<double>(_current->AsFloatingPoint());
		return true;
	}

	return false;
}

auto BlueprintReader::Read_(long double& number) -> bool
{
	if (_current->IsFloatingPoint())
	{
		number = static_cast<long double>(_current->AsFloatingPoint());
		return true;
	}

	return false;
}

auto BlueprintReader::Read_(Blueprint& blueprint) -> bool
{
	blueprint = *_current;
	return true;
}

auto BlueprintReader::Read_(Buffer& buffer) -> bool
{
	if (_current->IsString())
	{
		auto string = _current->AsStringView();
		buffer = FromBase64(string);
		return true;
	}

	return false;
}

auto BlueprintReader::Read_(String& string) -> bool
{
	if (_current->IsString())
	{
		string = *_current->AsString();
		return true;
	}

	return false;
}

auto BlueprintReader::Read_(StringView& string) -> bool
{
	if (_current->IsString())
	{
		string = _current->AsStringView();
		return true;
	}

	return false;
}

auto BlueprintReader::MoveDown(StringView child) -> bool
{
	auto object = _current->AsObject();

	if (object == nullptr || object->Children.IsEmpty())
		return false;

	auto index = object->Children.GetIndex(child);
	if (index == Sequence::InvalidIndex)
		return false;

	_tree.Add({ _current, _currentIndex });
	_current = std::addressof(object->Children.ItemAtIndex(index));
	_currentIndex = 0u;
	
	return true;
}

auto BlueprintReader::MoveDown(int index) -> bool
{
	auto array = _current->AsArray();

	if (array == nullptr || array->Children.IsEmpty())
		return false;

	if (index >= array->Children.Count())
		return false;

	_tree.Add({ _current, _currentIndex });
	_current = std::addressof(array->Children.Item(index));
	_currentIndex = 0u;
	
	return true;
}

auto BlueprintReader::MoveUp() -> bool
{
	if (_tree.IsEmpty())
		return false;

	auto& parent = _tree.Last();

	_current = parent.Node;
	_currentIndex = parent.Index;

	_tree.RemoveLast();

	return true;
}

auto BlueprintReader::MoveNext() -> bool
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

auto BlueprintReader::FirstChild() -> bool
{
	auto object = _current->AsObject();
	auto array = _current->AsArray();

	if (object != nullptr && !object->Children.IsEmpty())
	{
		_tree.Add({ _current, _currentIndex });
		_current = std::addressof(object->Children.First());
		_currentIndex = 0u;
	}
	else if (array != nullptr && !array->Children.IsEmpty())
	{
		_tree.Add({ _current, _currentIndex });
		_current = std::addressof(array->Children.First());
		_currentIndex = 0u;
	}
	else
	{
		return false;
	}

	return true;
}

auto BlueprintReader::NextChild() -> bool
{
	if (_tree.IsEmpty())
		return false;

	auto parent = _tree.Last();
	auto object = parent.Node->AsObject();
	auto array = parent.Node->AsArray();

	if (object != nullptr)
	{
		if (_currentIndex >= object->Children.LastIndex())
		{
			MoveUp();
			return false;
		}

		_currentIndex++;
		_current = std::addressof(object->Children.ItemAtIndex(_currentIndex));
	}
	else if (array != nullptr)
	{
		if (_currentIndex >= array->Children.LastIndex())
		{
			MoveUp();
			return false;
		}

		_currentIndex++;
		_current = std::addressof(array->Children.Item(_currentIndex));
	}
	else
	{
		MoveUp();
		return false;
	}

	return true;
}

auto BlueprintReader::ChildCount() -> int
{
	auto object = _current->AsObject();
	auto array = _current->AsArray();

	if (object != nullptr && !object->Children.IsEmpty())
		return object->Children.Count();
	else if (array != nullptr && !array->Children.IsEmpty())
		return array->Children.Count();

	return 0u;
}
