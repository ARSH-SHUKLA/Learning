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

class CIncludeComponentScheduleManager
{
public:
	CIncludeComponentScheduleManager(const HPATCON hPatCon);

	void UpdatePhaseScheduleOnIncludeComponent(IPhase& phase, IComponent& component);

private:
	bool UpdatePhaseScheduleOnIncludeComponentInternal(IPhase& phase, IComponent& component);
	void UpdateTimeZeroLinkedComponentSchedulesOnIncludeMostNegativeComponent(IPhase& phase, IComponent& includedComponent);
	void UpdateIncludedComponentSchedule(IPhase& phase, IComponent& includedComponent,
										 const Cerner::Foundations::Calendar& phaseStartDateTime, const Cerner::Foundations::Calendar& phaseTimeZeroDateTime,
										 const Cerner::Foundations::Calendar& phaseStopDateTime, std::list<PvOrderObj*>& includedOrders);
	void UpdateLinkedSubPhaseSchedule(IPhase& subPhase, const Cerner::Foundations::Calendar& phaseStartDateTime,
									  const Cerner::Foundations::Calendar& phaseTimeZeroDateTime, const Cerner::Foundations::Calendar& phaseStopDateTime,
									  std::list<PvOrderObj*>& orders);
	void UpdateIncludedSubPhaseSchedule(IPhase& subPhase, const Cerner::Foundations::Calendar& phaseStartDateTime,
										const Cerner::Foundations::Calendar& phaseTimeZeroDateTime, const Cerner::Foundations::Calendar& phaseStopDateTime,
										std::list<PvOrderObj*>& orders);

	void CallInpatientOrdersScheduleService(std::list<PvOrderObj*>& includedOrders, std::list<PvOrderObj*>& linkedOrders);

	PvOrderObj* GetIncludedOrderWithUpdatedSchedule(IComponent& orderComponent,
			const Cerner::Foundations::Calendar& phaseStartDateTime, const Cerner::Foundations::Calendar& phaseTimeZeroDateTime,
			const Cerner::Foundations::Calendar& phaseStopDateTime);
	PvOrderObj* GetLinkedOrderWithUpdatedSchedule(IComponent& orderComponent,
			const Cerner::Foundations::Calendar& phaseStartDateTime, const Cerner::Foundations::Calendar& phaseTimeZeroDateTime,
			const Cerner::Foundations::Calendar& phaseStopDateTime);

	void UpdatePrescriptionComponentScheduleOnInclude(IComponent& prescriptionComponent,
			const Cerner::Foundations::Calendar& phaseStartDateTime);
	void UpdateOutcomeComponentScheduleOnInclude(IPhase& phase, IComponent& outcomeComponent);

	void DisplayUnlinkFromTimeZeroMessage(IComponent& component);
	CString GetComponentDescription(IComponent& component);

private:
	const HPATCON m_hPatCon;
};