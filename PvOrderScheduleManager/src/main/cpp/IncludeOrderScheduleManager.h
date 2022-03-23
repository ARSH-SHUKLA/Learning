#pragma once

#include <srvcalendar.h>
#include <afxstr.h>

// Forward Declarations
struct IComponent;
class PvOrderObj;

class CIncludeOrderScheduleManager
{
public:
	bool UpdateOrderScheduleOnInclude(IComponent& component, PvOrderObj& orderObj,
									  const Cerner::Foundations::Calendar& phaseStartDateTime, const Cerner::Foundations::Calendar& timeZeroDateTime,
									  const Cerner::Foundations::Calendar& phaseStopDateTime) const;

private:
	bool UpdateOrderStopDateTime(IComponent& component, PvOrderObj& orderObj,
								 const Cerner::Foundations::Calendar& phaseStartDateTime, const Cerner::Foundations::Calendar& phaseStopDateTime) const;
	void UpdateStartDateTime(IComponent& component, PvOrderObj& orderObj,
							 const Cerner::Foundations::Calendar& phaseStartDateTime, const Cerner::Foundations::Calendar& actionDateTime,
							 const Cerner::Foundations::Calendar& timeZeroDateTime) const;
};