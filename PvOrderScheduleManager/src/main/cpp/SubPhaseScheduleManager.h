#pragma once

#include <srvcalendar.h>

// Forward Declarations
struct IPhase;

class CSubPhaseScheduleManager
{
public:
	void UpdateSubPhaseSchedule(IPhase& subPhase, const Cerner::Foundations::Calendar& parentPhaseStartDateTime,
								const Cerner::Foundations::Calendar& parentPhaseTimeZeroDateTime,
								const Cerner::Foundations::Calendar& parentPhaseStopDateTime) const;

private:
	Cerner::Foundations::Calendar CalculateSubPhaseStartDateTime(IPhase& subPhase,
			const Cerner::Foundations::Calendar& parentPhaseStartDateTime,
			const Cerner::Foundations::Calendar& parentPhaseTimeZeroDateTime) const;
	Cerner::Foundations::Calendar CalculateSubPhaseStopDateTime(IPhase& subPhase,
			const Cerner::Foundations::Calendar& subPhaseStartDateTime,
			const Cerner::Foundations::Calendar& parentPhaseStopDateTime) const;
};