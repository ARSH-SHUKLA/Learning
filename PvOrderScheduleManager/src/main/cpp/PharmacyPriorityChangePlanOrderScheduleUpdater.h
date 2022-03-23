#pragma once

// Forward Declarations
struct IComponent;
class PvOrderObj;

class CPharmacyPriorityChangePlanOrderScheduleUpdater
{
public:
	CPharmacyPriorityChangePlanOrderScheduleUpdater(const HPATCON hPatCon);

	bool UpdatePlanOrderScheduleOnPharmacyPriorityChange(IComponent& component, PvOrderObj& orderObj);

private:
	bool UpdatePlannedOrderScheduleOnPharmacyPriorityChange(IComponent& component, PvOrderObj& orderObj);
	bool UpdateInitiatedOrderScheduleOnPharmacyPriorityChange(IComponent& component, PvOrderObj& orderObj);

	bool UpdateOrderScheduleWhenPharmacyPriorityChangedToStatOrNow(IComponent& component, PvOrderObj& orderObj);
	bool UpdateOrderScheduleWhenPharmacyPriorityChangedFromStatOrNowToRoutine(IComponent& component, PvOrderObj& orderObj);

private:
	const HPATCON m_hPatCon;
};