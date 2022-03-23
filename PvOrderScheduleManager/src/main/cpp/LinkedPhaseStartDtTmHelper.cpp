#include "stdafx.h"
#include "LinkedPhaseStartDtTmHelper.h"
#include <PvOrdCalUtil.h>
#include "OrderStartDateTimeUpdater.h"
#include "PharmacyPriorityHelper.h"

/////////////////////////////////////////////////////////////////////////////
/// \fn         bool CLinkedPhaseStartDtTmHelper::IsComponentLinkedToValidPhase(IComponent& component, IPlanPtr pIPlan)
///
/// \brief      This method verifies if the component is linked to a valid phase. It also checks for Do Not Order Action on the phase
///
/// \return     bool
///
/// \param[in]  IComponent& component - component to be verified for link to phases
///
/// \param[in]  IPlanPtr pIPlan - plan containing the phases that the component may or may not be linked to
///
/// \owner      RH043349
/////////////////////////////////////////////////////////////////////////////
bool CLinkedPhaseStartDtTmHelper::IsComponentLinkedToValidPhase(IComponent& component, IPlanPtr pIPlan)
{
	const double dComponentId = component.GetActCompId();

	if (pIPlan == nullptr)
	{
		return false;
	}

	CArray <IPhase*, IPhase*> arrScheduledPhases;
	pIPlan->GetSchedulablePhasesByComponentId(dComponentId, (LONG_PTR)&arrScheduledPhases);

	INT_PTR iSize = arrScheduledPhases.GetSize();

	if (iSize <= 0)
	{
		return false;
	}

	for (INT_PTR iIndex = 0; iIndex < iSize; iIndex++)
	{
		IPhasePtr pIPhase(arrScheduledPhases.GetAt(iIndex));

		if (pIPhase == nullptr)
		{
			continue;
		}

		if (pIPhase->GetActionType() != eActionDoNotOrder)
		{
			return true;
		}
	}

	return false;
}

/////////////////////////////////////////////////////////////////////////////
/// \fn		    void CLinkedPhaseStartDtTmHelper::SetLinkedPhaseStartDtTmToAnchorOrder(PvOrderObj& orderObj, IComponent& component)
/// \brief		This funtion is used to set correct anchor order date/time with the help linked phase start date/time and default
///				start time which is set into Order sentence or Order Entry Format tool.
///				If anchor order doesn't have default start time built
///				then we need to check that default start time is set into the OS or OEF tool or not,
///				if default start time is not set into the Order Sentence or Order Entry Format tool then
///				we will set linked phase start date/time to schedule(anchor) order start date/time.
///
///				If anchor order has default start time built
///				(i.e. 0001 built on the order sentence or a default from the order entry format field),
///				the start time on the anchor order will be set to that default time and the start date on
///				the order will be updated to match the start date of the phase that the anchor order is associated with.
/// \return		void
///
/// \owner		DB024248
/////////////////////////////////////////////////////////////////////////////
void CLinkedPhaseStartDtTmHelper::SetLinkedPhaseStartDtTmToAnchorOrder(PvOrderObj& orderObj, IComponent& component)
{
	ICalendarPtr pIOrderStartDtTm(__uuidof(Calendar));
	pIOrderStartDtTm->InitToNull();

	_bstr_t bstrSystemRequestedStartString = component.GetSystemRequestedStartString();

	if (bstrSystemRequestedStartString.length() > 0)
	{
		pIOrderStartDtTm->InitFromHandle(orderObj.GetCurrentStartDtTm().GetHandle());
	}

	CPharmacyPriorityHelper::SetPharmacyPriorityToRoutineIfStatOrNow(orderObj);
	LPDISPATCH pIStartDtTmDisp(pIOrderStartDtTm);

	if (component.CalculateAnchorComponentStartDateTime(&pIStartDtTmDisp))
	{
		ICalendarPtr pIStartDateTime = (ICalendarPtr)pIStartDtTmDisp;

		Cerner::Foundations::Calendar requestedStartDateTime = PvOrdCalUtils::GetDateOnlyCalendarFromInterface(pIStartDateTime);
		COrderStartDateTimeUpdater::SetRequestedStartDateTime(orderObj, requestedStartDateTime, true);
	}

	if (component.GetTZActiveInd() == TRUE)
	{
		component.PutTZActiveInd(FALSE);
		component.PutModifiedTZRelationInd(TRUE);
	}

	component.PutLinkedToPhaseStartDateTime(FALSE);		
}

/////////////////////////////////////////////////////////////////////////////
/// \fn		    void CLinkedPhaseStartDtTmHelper::IsRequestedStartDateTimeManuallyEntered(IComponent& component, PvOrderObj& orderObj)
/// \brief		This funtion is used to check if the start date time is entered manually by the user
///
///				
/// \return		bool
///
/// \owner		DB024248
/////////////////////////////////////////////////////////////////////////////
bool CLinkedPhaseStartDtTmHelper::IsRequestedStartDateTimeManuallyEntered(IComponent& component, PvOrderObj& orderObj)
{
	const bool bLinkedToPhaseStart = component.GetLinkedToPhaseStartDateTime() == TRUE;

	if (bLinkedToPhaseStart)
	{
		const bool bProtocolReviewPhaseBeingAccepted = IsProtocolReviewBeingAccepted(component);

		if (bProtocolReviewPhaseBeingAccepted)
		{
			return false;
		}
	}

	Cerner::Foundations::Calendar manuallyEnteredRequestedStartDateTime = orderObj.m_orderFldArr.GetFieldDtTmValFromMeanId(
		eDetailReqStartDtTm);

	if (!manuallyEnteredRequestedStartDateTime.IsNull())
	{
		return true;
	}

	return false;
}

/////////////////////////////////////////////////////////////////////////////
/// \fn		    bool CLinkedPhaseStartDtTmHelper::IsProtocolReviewBeingAccepted(IComponent& component)
/// \brief		This funtion is used to check if the review required phase is accepted or not
///
///				
/// \return		bool
///
/// \owner		DB024248
/////////////////////////////////////////////////////////////////////////////
bool CLinkedPhaseStartDtTmHelper::IsProtocolReviewBeingAccepted(IComponent& component)
{
	IPhasePtr pIPhase = PowerPlanUtil::GetPhase(&component);

	if (pIPhase != nullptr)
	{
		// The phase for which we check if protocol review has been accepted.
		IPhase* pIReviewPhase(nullptr);

		if (pIPhase->GetPhaseType() == eSubPhase)
		{
			// If the current phase is a subphase, we must check its parent phase for the review required status
			// as we set review required status only for a phase and not for a subphase.
			pIReviewPhase = pIPhase->GetParentPhase();
		}
		else
		{
			pIReviewPhase = pIPhase;
		}

		if (pIReviewPhase != nullptr)
		{
			const CString sCalcPlanStatusMean = (LPCTSTR)pIReviewPhase->GetCalcPlanStatusMean();

			if (sCalcPlanStatusMean == "INITREVIEW" || sCalcPlanStatusMean == "FUTUREREVIEW")
			{
				const PROTOCOL_REVIEW_STATUS_FLAG enumPendingProtReviewStatus = pIReviewPhase->GetPendingReviewStatusFlag();

				if (enumPendingProtReviewStatus == eProtReviewStatusCompleted)
				{
					return true;
				}
			}
		}
	}

	return false;
}
