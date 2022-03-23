#include "StdAfx.h"

#include "PlanScheduleManager.h"
#include "PlannedComponentDeterminer.h"
#include "RequestedStartDateTimeChangePlanOrderScheduleUpdater.h"
#include "ReferenceStartDateTimeChangePlanOrderScheduleUpdater.h"
#include "StopDateTimeChangePlanOrderScheduleUpdater.h"
#include "FutureChangePlanOrderScheduleUpdater.h"
#include "DurationChangePlanOrderScheduleUpdater.h"
#include "FrequencyChangePlanOrderScheduleUpdater.h"
#include "PRNChangePlanOrderScheduleUpdater.h"
#include "InfuseOverChangePlanOrderScheduleUpdater.h"
#include "PharmacyPriorityChangePlanOrderScheduleUpdater.h"
#include "CollectionPriorityChangePlanOrderScheduleUpdater.h"
#include "GenericPriorityChangePlanOrderScheduleUpdater.h"
#include "SchedulingInstructionsChangePlanOrderScheduleUpdater.h"
#include "SubPhaseScheduleManager.h"
#include "ChangePhaseStartDateTimeManager.h"
#include "OutcomeScheduleManager.h"
#include "InpatientOrderScheduleServiceCaller.h"
#include "InitiatePhaseScheduleManager.h"
#include "ActivatePhaseScheduleManager.h"
#include "IncludeComponentScheduleManager.h"
#include "ExcludeComponentScheduleManager.h"
#include "ChangePhaseTimeZeroDateTimeManager.h"
#include "ChangePhaseStopDateTimeManager.h"
#include "ComponentOffsetChangeHandler.h"
#include "ConstantIndicatorChangePlanOrderScheduleUpdater.h"
#include "ModifyPlanOrderScheduleManager.h"

#include <pvorderobj.h>
#include <PvOrderProtocolObj.h>
#include <FutureOrderUtil.h>
#include <CPS_ImportPVCareCoordCom.h>
#include <PvGenericLoaderExtended.h>
#include <ActionDateTimeHelper.h>

CPlanScheduleManager::CPlanScheduleManager(const HPATCON hPatCon)
	: m_hPatCon(hPatCon)
{

}

/////////////////////////////////////////////////////////////////////////////
/// \fn		bool CPlanScheduleManager::UpdateOrderScheduleOnActivateOrder(PvOrderObj& orderObj)
/// \brief		Calculates the schedule for the given order when the order is individually activated, as opposed to during a phase activate.
///
/// \return		bool
///
/// \param[in]	PvOrderObj& orderObj - The order whose schedule needs updated.
/// \owner		DB024248
/////////////////////////////////////////////////////////////////////////////
bool CPlanScheduleManager::UpdateOrderScheduleOnActivateOrder(IComponent& component, PvOrderObj& orderObj)
{
	CPvGenericLoaderExtended(m_hPatCon).PrepareForModifyAction(&component);

	component.PutLinkedToPhaseStartDateTime(FALSE);

	std::list<PvOrderObj*> orders;
	orders.push_back(&orderObj);

	const bool bNewStyleFutureOrder = CFutureOrderUtil::WasFutureOrder(&orderObj);

	if (bNewStyleFutureOrder)
	{
		Cerner::Foundations::Calendar requestedStartDateTime = CPvGenericLoaderExtended(
					m_hPatCon).GetFutureOrderActivationStartDateTimeFromOrder(orderObj);

		Cerner::Foundations::Calendar actionDateTime = CActionDateTimeHelper::GetActionDateTime(orderObj);

		if (requestedStartDateTime.IsNull() || requestedStartDateTime.Compare(actionDateTime) < 0)
		{
			requestedStartDateTime = actionDateTime;
		}

		PvOrderFld* pRequestedStartDateTimeFld = orderObj.m_orderFldArr.GetFieldFromMeanId(eDetailReqStartDtTm);

		if (NULL != pRequestedStartDateTimeFld)
		{
			pRequestedStartDateTimeFld->AddOeFieldDtTmValue(requestedStartDateTime);
		}

		CInpatientOrderScheduleServiceCaller inpatientOrderScheduleServiceCaller(m_hPatCon);
		return inpatientOrderScheduleServiceCaller.CalculateActivateOrderSchedule(orders,
				CalculateActivateOrderScheduleRequest::eRequestedStartDateTimeChanged);
	}
	else
	{
		CInpatientOrderScheduleServiceCaller inpatientOrderScheduleServiceCaller(m_hPatCon);
		return inpatientOrderScheduleServiceCaller.CalculateActivateOrderSchedule(orders,
				CalculateActivateOrderScheduleRequest::eInitialActivate);
	}
}

/////////////////////////////////////////////////////////////////////////////
/// \fn		bool CPlanScheduleManager::UpdateOrderScheduleOnInitialModify(PvOrderObj& orderObj)
/// \brief		Calculates the schedule for the given order when a modify action is taken.
///
/// \return		bool
///
/// \param[in]	PvOrderObj& orderObj - The order whose schedule needs updated.
/// \owner		DB024248
/////////////////////////////////////////////////////////////////////////////
bool CPlanScheduleManager::UpdateOrderScheduleOnInitialModify(PvOrderObj& orderObj)
{
	CModifyPlanOrderScheduleManager modifyPlanOrderScheduleManager(m_hPatCon);
	return modifyPlanOrderScheduleManager.UpdateOrderScheduleOnInitialModify(orderObj);
}

/////////////////////////////////////////////////////////////////////////////
/// \fn		bool CPlanScheduleManager::UpdateOrderScheduleOnRequestedStartDateTimeChange(PvOrderObj& orderObj)
/// \brief		Handles updating the schedule of the given plan order when its requested start date/time is manually changed.
///
/// \return		bool
///
/// \param[in]	PvOrderObj& orderObj - The order whose schedule needs updated.
/// \owner		DB024248
/////////////////////////////////////////////////////////////////////////////
bool CPlanScheduleManager::UpdateOrderScheduleOnRequestedStartDateTimeChange(PvOrderObj& orderObj)
{
	IComponentPtr pIComponent = TransferDispatch(orderObj.GetPlanComponent());

	if (NULL == pIComponent)
	{
		return false;
	}

	CRequestedStartDateTimeChangePlanOrderScheduleUpdater requestedStartDateTimeChangeOrderScheduleUpdater(m_hPatCon);
	return requestedStartDateTimeChangeOrderScheduleUpdater.UpdatePlanOrderScheduleOnRequestedStartDateTimeChange(
			   *pIComponent, orderObj);
}

/////////////////////////////////////////////////////////////////////////////
/// \fn		bool CPlanScheduleManager::UpdateOrderScheduleOnReferenceStartDateTimeChange(PvOrderObj& orderObj)
/// \brief		Handles updating the schedule of the given plan order when its reference start date/time is manually changed.
///
/// \return		bool
///
/// \param[in]	PvOrderObj& orderObj - The order whose schedule needs updated.
/// \owner		DB024248
/////////////////////////////////////////////////////////////////////////////
bool CPlanScheduleManager::UpdateOrderScheduleOnReferenceStartDateTimeChange(PvOrderObj& orderObj)
{
	IComponentPtr pIComponent = TransferDispatch(orderObj.GetPlanComponent());

	if (NULL == pIComponent)
	{
		return false;
	}

	const bool bPlannedOrderComponent = CPlannedComponentDeterminer::IsPlannedOrderComponent(orderObj);

	if (bPlannedOrderComponent)
	{
		return false;
	}
	else
	{

		CReferenceStartDateTimeChangePlanOrderScheduleUpdater referenceStartDateTimeChangeOrderScheduleUpdater(m_hPatCon);
		return referenceStartDateTimeChangeOrderScheduleUpdater.UpdatePlanOrderScheduleOnReferenceStartDateTimeChange(
				   *pIComponent, orderObj);
	}

	return true;
}

/////////////////////////////////////////////////////////////////////////////
/// \fn		bool CPlanScheduleManager::UpdateOrderScheduleOnStopDateTimeChange(PvOrderObj& orderObj)
/// \brief		Handles updating the schedule of the given plan order when its stop date/time is manually changed.
///
/// \return		bool
///
/// \param[in]	PvOrderObj& orderObj - The order whose schedule needs updated.
/// \owner		DB024248
/////////////////////////////////////////////////////////////////////////////
bool CPlanScheduleManager::UpdateOrderScheduleOnStopDateTimeChange(PvOrderObj& orderObj)
{
	IComponentPtr pIComponent = TransferDispatch(orderObj.GetPlanComponent());

	if (NULL == pIComponent)
	{
		return false;
	}

	const bool bPlannedOrderComponent = CPlannedComponentDeterminer::IsPlannedOrderComponent(orderObj);

	if (bPlannedOrderComponent)
	{
		return false;
	}
	else
	{
		CStopDateTimeChangePlanOrderScheduleUpdater stopDateTimeChangeOrderScheduleUpdater(m_hPatCon);
		return stopDateTimeChangeOrderScheduleUpdater.UpdatePlanOrderScheduleOnStopDateTimeChange(*pIComponent, orderObj);
	}

	return true;
}

/////////////////////////////////////////////////////////////////////////////
/// \fn		bool CPlanScheduleManager::UpdateOrderScheduleOnFutureIndicatorChange(PvOrderObj& orderObj)
/// \brief		Handles updating the schedule of the given plan order when its future indicator is manually changed.
///
/// \return		bool
///
/// \param[in]	PvOrderObj& orderObj - The order whose schedule needs updated.
/// \owner		DB024248
/////////////////////////////////////////////////////////////////////////////
bool CPlanScheduleManager::UpdateOrderScheduleOnFutureIndicatorChange(PvOrderObj& orderObj)
{
	IComponentPtr pIComponent = TransferDispatch(orderObj.GetPlanComponent());

	if (NULL == pIComponent)
	{
		return false;
	}

	CFutureChangePlanOrderScheduleUpdater futureChangeOrderScheduleUpdater(m_hPatCon);
	return futureChangeOrderScheduleUpdater.UpdatePlanOrderScheduleOnFutureIndicatorChange(*pIComponent, orderObj);
}

/////////////////////////////////////////////////////////////////////////////
/// \fn		bool CPlanScheduleManager::UpdateOrderScheduleOnFrequencyChange(PvOrderObj& orderObj)
/// \brief		Handles updating the schedule of the given plan order when its frequency is manually changed.
///
/// \return		bool
///
/// \param[in]	PvOrderObj& orderObj - The order whose schedule needs updated.
/// \owner		DB024248
/////////////////////////////////////////////////////////////////////////////
bool CPlanScheduleManager::UpdateOrderScheduleOnFrequencyChange(PvOrderObj& orderObj)
{
	IComponentPtr pIComponent = TransferDispatch(orderObj.GetPlanComponent());

	if (NULL == pIComponent)
	{
		return false;
	}

	CFrequencyChangePlanOrderScheduleUpdater frequencyChangePlanOrderScheduleUpdater(m_hPatCon);
	return frequencyChangePlanOrderScheduleUpdater.UpdatePlanOrderScheduleOnFrequencyChange(*pIComponent, orderObj);
}

/////////////////////////////////////////////////////////////////////////////
/// \fn		bool CPlanScheduleManager::UpdateOrderScheduleOnGenericPriorityChange(PvOrderObj& orderObj)
/// \brief		Handles updating the schedule of the given plan order when its generic order priority is manually changed.
///
/// \return		bool
///
/// \param[in]	PvOrderObj& orderObj - The order whose schedule needs updated.
/// \owner		DB024248
/////////////////////////////////////////////////////////////////////////////
bool CPlanScheduleManager::UpdateOrderScheduleOnGenericPriorityChange(PvOrderObj& orderObj)
{
	IComponentPtr pIComponent = TransferDispatch(orderObj.GetPlanComponent());

	if (NULL == pIComponent)
	{
		return false;
	}

	CGenericPriorityChangePlanOrderScheduleUpdater genericPriorityChangeOrderScheduleUpdater(m_hPatCon);
	return genericPriorityChangeOrderScheduleUpdater.UpdatePlanOrderScheduleOnGenericPriorityChange(*pIComponent, orderObj);
}

/////////////////////////////////////////////////////////////////////////////
/// \fn		bool CPlanScheduleManager::UpdateOrderScheduleOnCollectionPriorityChange(PvOrderObj& orderObj)
/// \brief		Handles updating the schedule of the given plan order when its collection priority is manually changed.
///
/// \return		bool
///
/// \param[in]	PvOrderObj& orderObj - The order whose schedule needs updated.
/// \owner		DB024248
/////////////////////////////////////////////////////////////////////////////
bool CPlanScheduleManager::UpdateOrderScheduleOnCollectionPriorityChange(PvOrderObj& orderObj)
{
	IComponentPtr pIComponent = TransferDispatch(orderObj.GetPlanComponent());

	if (NULL == pIComponent)
	{
		return false;
	}

	CCollectionPriorityChangePlanOrderScheduleUpdater collectionPriorityChangeOrderScheduleUpdater(m_hPatCon);
	return collectionPriorityChangeOrderScheduleUpdater.UpdatePlanOrderScheduleOnCollectionPriorityChange(*pIComponent,
			orderObj);
}

/////////////////////////////////////////////////////////////////////////////
/// \fn		bool CPlanScheduleManager::UpdateOrderScheduleOnPharmacyPriorityChange(PvOrderObj& orderObj)
/// \brief		Handles updating the schedule of the given plan order when its pharmacy priority is manually changed.
///
/// \return		bool
///
/// \param[in]	PvOrderObj& orderObj - The order whose schedule needs updated.
/// \owner		DB024248
/////////////////////////////////////////////////////////////////////////////
bool CPlanScheduleManager::UpdateOrderScheduleOnPharmacyPriorityChange(PvOrderObj& orderObj)
{
	IComponentPtr pIComponent = TransferDispatch(orderObj.GetPlanComponent());

	if (NULL == pIComponent)
	{
		return false;
	}

	CPharmacyPriorityChangePlanOrderScheduleUpdater pharmacyPriorityChangeOrderScheduleUpdater(m_hPatCon);
	return pharmacyPriorityChangeOrderScheduleUpdater.UpdatePlanOrderScheduleOnPharmacyPriorityChange(*pIComponent,
			orderObj);
}

/////////////////////////////////////////////////////////////////////////////
/// \fn		bool CPlanScheduleManager::UpdateOrderScheduleOnDurationChange(PvOrderObj& orderObj)
/// \brief		Handles updating the schedule of the given plan order when its duration is manually changed.
///
/// \return		bool
///
/// \param[in]	PvOrderObj& orderObj - The order whose schedule needs updated.
/// \owner		DB024248
/////////////////////////////////////////////////////////////////////////////
bool CPlanScheduleManager::UpdateOrderScheduleOnDurationChange(PvOrderObj& orderObj)
{
	IComponentPtr pIComponent = TransferDispatch(orderObj.GetPlanComponent());

	if (NULL == pIComponent)
	{
		return false;
	}

	CDurationChangePlanOrderScheduleUpdater durationChangePlanOrderScheduleUpdater(m_hPatCon);
	return durationChangePlanOrderScheduleUpdater.UpdatePlanOrderScheduleOnDurationChange(*pIComponent, orderObj);
}

/////////////////////////////////////////////////////////////////////////////
/// \fn		bool CPlanScheduleManager::UpdateOrderScheduleOnPRNChange(PvOrderObj& orderObj)
/// \brief		Handles updating the schedule of the given plan order when its PRN indicator is manually changed.
///
/// \return		bool
///
/// \param[in]	PvOrderObj& orderObj - The order whose schedule needs updated.
/// \owner		DB024248
/////////////////////////////////////////////////////////////////////////////
bool CPlanScheduleManager::UpdateOrderScheduleOnPRNChange(PvOrderObj& orderObj)
{
	IComponentPtr pIComponent = TransferDispatch(orderObj.GetPlanComponent());

	if (NULL == pIComponent)
	{
		return false;
	}

	CPRNChangePlanOrderScheduleUpdater prnChangePlanOrderScheduleUpdater(m_hPatCon);
	return prnChangePlanOrderScheduleUpdater.UpdatePlanOrderScheduleOnPRNChange(*pIComponent, orderObj);
}

/////////////////////////////////////////////////////////////////////////////
/// \fn		bool CPlanScheduleManager::UpdateOrderScheduleOnSchedulingInstructionsChange(PvOrderObj& orderObj)
/// \brief		Handles updating the schedule of the given plan order when its scheduling instructions is manually changed.
///
/// \return		bool
///
/// \param[in]	PvOrderObj& orderObj - The order whose schedule needs updated.
/// \owner		DB024248
/////////////////////////////////////////////////////////////////////////////
bool CPlanScheduleManager::UpdateOrderScheduleOnSchedulingInstructionsChange(PvOrderObj& orderObj)
{
	IComponentPtr pIComponent = TransferDispatch(orderObj.GetPlanComponent());

	if (NULL == pIComponent)
	{
		return false;
	}

	CSchedulingInstructionsChangePlanOrderScheduleUpdater schedulingInstructionsChangePlanOrderScheduleUpdater(m_hPatCon);
	return schedulingInstructionsChangePlanOrderScheduleUpdater.UpdatePlanOrderScheduleOnSchedulingInstructionsChange(
			   *pIComponent, orderObj);
}

/////////////////////////////////////////////////////////////////////////////
/// \fn		bool CPlanScheduleManager::UpdateOrderScheduleOnInfuseOverChange(PvOrderObj& orderObj)
/// \brief		Handles updating the schedule of the given plan order when its infuse over is changed.
///
/// \return		bool
///
/// \param[in]	PvOrderObj& orderObj - The order whose schedule needs updated.
/// \owner		DB024248
/////////////////////////////////////////////////////////////////////////////
bool CPlanScheduleManager::UpdateOrderScheduleOnInfuseOverChange(PvOrderObj& orderObj)
{
	IComponentPtr pIComponent = TransferDispatch(orderObj.GetPlanComponent());

	if (NULL == pIComponent)
	{
		return false;
	}

	CInfuseOverChangePlanOrderScheduleUpdater infuseOverChangePlanOrderScheduleUpdater(m_hPatCon);
	return infuseOverChangePlanOrderScheduleUpdater.UpdatePlanOrderScheduleOnInfuseOverChange(*pIComponent, orderObj);
}

/////////////////////////////////////////////////////////////////////////////
/// \fn		bool CPlanScheduleManager::UpdateOrderScheduleOnConstantIndicatorChange(PvOrderObj& orderObj)
/// \brief		Handles updating the schedule of the given plan order when its constant indicator is changed.
///
/// \return		bool
///
/// \param[in]	PvOrderObj& orderObj - The order whose schedule needs updated.
/// \owner		DB024248
/////////////////////////////////////////////////////////////////////////////
bool CPlanScheduleManager::UpdateOrderScheduleOnConstantIndicatorChange(PvOrderObj& orderObj)
{
	IComponentPtr pIComponent = TransferDispatch(orderObj.GetPlanComponent());

	if (NULL == pIComponent)
	{
		return false;
	}

	CConstantIndicatorChangePlanOrderScheduleUpdater constantIndicatorChangePlanOrderScheduleUpdater(m_hPatCon);
	return constantIndicatorChangePlanOrderScheduleUpdater.UpdatePlanOrderScheduleOnConstantIndicatorChange(*pIComponent,
			orderObj);
}

/////////////////////////////////////////////////////////////////////////////
/// \fn		bool CPlanScheduleManager::UpdateScheduleOnComponentOffsetChange(IComponent& component)
/// \brief		Handles updating the schedule of the given plan order when its component offset is manually changed.
///
/// \return		bool
///
/// \param[out]	IComponent& component - The component whose offset changed.
/// \owner		DB024248
/////////////////////////////////////////////////////////////////////////////
bool CPlanScheduleManager::UpdateScheduleOnComponentOffsetChange(IComponent& component)
{
	bool bResult(false);

	CComponentOffsetChangeHandler componentOffsetChangeHandler(m_hPatCon);
	bResult = componentOffsetChangeHandler.UpdateScheduleOnComponentOffsetChange(component);

	return bResult;
}

/////////////////////////////////////////////////////////////////////////////
/// \fn		void CPlanScheduleManager::UpdateOutcomeSchedule(IPhase& phase, IComponent& component, IOutcome& outcome)
/// \brief		Updates the schedule of the given outcome.
///
/// \return		void
///
/// \param[in]	IPhase& phase - The phase containing the outcome whose schedule needs updated.
/// \param[in]	IComponent& component - The component of the outcome whose schedule needs updated.
/// \param[in]	IOutcome& outcome - The outcome whose schedule needs updated.
/// \owner		DB024248
/////////////////////////////////////////////////////////////////////////////
void CPlanScheduleManager::UpdateOutcomeSchedule(IPhase& phase, IComponent& component, IOutcome& outcome)
{
	COutcomeScheduleManager outcomeScheduleManager(m_hPatCon);
	outcomeScheduleManager.UpdateOutcomeSchedule(phase, component, outcome);
}

/////////////////////////////////////////////////////////////////////////////
/// \fn		void CPlanScheduleManager::UpdatePhaseScheduleOnInitiate(IPhase& phase)
/// \brief		Updates the start, time-zero, and stop date/times of the given phase, and all of its component's schedules during a phase initiate.
///				If the given phase is a protocol phase, the schedules of its treatment periods will also be updated.
///
/// \return		void
///
/// \param[in]	IPhase& phase - The phase that is being initiated.
/// \owner		DB024248
/////////////////////////////////////////////////////////////////////////////
void CPlanScheduleManager::UpdatePhaseScheduleOnInitiate(IPhase& phase)
{
	CInitiatePhaseScheduleManager initiatePhaseScheduleManager(m_hPatCon);
	initiatePhaseScheduleManager.UpdatePhaseScheduleOnInitiate(phase);
}

/////////////////////////////////////////////////////////////////////////////
/// \fn		void CPlanScheduleManager::UpdatePhaseScheduleOnActivate(IPhase& phase)
/// \brief		Updates the start, time-zero, and stop date/times of the given phase, and all of its component's schedules during a phase activate.
///
/// \return		void
///
/// \param[in]	IPhase& phase - The phase that is being activated
/// \owner		DB024248
/////////////////////////////////////////////////////////////////////////////
void CPlanScheduleManager::UpdatePhaseScheduleOnActivate(IPhase& phase)
{
	CActivatePhaseScheduleManager activatePhaseScheduleManager(m_hPatCon);
	activatePhaseScheduleManager.UpdatePhaseScheduleOnActivate(phase);
}

/////////////////////////////////////////////////////////////////////////////
/// \fn		void CPlanScheduleManager::UpdatePhaseScheduleOnChangePhaseStart(IPhase& phase, const bool bAdjustSchedulableOrder)
/// \brief		Updates the time-zero and stop date/times of the given phase, and all of its component's schedules when phase start is changed.
///
/// \return		void
///
/// \param[in]	IPhase& phase - The phase whose start was changed.
/// \param[in]	const bool bAdjustSchedulableOrder - True if the schedule of the linked schedulable order should be updated, if it exists. Otherwise, false.
/// \owner		DB024248
/////////////////////////////////////////////////////////////////////////////
void CPlanScheduleManager::UpdatePhaseScheduleOnChangePhaseStart(IPhase& phase, const bool bAdjustSchedulableOrder)
{
	CChangePhaseStartDateTimeManager changePhaseStartDateTimeManager(m_hPatCon);
	changePhaseStartDateTimeManager.UpdatePhaseScheduleOnChangePhaseStart(phase, bAdjustSchedulableOrder);
}

/////////////////////////////////////////////////////////////////////////////
/// \fn		void CPlanScheduleManager::UpdatePhaseScheduleOnChangePhaseTimeZero(IPhase& phase)
/// \brief		Updates the start and stop date/times of the given phase, and all of its component's schedules when the time-zero date/time of the given phase is changed.
///
/// \return		void
///
/// \param[in]	IPhase& phase - The phase whose time-zero date/time was changed.
/// \owner		DB024248
/////////////////////////////////////////////////////////////////////////////
void CPlanScheduleManager::UpdatePhaseScheduleOnChangePhaseTimeZero(IPhase& phase)
{
	CChangePhaseTimeZeroDateTimeManager changePhaseTimeZeroDateTimeManager(m_hPatCon);
	changePhaseTimeZeroDateTimeManager.UpdatePhaseScheduleOnChangePhaseTimeZero(phase);
}

/////////////////////////////////////////////////////////////////////////////
/// \fn		void CPlanScheduleManager::UpdatePhaseScheduleOnChangePhaseStop(IPhase& phase, const std::set<double>& setComponentIds)
/// \brief		Updates the schedules of the given phase's components when its stop date/time is changed.
///
/// \return		void
///
/// \param[in]	IPhase& phase - The phase whose stop date/time was changed.
/// \param[in]	const std::set<double>& setComponentIds - The set of components to update
/// \owner		DB024248
/////////////////////////////////////////////////////////////////////////////
void CPlanScheduleManager::UpdatePhaseScheduleOnChangePhaseStop(IPhase& phase, const std::set<double>& setComponentIds)
{
	CChangePhaseStopDateTimeManager changePhaseStopDateTimeManager(m_hPatCon);
	changePhaseStopDateTimeManager.UpdatePhaseScheduleOnChangePhaseStop(phase, setComponentIds);
}

/////////////////////////////////////////////////////////////////////////////
/// \fn		void CPlanScheduleManager::UpdatePhaseScheduleOnIncludeComponent(IPhase& phase, IComponent& component)
/// \brief		Updates the start, time-zero, and stop date/times of the given phase, and all of its component's schedules when the given component is included.
///				This should also be called during ad hoc component.
///
/// \return		void
///
/// \param[in]	IPhase& phase - The phase containing the component that was included or ad hoced in.
/// \param[in]	IComponent& component - The component that was included or ad hoced in.
/// \owner		DB024248
/////////////////////////////////////////////////////////////////////////////
void CPlanScheduleManager::UpdatePhaseScheduleOnIncludeComponent(IPhase& phase, IComponent& component)
{
	CIncludeComponentScheduleManager includeComponentScheduleManager(m_hPatCon);
	includeComponentScheduleManager.UpdatePhaseScheduleOnIncludeComponent(phase, component);
}

/////////////////////////////////////////////////////////////////////////////
/// \fn		void CPlanScheduleManager::UpdatePhaseScheduleOnExcludeComponent(IPhase& phase, IComponent& component)
/// \brief		Updates the start, time-zero, and stop date/times of the given phase, and all of its component's schedules when the given component is excluded.
///
/// \return		void
///
/// \param[in]	IPhase& phase - The phase that the component was excluded from.
/// \param[in]	IComponent& component - The component that was excluded.
/// \owner		DB024248
/////////////////////////////////////////////////////////////////////////////
void CPlanScheduleManager::UpdatePhaseScheduleOnExcludeComponent(IPhase& phase, IComponent& component)
{
	CExcludeComponentScheduleManager excludeComponentScheduleManager(m_hPatCon);
	excludeComponentScheduleManager.UpdatePhaseScheduleOnExcludeComponent(phase, component);
}