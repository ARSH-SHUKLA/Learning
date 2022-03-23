#pragma once

// Forward Declarations
struct IComponent;
class PvOrderObj;

class CPRNChangePlanOrderScheduleUpdater
{
public:
	CPRNChangePlanOrderScheduleUpdater(const HPATCON hPatCon);

	bool UpdatePlanOrderScheduleOnPRNChange(IComponent& component, PvOrderObj& orderObj);

private:
	bool UpdatePlannedOrderScheduleOnPRNChange(IComponent& component, PvOrderObj& orderObj);
	bool UpdateInitiatedOrderScheduleOnPRNChange(IComponent& component, PvOrderObj& orderObj);

private:
	const HPATCON m_hPatCon;
};