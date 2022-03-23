#include "StdAfx.h"

#include "ExcludeComponentScheduleManager.h"
#include "IncludeOrderScheduleManager.h"
#include "PrescriptionScheduleManager.h"
#include "InpatientOrderScheduleServiceCaller.h"
#include "SubPhaseScheduleManager.h"
#include "ProtocolOrderScheduleManager.h"
#include "PhaseTimeZeroDateTimeDeterminer.h"
#include "OutcomeScheduleManager.h"
#include "IncludedComponentRetriever.h"
#include "ModifiableOrderRetriever.h"
#include "ChangePhaseStartOrderScheduleManager.h"
#include "MostNegativeTimeZeroOffsetDeterminer.h"
#include "PlanPhaseDateTimesModifiableDeterminer.h"
#include "DoTComponentRetriever.h"

#include <pvorderobj.h>
#include <PvOrdCalUtil.h>
#include <srvcalendar.h>
#include <CPS_ImportPVCareCoordCom.h>
#include <CPS_ImportCpsDataCommon.h>
#include <ActionDateTimeHelper.h>
#include <PvGenericLoaderExtended.h>

CExcludeComponentScheduleManager::CExcludeComponentScheduleManager(const HPATCON hPatCon)
	: m_hPatCon(hPatCon)
{

}

/////////////////////////////////////////////////////////////////////////////
/// \fn		void CIncludeComponentScheduleManager::UpdatePhaseScheduleOnIncludeComponent(IPhase& phase, IComponent& component)
/// \brief		Updates the schedule of the phase when necessary, as well as all other affected components in the phase.
///				This should be called anytime a component is excluded from a phase, except when the phase is in a planned status.
///
/// \return		void
///
/// \param[in]	IPhase& phase - The phase containing the component that was excluded.
/// \param[in]	IComponent& component - The component that was excluded from the phase.
/// \owner		DB024248
/////////////////////////////////////////////////////////////////////////////
void CExcludeComponentScheduleManager::UpdatePhaseScheduleOnExcludeComponent(IPhase& phase, IComponent& component)
{
	const bool bProtocolPhase = phase.HasTreatmentSchedule() == TRUE;

	if (bProtocolPhase)
	{
		std::list<IComponent*> treatmentPeriodComponents;
		CDoTComponentRetriever::FindDoTComponentsFromProtocolComponent(component, treatmentPeriodComponents);

		for (auto treatmentPeriodComponentIter = treatmentPeriodComponents.cbegin();
			 treatmentPeriodComponentIter != treatmentPeriodComponents.cend(); treatmentPeriodComponentIter++)
		{
			IComponent* pITreatmentPeriodComponent = *treatmentPeriodComponentIter;

			IPhase* pITreatmentPeriodPhase = PowerPlanUtil::GetPhase(pITreatmentPeriodComponent);

			if (NULL != pITreatmentPeriodPhase)
			{
				UpdatePhaseScheduleOnExcludeComponentInternal(*pITreatmentPeriodPhase, *pITreatmentPeriodComponent);
			}
		}
	}
	else
	{
		UpdatePhaseScheduleOnExcludeComponentInternal(phase, component);
	}
}

/////////////////////////////////////////////////////////////////////////////
/// \fn		void CIncludeComponentScheduleManager::UpdatePhaseScheduleOnExcludeComponentInternal(IPhase& phase, IComponent& component)
/// \brief		Updates the schedule of the phase when necessary, as well as all other affected components in the phase.
///				This should be called anytime a component is excluded from a phase, except when the phase is in a planned status.
///
/// \return		void
///
/// \param[in]	IPhase& phase - The phase containing the component that was excluded.
/// \param[in]	IComponent& component - The component that was excluded from the phase.
/// \owner		DB024248
/////////////////////////////////////////////////////////////////////////////
void CExcludeComponentScheduleManager::UpdatePhaseScheduleOnExcludeComponentInternal(IPhase& phase,
		IComponent& component)
{
	CPlanPhaseDateTimesModifiableDeterminer planPhaseDateTimesModifiableDeterminer;
	const bool bPhaseModifiable = planPhaseDateTimesModifiableDeterminer.ArePhaseDateTimesModifiable(phase);

	if (bPhaseModifiable)
	{
		const bool bTimeZeroOrderExcluded = component.GetTZInd() == TRUE;

		if (bTimeZeroOrderExcluded)
		{
			phase.PutTZExistInd(FALSE);
			UpdateTimeZeroLinkedComponentSchedules(phase, true);
		}
		else
		{
			CMostNegativeTimeZeroOffsetDeterminer mostNegativeTimeZeroOffsetDeterminer;
			const bool bHasMostNegativeTimeZeroOffset =
				mostNegativeTimeZeroOffsetDeterminer.DoesComponentHaveMostNegativeTimeZeroOffset(component);

			if (bHasMostNegativeTimeZeroOffset)
			{
				IComponentPtr pTimeZeroComponent = phase.FindTZComponent();

				if (NULL != pTimeZeroComponent)
				{
					const CModifiableOrderRetriever modifiableOrderRetriever(m_hPatCon);
					PvOrderObj* pTimeZeroOrderObj = modifiableOrderRetriever.GetModifiableOrderFromComponent(*pTimeZeroComponent);

					if (NULL != pTimeZeroOrderObj)
					{
						UpdateTimeZeroLinkedComponentSchedules(phase, false);
					}
				}
			}
		}
	}

	if (component.GetTZActiveInd() == TRUE)
	{
		component.PutTZActiveInd(FALSE);
		component.PutModifiedTZRelationInd(TRUE);
	}
}

/////////////////////////////////////////////////////////////////////////////
/// \fn		void CExcludeComponentScheduleManager::UpdateTimeZeroLinkedComponentSchedules(IPhase& phase, const bool bTimeZeroOrderExcluded)
/// \brief		Updates the phase time-zero date/time, and the schedules of all of the time-zero linked components that can be modified.
///
/// \return		void
///
/// \param[in]	IPhase& phase - The phase whose schedule needs updated.
/// \param[in]	const bool bTimeZeroOrderExcluded - True if the time-zero order was excluded. Otherwise, false.
/// \owner		DB024248
/////////////////////////////////////////////////////////////////////////////
void CExcludeComponentScheduleManager::UpdateTimeZeroLinkedComponentSchedules(IPhase& phase,
		const bool bTimeZeroOrderExcluded)
{
	ICalendarPtr pIPhaseStartDateTime = phase.GetUTCStartDtTm();
	const Cerner::Foundations::Calendar phaseStartDateTime = PvOrdCalUtils::GetDateOnlyCalendarFromInterface(
				pIPhaseStartDateTime);

	CPhaseTimeZeroDateTimeDeterminer phaseTimeZeroDateTimeDeterminer;
	ICalendarPtr pIPhaseTimeZeroDateTime = phaseTimeZeroDateTimeDeterminer.DetermineTimeZeroDateTime(phase);
	phase.PutUTCCalcTimeZeroDtTm(pIPhaseTimeZeroDateTime);
	const Cerner::Foundations::Calendar phaseTimeZeroDateTime = PvOrdCalUtils::GetDateOnlyCalendarFromInterface(
				pIPhaseTimeZeroDateTime);

	ICalendarPtr pIPhaseStopDateTime = phase.GetUTCCalcEndDtTm();
	const Cerner::Foundations::Calendar phaseStopDateTime = PvOrdCalUtils::GetDateOnlyCalendarFromInterface(
				pIPhaseStopDateTime);

	std::list<PvOrderObj*> affectedOrders;

	std::list<IComponent*> includedComponents;
	CIncludedComponentRetriever::GetIncludedComponents(phase, includedComponents);

	for (auto componentIter = includedComponents.cbegin(); componentIter != includedComponents.cend(); componentIter++)
	{
		IComponent* pIComponent = *componentIter;

		const bool bTimeZeroActive = pIComponent->GetTZActiveInd() == TRUE;

		if (bTimeZeroActive)
		{
			if (bTimeZeroOrderExcluded)
			{
				pIComponent->PutTZActiveInd(FALSE);
				pIComponent->PutModifiedTZRelationInd(TRUE);
			}

			const CString sComponentTypeMeaning = (LPCTSTR)pIComponent->GetComponentTypeMean();

			if (sComponentTypeMeaning == "ORDER CREATE")
			{
				PvOrderObj* pLinkedOrderObj = GetLinkedOrderWithUpdatedSchedule(*pIComponent, phaseStartDateTime, phaseTimeZeroDateTime,
											  phaseStopDateTime);

				if (NULL == pLinkedOrderObj)
				{
					continue;
				}

				affectedOrders.push_back(pLinkedOrderObj);
			}
			else if (sComponentTypeMeaning == "SUBPHASE")
			{
				IPhasePtr pISubPhase = pIComponent->GetSubphaseDispatch();

				if (NULL != pISubPhase)
				{
					UpdateLinkedSubPhaseSchedule(*pISubPhase, phaseStartDateTime, phaseTimeZeroDateTime, phaseStopDateTime, affectedOrders);
				}
			}
		}
	}

	CallInpatientOrdersScheduleService(affectedOrders);

	CProtocolOrderScheduleManager protocolOrderScheduleManager(m_hPatCon);
	protocolOrderScheduleManager.UpdateProtocolSchedule(phase);
}

/////////////////////////////////////////////////////////////////////////////
/// \fn		void CExcludeComponentScheduleManager::CallInpatientOrdersScheduleService(std::list<PvOrderObj*>& affectedOrders)
/// \brief		Calls the appropriate schedule services for the given inpatient orders.
///
/// \return		void
///
/// \param[in]	std::list<PvOrderObj*>& affectedOrders - The inpatient orders that were linked to the phase that were updated.
/// \owner		DB024248
/////////////////////////////////////////////////////////////////////////////
void CExcludeComponentScheduleManager::CallInpatientOrdersScheduleService(std::list<PvOrderObj*>& affectedOrders)
{
	CInpatientOrderScheduleServiceCaller inpatientOrderScheduleService(m_hPatCon);

	std::list<PvOrderObj*> newOrders;
	std::list<PvOrderObj*> modifiedOrders;
	std::list<PvOrderObj*> activatedOrders;

	for (auto affectedOrderIter = affectedOrders.cbegin(); affectedOrderIter != affectedOrders.cend(); affectedOrderIter++)
	{
		PvOrderObj* pAffectedOrderObj = *affectedOrderIter;

		const double dFmtActionCd = pAffectedOrderObj->GetFmtActionCd();

		if (CDF::OrderAction::IsOrder(dFmtActionCd))
		{
			newOrders.push_back(pAffectedOrderObj);
		}
		else if (CDF::OrderAction::IsModify(dFmtActionCd) || CDF::OrderAction::IsReschedule(dFmtActionCd))
		{
			modifiedOrders.push_back(pAffectedOrderObj);
		}
		else if (CDF::OrderAction::IsActivate(dFmtActionCd))
		{
			activatedOrders.push_back(pAffectedOrderObj);
		}
	}

	const bool bNewOrdersSuccess = inpatientOrderScheduleService.CalculateNewOrderSchedule(newOrders,
								   CalculateNewOrderScheduleRequest::eReferenceStartDateTimeChanged);
	const bool bModifiedOrdersSuccess = inpatientOrderScheduleService.CalculateModifyOrderSchedule(modifiedOrders,
										CalculateModifyOrderScheduleRequest::eReferenceStartDateTimeChanged);
	const bool bActivatedOrdersSuccess = inpatientOrderScheduleService.CalculateActivateOrderSchedule(activatedOrders,
										 CalculateActivateOrderScheduleRequest::eReferenceStartDateTimeChanged);
}

/////////////////////////////////////////////////////////////////////////////
/// \fn		void CExcludeComponentScheduleManager::UpdateLinkedSubPhaseSchedule(IPhase& subPhase, const Cerner::Foundations::Calendar& phaseStartDateTime, const Cerner::Foundations::Calendar& phaseTimeZeroDateTime, const Cerner::Foundations::Calendar& phaseStopDateTime, std::list<PvOrderObj*>& orders)
/// \brief		Updates the schedule of the included sub-phase and all of its components that are linked to phase start date/time.
///
/// \return		void
///
/// \param[in]	IPhase& subPhase - The sub-phase whose schedule needs updated as a result of another component being included.
/// \param[in]	const Cerner::Foundations::Calendar& phaseStartDateTime - The start date/time of the sub-phase's parent phase.
/// \param[in]	const Cerner::Foundations::Calendar& phaseTimeZeroDateTime - The time-zero date/time of the sub-phase's parent phase.
/// \param[in]	const Cerner::Foundations::Calendar& phaseStopDateTime - The stop date/time of the sub-phase's parent phase.
/// \param[out]	std::list<PvOrderObj*>& orders - The sub-phase's inpatient orders whose schedules need updated.
/// \owner		DB024248
/////////////////////////////////////////////////////////////////////////////
void CExcludeComponentScheduleManager::UpdateLinkedSubPhaseSchedule(IPhase& subPhase,
		const Cerner::Foundations::Calendar& phaseStartDateTime, const Cerner::Foundations::Calendar& phaseTimeZeroDateTime,
		const Cerner::Foundations::Calendar& phaseStopDateTime, std::list<PvOrderObj*>& orders)
{
	CPvGenericLoaderExtended(m_hPatCon).PrepareForModifyAction(&subPhase);

	CSubPhaseScheduleManager subPhaseScheduleManager;
	subPhaseScheduleManager.UpdateSubPhaseSchedule(subPhase, phaseStartDateTime, phaseTimeZeroDateTime, phaseStopDateTime);

	ICalendarPtr pISubPhaseStartDateTime = subPhase.GetUTCStartDtTm();
	const Cerner::Foundations::Calendar subPhaseStartDateTime = PvOrdCalUtils::GetDateOnlyCalendarFromInterface(
				pISubPhaseStartDateTime);

	const Cerner::Foundations::Calendar subPhaseTimeZeroDateTime = Cerner::Foundations::Calendar::CreateNull();

	ICalendarPtr pISubPhaseStopDateTime = subPhase.GetUTCCalcEndDtTm();
	const Cerner::Foundations::Calendar subPhaseStopDateTime = PvOrdCalUtils::GetDateOnlyCalendarFromInterface(
				pISubPhaseStopDateTime);

	std::list<IComponent*> includedComponents;
	CIncludedComponentRetriever::GetIncludedComponents(subPhase, includedComponents);

	for (auto componentIter = includedComponents.cbegin(); componentIter != includedComponents.cend(); componentIter++)
	{
		IComponent* pIComponent = *componentIter;

		const bool bLinkedToPhaseStartDateTime = pIComponent->GetLinkedToPhaseStartDateTime() == TRUE;

		if (bLinkedToPhaseStartDateTime)
		{
			const CString sComponentTypeMeaning = (LPCTSTR)pIComponent->GetComponentTypeMean();

			if (sComponentTypeMeaning == "ORDER CREATE")
			{
				PvOrderObj* pOrderObj = GetLinkedOrderWithUpdatedSchedule(*pIComponent, subPhaseStartDateTime, subPhaseTimeZeroDateTime,
										subPhaseStopDateTime);

				if (NULL != pOrderObj)
				{
					orders.push_back(pOrderObj);
				}
			}
			else if (sComponentTypeMeaning == "PRESCRIPTION")
			{
				UpdateLinkedPrescriptionComponentSchedule(*pIComponent, subPhaseStartDateTime);
			}
			else if (sComponentTypeMeaning == "RESULT OUTCO")
			{
				UpdateLinkedOutcomeComponentSchedule(subPhase, *pIComponent);
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
/// \fn		PvOrderObj* CExcludeComponentScheduleManager::GetLinkedOrderWithUpdatedSchedule(IComponent& orderComponent, const Cerner::Foundations::Calendar& phaseStartDateTime, const Cerner::Foundations::Calendar& phaseTimeZeroDateTime, const Cerner::Foundations::Calendar& phaseStopDateTime)
/// \brief		Gets the component's order object if it is modifiable, and updates its schedule.
///
/// \return		PvOrderObj* - The component's order object with the updated schedule. This will be NULL if the order cannot be modified.
///
/// \param[in]	IComponent& orderComponent - The order component that needs its schedule updated.
/// \param[in]	const Cerner::Foundations::Calendar& phaseStartDateTime - The start date/time of the phase that contains the order component.
/// \param[in]	const Cerner::Foundations::Calendar& phaseTimeZeroDateTime - The time-zero date/time of the phase that contains the order component.
/// \param[in]	const Cerner::Foundations::Calendar& phaseStopDateTime - The stop date/time of the phase that contains the order component.
/// \owner		DB024248
/////////////////////////////////////////////////////////////////////////////
PvOrderObj* CExcludeComponentScheduleManager::GetLinkedOrderWithUpdatedSchedule(IComponent& orderComponent,
		const Cerner::Foundations::Calendar& phaseStartDateTime, const Cerner::Foundations::Calendar& phaseTimeZeroDateTime,
		const Cerner::Foundations::Calendar& phaseStopDateTime)
{
	const CModifiableOrderRetriever modifiableOrderRetriever(m_hPatCon);
	PvOrderObj* pOrderObj = modifiableOrderRetriever.GetModifiableOrderFromComponent(orderComponent);

	if (NULL != pOrderObj)
	{
		Cerner::Foundations::Calendar actionDateTime = CActionDateTimeHelper::GetActionDateTime(*pOrderObj);

		if (actionDateTime > phaseStartDateTime)
		{
			pOrderObj->SetActionDtTm(phaseStartDateTime);
		}

		CChangePhaseStartOrderScheduleManager changePhaseStartOrderScheduleManager;
		changePhaseStartOrderScheduleManager.UpdateOrderScheduleOnChangePhaseStart(orderComponent,
				*pOrderObj, phaseStartDateTime, phaseTimeZeroDateTime, phaseStopDateTime);

		return pOrderObj;
	}

	return NULL;
}

/////////////////////////////////////////////////////////////////////////////
/// \fn		void CExcludeComponentScheduleManager::UpdateLinkedPrescriptionComponentSchedule(IComponent& prescriptionComponent, const Cerner::Foundations::Calendar& phaseStartDateTime)
/// \brief		Updates the prescription component's schedule.
///
/// \return		void
///
/// \param[in]	IComponent& prescriptionComponent - The prescription component whose schedule needs updated.
/// \param[in]	const Cerner::Foundations::Calendar& phaseStartDateTime - The start date/time of the phase that contains the prescription component.
/// \owner		DB024248
/////////////////////////////////////////////////////////////////////////////
void CExcludeComponentScheduleManager::UpdateLinkedPrescriptionComponentSchedule(IComponent& prescriptionComponent,
		const Cerner::Foundations::Calendar& phaseStartDateTime)
{
	const CModifiableOrderRetriever modifiableOrderRetriever(m_hPatCon);
	PvOrderObj* pOrderObj = modifiableOrderRetriever.GetModifiableOrderFromComponent(prescriptionComponent);

	if (NULL != pOrderObj)
	{
		Cerner::Foundations::Calendar actionDateTime = CActionDateTimeHelper::GetActionDateTime(*pOrderObj);

		if (actionDateTime > phaseStartDateTime)
		{
			pOrderObj->SetActionDtTm(phaseStartDateTime);
		}

		CPrescriptionScheduleManager prescriptionScheduleManager(m_hPatCon);
		prescriptionScheduleManager.UpdatePrescriptionSchedule(prescriptionComponent, *pOrderObj, phaseStartDateTime);
	}
}

/////////////////////////////////////////////////////////////////////////////
/// \fn		void CExcludeComponentScheduleManager::UpdateLinkedOutcomeComponentSchedule(IPhase& phase, IComponent& outcomeComponent)
/// \brief		Updates the outcome component's schedule.
///
/// \return		void
///
/// \param[in]	IPhase& phase - The phase containing the outcome component whose schedule needs updated.
/// \param[in]	IComponent& outcomeComponent - The outcome component whose schedule needs updated.
/// \owner		DB024248
/////////////////////////////////////////////////////////////////////////////
void CExcludeComponentScheduleManager::UpdateLinkedOutcomeComponentSchedule(IPhase& phase, IComponent& outcomeComponent)
{
	IOutcomePtr pOutcome = outcomeComponent.GetDispatch();

	if (NULL != pOutcome)
	{
		CPvGenericLoaderExtended(m_hPatCon).PrepareOutcomeForUpdate(pOutcome);
		CPvGenericLoaderExtended(m_hPatCon).PrepareForModifyAction(&outcomeComponent);

		COutcomeScheduleManager outcomeScheduleManager(m_hPatCon);
		outcomeScheduleManager.UpdateOutcomeSchedule(phase, outcomeComponent, *pOutcome);
	}
}