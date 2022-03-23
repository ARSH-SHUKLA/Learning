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

class CChangePhaseStopDateTimeManager
{
public:
	CChangePhaseStopDateTimeManager(const HPATCON hPatCon);

	void UpdatePhaseScheduleOnChangePhaseStop(IPhase& phase, const std::set<double>& setComponentIds);

private:
	void UpdateComponentScheduleOnChangePhaseStop(IPhase& phase, IComponent& component,
			std::list<PvOrderObj*>& inpatientOrders, const Cerner::Foundations::Calendar& phaseStopDateTime,
			const std::set<double>& setComponentIds);

	PvOrderObj* GetUpdatedOrderOnChangePhaseStop(IComponent& orderComponent,
			const Cerner::Foundations::Calendar& phaseStopDateTime);
	void UpdateOutcomeComponentScheduleOnChangePhaseStop(IPhase& phase, IComponent& outcomeComponent);

	void CallInpatientOrderScheduleService(std::list<PvOrderObj*>& orders);

private:
	const HPATCON m_hPatCon;
};