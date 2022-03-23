#include "StdAfx.h"

#include "ChangePhaseStartOrderScheduleManager.h"
#include "TimeZeroHelper.h"
#include "OrderStartDateTimeUpdater.h"
#include "ComponentOffsetAndPriorityCombiner.h"

#include <pvorderobj.h>
#include <srvcalendar.h>
#include <PriorityDefs.h>
#include <pvorderdataexp.h>
#include <CPS_ImportPVCareCoordCom.h>
#include <ActionDateTimeHelper.h>

bool CChangePhaseStartOrderScheduleManager::UpdateOrderScheduleOnChangePhaseStart(IComponent& component,
		PvOrderObj& orderObj, const Cerner::Foundations::Calendar& phaseStartDateTime,
		const Cerner::Foundations::Calendar& timeZeroDateTime, const Cerner::Foundations::Calendar& phaseStopDateTime) const
{
	const Cerner::Foundations::Calendar& actionDateTime = CActionDateTimeHelper::GetActionDateTime(orderObj);

	Cerner::Foundations::Calendar updatedStartDateTime = GetUpdatedStartDateTime(component, orderObj, phaseStartDateTime,
			actionDateTime, timeZeroDateTime);

	COrderStartDateTimeUpdater::SetReferenceStartDateTime(orderObj, updatedStartDateTime, false);

	return true;
}

Cerner::Foundations::Calendar CChangePhaseStartOrderScheduleManager::GetUpdatedStartDateTime(IComponent& component,
		PvOrderObj& orderObj, const Cerner::Foundations::Calendar& phaseStartDateTime,
		const Cerner::Foundations::Calendar& actionDateTime, const Cerner::Foundations::Calendar& timeZeroDateTime) const
{
	const bool bIsTimeZeroActive = component.GetTZActiveInd() == TRUE;

	if (bIsTimeZeroActive)
	{
		return CTimeZeroHelper::CalculateStartDateTimeFromTimeZeroInformation(component, phaseStartDateTime, timeZeroDateTime);
	}

	return CComponentOffsetAndPriorityCombiner::CombineComponentOffsetAndPriority(component, orderObj, phaseStartDateTime);
}