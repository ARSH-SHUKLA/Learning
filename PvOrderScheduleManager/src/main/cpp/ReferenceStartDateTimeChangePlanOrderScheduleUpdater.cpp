#include "StdAfx.h"

#include "ReferenceStartDateTimeChangePlanOrderScheduleUpdater.h"
#include "InpatientOrderScheduleServiceCaller.h"
#include "ComponentOffsetHelper.h"

#include <pvorderobj.h>
#include <CPS_ImportPVCareCoordCom.h>

CReferenceStartDateTimeChangePlanOrderScheduleUpdater::CReferenceStartDateTimeChangePlanOrderScheduleUpdater(
	const HPATCON hPatCon)
	: m_hPatCon(hPatCon)
{

}

/////////////////////////////////////////////////////////////////////////////
/// \fn		bool CReferenceStartDateTimeChangePlanOrderScheduleUpdater::UpdatePlanOrderScheduleOnReferenceStartDateTimeChange(IComponent& component, PvOrderObj& orderObj)
/// \brief		Handles updating the schedule of the given plan order when its reference start date/time is manually changed.
///
/// \return		bool - True if the schedule was updated successfully. Otherwise, false.
///
/// \param[in]	IComponent& component - The component of the order whose reference start date/time changed.
/// \param[in]	PvOrderObj& orderObj - The order whose reference start date/time changed.
/// \owner		DB024248
/////////////////////////////////////////////////////////////////////////////
bool CReferenceStartDateTimeChangePlanOrderScheduleUpdater::UpdatePlanOrderScheduleOnReferenceStartDateTimeChange(
	IComponent& component, PvOrderObj& orderObj)
{
	component.PutLinkedToPhaseStartDateTime(FALSE);

	CComponentOffsetHelper::ClearComponentOffset(component);

	CInpatientOrderScheduleServiceCaller inpatientOrderScheduleServiceCaller(m_hPatCon);

	std::list<PvOrderObj*> orders;
	orders.push_back(&orderObj);

	const double dFmtActionCd = orderObj.GetFmtActionCd();

	if (CDF::OrderAction::IsOrder(dFmtActionCd))
	{
		return inpatientOrderScheduleServiceCaller.CalculateNewOrderSchedule(orders,
				CalculateNewOrderScheduleRequest::eReferenceStartDateTimeChanged);
	}
	else if (CDF::OrderAction::IsModify(dFmtActionCd) || CDF::OrderAction::IsReschedule(dFmtActionCd))
	{
		return inpatientOrderScheduleServiceCaller.CalculateModifyOrderSchedule(orders,
				CalculateModifyOrderScheduleRequest::eReferenceStartDateTimeChanged);
	}
	else if (CDF::OrderAction::IsActivate(dFmtActionCd))
	{
		return inpatientOrderScheduleServiceCaller.CalculateActivateOrderSchedule(orders,
				CalculateActivateOrderScheduleRequest::eReferenceStartDateTimeChanged);
	}

	return true;
}