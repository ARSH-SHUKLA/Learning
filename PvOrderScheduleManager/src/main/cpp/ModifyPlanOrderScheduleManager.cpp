#include "StdAfx.h"

#include "ModifyPlanOrderScheduleManager.h"
#include "ComponentOffsetAndPriorityCombiner.h"
#include "OrderStartDateTimeUpdater.h"
#include "TimeZeroHelper.h"
#include "PhaseTimeZeroDateTimeDeterminer.h"
#include "PlannedComponentDeterminer.h"
#include "InpatientOrderScheduleServiceCaller.h"
#include "ProtocolOrderScheduleManager.h"

#include <PvOrdCalUtil.h>
#include <pvorderobj.h>
#include <PvOrderProtocolObj.h>
#include <CPS_ImportPVCareCoordCom.h>

CModifyPlanOrderScheduleManager::CModifyPlanOrderScheduleManager(HPATCON hPatCon)
	: m_hPatCon(hPatCon)
{

}

/////////////////////////////////////////////////////////////////////////////
/// \fn		void CModifyPlanOrderScheduleManager::AddOrderReferenceStartDateTimeOnInitialModify(PvOrderObj& orderObj)
/// \brief		Adds Reference Start date/time to the orderObj when Reference Start date/time field is not present in the format.
///
/// \return		void
///
/// \param[in]	PvOrderObj& orderObj - The order whose Reference Start date/time is added.
/// \owner		GN046414
/////////////////////////////////////////////////////////////////////////////
void CModifyPlanOrderScheduleManager::AddOrderReferenceStartDateTimeOnInitialModify(PvOrderObj& orderObj)
{
	IComponentPtr pIComponent = TransferDispatch(orderObj.GetPlanComponent());

	if (NULL == pIComponent)
	{
		return;
	}

	const bool bLinkedToPhaseStart = pIComponent->GetLinkedToPhaseStartDateTime() == TRUE;

	if (bLinkedToPhaseStart)
	{
		IPhasePtr pIPhase(PowerPlanUtil::GetPhase(pIComponent));

		if (NULL == pIPhase)
		{
			return;
		}

		ICalendarPtr pIPhaseStartDateTime = pIPhase->GetUTCStartDtTm();

		if (NULL == pIPhaseStartDateTime)
		{
			return;
		}

		const Cerner::Foundations::Calendar phaseStartDateTime = PvOrdCalUtils::GetDateOnlyCalendarFromInterface(
					pIPhaseStartDateTime);

		const bool bIsTimeZeroActive = pIComponent->GetTZActiveInd() == TRUE;

		if (bIsTimeZeroActive)
		{
			CPhaseTimeZeroDateTimeDeterminer phaseTimeZeroDateTimeDeterminer;
			ICalendarPtr pIPhaseTimeZeroDateTime = phaseTimeZeroDateTimeDeterminer.DetermineTimeZeroDateTime(*pIPhase);
			const Cerner::Foundations::Calendar timeZeroDateTime = PvOrdCalUtils::GetDateOnlyCalendarFromInterface(
						pIPhaseTimeZeroDateTime);

			Cerner::Foundations::Calendar referenceStartDateTime = CTimeZeroHelper::CalculateStartDateTimeFromTimeZeroInformation(
						*pIComponent, phaseStartDateTime, timeZeroDateTime);
			COrderStartDateTimeUpdater::SetReferenceStartDateTime(orderObj, referenceStartDateTime, false);
		}
		else
		{
			Cerner::Foundations::Calendar referenceStartDateTime =
				CComponentOffsetAndPriorityCombiner::CombineComponentOffsetAndPriority(*pIComponent, orderObj, phaseStartDateTime);
			COrderStartDateTimeUpdater::SetReferenceStartDateTime(orderObj, referenceStartDateTime, false);
		}
	}
	else
	{
		Cerner::Foundations::Calendar referenceStartDateTime = orderObj.m_orderFldArr.GetFieldDtTmValFromMeanId(
					eDetailReqStartDtTm);
		COrderStartDateTimeUpdater::SetReferenceStartDateTime(orderObj, referenceStartDateTime, false);
	}
}

/////////////////////////////////////////////////////////////////////////////
/// \fn		bool CModifyPlanOrderScheduleManager::UpdateOrderScheduleOnInitialModify(PvOrderObj& orderObj)
/// \brief		Calculates the schedule for the given order when a modify action is taken.
///
/// \return		bool
///
/// \param[in]	PvOrderObj& orderObj - The order whose schedule needs updated.
/// \owner		DB024248
/////////////////////////////////////////////////////////////////////////////
bool CModifyPlanOrderScheduleManager::UpdateOrderScheduleOnInitialModify(PvOrderObj& orderObj)
{
	CInpatientOrderScheduleServiceCaller inpatientOrderScheduleServiceCaller(m_hPatCon);

	bool bResult = true;

	const bool bPlannedOrderComponent = CPlannedComponentDeterminer::IsPlannedOrderComponent(orderObj);

	//Checks whether the component is in PLANNED status. For planned status, the Reference start date and time is not updated.
	if (!bPlannedOrderComponent)
	{
		std::list<PvOrderObj*> orders;
		orders.push_back(&orderObj);

		Cerner::Foundations::Calendar referenceStartDateTime = orderObj.m_orderFldArr.GetFieldDtTmValFromMeanId(
					eDetailReferenceStartDtTm);

		if (referenceStartDateTime.IsNull())
		{
			AddOrderReferenceStartDateTimeOnInitialModify(orderObj);
		}

		bResult = inpatientOrderScheduleServiceCaller.CalculateModifyOrderSchedule(orders,
				  CalculateModifyOrderScheduleRequest::eInitialModify);

		// FUTURE - If modifying incomplete or medstudent after start, start date/time will be udpated. This should probably unlink the order from phase start.
		PvOrderProtocolObj* pOrderProtocolObj = dynamic_cast<PvOrderProtocolObj*>(&orderObj);

		if (NULL != pOrderProtocolObj)
		{
			if (!pOrderProtocolObj->IsFutureRecurringOrder())
			{
				CProtocolOrderScheduleManager protocolOrderScheduleManager(m_hPatCon);
				protocolOrderScheduleManager.UpdateProtocolSchedule(*pOrderProtocolObj);
			}
		}
	}

	return bResult;
}