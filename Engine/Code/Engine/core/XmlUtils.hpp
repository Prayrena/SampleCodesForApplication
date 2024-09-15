#pragma once
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/core/Rgba8.hpp"
#include "ThirdParty/TinyXML2/tinyxml2.h"
#include "Engine/core/StringUtils.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Math/FloatRange.hpp"

// todo:??? where should I type def
typedef tinyxml2::XMLDocument    XmlDocument;
typedef tinyxml2::XMLElement     XmlElement;
typedef tinyxml2::XMLAttribute   XmlAttribute;
typedef tinyxml2::XMLError       XmlResult;

int			ParseXmlAttribute(XmlElement const& element, char const* attributeName, int defaultValue);
char		ParseXmlAttribute(XmlElement const& element, char const* attributeName, char defaultValue);
bool		ParseXmlAttribute(XmlElement const& element, char const* attributeName, bool defaultValue);
float		ParseXmlAttribute(XmlElement const& element, char const* attributeName, float defaultValue);
FloatRange	ParseXmlAttribute(XmlElement const& element, char const* attributeName, FloatRange range);
Rgba8		ParseXmlAttribute(XmlElement const& element, char const* attributeName, Rgba8 const& defaultValue);
Vec2		ParseXmlAttribute(XmlElement const& element, char const* attributeName, Vec2 const& defaultValue);
Vec3		ParseXmlAttribute(XmlElement const& element, char const* attributeName, Vec3 const& defaultValue);
EulerAngles ParseXmlAttribute(XmlElement const& element, char const* attributeName, EulerAngles const& defaultValue);
IntVec2		ParseXmlAttribute(XmlElement const& element, char const* attributeName, IntVec2 const& defaultValue);
std::string ParseXmlAttribute(XmlElement const& element, char const* attributeName, std::string const& defaultValue);
Strings		ParseXmlAttribute(XmlElement const& element, char const* attributeName, Strings const& defaultValues);// todo:???
std::string ParseXmlAttribute(XmlElement const& element, char const* attributeName, char const* defaultValue);