#include "StdAfx.h"

#include "ComponentOffsetChangeHandler.h"
#include "ComponentOffsetChangePlanOrderScheduleUpdater.h"
#include "OutcomeScheduleManager.h"
#include "SubPhaseScheduleManager.h"
#include "ChangePhaseStartDateTimeManager.h"

#include <PvOrdCalUtil.h>
#include <CPS_ImportPVCareCoordCom.h>
#include <PvGenericLoaderExtended.h>
#include <dcp_genericloader.h>

CComponentOffsetChangeHandler::CComponentOffsetChangeHandler(const HPATCON hPatCon)
	: m_hPatCon(hPatCon)
{

}

/////////////////////////////////////////////////////////////////////////////
/// \fn		bool CComponentOffsetChangeHandler::UpdateScheduleOnComponentOffsetChange(IComponent& component)
/// \brief		Handles updating the schedule of the given plan component when its start offset is manually changed.
///
/// \return		bool - True if the schedule was updated successfully. Otherwise, false.
///
/// \param[in]	IComponent& component - The component whose start offset was changed.
/// \owner		DB024248
/////////////////////////////////////////////////////////////////////////////
bool CComponentOffsetChangeHandler::UpdateScheduleOnComponentOffsetChange(IComponent& component)
{
	CPvGenericLoaderExtended(m_hPatCon).PrepareForModifyAction(&component);

	CString sCompTypeMean((LPCTSTR)component.GetComponentTypeMean());

	if (sCompTypeMean == _T("ORDER CREATE") || sCompTypeMean == _T("PRESCRIPTION"))
	{
		return UpdateScheduleOnComponentOffsetChangeForOrderComponent(component);
	}
	else if (sCompTypeMean == _T("RESULT OUTCO"))
	{
		return UpdateScheduleOnComponentOffsetChangeForOutcomeComponent(component);
	}
	else if (sCompTypeMean == _T("SUBPHASE"))
	{
		return UpdateScheduleOnComponentOffsetChangeForSubPhaseComponent(component);
	}

	return false;
}

bool CComponentOffsetChangeHandler::UpdateScheduleOnComponentOffsetChangeForOrderComponent(IComponent& component)
{
	PvOrderObj* pOrderObj = CGenLoader().GetOrderObjFromComponent(m_hPatCon, component);

	if (NULL != pOrderObj)
	{
		CComponentOffsetChangePlanOrderScheduleUpdater componentOffsetChangeOrderScheduleUpdater(m_hPatCon);
		return componentOffsetChangeOrderScheduleUpdater.UpdatePlanOrderScheduleOnComponentOffsetChange(component, *pOrderObj);
	}

	return false;
}

bool CComponentOffsetChangeHandler::UpdateScheduleOnComponentOffsetChangeForOutcomeComponent(IComponent& component)
{
	IPhase* pIPhase = PowerPlanUtil::GetPhase(&component);
	IOutcomePtr pIOutcome = component.GetDispatch();

	if (NULL != pIPhase && NULL != pIOutcome)
	{
		COutcomeScheduleManager outcomeScheduleManager(m_hPatCon);
		outcomeScheduleManager.UpdateOutcomeSchedule(*pIPhase, component, *pIOutcome);

		return true;
	}

	return false;
}

bool CComponentOffsetChangeHandler::UpdateScheduleOnComponentOffsetChangeForSubPhaseComponent(IComponent& component)
{
	IPhasePtr pISubPhase = component.GetSubphaseDispatch();

	if (pISubPhase != NULL)
	{
		IPhase* pIParentPhase = pISubPhase->GetParentPhase();

		if (NULL != pIParentPhase)
		{
			ICalendarPtr pIPhaseStartDateTime = pIParentPhase->GetUTCStartDtTm();
			const Cerner::Foundations::Calendar phaseStartDateTime = PvOrdCalUtils::GetDateOnlyCalendarFromInterface(
						pIPhaseStartDateTime);

			ICalendarPtr pIPhaseTimeZeroDateTime = pIParentPhase->GetUTCCalcTimeZeroDtTm();
			const Cerner::Foundations::Calendar phaseTimeZeroDateTime = PvOrdCalUtils::GetDateOnlyCalendarFromInterface(
						pIPhaseTimeZeroDateTime);

			ICalendarPtr pIPhaseStopDateTime = pIParentPhase->GetUTCCalcEndDtTm();
			const Cerner::Foundations::Calendar phaseStopDateTime = PvOrdCalUtils::GetDateOnlyCalendarFromInterface(
						pIPhaseStopDateTime);

			CSubPhaseScheduleManager subPhaseScheduleManager;
			subPhaseScheduleManager.UpdateSubPhaseSchedule(*pISubPhase, phaseStartDateTime, phaseTimeZeroDateTime,
					phaseStopDateTime);

			CChangePhaseStartDateTimeManager changePhaseStartDateTimeManager(m_hPatCon);
			changePhaseStartDateTimeManager.UpdatePhaseScheduleOnChangePhaseStart(*pISubPhase, false);

			return true;
		}
	}

	return false;
}