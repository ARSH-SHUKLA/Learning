#include "StdAfx.h"

#include "ComponentOffsetChangePlanOrderScheduleUpdater.h"
#include "InpatientOrderScheduleServiceCaller.h"
#include "ProtocolOrderScheduleManager.h"
#include "ComponentOffsetAndPriorityCombiner.h"
#include "PlannedComponentDeterminer.h"
#include "OrderStartDateTimeUpdater.h"
#include "ComponentOffsetHelper.h"

#include <OrderDetailHelper.h>
#include <pvorderobj.h>
#include <PvOrderProtocolObj.h>
#include <PvOrdCalUtil.h>
#include <FutureOrderUtil.h>
#include <FutureOrderControllerFactory.h>
#include <CPS_ImportPVCareCoordCom.h>
#include <PvGenericLoaderExtended.h>
#include <dcp_genericloader.h>
#include <ActionDateTimeHelper.h>

CComponentOffsetChangePlanOrderScheduleUpdater::CComponentOffsetChangePlanOrderScheduleUpdater(const HPATCON hPatCon)
	: m_hPatCon(hPatCon)
{

}

/////////////////////////////////////////////////////////////////////////////
/// \fn		bool CComponentOffsetChangePlanOrderScheduleUpdater::UpdatePlanOrderScheduleOnComponentOffsetChange(IComponent& component, PvOrderObj& orderObj)
/// \brief		Handles updating the schedule of the given plan order when its component offset is manually changed.
///
/// \return		bool - True if the schedule was updated successfully. Otherwise, false.
///
/// \param[in]	IComponent& component - The component of the order whose component offset changed.
/// \param[in]	PvOrderObj& orderObj - The order whose component offset changed.
/// \owner		DB024248
/////////////////////////////////////////////////////////////////////////////
bool CComponentOffsetChangePlanOrderScheduleUpdater::UpdatePlanOrderScheduleOnComponentOffsetChange(
	IComponent& component, PvOrderObj& orderObj)
{
	const bool bPlannedOrderComponent = CPlannedComponentDeterminer::IsPlannedOrderComponent(orderObj);

	if (bPlannedOrderComponent)
	{
		return UpdatePlannedOrderScheduleOnComponentOffsetChange(component, orderObj);
	}
	else
	{
		return UpdateInitiatedOrderScheduleOnComponentOffsetChange(component, orderObj);
	}
}

bool CComponentOffsetChangePlanOrderScheduleUpdater::UpdatePlannedOrderScheduleOnComponentOffsetChange(
	IComponent& component, PvOrderObj& orderObj)
{
	COrderDetailHelper::ClearField(orderObj, eDetailReqStartDtTm);

	component.PutLinkedToPhaseStartDateTime(TRUE);

	return true;
}

bool CComponentOffsetChangePlanOrderScheduleUpdater::UpdateInitiatedOrderScheduleOnComponentOffsetChange(
	IComponent& component, PvOrderObj& orderObj)
{
	PvOrderProtocolObj* pOrderProtocolObj = dynamic_cast<PvOrderProtocolObj*>(&orderObj);

	const double dFmtActionCd = orderObj.GetFmtActionCd();

	std::list<PvOrderObj*> orders;

	if (NULL != pOrderProtocolObj)
	{
		component.PutLinkedToPhaseStartDateTime(TRUE);

		std::list<PvOrderObj*> dotOrders;
		CGenLoader().GetDayOfTreatmentOrders(m_hPatCon, *pOrderProtocolObj, dotOrders);

		for (auto orderIter = dotOrders.cbegin(); orderIter != dotOrders.cend(); orderIter++)
		{
			PvOrderObj* pDoTOrderObj = *orderIter;

			const double dDoTFmtActionCd = pDoTOrderObj->GetFmtActionCd();

			if (dDoTFmtActionCd == dFmtActionCd)
			{
				IComponent* pDoTComponent = PowerPlanUtil::GetComponent(pDoTOrderObj);

				if (NULL != pDoTComponent)
				{
					UpdateOrderStartDateTime(*pDoTComponent, *pDoTOrderObj);
					orders.push_back(pDoTOrderObj);
				}
			}
		}
	}
	else
	{
		UpdateOrderStartDateTime(component, orderObj);
		orders.push_back(&orderObj);
	}

	bool bResult = true;
	CInpatientOrderScheduleServiceCaller inpatientOrderScheduleServiceCaller(m_hPatCon);

	if (CDF::OrderAction::IsOrder(dFmtActionCd))
	{
		bResult = inpatientOrderScheduleServiceCaller.CalculateNewOrderSchedule(orders,
				  CalculateNewOrderScheduleRequest::eReferenceStartDateTimeChanged);
	}
	else if (CDF::OrderAction::IsModify(dFmtActionCd) || CDF::OrderAction::IsReschedule(dFmtActionCd))
	{
		bResult = inpatientOrderScheduleServiceCaller.CalculateModifyOrderSchedule(orders,
				  CalculateModifyOrderScheduleRequest::eReferenceStartDateTimeChanged);
	}
	else if (CDF::OrderAction::IsActivate(dFmtActionCd))
	{
		bResult = inpatientOrderScheduleServiceCaller.CalculateActivateOrderSchedule(orders,
				  CalculateActivateOrderScheduleRequest::eReferenceStartDateTimeChanged);
	}

	if (NULL != pOrderProtocolObj)
	{
		CProtocolOrderScheduleManager protocolOrderScheduleManager(m_hPatCon);
		protocolOrderScheduleManager.UpdateProtocolSchedule(*pOrderProtocolObj);
	}

	return bResult;
}

void CComponentOffsetChangePlanOrderScheduleUpdater::UpdateOrderStartDateTime(IComponent& component,
		PvOrderObj& orderObj)
{
	component.PutLinkedToPhaseStartDateTime(TRUE);

	const bool bHasComponentOffset = component.HasComponentOffset() == TRUE;

	if (bHasComponentOffset)
	{
		if (CFutureOrderUtil::IsFutureOrderScheduleValid(&orderObj))
		{
			CFutureOrderControllerFactory().CreateFutureOrderController(&orderObj).ClearFutureOrderSchedule(&orderObj);
		}
	}

	IPhase* pIPhase = PowerPlanUtil::GetPhase(&component);

	if (pIPhase != nullptr)
	{
		ICalendarPtr pIPhaseStartDateTime = pIPhase->GetUTCStartDtTm();

		if (pIPhaseStartDateTime != nullptr && !pIPhaseStartDateTime->IsNull())
		{
			const Cerner::Foundations::Calendar phaseStartDateTime = PvOrdCalUtils::GetDateOnlyCalendarFromInterface(
						pIPhaseStartDateTime);
			Cerner::Foundations::Calendar referenceStartDateTime =
				CComponentOffsetAndPriorityCombiner::CombineComponentOffsetAndPriority(component, orderObj, phaseStartDateTime);
			const Cerner::Foundations::Calendar& actionDateTime = CActionDateTimeHelper::GetActionDateTime(orderObj);

			// Reset the order's date time to action date time if the action date time is greter than reference start date time.
			if (!referenceStartDateTime.IsNull() && actionDateTime.Compare(referenceStartDateTime) == 1)
			{
				component.PutLinkedToPhaseStartDateTime(FALSE);
				COrderStartDateTimeUpdater::SetReferenceStartDateTime(orderObj, actionDateTime, true);
				return;
			}

			COrderStartDateTimeUpdater::SetReferenceStartDateTime(orderObj, referenceStartDateTime, false);
		}
	}
}