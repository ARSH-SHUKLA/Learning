#include "StdAfx.h"

#include "InitiateOrderScheduleManager.h"
#include "TimeZeroHelper.h"
#include "OrderDetailHelper.h"
#include "PharmacyPriorityHelper.h"
#include "ComponentOffsetHelper.h"
#include "OrderStartDateTimeUpdater.h"
#include "ComponentOffsetAndPriorityCombiner.h"
#include "GenericPriorityOffsetDeterminer.h"
#include "CollectionPriorityOffsetDeterminer.h"
#include "LinkedPhaseStartDtTmHelper.h"

#include <OrderDetailHelper.h>
#include <pvorderobj.h>
#include <srvcalendar.h>
#include <PriorityDefs.h>
#include <PvOrdCalUtil.h>
#include <pvorderdataexp.h>
#include <FutureOrderUtil.h>
#include <PvDateTimeOffsetUtil.h>
#include <CPS_ImportPVCareCoordCom.h>
#include <ActionDateTimeHelper.h>
#include <PvGenericLoaderExtended.h>

bool CInitiateOrderScheduleManager::UpdateOrderScheduleOnInitiate(IComponent& component, PvOrderObj& orderObj,
		const Cerner::Foundations::Calendar& phaseStartDateTime, const Cerner::Foundations::Calendar& timeZeroDateTime,
		const Cerner::Foundations::Calendar& phaseStopDateTime)
{
	const Cerner::Foundations::Calendar& actionDateTime = CActionDateTimeHelper::GetActionDateTime(orderObj);

	if (CFutureOrderUtil::IsFutureOrder(&orderObj))
	{
		const HPATCON hPatCon = orderObj.GetPatCon();
		CPvGenericLoaderExtended(hPatCon).ProcessFutureOrderDetails(orderObj);

		if (!CFutureOrderUtil::IsFutureOrderScheduleValid(&orderObj) && !orderObj.IsProtocolOrder()
			&& !orderObj.IsDayOfTreatmentOrder())
		{
			return false;
		}
	}

	// For a protocol/dot order, the above if block will be executed and the future recurring order details will be cleared,
	// the order will no more be a future recurring order and hence the start and stop date/time has to be updated.
	if (!CFutureOrderUtil::IsFutureOrder(&orderObj))
	{
		UpdateStartDateTime(component, orderObj, phaseStartDateTime, actionDateTime, timeZeroDateTime);

		const bool bOrderStopUpdatedToPhaseStop = UpdateOrderStopDateTime(component, orderObj, phaseStartDateTime,
				phaseStopDateTime);
		component.PutLinkToPhase(bOrderStopUpdatedToPhaseStop ? TRUE : FALSE);
	}

	return true;
}

void CInitiateOrderScheduleManager::UpdateStartDateTime(IComponent& component, PvOrderObj& orderObj,
		const Cerner::Foundations::Calendar& phaseStartDateTime, const Cerner::Foundations::Calendar& actionDateTime,
		const Cerner::Foundations::Calendar& timeZeroDateTime)
{
	IPlanPtr pIPlan = PowerPlanUtil::GetPlan(&component);
	const bool bIsRequestedStartDateTimeManuallyEntered = CLinkedPhaseStartDtTmHelper::IsRequestedStartDateTimeManuallyEntered(component, orderObj);
	
	if (pIPlan != nullptr)
	{
		// If the Requested start date/time was manually entered/modified then retain it and
		// do not update it to linked phase's start date/time.
		if (!component.HasBeenModified() && !bIsRequestedStartDateTimeManuallyEntered)
		{
			if (CLinkedPhaseStartDtTmHelper::IsComponentLinkedToValidPhase(component, pIPlan))
			{
				CLinkedPhaseStartDtTmHelper::SetLinkedPhaseStartDtTmToAnchorOrder(orderObj, component);
				return;
			}
		}
	}

	const bool bIsTimeZeroActive = component.GetTZActiveInd() == TRUE;

	if (bIsTimeZeroActive)
	{
		Cerner::Foundations::Calendar referenceStartDateTime = CTimeZeroHelper::CalculateStartDateTimeFromTimeZeroInformation(
					component, phaseStartDateTime, timeZeroDateTime);
		COrderStartDateTimeUpdater::SetReferenceStartDateTime(orderObj, referenceStartDateTime, true);

		component.PutLinkedToPhaseStartDateTime(TRUE);

		return;
	}

	const bool bHasStatOrNowPharmacyPriority = CPharmacyPriorityHelper::DoesMedOrderHavePharmacyPriorityOfStatOrNow(
				orderObj);

	if (bHasStatOrNowPharmacyPriority)
	{
		Cerner::Foundations::Calendar requestedStartDateTime = actionDateTime;
		COrderStartDateTimeUpdater::SetRequestedStartDateTime(orderObj, requestedStartDateTime, true);

		component.PutLinkedToPhaseStartDateTime(FALSE);

		return;
	}

	if (bIsRequestedStartDateTimeManuallyEntered)
	{
		Cerner::Foundations::Calendar manuallyEnteredRequestedStartDateTime = orderObj.m_orderFldArr.GetFieldDtTmValFromMeanId(
					eDetailReqStartDtTm);

		// If manually entered start date/time is in the past, use phase start date/time.
		if (actionDateTime.Compare(manuallyEnteredRequestedStartDateTime) == 1)
		{
			Cerner::Foundations::Calendar referenceStartDateTime = phaseStartDateTime;
			COrderStartDateTimeUpdater::SetReferenceStartDateTime(orderObj, referenceStartDateTime, true);

			component.PutLinkedToPhaseStartDateTime(TRUE);
		}
		else
		{
			COrderStartDateTimeUpdater::SetRequestedStartDateTime(orderObj, manuallyEnteredRequestedStartDateTime, true);

			component.PutLinkedToPhaseStartDateTime(FALSE);
		}

		return;
	}

	component.PutLinkedToPhaseStartDateTime(TRUE);

	Cerner::Foundations::Calendar referenceStartDateTime =
		CComponentOffsetAndPriorityCombiner::CombineComponentOffsetAndPriority(component, orderObj, phaseStartDateTime);
	COrderStartDateTimeUpdater::SetReferenceStartDateTime(orderObj, referenceStartDateTime, true);
}

bool CInitiateOrderScheduleManager::UpdateOrderStopDateTime(IComponent& component, PvOrderObj& orderObj,
		const Cerner::Foundations::Calendar& phaseStartDateTime, const Cerner::Foundations::Calendar& phaseStopDateTime)
{
	PvOrderFld* pStopDateTimeFld = orderObj.m_orderFldArr.GetFieldFromMeanId(eDetailOrdStopDtTm);

	if (NULL != pStopDateTimeFld)
	{
		// If Stop date/time is already populated, return.
		const Cerner::Foundations::Calendar currentStopDateTime = pStopDateTimeFld->GetLastOeFldDtTmValue();

		if (!currentStopDateTime.IsNull())
		{
			return false;
		}

		const CString sStopDateTimeOffset = pStopDateTimeFld->GetLastOeFldDisplayValue();

		if (!sStopDateTimeOffset.IsEmpty())
		{
			Cerner::Foundations::Calendar calculatedStopDateTime = CPvDateTimeOffsetUtil::AddOffsetToDateTime(phaseStartDateTime,
					sStopDateTimeOffset);
			pStopDateTimeFld->AddOeFieldDtTmValue(calculatedStopDateTime);

			return false;
		}

		// If Duration exists, return.
		const bool bDurationExists = COrderDetailHelper::DoesFieldHaveValue(orderObj, eDetailOrdDuration);
		const bool bDurationUnitExists = COrderDetailHelper::DoesFieldHaveValue(orderObj, eDetailOrdDurationUnit);

		if (bDurationExists && bDurationUnitExists)
		{
			return false;
		}

		// If Frequency is one-time and PRN is no, return.
		const EFreqTypeFlag frequencyTypeFlag = (EFreqTypeFlag)orderObj.GetFreqTypeFlag();

		if (frequencyTypeFlag == eFTFOneTime)
		{
			const bool bIsPRN = orderObj.m_orderFldArr.GetFieldValFromMeanId(eDetailSchPrn) == 1;

			if (!bIsPRN)
			{
				return false;
			}
		}

		const bool bIsLinkedToPhaseDuration = component.GetLinkToPhase() == TRUE;

		if (bIsLinkedToPhaseDuration || orderObj.IsDayOfTreatmentOrder())
		{
			component.PutLinkToPhase(TRUE);
			Cerner::Foundations::Calendar orderStopDateTime = phaseStopDateTime;
			pStopDateTimeFld->AddOeFieldDtTmValue(orderStopDateTime);

			return true;
		}
	}

	return false;
}

