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

class CChangePhaseTimeZeroDateTimeManager
{
public:
	CChangePhaseTimeZeroDateTimeManager(const HPATCON hPatCon);

	void UpdatePhaseScheduleOnChangePhaseTimeZero(IPhase& phase);

private:
	void UpdateComponentSchedulesOnChangePhaseTimeZero(const std::list<IComponent*>& components,
			std::list<PvOrderObj*>& newOrders, std::list<PvOrderObj*>& modifyOrders, std::list<PvOrderObj*>& activateOrders,
			std::list<PvOrderObj*>& resumeOrders, const Cerner::Foundations::Calendar& phaseStartDateTime,
			const Cerner::Foundations::Calendar& phaseTimeZeroDateTime, const Cerner::Foundations::Calendar& phaseStopDateTime);

	void CallInpatientOrdersScheduleService(std::list<PvOrderObj*>& timeZeroOrders);

	void UpdateSubPhaseSchedule(IPhase& subPhase, const Cerner::Foundations::Calendar& phaseStartDateTime,
								const Cerner::Foundations::Calendar& phaseTimeZeroDateTime, const Cerner::Foundations::Calendar& phaseStopDateTime,
								std::list<PvOrderObj*>& orders);
	PvOrderObj* GetOrderWithUpdatedSchedule(IComponent& orderComponent,
											const Cerner::Foundations::Calendar& phaseStartDateTime, const Cerner::Foundations::Calendar& phaseTimeZeroDateTime,
											const Cerner::Foundations::Calendar& phaseStopDateTime);
	void UpdatePrescriptionComponentSchedule(IComponent& prescriptionComponent,
			const Cerner::Foundations::Calendar& phaseStartDateTime);
	void UpdateOutcomeComponentSchedule(IPhase& phase, IComponent& outcomeComponent);

private:
	const HPATCON m_hPatCon;
};