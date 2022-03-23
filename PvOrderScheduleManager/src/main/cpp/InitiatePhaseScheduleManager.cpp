#include "StdAfx.h"

#include "InitiatePhaseScheduleManager.h"
#include "InitiateOrderScheduleManager.h"
#include "PrescriptionScheduleManager.h"
#include "InpatientOrderScheduleServiceCaller.h"
#include "SubPhaseScheduleManager.h"
#include "ProtocolOrderScheduleManager.h"
#include "PhaseTimeZeroDateTimeDeterminer.h"
#include "OutcomeScheduleManager.h"
#include "OrderComponentRetriever.h"
#include "PrescriptionComponentRetriever.h"
#include "OutcomeComponentRetriever.h"
#include "SubPhaseRetriever.h"

#include <pvorderobj.h>
#include <pvorderpcobj.h>
#include <RTZDefs.h>
#include <PvOrdCalUtil.h>
#include <srvcalendar.h>
#include <CPS_ImportPVCareCoordCom.h>
#include <CPS_ImportCpsDataCommon.h>
#include <ActionDateTimeHelper.h>
#include <FutureOrderUtil.h>

CInitiatePhaseScheduleManager::CInitiatePhaseScheduleManager(const HPATCON hPatCon)
	: m_hPatCon(hPatCon)
{

}

void CInitiatePhaseScheduleManager::UpdatePhaseScheduleOnInitiate(IPhase& phase)
{
	const bool bHasTreatmentSchedule = phase.HasTreatmentSchedule() == TRUE;

	if (bHasTreatmentSchedule)
	{
		bool bSuccess = true;
		IPlanPtr pIPlan = phase.GetParentDispatch();

		if (pIPlan != NULL)
		{
			TreatmentPeriodVector treatmentPeriodVector;
			phase.GetTreatmentPeriods((LONG_PTR)&treatmentPeriodVector);

			for (TreatmentPeriodVector::const_iterator itr(treatmentPeriodVector.begin()); itr != treatmentPeriodVector.end();
				 itr++)
			{
				IPhasePtr pITreatmentPeriod(pIPlan->GetPhaseByKey((*itr).second));

				if (pITreatmentPeriod != NULL)
				{
					UpdatePhaseScheduleOnInitiateInternal(*pITreatmentPeriod);
				}
			}
		}

		CProtocolOrderScheduleManager protocolOrderScheduleManager(m_hPatCon);
		protocolOrderScheduleManager.UpdateProtocolSchedule(phase);
	}
	else
	{
		UpdatePhaseScheduleOnInitiateInternal(phase);
	}
}

void CInitiatePhaseScheduleManager::UpdatePhaseScheduleOnInitiateInternal(IPhase& phase)
{
	bool bSuccess = true;

	ICalendarPtr pIPhaseStartDateTime = phase.GetUTCStartDtTm();
	const Cerner::Foundations::Calendar phaseStartDateTime = PvOrdCalUtils::GetDateOnlyCalendarFromInterface(
				pIPhaseStartDateTime);

	CPhaseTimeZeroDateTimeDeterminer phaseTimeZeroDateTimeDeterminer;
	ICalendarPtr pIPhaseTimeZeroDateTime = phaseTimeZeroDateTimeDeterminer.DetermineTimeZeroDateTime(phase);
	phase.PutUTCCalcTimeZeroDtTm(pIPhaseTimeZeroDateTime);
	const Cerner::Foundations::Calendar phaseTimeZeroDateTime = PvOrdCalUtils::GetDateOnlyCalendarFromInterface(
				pIPhaseTimeZeroDateTime);

	phase.CalculateEndDtTm();
	ICalendarPtr pIPhaseStopDateTime = phase.GetUTCCalcEndDtTm();
	const Cerner::Foundations::Calendar phaseStopDateTime = PvOrdCalUtils::GetDateOnlyCalendarFromInterface(
				pIPhaseStopDateTime);

	std::list<IComponent*> orderComponents;
	COrderComponentRetriever::GetIncludedOrderComponents(phase, orderComponents);

	std::list<PvOrderObj*> orders;
	UpdateComponentSchedulesOnInitiate(orderComponents, orders, phaseStartDateTime, phaseTimeZeroDateTime,
									   phaseStopDateTime);

	std::list<IComponent*> prescriptionComponents;
	CPrescriptionComponentRetriever::GetIncludedPrescriptionComponents(phase, prescriptionComponents);

	UpdatePrescriptionComponentSchedulesOnInitiate(prescriptionComponents, phaseStartDateTime);

	std::list<IComponent*> outcomeComponents;
	COutcomeComponentRetriever::GetIncludedOutcomeComponents(phase, outcomeComponents);

	UpdateOutcomeComponentSchedulesOnInitiate(phase, outcomeComponents);

	std::list<IPhase*> subPhases;
	CSubPhaseRetriever::GetIncludedSubPhases(phase, subPhases);

	CSubPhaseScheduleManager subPhaseScheduleManager;

	for (auto subPhaseIter = subPhases.cbegin(); subPhaseIter != subPhases.cend(); subPhaseIter++)
	{
		IPhase* pISubPhase = *subPhaseIter;

		subPhaseScheduleManager.UpdateSubPhaseSchedule(*pISubPhase, phaseStartDateTime, phaseTimeZeroDateTime,
				phaseStopDateTime);

		ICalendarPtr pISubPhaseStartDateTime = pISubPhase->GetUTCStartDtTm();
		const Cerner::Foundations::Calendar subPhaseStartDateTime = PvOrdCalUtils::GetDateOnlyCalendarFromInterface(
					pISubPhaseStartDateTime);

		const Cerner::Foundations::Calendar subPhaseTimeZeroDateTime = Cerner::Foundations::Calendar::CreateNull();

		ICalendarPtr pISubPhaseStopDateTime = pISubPhase->GetUTCCalcEndDtTm();
		const Cerner::Foundations::Calendar subPhaseStopDateTime = PvOrdCalUtils::GetDateOnlyCalendarFromInterface(
					pISubPhaseStopDateTime);

		std::list<IComponent*> subPhaseOrderComponents;
		COrderComponentRetriever::GetIncludedOrderComponents(*pISubPhase, subPhaseOrderComponents);

		UpdateComponentSchedulesOnInitiate(subPhaseOrderComponents, orders, subPhaseStartDateTime, subPhaseTimeZeroDateTime,
										   subPhaseStopDateTime);

		std::list<IComponent*> subPhasePrescriptionComponents;
		CPrescriptionComponentRetriever::GetIncludedPrescriptionComponents(*pISubPhase, subPhasePrescriptionComponents);

		UpdatePrescriptionComponentSchedulesOnInitiate(subPhasePrescriptionComponents, subPhaseStartDateTime);

		std::list<IComponent*> subPhaseOutcomeComponents;
		COutcomeComponentRetriever::GetIncludedOutcomeComponents(*pISubPhase, subPhaseOutcomeComponents);

		UpdateOutcomeComponentSchedulesOnInitiate(*pISubPhase, subPhaseOutcomeComponents);
	}

	CInpatientOrderScheduleServiceCaller inpatientOrderScheduleService(m_hPatCon);
	inpatientOrderScheduleService.CalculateNewOrderSchedule(orders, CalculateNewOrderScheduleRequest::eNewOrderAdded);
}

void CInitiatePhaseScheduleManager::UpdateComponentSchedulesOnInitiate(const std::list<IComponent*>& components,
		std::list<PvOrderObj*>& orders, const Cerner::Foundations::Calendar& phaseStartDateTime,
		const Cerner::Foundations::Calendar& phaseTimeZeroDateTime, const Cerner::Foundations::Calendar& phaseStopDateTime)
{
	for (auto componentIter = components.cbegin(); componentIter != components.cend(); componentIter++)
	{
		IComponent* pIComponent = *componentIter;
		LONG_PTR lpOrderObj = pIComponent->GetPvOrderObj();

		if (NULL == lpOrderObj)
		{
			lpOrderObj = pIComponent->GetOrderProposal();
		}

		PvOrderObj* pOrderObj = (PvOrderObj*)lpOrderObj;

		if (NULL != pOrderObj)
		{
			PvOrderPCObj* pOrderPCObj = dynamic_cast<PvOrderPCObj*>(pOrderObj);

			if (NULL != pOrderPCObj)
			{
				if (pOrderPCObj->GetDummyParentFlag())
				{
					// Schedule cannot be calculated, since this is an additive only order, with a "dummy" diluent and no true format to use.
					continue;
				}
			}

			Cerner::Foundations::Calendar actionDateTime = CActionDateTimeHelper::GetActionDateTime(*pOrderObj);

			if (actionDateTime > phaseStartDateTime)
			{
				pOrderObj->SetActionDtTm(phaseStartDateTime);
			}

			// If the order with format defaults for future recurring order details is part of DOT, then those details will be cleared and will not be treated a future recurring order.
			if (!CFutureOrderUtil::IsRecurringFutureOrder(pOrderObj) || pOrderObj->IsProtocolOrder()
				|| pOrderObj->IsDayOfTreatmentOrder())
			{
				orders.push_back(pOrderObj);
			}

			CInitiateOrderScheduleManager initiateOrderScheduleManager;
			initiateOrderScheduleManager.UpdateOrderScheduleOnInitiate(*pIComponent, *pOrderObj, phaseStartDateTime,
					phaseTimeZeroDateTime, phaseStopDateTime);
		}
	}
}

void CInitiatePhaseScheduleManager::UpdatePrescriptionComponentSchedulesOnInitiate(const std::list<IComponent*>&
		prescriptionComponents, const Cerner::Foundations::Calendar& phaseStartDateTime)
{
	CPrescriptionScheduleManager prescriptionScheduleManager(m_hPatCon);

	for (auto componentIter = prescriptionComponents.cbegin(); componentIter != prescriptionComponents.cend();
		 componentIter++)
	{
		IComponent* pIComponent = *componentIter;
		LONG_PTR lpOrderObj = pIComponent->GetPvOrderObj();

		if (NULL == lpOrderObj)
		{
			lpOrderObj = pIComponent->GetOrderProposal();
		}

		PvOrderObj* pOrderObj = (PvOrderObj*)lpOrderObj;

		if (NULL != pOrderObj)
		{
			Cerner::Foundations::Calendar actionDateTime = CActionDateTimeHelper::GetActionDateTime(*pOrderObj);

			if (actionDateTime > phaseStartDateTime)
			{
				pOrderObj->SetActionDtTm(phaseStartDateTime);
			}

			prescriptionScheduleManager.UpdatePrescriptionSchedule(*pIComponent, *pOrderObj, phaseStartDateTime);
		}
	}
}

void CInitiatePhaseScheduleManager::UpdateOutcomeComponentSchedulesOnInitiate(IPhase& phase,
		const std::list<IComponent*>& components)
{
	COutcomeScheduleManager outcomeScheduleManager(m_hPatCon);

	for (auto componentIter = components.cbegin(); componentIter != components.cend(); componentIter++)
	{
		IComponent* pIComponent = *componentIter;
		IOutcomePtr pOutcome = pIComponent->GetDispatch();

		if (NULL != pOutcome)
		{
			outcomeScheduleManager.UpdateOutcomeSchedule(phase, *pIComponent, *pOutcome);
		}
	}
}