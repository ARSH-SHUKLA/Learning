#pragma once

#include <srvcalendar.h>

// Forward Declarations
struct IPhase;

class CPhaseTimeZeroDateTimeDeterminer
{
public:
	ICalendarPtr DetermineTimeZeroDateTime(IPhase& phase) const;
};