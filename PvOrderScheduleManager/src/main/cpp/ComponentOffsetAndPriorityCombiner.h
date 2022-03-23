#pragma once

#include <srvcalendar.h>

// Forward Declarations
struct IComponent;
class PvOrderObj;

class CComponentOffsetAndPriorityCombiner
{
public:
	static Cerner::Foundations::Calendar CombineComponentOffsetAndPriority(IComponent& component, PvOrderObj& orderObj,
			const Cerner::Foundations::Calendar& phaseStartDateTime);
	static Cerner::Foundations::Calendar ApplyStartDateTimePadding(IComponent& component, PvOrderObj& orderObj,
			const Cerner::Foundations::Calendar& dateTimeToBeUpdated);

private:
	static CString GetStartDateTimeOffset(IComponent& component, PvOrderObj& orderObj, const bool bHasComponentOffset);
	static bool IsTodayNowOffset(CString sOffset);
	static CString GetStartDateTimePaddingVal(IComponent& component, PvOrderObj& orderObj);
};