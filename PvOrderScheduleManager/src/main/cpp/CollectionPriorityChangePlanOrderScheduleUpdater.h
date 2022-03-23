#pragma once

// Forward Declarations
struct IComponent;
class PvOrderObj;
class PvOrderProtocolObj;

class CCollectionPriorityChangePlanOrderScheduleUpdater
{
public:
	CCollectionPriorityChangePlanOrderScheduleUpdater(const HPATCON hPatCon);

	bool UpdatePlanOrderScheduleOnCollectionPriorityChange(IComponent& component, PvOrderObj& orderObj);

private:
	bool UpdatePlannedOrderScheduleOnCollectionPriorityChange(IComponent& component, PvOrderObj& orderObj,
			CString& sCollectionPriorityOffset);
	bool UpdateInitiatedOrderScheduleOnCollectionPriorityChange(IComponent& component, PvOrderObj& orderObj,
			CString& sCollectionPriorityOffset);
	bool UpdateProtocolOrderScheduleOnCollectionPriorityChange(IComponent& protocolComponent,
			PvOrderProtocolObj& protocolOrderObj, CString& sCollectionPriorityOffset);

	bool CallInpatientOrdersScheduleService(std::list<PvOrderObj*>& orders, const double dFmtActionCd);
	bool DoesOrderScheduleNeedUpdated(IComponent& component, PvOrderObj& orderObj, CString& sCollectionPriorityOffset);
	void UpdateOrderStartDateTime(IComponent& component, PvOrderObj& orderObj, CString& sCollectionPriorityOffset);
	bool CallInpatientOrderScheduleServiceForSemicolonPriorityOnCollectionPriorityChange(std::list<PvOrderObj*>& orders,
			const double dFmtActionCd);

private:
	const HPATCON m_hPatCon;
};