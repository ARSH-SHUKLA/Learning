#pragma once

#include <srvcalendar.h>

// Forward Declarations
struct IComponent;
class PvOrderObj;

class CPrescriptionScheduleManager
{
public:
	CPrescriptionScheduleManager(const HPATCON hPatCon);

	void UpdatePrescriptionSchedule(IComponent& component, PvOrderObj& orderObj,
									const Cerner::Foundations::Calendar& phaseStartDateTime);

private:
	void UpdateStopDateTime(IComponent& component, PvOrderObj& orderObj,
							const Cerner::Foundations::Calendar& phaseStartDateTime);
	void UpdateStartDateTime(IComponent& component, PvOrderObj& orderObj,
							 const Cerner::Foundations::Calendar& phaseStartDateTime, PvOrderFld& requestedStartDateTimeFld);

private:
	const HPATCON m_hPatCon;
};