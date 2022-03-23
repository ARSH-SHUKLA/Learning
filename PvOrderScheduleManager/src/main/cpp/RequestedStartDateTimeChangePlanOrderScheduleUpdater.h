#pragma once

// Forward Declarations
struct IComponent;
class PvOrderObj;

class CRequestedStartDateTimeChangePlanOrderScheduleUpdater
{
public:
	CRequestedStartDateTimeChangePlanOrderScheduleUpdater(const HPATCON hPatCon);

	bool UpdatePlanOrderScheduleOnRequestedStartDateTimeChange(IComponent& component, PvOrderObj& orderObj);

private:
	bool UpdatePlannedOrderScheduleOnRequestedStartDateTimeChange(IComponent& component, PvOrderObj& orderObj);
	bool UpdateInitiatedOrderScheduleOnRequestedStartDateTimeChange(IComponent& component, PvOrderObj& orderObj);

private:
	const HPATCON m_hPatCon;
};