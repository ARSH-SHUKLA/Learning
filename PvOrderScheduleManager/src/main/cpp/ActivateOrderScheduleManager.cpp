#include "StdAfx.h"

#include "ActivateOrderScheduleManager.h"
#include "TimeZeroHelper.h"
#include "PharmacyPriorityHelper.h"
#include "ComponentOffsetHelper.h"
#include "OrderStartDateTimeUpdater.h"
#include "ComponentOffsetAndPriorityCombiner.h"

#include <FutureOrderUtil.h>
#include <ActionDateTimeHelper.h>
#include <OrderDetailHelper.h>
#include <PvGenericLoaderExtended.h>

CCalculateActivateOrderScheduleCriteria CActivateOrderScheduleManager::UpdateOrderScheduleOnActivate(
	IComponent& component, PvOrderObj& orderObj, const Cerner::Foundations::Calendar& phaseStartDateTime,
	const Cerner::Foundations::Calendar& timeZeroDateTime, const Cerner::Foundations::Calendar& phaseStopDateTime)
{
	const Cerner::Foundations::Calendar& actionDateTime = CActionDateTimeHelper::GetActionDateTime(orderObj);

	return UpdateStartDateTime(component, orderObj, phaseStartDateTime, actionDateTime, timeZeroDateTime);
}

CCalculateActivateOrderScheduleCriteria CActivateOrderScheduleManager::UpdateStartDateTime(IComponent& component,
		PvOrderObj& orderObj, const Cerner::Foundations::Calendar& phaseStartDateTime,
		const Cerner::Foundations::Calendar& actionDateTime, const Cerner::Foundations::Calendar& timeZeroDateTime)
{
	const bool bIsTimeZeroActive = component.GetTZActiveInd() == TRUE;

	if (bIsTimeZeroActive)
	{
		Cerner::Foundations::Calendar referenceStartDateTime = CTimeZeroHelper::CalculateStartDateTimeFromTimeZeroInformation(
					component, phaseStartDateTime, timeZeroDateTime);
		COrderStartDateTimeUpdater::SetReferenceStartDateTime(orderObj, referenceStartDateTime, false);

		return CCalculateActivateOrderScheduleCriteria(&orderObj,
				CalculateActivateOrderScheduleRequest::eReferenceStartDateTimeChanged, 3);
	}

	const bool bHasStatOrNowPharmacyPriority = CPharmacyPriorityHelper::DoesMedOrderHavePharmacyPriorityOfStatOrNow(
				orderObj);

	if (bHasStatOrNowPharmacyPriority)
	{
		COrderStartDateTimeUpdater::SetRequestedStartDateTime(orderObj, actionDateTime, false);

		return CCalculateActivateOrderScheduleCriteria(&orderObj, CalculateActivateOrderScheduleRequest::eInitialActivate, 3);
	}

	const bool bNewStyleFutureOrder = CFutureOrderUtil::WasFutureOrder(&orderObj);

	if (bNewStyleFutureOrder)
	{
		const HPATCON hPatCon = orderObj.GetPatCon();
		Cerner::Foundations::Calendar requestedStartDateTime = CPvGenericLoaderExtended(
					hPatCon).GetFutureOrderActivationStartDateTime(orderObj, actionDateTime);
		COrderStartDateTimeUpdater::SetRequestedStartDateTime(orderObj, requestedStartDateTime, false);

		return CCalculateActivateOrderScheduleCriteria(&orderObj,
				CalculateActivateOrderScheduleRequest::eRequestedStartDateTimeChanged, 3);
	}

	const bool bLinkedToPhaseStartDateTime = component.GetLinkedToPhaseStartDateTime() == TRUE;

	if (bLinkedToPhaseStartDateTime)
	{
		Cerner::Foundations::Calendar referenceStartDateTime =
			CComponentOffsetAndPriorityCombiner::CombineComponentOffsetAndPriority(component, orderObj, phaseStartDateTime);
		COrderStartDateTimeUpdater::SetReferenceStartDateTime(orderObj, referenceStartDateTime, false);

		return CCalculateActivateOrderScheduleCriteria(&orderObj,
				CalculateActivateOrderScheduleRequest::eReferenceStartDateTimeChanged, 3);
	}

	Cerner::Foundations::Calendar currentStartDateTime = orderObj.m_orderFldArr.GetFieldDtTmValFromMeanId(
				eDetailReqStartDtTm);

	if (currentStartDateTime.IsNull() || actionDateTime.Compare(currentStartDateTime) == 1)
	{
		component.PutLinkedToPhaseStartDateTime(TRUE);

		COrderStartDateTimeUpdater::SetReferenceStartDateTime(orderObj, phaseStartDateTime, false);

		return CCalculateActivateOrderScheduleCriteria(&orderObj,
				CalculateActivateOrderScheduleRequest::eReferenceStartDateTimeChanged, 3);
	}

	return CCalculateActivateOrderScheduleCriteria(&orderObj,
			CalculateActivateOrderScheduleRequest::eRequestedStartDateTimeChanged, 3);
}