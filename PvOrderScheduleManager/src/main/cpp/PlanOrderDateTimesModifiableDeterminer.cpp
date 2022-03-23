#include "StdAfx.h"

#include "PlanOrderDateTimesModifiableDeterminer.h"

#include <pvorderobj.h>
#include <pvorderpcobj.h>
#include <CPS_ImportPVCareCoordCom.h>
#include <ActionDateTimeHelper.h>

// Check to make sure the start dt/tm can be modified.
// PreReq:  The order must have already loaded it's details before calling this function.
bool CPlanOrderDateTimesModifiableDeterminer::IsOrderStartDateTimeModifiable(PvOrderObj& orderObj) const
{
	PvOrderFld* pStartDtTmFld = orderObj.m_orderFldArr.GetFieldFromMeanId(eDetailReqStartDtTm);

	if (!pStartDtTmFld)
	{
		return false;
	}

	PvOrderPCObj* pOrderPCObj = dynamic_cast<PvOrderPCObj*>(&orderObj);

	if (NULL != pOrderPCObj)
	{
		if (pOrderPCObj->GetDummyParentFlag())
		{
			return false;
		}
	}

	IComponentPtr pIComponent(TransferDispatch(orderObj.GetPlanComponent()));

	if (pIComponent != NULL)
	{
		IPhasePtr pIPhase(pIComponent->GetParentDispatch());

		if (NULL != pIPhase)
		{
			return AreOrderComponentsDateTimesModifiable(*pIPhase, *pIComponent, orderObj, *pStartDtTmFld);
		}
	}

	return true;
}

bool CPlanOrderDateTimesModifiableDeterminer::AreOrderComponentsDateTimesModifiable(IPhase& phase,
		IComponent& component, PvOrderObj& orderObj, PvOrderFld& startDateTimeFld) const
{
	const CString sCalcPlanStatusMean = (LPCTSTR)phase.GetCalcPlanStatusMean();
	const CString compStatusMean = (LPCTSTR)component.GetCompStatusMean();

	PvOrderFld::OeFieldValue startDateTimeFldValue;
	startDateTimeFld.GetOrigOeFieldValue(startDateTimeFldValue);

	Cerner::Foundations::Calendar actionDateTime = CActionDateTimeHelper::GetActionDateTime(orderObj);

	if (!startDateTimeFldValue.IsEmpty() && startDateTimeFldValue.oeFieldDtTmValue <= actionDateTime)
	{
		if (component.GetInitiated() == TRUE)
		{
			if (compStatusMean == _T("ACTIVATED"))
			{
				const bool bFuturePlanStatus = sCalcPlanStatusMean == _T("FUTURE") || sCalcPlanStatusMean == _T("FUTUREREVIEW");

				if (!bFuturePlanStatus)  // Future Initiate only allow a modify in a future status if the order is in a future status.
				{
					const EOrderStatus orderStatus = orderObj.GetStatus();

					if (orderStatus != eOrdFuture)
					{
						return false;
					}
				}
			}
		}
	}

	return true;
}

/////////////////////////////////////////////////////////////////////////////
/// \fn		bool CPlanOrderDateTimesModifiableDeterminer::IsOrderStopDateTimeModifiable(PvOrderObj& orderObj) const
/// \brief		Will check if the stop date time of the orderable can be modified.
///
/// \return     bool - True if the stop date time can be modified.
///
/// \param[in]  PvOrderObj& orderObj
///
/// \owner		SN069613
/////////////////////////////////////////////////////////////////////////////
bool CPlanOrderDateTimesModifiableDeterminer::IsOrderStopDateTimeModifiable(PvOrderObj& orderObj) const
{
	PvOrderFld* pStopDtTmFld = orderObj.m_orderFldArr.GetFieldFromMeanId(eDetailOrdStopDtTm);

	if (pStopDtTmFld == nullptr)
	{
		return false;
	}

	EOeAccept acceptFlag = pStopDtTmFld->GetAcceptFlag();

	if (eOeAcceptDisplayOnly == acceptFlag || eOeAcceptNoDisplay == acceptFlag)
	{
		return false;
	}

	return true;
}