#include "StdAfx.h"

#include "ChangePhaseStartDateTimeManager.h"
#include "ProtocolOrderScheduleManager.h"
#include "PhaseTimeZeroDateTimeDeterminer.h"
#include "SubPhaseScheduleManager.h"
#include "InpatientOrderScheduleServiceCaller.h"
#include "ChangePhaseStartOrderScheduleManager.h"
#include "PrescriptionScheduleManager.h"
#include "ModifiableOrderRetriever.h"
#include "OutcomeScheduleManager.h"
#include "IncludedComponentRetriever.h"

#include <pvorderobj.h>
#include <PvOrdCalUtil.h>
#include <srvcalendar.h>
#include <CPS_ImportPVCareCoordCom.h>
#include <CPS_ImportCpsDataCommon.h>
#include <ActionDateTimeHelper.h>
#include <PvGenericLoaderExtended.h>

CChangePhaseStartDateTimeManager::CChangePhaseStartDateTimeManager(const HPATCON hPatCon)
	: m_hPatCon(hPatCon)
{

}

void CChangePhaseStartDateTimeManager::UpdatePhaseScheduleOnChangePhaseStart(IPhase& phase,
		const bool bAdjustSchedulableOrder)
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

	if (phase.GetInitiated())
	{
		std::list<PvOrderObj*> inpatientOrders;

		std::list<IComponent*> components;
		CIncludedComponentRetriever::GetIncludedComponents(phase, components);

		for (auto componentIter = components.cbegin(); componentIter != components.cend(); componentIter++)
		{
			IComponent* pIComponent = *componentIter;

			const bool bLinkedToPhaseStart = pIComponent->GetLinkedToPhaseStartDateTime() == TRUE;

			if (bLinkedToPhaseStart)
			{
				UpdateComponentScheduleOnChangePhaseStart(phase, *pIComponent, inpatientOrders, phaseStartDateTime,
						phaseTimeZeroDateTime, phaseStopDateTime);
			}
		}

		CallInpatientOrderScheduleService(inpatientOrders);

		CProtocolOrderScheduleManager protocolOrderScheduleManager(m_hPatCon);
		protocolOrderScheduleManager.UpdateProtocolSchedule(phase);
	}

	if (bAdjustSchedulableOrder)
	{
		AdjustSchedulableOrder(phase, phaseStartDateTime);
	}
}

void CChangePhaseStartDateTimeManager::UpdateComponentScheduleOnChangePhaseStart(IPhase& phase, IComponent& component,
		std::list<PvOrderObj*>& inpatientOrders, const Cerner::Foundations::Calendar& phaseStartDateTime,
		const Cerner::Foundations::Calendar& phaseTimeZeroDateTime, const Cerner::Foundations::Calendar& phaseStopDateTime)
{
	const CString sComponentTypeMeaning = (LPCTSTR)component.GetComponentTypeMean();

	if (sComponentTypeMeaning == "ORDER CREATE")
	{
		PvOrderObj* pOrderObj = GetUpdatedOrderOnChangePhaseStart(component, phaseStartDateTime, phaseTimeZeroDateTime,
								phaseStopDateTime);

		if (NULL != pOrderObj)
		{
			inpatientOrders.push_back(pOrderObj);
		}
	}
	else if (sComponentTypeMeaning == "PRESCRIPTION")
	{
		UpdatePrescriptionComponentScheduleOnChangePhaseStart(component, phaseStartDateTime);
	}
	else if (sComponentTypeMeaning == "RESULT OUTCO")
	{
		UpdateOutcomeComponentScheduleOnChangePhaseStart(phase, component);
	}
	else if (sComponentTypeMeaning == "SUBPHASE")
	{
		IPhasePtr pISubPhase = component.GetSubphaseDispatch();

		if (pISubPhase != NULL)
		{
			UpdateSubPhaseScheduleOnChangePhaseStart(*pISubPhase, inpatientOrders, phaseStartDateTime, phaseTimeZeroDateTime,
					phaseStopDateTime);
		}
	}
}

void CChangePhaseStartDateTimeManager::AdjustSchedulableOrder(IPhase& phase,
		const Cerner::Foundations::Calendar& phaseStartDateTime)
{
	IPlanPtr pIPlan = PowerPlanUtil::GetPlan(&phase);

	if (pIPlan != NULL)
	{
		const double dPhaseId = phase.GetPlanId();

		IComponentPtr pISchedulableComponent = pIPlan->GetSchedulableComponentByPhaseId(dPhaseId);

		if (pISchedulableComponent != NULL)
		{
			const CModifiableOrderRetriever modifiableOrderRetriever(m_hPatCon);
			PvOrderObj* pOrderObj = modifiableOrderRetriever.GetModifiableSchedulableOrderFromComponent(*pISchedulableComponent);

			if (NULL != pOrderObj)
			{
				// Set the requested start dt/tm of the order to the start of its scheduled phase.
				PvOrderFld* pStartDtTmFld = pOrderObj->m_orderFldArr.GetFieldFromMeanId(eDetailReqStartDtTm);

				if (pStartDtTmFld != NULL)
				{
					Cerner::Foundations::Calendar startDateTime = phaseStartDateTime;
					pStartDtTmFld->AddOeFieldDtTmValue(startDateTime);

					if (pISchedulableComponent->GetInitiated())
					{
						CallInpatientOrderScheduleServiceForSchedulableOrder(pOrderObj);
					}
					else
					{
						pOrderObj->BuildDisplayLines();
					}
				}
			}
		}
	}
}

PvOrderObj* CChangePhaseStartDateTimeManager::GetUpdatedOrderOnChangePhaseStart(IComponent& orderComponent,
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

		CChangePhaseStartOrderScheduleManager orderScheduleManager;
		orderScheduleManager.UpdateOrderScheduleOnChangePhaseStart(orderComponent, *pOrderObj, phaseStartDateTime,
				phaseTimeZeroDateTime, phaseStopDateTime);

		return pOrderObj;
	}

	return NULL;
}

void CChangePhaseStartDateTimeManager::UpdatePrescriptionComponentScheduleOnChangePhaseStart(
	IComponent& prescriptionComponent, const Cerner::Foundations::Calendar& phaseStartDateTime)
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
/// \fn		void CChangePhaseStartDateTimeManager::UpdateOutcomeComponentScheduleOnChangePhaseStart(IPhase& phase, IComponent& outcomeComponent)
/// \brief		Updates the outcome component's schedule.
///
/// \return		void
///
/// \param[in]	IPhase& phase - The phase containing the outcome component whose schedule needs updated.
/// \param[in]	IComponent& outcomeComponent - The outcome component whose schedule needs updated.
/// \owner		DB024248
/////////////////////////////////////////////////////////////////////////////
void CChangePhaseStartDateTimeManager::UpdateOutcomeComponentScheduleOnChangePhaseStart(IPhase& phase,
		IComponent& outcomeComponent)
{
	IOutcomePtr pOutcome = outcomeComponent.GetDispatch();

	if (pOutcome != nullptr)
	{
		// Preparing the outcome for update.
		CPvGenericLoaderExtended(m_hPatCon).PrepareOutcomeForUpdate(pOutcome);
		CPvGenericLoaderExtended(m_hPatCon).PrepareForModifyAction(&outcomeComponent);

		if (pOutcome->GetActionType() == eActionNone)
		{
			pOutcome->PutActionType(eActionActModify);
		}

		COutcomeScheduleManager outcomeScheduleManager(m_hPatCon);
		outcomeScheduleManager.UpdateOutcomeSchedule(phase, outcomeComponent, *pOutcome);
	}
}

void CChangePhaseStartDateTimeManager::UpdateSubPhaseScheduleOnChangePhaseStart(IPhase& subPhase,
		std::list<PvOrderObj*>& inpatientOrders, const Cerner::Foundations::Calendar& phaseStartDateTime,
		const Cerner::Foundations::Calendar& phaseTimeZeroDateTime, const Cerner::Foundations::Calendar& phaseStopDateTime)
{
	// Prepares the subphase for update.
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

	std::list<IComponent*> components;
	CIncludedComponentRetriever::GetIncludedComponents(subPhase, components);

	for (auto componentIter = components.cbegin(); componentIter != components.cend(); componentIter++)
	{
		IComponent* pIComponent = *componentIter;

		const bool bLinkedToPhaseStart = pIComponent->GetLinkedToPhaseStartDateTime() == TRUE;

		if (bLinkedToPhaseStart)
		{
			UpdateComponentScheduleOnChangePhaseStart(subPhase, *pIComponent, inpatientOrders, subPhaseStartDateTime,
					subPhaseTimeZeroDateTime, subPhaseStopDateTime);
		}
	}
}

void CChangePhaseStartDateTimeManager::CallInpatientOrderScheduleService(std::list<PvOrderObj*>& orders)
{
	std::list<PvOrderObj*> newOrders;
	std::list<PvOrderObj*> modifyOrders;
	std::list<PvOrderObj*> activateOrders;

	for (auto orderIter = orders.cbegin(); orderIter != orders.cend(); orderIter++)
	{
		PvOrderObj* pOrderObj = *orderIter;

		const double dFmtActionCd = pOrderObj->GetFmtActionCd();

		if (CDF::OrderAction::IsOrder(dFmtActionCd))
		{
			newOrders.push_back(pOrderObj);
		}
		else if (CDF::OrderAction::IsModify(dFmtActionCd) || CDF::OrderAction::IsReschedule(dFmtActionCd))
		{
			modifyOrders.push_back(pOrderObj);
		}
		else if (CDF::OrderAction::IsActivate(dFmtActionCd))
		{
			activateOrders.push_back(pOrderObj);
		}
	}

	CInpatientOrderScheduleServiceCaller inpatientOrderScheduleService(m_hPatCon);

	const bool bNewOrderSuccess = inpatientOrderScheduleService.CalculateNewOrderSchedule(newOrders,
								  CalculateNewOrderScheduleRequest::eReferenceStartDateTimeChanged);
	const bool bModifySuccess = inpatientOrderScheduleService.CalculateModifyOrderSchedule(modifyOrders,
								CalculateModifyOrderScheduleRequest::eReferenceStartDateTimeChanged);
	const bool bActivateSuccess = inpatientOrderScheduleService.CalculateActivateOrderSchedule(activateOrders,
								  CalculateActivateOrderScheduleRequest::eReferenceStartDateTimeChanged);
}

void CChangePhaseStartDateTimeManager::CallInpatientOrderScheduleServiceForSchedulableOrder(
	PvOrderObj* pSchedulableOrder)
{
	std::list<PvOrderObj*> orders;
	orders.push_back(pSchedulableOrder);

	CInpatientOrderScheduleServiceCaller inpatientOrderScheduleService(m_hPatCon);

	const double dFmtActionCd = pSchedulableOrder->GetFmtActionCd();

	if (CDF::OrderAction::IsOrder(dFmtActionCd))
	{
		const bool bNewOrderSuccess = inpatientOrderScheduleService.CalculateNewOrderSchedule(orders,
									  CalculateNewOrderScheduleRequest::eRequestedStartDateTimeChanged);
	}
	else if (CDF::OrderAction::IsModify(dFmtActionCd) || CDF::OrderAction::IsReschedule(dFmtActionCd))
	{
		const bool bModifySuccess = inpatientOrderScheduleService.CalculateModifyOrderSchedule(orders,
									CalculateModifyOrderScheduleRequest::eRequestedStartDateTimeChanged);
	}
	else if (CDF::OrderAction::IsActivate(dFmtActionCd))
	{
		const bool bActivateSuccess = inpatientOrderScheduleService.CalculateActivateOrderSchedule(orders,
									  CalculateActivateOrderScheduleRequest::eRequestedStartDateTimeChanged);
	}
}