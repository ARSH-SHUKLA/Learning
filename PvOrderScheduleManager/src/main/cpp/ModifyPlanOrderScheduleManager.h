#pragma once

// Forward Declarations
class PvOrderObj;

class CModifyPlanOrderScheduleManager
{
public:
	CModifyPlanOrderScheduleManager(const HPATCON hPatCon);

	bool UpdateOrderScheduleOnInitialModify(PvOrderObj& orderObj);

private:
	HPATCON m_hPatCon;

	void AddOrderReferenceStartDateTimeOnInitialModify(PvOrderObj& orderObj);
};