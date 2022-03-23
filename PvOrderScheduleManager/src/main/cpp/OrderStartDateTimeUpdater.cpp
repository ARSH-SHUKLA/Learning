#include "StdAfx.h"

#include "OrderStartDateTimeUpdater.h"

void COrderStartDateTimeUpdater::SetRequestedStartDateTime(PvOrderObj& orderObj,
		const Cerner::Foundations::Calendar& requestedStartDateTime, const bool bClearReference)
{
	PvOrderFld* pRequestedStartDateTimeFld = orderObj.m_orderFldArr.GetFieldFromMeanId(eDetailReqStartDtTm);

	if (NULL != pRequestedStartDateTimeFld)
	{
		if (!requestedStartDateTime.IsNull())
		{
			pRequestedStartDateTimeFld->AddOeFieldDtTmValue(Cerner::Foundations::Calendar(requestedStartDateTime));
		}
		else
		{
			pRequestedStartDateTimeFld->Clear();
		}
	}

	if (bClearReference)
	{
		PvOrderFld* pReferenceStartDateTimeFld = orderObj.m_orderFldArr.GetFieldFromMeanId(eDetailReferenceStartDtTm);

		if (NULL != pReferenceStartDateTimeFld)
		{
			pReferenceStartDateTimeFld->Clear();
		}
	}
}

void COrderStartDateTimeUpdater::SetReferenceStartDateTime(PvOrderObj& orderObj,
		const Cerner::Foundations::Calendar& referenceStartDateTime, const bool bClearRequested)
{
	PvOrderFld* pReferenceStartDateTimeFld = orderObj.m_orderFldArr.GetFieldFromMeanId(eDetailReferenceStartDtTm);

	if (NULL == pReferenceStartDateTimeFld)
	{
		pReferenceStartDateTimeFld = new PvOrderFld(&orderObj);
		pReferenceStartDateTimeFld->SetOeFieldId(EMockOrderFields::eReferenceStartDateTime);
		pReferenceStartDateTimeFld->SetOeFieldMeaningId(eDetailReferenceStartDtTm);
		pReferenceStartDateTimeFld->SetOrigAcceptFlag(eOeAcceptNoDisplay);
		pReferenceStartDateTimeFld->SetAcceptFlag(eOeAcceptNoDisplay);
		pReferenceStartDateTimeFld->SetFieldTypeFlag(eOrdDateTime);

		orderObj.m_orderFldArr.AddTempFld(pReferenceStartDateTimeFld);
	}

	if (!referenceStartDateTime.IsNull())
	{
		pReferenceStartDateTimeFld->AddOeFieldDtTmValue(Cerner::Foundations::Calendar(referenceStartDateTime));
	}
	else
	{
		pReferenceStartDateTimeFld->Clear();
	}

	if (bClearRequested)
	{
		PvOrderFld* pRequestedStartDateTimeFld = orderObj.m_orderFldArr.GetFieldFromMeanId(eDetailReqStartDtTm);

		if (NULL != pRequestedStartDateTimeFld)
		{
			pRequestedStartDateTimeFld->Clear();
		}
	}
}