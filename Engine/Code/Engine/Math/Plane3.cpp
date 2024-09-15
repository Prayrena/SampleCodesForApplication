#include "Engine/Math/Plane3.hpp"
#include "Engine/Math/MathUtils.hpp"

Plane3::Plane3(Vec3 const& normal, float dist)
	: m_normal(normal)
	, m_distAlongNormalFromOrigin(dist)
{

}

float Plane3::GetAltitudeOfPoint(Vec3 const& refPoint) const
{
	float result = DotProduct3D(refPoint, m_normal);
	return (result - m_distAlongNormalFromOrigin);
}

bool Plane3::IfThePointIsInFrontOfPlane(Vec3 const& pt) const
{
	float distOnNormal = DotProduct3D(pt, m_normal);
	if (distOnNormal > m_distAlongNormalFromOrigin)
	{
		return true;
	}
	else // (distOnNormal <= m_distAlongNormalFromOrigin)
	{
		return false;
	}
}

bool Plane3::IfThePointIsOnThePlane(Vec3 const& pt) const
{
	float distOnNormal = DotProduct3D(pt, m_normal);
	if (distOnNormal == m_distAlongNormalFromOrigin)
	{
		return true;
	}
	else
	{
		return false;
	}
}
