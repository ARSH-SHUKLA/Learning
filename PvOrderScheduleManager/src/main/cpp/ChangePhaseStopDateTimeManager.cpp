#include "StdAfx.h"

#include "ChangePhaseStopDateTimeManager.h"
#include "ProtocolOrderScheduleManager.h"
#include "PhaseTimeZeroDateTimeDeterminer.h"
#include "InpatientOrderScheduleServiceCaller.h"
#include "ChangePhaseStopOrderScheduleManager.h"
#include "ModifiableOrderRetriever.h"
#include "IncludedComponentRetriever.h"
#include "OutcomeScheduleManager.h"

#include <pvorderobj.h>
#include <PvOrdCalUtil.h>
#include <srvcalendar.h>
#include <CPS_ImportPVCareCoordCom.h>
#include <CPS_ImportCpsDataCommon.h>
#include <PvGenericLoaderExtended.h>

CChangePhaseStopDateTimeManager::CChangePhaseStopDateTimeManager(const HPATCON hPatCon)
	: m_hPatCon(hPatCon)
{

}

void CChangePhaseStopDateTimeManager::UpdatePhaseScheduleOnChangePhaseStop(IPhase& phase, const std::set<double>& setComponentIds)
{
	ICalendarPtr pIPhaseStopDateTime = phase.GetUTCCalcEndDtTm();
	const Cerner::Foundations::Calendar phaseStopDateTime = PvOrdCalUtils::GetDateOnlyCalendarFromInterface(
				pIPhaseStopDateTime);

	std::list<PvOrderObj*> inpatientOrders;

	std::list<IComponent*> components;
	CIncludedComponentRetriever::GetIncludedComponents(phase, components);

	for (auto componentIter = components.cbegin(); componentIter != components.cend(); componentIter++)
	{
		IComponent* pIComponent = *componentIter;

		if (pIComponent != nullptr)
		{
			UpdateComponentScheduleOnChangePhaseStop(phase, *pIComponent, inpatientOrders, phaseStopDateTime, setComponentIds);
		}
	}

	CallInpatientOrderScheduleService(inpatientOrders);

	CProtocolOrderScheduleManager protocolOrderScheduleManager(m_hPatCon);
	protocolOrderScheduleManager.UpdateProtocolSchedule(phase);
}

void CChangePhaseStopDateTimeManager::UpdateComponentScheduleOnChangePhaseStop(IPhase& phase, IComponent& component,
		std::list<PvOrderObj*>& inpatientOrders, const Cerner::Foundations::Calendar& phaseStopDateTime,
		const std::set<double>& setComponentIds)
{
	const CString sComponentTypeMeaning = (LPCTSTR)component.GetComponentTypeMean();

	if ((sComponentTypeMeaning == "ORDER CREATE") || (sComponentTypeMeaning == "PRESCRIPTION"))
	{
		if (setComponentIds.find(component.GetActCompId()) != setComponentIds.end())
		{
			PvOrderObj* pOrderObj = GetUpdatedOrderOnChangePhaseStop(component, phaseStopDateTime);

			if (NULL != pOrderObj)
			{
				inpatientOrders.push_back(pOrderObj);
			}
		}
	}
	else if (sComponentTypeMeaning == "RESULT OUTCO")
	{
		if (setComponentIds.find(component.GetActCompId()) != setComponentIds.end())
		{
			UpdateOutcomeComponentScheduleOnChangePhaseStop(phase, component);
		}
	}
	else if (sComponentTypeMeaning == "SUBPHASE")
	{
		IPhasePtr pISubPhase = component.GetSubphaseDispatch();
		std::list<IComponent*> components;
		CIncludedComponentRetriever::GetIncludedComponents(pISubPhase, components);

		for (auto componentIter = components.cbegin(); componentIter != components.cend(); componentIter++)
		{
			IComponent* pIComponent = *componentIter;

			if (pIComponent != nullptr)
			{
				if (setComponentIds.find(pIComponent->GetActCompId()) != setComponentIds.end())
				{
					const CString sComponentInSubPhaseTypeMeaning = (LPCTSTR)component.GetComponentTypeMean();

					// Change stop date time is very weird, so we need to do some weird stuff in order for
					//  components inside a subphase to work.

					if ((sComponentTypeMeaning == "ORDER CREATE") || (sComponentTypeMeaning == "PRESCRIPTION"))
					{
						// Break the link, so the case of components inside a sub phase with link to phase
						//  would not just restore the sub phase stop date time to the order component.
						pIComponent->PutLinkToPhase(FALSE);
					}

					// Using phase instead of sub phase here, because the stop date time is only on the parent
					UpdateComponentScheduleOnChangePhaseStop(phase, *pIComponent, inpatientOrders, phaseStopDateTime, setComponentIds);
				}
			}
		}
	}
}

PvOrderObj* CChangePhaseStopDateTimeManager::GetUpdatedOrderOnChangePhaseStop(IComponent& orderComponent,
		const Cerner::Foundations::Calendar& phaseStopDateTime)
{
	const CModifiableOrderRetriever modifiableOrderRetriever(m_hPatCon);
	PvOrderObj* pOrderObj = modifiableOrderRetriever.GetModifiableOrderFromComponentForStopDateTimeChange(orderComponent);

	if (NULL != pOrderObj)
	{
		CChangePhaseStopOrderScheduleManager orderScheduleManager;
		orderScheduleManager.UpdateOrderScheduleOnChangePhaseStop(orderComponent, *pOrderObj, phaseStopDateTime);

		return pOrderObj;
	}

	return NULL;
}

/////////////////////////////////////////////////////////////////////////////
/// \fn		void CChangePhaseStopDateTimeManager::UpdateOutcomeComponentScheduleOnChangePhaseStop(IPhase& phase, IComponent& outcomeComponent)
/// \brief		Updates the outcome component's schedule.
///
/// \return		void
///
/// \param[in]	IPhase& phase - The phase containing the outcome component whose schedule needs updated.
/// \param[in]	IComponent& outcomeComponent - The outcome component whose schedule needs updated.
/// \owner		DB024248
/////////////////////////////////////////////////////////////////////////////
void CChangePhaseStopDateTimeManager::UpdateOutcomeComponentScheduleOnChangePhaseStop(IPhase& phase,
		IComponent& outcomeComponent)
{
	IOutcomePtr pOutcome = outcomeComponent.GetDispatch();

	if (NULL != pOutcome)
	{
		CPvGenericLoaderExtended(m_hPatCon).PrepareOutcomeForUpdate(pOutcome);
		CPvGenericLoaderExtended(m_hPatCon).PrepareForModifyAction(&outcomeComponent);

		COutcomeScheduleManager outcomeScheduleManager(m_hPatCon);
		outcomeScheduleManager.UpdateOutcomeSchedule(phase, outcomeComponent, *pOutcome);
	}
}

void CChangePhaseStopDateTimeManager::CallInpatientOrderScheduleService(std::list<PvOrderObj*>& orders)
{
	std::list<PvOrderObj*> newOrders;
	std::list<PvOrderObj*> modifyOrders;

	for (auto orderIter = orders.cbegin(); orderIter != orders.cend(); orderIter++)
	{
		PvOrderObj* pOrderObj = *orderIter;

		const double dFmtActionCd = pOrderObj->GetFmtActionCd();

		if (CDF::OrderAction::IsOrder(dFmtActionCd))
		{
			newOrders.push_back(pOrderObj);
		}
		else if (CDF::OrderAction::IsModify(dFmtActionCd) || CDF::OrderAction::IsReschedule(dFmtActionCd))
		{
			modifyOrders.push_back(pOrderObj);
		}
	}

	CInpatientOrderScheduleServiceCaller inpatientOrderScheduleService(m_hPatCon);

	const bool bNewOrderSuccess = inpatientOrderScheduleService.CalculateNewOrderSchedule(newOrders,
								  CalculateNewOrderScheduleRequest::eStopDateTimeChanged);
	const bool bModifySuccess = inpatientOrderScheduleService.CalculateModifyOrderSchedule(modifyOrders,
								CalculateModifyOrderScheduleRequest::eStopDateTimeChanged);
}