#pragma once

#include <srvcalendar.h>

// Forward Declarations
struct IComponent;
struct IPhase;

class CTimeZeroHelper
{
public:
	static Cerner::Foundations::Calendar CalculateStartDateTimeFromTimeZeroInformation(IComponent& component,
			const Cerner::Foundations::Calendar& phaseStartDateTime, const Cerner::Foundations::Calendar& timeZeroDateTime);
	static double GetTimeZeroOffsetInMinutes(IComponent& component);
};