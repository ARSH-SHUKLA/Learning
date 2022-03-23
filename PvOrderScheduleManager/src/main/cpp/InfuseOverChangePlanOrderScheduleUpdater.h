#pragma once

// Forward Declarations
struct IComponent;
class PvOrderObj;

class CInfuseOverChangePlanOrderScheduleUpdater
{
public:
	CInfuseOverChangePlanOrderScheduleUpdater(const HPATCON hPatCon);

	bool UpdatePlanOrderScheduleOnInfuseOverChange(IComponent& component, PvOrderObj& orderObj);

private:
	const HPATCON m_hPatCon;
};