// PrnIndiactorDeterminer.cpp: implementation of the CPrnIndiactorDeterminer class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "PrnIndicatorDeterminer.h"
#include <pvorderobj.h>

CPrnIndicatorDeterminer::CPrnIndicatorDeterminer()
{
}

CPrnIndicatorDeterminer::~CPrnIndicatorDeterminer()
{
}

/////////////////////////////////////////////////////////////////////////////
/// \fn      CPrnIndicatorDeterminer::IsPrnIndSetOnOrder(PvOrderObj& orderObj)
/// \brief      Checks to see if PRN indicator is set on the Order.
///
/// \return     bool- True- If PRN indicator is set to Yes.
///
/// \owner		KM019227
/////////////////////////////////////////////////////////////////////////////
bool CPrnIndicatorDeterminer::IsPrnIndSetOnOrder(PvOrderObj& orderObj)
{
	PvOrderFld* pFld = orderObj.m_orderFldArr.GetFieldFromMeanId(eDetailSchPrn);

	if (NULL != pFld)
	{
		PvOrderFld::OeFieldValue prnVal;
		pFld->GetLastOeFieldValue(prnVal);

		if (prnVal.oeFieldValue == 1)
		{
			return true;
		}
	}

	return false;
}