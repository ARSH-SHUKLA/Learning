#pragma once

#include <list>

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

class CExcludeComponentScheduleManager
{
public:
	CExcludeComponentScheduleManager(const HPATCON hPatCon);

	void UpdatePhaseScheduleOnExcludeComponent(IPhase& phase, IComponent& component);

private:
	void UpdatePhaseScheduleOnExcludeComponentInternal(IPhase& phase, IComponent& component);
	void UpdateTimeZeroLinkedComponentSchedules(IPhase& phase, const bool bTimeZeroOrderExcluded);
	void CallInpatientOrdersScheduleService(std::list<PvOrderObj*>& affectedOrders);
	void UpdateLinkedSubPhaseSchedule(IPhase& subPhase, const Cerner::Foundations::Calendar& phaseStartDateTime,
									  const Cerner::Foundations::Calendar& phaseTimeZeroDateTime, const Cerner::Foundations::Calendar& phaseStopDateTime,
									  std::list<PvOrderObj*>& orders);

	PvOrderObj* GetLinkedOrderWithUpdatedSchedule(IComponent& orderComponent,
			const Cerner::Foundations::Calendar& phaseStartDateTime, const Cerner::Foundations::Calendar& phaseTimeZeroDateTime,
			const Cerner::Foundations::Calendar& phaseStopDateTime);
	void UpdateLinkedPrescriptionComponentSchedule(IComponent& prescriptionComponent,
			const Cerner::Foundations::Calendar& phaseStartDateTime);
	void UpdateLinkedOutcomeComponentSchedule(IPhase& phase, IComponent& outcomeComponent);

private:
	const HPATCON m_hPatCon;
};