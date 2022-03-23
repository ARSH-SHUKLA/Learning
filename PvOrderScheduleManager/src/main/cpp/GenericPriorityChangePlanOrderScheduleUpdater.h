#pragma once

// Forward Declarations
struct IComponent;
class PvOrderObj;
class PvOrderProtocolObj;

class CGenericPriorityChangePlanOrderScheduleUpdater
{
public:
	CGenericPriorityChangePlanOrderScheduleUpdater(const HPATCON hPatCon);

	bool UpdatePlanOrderScheduleOnGenericPriorityChange(IComponent& component, PvOrderObj& orderObj);

private:
	bool UpdatePlannedOrderScheduleOnGenericPriorityChange(IComponent& component, PvOrderObj& orderObj,
			CString& sGenericPriorityOffset);
	bool UpdateInitiatedOrderScheduleOnGenericPriorityChange(IComponent& component, PvOrderObj& orderObj,
			CString& sGenericPriorityOffset);
	bool UpdateProtocolOrderScheduleOnGenericPriorityChange(IComponent& protocolComponent,
			PvOrderProtocolObj& protocolOrderObj, CString& sGenericPriorityOffset);

	bool CallInpatientOrdersScheduleService(std::list<PvOrderObj*>& orders, const double dFmtActionCd);
	bool DoesOrderScheduleNeedUpdated(IComponent& component, PvOrderObj& orderObj, CString& sGenericPriorityOffset);
	void UpdateOrderStartDateTime(IComponent& component, PvOrderObj& orderObj, CString& sGenericPriorityOffset);
	bool CallInpatientOrderScheduleServiceForSemicolonPriorityOnGenericPriorityChange(std::list<PvOrderObj*>& orders,
			const double dFmtActionCd);

private:
	const HPATCON m_hPatCon;
};