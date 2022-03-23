#include "StdAfx.h"

#include "SubPhaseScheduleManager.h"
#include "TimeZeroHelper.h"
#include "ComponentOffsetHelper.h"

#include <CPS_ImportPVCareCoordCom.h>
#include <PvOrdCalUtil.h>

void CSubPhaseScheduleManager::UpdateSubPhaseSchedule(IPhase& subPhase,
		const Cerner::Foundations::Calendar& parentPhaseStartDateTime,
		const Cerner::Foundations::Calendar& parentPhaseTimeZeroDateTime,
		const Cerner::Foundations::Calendar& parentPhaseStopDateTime) const
{
	Cerner::Foundations::Calendar subPhaseStartDateTime = CalculateSubPhaseStartDateTime(subPhase, parentPhaseStartDateTime,
			parentPhaseTimeZeroDateTime);

	ICalendarPtr pISubPhaseStartDateTime(__uuidof(Calendar));
	pISubPhaseStartDateTime->InitFromHandle(subPhaseStartDateTime.GetHandle());
	subPhase.PutUTCStartDtTm(pISubPhaseStartDateTime);

	ICalendarPtr pISubPhaseTimeZeroDateTime(__uuidof(Calendar));
	pISubPhaseTimeZeroDateTime->InitToNull();
	subPhase.PutUTCCalcTimeZeroDtTm(pISubPhaseTimeZeroDateTime);

	Cerner::Foundations::Calendar subPhaseStopDateTime = CalculateSubPhaseStopDateTime(subPhase, subPhaseStartDateTime,
			parentPhaseStopDateTime);

	ICalendarPtr pISubPhaseStopDateTime(__uuidof(Calendar));
	pISubPhaseStopDateTime->InitFromHandle(subPhaseStopDateTime.GetHandle());
	subPhase.PutUTCCalcEndDtTm(pISubPhaseStopDateTime);
}

Cerner::Foundations::Calendar CSubPhaseScheduleManager::CalculateSubPhaseStartDateTime(IPhase& subPhase,
		const Cerner::Foundations::Calendar& parentPhaseStartDateTime,
		const Cerner::Foundations::Calendar& parentPhaseTimeZeroDateTime) const
{
	IComponentPtr pISubPhaseComp = subPhase.GetSubphaseCompDispatch();

	if (NULL == pISubPhaseComp)
	{
		return parentPhaseStartDateTime;
	}

	const bool bIsTimeZeroActive = pISubPhaseComp->GetTZActiveInd() == TRUE;

	if (bIsTimeZeroActive)
	{
		Cerner::Foundations::Calendar actionDateTime;
		return CTimeZeroHelper::CalculateStartDateTimeFromTimeZeroInformation(*pISubPhaseComp, parentPhaseStartDateTime,
				parentPhaseTimeZeroDateTime);
	}

	const bool bHasComponentOffset = pISubPhaseComp->HasComponentOffset() == TRUE;

	if (bHasComponentOffset)
	{
		return CComponentOffsetHelper::AddComponentOffset(*pISubPhaseComp, parentPhaseStartDateTime);
	}

	return parentPhaseStartDateTime;
}

Cerner::Foundations::Calendar CSubPhaseScheduleManager::CalculateSubPhaseStopDateTime(IPhase& subPhase,
		const Cerner::Foundations::Calendar& subPhaseStartDateTime,
		const Cerner::Foundations::Calendar& parentPhaseStopDateTime) const
{
	const long lDuration = subPhase.GetDuration();
	const double dDurationUnitCd = subPhase.GetDurationUnitCd();

	if (lDuration > 0 && dDurationUnitCd > 0.0)
	{
		CString sDurationUnitMeaning = (LPCTSTR) subPhase.GetDurationUnitMean();

		if (sDurationUnitMeaning == _T("DAYS"))
		{
			return subPhaseStartDateTime.AddDays(lDuration);
		}
		else if (sDurationUnitMeaning == _T("HOURS"))
		{
			return subPhaseStartDateTime.AddHours(lDuration);
		}
		else if (sDurationUnitMeaning == _T("MINUTES"))
		{
			return subPhaseStartDateTime.AddMinutes(lDuration);
		}
	}

	if (!parentPhaseStopDateTime.IsNull())
	{
		if (parentPhaseStopDateTime >= subPhaseStartDateTime)
		{
			return parentPhaseStopDateTime;
		}
	}

	return Cerner::Foundations::Calendar::CreateNull();
}