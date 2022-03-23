#include "StdAfx.h"

#include "ChangePhaseTimeZeroDateTimeManager.h"
#include "ProtocolOrderScheduleManager.h"
#include "PhaseTimeZeroDateTimeDeterminer.h"
#include "SubPhaseScheduleManager.h"
#include "InpatientOrderScheduleServiceCaller.h"
#include "ChangePhaseStartOrderScheduleManager.h"
#include "PrescriptionScheduleManager.h"
#include "OutcomeScheduleManager.h"
#include "IncludedComponentRetriever.h"
#include "ModifiableOrderRetriever.h"

#include <pvorderobj.h>
#include <PvOrdCalUtil.h>
#include <srvcalendar.h>
#include <CPS_ImportPVCareCoordCom.h>
#include <CPS_ImportCpsDataCommon.h>
#include <ActionDateTimeHelper.h>
#include <PvGenericLoaderExtended.h>

CChangePhaseTimeZeroDateTimeManager::CChangePhaseTimeZeroDateTimeManager(const HPATCON hPatCon)
	: m_hPatCon(hPatCon)
{

}

void CChangePhaseTimeZeroDateTimeManager::UpdatePhaseScheduleOnChangePhaseTimeZero(IPhase& phase)
{
	ICalendarPtr pIPhaseStartDateTime = phase.GetUTCStartDtTm();
	const Cerner::Foundations::Calendar phaseStartDateTime = PvOrdCalUtils::GetDateOnlyCalendarFromInterface(
				pIPhaseStartDateTime);

	ICalendarPtr pIPhaseTimeZeroDateTime = phase.GetUTCCalcTimeZeroDtTm();
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
			const CString sComponentTypeMeaning = (LPCTSTR)pIComponent->GetComponentTypeMean();

			if (sComponentTypeMeaning == "ORDER CREATE")
			{
				PvOrderObj* pOrderObj = GetOrderWithUpdatedSchedule(*pIComponent, phaseStartDateTime, phaseTimeZeroDateTime,
										phaseStopDateTime);

				if (NULL == pOrderObj)
				{
					pIComponent->PutTZActiveInd(FALSE);
					pIComponent->PutModifiedTZRelationInd(TRUE);
					continue;
				}

				pOrderObj->SetFmtActionCd(CDF::OrderAction::GetRescheduleCd());

				affectedOrders.push_back(pOrderObj);
			}
			else if (sComponentTypeMeaning == "SUBPHASE")
			{
				IPhasePtr pISubPhase = pIComponent->GetSubphaseDispatch();

				if (NULL != pISubPhase)
				{
					UpdateSubPhaseSchedule(*pISubPhase, phaseStartDateTime, phaseTimeZeroDateTime, phaseStopDateTime, affectedOrders);
				}
			}
		}
	}

	CallInpatientOrdersScheduleService(affectedOrders);

	CProtocolOrderScheduleManager protocolOrderScheduleManager(m_hPatCon);
	protocolOrderScheduleManager.UpdateProtocolSchedule(phase);
}

/////////////////////////////////////////////////////////////////////////////
/// \fn		void CChangePhaseTimeZeroDateTimeManager::CallInpatientOrdersScheduleService(std::list<PvOrderObj*>& timeZeroOrders)
/// \brief		Calls the appropriate schedule services for the given inpatient orders.
///
/// \return		void
///
/// \param[in]	std::list<PvOrderObj*>& timeZeroOrders - The inpatient orders that had time-zero links.
/// \owner		DB024248
/////////////////////////////////////////////////////////////////////////////
void CChangePhaseTimeZeroDateTimeManager::CallInpatientOrdersScheduleService(std::list<PvOrderObj*>& timeZeroOrders)
{
	CInpatientOrderScheduleServiceCaller inpatientOrderScheduleService(m_hPatCon);

	std::list<PvOrderObj*> newOrders;
	std::list<PvOrderObj*> modifiedOrders;
	std::list<PvOrderObj*> activatedOrders;

	for (auto orderIter = timeZeroOrders.cbegin(); orderIter != timeZeroOrders.cend(); orderIter++)
	{
		PvOrderObj* pOrderObj = *orderIter;

		const double dFmtActionCd = pOrderObj->GetFmtActionCd();

		if (CDF::OrderAction::IsOrder(dFmtActionCd))
		{
			newOrders.push_back(pOrderObj);
		}
		else if (CDF::OrderAction::IsModify(dFmtActionCd) || CDF::OrderAction::IsReschedule(dFmtActionCd))
		{
			modifiedOrders.push_back(pOrderObj);
		}
		else if (CDF::OrderAction::IsActivate(dFmtActionCd))
		{
			activatedOrders.push_back(pOrderObj);
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
/// \fn		void CChangePhaseTimeZeroDateTimeManager::UpdateSubPhaseSchedule(IPhase& subPhase, const Cerner::Foundations::Calendar& phaseStartDateTime, const Cerner::Foundations::Calendar& phaseTimeZeroDateTime, const Cerner::Foundations::Calendar& phaseStopDateTime, std::list<PvOrderObj*>& orders)
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
void CChangePhaseTimeZeroDateTimeManager::UpdateSubPhaseSchedule(IPhase& subPhase,
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
				PvOrderObj* pOrderObj = GetOrderWithUpdatedSchedule(*pIComponent, subPhaseStartDateTime, subPhaseTimeZeroDateTime,
										subPhaseStopDateTime);

				if (NULL != pOrderObj)
				{
					orders.push_back(pOrderObj);
				}
			}
			else if (sComponentTypeMeaning == "PRESCRIPTION")
			{
				UpdatePrescriptionComponentSchedule(*pIComponent, subPhaseStartDateTime);
			}
			else if (sComponentTypeMeaning == "RESULT OUTCO")
			{
				UpdateOutcomeComponentSchedule(subPhase, *pIComponent);
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
/// \fn		PvOrderObj* CChangePhaseTimeZeroDateTimeManager::GetOrderWithUpdatedSchedule(IComponent& orderComponent, const Cerner::Foundations::Calendar& phaseStartDateTime, const Cerner::Foundations::Calendar& phaseTimeZeroDateTime, const Cerner::Foundations::Calendar& phaseStopDateTime)
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
PvOrderObj* CChangePhaseTimeZeroDateTimeManager::GetOrderWithUpdatedSchedule(IComponent& orderComponent,
		const Cerner::Foundations::Calendar& phaseStartDateTime, const Cerner::Foundations::Calendar& phaseTimeZeroDateTime,
		const Cerner::Foundations::Calendar& phaseStopDateTime)
{
	const CModifiableOrderRetriever modifiableOrderRetriever(m_hPatCon);
	PvOrderObj* pOrderObj = modifiableOrderRetriever.GetModifiableOrderFromComponent(orderComponent);

	if (NULL != pOrderObj)
	{
		Cerner::Foundations::Calendar actionDateTime = CActionDateTimeHelper::GetActionDateTime(*pOrderObj);

		if (actionDateTime > phaseTimeZeroDateTime)
		{
			pOrderObj->SetActionDtTm(phaseTimeZeroDateTime);
		}

		CChangePhaseStartOrderScheduleManager changePhaseStartOrderScheduleManager;
		const bool bOrderSuccess = changePhaseStartOrderScheduleManager.UpdateOrderScheduleOnChangePhaseStart(orderComponent,
								   *pOrderObj, phaseStartDateTime, phaseTimeZeroDateTime, phaseStopDateTime);

		return pOrderObj;
	}

	return NULL;
}

/////////////////////////////////////////////////////////////////////////////
/// \fn		void CChangePhaseTimeZeroDateTimeManager::UpdatePrescriptionComponentSchedule(IComponent& prescriptionComponent, const Cerner::Foundations::Calendar& phaseStartDateTime)
/// \brief		Updates the prescription component's schedule.
///
/// \return		void
///
/// \param[in]	IComponent& prescriptionComponent - The prescription component whose schedule needs updated.
/// \param[in]	const Cerner::Foundations::Calendar& phaseStartDateTime - The start date/time of the phase that contains the prescription component.
/// \owner		DB024248
/////////////////////////////////////////////////////////////////////////////
void CChangePhaseTimeZeroDateTimeManager::UpdatePrescriptionComponentSchedule(IComponent& prescriptionComponent,
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
/// \fn		void CChangePhaseTimeZeroDateTimeManager::UpdateOutcomeComponentSchedule(IPhase& phase, IComponent& outcomeComponent)
/// \brief		Updates the outcome component's schedule.
///
/// \return		void
///
/// \param[in]	IPhase& phase - The phase containing the outcome component whose schedule needs updated.
/// \param[in]	IComponent& outcomeComponent - The outcome component whose schedule needs updated.
/// \owner		DB024248
/////////////////////////////////////////////////////////////////////////////
void CChangePhaseTimeZeroDateTimeManager::UpdateOutcomeComponentSchedule(IPhase& phase, IComponent& outcomeComponent)
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