#include "StdAfx.h"

#include "SchedulingInstructionsChangePlanOrderScheduleUpdater.h"
#include "InpatientOrderScheduleServiceCaller.h"
#include "ProtocolOrderScheduleManager.h"
#include "ComponentOffsetAndPriorityCombiner.h"
#include "GenericPriorityOffsetDeterminer.h"
#include "CollectionPriorityOffsetDeterminer.h"
#include "OrderStartDateTimeUpdater.h"
#include "PharmacyPriorityHelper.h"
#include "SchedulingInstructionsHelper.h"
#include "PlannedComponentDeterminer.h"

#include <OrderDetailHelper.h>
#include <pvorderobj.h>
#include <PvOrderProtocolObj.h>
#include <FutureOrderUtil.h>
#include <PvOrdCalUtil.h>
#include <pvorderdataexp.h>
#include <PvDateTimeOffsetUtil.h>
#include <CPS_ImportPVCareCoordCom.h>
#include <ActionDateTimeHelper.h>
#include <PvGenericLoaderExtended.h>
#include <dcp_genericloader.h>

CSchedulingInstructionsChangePlanOrderScheduleUpdater::CSchedulingInstructionsChangePlanOrderScheduleUpdater(
	const HPATCON hPatCon)
	: m_hPatCon(hPatCon)
{

}

/////////////////////////////////////////////////////////////////////////////
/// \fn		bool CSchedulingInstructionsChangePlanOrderScheduleUpdater::UpdatePlanOrderScheduleOnSchedulingInstructionsChange(IComponent& component, PvOrderObj& orderObj)
/// \brief		Handles updating the schedule of the given plan order when its scheduling instructions detail is manually changed.
///
/// \return		bool - True if the schedule was updated successfully. Otherwise, false.
///
/// \param[in]	IComponent& component - The component of the order whose scheduling instructions changed.
/// \param[in]	PvOrderObj& orderObj - The order whose scheduling instructions changed.
/// \owner		DB024248
/////////////////////////////////////////////////////////////////////////////
bool CSchedulingInstructionsChangePlanOrderScheduleUpdater::UpdatePlanOrderScheduleOnSchedulingInstructionsChange(
	IComponent& component, PvOrderObj& orderObj)
{
	const bool bPlannedOrderComponent = CPlannedComponentDeterminer::IsPlannedOrderComponent(orderObj);

	if (bPlannedOrderComponent)
	{
		return UpdatePlannedOrderScheduleOnSchedulingInstructionsChange(component, orderObj);
	}
	else
	{
		return UpdateInitiatedOrderScheduleOnSchedulingInstructionsChange(component, orderObj);
	}
}

bool CSchedulingInstructionsChangePlanOrderScheduleUpdater::UpdatePlannedOrderScheduleOnSchedulingInstructionsChange(
	IComponent& component, PvOrderObj& orderObj)
{
	const double dScheduleInstructionsCd = orderObj.m_orderFldArr.GetFieldValFromMeanId(eDetailSchedInstructions);

	if (dScheduleInstructionsCd > 0.0)
	{
		// Execute script to retrieve scheduling instructions details.
		SSchdlInstrs scheduleInstructions;

		if (!GetSchdlInstr(dScheduleInstructionsCd, scheduleInstructions))
		{
			MsgWriteF(MSG_DEFAULT, eMsgLog_Commit, MSG_EVENT_NAME, eMsgLvl_Warning,
					  _T("Unable to get schedule instructions for %.0f."), dScheduleInstructionsCd);
			return false;
		}

		if (!scheduleInstructions.defaultStartDtTm.IsEmpty())
		{
			if (!CFutureOrderUtil::IsFutureOrder(&orderObj))
			{
				COrderDetailHelper::ClearField(orderObj, eDetailReqStartDtTm);
			}
		}

		PvOrderFld* pFutureOrder = orderObj.m_orderFldArr.GetFieldFromMeanId(eDetailFutureOrder);

		if (NULL != pFutureOrder)
		{
			const bool bScheduleInstructionsFutureInd = (scheduleInstructions.futureInd == 1);
			const bool bCurrentFutureIndicator = (pFutureOrder->GetLastOeFldValue() == 1);

			if (bCurrentFutureIndicator != bScheduleInstructionsFutureInd)
			{
				pFutureOrder->AddOeFieldValue(bScheduleInstructionsFutureInd, "");
				pFutureOrder->SetValuedSourceInd(eVSUser);

				UpdatePlannedOrderScheduleOnFutureIndicatorChange(component, orderObj);
			}
		}
	}

	return true;
}

void CSchedulingInstructionsChangePlanOrderScheduleUpdater::UpdatePlannedOrderScheduleOnFutureIndicatorChange(
	IComponent& component, PvOrderObj& orderObj)
{
	const short nFutureInd = (short)orderObj.m_orderFldArr.GetFieldValFromMeanId(eDetailFutureOrder);

	if (nFutureInd == 0)
	{
		const bool bOrderHasPharmacyPriorityOfStatOrNow = CPharmacyPriorityHelper::DoesMedOrderHavePharmacyPriorityOfStatOrNow(
					orderObj);

		if (bOrderHasPharmacyPriorityOfStatOrNow)
		{
			Cerner::Foundations::Calendar actionDateTime = CActionDateTimeHelper::GetActionDateTime(orderObj);

			PvOrderFld* pRequestedStartDateTimeFld = orderObj.m_orderFldArr.GetFieldFromMeanId(eDetailReqStartDtTm);

			if (NULL != pRequestedStartDateTimeFld)
			{
				pRequestedStartDateTimeFld->AddOeFieldDtTmValue(actionDateTime);
				pRequestedStartDateTimeFld->SetValuedSourceInd(eVSUser);
			}
		}
	}
}

bool CSchedulingInstructionsChangePlanOrderScheduleUpdater::UpdateInitiatedOrderScheduleOnSchedulingInstructionsChange(
	IComponent& component, PvOrderObj& orderObj)
{
	const CString sSchedulingInstructionsOffset = CSchedulingInstructionsHelper::DetermineSchedulingInstructionsOffset(
				orderObj);

	if (sSchedulingInstructionsOffset.IsEmpty())
	{
		return true;
	}

	const bool bIsNewStyleFutureOrder = CFutureOrderUtil::IsFutureOrder(&orderObj);

	if (bIsNewStyleFutureOrder)
	{
		return true;
	}

	PvOrderProtocolObj* pOrderProtocolObj = dynamic_cast<PvOrderProtocolObj*>(&orderObj);

	const double dFmtActionCd = orderObj.GetFmtActionCd();

	std::list<PvOrderObj*> orders;

	if (NULL != pOrderProtocolObj)
	{
		std::list<PvOrderObj*> dotOrders;
		CGenLoader().GetDayOfTreatmentOrders(m_hPatCon, *pOrderProtocolObj, dotOrders);

		for (auto dotIter = dotOrders.cbegin(); dotIter != dotOrders.cend(); dotIter++)
		{
			PvOrderObj* pDoTOrderObj = *dotIter;
			const double dDoTFmtActionCd = pDoTOrderObj->GetFmtActionCd();

			if (dDoTFmtActionCd == dFmtActionCd)
			{
				IComponent* pDoTComponent = PowerPlanUtil::GetComponent(pDoTOrderObj);

				if (NULL != pDoTComponent)
				{
					const bool bOrderScheduleNeedsUpdated = DoesOrderScheduleNeedUpdated(*pDoTComponent, *pDoTOrderObj);

					if (bOrderScheduleNeedsUpdated)
					{
						UpdateOrderStartDateTime(*pDoTComponent, *pDoTOrderObj, sSchedulingInstructionsOffset);
						orders.push_back(pDoTOrderObj);
					}
				}
			}
		}
	}
	else
	{
		const bool bOrderScheduleNeedsUpdated = DoesOrderScheduleNeedUpdated(component, orderObj);

		if (bOrderScheduleNeedsUpdated)
		{
			UpdateOrderStartDateTime(component, orderObj, sSchedulingInstructionsOffset);
			orders.push_back(&orderObj);
		}
	}

	bool bResult = true;

	CInpatientOrderScheduleServiceCaller inpatientOrderScheduleServiceCaller(m_hPatCon);

	if (CDF::OrderAction::IsOrder(dFmtActionCd))
	{
		bResult = inpatientOrderScheduleServiceCaller.CalculateNewOrderSchedule(orders,
				  CalculateNewOrderScheduleRequest::eReferenceStartDateTimeChanged);
	}

	if (NULL != pOrderProtocolObj)
	{
		CProtocolOrderScheduleManager protocolOrderScheduleManager(m_hPatCon);
		protocolOrderScheduleManager.UpdateProtocolSchedule(*pOrderProtocolObj);
	}

	return bResult;
}

bool CSchedulingInstructionsChangePlanOrderScheduleUpdater::DoesOrderScheduleNeedUpdated(IComponent& component,
		PvOrderObj& orderObj)
{
	const bool bIsTimeZeroActive = component.GetTZActiveInd() == TRUE;

	if (!bIsTimeZeroActive)
	{
		const bool bIsNewStyleFutureOrder = CFutureOrderUtil::IsFutureOrder(&orderObj);

		if (!bIsNewStyleFutureOrder)
		{
			const double dOffset = component.GetStartOffsetQty();

			if (dOffset == 0.0)
			{
				const CString sGenericPriorityOffset = CGenericPriorityOffsetDeterminer::DetermineGenericPriorityOffset(orderObj);

				if (sGenericPriorityOffset.IsEmpty())
				{
					const CString sCollectionPriorityOffset = CCollectionPriorityOffsetDeterminer::DetermineCollectionPriorityOffset(
								orderObj);

					if (sCollectionPriorityOffset.IsEmpty())
					{
						const bool bHasStatOrNowPriority = CPharmacyPriorityHelper::DoesMedOrderHavePharmacyPriorityOfStatOrNow(orderObj);

						if (!bHasStatOrNowPriority)
						{
							return true;
						}
					}
				}
			}
		}
	}

	return false;
}

void CSchedulingInstructionsChangePlanOrderScheduleUpdater::UpdateOrderStartDateTime(IComponent& component,
		PvOrderObj& orderObj, const CString& sSchedulingInstructionsOffset)
{
	component.PutLinkedToPhaseStartDateTime(TRUE);

	IPhase* pIPhase = PowerPlanUtil::GetPhase(&component);

	if (pIPhase != nullptr)
	{
		ICalendarPtr pIPhaseStartDateTime = pIPhase->GetUTCStartDtTm();

		if (pIPhaseStartDateTime != nullptr && !pIPhaseStartDateTime->IsNull())
		{
			const Cerner::Foundations::Calendar phaseStartDateTime = PvOrdCalUtils::GetDateOnlyCalendarFromInterface(
						pIPhaseStartDateTime);
			const Cerner::Foundations::Calendar actionDateTime = CActionDateTimeHelper::GetActionDateTime(orderObj);
			Cerner::Foundations::Calendar referenceStartDateTime = CPvDateTimeOffsetUtil::AddOffsetToDateTime(phaseStartDateTime,
					sSchedulingInstructionsOffset);
			CString phaseStatusMean((LPCTSTR)pIPhase->GetPlanStatusMean());

			// Reset the order's date time to action date time if the action date time is greter than reference start date time.
			// For future plans, do not reset the order's date and time to action date time. PutLinkedToPhaseStartDateTime will be maintained to invoke
			// CombineComponentOffsetAndPriority() method to apply scheduling instruction offset while activating the phase
			if (!referenceStartDateTime.IsNull() && actionDateTime.Compare(referenceStartDateTime) == 1
				&& phaseStatusMean != _T("FUTURE"))
			{
				component.PutLinkedToPhaseStartDateTime(FALSE);
				COrderStartDateTimeUpdater::SetReferenceStartDateTime(orderObj, actionDateTime, true);
				return;
			}

			COrderStartDateTimeUpdater::SetReferenceStartDateTime(orderObj, referenceStartDateTime, false);
		}
	}
}