#pragma once

#include <orderdetail.h>

// Forward Declarations
struct IPhase;
struct IComponent;
struct IOutcome;
class PvOrderObj;

/////////////////////////////////////////////////////////////////////////////
///\class	CPlanScheduleManager PlanScheduleManager.h
///\brief	This class acts as a public facing interface to all of the plan schedule calculation classes.
///			All functions in this class should be little more than pass-throughs to the specific class that actually does the schedule calculation.
///			It is the author's intention that, in the future, when Rx schedule calculation is done in a service, that all plan schedule logic should
///			be moved to its own DLL, and that this class be its only export.
/////////////////////////////////////////////////////////////////////////////
class CPlanScheduleManager
{
public:
	CPlanScheduleManager(const HPATCON hPatCon);

	// Order Schedule
	bool UpdateOrderScheduleOnActivateOrder(IComponent& component, PvOrderObj& orderObj);
	bool UpdateOrderScheduleOnInitialModify(PvOrderObj& orderObj);
	bool UpdateOrderScheduleOnRequestedStartDateTimeChange(PvOrderObj& orderObj);
	bool UpdateOrderScheduleOnReferenceStartDateTimeChange(PvOrderObj& orderObj);
	bool UpdateOrderScheduleOnStopDateTimeChange(PvOrderObj& orderObj);
	bool UpdateOrderScheduleOnFutureIndicatorChange(PvOrderObj& orderObj);
	bool UpdateOrderScheduleOnFrequencyChange(PvOrderObj& orderObj);
	bool UpdateOrderScheduleOnGenericPriorityChange(PvOrderObj& orderObj);
	bool UpdateOrderScheduleOnCollectionPriorityChange(PvOrderObj& orderObj);
	bool UpdateOrderScheduleOnPharmacyPriorityChange(PvOrderObj& orderObj);
	bool UpdateOrderScheduleOnDurationChange(PvOrderObj& orderObj);
	bool UpdateOrderScheduleOnPRNChange(PvOrderObj& orderObj);
	bool UpdateOrderScheduleOnSchedulingInstructionsChange(PvOrderObj& orderObj);
	bool UpdateOrderScheduleOnInfuseOverChange(PvOrderObj& orderObj);
	bool UpdateOrderScheduleOnConstantIndicatorChange(PvOrderObj& orderObj);

	// Component Schedule
	bool UpdateScheduleOnComponentOffsetChange(IComponent& component);

	// Outcome Schedule
	void UpdateOutcomeSchedule(IPhase& phase, IComponent& component, IOutcome& outcome);

	// Phase Schedule
	void UpdatePhaseScheduleOnInitiate(IPhase& phase);
	void UpdatePhaseScheduleOnActivate(IPhase& phase);
	void UpdatePhaseScheduleOnChangePhaseStart(IPhase& phase, const bool bAdjustSchedulableOrder);
	void UpdatePhaseScheduleOnChangePhaseTimeZero(IPhase& phase);
	void UpdatePhaseScheduleOnChangePhaseStop(IPhase& phase, const std::set<double>& setComponentIds);
	void UpdatePhaseScheduleOnIncludeComponent(IPhase& phase, IComponent& component);
	void UpdatePhaseScheduleOnExcludeComponent(IPhase& phase, IComponent& component);

private:
	const HPATCON m_hPatCon;
};