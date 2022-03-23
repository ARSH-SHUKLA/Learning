#include "StdAfx.h"


#include "OutcomeScheduleManager.h"
#include "ComponentOffsetHelper.h"

#include <pvcdf340.h>
#include <PvOrdCalUtil.h>
#include <CPS_ImportPVCareCoordCom.h>
#include <PvGenericLoaderExtended.h>
#include <Pvordencounterutil.h>

COutcomeScheduleManager::COutcomeScheduleManager(const HPATCON hPatCon)
	: m_hPatCon(hPatCon)
{

}

void COutcomeScheduleManager::UpdateOutcomeSchedule(IPhase& phase, IComponent& component, IOutcome& outcome)
{
	const bool bDetailsLoaded = outcome.GetDetailsLoaded() == TRUE;

	if (!bDetailsLoaded)
	{
		BOOL bLoadDetailsResult = CPvGenericLoaderExtended(m_hPatCon).LoadOutcomeDetails(&outcome);

		if (bLoadDetailsResult == FALSE)
		{
			return;
		}
	}

	double dPatientId = phase.GetPersonId();
	double dEncounterId = phase.GetEncntrId();
	ICalendarPtr pNow = GetCurrentDateTime(dPatientId, dEncounterId);

	ICalendarPtr pIPhaseStartDateTime = phase.GetUTCStartDtTm();
	const Cerner::Foundations::Calendar phaseStartDateTime = PvOrdCalUtils::GetDateOnlyCalendarFromInterface(
				pIPhaseStartDateTime);

	phase.CalculateEndDtTm();
	ICalendarPtr pIPhaseStopDateTime = phase.GetUTCCalcEndDtTm();
	const Cerner::Foundations::Calendar phaseStopDateTime = PvOrdCalUtils::GetDateOnlyCalendarFromInterface(
				pIPhaseStopDateTime);

	// Start Date/Time
	Cerner::Foundations::Calendar startDateTime = CComponentOffsetHelper::AddComponentOffset(component, phaseStartDateTime);
	Cerner::Foundations::Calendar currentDateTime = PvOrdCalUtils::GetDateOnlyCalendarFromInterface(pNow);

	// Update the outcome start date/time to current date/time
	if (startDateTime.Compare(currentDateTime) < 0 && component.GetProActiveInd() == FALSE)
	{
		startDateTime = currentDateTime;
		outcome.PutUTCStartDtTm(pNow);
	}
	else
	{
		ICalendarPtr pIStartDateTime(__uuidof(Calendar));
		pIStartDateTime->InitFromHandle(startDateTime.GetHandle());
		outcome.PutUTCStartDtTm(pIStartDateTime);
	}

	 CPvGenericLoaderExtended(m_hPatCon).PrepareOutcomeForUpdate(&outcome);

	// Outcomes dirty flag need to be set here since we are updating outcome schedule.
	// This will make sure to save the changes with outcomes write transaction (601520).
	outcome.PutDirtyFlag(TRUE);
	
	if (outcome.GetActionType() == eActionNone && component.GetProActiveInd() == TRUE)
	{
		// Mark the outcome as new order if doesnt have a status code.
		if (outcome.GetStatusCd() == 0)
		{
			outcome.PutActionType(eActionActOrder);
		}
		// Mark the outcome as modified.
		else
		{
			outcome.PutActionType(eActionActModify);
		}
	}

	const bool bIsDoTComponent = component.IsDoTComponent() == TRUE;

	if (bIsDoTComponent)
	{
		CalculateAndSetStopDateTimeForTreatmentPeriodOutcome(phase, component, outcome, startDateTime, phaseStartDateTime,
				phaseStopDateTime);
	}
	else
	{
		CalculateAndSetStopDateTime(component, outcome, startDateTime, phaseStartDateTime, phaseStopDateTime);
	}
}

void COutcomeScheduleManager::CalculateAndSetStopDateTimeForTreatmentPeriodOutcome(IPhase& phase, IComponent& component,
		IOutcome& outcome, const Cerner::Foundations::Calendar& outcomeStartDateTime,
		const Cerner::Foundations::Calendar& phaseStartDateTime, const Cerner::Foundations::Calendar& phaseStopDateTime)
{
	const bool bIsProActive = component.GetProActiveInd() == TRUE;

	if (!bIsProActive)
	{
		// Note: CalculateAndSetStopDateTime will unlink the component from phase duration if it has a duration.
		component.PutLinkToPhase(TRUE);
	}

	CalculateAndSetStopDateTime(component, outcome, outcomeStartDateTime, phaseStartDateTime, phaseStopDateTime);

	if (!phaseStopDateTime.IsNull())
	{
		// If for some bizarre reason the outcome start date/time is past the phase end (possibly because of a large
		// start offset) then set the outcome start to the phase end and make the start offset equal to the phase
		// duration.
		if (!outcomeStartDateTime.IsNull() && outcomeStartDateTime.Compare(phaseStopDateTime) >= 0)
		{
			ICalendarPtr pIStopDateTime(__uuidof(Calendar));
			pIStopDateTime->InitFromHandle(phaseStartDateTime.GetHandle());
			outcome.PutUTCStartDtTm(pIStopDateTime);

			const long lDuration = phase.GetDuration();
			const double dDurationUnitCd = phase.GetDurationUnitCd();

			if (lDuration > 0 && dDurationUnitCd > 0.0)
			{
				component.PutStartOffsetQty(lDuration);
				component.PutStartOffsetUnitCd(dDurationUnitCd);
				component.PutStartOffsetUnitDisp((_bstr_t)CDF::AgeUnitCodeset::GetDisplay(dDurationUnitCd));
				component.PutStartOffsetUnitMean((_bstr_t)CDF::AgeUnitCodeset::GetMeaning(dDurationUnitCd));
			}
		}

		// Change the outcome end date/time if it falls after the phase end date/time.
		ICalendarPtr pIOutcomeEndDateTime = outcome.GetUTCEndDtTm();
		const Cerner::Foundations::Calendar outcomeStopDateTime = PvOrdCalUtils::GetDateOnlyCalendarFromInterface(
					pIOutcomeEndDateTime);

		// If the outcome end date/time falls on or after the treatment period end then use the treatment period end date/time
		// and link the outcome to phase duration
		if (!outcomeStopDateTime.IsNull() && outcomeStopDateTime.Compare(phaseStopDateTime) >= 0)
		{
			ICalendarPtr pIPhaseEndDtTm(__uuidof(Calendar));
			pIPhaseEndDtTm->InitFromHandle(phaseStopDateTime.GetHandle());

			outcome.PutUTCEndDtTm(pIPhaseEndDtTm);
			outcome.PutDurationQty(0);
			outcome.PutDurationUnitCd(0.0);
			outcome.PutDurationUnitDisp((_bstr_t)_T(""));
			outcome.PutDurationUnitMean((_bstr_t)_T(""));
			outcome.PutTargetDurationQty(0);
			outcome.PutTargetDurationUnitCd(0.0);
			outcome.PutTargetDurationUnitCdDisp((_bstr_t)_T(""));
			outcome.PutTargetDurationUnitCdMean((_bstr_t)_T(""));

			component.PutLinkToPhase(TRUE);
			component.PutDurationQty(0);
			component.PutDurationUnitCd(0.0);
			component.PutDurationUnitDisp((_bstr_t)_T(""));
			component.PutDurationUnitMean((_bstr_t)_T(""));
		}
	}
}

void COutcomeScheduleManager::CalculateAndSetStopDateTime(IComponent& component, IOutcome& outcome,
		const Cerner::Foundations::Calendar& outcomeStartDateTime, const Cerner::Foundations::Calendar& phaseStartDateTime,
		const Cerner::Foundations::Calendar& phaseStopDateTime)
{
	long lDurationQty = outcome.GetDurationQty();
	double dDurationUnitCd = outcome.GetDurationUnitCd();

	if (dDurationUnitCd <= 0.0)
	{
		lDurationQty = component.GetDurationQty();
		dDurationUnitCd = component.GetDurationUnitCd();
	}

	if (dDurationUnitCd > 0.0)
	{
		component.PutLinkToPhase(FALSE);

		Cerner::Foundations::Calendar stopDateTime = AddDurationToStartDateTime(outcomeStartDateTime, lDurationQty,
				dDurationUnitCd);
		ICalendarPtr pIStopDateTime(__uuidof(Calendar));
		pIStopDateTime->InitFromHandle(stopDateTime.GetHandle());
		outcome.PutUTCEndDtTm(pIStopDateTime);

		return;
	}

	const bool bLinkedToPhase = component.GetLinkToPhase() == TRUE;

	if (bLinkedToPhase)
	{
		ICalendarPtr pIStopDateTime(__uuidof(Calendar));
		pIStopDateTime->InitFromHandle(phaseStopDateTime.GetHandle());
		outcome.PutUTCEndDtTm(pIStopDateTime);

		return;
	}

	const double dOffsetQty = component.GetStartOffsetQty();

	if (dOffsetQty < 0.0)
	{
		ICalendarPtr pIStopDateTime(__uuidof(Calendar));
		pIStopDateTime->InitFromHandle(phaseStartDateTime.GetHandle());
		outcome.PutUTCEndDtTm(pIStopDateTime);
	}
}

Cerner::Foundations::Calendar COutcomeScheduleManager::AddDurationToStartDateTime(Cerner::Foundations::Calendar
		startDateTime, const long lDuration, const double dDurationUnitCd)
{
	if (CDF::AgeUnitCodeset::IsDays(dDurationUnitCd))
	{
		return startDateTime.AddDays(lDuration);
	}
	else if (CDF::AgeUnitCodeset::IsHours(dDurationUnitCd))
	{
		return startDateTime.AddHours(lDuration);
	}
	else if (CDF::AgeUnitCodeset::IsMinutes(dDurationUnitCd))
	{
		return startDateTime.AddMinutes(lDuration);
	}

	return startDateTime;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// \fn			ICalendarPtr GetCurrentDateTime(double personId, double enctrId)
/// \brief		Returns the current date/time configured withe the given patient's time zone.
///
/// \return     pNow - CalenderPtr returns the current date time.
///
/// \param[in]	dPatientId   - Patient Id.
///             dEncounterId - Encounter Id used to get the timezone.
///
/// \owner      RP049736
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ICalendarPtr COutcomeScheduleManager::GetCurrentDateTime(double dPatientId, double dEncounterId)
{
	ITimeZonePtr pIPatTimeZone(__uuidof(TimeZone));

	size_t iPatientTimeZoneIndex(Pvordencounterutil::GetTimeZone(dPatientId, dEncounterId));
	pIPatTimeZone->InitFromIndexWithDefault(iPatientTimeZoneIndex, VARIANT_TRUE);

	ICalendarPtr pNow(__uuidof(Calendar));
	pNow->Init(SRV_CALENDAR_TYPE_DEFAULT);
	pNow = pNow->ToUTC();
	pNow = pNow->RollSeconds(-static_cast<int>(pNow->GetSecond()));
	pNow = pNow->RollMilliseconds(-static_cast<int>(pNow->GetMillisecond()));
	pNow = pNow->ToTimeZone(pIPatTimeZone);
	return pNow;
}