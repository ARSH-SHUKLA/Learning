#pragma once

#include <srvcalendar.h>

// Forward Declarations
struct IPhase;
struct IComponent;
struct IOutcome;

class COutcomeScheduleManager
{
public:
	COutcomeScheduleManager(const HPATCON hPatCon);

	void UpdateOutcomeSchedule(IPhase& phase, IComponent& component, IOutcome& outcome);

private:
	void CalculateAndSetStopDateTimeForTreatmentPeriodOutcome(IPhase& phase, IComponent& component, IOutcome& outcome,
			const Cerner::Foundations::Calendar& outcomeStartDateTime, const Cerner::Foundations::Calendar& phaseStartDateTime,
			const Cerner::Foundations::Calendar& phaseStopDateTime);
	void CalculateAndSetStopDateTime(IComponent& component, IOutcome& outcome,
									 const Cerner::Foundations::Calendar& outcomeStartDateTime, const Cerner::Foundations::Calendar& phaseStartDateTime,
									 const Cerner::Foundations::Calendar& phaseStopDateTime);

	Cerner::Foundations::Calendar AddDurationToStartDateTime(Cerner::Foundations::Calendar startDateTime,
			const long lDuration, const double dDurationUnitCd);
	ICalendarPtr GetCurrentDateTime(double dPatientId, double dEncounterId);

private:
	const HPATCON m_hPatCon;
};