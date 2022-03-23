#pragma once

// Forward Declarations
struct IComponent;
class PvOrderObj;

class CComponentOffsetChangePlanOrderScheduleUpdater
{
public:
	CComponentOffsetChangePlanOrderScheduleUpdater(const HPATCON hPatCon);

	bool UpdatePlanOrderScheduleOnComponentOffsetChange(IComponent& component, PvOrderObj& orderObj);

private:
	bool UpdatePlannedOrderScheduleOnComponentOffsetChange(IComponent& component, PvOrderObj& orderObj);
	bool UpdateInitiatedOrderScheduleOnComponentOffsetChange(IComponent& component, PvOrderObj& orderObj);

	void UpdateOrderStartDateTime(IComponent& component, PvOrderObj& orderObj);

private:
	const HPATCON m_hPatCon;
};