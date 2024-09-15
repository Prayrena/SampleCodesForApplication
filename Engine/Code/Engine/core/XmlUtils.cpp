#include "Engine/core/XmlUtils.hpp"
#include "StringUtils.hpp"
#include <string>

int ParseXmlAttribute(XmlElement const& element, char const* attributeName, int defaultValue)
{
	char const* valueAsText = element.Attribute(attributeName);
	if (valueAsText)
	{
		return atoi(valueAsText);
	}
	else return defaultValue;
}

char ParseXmlAttribute(XmlElement const& element, char const* attributeName, char defaultValue)
{
	char const* valueAsText = element.Attribute(attributeName);
	if (valueAsText)
	{
		return *valueAsText;
	}
	else return defaultValue;
}

bool ParseXmlAttribute(XmlElement const& element, char const* attributeName, bool defaultValue)
{
	// todo: parse xml has issues sometimes
	char const* valueAsText = element.Attribute(attributeName);
	char char_true[] = "true";
	if (valueAsText)
	{
		if (*valueAsText == *char_true)
		{
			return true;
		}
		else return false;
	}
	else return defaultValue;
}

float ParseXmlAttribute(XmlElement const& element, char const* attributeName, float defaultValue)
{
	char const* valueAsText = element.Attribute(attributeName);
	if (valueAsText)
	{
		return (float)atof(valueAsText);
	}
	else return defaultValue;
}

FloatRange ParseXmlAttribute(XmlElement const& element, char const* attributeName, FloatRange range)
{
	char const* valueAsText = element.Attribute(attributeName); // attribute translate element into char
	FloatRange floatRange = range;
	if (valueAsText)
	{
		if (floatRange.SetFromText(valueAsText))
		{
			return floatRange;
		}
		else return floatRange;
	}
	else return floatRange;
}

Rgba8 ParseXmlAttribute(XmlElement const& element, char const* attributeName, Rgba8 const& defaultValue)
{
	char const* valueAsText = element.Attribute(attributeName);
	Rgba8 rgb8 = defaultValue;
	if (valueAsText)
	{
		rgb8.SetFromText(valueAsText);
		return rgb8;
	}
	else return defaultValue;
}

Vec2 ParseXmlAttribute(XmlElement const& element, char const* attributeName, Vec2 const& defaultValue)
{
	char const* valueAsText = element.Attribute(attributeName);
	Vec2 vec2 = defaultValue;
	if (valueAsText)
	{
		vec2.SetFromText(valueAsText);
		return vec2;
	}
	else return defaultValue;
}

Vec3 ParseXmlAttribute(XmlElement const& element, char const* attributeName, Vec3 const& defaultValue)
{
	char const* valueAsText = element.Attribute(attributeName);
	Vec3 vec3 = defaultValue;
	if (valueAsText)
	{
		vec3.SetFromText(valueAsText);
		return vec3;
	}
	else return defaultValue;
}


EulerAngles ParseXmlAttribute(XmlElement const& element, char const* attributeName, EulerAngles const& defaultValue)
{
	char const* valueAsText = element.Attribute(attributeName);
	EulerAngles angle = defaultValue;
	if (valueAsText)
	{
		angle.SetFromText(valueAsText);
		return angle;
	}
	else return defaultValue;
}

IntVec2 ParseXmlAttribute(XmlElement const& element, char const* attributeName, IntVec2 const& defaultValue)
{
	// todo:??? what if the element is 0, then I could not get the value
	char const* valueAsText = element.Attribute(attributeName); // attribute translate element into char
	IntVec2 intVec2 = defaultValue;
	if (valueAsText)
	{
		intVec2.SetFromText(valueAsText);
		return intVec2;
	}
	else return defaultValue;
}

std::string ParseXmlAttribute(XmlElement const& element, char const* attributeName, std::string const& defaultValue)
{
	char const* valueAsText = element.Attribute(attributeName);
	std::string stringValue = defaultValue;
	if (valueAsText)
	{
		stringValue = valueAsText;
		return stringValue;
	}
	else return defaultValue;
}

Strings ParseXmlAttribute(XmlElement const& element, char const* attributeName, Strings const& defaultValues)
{
	char const* valueAsText = element.Attribute(attributeName); // attribute translate element into char
	Strings stringsValues = defaultValues;
	if (valueAsText)
	{
		// todo: ???
		std::string originalString = valueAsText;
		stringsValues = SplitStringOnDelimiter(originalString, ',');
		return stringsValues;
	}
	else return defaultValues;
}

std::string ParseXmlAttribute(XmlElement const& element, char const* attributeName, char const* defaultValue)
{
	char const* valueAsText = element.Attribute(attributeName);
	std::string stringValue = defaultValue;
	if (valueAsText)
	{
		stringValue = valueAsText;
		return stringValue;
	}
	else return defaultValue;
}


