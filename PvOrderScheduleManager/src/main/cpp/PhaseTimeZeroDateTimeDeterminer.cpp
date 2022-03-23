#include "StdAfx.h"

#include "PhaseTimeZeroDateTimeDeterminer.h"

#include <CPS_ImportPVCareCoordCom.h>

ICalendarPtr CPhaseTimeZeroDateTimeDeterminer::DetermineTimeZeroDateTime(IPhase& phase) const
{
	IPlan* pIPlan = PowerPlanUtil::GetPlan(&phase);

	if (pIPlan == NULL)
	{
		return NULL;
	}

	ICalendarPtr pIStartDateTime = phase.GetUTCStartDtTm();

	if (NULL == pIStartDateTime)
	{
		return NULL;
	}

	IComponentPtr pITimeZeroComponent = phase.FindTZComponent();

	if (NULL != pITimeZeroComponent)
	{
		const bool bTimeZeroActive = pITimeZeroComponent->GetTZActiveInd() == TRUE;

		if (bTimeZeroActive)
		{
			const bool bComponentIncluded = pITimeZeroComponent->GetIncluded() == TRUE;

			if (bComponentIncluded)
			{
				const double dEarliestOffsetInMinutes = pIPlan->GetEarliestOffsetInMinutes(&phase);
				return pIStartDateTime->AddMinutes((long)fabs(dEarliestOffsetInMinutes));
			}
		}
	}

	return NULL;
}