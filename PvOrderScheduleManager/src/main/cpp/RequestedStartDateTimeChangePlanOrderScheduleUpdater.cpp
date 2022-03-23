#include "StdAfx.h"

#include "RequestedStartDateTimeChangePlanOrderScheduleUpdater.h"
#include "InpatientOrderScheduleServiceCaller.h"
#include "ComponentOffsetHelper.h"
#include "PlannedComponentDeterminer.h"

#include <OrderDetailHelper.h>
#include <pvorderobj.h>
#include <CPS_ImportPVCareCoordCom.h>
#include <PvGenericLoaderExtended.h>

CRequestedStartDateTimeChangePlanOrderScheduleUpdater::CRequestedStartDateTimeChangePlanOrderScheduleUpdater(
	const HPATCON hPatCon)
	: m_hPatCon(hPatCon)
{

}

/////////////////////////////////////////////////////////////////////////////
/// \fn		bool CRequestedStartDateTimeChangePlanOrderScheduleUpdater::UpdatePlanOrderScheduleOnRequestedStartDateTimeChange(IComponent& component, PvOrderObj& orderObj)
/// \brief		Handles updating the schedule of the given plan order when its requested start date/time is manually changed.
///
/// \return		bool - True if the schedule was updated successfully. Otherwise, false.
///
/// \param[in]	IComponent& component - The component of the order whose requested start date/time changed.
/// \param[in]	PvOrderObj& orderObj - The order whose requested start date/time changed.
/// \owner		DB024248
/////////////////////////////////////////////////////////////////////////////
bool CRequestedStartDateTimeChangePlanOrderScheduleUpdater::UpdatePlanOrderScheduleOnRequestedStartDateTimeChange(
	IComponent& component, PvOrderObj& orderObj)
{
	const bool bPlannedOrderComponent = CPlannedComponentDeterminer::IsPlannedOrderComponent(orderObj);

	if (bPlannedOrderComponent)
	{
		return UpdatePlannedOrderScheduleOnRequestedStartDateTimeChange(component, orderObj);
	}
	else
	{
		return UpdateInitiatedOrderScheduleOnRequestedStartDateTimeChange(component, orderObj);
	}
}

bool CRequestedStartDateTimeChangePlanOrderScheduleUpdater::UpdatePlannedOrderScheduleOnRequestedStartDateTimeChange(
	IComponent& component, PvOrderObj& orderObj)
{
	// Even if there's no value, we can probably just clear offset. Leaving this here for now, though.
	const bool bStartDateTimeHasValue = COrderDetailHelper::DoesFieldHaveValue(orderObj, eDetailReqStartDtTm);

	if (bStartDateTimeHasValue)
	{
		// If planned component has start offsets and the user changes the order start date, offsets need to be cleared
		CComponentOffsetHelper::ClearComponentOffset(component);

		component.PutLinkedToPhaseStartDateTime(FALSE);
	}

	return true;
}

bool CRequestedStartDateTimeChangePlanOrderScheduleUpdater::UpdateInitiatedOrderScheduleOnRequestedStartDateTimeChange(
	IComponent& component, PvOrderObj& orderObj)
{
	CPvGenericLoaderExtended(m_hPatCon).PrepareForModifyAction(&component);

	component.PutLinkedToPhaseStartDateTime(FALSE);

	CComponentOffsetHelper::ClearComponentOffset(component);

	CInpatientOrderScheduleServiceCaller inpatientOrderScheduleServiceCaller(m_hPatCon);

	std::list<PvOrderObj*> orders;
	orders.push_back(&orderObj);

	const double dFmtActionCd = orderObj.GetFmtActionCd();

	if (CDF::OrderAction::IsOrder(dFmtActionCd))
	{
		return inpatientOrderScheduleServiceCaller.CalculateNewOrderSchedule(orders,
				CalculateNewOrderScheduleRequest::eRequestedStartDateTimeChanged);
	}
	else if (CDF::OrderAction::IsModify(dFmtActionCd) || CDF::OrderAction::IsReschedule(dFmtActionCd))
	{
		return inpatientOrderScheduleServiceCaller.CalculateModifyOrderSchedule(orders,
				CalculateModifyOrderScheduleRequest::eRequestedStartDateTimeChanged);
	}
	else if (CDF::OrderAction::IsActivate(dFmtActionCd))
	{
		return inpatientOrderScheduleServiceCaller.CalculateActivateOrderSchedule(orders,
				CalculateActivateOrderScheduleRequest::eRequestedStartDateTimeChanged);
	}

	return true;
}