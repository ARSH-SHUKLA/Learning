#include "StdAfx.h"

#include "InfuseOverChangePlanOrderScheduleUpdater.h"
#include "InpatientOrderScheduleServiceCaller.h"
#include "ProtocolOrderScheduleManager.h"
#include "PlannedComponentDeterminer.h"

#include <pvorderobj.h>
#include <PvOrderProtocolObj.h>
#include <CPS_ImportPVCareCoordCom.h>
#include <PvGenericLoaderExtended.h>
#include <dcp_genericloader.h>

CInfuseOverChangePlanOrderScheduleUpdater::CInfuseOverChangePlanOrderScheduleUpdater(const HPATCON hPatCon)
	: m_hPatCon(hPatCon)
{

}

/////////////////////////////////////////////////////////////////////////////
/// \fn		bool CInfuseOverChangePlanOrderScheduleUpdater::UpdatePlanOrderScheduleOnInfuseOverChange(IComponent& component, PvOrderObj& orderObj)
/// \brief		Handles updating the schedule of the given plan order when its infuse over is manually changed.
///
/// \return		bool - True if the schedule was updated successfully. Otherwise, false.
///
/// \param[in]	IComponent& component - The component of the order whose infuse over changed.
/// \param[in]	PvOrderObj& orderObj - The order whose infuse over changed.
/// \owner		DB024248
/////////////////////////////////////////////////////////////////////////////
bool CInfuseOverChangePlanOrderScheduleUpdater::UpdatePlanOrderScheduleOnInfuseOverChange(IComponent& component,
		PvOrderObj& orderObj)
{
	const bool bPlannedOrderComponent = CPlannedComponentDeterminer::IsPlannedOrderComponent(orderObj);

	if (bPlannedOrderComponent)
	{
		return true;
	}

	component.PutLinkToPhase(FALSE);

	PvOrderProtocolObj* pOrderProtocolObj = dynamic_cast<PvOrderProtocolObj*>(&orderObj);
	const double dFmtActionCd = orderObj.GetFmtActionCd();

	std::list<PvOrderObj*> orders;

	if (NULL != pOrderProtocolObj)
	{
		std::list<PvOrderObj*> dotOrders;
		CGenLoader().GetDayOfTreatmentOrders(m_hPatCon, *pOrderProtocolObj, dotOrders);

		for (auto dotIter = dotOrders.cbegin(); dotIter != dotOrders.cend(); dotIter++)
		{
			PvOrderObj* pDoTOrderObj = *dotIter;
			const double dDoTFmtActionCd = pDoTOrderObj->GetFmtActionCd();

			if (dDoTFmtActionCd == dFmtActionCd)
			{
				orders.push_back(pDoTOrderObj);
			}
		}
	}
	else
	{
		orders.push_back(&orderObj);
	}

	bool bResult = true;

	CInpatientOrderScheduleServiceCaller inpatientOrderScheduleServiceCaller(m_hPatCon);

	if (CDF::OrderAction::IsOrder(dFmtActionCd))
	{
		bResult = inpatientOrderScheduleServiceCaller.CalculateNewOrderSchedule(orders,
				  CalculateNewOrderScheduleRequest::eInfuseOrderChanged);
	}
	else if (CDF::OrderAction::IsModify(dFmtActionCd) || CDF::OrderAction::IsReschedule(dFmtActionCd))
	{
		bResult = inpatientOrderScheduleServiceCaller.CalculateModifyOrderSchedule(orders,
				  CalculateModifyOrderScheduleRequest::eInfuseOrderChanged);
	}

	if (NULL != pOrderProtocolObj)
	{
		CProtocolOrderScheduleManager protocolOrderScheduleManager(m_hPatCon);
		protocolOrderScheduleManager.UpdateProtocolSchedule(*pOrderProtocolObj);
	}

	return bResult;
}