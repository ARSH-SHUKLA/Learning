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

class CInitiatePhaseScheduleManager
{
public:
	CInitiatePhaseScheduleManager(const HPATCON hPatCon);

	void UpdatePhaseScheduleOnInitiate(IPhase& phase);

private:
	void UpdatePhaseScheduleOnInitiateInternal(IPhase& phase);
	void UpdateComponentSchedulesOnInitiate(const std::list<IComponent*>& components, std::list<PvOrderObj*>& orders,
											const Cerner::Foundations::Calendar& phaseStartDateTime, const Cerner::Foundations::Calendar& phaseTimeZeroDateTime,
											const Cerner::Foundations::Calendar& phaseStopDateTime);
	void UpdatePrescriptionComponentSchedulesOnInitiate(const std::list<IComponent*>& prescriptionComponents,
			const Cerner::Foundations::Calendar& phaseStartDateTime);
	void UpdateOutcomeComponentSchedulesOnInitiate(IPhase& phase, const std::list<IComponent*>& components);

private:
	const HPATCON m_hPatCon;
};