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

class CChangePhaseStartDateTimeManager
{
public:
	CChangePhaseStartDateTimeManager(const HPATCON hPatCon);

	void UpdatePhaseScheduleOnChangePhaseStart(IPhase& phase, const bool bAdjustSchedulableOrder);

private:
	void UpdateComponentScheduleOnChangePhaseStart(IPhase& phase, IComponent& component,
			std::list<PvOrderObj*>& inpatientOrders, const Cerner::Foundations::Calendar& phaseStartDateTime,
			const Cerner::Foundations::Calendar& phaseTimeZeroDateTime, const Cerner::Foundations::Calendar& phaseStopDateTime);
	void AdjustSchedulableOrder(IPhase& phase, const Cerner::Foundations::Calendar& phaseStartDateTime);

	PvOrderObj* GetUpdatedOrderOnChangePhaseStart(IComponent& orderComponent,
			const Cerner::Foundations::Calendar& phaseStartDateTime, const Cerner::Foundations::Calendar& phaseTimeZeroDateTime,
			const Cerner::Foundations::Calendar& phaseStopDateTime);
	void UpdatePrescriptionComponentScheduleOnChangePhaseStart(IComponent& prescriptionComponent,
			const Cerner::Foundations::Calendar& phaseStartDateTime);
	void UpdateOutcomeComponentScheduleOnChangePhaseStart(IPhase& phase, IComponent& outcomeComponent);
	void UpdateSubPhaseScheduleOnChangePhaseStart(IPhase& subPhase, std::list<PvOrderObj*>& inpatientOrders,
			const Cerner::Foundations::Calendar& phaseStartDateTime, const Cerner::Foundations::Calendar& phaseTimeZeroDateTime,
			const Cerner::Foundations::Calendar& phaseStopDateTime);

	void CallInpatientOrderScheduleService(std::list<PvOrderObj*>& orders);
	void CallInpatientOrderScheduleServiceForSchedulableOrder(PvOrderObj* pSchedulableOrder);

private:
	const HPATCON m_hPatCon;
};