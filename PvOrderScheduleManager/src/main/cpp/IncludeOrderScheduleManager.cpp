#include "StdAfx.h"

#include "IncludeOrderScheduleManager.h"
#include "TimeZeroHelper.h"
#include "PharmacyPriorityHelper.h"
#include "ComponentOffsetHelper.h"
#include "ComponentOffsetAndPriorityCombiner.h"
#include "OrderStartDateTimeUpdater.h"
#include "GenericPriorityOffsetDeterminer.h"
#include "CollectionPriorityOffsetDeterminer.h"

#include <OrderDetailHelper.h>
#include <pvorderobj.h>
#include <srvcalendar.h>
#include <PriorityDefs.h>
#include <pvorderdataexp.h>
#include <FutureOrderUtil.h>
#include <PvDateTimeOffsetUtil.h>
#include <CPS_ImportPVCareCoordCom.h>
#include <ActionDateTimeHelper.h>
#include <PvGenericLoaderExtended.h>
#include "LinkedPhaseStartDtTmHelper.h"

bool CIncludeOrderScheduleManager::UpdateOrderScheduleOnInclude(IComponent& component, PvOrderObj& orderObj,
		const Cerner::Foundations::Calendar& phaseStartDateTime, const Cerner::Foundations::Calendar& timeZeroDateTime,
		const Cerner::Foundations::Calendar& phaseStopDateTime) const
{
	const Cerner::Foundations::Calendar& actionDateTime = CActionDateTimeHelper::GetActionDateTime(orderObj);

	if (CFutureOrderUtil::IsFutureOrder(&orderObj))
	{
		const HPATCON hPatCon = orderObj.GetPatCon();
		CPvGenericLoaderExtended(hPatCon).ProcessFutureOrderDetails(orderObj);

		if (!CFutureOrderUtil::IsFutureOrderScheduleValid(&orderObj))
		{
			return false;
		}
	}
	else
	{
		UpdateStartDateTime(component, orderObj, phaseStartDateTime, actionDateTime, timeZeroDateTime);

		const bool bOrderStopUpdatedToPhaseStop = UpdateOrderStopDateTime(component, orderObj, phaseStartDateTime,
				phaseStopDateTime);
		component.PutLinkToPhase(bOrderStopUpdatedToPhaseStop ? TRUE : FALSE);
	}

	return true;
}

void CIncludeOrderScheduleManager::UpdateStartDateTime(IComponent& component, PvOrderObj& orderObj,
		const Cerner::Foundations::Calendar& phaseStartDateTime, const Cerner::Foundations::Calendar& actionDateTime,
		const Cerner::Foundations::Calendar& timeZeroDateTime) const
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

		if (referenceStartDateTime >= actionDateTime)
		{
			component.PutLinkedToPhaseStartDateTime(TRUE);
			COrderStartDateTimeUpdater::SetReferenceStartDateTime(orderObj, referenceStartDateTime, true);
		}
		else
		{
			component.PutTZActiveInd(FALSE);
			component.PutModifiedTZRelationInd(TRUE);
			component.PutLinkedToPhaseStartDateTime(FALSE);
			COrderStartDateTimeUpdater::SetReferenceStartDateTime(orderObj, actionDateTime, true);
		}

		return;
	}

	const bool bHasStatOrNowPharmacyPriority = CPharmacyPriorityHelper::DoesMedOrderHavePharmacyPriorityOfStatOrNow(
				orderObj);

	if (bHasStatOrNowPharmacyPriority)
	{
		component.PutLinkedToPhaseStartDateTime(FALSE);
		Cerner::Foundations::Calendar requestedStartDateTime = actionDateTime;
		COrderStartDateTimeUpdater::SetRequestedStartDateTime(orderObj, requestedStartDateTime, true);

		return;
	}
	
	if (bIsRequestedStartDateTimeManuallyEntered)
	{
		Cerner::Foundations::Calendar manuallyEnteredRequestedStartDateTime = orderObj.m_orderFldArr.GetFieldDtTmValFromMeanId(
			eDetailReqStartDtTm);

		// If manually entered start date/time is in the past, use phase start date/time.
		if (actionDateTime.Compare(manuallyEnteredRequestedStartDateTime) == 1)
		{
			COrderStartDateTimeUpdater::SetReferenceStartDateTime(orderObj, phaseStartDateTime, true);

			component.PutLinkedToPhaseStartDateTime(TRUE);
		}
		else
		{
			COrderStartDateTimeUpdater::SetRequestedStartDateTime(orderObj, manuallyEnteredRequestedStartDateTime, true);

			component.PutLinkedToPhaseStartDateTime(FALSE);
		}

		return;
	}

	Cerner::Foundations::Calendar referenceStartDateTimeWithOffsets =
		CComponentOffsetAndPriorityCombiner::CombineComponentOffsetAndPriority(component, orderObj, phaseStartDateTime);

	// the start date time padding should be added to actionDateTime for an initiated or pending initiate phase
	// workflow impacted: include an inpatient med after phase is initiated or pending initiate
	//                    adhoc an inpatient medication to an initiated or pending initiate phase
	Cerner::Foundations::Calendar paddedActionDateTime =
		CComponentOffsetAndPriorityCombiner::ApplyStartDateTimePadding(component, orderObj, actionDateTime);

	if (paddedActionDateTime.Compare(referenceStartDateTimeWithOffsets) == 1 &&
		(!referenceStartDateTimeWithOffsets.IsNull() || !orderObj.IsNonMedOrder()))
	{
		component.PutStartOffsetQty(0.0);
		component.PutStartOffsetUnitCd(0.0);

		component.PutLinkedToPhaseStartDateTime(FALSE);
		Cerner::Foundations::Calendar referenceStartDateTime = paddedActionDateTime;
		COrderStartDateTimeUpdater::SetReferenceStartDateTime(orderObj, referenceStartDateTime, true);

		return;
	}

	COrderStartDateTimeUpdater::SetReferenceStartDateTime(orderObj, referenceStartDateTimeWithOffsets, true);
}

bool CIncludeOrderScheduleManager::UpdateOrderStopDateTime(IComponent& component, PvOrderObj& orderObj,
		const Cerner::Foundations::Calendar& phaseStartDateTime, const Cerner::Foundations::Calendar& phaseStopDateTime) const
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

		// If phase stop date/time is null, there is no reason to update
		if (phaseStopDateTime.IsNull())
		{
			return false;
		}

		// If Duration exists, return.
		const bool bDurationExists = COrderDetailHelper::DoesFieldHaveValue(orderObj, eDetailOrdDuration);
		const bool bDurationUnitExists = COrderDetailHelper::DoesFieldHaveValue(orderObj, eDetailOrdDurationUnit);

		if (bDurationExists && bDurationUnitExists)
		{
			return false;
		}

		const bool bIsDayOfTreatmentOrder = orderObj.IsDayOfTreatmentOrder();

		// If Frequency is one-time and PRN is no, return.
		const EFreqTypeFlag frequencyTypeFlag = (EFreqTypeFlag)orderObj.GetFreqTypeFlag();

		if (frequencyTypeFlag == eFTFOneTime)
		{
			const bool bIsPRN = orderObj.m_orderFldArr.GetFieldValFromMeanId(eDetailSchPrn) == 1;

			if (!bIsPRN || !bIsDayOfTreatmentOrder)
			{
				return false;
			}
		}
		
		// If Linked To Phase Duration is false, return.
		const bool bIsLinkedToPhaseDuration = component.GetLinkToPhase() == TRUE;

		if ((pStopDateTimeFld->IsFieldModifiable() && bIsLinkedToPhaseDuration) || bIsDayOfTreatmentOrder)
		{
			Cerner::Foundations::Calendar orderStopDateTime = phaseStopDateTime;
			pStopDateTimeFld->AddOeFieldDtTmValue(orderStopDateTime);

			return true;
		}
	}

	return false;
}