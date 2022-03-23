#pragma once

#include <srvcalendar.h>
#include <CalculateActivateOrderScheduleCriteria.h>

// Forward Declarations
struct IComponent;
class PvOrderObj;

class CActivateOrderScheduleManager
{
public:
	CCalculateActivateOrderScheduleCriteria UpdateOrderScheduleOnActivate(IComponent& component, PvOrderObj& orderObj,
			const Cerner::Foundations::Calendar& phaseStartDateTime, const Cerner::Foundations::Calendar& timeZeroDateTime,
			const Cerner::Foundations::Calendar& phaseStopDateTime);

private:
	CCalculateActivateOrderScheduleCriteria UpdateStartDateTime(IComponent& component, PvOrderObj& orderObj,
			const Cerner::Foundations::Calendar& phaseStartDateTime, const Cerner::Foundations::Calendar& actionDateTime,
			const Cerner::Foundations::Calendar& timeZeroDateTime);
};