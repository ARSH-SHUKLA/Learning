#include "StdAfx.h"

#include "ConstantIndicatorHelper.h"
#include "OrderDetailHelper.h"
#include "OrderDurationHelper.h"

#include <OrderDetailHelper.h>
#include <pvorderobj.h>

ERelatedFldsStatus CConstantIndicatorHelper::UpdateConstantIndicatorAcceptFlag(PvOrderObj& orderObj)
{
	PvOrderFld* pConstantFld = orderObj.m_orderFldArr.GetFieldFromMeanId(eDetailConstantInd);

	if (NULL == pConstantFld)
	{
		return eRFSNoChange;
	}

	if (orderObj.IsIV())
	{
		return COrderDetailHelper::DisableField(orderObj, eDetailConstantInd);
	}

	const double dFrequencyCd = orderObj.m_orderFldArr.GetFieldValFromMeanId(eDetailOrdFrequency);
	const Cerner::Foundations::Calendar startDateTime = orderObj.m_orderFldArr.GetFieldDtTmValFromMeanId(
				eDetailReqStartDtTm);
	const Cerner::Foundations::Calendar stopDateTime = orderObj.m_orderFldArr.GetFieldDtTmValFromMeanId(eDetailOrdStopDtTm);

	if (dFrequencyCd == 0.0)
	{
		// If the duration unit is DOSES set the constant indicator to display only and NO
		const bool bIsPrn = (orderObj.m_orderFldArr.GetFieldValFromMeanId(eDetailSchPrn) == 1.0);

		if (!bIsPrn)
		{
			const bool bDurationInDoses = COrderDurationHelper::IsDurationInDoses(orderObj);

			if (!bDurationInDoses)
			{
				if (!startDateTime.IsNull() && !stopDateTime.IsNull() && startDateTime.Compare(stopDateTime) == 0)
				{
					return COrderDetailHelper::DisableField(orderObj, eDetailConstantInd);
				}
				else
				{
					return COrderDetailHelper::ResetAcceptFlag(orderObj, eDetailConstantInd);
				}
			}
		}
	}

	return COrderDetailHelper::DisableField(orderObj, eDetailConstantInd);
}