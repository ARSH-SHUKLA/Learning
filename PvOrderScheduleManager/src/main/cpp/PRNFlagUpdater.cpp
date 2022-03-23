#include "StdAfx.h"
#include "resource.h"

#include "PRNFlagUpdater.h"

#include <OrderDetailHelper.h>
#include <pvorderobj.h>
#include <pvorderfld.h>
#include <PvOrdTimer.h>
#include <pvordutil.h>
#include <OrderMessageBox.h>

/////////////////////////////////////////////////////////////////////////////
/// \fn      bool CPvPRNDetailManagerInpatient::SetPRNFlagsForInpatientOrder(PvOrderObj& orderObj, const bool bSuppressWarning)
/// \brief      Sets the accept flags of PRN Reason and Instruction fields based on their OEF values, current values, and the state of the PRN Indicator.
///
/// \return     bool - True if flags could be set. Otherwise, false.
///
/// \param[in]  PvOrderObj& orderObj - Order object to update the PRN flags for.
/// \param[in]  const bool bSuppressWarning - True if warning should be suppressed when reason & instructions field don't exist.
/// \owner		DB024248
/////////////////////////////////////////////////////////////////////////////
bool CPRNFlagUpdater::SetPRNFlagsForInpatientOrder(PvOrderObj& orderObj, const bool bSuppressWarning) const
{
	PvOrderFld* pPrnFld = orderObj.m_orderFldArr.GetFieldFromMeanId(eDetailSchPrn);

	if (NULL != pPrnFld)
	{
		const bool bIsPRNOrder = pPrnFld->GetLastOeFldValue() == 1;

		if (bIsPRNOrder)
		{
			return SetFlagsForPRNOrder(orderObj, bSuppressWarning);
		}
		else
		{
			COrderDetailHelper::ResetAcceptFlags(orderObj, eDetailPRNReason, eDetailPRNInstructions);
		}
	}

	return true;
}

/////////////////////////////////////////////////////////////////////////////
/// \fn      bool CPvPRNDetailManagerInpatient::SetFlagsForPRNOrder(PvOrderObj& orderObj, const bool bSuppressWarning)
/// \brief      Sets the PRN order's accept flags of PRN Reason and Instruction fields based on their OEF values and current values.
///
/// \return     bool - True if flags could be set. Otherwise, false.
///
/// \param[in]  PvOrderObj& orderObj - PRN order object to update the PRN flags for.
/// \param[in]  const bool bSuppressWarning - True if warning should be suppressed when reason & instructions field don't exist.
/// \owner		DB024248
/////////////////////////////////////////////////////////////////////////////
bool CPRNFlagUpdater::SetFlagsForPRNOrder(PvOrderObj& orderObj, const bool bSuppressWarning) const
{
	PvOrderFld* pPrnReasonFld = orderObj.m_orderFldArr.GetFieldFromMeanId(eDetailPRNReason);
	PvOrderFld* pPrnInstrFld = orderObj.m_orderFldArr.GetFieldFromMeanId(eDetailPRNInstructions);

	if (!bSuppressWarning)
	{
		if (NULL == pPrnReasonFld && NULL == pPrnInstrFld)
		{
			const double dCatalogTypeCd = orderObj.GetCatalogTypeCd();

			if (CDF::CatalogType::IsPharmacy(dCatalogTypeCd))
			{
				// PRN Indicator is set to yes, but neither PRN Instructions or PRN Reason exists on the format.
				Orders::MessageBox(nullptr, MAKEINTRESOURCE(IDS_ERR_PRN_OEF), MAKEINTRESOURCE(IDS_APP_NAME),
								   MB_OK | MB_ICONEXCLAMATION);
				return false;
			}
		}
	}

	if (pPrnReasonFld != nullptr && !pPrnReasonFld->IsEmpty())
	{
		COrderDetailHelper::SetAcceptFlag(orderObj, eDetailPRNReason, eOeAcceptRequired);

		if (pPrnInstrFld != nullptr && pPrnInstrFld->GetAcceptFlag() == eOeAcceptRequired)
		{
			COrderDetailHelper::SetAcceptFlagAndValueRequiredInd(orderObj, eDetailPRNInstructions, eOeAcceptOptional, eIndFalse);
		}
	}
	else if (pPrnInstrFld != nullptr && !pPrnInstrFld->IsEmpty())
	{
		COrderDetailHelper::SetAcceptFlag(orderObj, eDetailPRNInstructions, eOeAcceptRequired);

		if (pPrnReasonFld != nullptr && pPrnReasonFld->GetAcceptFlag() == eOeAcceptRequired)
		{
			COrderDetailHelper::SetAcceptFlagAndValueRequiredInd(orderObj, eDetailPRNReason, eOeAcceptOptional, eIndFalse);
		}
	}
	else
	{
		// Require both reason and instructions if both are required on the OEF.  Otherwise, require whichever field has the strictest flag on the OEF.
		// If both fields have the same flag but it is not required, prefer reason to instructions.
		const EOeAccept prnReasonAcceptFlag = (NULL == pPrnReasonFld ? eOeAcceptEmpty : pPrnReasonFld->GetOrigAcceptFlag());
		const EOeAccept prnInstrAcceptFlag = (NULL == pPrnInstrFld ? eOeAcceptEmpty : pPrnInstrFld->GetOrigAcceptFlag());
		const EOeAccept higherAcceptFlag = PvOrdUtil::IdentifyHigherMeritAcceptFlag(prnReasonAcceptFlag, prnInstrAcceptFlag);

		if (prnInstrAcceptFlag == eOeAcceptRequired && prnReasonAcceptFlag == eOeAcceptRequired)
		{
			COrderDetailHelper::SetAcceptFlag(orderObj, eDetailPRNReason, eOeAcceptRequired);
			COrderDetailHelper::SetAcceptFlag(orderObj, eDetailPRNInstructions, eOeAcceptRequired);
		}
		else if (higherAcceptFlag == prnReasonAcceptFlag)
		{
			COrderDetailHelper::SetAcceptFlag(orderObj, eDetailPRNReason, eOeAcceptRequired);
			COrderDetailHelper::ResetAcceptFlag(orderObj, eDetailPRNInstructions);
		}
		else
		{
			COrderDetailHelper::SetAcceptFlag(orderObj, eDetailPRNInstructions, eOeAcceptRequired);
			COrderDetailHelper::ResetAcceptFlag(orderObj, eDetailPRNReason);
		}
	}

	return true;
}
