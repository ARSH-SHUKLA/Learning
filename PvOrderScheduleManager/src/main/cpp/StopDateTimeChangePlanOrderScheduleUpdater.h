#pragma once

// Forward Declarations
struct IComponent;
class PvOrderObj;

class CStopDateTimeChangePlanOrderScheduleUpdater
{
public:
	CStopDateTimeChangePlanOrderScheduleUpdater(const HPATCON hPatCon);

	bool UpdatePlanOrderScheduleOnStopDateTimeChange(IComponent& component, PvOrderObj& orderObj);

private:
	const HPATCON m_hPatCon;
};