#include "StdAfx.h"

#include "ModifiableOrderRetriever.h"
#include "PlanOrderDateTimesModifiableDeterminer.h"

#include <pvorderobj.h>
#include <pvsingletonmgr.h>
#include <CPS_ImportPVCareCoordCom.h>
#include <PvGenericLoaderExtended.h>
#include <PvPlanActionAvailabilityExp.h>
#include <dcp_genericloader.h>

CModifiableOrderRetriever::CModifiableOrderRetriever(const HPATCON hPatCon)
	: m_hPatCon(hPatCon)
{

}

PvOrderObj* CModifiableOrderRetriever::GetModifiableOrderFromComponent(IComponent& orderComponent) const
{
	const double dOrderId = orderComponent.GetParentEntId();

	if (dOrderId <= 0.0)
	{
		return NULL;
	}

	const bool bOrderWasAddedToScratchpad = PrepareComponentIfNeeded(orderComponent, dOrderId);

	PvOrderObj* pOrderObj = CGenLoader().GetOrderObjFromComponent(m_hPatCon, orderComponent);

	if (NULL != pOrderObj)
	{
		if (pOrderObj->IsDayOfTreatmentOrder())
		{
			if (bOrderWasAddedToScratchpad)
			{
				// We needed to invoke a reschedule on the DoT order but doing that launches a dialog and calls DirectSign().
				// So we instead invoke a modify and then hack it here. Your welcome pharmacy.

				pOrderObj->SetFmtActionCd(CDF::OrderAction::GetRescheduleCd());
			}
		}

		CPlanOrderDateTimesModifiableDeterminer planOrderDateTimesModifiableDeterminer;
		const bool bOrderDateTimesModifiable = planOrderDateTimesModifiableDeterminer.IsOrderStartDateTimeModifiable(*pOrderObj);

		if (!bOrderDateTimesModifiable)
		{
			if (bOrderWasAddedToScratchpad)
			{
				const ESpType enumSpType = GetScratchpadType();

				if (enumSpType == eLocalSp)
				{
					CPvGenericLoaderExtended(m_hPatCon).RemoveFromLocalSp(dOrderId);
				}
				else
				{
					CPvGenericLoaderExtended(m_hPatCon).Remove(dOrderId);
				}
			}

			return NULL;
		}
	}

	return pOrderObj;
}

// Returns true if order was added to scratchpad.
bool CModifiableOrderRetriever::PrepareComponentIfNeeded(IComponent& orderComponent, const double dOrderId) const
{
	const bool bOnSp = CPvGenericLoaderExtended(m_hPatCon).IsItemOnSp(dOrderId);

	if (!bOnSp)
	{
		BOOL bModifyAvailable = CPvGenericLoaderExtended(m_hPatCon).IsComponentModifyAvail(&orderComponent);

		if (bModifyAvailable == TRUE)
		{
			const ESpType enumSpType = GetScratchpadType();
			_bstr_t actionMean(_T("PHASE_RESCHEDULE"));
			BOOL bActionInvoked = CPvGenericLoaderExtended(m_hPatCon).PrepareComponentOrderForAction(actionMean, dOrderId,
								  (long)enumSpType);

			if (!bActionInvoked)
			{
				return false;
			}

			return true;
		}
	}

	return false;
}

ESpType CModifiableOrderRetriever::GetScratchpadType() const
{
	PlanOrderCreationContext* pPlanOrderCreationContext = NULL;
	GetSingleInstance(PLANORDERCREATIONCONTEXT_KEY, pPlanOrderCreationContext, m_hPatCon);

	if (NULL != pPlanOrderCreationContext)
	{
		if (pPlanOrderCreationContext->bValid)
		{
			return pPlanOrderCreationContext->enumSpType;
		}
	}

	return eGlobalSp;
}

/////////////////////////////////////////////////////////////////////////////
/// \fn		PvOrderObj* CModifiableOrderRetriever::GetModifiableSchedulableOrderFromComponent(IComponent& orderComponent) const
/// \brief		Retrieve a modifable schedulable order object from given schedulable component.
///
/// \return		PvOrderObj* - pointer to modifiable schedulable order object. It can be null.
///
/// \param[in]	IComponent& schedulableComponent - schedulable component.
///
/// \owner		SS026302
/////////////////////////////////////////////////////////////////////////////
PvOrderObj* CModifiableOrderRetriever::GetModifiableSchedulableOrderFromComponent(IComponent& schedulableComponent)
const
{
	const double dOrderId = schedulableComponent.GetOrderId();

	if (dOrderId <= 0.0)
	{
		return nullptr;
	}

	// Set modify action to order component.
	const bool bOrderWasAddedToScratchpad = PrepareComponentIfNeeded(schedulableComponent, dOrderId);

	// Retrieve modifiable schedulable order object from component.
	PvOrderObj* pOrderObj = CGenLoader().GetOrderObjFromComponent(m_hPatCon, schedulableComponent);

	if (pOrderObj == nullptr)
	{
		// If order object isn't available on component then get proposal,
		// this is needed incase of plan copy forward.
		pOrderObj = (PvOrderObj*)schedulableComponent.GetOrderProposal();
	}

	if (pOrderObj != nullptr)
	{
		if (bOrderWasAddedToScratchpad)
		{
			pOrderObj->SetFmtActionCd(CDF::OrderAction::GetModifyCd());
		}
	}

	return pOrderObj;
}

/////////////////////////////////////////////////////////////////////////////
/// \fn		PvOrderObj* CModifiableOrderRetriever::GetModifiableOrderFromComponentForStopDateTimeChange(IComponent& orderComponent) const
/// \brief		Retrieve a modifiable order object from the given order component.
///
/// \return		PvOrderObj* - pointer to modifiable order object. It can be null.
///
/// \param[in]	IComponent& orderComponent - order component.
///
/// \owner		SN069613
/////////////////////////////////////////////////////////////////////////////
PvOrderObj* CModifiableOrderRetriever::GetModifiableOrderFromComponentForStopDateTimeChange(IComponent& orderComponent) const
{
	const double dOrderId = orderComponent.GetParentEntId();

	if (dOrderId <= 0.0)
	{
		return nullptr;
	}

	PvOrderObj* pOrderObj = CGenLoader().GetOrderObjFromComponent(m_hPatCon, orderComponent);

	if (nullptr != pOrderObj)
	{
		CPlanOrderDateTimesModifiableDeterminer planOrderDateTimesModifiableDeterminer;
		const bool bOrderDateTimesModifiable = planOrderDateTimesModifiableDeterminer.IsOrderStopDateTimeModifiable(*pOrderObj);

		if (!bOrderDateTimesModifiable)
		{
			return nullptr;
		}
	}

	return pOrderObj;
}
