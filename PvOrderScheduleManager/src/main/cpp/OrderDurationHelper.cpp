#include "StdAfx.h"

#include "OrderDurationHelper.h"

#include <pvorderobj.h>
#include <pvorderfld.h>
#include <dcp_codevaluedata.h>
#include <pvcdf54.h>

bool COrderDurationHelper::IsDurationInDoses(const PvOrderObj& orderObj)
{
	const double dDurationUnitCd = orderObj.m_orderFldArr.GetFieldValFromMeanId(eDetailOrdDurationUnit);

	if (dDurationUnitCd > 0.0)
	{
		if (CDF::Units::DurationUnit::IsDoses(dDurationUnitCd))
		{
			return true;
		}
	}

	return false;
}