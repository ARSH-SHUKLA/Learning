#pragma once

// Forward Declarations
struct IComponent;
class PvOrderObj;

class CSchedulingInstructionsChangePlanOrderScheduleUpdater
{
public:
	CSchedulingInstructionsChangePlanOrderScheduleUpdater(const HPATCON hPatCon);

	bool UpdatePlanOrderScheduleOnSchedulingInstructionsChange(IComponent& component, PvOrderObj& orderObj);

private:
	bool UpdatePlannedOrderScheduleOnSchedulingInstructionsChange(IComponent& component, PvOrderObj& orderObj);
	bool UpdateInitiatedOrderScheduleOnSchedulingInstructionsChange(IComponent& component, PvOrderObj& orderObj);

	void UpdatePlannedOrderScheduleOnFutureIndicatorChange(IComponent& component, PvOrderObj& orderObj);
	bool DoesOrderScheduleNeedUpdated(IComponent& component, PvOrderObj& orderObj);
	void UpdateOrderStartDateTime(IComponent& component, PvOrderObj& orderObj,
								  const CString& sSchedulingInstructionsOffset);

private:
	const HPATCON m_hPatCon;
};