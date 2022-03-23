#include "StdAfx.h"

#include "GenericPriorityChangePlanOrderScheduleUpdater.h"
#include "InpatientOrderScheduleServiceCaller.h"
#include "ProtocolOrderScheduleManager.h"
#include "ComponentOffsetAndPriorityCombiner.h"
#include "GenericPriorityOffsetDeterminer.h"
#include "OrderStartDateTimeUpdater.h"
#include "ComponentOffsetHelper.h"
#include "PlannedComponentDeterminer.h"

#include <OrderDetailHelper.h>
#include <pvorderobj.h>
#include <PvOrderProtocolObj.h>
#include <FutureOrderUtil.h>
#include <PvOrdCalUtil.h>
#include <PvDateTimeOffsetUtil.h>
#include <CPS_ImportPVCareCoordCom.h>
#include <PvGenericLoaderExtended.h>
#include <dcp_genericloader.h>
#include <ActionDateTimeHelper.h>

CGenericPriorityChangePlanOrderScheduleUpdater::CGenericPriorityChangePlanOrderScheduleUpdater(const HPATCON hPatCon)
	: m_hPatCon(hPatCon)
{

}

/////////////////////////////////////////////////////////////////////////////
/// \fn		bool CGenericPriorityChangePlanOrderScheduleUpdater::UpdatePlanOrderScheduleOnGenericPriorityChange(IComponent& component, PvOrderObj& orderObj)
/// \brief		Handles updating the schedule of the given plan order when its generic order priority is manually changed.
///
/// \return		bool - True if the schedule was updated successfully. Otherwise, false.
///
/// \param[in]	IComponent& component - The component of the order whose generic order priority changed.
/// \param[in]	PvOrderObj& orderObj - The order whose generic order priority changed.
/// \owner		DB024248
/////////////////////////////////////////////////////////////////////////////
bool CGenericPriorityChangePlanOrderScheduleUpdater::UpdatePlanOrderScheduleOnGenericPriorityChange(
	IComponent& component, PvOrderObj& orderObj)
{
	const bool bPlannedOrderComponent = CPlannedComponentDeterminer::IsPlannedOrderComponent(orderObj);

	CString sGenericPriorityOffset = CGenericPriorityOffsetDeterminer::DetermineGenericPriorityOffset(orderObj);

	if (bPlannedOrderComponent)
	{
		return UpdatePlannedOrderScheduleOnGenericPriorityChange(component, orderObj, sGenericPriorityOffset);
	}
	else
	{
		return UpdateInitiatedOrderScheduleOnGenericPriorityChange(component, orderObj, sGenericPriorityOffset);
	}
}

bool CGenericPriorityChangePlanOrderScheduleUpdater::UpdatePlannedOrderScheduleOnGenericPriorityChange(
	IComponent& component, PvOrderObj& orderObj, CString& sGenericPriorityOffset)
{
	const bool bOrderScheduleNeedsUpdated = DoesOrderScheduleNeedUpdated(component, orderObj, sGenericPriorityOffset);

	if (bOrderScheduleNeedsUpdated)
	{
		if (!sGenericPriorityOffset.IsEmpty())
		{
			if (sGenericPriorityOffset == _T(";"))
			{
				component.PutLinkedToPhaseStartDateTime(FALSE);
			}
			else
			{
				component.PutLinkedToPhaseStartDateTime(TRUE);
			}

			COrderDetailHelper::ClearField(orderObj, eDetailReqStartDtTm);

			// If the selected priority has an absolute time (ie. 'T;1234'), component offset cannot be in hours or minutes.
			const bool bHasAbsoluteTime = CPvDateTimeOffsetUtil::DoesOffsetHaveAbsoluteTime(sGenericPriorityOffset);

			if (bHasAbsoluteTime)
			{
				const bool bHasComponentOffsetInHoursOrMinutes = CComponentOffsetHelper::DoesComponentHaveOffsetInHoursOrMinutes(
							component);

				if (bHasComponentOffsetInHoursOrMinutes)
				{
					CComponentOffsetHelper::ClearComponentOffset(component);
				}
			}
		}
	}

	return true;
}

bool CGenericPriorityChangePlanOrderScheduleUpdater::UpdateInitiatedOrderScheduleOnGenericPriorityChange(
	IComponent& component, PvOrderObj& orderObj, CString& sGenericPriorityOffset)
{
	PvOrderProtocolObj* pOrderProtocolObj = dynamic_cast<PvOrderProtocolObj*>(&orderObj);

	if (NULL != pOrderProtocolObj)
	{
		return UpdateProtocolOrderScheduleOnGenericPriorityChange(component, *pOrderProtocolObj, sGenericPriorityOffset);
	}
	else
	{
		const bool bOrderScheduleNeedsUpdated = DoesOrderScheduleNeedUpdated(component, orderObj, sGenericPriorityOffset);

		if (bOrderScheduleNeedsUpdated)
		{
			std::list<PvOrderObj*> orders;
			orders.push_back(&orderObj);

			const double dFmtActionCd = orderObj.GetFmtActionCd();

			if (sGenericPriorityOffset == _T(";"))
			{
				component.PutLinkedToPhaseStartDateTime(FALSE);

				return CallInpatientOrderScheduleServiceForSemicolonPriorityOnGenericPriorityChange(orders, dFmtActionCd);
			}
			else
			{
				UpdateOrderStartDateTime(component, orderObj, sGenericPriorityOffset);

				return CallInpatientOrdersScheduleService(orders, dFmtActionCd);
			}
		}
	}

	return true;
}

bool CGenericPriorityChangePlanOrderScheduleUpdater::UpdateProtocolOrderScheduleOnGenericPriorityChange(
	IComponent& protocolComponent, PvOrderProtocolObj& protocolOrderObj, CString& sGenericPriorityOffset)
{
	const double dProtocolFmtActionCd = protocolOrderObj.GetFmtActionCd();

	std::list<PvOrderObj*> updatedOrders;

	std::list<PvOrderObj*> dotOrders;
	CGenLoader().GetDayOfTreatmentOrders(m_hPatCon, protocolOrderObj, dotOrders);

	for (auto orderIter = dotOrders.cbegin(); orderIter != dotOrders.cend(); orderIter++)
	{
		PvOrderObj* pDoTOrderObj = *orderIter;

		const double dDoTFmtActionCd = pDoTOrderObj->GetFmtActionCd();

		if (dDoTFmtActionCd == dProtocolFmtActionCd)
		{
			IComponent* pDoTComponent = PowerPlanUtil::GetComponent(pDoTOrderObj);

			if (NULL != pDoTComponent)
			{
				const bool bOrderScheduleNeedsUpdated = DoesOrderScheduleNeedUpdated(*pDoTComponent, *pDoTOrderObj,
														sGenericPriorityOffset);

				if (bOrderScheduleNeedsUpdated)
				{
					UpdateOrderStartDateTime(*pDoTComponent, *pDoTOrderObj, sGenericPriorityOffset);
					updatedOrders.push_back(pDoTOrderObj);
				}
			}
		}
	}

	const bool bResult = CallInpatientOrdersScheduleService(updatedOrders, dProtocolFmtActionCd);

	CProtocolOrderScheduleManager protocolOrderScheduleManager(m_hPatCon);
	protocolOrderScheduleManager.UpdateProtocolSchedule(protocolOrderObj);

	return bResult;
}

bool CGenericPriorityChangePlanOrderScheduleUpdater::CallInpatientOrdersScheduleService(std::list<PvOrderObj*>& orders,
		const double dFmtActionCd)
{
	bool bResult = true;

	CInpatientOrderScheduleServiceCaller inpatientOrderScheduleServiceCaller(m_hPatCon);

	if (CDF::OrderAction::IsOrder(dFmtActionCd))
	{
		bResult = inpatientOrderScheduleServiceCaller.CalculateNewOrderSchedule(orders,
				  CalculateNewOrderScheduleRequest::eReferenceStartDateTimeChanged);
	}
	else if (CDF::OrderAction::IsModify(dFmtActionCd) || CDF::OrderAction::IsReschedule(dFmtActionCd))
	{
		bResult = inpatientOrderScheduleServiceCaller.CalculateModifyOrderSchedule(orders,
				  CalculateModifyOrderScheduleRequest::eReferenceStartDateTimeChanged);
	}

	return bResult;
}

bool CGenericPriorityChangePlanOrderScheduleUpdater::DoesOrderScheduleNeedUpdated(IComponent& component,
		PvOrderObj& orderObj, CString& sGenericPriorityOffset)
{
	const bool bIsTimeZeroActive = component.GetTZActiveInd() == TRUE;

	if (!bIsTimeZeroActive)
	{
		const double dCatalogTypeCd = orderObj.GetCatalogTypeCd();

		if (!CDF::CatalogType::IsLaboratory(dCatalogTypeCd) && !CDF::CatalogType::IsPharmacy(dCatalogTypeCd))
		{
			const bool bIsNewStyleFutureOrder = CFutureOrderUtil::IsFutureOrder(&orderObj);

			if (!bIsNewStyleFutureOrder)
			{
				const bool bLinkedToPhaseStart = component.GetLinkedToPhaseStartDateTime() == TRUE;

				if (bLinkedToPhaseStart)
				{
					return true;
				}

				if (!sGenericPriorityOffset.IsEmpty())
				{
					return true;
				}
			}
		}
	}

	return false;
}

void CGenericPriorityChangePlanOrderScheduleUpdater::UpdateOrderStartDateTime(IComponent& component,
		PvOrderObj& orderObj, CString& sGenericPriorityOffset)
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
			Cerner::Foundations::Calendar referenceStartDateTime =
				CComponentOffsetAndPriorityCombiner::CombineComponentOffsetAndPriority(component, orderObj, phaseStartDateTime);
			const Cerner::Foundations::Calendar& actionDateTime = CActionDateTimeHelper::GetActionDateTime(orderObj);
			CString phaseStatusMean((LPCTSTR)pIPhase->GetPlanStatusMean());

			// Reset the order's date time to action date time if the action date time is greter than reference start date time.
			// For future plans, do not reset the order's date and time to action date time. PutLinkedToPhaseStartDateTime will be maintained to invoke
			// CombineComponentOffsetAndPriority() method to apply priority offset while activating the phase
			if (!referenceStartDateTime.IsNull() && actionDateTime.Compare(referenceStartDateTime) == 1
				&& phaseStatusMean != _T("FUTURE") && !sGenericPriorityOffset.IsEmpty()
				&& !sGenericPriorityOffset.CompareNoCase("E") == 0)
			{
				component.PutLinkedToPhaseStartDateTime(FALSE);
				COrderStartDateTimeUpdater::SetReferenceStartDateTime(orderObj, actionDateTime, true);
				return;
			}

			COrderStartDateTimeUpdater::SetReferenceStartDateTime(orderObj, referenceStartDateTime, false);
		}
	}
}

bool CGenericPriorityChangePlanOrderScheduleUpdater::CallInpatientOrderScheduleServiceForSemicolonPriorityOnGenericPriorityChange(
	std::list<PvOrderObj*>& orders, const double dFmtActionCd)
{
	bool bResult = true;

	CInpatientOrderScheduleServiceCaller inpatientOrderScheduleServiceCaller(m_hPatCon);

	if (CDF::OrderAction::IsOrder(dFmtActionCd))
	{
		bResult = inpatientOrderScheduleServiceCaller.CalculateNewOrderSchedule(orders,
				  CalculateNewOrderScheduleRequest::ePriorityChanged);
	}
	else if (CDF::OrderAction::IsModify(dFmtActionCd) || CDF::OrderAction::IsReschedule(dFmtActionCd))
	{
		bResult = inpatientOrderScheduleServiceCaller.CalculateModifyOrderSchedule(orders,
				  CalculateModifyOrderScheduleRequest::ePriorityChanged);
	}

	return bResult;
}