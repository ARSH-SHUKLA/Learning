#include "StdAfx.h"

#include "StopDateTimeChangePlanOrderScheduleUpdater.h"
#include "InpatientOrderScheduleServiceCaller.h"
#include "ComponentOffsetHelper.h"

#include <pvorderobj.h>
#include <CPS_ImportPVCareCoordCom.h>

CStopDateTimeChangePlanOrderScheduleUpdater::CStopDateTimeChangePlanOrderScheduleUpdater(const HPATCON hPatCon)
	: m_hPatCon(hPatCon)
{

}

/////////////////////////////////////////////////////////////////////////////
/// \fn		bool CStopDateTimeChangePlanOrderScheduleUpdater::UpdatePlanOrderScheduleOnStopDateTimeChange(IComponent& component, PvOrderObj& orderObj)
/// \brief		Handles updating the schedule of the given plan order when its stop date/time is manually changed.
///
/// \return		bool - True if the schedule was updated successfully. Otherwise, false.
///
/// \param[in]	IComponent& component - The component of the order whose stop date/time changed.
/// \param[in]	PvOrderObj& orderObj - The order whose stop date/time changed.
/// \owner		DB024248
/////////////////////////////////////////////////////////////////////////////
bool CStopDateTimeChangePlanOrderScheduleUpdater::UpdatePlanOrderScheduleOnStopDateTimeChange(IComponent& component,
		PvOrderObj& orderObj)
{
	component.PutLinkToPhase(FALSE);

	CInpatientOrderScheduleServiceCaller inpatientOrderScheduleServiceCaller(m_hPatCon);

	std::list<PvOrderObj*> orders;
	orders.push_back(&orderObj);

	const double dFmtActionCd = orderObj.GetFmtActionCd();

	if (CDF::OrderAction::IsOrder(dFmtActionCd))
	{
		return inpatientOrderScheduleServiceCaller.CalculateNewOrderSchedule(orders,
				CalculateNewOrderScheduleRequest::eStopDateTimeChanged);
	}
	else if (CDF::OrderAction::IsModify(dFmtActionCd) || CDF::OrderAction::IsReschedule(dFmtActionCd))
	{
		return inpatientOrderScheduleServiceCaller.CalculateModifyOrderSchedule(orders,
				CalculateModifyOrderScheduleRequest::eStopDateTimeChanged);
	}

	return true;
}