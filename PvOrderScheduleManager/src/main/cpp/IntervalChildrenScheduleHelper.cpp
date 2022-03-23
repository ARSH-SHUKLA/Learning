#include "StdAfx.h"

#include "IntervalChildrenScheduleHelper.h"

#include <pvorderpcobj.h>
#include <PvGenericLoaderExtended.h>

CIntervalChildrenScheduleHelper::CIntervalChildrenScheduleHelper(const HPATCON hPatCon)
	: m_hPatCon(hPatCon)
{

}

ERelatedFldsStatus CIntervalChildrenScheduleHelper::SetReqStartDtTmForIntervalChildren(PvOrderPCObj& orderPCObj)
{
	ERelatedFldsStatus returnStatus(eRFSNoChange);

	PvOrderPCObj* pParentPCObj = orderPCObj.GetParent();

	if (NULL == pParentPCObj)
	{
		return eRFSFalse;
	}

	POSITION pos = orderPCObj.GetChildIntervalList()->GetHeadPosition();

	while (NULL != pos)
	{
		const double dOrderId = orderPCObj.GetChildIntervalList()->GetNext(pos);

		PvOrderPCObj* pIntervalObj = FindIntervalChildById(*pParentPCObj, dOrderId);

		if (NULL != pIntervalObj)
		{
			returnStatus = SetReqStartDtTmForIntervalChild(orderPCObj, *pIntervalObj);
		}
	}

	return returnStatus;
}

PvOrderPCObj* CIntervalChildrenScheduleHelper::FindIntervalChildById(PvOrderPCObj& parentPCObj,
		const double dOrderId) const
{
	POSITION posParent = parentPCObj.m_childList.GetHeadPosition();

	while (NULL != posParent)
	{
		PvOrderPCObj* pTempOrdPCObj = (PvOrderPCObj*)parentPCObj.m_childList.GetNext(posParent);

		if (pTempOrdPCObj->GetType() & eOrderPc)
		{
			if (dOrderId == pTempOrdPCObj->GetId())
			{
				return pTempOrdPCObj;
			}
		}
	}

	return NULL;
}

ERelatedFldsStatus CIntervalChildrenScheduleHelper::SetReqStartDtTmForIntervalChild(PvOrderPCObj& orderPCObj,
		PvOrderPCObj& intervalChildPCObj)
{
	if (intervalChildPCObj.GetChildIntverInd() && intervalChildPCObj.GetIntervalOffset() >= 0)
	{
		Cerner::Foundations::Calendar startDtTm = orderPCObj.m_orderFldArr.GetFieldDtTmValFromMeanId(eDetailReqStartDtTm);

		if (!startDtTm.IsNull())
		{
			startDtTm = startDtTm.AddMinutes(intervalChildPCObj.GetIntervalOffset());

			PvOrderFld::OeFieldValue startDtTmFld;
			startDtTmFld.oeFieldDtTmValue = startDtTm;

			PvOrderFld* pStartDtTmFld = intervalChildPCObj.m_orderFldArr.GetFieldFromMeanId(eDetailReqStartDtTm);

			if (NULL != pStartDtTmFld)
			{
				pStartDtTmFld->AddOeFieldDtTmValue(startDtTm);
				pStartDtTmFld->SetVisualValueMask(VisualValue::eOeVShow);
				pStartDtTmFld->SetValuedSourceInd(eVSUser);

				CPvGenericLoaderExtended(m_hPatCon).SetSpecificRelatedFields(intervalChildPCObj, eDetailReqStartDtTm, startDtTmFld);
			}
		}
	}

	return eRFSNoChange;
}