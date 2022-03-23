#include "StdAfx.h"

#include "DurationDisplayHelper.h"

#include <pvorderobj.h>
#include <pvorderfld.h>
#include <OrderDetailHelper.h>
#include <pvcdf4009.h>
#include <PvReconciliationUtil.h>

ERelatedFldsStatus CDurationDisplayHelper::SetDurationDisplay(PvOrderObj& orderObj)
{
	const EFreqTypeFlag frequencyTypeFlag = (EFreqTypeFlag)orderObj.GetFreqTypeFlag();

	if (frequencyTypeFlag == eFTFOneTime)
	{
		return COrderDetailHelper::DisableFields(orderObj, eDetailOrdDuration, eDetailOrdDurationUnit);
	}
	else
	{
		return SetDurationDisplayForNonOneTimeOrder(orderObj);
	}
}

ERelatedFldsStatus CDurationDisplayHelper::SetDurationDisplayForNonOneTimeOrder(PvOrderObj& orderObj)
{
	ERelatedFldsStatus durReqStatus = eRFSNoChange;

	PvOrderFld* pDurFld = orderObj.m_orderFldArr.GetFieldFromMeanId(eDetailOrdDuration);
	PvOrderFld* pDurUnitFld = orderObj.m_orderFldArr.GetFieldFromMeanId(eDetailOrdDurationUnit);

	if (NULL != pDurFld && NULL != pDurUnitFld)
	{
		// If either duration or duration unit are filled in and the other is empty.
		// If this occurs we make the empty field required.

		const EOeAccept durOrigAcceptFlag = pDurFld->GetOrigAcceptFlag();
		const EOeAccept durUnitOrigAcceptFlag = pDurUnitFld->GetOrigAcceptFlag();
		const EOeAccept durCurAcceptFlag = pDurFld->GetAcceptFlag();
		const EOeAccept durUnitCurAcceptFlag = pDurUnitFld->GetAcceptFlag();

		// If both fields are currently display only, the current details do not allow a duration (e.g. one time frequency).
		// In this case, we should not apply any logic.

		if (pDurFld->IsEmpty() && pDurUnitFld->IsEmpty())
		{
			// If there are no remaining doses then we should not reset the acceptflag.
			// Because duration and duration unit fields should be required incase of Hard and Dr stop.
			// If cross encounter transfer reconciliation window and there are remaining doses or stop type is soft stop.
			// then reset the accept flags

			if (!(CPvReconciliationUtil::IsEnhancedCrossEncounterTransferMedsRecProfile())
				|| (CPvReconciliationUtil::IsEnhancedCrossEncounterTransferMedsRecProfile()
					&& ((orderObj.GetNumberOfRemainingDoses() > 0) || CDF::StopType::IsSoft(orderObj.GetStopTypeCd()))))
			{
				pDurFld->SetAcceptFlag(durOrigAcceptFlag);
				pDurUnitFld->SetAcceptFlag(durUnitOrigAcceptFlag);

				if (durUnitOrigAcceptFlag != eOeAcceptRequired)
				{
					durReqStatus = eRFSAddNone;
				}
			}
		}

		else
		{
			if (!pDurFld->IsEmpty() && pDurFld->GetVisualValueMask() & VisualValue::eOeVShow)
			{
				if (pDurUnitFld->IsEmpty() || (pDurUnitFld->GetVisualValueMask() & VisualValue::eOeVHide))
				{
					pDurUnitFld->SetValuedSourceInd(eVSUser);
					pDurUnitFld->SetAcceptFlag(eOeAcceptRequired);
					durReqStatus = eRFSRemoveNone;
				}
				else if (!pDurUnitFld->IsEmpty() && (pDurUnitFld->GetVisualValueMask() & VisualValue::eOeVShow))
				{
					pDurFld->SetAcceptFlag(durOrigAcceptFlag);
					pDurUnitFld->SetAcceptFlag(durUnitOrigAcceptFlag);

					if (durUnitOrigAcceptFlag == eOeAcceptRequired)
					{
						durReqStatus = eRFSRemoveNone;
					}
					else
					{
						durReqStatus = eRFSAddNone;
					}
				}
			}
			else if (pDurFld->IsEmpty() || pDurFld->GetVisualValueMask() & VisualValue::eOeVHide)
			{
				if (!pDurUnitFld->IsEmpty() && pDurUnitFld->GetVisualValueMask() & VisualValue::eOeVShow)
				{
					pDurFld->SetValuedSourceInd(eVSUser);
					pDurFld->SetAcceptFlag(eOeAcceptRequired);
					durReqStatus = eRFSTrue;
				}
				else if (!pDurFld->IsEmpty() && pDurFld->GetVisualValueMask() & VisualValue::eOeVHide)
				{
					if (pDurUnitFld->IsEmpty())
					{
						pDurFld->SetAcceptFlag(durOrigAcceptFlag);
						pDurUnitFld->SetAcceptFlag(durUnitOrigAcceptFlag);

						if (durUnitOrigAcceptFlag == eOeAcceptRequired)
						{
							durReqStatus = eRFSRemoveNone;
						}
						else
						{
							durReqStatus = eRFSAddNone;
						}
					}
				}
				else if (!pDurUnitFld->IsEmpty() && pDurUnitFld->GetVisualValueMask() & VisualValue::eOeVHide)
				{
					if (pDurFld->IsEmpty())
					{
						pDurFld->SetAcceptFlag(durOrigAcceptFlag);
						pDurUnitFld->SetAcceptFlag(durUnitOrigAcceptFlag);

						if (durUnitOrigAcceptFlag == eOeAcceptRequired)
						{
							durReqStatus = eRFSRemoveNone;
						}
						else
						{
							durReqStatus = eRFSAddNone;
						}
					}
				}
			}
		}
	}

	return durReqStatus;
}