#pragma once

// Forward Declarations
struct IComponent;
class PvOrderObj;

class CFutureChangePlanOrderScheduleUpdater
{
public:
	CFutureChangePlanOrderScheduleUpdater(const HPATCON hPatCon);

	bool UpdatePlanOrderScheduleOnFutureIndicatorChange(IComponent& component, PvOrderObj& orderObj);

private:
	bool UpdatePlannedOrderScheduleOnFutureIndicatorChange(IComponent& component, PvOrderObj& orderObj);
	bool UpdateInitiatedOrderScheduleOnFutureIndicatorChange(IComponent& component, PvOrderObj& orderObj);

private:
	const HPATCON m_hPatCon;
};