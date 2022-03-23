#include "StdAfx.h"

#include "LinkedToPhaseDurationUpdater.h"

#include <srvcalendar.h>
#include <PvOrdCalUtil.h>
#include <dcp_atlutils.h>
#include <CPS_ImportPVCareCoordCom.h>
#include <PvScheduleTransactionAssistantExp.h>

void CLinkedToPhaseDurationUpdater::UpdateLinkedToPhaseDurationOrders(const std::list<PvOrderObj*>& orders) const
{
	std::list<PvOrderObj*> updatedOrders;

	for (auto orderIter = orders.cbegin(); orderIter != orders.cend(); orderIter++)
	{
		PvOrderObj* pOrderObj = *orderIter;

		IComponentPtr pIComponent = TransferDispatch(pOrderObj->GetPlanComponent());

		if (NULL != pIComponent)
		{
			IPhase* pIPhase = PowerPlanUtil::GetPhase(pIComponent);

			if (NULL != pIPhase)
			{
				const bool bLinkedToPhaseDuration = pIComponent->GetLinkToPhase() == TRUE;

				if (bLinkedToPhaseDuration)
				{
					const bool bStopDateTimeMoved = UpdateLinkOnCurrentlyLinkedOrder(*pIPhase, *pIComponent, *pOrderObj);

					if (bStopDateTimeMoved)
					{
						if (pOrderObj->GetFreqTypeFlag() != eFTFOneTime)
						{
							updatedOrders.push_back(pOrderObj);
						}
					}
				}
			}
		}
	}

	CallInpatientOrdersScheduleService(updatedOrders);
}

bool CLinkedToPhaseDurationUpdater::UpdateLinkOnCurrentlyLinkedOrder(IPhase& phase, IComponent& component,
		PvOrderObj& orderObj) const
{
	ICalendarPtr pIPhaseStopDateTime = phase.GetUTCCalcEndDtTm();
	Cerner::Foundations::Calendar phaseStopDateTime = PvOrdCalUtils::GetDateOnlyCalendarFromInterface(pIPhaseStopDateTime);

	if (!phaseStopDateTime.IsNull())
	{
		PvOrderFld* pStopDateTimeFld = orderObj.m_orderFldArr.GetFieldFromMeanId(eDetailOrdStopDtTm);

		if (NULL != pStopDateTimeFld)
		{
			const double dFmtActionCd = orderObj.GetFmtActionCd();

			if (CDF::OrderAction::IsOrder(dFmtActionCd) || CDF::OrderAction::IsModify(dFmtActionCd)
				|| CDF::OrderAction::IsReschedule(dFmtActionCd))
			{
				const double dDurationValue = orderObj.m_orderFldArr.GetFieldValFromMeanId(eDetailOrdDuration);

				if (dDurationValue == 0.0)
				{
					const Cerner::Foundations::Calendar orderStopDateTime = pStopDateTimeFld->GetLastOeFldDtTmValue();

					if (!orderStopDateTime.IsNull())
					{
						if (phaseStopDateTime == orderStopDateTime)
						{
							return false;
						}
					}

					const Cerner::Foundations::Calendar orderStartDateTime = orderObj.m_orderFldArr.GetFieldDtTmValFromMeanId(
								eDetailReqStartDtTm);

					if (!orderStartDateTime.IsNull())
					{
						if (phaseStopDateTime >= orderStartDateTime)
						{
							const bool bIsPRN = orderObj.m_orderFldArr.GetFieldValFromMeanId(eDetailSchPrn) == 1;
							// It sets the stop date/time of the order to the phase stop date/time, if it is not a One-Time order 
							// or it is an One-Time and PRN order .
							if (!(orderObj.GetFreqTypeFlag() == eFTFOneTime && !bIsPRN))
							{
								pStopDateTimeFld->AddOeFieldDtTmValue(phaseStopDateTime);
								return true;
							}
							
							return false;
						}
					}
				}
			}
		}
	}
	return false;
}

void CLinkedToPhaseDurationUpdater::CallInpatientOrdersScheduleService(std::list<PvOrderObj*>& orders) const
{
	std::list<CCalculateNewOrderScheduleCriteria> newOrderCriteriaList;
	std::list<CCalculateModifyOrderScheduleCriteria> modifyOrderCriteriaList;

	for (auto orderIter = orders.cbegin(); orderIter != orders.cend(); orderIter++)
	{
		PvOrderObj* pOrderObj = *orderIter;

		const double dFmtActionCd = pOrderObj->GetFmtActionCd();

		if (CDF::OrderAction::IsOrder(dFmtActionCd))
		{
			CCalculateNewOrderScheduleCriteria newOrderScheduleCriteria(pOrderObj,
					CalculateNewOrderScheduleRequest::eStopDateTimeChanged, 3);
			newOrderCriteriaList.push_back(newOrderScheduleCriteria);
		}
		else if (CDF::OrderAction::IsModify(dFmtActionCd) || CDF::OrderAction::IsReschedule(dFmtActionCd))
		{
			CCalculateModifyOrderScheduleCriteria modifyOrderScheduleCriteria(pOrderObj,
					CalculateModifyOrderScheduleRequest::eStopDateTimeChanged, 3);
			modifyOrderCriteriaList.push_back(modifyOrderScheduleCriteria);
		}
	}

	PvScheduleTransactionAssistant::CalculateNewOrderSchedule(newOrderCriteriaList);
	PvScheduleTransactionAssistant::CalculateModifyOrderSchedule(modifyOrderCriteriaList);
}