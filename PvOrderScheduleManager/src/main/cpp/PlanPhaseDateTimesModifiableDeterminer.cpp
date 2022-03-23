#include "StdAfx.h"

#include "PlanPhaseDateTimesModifiableDeterminer.h"

#include <srvcalendar.h>
#include <PvOrdCalUtil.h>
#include <CPS_ImportPVCareCoordCom.h>

bool CPlanPhaseDateTimesModifiableDeterminer::ArePhaseDateTimesModifiable(IPhase& phase) const
{
	const CString sCalcPlanStatusMean = (LPCTSTR)phase.GetCalcPlanStatusMean();

	ICalendarPtr pIPhaseStartDateTime = phase.GetUTCStartDtTm();
	const Cerner::Foundations::Calendar phaseStartDateTime = PvOrdCalUtils::GetDateOnlyCalendarFromInterface(
				pIPhaseStartDateTime);

	if (!phaseStartDateTime.IsNull())
	{
		if (phase.GetInitiated() == TRUE)
		{
			const long lPhaseStatusMask = phase.GetStatus();

			if (lPhaseStatusMask & ePhaseStatusPending)
			{
				return true;
			}

			// PowerPlans still allows you to modify the phase start date/time when the current phase start date/time is in the past, as long as it's not more than 5 minutes in the past.
			Cerner::Foundations::Calendar currentDateTime;
			Cerner::Foundations::Calendar lapsedDateTime = currentDateTime.AddMinutes(-5);

			if (phaseStartDateTime.Compare(lapsedDateTime) < 0)
			{
				const bool bFuturePlanStatus = sCalcPlanStatusMean == _T("FUTURE") || sCalcPlanStatusMean == _T("FUTUREREVIEW");

				if (!bFuturePlanStatus)
				{
					return false;
				}
			}
		}
	}

	return true;
}