#include "StdAfx.h"

#include "InpatientOrderScheduleServiceCaller.h"
#include "ConstantIndicatorHelper.h"
#include "IntervalChildrenScheduleHelper.h"
#include "OrderDurationHelper.h"
#include "DoTOrderDateTimeUpdater.h"
#include "LinkedToPhaseDurationUpdater.h"

#include <PvResumeScheduleControllerExp.h>
#include <PvResumeScheduleControllerFactoryExp.h>
#include <pvsingletonmgr.h>
#include <OrderDetailHelper.h>
#include <list>
#include <pvorderobj.h>
#include <pvorderpcobj.h>
#include <pvordutil.h>
#include <PvScheduleTransactionAssistantExp.h>
#include <PvGenericLoaderExtended.h>

CInpatientOrderScheduleServiceCaller::CInpatientOrderScheduleServiceCaller(const HPATCON hPatCon)
	: m_hPatCon(hPatCon)
{

}

bool CInpatientOrderScheduleServiceCaller::CalculateNewOrderSchedule(std::list<PvOrderObj*>& orders,
		const CalculateNewOrderScheduleRequest::ETriggeringActionFlag triggeringAction)
{
	std::list<CCalculateNewOrderScheduleCriteria> criteria;

	for (auto orderIter = orders.cbegin(); orderIter != orders.cend(); orderIter++)
	{
		PvOrderObj* pOrderObj = *orderIter;

		const short sNumberOfFollowingDosesToReturn = 3;

		CCalculateNewOrderScheduleCriteria orderCriteria(pOrderObj, triggeringAction, sNumberOfFollowingDosesToReturn);
		criteria.push_back(orderCriteria);
	}

	return CalculateNewOrderSchedule(criteria);
}

bool CInpatientOrderScheduleServiceCaller::CalculateNewOrderSchedule(std::list<CCalculateNewOrderScheduleCriteria>&
		criteria)
{
	std::list<PvOrderObj*> failedOrders = PvScheduleTransactionAssistant::CalculateNewOrderSchedule(criteria);

	if (!failedOrders.empty())
	{
		return false;
	}

	std::list<PvOrderObj*> orders;

	for (auto orderCriteriaIter = criteria.cbegin(); orderCriteriaIter != criteria.cend(); orderCriteriaIter++)
	{
		CCalculateNewOrderScheduleCriteria orderCriteria = *orderCriteriaIter;
		PvOrderObj* pOrderObj = orderCriteria.GetOrderObj();

		orders.push_back(pOrderObj);

		// Fuzzy Date Time
		if (PvOrdUtil::IsUnsignedFutureOrder(pOrderObj))
		{
			pOrderObj->SetStopDateTimeAsEstimated();
			pOrderObj->SetStartDateTimeAsEstimated();
		}
		else
		{
			pOrderObj->SetStopDateTimeAsPrecise();
			pOrderObj->SetStartDateTimeAsPrecise();
		}
	}

	HandleSuccessfulScheduleServiceCall(orders);

	return true;
}

bool CInpatientOrderScheduleServiceCaller::CalculateModifyOrderSchedule(std::list<PvOrderObj*>& orders,
		const CalculateModifyOrderScheduleRequest::ETriggeringActionFlag triggeringAction)
{
	std::list<CCalculateModifyOrderScheduleCriteria> criteria;

	for (auto orderIter = orders.cbegin(); orderIter != orders.cend(); orderIter++)
	{
		PvOrderObj* pOrderObj = *orderIter;

		const short sNumberOfFollowingDosesToReturn = 3;

		CCalculateModifyOrderScheduleCriteria orderCriteria(pOrderObj, triggeringAction, sNumberOfFollowingDosesToReturn);
		criteria.push_back(orderCriteria);
	}

	return CalculateModifyOrderSchedule(criteria);
}

bool CInpatientOrderScheduleServiceCaller::CalculateModifyOrderSchedule(
	std::list<CCalculateModifyOrderScheduleCriteria>& criteria)
{
	std::list<PvOrderObj*> failedOrders = PvScheduleTransactionAssistant::CalculateModifyOrderSchedule(criteria);

	if (!failedOrders.empty())
	{
		return false;
	}

	std::list<PvOrderObj*> orders;

	for (auto orderCriteriaIter = criteria.cbegin(); orderCriteriaIter != criteria.cend(); orderCriteriaIter++)
	{
		CCalculateModifyOrderScheduleCriteria orderCriteria = *orderCriteriaIter;
		PvOrderObj* pOrderObj = orderCriteria.GetOrderObj();

		orders.push_back(pOrderObj);
	}

	HandleSuccessfulScheduleServiceCall(orders);

	return true;
}

bool CInpatientOrderScheduleServiceCaller::CalculateRenewOrderSchedule(std::list<PvOrderObj*>& orders,
		const CalculateRenewOrderScheduleRequest::ETriggeringActionFlag triggeringAction)
{
	std::list<CCalculateRenewOrderScheduleCriteria> criteria;

	for (auto orderIter = orders.cbegin(); orderIter != orders.cend(); orderIter++)
	{
		PvOrderObj* pOrderObj = *orderIter;

		const short sNumberOfFollowingDosesToReturn = 3;

		CCalculateRenewOrderScheduleCriteria orderCriteria(pOrderObj, triggeringAction);
		criteria.push_back(orderCriteria);
	}

	return CalculateRenewOrderSchedule(criteria);
}

bool CInpatientOrderScheduleServiceCaller::CalculateRenewOrderSchedule(std::list<CCalculateRenewOrderScheduleCriteria>&
		criteria)
{
	std::list<PvOrderObj*> failedOrders = PvScheduleTransactionAssistant::CalculateRenewOrderSchedule(criteria);

	if (!failedOrders.empty())
	{
		return false;
	}

	std::list<PvOrderObj*> orders;

	for (auto orderCriteriaIter = criteria.cbegin(); orderCriteriaIter != criteria.cend(); orderCriteriaIter++)
	{
		CCalculateRenewOrderScheduleCriteria orderCriteria = *orderCriteriaIter;
		PvOrderObj* pOrderObj = orderCriteria.GetOrderObj();

		orders.push_back(pOrderObj);
	}

	HandleSuccessfulScheduleServiceCall(orders);

	return true;
}

bool CInpatientOrderScheduleServiceCaller::CalculateActivateOrderSchedule(std::list<PvOrderObj*>& orders,
		const CalculateActivateOrderScheduleRequest::ETriggeringActionFlag triggeringAction)
{
	std::list<CCalculateActivateOrderScheduleCriteria> criteria;

	for (auto orderIter = orders.cbegin(); orderIter != orders.cend(); orderIter++)
	{
		PvOrderObj* pOrderObj = *orderIter;

		const short sNumberOfFollowingDosesToReturn = 3;

		CCalculateActivateOrderScheduleCriteria orderCriteria(pOrderObj, triggeringAction, sNumberOfFollowingDosesToReturn);
		criteria.push_back(orderCriteria);
	}

	return CalculateActivateOrderSchedule(criteria);
}

bool CInpatientOrderScheduleServiceCaller::CalculateActivateOrderSchedule(
	std::list<CCalculateActivateOrderScheduleCriteria>& criteria)
{
	std::list<PvOrderObj*> failedOrders = PvScheduleTransactionAssistant::CalculateActivateOrderSchedule(criteria);

	if (!failedOrders.empty())
	{
		return false;
	}

	std::list<PvOrderObj*> orders;

	for (auto orderCriteriaIter = criteria.cbegin(); orderCriteriaIter != criteria.cend(); orderCriteriaIter++)
	{
		CCalculateActivateOrderScheduleCriteria orderCriteria = *orderCriteriaIter;
		PvOrderObj* pOrderObj = orderCriteria.GetOrderObj();

		orders.push_back(pOrderObj);
	}

	HandleSuccessfulScheduleServiceCall(orders);

	return true;
}

bool CInpatientOrderScheduleServiceCaller::CalculateResumeOrderSchedule(std::list<PvOrderObj*>& orders,
		const CalculateResumeOrderScheduleRequest::ETriggeringActionFlag triggeringAction)
{
	std::list<CCalculateResumeOrderScheduleCriteria> criteria;

	for (auto orderIter = orders.cbegin(); orderIter != orders.cend(); orderIter++)
	{
		PvOrderObj* pOrderObj = *orderIter;

		const short sNumberOfFollowingDosesToReturn = 3;
		Cerner::Foundations::Calendar prevChartAdminDtTm(Cerner::Foundations::Calendar::CreateNull());

		CPvResumeScheduleControllerFactoryExp* pFactory = NULL;
		GetSingleInstance("RESUMESCHEDULEFACTORY_KEY", pFactory, AppCon_GetContext());

		if (NULL != pFactory)
		{
			// We need this pointer simply to sync the resume dt/tm with the schedule object.
			//pResumeController will be empty as we are removing the Resume Schedule Controller object when plan,sign is taken

			CPvResumeScheduleControllerExp* pResumeController = pFactory->GetResumeScheduleController(*pOrderObj);

			if (NULL != pResumeController)
			{
				bool bIsVDPA(false);
				VDPADoseStruct vdpaDoseStruct;
				eAdminType adminType(eNoAdminType);
				pResumeController->GetPreviouslyScheduledChartedAdministration(prevChartAdminDtTm, adminType, bIsVDPA,
						vdpaDoseStruct);
			}
		}

		CCalculateResumeOrderScheduleCriteria orderCriteria(pOrderObj, triggeringAction, sNumberOfFollowingDosesToReturn,
				prevChartAdminDtTm);
		criteria.push_back(orderCriteria);
	}

	return CalculateResumeOrderSchedule(criteria);
}

bool CInpatientOrderScheduleServiceCaller::CalculateResumeOrderSchedule(
	std::list<CCalculateResumeOrderScheduleCriteria>& criteria)
{
	std::list<PvOrderObj*> failedOrders = PvScheduleTransactionAssistant::CalculateResumeOrderSchedule(criteria);

	if (!failedOrders.empty())
	{
		return false;
	}

	std::list<PvOrderObj*> orders;

	for (auto orderCriteriaIter = criteria.cbegin(); orderCriteriaIter != criteria.cend(); orderCriteriaIter++)
	{
		CCalculateResumeOrderScheduleCriteria orderCriteria = *orderCriteriaIter;
		PvOrderObj* pOrderObj = orderCriteria.GetOrderObj();

		orders.push_back(pOrderObj);
	}

	HandleSuccessfulScheduleServiceCall(orders);

	return true;
}

void CInpatientOrderScheduleServiceCaller::HandleSuccessfulScheduleServiceCall(std::list<PvOrderObj*>& orders)
{
	const CLinkedToPhaseDurationUpdater linkedToPhaseDurationUpdater;
	linkedToPhaseDurationUpdater.UpdateLinkedToPhaseDurationOrders(orders);

	for (PvOrderObj* pOrderObj : orders)
	{
		if (pOrderObj == nullptr)
		{
			continue;
		}

		PvOrderFld* pStopDtTmFld = pOrderObj->m_orderFldArr.GetFieldFromMeanId(eDetailOrdStopDtTm);

		if (NULL != pStopDtTmFld)
		{
			pStopDtTmFld->SetVisualValueMask(VisualValue::eOeVShow);
		}

		PvOrderFld* pNextDoseDtTmFld = pOrderObj->m_orderFldArr.GetFieldFromMeanId(eDetailNextDoseDtTm);

		if (NULL != pNextDoseDtTmFld)
		{
			pNextDoseDtTmFld->SetVisualValueMask(VisualValue::eOeVShow);
		}

		CConstantIndicatorHelper::UpdateConstantIndicatorAcceptFlag(*pOrderObj);

		const double dFmtActionCd = pOrderObj->GetFmtActionCd();

		if (!CDF::OrderAction::IsActivate(dFmtActionCd))
		{
			if (!pOrderObj->IsIV())
			{
				if (COrderDurationHelper::IsDurationInDoses(*pOrderObj))
				{
					COrderDetailHelper::SetAcceptFlag(*pOrderObj, eDetailOrdFrequency, eOeAcceptRequired);
				}
				else if (!pOrderObj->IsIntermittent())
				{
					COrderDetailHelper::ResetAcceptFlag(*pOrderObj, eDetailOrdFrequency);
				}
			}
		}

		PvOrderPCObj* pOrderPCObj = dynamic_cast<PvOrderPCObj*>(pOrderObj);

		if (NULL != pOrderPCObj)
		{
			if (pOrderPCObj->IsIntervalBaselineComponent())
			{
				CIntervalChildrenScheduleHelper intervalChildrenScheduleHelper(m_hPatCon);
				const ERelatedFldsStatus intervalChildrenStatus = intervalChildrenScheduleHelper.SetReqStartDtTmForIntervalChildren(
							*pOrderPCObj);
			}
		}

		const bool bIsVDPA = pOrderObj->GetVDPA();

		if (bIsVDPA)
		{
			CPvVDPAOb currentVDPAObj = pOrderObj->GetCurrentVDPAObject();

			CScheduleObj scheduleObj(*pOrderObj);
			currentVDPAObj.SetNextDoseBucket(scheduleObj.GetStartDtTmScheduleSequence());

			pOrderObj->SetCurrentVDPAObject(currentVDPAObj);
		}

		PoeGenericStruct poeGenericStruct;
		poeGenericStruct.val1 = pOrderObj->GetId();

		if (EReviewScheduleAlert::eReviewScheduleAlert == pOrderObj->GetReviewScheduleAlert())
		{
			poeGenericStruct.val2 = TRUE;
		}
		else
		{
			poeGenericStruct.val2 = FALSE;
		}

		CPvGenericLoaderExtended(m_hPatCon).FireOrderPlanManagerEvent(eDisplayScheduleAlert, (LONG_PTR)&poeGenericStruct);

		CDoTOrderDateTimeUpdater dotOrderDateTimeUpdater;
		dotOrderDateTimeUpdater.UpdateDoTOrderDateTimes(*pOrderObj);

		pOrderObj->BuildDisplayLines();

		// Copy the current values to the previous values so we can determine what has changed.
		pOrderObj->CopyCurrAdhocToPrevAdhoc();
		pOrderObj->m_orderFldArr.CopyCurrValuesToPrevValues();
	}
}