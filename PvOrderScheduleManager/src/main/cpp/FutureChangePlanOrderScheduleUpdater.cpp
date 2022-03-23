#include "StdAfx.h"

#include "FutureChangePlanOrderScheduleUpdater.h"
#include "InpatientOrderScheduleServiceCaller.h"
#include "ProtocolOrderScheduleManager.h"
#include "PlannedComponentDeterminer.h"
#include "PharmacyPriorityHelper.h"

#include <pvorderobj.h>
#include <PvOrderProtocolObj.h>
#include <CPS_ImportPVCareCoordCom.h>
#include <ActionDateTimeHelper.h>
#include <PvGenericLoaderExtended.h>
#include <dcp_genericloader.h>

CFutureChangePlanOrderScheduleUpdater::CFutureChangePlanOrderScheduleUpdater(const HPATCON hPatCon)
	: m_hPatCon(hPatCon)
{

}

/////////////////////////////////////////////////////////////////////////////
/// \fn		bool CFutureChangePlanOrderScheduleUpdater::UpdatePlanOrderScheduleOnFutureIndicatorChange(IComponent& component, PvOrderObj& orderObj)
/// \brief		Handles updating the schedule of the given plan order when its future indicator is manually changed.
///
/// \return		bool - True if the schedule was updated successfully. Otherwise, false.
///
/// \param[in]	IComponent& component - The component of the order whose future indicator changed.
/// \param[in]	PvOrderObj& orderObj - The order whose future indicator changed.
/// \owner		DB024248
/////////////////////////////////////////////////////////////////////////////
bool CFutureChangePlanOrderScheduleUpdater::UpdatePlanOrderScheduleOnFutureIndicatorChange(IComponent& component,
		PvOrderObj& orderObj)
{
	const bool bPlannedOrderComponent = CPlannedComponentDeterminer::IsPlannedOrderComponent(orderObj);

	if (bPlannedOrderComponent)
	{
		return UpdatePlannedOrderScheduleOnFutureIndicatorChange(component, orderObj);
	}
	else
	{
		return UpdateInitiatedOrderScheduleOnFutureIndicatorChange(component, orderObj);
	}
}

bool CFutureChangePlanOrderScheduleUpdater::UpdatePlannedOrderScheduleOnFutureIndicatorChange(IComponent& component,
		PvOrderObj& orderObj)
{
	const short nFutureInd = (short)orderObj.m_orderFldArr.GetFieldValFromMeanId(eDetailFutureOrder);

	if (nFutureInd == 0)
	{
		const bool bOrderHasPharmacyPriorityOfStatOrNow = CPharmacyPriorityHelper::DoesMedOrderHavePharmacyPriorityOfStatOrNow(
					orderObj);

		if (bOrderHasPharmacyPriorityOfStatOrNow)
		{
			Cerner::Foundations::Calendar actionDateTime = CActionDateTimeHelper::GetActionDateTime(orderObj);

			PvOrderFld* pRequestedStartDateTimeFld = orderObj.m_orderFldArr.GetFieldFromMeanId(eDetailReqStartDtTm);

			if (NULL != pRequestedStartDateTimeFld)
			{
				pRequestedStartDateTimeFld->AddOeFieldDtTmValue(actionDateTime);
				pRequestedStartDateTimeFld->SetValuedSourceInd(eVSUser);
			}
		}
	}

	return true;
}

bool CFutureChangePlanOrderScheduleUpdater::UpdateInitiatedOrderScheduleOnFutureIndicatorChange(IComponent& component,
		PvOrderObj& orderObj)
{
	PvOrderProtocolObj* pOrderProtocolObj = dynamic_cast<PvOrderProtocolObj*>(&orderObj);
	const double dFmtActionCd = orderObj.GetFmtActionCd();

	std::list<PvOrderObj*> orders;

	if (NULL != pOrderProtocolObj)
	{
		std::list<PvOrderObj*> dotOrders;
		CGenLoader().GetDayOfTreatmentOrders(m_hPatCon, *pOrderProtocolObj, dotOrders);

		for (auto orderIter = dotOrders.cbegin(); orderIter != dotOrders.cend(); orderIter++)
		{
			PvOrderObj* pDoTOrderObj = *orderIter;

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
				  CalculateNewOrderScheduleRequest::eFutureIndicatorChanged);
	}

	if (NULL != pOrderProtocolObj)
	{
		CProtocolOrderScheduleManager protocolOrderScheduleManager(m_hPatCon);
		protocolOrderScheduleManager.UpdateProtocolSchedule(*pOrderProtocolObj);
	}

	return bResult;
}