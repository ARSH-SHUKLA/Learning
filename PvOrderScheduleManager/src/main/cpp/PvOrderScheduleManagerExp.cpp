#include "stdafx.h"

#include "InpatientOrderScheduleUpdater.h"
#include "PlanScheduleManager.h"
#include "PvFrequencyScheduleManager.h"
#include "PhaseTimeZeroDateTimeDeterminer.h"
#include "PlanOrderDateTimesModifiableDeterminer.h"
#include "IntervalChildrenScheduleHelper.h"
#include "PharmacyPriorityHelper.h"
#include "DurationDisplayHelper.h"
#include "PriorityFieldHelper.h"
#include "PrnIndicatorDeterminer.h"
#include "PRNFlagUpdater.h"
#include "AdhocFrequencyDeterminer.h"
#include "protocolorderschedulemanager.h"

#include <PvOrderScheduleManagerExp.h>
#include <PvOrderScheduleManagerDefs.h>

CPvOrderScheduleManagerExp::CPvOrderScheduleManagerExp(const HPATCON hPatCon)
	: m_hPatCon(hPatCon)
{

}

// Frequency Manager
bool CPvOrderScheduleManagerExp::GetFreqSchedInfoByIDFromOrder(PvOrderObj& orderObj,
		PvOrderScheduleManager::FrequencyInfoStruct& freqStruct)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	return CPvFrequencyScheduleManager::GetInstance().GetFreqSchedInfoByIDFromOrder(orderObj, freqStruct);
}

bool CPvOrderScheduleManagerExp::GetFreqSchedInfoByCD(PvOrderObj& orderObj,
		PvOrderScheduleManager::FrequencyInfoStruct& freqStruct, PvOrderFld::OeFieldValue& freqCDVal, double dEncounterId,
		bool& bIsCustomFreq)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	return CPvFrequencyScheduleManager::GetInstance().GetFreqSchedInfoByCD(orderObj, freqStruct, freqCDVal, dEncounterId,
			bIsCustomFreq);
}

void CPvOrderScheduleManagerExp::AddFreqSchedInfo(PvOrderScheduleManager::FrequencyInfoStruct& freqStruct)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	CPvFrequencyScheduleManager::GetInstance().AddFreqSchedInfo(freqStruct);
}

bool CPvOrderScheduleManagerExp::IsAdHocFrequency(const double dFrequencyId)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	return CAdhocFrequencyDeterminer().LoadAdhocIndFromFreqId(dFrequencyId);
}


// Initial Actions
bool CPvOrderScheduleManagerExp::UpdateOrderScheduleOnInitialAction(PvOrderObj& orderObj)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	return CInpatientOrderScheduleUpdater(m_hPatCon).UpdateOrderScheduleOnInitialAction(orderObj);
}

bool CPvOrderScheduleManagerExp::UpdateOrderScheduleOnAddOrder(PvOrderObj& orderObj)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	return CInpatientOrderScheduleUpdater(m_hPatCon).UpdateOrderScheduleOnAddOrder(orderObj);
}

bool CPvOrderScheduleManagerExp::UpdateOrderScheduleOnInitialModify(PvOrderObj& orderObj)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	return CInpatientOrderScheduleUpdater(m_hPatCon).UpdateOrderScheduleOnInitialModify(orderObj);
}

bool CPvOrderScheduleManagerExp::UpdateOrderScheduleOnInitialActivate(PvOrderObj& orderObj)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	return CInpatientOrderScheduleUpdater(m_hPatCon).UpdateOrderScheduleOnInitialActivate(orderObj);
}

bool CPvOrderScheduleManagerExp::UpdateOrderScheduleOnInitialRenew(PvOrderObj& orderObj)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	return CInpatientOrderScheduleUpdater(m_hPatCon).UpdateOrderScheduleOnInitialRenew(orderObj);
}

bool CPvOrderScheduleManagerExp::UpdateOrderScheduleOnInitialResume(PvOrderObj& orderObj)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	return CInpatientOrderScheduleUpdater(m_hPatCon).UpdateOrderScheduleOnInitialResume(orderObj);
}

bool CPvOrderScheduleManagerExp::UpdateOrderScheduleOnActivatePlanOrder(IComponent& component, PvOrderObj& orderObj)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	return CPlanScheduleManager(m_hPatCon).UpdateOrderScheduleOnActivateOrder(component, orderObj);
}

// Detail Changes
bool CPvOrderScheduleManagerExp::UpdateOrderScheduleOnRequestedStartChange(PvOrderObj& orderObj)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	return CInpatientOrderScheduleUpdater(m_hPatCon).UpdateOrderScheduleOnRequestedStartChange(orderObj);
}

bool CPvOrderScheduleManagerExp::UpdateOrderScheduleOnReferenceStartChange(PvOrderObj& orderObj)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	return CInpatientOrderScheduleUpdater(m_hPatCon).UpdateOrderScheduleOnReferenceStartChange(orderObj);
}

bool CPvOrderScheduleManagerExp::UpdateOrderScheduleOnPRNChange(PvOrderObj& orderObj)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	return CInpatientOrderScheduleUpdater(m_hPatCon).UpdateOrderScheduleOnPRNChange(orderObj);
}

bool CPvOrderScheduleManagerExp::UpdateOrderScheduleOnGenericPriorityChange(PvOrderObj& orderObj)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	return CInpatientOrderScheduleUpdater(m_hPatCon).UpdateOrderScheduleOnGenericPriorityChange(orderObj);
}

bool CPvOrderScheduleManagerExp::UpdateOrderScheduleOnCollectionPriorityChange(PvOrderObj& orderObj)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	return CInpatientOrderScheduleUpdater(m_hPatCon).UpdateOrderScheduleOnCollectionPriorityChange(orderObj);
}

bool CPvOrderScheduleManagerExp::UpdateOrderScheduleOnPharmacyPriorityChange(PvOrderObj& orderObj)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	return CInpatientOrderScheduleUpdater(m_hPatCon).UpdateOrderScheduleOnPharmacyPriorityChange(orderObj);
}

bool CPvOrderScheduleManagerExp::UpdateOrderScheduleOnDurationChange(PvOrderObj& orderObj)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	return CInpatientOrderScheduleUpdater(m_hPatCon).UpdateOrderScheduleOnDurationChange(orderObj);
}

bool CPvOrderScheduleManagerExp::UpdateOrderScheduleOnFrequencyChange(PvOrderObj& orderObj)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	return CInpatientOrderScheduleUpdater(m_hPatCon).UpdateOrderScheduleOnFrequencyChange(orderObj);
}

bool CPvOrderScheduleManagerExp::UpdateOrderScheduleOnInfuseOverChange(PvOrderObj& orderObj)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	return CInpatientOrderScheduleUpdater(m_hPatCon).UpdateOrderScheduleOnInfuseOverChange(orderObj);
}

bool CPvOrderScheduleManagerExp::UpdateOrderScheduleOnStopChange(PvOrderObj& orderObj)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	return CInpatientOrderScheduleUpdater(m_hPatCon).UpdateOrderScheduleOnStopChange(orderObj);
}

bool CPvOrderScheduleManagerExp::UpdateOrderScheduleOnSchedulingInstructionsChange(PvOrderObj& orderObj)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	return CInpatientOrderScheduleUpdater(m_hPatCon).UpdateOrderScheduleOnSchedulingInstructionsChange(orderObj);
}

bool CPvOrderScheduleManagerExp::UpdateOrderScheduleOnFutureChange(PvOrderObj& orderObj)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	return CInpatientOrderScheduleUpdater(m_hPatCon).UpdateOrderScheduleOnFutureChange(orderObj);
}

bool CPvOrderScheduleManagerExp::UpdateOrderScheduleOnConstantChange(PvOrderObj& orderObj)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	return CInpatientOrderScheduleUpdater(m_hPatCon).UpdateOrderScheduleOnConstantChange(orderObj);
}

bool CPvOrderScheduleManagerExp::UpdateOrderScheduleOnMoveDose(PvOrderObj& orderObj)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	return CInpatientOrderScheduleUpdater(m_hPatCon).UpdateOrderScheduleOnMoveDose(orderObj);
}

bool CPvOrderScheduleManagerExp::UpdateOrderScheduleOnAcceptProposalAction(PvOrderObj& orderObj)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	return CInpatientOrderScheduleUpdater(m_hPatCon).UpdateOrderScheduleOnAcceptProposalAction(orderObj);
}

// Component Schedule
bool CPvOrderScheduleManagerExp::UpdateScheduleOnComponentOffsetChange(IComponent& component)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	return CPlanScheduleManager(m_hPatCon).UpdateScheduleOnComponentOffsetChange(component);
}

// Outcome Schedule
void CPvOrderScheduleManagerExp::UpdateOutcomeSchedule(IPhase& phase, IComponent& component, IOutcome& outcome)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	CPlanScheduleManager(m_hPatCon).UpdateOutcomeSchedule(phase, component, outcome);
}

// Phase Schedule
void CPvOrderScheduleManagerExp::UpdatePhaseScheduleOnInitiate(IPhase& phase)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	CPlanScheduleManager(m_hPatCon).UpdatePhaseScheduleOnInitiate(phase);
}

void CPvOrderScheduleManagerExp::UpdatePhaseScheduleOnActivate(IPhase& phase)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	CPlanScheduleManager(m_hPatCon).UpdatePhaseScheduleOnActivate(phase);
}

void CPvOrderScheduleManagerExp::UpdatePhaseScheduleOnChangePhaseStart(IPhase& phase,
		const bool bAdjustSchedulableOrder)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	CPlanScheduleManager(m_hPatCon).UpdatePhaseScheduleOnChangePhaseStart(phase, bAdjustSchedulableOrder);
}

void CPvOrderScheduleManagerExp::UpdatePhaseScheduleOnChangePhaseTimeZero(IPhase& phase)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	CPlanScheduleManager(m_hPatCon).UpdatePhaseScheduleOnChangePhaseTimeZero(phase);
}

void CPvOrderScheduleManagerExp::UpdatePhaseScheduleOnChangePhaseStop(IPhase& phase,
		const std::set<double>& setComponentIds)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	CPlanScheduleManager(m_hPatCon).UpdatePhaseScheduleOnChangePhaseStop(phase, setComponentIds);
}

void CPvOrderScheduleManagerExp::UpdatePhaseScheduleOnIncludeComponent(IPhase& phase, IComponent& component)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	CPlanScheduleManager(m_hPatCon).UpdatePhaseScheduleOnIncludeComponent(phase, component);
}

void CPvOrderScheduleManagerExp::UpdatePhaseScheduleOnExcludeComponent(IPhase& phase, IComponent& component)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	CPlanScheduleManager(m_hPatCon).UpdatePhaseScheduleOnExcludeComponent(phase, component);
}

// Helpers
bool CPvOrderScheduleManagerExp::IsOrderStartDateTimeModifiable(PvOrderObj& orderObj)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	return CPlanOrderDateTimesModifiableDeterminer().IsOrderStartDateTimeModifiable(orderObj);
}

ERelatedFldsStatus CPvOrderScheduleManagerExp::SetReqStartDtTmForIntervalChildren(PvOrderPCObj& orderPCObj)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	return CIntervalChildrenScheduleHelper(m_hPatCon).SetReqStartDtTmForIntervalChildren(orderPCObj);
}

ICalendarPtr CPvOrderScheduleManagerExp::DetermineTimeZeroDateTime(IPhase& phase)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	return CPhaseTimeZeroDateTimeDeterminer().DetermineTimeZeroDateTime(phase);
}

bool CPvOrderScheduleManagerExp::DoesMedOrderHavePharmacyPriorityOfStatOrNow(PvOrderObj& orderObj)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	return CPharmacyPriorityHelper::DoesMedOrderHavePharmacyPriorityOfStatOrNow(orderObj);
}

bool CPvOrderScheduleManagerExp::SetPharmacyPriorityToRoutineIfStatOrNow(PvOrderObj& orderObj)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	return CPharmacyPriorityHelper().SetPharmacyPriorityToRoutineIfStatOrNow(orderObj);
}

ERelatedFldsStatus CPvOrderScheduleManagerExp::SetDurationDisplay(PvOrderObj& orderObj)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	return CDurationDisplayHelper().SetDurationDisplay(orderObj);
}

void CPvOrderScheduleManagerExp::DisablePriorityFieldsForIncompatibleOrderTypes(PvOrderObj& orderObj)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	CPriorityFieldHelper::DisablePriorityFieldsForIncompatibleOrderTypes(orderObj);
}

bool CPvOrderScheduleManagerExp::IsPrnIndSetOnOrder(PvOrderObj& orderObj)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	return CPrnIndicatorDeterminer().IsPrnIndSetOnOrder(orderObj);
}

bool CPvOrderScheduleManagerExp::SetPRNFlagsForInpatientOrder(PvOrderObj& orderObj, const bool bSuppressWarning)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	return CPRNFlagUpdater().SetPRNFlagsForInpatientOrder(orderObj, bSuppressWarning);
}

void CPvOrderScheduleManagerExp::UpdateProtocolSchedule(PvOrderProtocolObj& protocolObj)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	CProtocolOrderScheduleManager(m_hPatCon).UpdateProtocolSchedule(protocolObj);
}

bool CPvOrderScheduleManagerExp::UpdateOrderScheduleOnDiscernModify(PvOrderObj& pOrderObj)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	return CInpatientOrderScheduleUpdater(m_hPatCon).UpdateOrderScheduleOnDiscernModify(pOrderObj);
}