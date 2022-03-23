#pragma once

#include <srvcalendar.h>
#include <afxstr.h>

// Forward Declarations
struct IComponent;
class PvOrderObj;

class CChangePhaseStartOrderScheduleManager
{
public:
	bool UpdateOrderScheduleOnChangePhaseStart(IComponent& component, PvOrderObj& orderObj,
			const Cerner::Foundations::Calendar& phaseStartDateTime, const Cerner::Foundations::Calendar& timeZeroDateTime,
			const Cerner::Foundations::Calendar& phaseStopDateTime) const;

private:
	Cerner::Foundations::Calendar GetUpdatedStartDateTime(IComponent& component, PvOrderObj& orderObj,
			const Cerner::Foundations::Calendar& phaseStartDateTime, const Cerner::Foundations::Calendar& actionDateTime,
			const Cerner::Foundations::Calendar& timeZeroDateTime) const;
};