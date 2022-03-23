#include "StdAfx.h"


#include "ActivatePhaseScheduleManager.h"
#include "ActivateOrderScheduleManager.h"
#include "PrescriptionScheduleManager.h"
#include "InpatientOrderScheduleServiceCaller.h"
#include "SubPhaseScheduleManager.h"
#include "ProtocolOrderScheduleManager.h"
#include "PhaseTimeZeroDateTimeDeterminer.h"
#include "OrderComponentRetriever.h"
#include "PrescriptionComponentRetriever.h"
#include "SubPhaseRetriever.h"
#include "PharmacyPriorityHelper.h"

#include <pvorderobj.h>
#include <PvOrderProtocolObj.h>
#include <PvOrdCalUtil.h>
#include <srvcalendar.h>
#include <uiconstants.h>
#include <CPS_ImportPVCareCoordCom.h>
#include <CPS_ImportCpsDataCommon.h>
#include <ActionDateTimeHelper.h>
#include <dcp_genericloader.h>
#include <PvGenericLoaderExtended.h>

CActivatePhaseScheduleManager::CActivatePhaseScheduleManager(const HPATCON hPatCon)
	: m_hPatCon(hPatCon)
{

}

void CActivatePhaseScheduleManager::UpdatePhaseScheduleOnActivate(IPhase& phase)
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

	std::list<IComponent*> orderComponents;
	COrderComponentRetriever::GetIncludedOrderComponents(phase, orderComponents);

	std::list<CCalculateActivateOrderScheduleCriteria> activateOrderScheduleCriteria;
	UpdateComponentSchedulesOnActivate(orderComponents, activateOrderScheduleCriteria, phaseStartDateTime,
									   phaseTimeZeroDateTime, phaseStopDateTime);

	std::list<IComponent*> prescriptionComponents;
	CPrescriptionComponentRetriever::GetIncludedPrescriptionComponents(phase, prescriptionComponents);

	UpdatePrescriptionComponentSchedulesOnActivate(prescriptionComponents, phaseStartDateTime);

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

		UpdateComponentSchedulesOnActivate(subPhaseOrderComponents, activateOrderScheduleCriteria, subPhaseStartDateTime,
										   subPhaseTimeZeroDateTime, subPhaseStopDateTime);

		std::list<IComponent*> subPhasePrescriptionComponents;
		CPrescriptionComponentRetriever::GetIncludedPrescriptionComponents(*pISubPhase, subPhasePrescriptionComponents);

		UpdatePrescriptionComponentSchedulesOnActivate(subPhasePrescriptionComponents, subPhaseStartDateTime);
	}

	CInpatientOrderScheduleServiceCaller inpatientOrderScheduleService(m_hPatCon);
	inpatientOrderScheduleService.CalculateActivateOrderSchedule(activateOrderScheduleCriteria);

	CProtocolOrderScheduleManager protocolOrderScheduleManager(m_hPatCon);
	protocolOrderScheduleManager.UpdateProtocolSchedule(phase);
}

void CActivatePhaseScheduleManager::UpdateComponentSchedulesOnActivate(const std::list<IComponent*>& components,
		std::list<CCalculateActivateOrderScheduleCriteria>& activateOrderScheduleCriteria,
		const Cerner::Foundations::Calendar& phaseStartDateTime, const Cerner::Foundations::Calendar& phaseTimeZeroDateTime,
		const Cerner::Foundations::Calendar& phaseStopDateTime)
{
	for (auto componentIter = components.cbegin(); componentIter != components.cend(); componentIter++)
	{
		IComponent* pIComponent = *componentIter;

		PvOrderObj* pOrderObj = CGenLoader().GetOrderObjFromComponent(m_hPatCon, *pIComponent);

		if (NULL != pOrderObj)
		{
			const bool bFutureRecurring = pOrderObj->IsFutureRecurringOrder();

			if (bFutureRecurring)
			{
				pOrderObj = GetFutureRecurringChildOrderBeingActivated(*pOrderObj);

				if (NULL == pOrderObj)
				{
					continue;
				}
			}

			const double dFmtActionCd = pOrderObj->GetFmtActionCd();

			if (!CDF::OrderAction::IsActivate(dFmtActionCd))
			{
				continue;
			}

			Cerner::Foundations::Calendar actionDateTime = CActionDateTimeHelper::GetActionDateTime(*pOrderObj);

			if (actionDateTime > phaseStartDateTime)
			{
				pOrderObj->SetActionDtTm(phaseStartDateTime);
			}

			CActivateOrderScheduleManager activateOrderScheduleManager;
			CCalculateActivateOrderScheduleCriteria orderScheduleCriteria =
				activateOrderScheduleManager.UpdateOrderScheduleOnActivate(*pIComponent, *pOrderObj, phaseStartDateTime,
						phaseTimeZeroDateTime, phaseStopDateTime);
			activateOrderScheduleCriteria.push_back(orderScheduleCriteria);
		}
	}
}

void CActivatePhaseScheduleManager::UpdatePrescriptionComponentSchedulesOnActivate(const std::list<IComponent*>&
		prescriptionComponents, const Cerner::Foundations::Calendar& phaseStartDateTime)
{
	for (auto componentIter = prescriptionComponents.cbegin(); componentIter != prescriptionComponents.cend();
		 componentIter++)
	{
		IComponent* pIComponent = *componentIter;

		PvOrderObj* pOrderObj = CGenLoader().GetOrderObjFromComponent(m_hPatCon, *pIComponent);

		if (NULL != pOrderObj)
		{
			Cerner::Foundations::Calendar actionDateTime = CActionDateTimeHelper::GetActionDateTime(*pOrderObj);

			if (actionDateTime > phaseStartDateTime)
			{
				pOrderObj->SetActionDtTm(phaseStartDateTime);
			}

			CPrescriptionScheduleManager prescriptionScheduleManager(m_hPatCon);
			prescriptionScheduleManager.UpdatePrescriptionSchedule(*pIComponent, *pOrderObj, phaseStartDateTime);
		}
	}
}

PvOrderObj* CActivatePhaseScheduleManager::GetFutureRecurringChildOrderBeingActivated(PvOrderObj& futureRecurringOrder)
{
	PvOrderProtocolObj* pProtocolOrderObj = dynamic_cast<PvOrderProtocolObj*>(&futureRecurringOrder);

	if (NULL != pProtocolOrderObj)
	{
		std::list<PvOrderObj*> dotList;
		CGenLoader().GetDayOfTreatmentOrders(m_hPatCon, *pProtocolOrderObj, dotList);

		for (std::list<PvOrderObj*>::iterator itr(dotList.begin()), end(dotList.end()); itr != end; ++itr)
		{
			PvOrderObj* pChildOrderObj = *itr;

			if (NULL != pChildOrderObj)
			{
				// Confirm child order is in appropriate status
				const double dFmtActionCd = pChildOrderObj->GetFmtActionCd();

				if (CDF::OrderAction::IsActivate(dFmtActionCd))
				{
					return pChildOrderObj;
				}
			}
		}
	}

	return NULL;
}