#pragma once

#include <list>
#include <CalculateActivateOrderScheduleCriteria.h>

// Forward Declarations
struct IPhase;
struct IComponent;
class PvOrderObj;

namespace Cerner
{
	namespace Foundations
	{
		class Calendar;
	}
}

class CActivatePhaseScheduleManager
{
public:
	CActivatePhaseScheduleManager(const HPATCON hPatCon);

	void UpdatePhaseScheduleOnActivate(IPhase& phase);

private:
	void UpdateComponentSchedulesOnActivate(const std::list<IComponent*>& components,
											std::list<CCalculateActivateOrderScheduleCriteria>& activateOrderScheduleCriteria,
											const Cerner::Foundations::Calendar& phaseStartDateTime, const Cerner::Foundations::Calendar& phaseTimeZeroDateTime,
											const Cerner::Foundations::Calendar& phaseStopDateTime);
	void UpdatePrescriptionComponentSchedulesOnActivate(const std::list<IComponent*>& prescriptionComponents,
			const Cerner::Foundations::Calendar& phaseStartDateTime);
	PvOrderObj* GetFutureRecurringChildOrderBeingActivated(PvOrderObj& futureRecurringOrder);

private:
	const HPATCON m_hPatCon;
};