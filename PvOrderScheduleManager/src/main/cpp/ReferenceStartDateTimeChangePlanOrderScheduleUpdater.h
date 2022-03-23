#pragma once

// Forward Declarations
struct IComponent;
class PvOrderObj;

class CReferenceStartDateTimeChangePlanOrderScheduleUpdater
{
public:
	CReferenceStartDateTimeChangePlanOrderScheduleUpdater(const HPATCON hPatCon);

	bool UpdatePlanOrderScheduleOnReferenceStartDateTimeChange(IComponent& component, PvOrderObj& orderObj);

private:
	const HPATCON m_hPatCon;
};