#include "StdAfx.h"

#include "ComponentOffsetAndPriorityCombiner.h"
#include "ComponentOffsetHelper.h"
#include "CollectionPriorityOffsetDeterminer.h"
#include "GenericPriorityOffsetDeterminer.h"
#include "SchedulingInstructionsHelper.h"
#include <pvorderobj.h>
#include <PriorityDefs.h>
#include <PvDateTimeOffsetUtil.h>
#include <CPS_ImportPVCareCoordCom.h>
#include <OrderContextImports.h>
#include <preferenceutility.h>

Cerner::Foundations::Calendar CComponentOffsetAndPriorityCombiner::CombineComponentOffsetAndPriority(
	IComponent& component, PvOrderObj& orderObj, const Cerner::Foundations::Calendar& phaseStartDateTime)
{
	const bool bHasComponentOffset = component.HasComponentOffset() == TRUE;

	const CString startDateTimeOffset = GetStartDateTimeOffset(component, orderObj, bHasComponentOffset);

	if (!startDateTimeOffset.IsEmpty())
	{
		if (startDateTimeOffset == _T(";") && orderObj.IsNonMedOrder() && !bHasComponentOffset)
		{
			return Cerner::Foundations::Calendar::CreateNull();
		}

		if (!bHasComponentOffset)
		{
			return CPvDateTimeOffsetUtil::AddOffsetToDateTime(phaseStartDateTime, startDateTimeOffset);
		}

		const bool bOffsetInDays = CComponentOffsetHelper::DoesComponentHaveOffsetInDays(component);
		const bool bOffsetHasAbsoluteTime = CPvDateTimeOffsetUtil::DoesOffsetHaveAbsoluteTime(startDateTimeOffset);

		if (bOffsetHasAbsoluteTime && !bOffsetInDays)
		{
			CComponentOffsetHelper::ClearComponentOffset(component);

			return CPvDateTimeOffsetUtil::AddOffsetToDateTime(phaseStartDateTime, startDateTimeOffset);
		}

		component.PutLinkedToPhaseStartDateTime(TRUE);

		if (bOffsetInDays)
		{
			Cerner::Foundations::Calendar startDateTimeWithComponentOffset = CComponentOffsetHelper::AddComponentOffset(component,
					phaseStartDateTime);
			return CPvDateTimeOffsetUtil::AddTimeOnlyPortionOfOffsetToDateTime(startDateTimeWithComponentOffset,
					startDateTimeOffset);
		}
		else
		{
			Cerner::Foundations::Calendar startDateTimeWithOffsetDays = CPvDateTimeOffsetUtil::AddDateOnlyPortionOfOffsetToDateTime(
						phaseStartDateTime, startDateTimeOffset);
			return CComponentOffsetHelper::AddComponentOffset(component, startDateTimeWithOffsetDays);
		}
	}

	if (bHasComponentOffset)
	{
		return CComponentOffsetHelper::AddComponentOffset(component, phaseStartDateTime);
	}

	const CString sSchedulingInstructionsOffset = CSchedulingInstructionsHelper::DetermineSchedulingInstructionsOffset(
				orderObj);

	if (!sSchedulingInstructionsOffset.IsEmpty())
	{
		component.PutLinkedToPhaseStartDateTime(TRUE);
		return CPvDateTimeOffsetUtil::AddOffsetToDateTime(phaseStartDateTime, sSchedulingInstructionsOffset);
	}

	const CString sInpStartDtTmPaddingPref = GetStartDateTimePaddingVal(component, orderObj);
	if (!sInpStartDtTmPaddingPref.IsEmpty())
	{
		component.PutLinkedToPhaseStartDateTime(TRUE);
		return CPvDateTimeOffsetUtil::AddOffsetToDateTime(phaseStartDateTime, sInpStartDtTmPaddingPref);
	}	

	return phaseStartDateTime;
}

CString CComponentOffsetAndPriorityCombiner::GetStartDateTimeOffset(IComponent& component, PvOrderObj& orderObj,
		const bool bHasComponentOffset)
{
	Cerner::Foundations::Calendar requestedStartDateTime = orderObj.m_orderFldArr.GetFieldDtTmValFromMeanId(
				eDetailReqStartDtTm);
	CString sRequestedStartDateTimeOffset = orderObj.m_orderFldArr.GetFieldDispFromMeanId(eDetailReqStartDtTm);

	if (!orderObj.IsNonMedOrder() && sRequestedStartDateTimeOffset == _T(";"))
	{
		sRequestedStartDateTimeOffset = _T("T;N");
	}

	if (orderObj.IsNonMedOrder() && sRequestedStartDateTimeOffset == _T(";") && bHasComponentOffset)
	{
		PvOrderFld* pRequestedStartDtTmFld = orderObj.m_orderFldArr.GetFieldFromMeanId(eDetailReqStartDtTm);

		if (pRequestedStartDtTmFld != nullptr)
		{
			pRequestedStartDtTmFld->SetDefaultValue("");
		}
	}

	if (requestedStartDateTime.IsNull() && !sRequestedStartDateTimeOffset.IsEmpty())
	{
		if (!IsTodayNowOffset(sRequestedStartDateTimeOffset))
		{
			component.PutLinkedToPhaseStartDateTime(FALSE);
		}

		return sRequestedStartDateTimeOffset;
	}

	Cerner::Foundations::Calendar referenceStartDateTime = orderObj.m_orderFldArr.GetFieldDtTmValFromMeanId(
				eDetailReferenceStartDtTm);
	CString sReferenceStartDateTimeOffset = orderObj.m_orderFldArr.GetFieldDispFromMeanId(eDetailReferenceStartDtTm);

	if (!orderObj.IsNonMedOrder() && sReferenceStartDateTimeOffset == _T(";"))
	{
		sReferenceStartDateTimeOffset = _T("T;N");
	}

	if (orderObj.IsNonMedOrder() && sReferenceStartDateTimeOffset == _T(";") && bHasComponentOffset)
	{
		PvOrderFld* pReferenceStartDateTimeFld = orderObj.m_orderFldArr.GetFieldFromMeanId(eDetailReferenceStartDtTm);

		if (pReferenceStartDateTimeFld != nullptr)
		{
			pReferenceStartDateTimeFld->SetDefaultValue("");
		}
	}

	if (referenceStartDateTime.IsNull() && !sReferenceStartDateTimeOffset.IsEmpty())
	{
		if (!IsTodayNowOffset(sReferenceStartDateTimeOffset))
		{
			component.PutLinkedToPhaseStartDateTime(FALSE);
		}

		return sReferenceStartDateTimeOffset;
	}

	const CString sCollectionPriorityOffset = CCollectionPriorityOffsetDeterminer::DetermineCollectionPriorityOffset(
				orderObj);

	if (!sCollectionPriorityOffset.IsEmpty())
	{
		return sCollectionPriorityOffset;
	}

	const CString sGenericPriorityOffset = CGenericPriorityOffsetDeterminer::DetermineGenericPriorityOffset(orderObj);

	if (!sGenericPriorityOffset.IsEmpty())
	{
		return sGenericPriorityOffset;
	}

	return "";
}

bool CComponentOffsetAndPriorityCombiner::IsTodayNowOffset(CString sOffset)
{
	sOffset.MakeUpper();

	if (sOffset == _T("T;N"))
	{
		return true;
	}

	return false;
}

CString CComponentOffsetAndPriorityCombiner::GetStartDateTimePaddingVal(IComponent& component, PvOrderObj& orderObj)
{
	IPlanPtr pIPlan = PowerPlanUtil::GetPlan(&component);

	if (pIPlan != nullptr && pIPlan->IsAnIVSequence(eActiveSequence))
	{
		return "";
	}

	const double dFmtActionCd = orderObj.GetFmtActionCd();
	
	if (CDF::OrderAction::IsOrder(dFmtActionCd) && orderObj.IsInpatientMedOrder())
	{
		const double dFacilityCd = OrderContext_GetFacilityCd(orderObj.GetPatientId());
		const double dNurseUnitCd = OrderContext_GetNurseUnitCd(orderObj.GetPatientId());
		return CPreferenceUtility::GetInpatientPharmacyOrderStartTimePadding(dFacilityCd, dNurseUnitCd);
	}

	return "";
}

/////////////////////////////////////////////////////////////////////////////
/// \fn        Cerner::Foundations::Calendar ApplyStartDateTimePadding(IComponent& component, PvOrderObj& orderObj,
///														const Cerner::Foundations::Calendar& dateTimeToBeUpdated)
/// \brief      This method verifies if the component order has valid start date time padding and apply the information to action date time
///
/// \return     Cerner::Foundations::Calendar
///
/// \param[in]  IComponent& component - component to be validate
/// \param[in]  PvOrderObj& orderObj - component order to be validate
/// \param[in]  const Cerner::Foundations::Calendar dateTimeToBeUpdated - date time to be updated
///
/// \owner      
/////////////////////////////////////////////////////////////////////////////
Cerner::Foundations::Calendar CComponentOffsetAndPriorityCombiner::ApplyStartDateTimePadding(IComponent& component, PvOrderObj& orderObj,
	const Cerner::Foundations::Calendar& dateTimeToBeUpdated)
{
	const CString sInpStartDtTmPaddingPref = GetStartDateTimePaddingVal(component, orderObj);
	
	if (!sInpStartDtTmPaddingPref.IsEmpty())
	{
		return CPvDateTimeOffsetUtil::AddOffsetToDateTime(dateTimeToBeUpdated, sInpStartDtTmPaddingPref);
	}
	
	return dateTimeToBeUpdated;
}