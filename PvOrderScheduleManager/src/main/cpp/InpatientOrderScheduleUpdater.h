#pragma once

#include <CalculateNewOrderTriggeringActionFlag.h>
#include <CalculateModifyOrderTriggeringActionFlag.h>
#include <CalculateRenewOrderTriggeringActionFlag.h>
#include <CalculateActivateOrderTriggeringActionFlag.h>
#include <CalculateResumeOrderTriggeringActionFlag.h>

// Forward Declarations
struct IComponent;
class PvOrderObj;

class CInpatientOrderScheduleUpdater
{
public:
	CInpatientOrderScheduleUpdater(const HPATCON hPatCon);

	bool UpdateOrderScheduleOnInitialAction(PvOrderObj& orderObj);
	bool UpdateOrderScheduleOnAddOrder(PvOrderObj& orderObj);
	bool UpdateOrderScheduleOnInitialModify(PvOrderObj& orderObj);
	bool UpdateOrderScheduleOnInitialActivate(PvOrderObj& orderObj);
	bool UpdateOrderScheduleOnInitialRenew(PvOrderObj& orderObj);
	bool UpdateOrderScheduleOnInitialResume(PvOrderObj& orderObj);

	bool UpdateOrderScheduleOnRequestedStartChange(PvOrderObj& orderObj);
	bool UpdateOrderScheduleOnReferenceStartChange(PvOrderObj& orderObj);
	bool UpdateOrderScheduleOnPRNChange(PvOrderObj& orderObj);
	bool UpdateOrderScheduleOnGenericPriorityChange(PvOrderObj& orderObj);
	bool UpdateOrderScheduleOnCollectionPriorityChange(PvOrderObj& orderObj);
	bool UpdateOrderScheduleOnPharmacyPriorityChange(PvOrderObj& orderObj);
	bool UpdateOrderScheduleOnDurationChange(PvOrderObj& orderObj);
	bool UpdateOrderScheduleOnFrequencyChange(PvOrderObj& orderObj);
	bool UpdateOrderScheduleOnInfuseOverChange(PvOrderObj& orderObj);
	bool UpdateOrderScheduleOnStopChange(PvOrderObj& orderObj);
	bool UpdateOrderScheduleOnSchedulingInstructionsChange(PvOrderObj& orderObj);
	bool UpdateOrderScheduleOnFutureChange(PvOrderObj& orderObj);
	bool UpdateOrderScheduleOnConstantChange(PvOrderObj& orderObj);
	bool UpdateOrderScheduleOnMoveDose(PvOrderObj& orderObj);
	bool UpdateOrderScheduleOnAcceptProposalAction(PvOrderObj& orderObj);
	bool UpdateOrderScheduleOnDiscernModify(PvOrderObj& orderObj);

private:
	bool IsPlanOrder(PvOrderObj& orderObj);
	IComponent* GetPlanComponent(PvOrderObj& orderObj);
	bool CanScheduleBeCalculated(PvOrderObj& orderObj);

	bool UpdateOrderScheduleOnAcceptProposalOrder(PvOrderObj& orderObj);
	bool UpdateOrderScheduleOnAcceptProposalModify(PvOrderObj& orderObj);
	bool UpdateOrderScheduleOnAcceptProposalResume(PvOrderObj& orderObj);
	bool UpdateOrderScheduleOnAcceptProposalRenew(PvOrderObj& orderObj);

private:
	const HPATCON m_hPatCon;
};