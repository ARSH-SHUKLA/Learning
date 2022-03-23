#include "StdAfx.h"
#include "resource.h"

#include "IncludeComponentScheduleManager.h"
#include "IncludeOrderScheduleManager.h"
#include "ChangePhaseStartOrderScheduleManager.h"
#include "PrescriptionScheduleManager.h"
#include "InpatientOrderScheduleServiceCaller.h"
#include "SubPhaseScheduleManager.h"
#include "ProtocolOrderScheduleManager.h"
#include "PhaseTimeZeroDateTimeDeterminer.h"
#include "OutcomeScheduleManager.h"
#include "IncludedComponentRetriever.h"
#include "OrderComponentRetriever.h"
#include "ModifiableOrderRetriever.h"
#include "SubPhaseRetriever.h"
#include "MostNegativeTimeZeroOffsetDeterminer.h"
#include "PlanOrderDateTimesModifiableDeterminer.h"
#include "PlanPhaseDateTimesModifiableDeterminer.h"
#include "DoTComponentRetriever.h"

#include <PvOrderProtocolObj.h>
#include <PvOrdCalUtil.h>
#include <ActionDateTimeHelper.h>
#include <PvGenericLoaderExtended.h>
#include <dcp_genericloader.h>
#include <OrderMessageBox.h>

CIncludeComponentScheduleManager::CIncludeComponentScheduleManager(const HPATCON hPatCon)
	: m_hPatCon(hPatCon)
{

}

/////////////////////////////////////////////////////////////////////////////
/// \fn		void CIncludeComponentScheduleManager::UpdatePhaseScheduleOnIncludeComponent(IPhase& phase, IComponent& component)
/// \brief		Updates the schedule of the included component and the phase when necessary, as well as all other affected components in the phase.
///				This should be called anytime a component is included into a phase, except when the phase is in a planned status.
///
/// \return		void
///
/// \param[in]	IPhase& phase - The phase containing the component that was included.
/// \param[in]	IComponent& component - The component that was included in the phase.
/// \owner		DB024248
/////////////////////////////////////////////////////////////////////////////
void CIncludeComponentScheduleManager::UpdatePhaseScheduleOnIncludeComponent(IPhase& phase, IComponent& component)
{
	bool bSuccess = true;

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
				const bool bTreatmentPeriodSuccess = UpdatePhaseScheduleOnIncludeComponentInternal(*pITreatmentPeriodPhase,
													 *pITreatmentPeriodComponent);
				bSuccess = bSuccess && bTreatmentPeriodSuccess;
			}
		}
	}
	else
	{
		bSuccess = UpdatePhaseScheduleOnIncludeComponentInternal(phase, component);
	}

	if (!bSuccess)
	{
		DisplayUnlinkFromTimeZeroMessage(component);
	}
}

/////////////////////////////////////////////////////////////////////////////
/// \fn		bool CIncludeComponentScheduleManager::UpdatePhaseScheduleOnIncludeComponentInternal(IPhase& phase, IComponent& component)
/// \brief		Updates the schedule of the included component and the phase when necessary, as well as all other affected components in the phase.
///				This should be called anytime a component is included into a phase, except when the phase is in a planned status.
///
/// \return		bool - True if there were no issues. False if the component got unlinked from time-zero.
///
/// \param[in]	IPhase& phase - The phase containing the component that was included.
/// \param[in]	IComponent& component - The component that was included in the phase.
/// \owner		DB024248
/////////////////////////////////////////////////////////////////////////////
bool CIncludeComponentScheduleManager::UpdatePhaseScheduleOnIncludeComponentInternal(IPhase& phase,
		IComponent& component)
{
	bool bSuccess = true;

	const bool bTimeZeroActiveInd = component.GetTZActiveInd() == TRUE;

	if (bTimeZeroActiveInd)
	{
		CPlanPhaseDateTimesModifiableDeterminer planPhaseDateTimesModifiableDeterminer;
		const bool bPhaseModifiable = planPhaseDateTimesModifiableDeterminer.ArePhaseDateTimesModifiable(phase);

		if (bPhaseModifiable)
		{
			bool bUpdateTimeZeroLinkedComponent(true);

			if (phase.GetPhaseType() == eTreatmentPeriod)
			{
				IPhasePtr pParentPhase(PowerPlanUtil::GetProtocolPhase(&phase));

				//If this is a DOT phase pending protocol review then DOTs are not exploded properly.
				//Therefore refrain from Most Negative Offset check.
				if (pParentPhase != nullptr && pParentPhase->IsSigningIntoPendingProtocolReview())
				{
					bUpdateTimeZeroLinkedComponent = false;
				}
			}

			if (bUpdateTimeZeroLinkedComponent)
			{
				CMostNegativeTimeZeroOffsetDeterminer mostNegativeTimeZeroOffsetDeterminer;
				const bool bHasMostNegativeTimeZeroOffset =
					mostNegativeTimeZeroOffsetDeterminer.DoesComponentHaveMostNegativeTimeZeroOffset(component);

				if (bHasMostNegativeTimeZeroOffset)
				{
					IComponentPtr pTimeZeroComponent = phase.FindTZComponent();

					if (pTimeZeroComponent != nullptr)
					{
						const CModifiableOrderRetriever modifiableOrderRetriever(m_hPatCon);
						PvOrderObj* pTimeZeroOrderObj = modifiableOrderRetriever.GetModifiableOrderFromComponent(*pTimeZeroComponent);

						if (pTimeZeroOrderObj != nullptr)
						{
							UpdateTimeZeroLinkedComponentSchedulesOnIncludeMostNegativeComponent(phase, component);
							return bSuccess;
						}
					}

					component.PutTZActiveInd(FALSE);
					component.PutModifiedTZRelationInd(TRUE);
					bSuccess = false;
				}
				else if (phase.GetUTCCalcTimeZeroDtTm() == nullptr)
				{
					CPhaseTimeZeroDateTimeDeterminer phaseTimeZeroDateTimeDeterminer;
					ICalendarPtr pIPhaseTimeZeroDateTime = phaseTimeZeroDateTimeDeterminer.DetermineTimeZeroDateTime(phase);
					phase.PutUTCCalcTimeZeroDtTm(pIPhaseTimeZeroDateTime);
				}
			}
		}
		else
		{
			component.PutTZActiveInd(FALSE);
			component.PutModifiedTZRelationInd(TRUE);
			bSuccess = false;
		}
	}

	ICalendarPtr pIPhaseStartDateTime = phase.GetUTCStartDtTm();
	const Cerner::Foundations::Calendar phaseStartDateTime = PvOrdCalUtils::GetDateOnlyCalendarFromInterface(
				pIPhaseStartDateTime);

	ICalendarPtr pIPhaseTimeZeroDateTime = phase.GetUTCCalcTimeZeroDtTm();
	const Cerner::Foundations::Calendar phaseTimeZeroDateTime = PvOrdCalUtils::GetDateOnlyCalendarFromInterface(
				pIPhaseTimeZeroDateTime);

	ICalendarPtr pIPhaseStopDateTime = phase.GetUTCCalcEndDtTm();
	const Cerner::Foundations::Calendar phaseStopDateTime = PvOrdCalUtils::GetDateOnlyCalendarFromInterface(
				pIPhaseStopDateTime);

	std::list<PvOrderObj*> includedOrders;
	UpdateIncludedComponentSchedule(phase, component, phaseStartDateTime, phaseTimeZeroDateTime, phaseStopDateTime,
									includedOrders);

	std::list<PvOrderObj*> affectedOrders;
	CallInpatientOrdersScheduleService(includedOrders, affectedOrders);

	CProtocolOrderScheduleManager protocolOrderScheduleManager(m_hPatCon);
	protocolOrderScheduleManager.UpdateProtocolSchedule(phase);

	return bSuccess;
}

/////////////////////////////////////////////////////////////////////////////
/// \fn		void CIncludeComponentScheduleManager::UpdateTimeZeroLinkedComponentSchedulesOnIncludeMostNegativeComponent(IPhase& phase, IComponent& includedComponent)
/// \brief		Updates the phase time-zero date/time, and the schedules of all of the time-zero linked components that can be modified.
///
/// \return		void
///
/// \param[in]	IPhase& phase - The phase containing the component that was included.
/// \param[in]	IComponent& component - The component that was included in the phase.
/// \owner		DB024248
/////////////////////////////////////////////////////////////////////////////
void CIncludeComponentScheduleManager::UpdateTimeZeroLinkedComponentSchedulesOnIncludeMostNegativeComponent(
	IPhase& phase, IComponent& includedComponent)
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

	std::list<PvOrderObj*> includedOrders;
	UpdateIncludedComponentSchedule(phase, includedComponent, phaseStartDateTime, phaseTimeZeroDateTime, phaseStopDateTime,
									includedOrders);

	std::list<PvOrderObj*> linkedOrders;

	const double dIncludedComponentId = includedComponent.GetActCompId();

	std::list<IComponent*> orderComponents;
	COrderComponentRetriever::GetIncludedOrderComponents(phase, orderComponents);

	for (auto orderComponentIter = orderComponents.cbegin(); orderComponentIter != orderComponents.cend();
		 orderComponentIter++)
	{
		IComponent* pIOrderComponent = *orderComponentIter;

		const double dComponentId = pIOrderComponent->GetActCompId();

		if (dComponentId != dIncludedComponentId)
		{
			const bool bTimeZeroActive = pIOrderComponent->GetTZActiveInd() == TRUE;

			if (bTimeZeroActive)
			{
				PvOrderObj* pLinkedOrderObj = GetLinkedOrderWithUpdatedSchedule(*pIOrderComponent, phaseStartDateTime,
											  phaseTimeZeroDateTime, phaseStopDateTime);

				if (NULL == pLinkedOrderObj)
				{
					pIOrderComponent->PutTZActiveInd(FALSE);
					pIOrderComponent->PutModifiedTZRelationInd(TRUE);
					continue;
				}

				linkedOrders.push_back(pLinkedOrderObj);
			}
		}
	}

	std::list<IPhase*> subPhases;
	CSubPhaseRetriever::GetIncludedSubPhases(phase, subPhases);

	for (auto subPhaseIter = subPhases.cbegin(); subPhaseIter != subPhases.cend(); subPhaseIter++)
	{
		IPhase* pISubPhase = *subPhaseIter;

		IComponentPtr pISubPhaseComponent = pISubPhase->GetSubphaseCompDispatch();

		if (NULL != pISubPhaseComponent)
		{
			const double dSubPhaseComponentId = pISubPhaseComponent->GetActCompId();

			if (dSubPhaseComponentId != dIncludedComponentId)
			{
				const bool bTimeZeroActive = pISubPhaseComponent->GetTZActiveInd() == TRUE;

				if (bTimeZeroActive)
				{
					UpdateLinkedSubPhaseSchedule(*pISubPhase, phaseStartDateTime, phaseTimeZeroDateTime, phaseStopDateTime, linkedOrders);
				}
			}
		}
	}

	CallInpatientOrdersScheduleService(includedOrders, linkedOrders);

	CProtocolOrderScheduleManager protocolOrderScheduleManager(m_hPatCon);
	protocolOrderScheduleManager.UpdateProtocolSchedule(phase);
}

/////////////////////////////////////////////////////////////////////////////
/// \fn		void CIncludeComponentScheduleManager::UpdateIncludedComponentSchedule(IPhase& phase, IComponent& includedComponent, const Cerner::Foundations::Calendar& phaseStartDateTime, const Cerner::Foundations::Calendar& phaseTimeZeroDateTime, const Cerner::Foundations::Calendar& phaseStopDateTime, std::list<PvOrderObj*>& includedOrders)
/// \brief		Updates the component schedule for the included component.
///
/// \return		void
///
/// \param[in]	IPhase& phase - The phase the component was included in.
/// \param[in]	IComponent& includedComponent - The component that was included.
/// \param[in]	const Cerner::Foundations::Calendar& phaseStartDateTime - The start date/time of the phase that the component was included in.
/// \param[in]	const Cerner::Foundations::Calendar& phaseTimeZeroDateTime - The time-zero date/time of the phase that the component was included in.
/// \param[in]	const Cerner::Foundations::Calendar& phaseStopDateTime - The stop date/time of the phase that the component was included in.
/// \param[out]	std::list<PvOrderObj*>& includedOrders - This will contain the order objects for any inpatient orders that were included.
/// \owner		DB024248
/////////////////////////////////////////////////////////////////////////////
void CIncludeComponentScheduleManager::UpdateIncludedComponentSchedule(IPhase& phase, IComponent& includedComponent,
		const Cerner::Foundations::Calendar& phaseStartDateTime, const Cerner::Foundations::Calendar& phaseTimeZeroDateTime,
		const Cerner::Foundations::Calendar& phaseStopDateTime, std::list<PvOrderObj*>& includedOrders)
{
	includedComponent.PutLinkedToPhaseStartDateTime(TRUE);

	const CString sComponentTypeMeaning = (LPCTSTR)includedComponent.GetComponentTypeMean();

	if (sComponentTypeMeaning == "ORDER CREATE")
	{
		PvOrderObj* pIncludedOrderObj = GetIncludedOrderWithUpdatedSchedule(includedComponent, phaseStartDateTime,
										phaseTimeZeroDateTime, phaseStopDateTime);

		if (NULL != pIncludedOrderObj)
		{
			includedOrders.push_back(pIncludedOrderObj);
		}
	}
	else if (sComponentTypeMeaning == "PRESCRIPTION")
	{
		UpdatePrescriptionComponentScheduleOnInclude(includedComponent, phaseStartDateTime);
	}
	else if (sComponentTypeMeaning == "RESULT OUTCO")
	{
		UpdateOutcomeComponentScheduleOnInclude(phase, includedComponent);
	}
	else if (sComponentTypeMeaning == "SUBPHASE")
	{
		IPhasePtr pISubPhase = includedComponent.GetSubphaseDispatch();

		if (pISubPhase != NULL)
		{
			UpdateIncludedSubPhaseSchedule(*pISubPhase, phaseStartDateTime, phaseTimeZeroDateTime, phaseStopDateTime,
										   includedOrders);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
/// \fn		void CIncludeComponentScheduleManager::CallInpatientOrdersScheduleService(std::list<PvOrderObj*>& includedOrders, std::list<PvOrderObj*>& linkedOrders)
/// \brief		Calls the appropriate schedule services for the given inpatient orders.
///
/// \return		void
///
/// \param[in]	std::list<PvOrderObj*>& includedOrders - The inpatient orders that were included into the phase.
/// \param[in]	std::list<PvOrderObj*>& linkedOrders - The inpatient orders that were linked to the phase that were updated.
/// \owner		DB024248
/////////////////////////////////////////////////////////////////////////////
void CIncludeComponentScheduleManager::CallInpatientOrdersScheduleService(std::list<PvOrderObj*>& includedOrders,
		std::list<PvOrderObj*>& linkedOrders)
{
	std::list<CCalculateNewOrderScheduleCriteria> newOrderCriteria;
	std::list<PvOrderObj*> modifiedOrders;
	std::list<PvOrderObj*> activatedOrders;

	for (auto includedOrderIter = includedOrders.cbegin(); includedOrderIter != includedOrders.cend(); includedOrderIter++)
	{
		PvOrderObj* pIncludedOrderObj = *includedOrderIter;

		CCalculateNewOrderScheduleCriteria includedOrderCriteria(pIncludedOrderObj,
				CalculateNewOrderScheduleRequest::eNewOrderAdded, 3);
		newOrderCriteria.push_back(includedOrderCriteria);
	}

	for (auto linkedOrderIter = linkedOrders.cbegin(); linkedOrderIter != linkedOrders.cend(); linkedOrderIter++)
	{
		PvOrderObj* pLinkedOrderObj = *linkedOrderIter;

		const double dFmtActionCd = pLinkedOrderObj->GetFmtActionCd();

		if (CDF::OrderAction::IsOrder(dFmtActionCd))
		{
			CCalculateNewOrderScheduleCriteria linkedOrderCriteria(pLinkedOrderObj,
					CalculateNewOrderScheduleRequest::eReferenceStartDateTimeChanged, 3);
			newOrderCriteria.push_back(linkedOrderCriteria);
		}
		else if (CDF::OrderAction::IsModify(dFmtActionCd) || CDF::OrderAction::IsReschedule(dFmtActionCd))
		{
			modifiedOrders.push_back(pLinkedOrderObj);
		}
		else if (CDF::OrderAction::IsActivate(dFmtActionCd))
		{
			activatedOrders.push_back(pLinkedOrderObj);
		}
	}

	CInpatientOrderScheduleServiceCaller inpatientOrderScheduleService(m_hPatCon);

	const bool bNewOrdersSuccess = inpatientOrderScheduleService.CalculateNewOrderSchedule(newOrderCriteria);
	const bool bModifiedOrdersSuccess = inpatientOrderScheduleService.CalculateModifyOrderSchedule(modifiedOrders,
										CalculateModifyOrderScheduleRequest::eReferenceStartDateTimeChanged);
	const bool bActivatedOrdersSuccess = inpatientOrderScheduleService.CalculateActivateOrderSchedule(activatedOrders,
										 CalculateActivateOrderScheduleRequest::eReferenceStartDateTimeChanged);
}

/////////////////////////////////////////////////////////////////////////////
/// \fn		void CIncludeComponentScheduleManager::UpdateIncludedSubPhaseSchedule(IPhase& subPhase, const Cerner::Foundations::Calendar& phaseStartDateTime, const Cerner::Foundations::Calendar& phaseTimeZeroDateTime, const Cerner::Foundations::Calendar& phaseStopDateTime, std::list<PvOrderObj*>& orders)
/// \brief		Updates the schedule of the included sub-phase and all of its components.
///
/// \return		void
///
/// \param[in]	IPhase& subPhase - The sub-phase that was included.
/// \param[in]	const Cerner::Foundations::Calendar& phaseStartDateTime - The start date/time of the phase that the sub-phase was included in.
/// \param[in]	const Cerner::Foundations::Calendar& phaseTimeZeroDateTime - The time-zero date/time of the phase that the sub-phase was included in.
/// \param[in]	const Cerner::Foundations::Calendar& phaseStopZeroDateTime - The stop date/time of the phase that the sub-phase was included in.
/// \param[out]	std::list<PvOrderObj*>& orders - The sub-phase's inpatient orders whose schedules need updated.
/// \owner		DB024248
/////////////////////////////////////////////////////////////////////////////
void CIncludeComponentScheduleManager::UpdateIncludedSubPhaseSchedule(IPhase& subPhase,
		const Cerner::Foundations::Calendar& phaseStartDateTime, const Cerner::Foundations::Calendar& phaseTimeZeroDateTime,
		const Cerner::Foundations::Calendar& phaseStopDateTime, std::list<PvOrderObj*>& orders)
{
	const ACTION_TYPE actionType = subPhase.GetActionType();

	if (actionType == eActionNone || actionType == eActionActReplicated || actionType == eActionActCopyForward)
	{
		CPvGenericLoaderExtended(m_hPatCon).PreparePhaseForUpdate(&subPhase);
		subPhase.PutActionType(eActionCreate);
	}

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

		const CString sComponentTypeMeaning = (LPCTSTR)pIComponent->GetComponentTypeMean();

		if (sComponentTypeMeaning == "ORDER CREATE")
		{
			PvOrderObj* pOrderObj = GetIncludedOrderWithUpdatedSchedule(*pIComponent, subPhaseStartDateTime,
									subPhaseTimeZeroDateTime, subPhaseStopDateTime);

			if (NULL != pOrderObj)
			{
				orders.push_back(pOrderObj);
			}
		}
		else if (sComponentTypeMeaning == "PRESCRIPTION")
		{
			UpdatePrescriptionComponentScheduleOnInclude(*pIComponent, subPhaseStartDateTime);
		}
		else if (sComponentTypeMeaning == "RESULT OUTCO")
		{
			UpdateOutcomeComponentScheduleOnInclude(subPhase, *pIComponent);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
/// \fn		void CIncludeComponentScheduleManager::UpdateLinkedSubPhaseSchedule(IPhase& subPhase, const Cerner::Foundations::Calendar& phaseStartDateTime, const Cerner::Foundations::Calendar& phaseTimeZeroDateTime, const Cerner::Foundations::Calendar& phaseStopDateTime, std::list<PvOrderObj*>& orders)
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
void CIncludeComponentScheduleManager::UpdateLinkedSubPhaseSchedule(IPhase& subPhase,
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
				UpdatePrescriptionComponentScheduleOnInclude(*pIComponent, subPhaseStartDateTime);
			}
			else if (sComponentTypeMeaning == "RESULT OUTCO")
			{
				UpdateOutcomeComponentScheduleOnInclude(subPhase, *pIComponent);
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
/// \fn		PvOrderObj* CIncludeComponentScheduleManager::GetIncludedOrderWithUpdatedSchedule(IComponent& orderComponent, const Cerner::Foundations::Calendar& phaseStartDateTime, const Cerner::Foundations::Calendar& phaseTimeZeroDateTime, const Cerner::Foundations::Calendar& phaseStopDateTime)
/// \brief		Gets the order object of the component that was included, and updates its schedule.
///
/// \return		PvOrderObj* - The component's order object with the updated schedule.
///
/// \param[in]	IComponent& orderComponent - The order component that was included.
/// \param[in]	const Cerner::Foundations::Calendar& phaseStartDateTime - The start date/time of the phase that contains the order component.
/// \param[in]	const Cerner::Foundations::Calendar& phaseTimeZeroDateTime - The time-zero date/time of the phase that contains the order component.
/// \param[in]	const Cerner::Foundations::Calendar& phaseStopDateTime - The stop date/time of the phase that contains the order component.
/// \owner		DB024248
/////////////////////////////////////////////////////////////////////////////
PvOrderObj* CIncludeComponentScheduleManager::GetIncludedOrderWithUpdatedSchedule(IComponent& orderComponent,
		const Cerner::Foundations::Calendar& phaseStartDateTime, const Cerner::Foundations::Calendar& phaseTimeZeroDateTime,
		const Cerner::Foundations::Calendar& phaseStopDateTime)
{
	const CModifiableOrderRetriever modifiableOrderRetriever(m_hPatCon);
	PvOrderObj* pOrderObj = modifiableOrderRetriever.GetModifiableOrderFromComponent(orderComponent);

	if (NULL != pOrderObj)
	{
		CIncludeOrderScheduleManager includeOrderScheduleManager;
		const bool bOrderSuccess = includeOrderScheduleManager.UpdateOrderScheduleOnInclude(orderComponent, *pOrderObj,
								   phaseStartDateTime, phaseTimeZeroDateTime, phaseStopDateTime);

		return pOrderObj;
	}

	return NULL;
}

/////////////////////////////////////////////////////////////////////////////
/// \fn		PvOrderObj* CIncludeComponentScheduleManager::GetLinkedOrderWithUpdatedSchedule(IComponent& orderComponent, const Cerner::Foundations::Calendar& phaseStartDateTime, const Cerner::Foundations::Calendar& phaseTimeZeroDateTime, const Cerner::Foundations::Calendar& phaseStopDateTime)
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
PvOrderObj* CIncludeComponentScheduleManager::GetLinkedOrderWithUpdatedSchedule(IComponent& orderComponent,
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
		const bool bOrderSuccess = changePhaseStartOrderScheduleManager.UpdateOrderScheduleOnChangePhaseStart(orderComponent,
								   *pOrderObj, phaseStartDateTime, phaseTimeZeroDateTime, phaseStopDateTime);

		return pOrderObj;
	}

	return NULL;
}

/////////////////////////////////////////////////////////////////////////////
/// \fn		void CIncludeComponentScheduleManager::UpdatePrescriptionComponentScheduleOnInclude(IComponent& prescriptionComponent, const Cerner::Foundations::Calendar& phaseStartDateTime)
/// \brief		Updates the prescription component's schedule.
///
/// \return		void
///
/// \param[in]	IComponent& prescriptionComponent - The prescription component whose schedule needs updated.
/// \param[in]	const Cerner::Foundations::Calendar& phaseStartDateTime - The start date/time of the phase that contains the prescription component.
/// \owner		DB024248
/////////////////////////////////////////////////////////////////////////////
void CIncludeComponentScheduleManager::UpdatePrescriptionComponentScheduleOnInclude(IComponent& prescriptionComponent,
		const Cerner::Foundations::Calendar& phaseStartDateTime)
{
	const CModifiableOrderRetriever modifiableOrderRetriever(m_hPatCon);
	PvOrderObj* pOrderObj = modifiableOrderRetriever.GetModifiableOrderFromComponent(prescriptionComponent);

	if (NULL != pOrderObj)
	{
		CPrescriptionScheduleManager prescriptionScheduleManager(m_hPatCon);
		prescriptionScheduleManager.UpdatePrescriptionSchedule(prescriptionComponent, *pOrderObj, phaseStartDateTime);
	}
}

/////////////////////////////////////////////////////////////////////////////
/// \fn		void CIncludeComponentScheduleManager::UpdateOutcomeComponentScheduleOnInclude(IPhase& phase, IComponent& outcomeComponent)
/// \brief		Updates the outcome component's schedule.
///
/// \return		void
///
/// \param[in]	IPhase& phase - The phase containing the outcome component whose schedule needs updated.
/// \param[in]	IComponent& outcomeComponent - The outcome component whose schedule needs updated.
/// \owner		DB024248
/////////////////////////////////////////////////////////////////////////////
void CIncludeComponentScheduleManager::UpdateOutcomeComponentScheduleOnInclude(IPhase& phase,
		IComponent& outcomeComponent)
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

/////////////////////////////////////////////////////////////////////////////
/// \fn		void CIncludeComponentScheduleManager::DisplayUnlinkFromTimeZeroMessage(IComponent& component)
/// \brief		Displays a message alerting the user that the component's time-zero offset cannot be honored.
///
/// \return		void
///
/// \param[in]	IComponent& component - The component whose time-zero offset cannot be honored.
/// \owner		DB024248
/////////////////////////////////////////////////////////////////////////////
void CIncludeComponentScheduleManager::DisplayUnlinkFromTimeZeroMessage(IComponent& component)
{
	CString msg;
	CString componentDescription = GetComponentDescription(component);
	msg.FormatMessage(IDS_COMP_UNLINK_FROM_TIME_ZERO, componentDescription);

	Orders::MessageBox(nullptr, msg, MAKEINTRESOURCE(IDS_PLANS), MB_ICONEXCLAMATION | MB_OK);
}

/////////////////////////////////////////////////////////////////////////////
/// \fn		CString CIncludeComponentScheduleManager::GetComponentDescription(IComponent& component)
/// \brief		Gets the component description to display for the time-zero unlinked message.
///
/// \return		CString - The component description.
///
/// \param[in]	IComponent& component - The component to determine the description of. This should be an order or sub-phase component.
/// \owner		DB024248
/////////////////////////////////////////////////////////////////////////////
CString CIncludeComponentScheduleManager::GetComponentDescription(IComponent& component)
{
	CString compTypeMean((LPCTSTR)component.GetComponentTypeMean());

	if (compTypeMean == _T("ORDER CREATE"))
	{
		double dOrderId = component.GetParentEntId();

		if (dOrderId > 0)
		{
			PvOrderObj* pOrderObj = dynamic_cast<PvOrderObj*>(CGenLoader().FindProfileOrderById(m_hPatCon, dOrderId));

			if (pOrderObj != NULL)
			{
				return pOrderObj->GetHnaMnemonic();
			}
		}

		return (LPCTSTR)component.GetDisplayMnemonic();
	}
	else if (compTypeMean == _T("SUBPHASE"))
	{
		return (LPCTSTR)component.GetSubPhaseDisplayDesc();
	}

	return "";
}