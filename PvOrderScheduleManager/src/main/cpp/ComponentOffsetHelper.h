#pragma once

#include <srvcalendar.h>

// Forward Declarations
struct IComponent;

class CComponentOffsetHelper
{
public:
	static bool DoesComponentHaveOffsetInHoursOrMinutes(IComponent& component);
	static bool DoesComponentHaveOffsetInDays(IComponent& component);
	static void ClearComponentOffset(IComponent& component);
	static Cerner::Foundations::Calendar AddComponentOffset(IComponent& component,
			const Cerner::Foundations::Calendar& dateTime);
};