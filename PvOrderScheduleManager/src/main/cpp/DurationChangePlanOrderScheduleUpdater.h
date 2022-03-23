#pragma once

// Forward Declarations
struct IComponent;
class PvOrderObj;

class CDurationChangePlanOrderScheduleUpdater
{
public:
	CDurationChangePlanOrderScheduleUpdater(const HPATCON hPatCon);

	bool UpdatePlanOrderScheduleOnDurationChange(IComponent& component, PvOrderObj& orderObj);

private:
	bool UpdatePlannedOrderScheduleOnDurationChange(IComponent& component, PvOrderObj& orderObj);
	bool UpdateInitiatedOrderScheduleOnDurationChange(IComponent& component, PvOrderObj& orderObj);

private:
	const HPATCON m_hPatCon;
};