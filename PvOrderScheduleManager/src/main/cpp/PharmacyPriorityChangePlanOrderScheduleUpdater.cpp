#include "StdAfx.h"

#include "PharmacyPriorityChangePlanOrderScheduleUpdater.h"
#include "InpatientOrderScheduleServiceCaller.h"
#include "ComponentOffsetHelper.h"
#include "PharmacyPriorityHelper.h"
#include "OrderStartDateTimeUpdater.h"
#include "PlannedComponentDeterminer.h"
#include "ComponentOffsetAndPriorityCombiner.h"

#include <OrderDetailHelper.h>
#include <pvorderobj.h>
#include <PvOrderProtocolObj.h>
#include <PvOrdCalUtil.h>
#include <CPS_ImportPVCareCoordCom.h>
#include <ActionDateTimeHelper.h>

CPharmacyPriorityChangePlanOrderScheduleUpdater::CPharmacyPriorityChangePlanOrderScheduleUpdater(const HPATCON hPatCon)
	: m_hPatCon(hPatCon)
{

}

/////////////////////////////////////////////////////////////////////////////
/// \fn		bool CPharmacyPriorityChangePlanOrderScheduleUpdater::UpdatePlanOrderScheduleOnPharmacyPriorityChange(IComponent& component, PvOrderObj& orderObj)
/// \brief		Handles updating the schedule of the given plan order when its pharmacy priority is manually changed.
///
/// \return		bool - True if the schedule was updated successfully. Otherwise, false.
///
/// \param[in]	IComponent& component - The component of the order whose pharmacy priority changed.
/// \param[in]	PvOrderObj& orderObj - The order whose pharmacy priority changed.
/// \owner		DB024248
/////////////////////////////////////////////////////////////////////////////
bool CPharmacyPriorityChangePlanOrderScheduleUpdater::UpdatePlanOrderScheduleOnPharmacyPriorityChange(
	IComponent& component, PvOrderObj& orderObj)
{
	// Pharmacy Priorities of STAT/NOW are not allowed on protocol orders, so changing pharmacy priority shouldn't affect schedule.
	PvOrderProtocolObj* pOrderProtocolObj = dynamic_cast<PvOrderProtocolObj*>(&orderObj);

	if (NULL != pOrderProtocolObj)
	{
		return true;
	}

	// Pharmacy Priorities of STAT/NOW are not allowed on time-zero linked orders, so changing pharmacy priority shouldn't affect schedule.
	const bool bIsTimeZeroActive = component.GetTZActiveInd() == TRUE;

	if (bIsTimeZeroActive)
	{
		return true;
	}

	const bool bPlannedOrderComponent = CPlannedComponentDeterminer::IsPlannedOrderComponent(orderObj);

	if (bPlannedOrderComponent)
	{
		return UpdatePlannedOrderScheduleOnPharmacyPriorityChange(component, orderObj);
	}
	else
	{
		return UpdateInitiatedOrderScheduleOnPharmacyPriorityChange(component, orderObj);
	}
}

bool CPharmacyPriorityChangePlanOrderScheduleUpdater::UpdatePlannedOrderScheduleOnPharmacyPriorityChange(
	IComponent& component, PvOrderObj& orderObj)
{
	const bool bHasStatOrNowPriority = CPharmacyPriorityHelper::DoesMedOrderHavePharmacyPriorityOfStatOrNow(orderObj);

	if (bHasStatOrNowPriority)
	{
		CComponentOffsetHelper::ClearComponentOffset(component);

		COrderDetailHelper::ClearField(orderObj, eDetailReqStartDtTm);

		component.PutLinkedToPhaseStartDateTime(FALSE);

		return true;
	}

	const bool bChangedFromStatOrNowToRoutine = CPharmacyPriorityHelper::WasPharmacyPriorityChangedFromStatOrNowToRoutine(
				orderObj);

	if (bChangedFromStatOrNowToRoutine)
	{
		COrderDetailHelper::ClearField(orderObj, eDetailReqStartDtTm);

		component.PutLinkedToPhaseStartDateTime(TRUE);

		return true;
	}

	return true;
}

bool CPharmacyPriorityChangePlanOrderScheduleUpdater::UpdateInitiatedOrderScheduleOnPharmacyPriorityChange(
	IComponent& component, PvOrderObj& orderObj)
{
	const bool bChangedToStatOrNow = CPharmacyPriorityHelper::DoesMedOrderHavePharmacyPriorityOfStatOrNow(orderObj);

	if (bChangedToStatOrNow)
	{
		return UpdateOrderScheduleWhenPharmacyPriorityChangedToStatOrNow(component, orderObj);
	}

	const bool bChangedFromStatOrNowToRoutine = CPharmacyPriorityHelper::WasPharmacyPriorityChangedFromStatOrNowToRoutine(
				orderObj);

	if (bChangedFromStatOrNowToRoutine)
	{
		return UpdateOrderScheduleWhenPharmacyPriorityChangedFromStatOrNowToRoutine(component, orderObj);
	}

	return true;
}

bool CPharmacyPriorityChangePlanOrderScheduleUpdater::UpdateOrderScheduleWhenPharmacyPriorityChangedToStatOrNow(
	IComponent& component, PvOrderObj& orderObj)
{
	bool bResult = true;

	std::list<PvOrderObj*> orders;
	orders.push_back(&orderObj);

	const double dFmtActionCd = orderObj.GetFmtActionCd();

	component.PutLinkedToPhaseStartDateTime(FALSE);
	CComponentOffsetHelper::ClearComponentOffset(component);

	CInpatientOrderScheduleServiceCaller inpatientOrderScheduleServiceCaller(m_hPatCon);

	if (CDF::OrderAction::IsOrder(dFmtActionCd))
	{
		bResult = inpatientOrderScheduleServiceCaller.CalculateNewOrderSchedule(orders,
				  CalculateNewOrderScheduleRequest::ePriorityChanged);
	}
	else if (CDF::OrderAction::IsModify(dFmtActionCd) || CDF::OrderAction::IsReschedule(dFmtActionCd))
	{
		bResult = inpatientOrderScheduleServiceCaller.CalculateModifyOrderSchedule(orders,
				  CalculateModifyOrderScheduleRequest::ePriorityChanged);
	}

	return bResult;
}

bool CPharmacyPriorityChangePlanOrderScheduleUpdater::UpdateOrderScheduleWhenPharmacyPriorityChangedFromStatOrNowToRoutine(
	IComponent& component, PvOrderObj& orderObj)
{
	bool bResult = true;

	std::list<PvOrderObj*> orders;
	orders.push_back(&orderObj);

	const double dFmtActionCd = orderObj.GetFmtActionCd();

	IPhase* pIPhase = PowerPlanUtil::GetPhase(&component);

	if (NULL != pIPhase)
	{
		ICalendarPtr pIPhaseStartDateTime = pIPhase->GetUTCStartDtTm();
		const Cerner::Foundations::Calendar phaseStartDateTime = PvOrdCalUtils::GetDateOnlyCalendarFromInterface(
					pIPhaseStartDateTime);

		Cerner::Foundations::Calendar actionDateTime = CActionDateTimeHelper::GetActionDateTime(orderObj);

		if (phaseStartDateTime >= actionDateTime)
		{
			COrderStartDateTimeUpdater::SetReferenceStartDateTime(orderObj, phaseStartDateTime, false);
			component.PutLinkedToPhaseStartDateTime(TRUE);
		}
		else
		{
			COrderStartDateTimeUpdater::SetReferenceStartDateTime(orderObj, actionDateTime, false);
		}

		CInpatientOrderScheduleServiceCaller inpatientOrderScheduleServiceCaller(m_hPatCon);

		if (CDF::OrderAction::IsOrder(dFmtActionCd))
		{
			Cerner::Foundations::Calendar referenceStartDtTm = orderObj.m_orderFldArr.GetFieldDtTmValFromMeanId(
				eDetailReferenceStartDtTm);

			Cerner::Foundations::Calendar paddedReferenceStartDateTime =
				CComponentOffsetAndPriorityCombiner::ApplyStartDateTimePadding(component, orderObj, referenceStartDtTm);

			COrderStartDateTimeUpdater::SetReferenceStartDateTime(orderObj, paddedReferenceStartDateTime, false);

			bResult = inpatientOrderScheduleServiceCaller.CalculateNewOrderSchedule(orders,
					  CalculateNewOrderScheduleRequest::eReferenceStartDateTimeChanged);
		}
		else if (CDF::OrderAction::IsModify(dFmtActionCd) || CDF::OrderAction::IsReschedule(dFmtActionCd))
		{
			bResult = inpatientOrderScheduleServiceCaller.CalculateModifyOrderSchedule(orders,
					  CalculateModifyOrderScheduleRequest::eReferenceStartDateTimeChanged);
		}
	}

	return bResult;
}