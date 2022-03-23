#pragma once

// Forward Declarations
struct IComponent;
class PvOrderObj;

class CConstantIndicatorChangePlanOrderScheduleUpdater
{
public:
	CConstantIndicatorChangePlanOrderScheduleUpdater(const HPATCON hPatCon);

	bool UpdatePlanOrderScheduleOnConstantIndicatorChange(IComponent& component, PvOrderObj& orderObj);

private:
	const HPATCON m_hPatCon;
};