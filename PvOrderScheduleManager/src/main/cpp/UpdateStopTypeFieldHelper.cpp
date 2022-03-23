// UpdateStopTypeField.cpp: implementation file.Helper class to set stop type field whenever we change the plan order duration or frequency.
//
//////////////////////////////////////////////////////////////////////
#include "StdAfx.h"

#include <pvorderobj.h>
#include <pvorderfld.h>
#include "UpdateStopTypeFieldHelper.h"
#include "pvcdf4009.h"

/////////////////////////////////////////////////////////////////////////////
/// \fn			void CUpdateStopTypeFieldHelper::UpdateStopTypeFldToPhysicianStop(PvOrderObj& orderObj)
/// \brief		Handles updating the stop type to physician stop if plan order duration is manually changed
///				or frequency is updated to one-time frquency.
///
/// \return		void - return nothing
///
/// \param[in]	PvOrderObj& orderObj - The order whose duration changed.
/// \owner		KM050466
/////////////////////////////////////////////////////////////////////////////
void CUpdateStopTypeFieldHelper::UpdateStopTypeFldToPhysicianStop(PvOrderObj& orderObj)
{
	PvOrderFld::OeFieldValue oeFldVal;
	PvOrderFld* pOrderFld = orderObj.m_orderFldArr.GetFieldFromMeanId(eDetailStopType);

	if (pOrderFld != nullptr)
	{
		double dStopTypeCd(CDF::StopType::GetDrStopCd());
		oeFldVal.oeFieldValue = dStopTypeCd;
		oeFldVal.oeFieldDisplayValue = CodeValue_GetDisplay(CDF::StopType::CODESET, dStopTypeCd);
		pOrderFld->AddOeFieldValue(oeFldVal);
		pOrderFld->SetValuedSourceInd(eVSUser);
	}
}