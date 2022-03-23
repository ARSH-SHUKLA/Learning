#pragma once

// Forward Declarations
struct IComponent;
class PvOrderObj;

class CFrequencyChangePlanOrderScheduleUpdater
{
public:
	CFrequencyChangePlanOrderScheduleUpdater(const HPATCON hPatCon);

	bool UpdatePlanOrderScheduleOnFrequencyChange(IComponent& component, PvOrderObj& orderObj);

private:
	bool UpdatePlannedOrderScheduleOnFrequencyChange(IComponent& component, PvOrderObj& orderObj);
	bool UpdateInitiatedOrderScheduleOnFrequencyChange(IComponent& component, PvOrderObj& orderObj);
	void SetFrequencyRelatedFieldsOnFrequencyChangeForPlannedOrder(PvOrderObj& orderObj);

private:
	const HPATCON m_hPatCon;
};