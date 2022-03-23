#include "StdAfx.h"

#include "PrescriptionScheduleManager.h"

#include <pvorderobj.h>
#include <RelatedFldsStatus.h>
#include <PvDateTimeOffsetUtil.h>
#include <CPS_ImportPVCareCoordCom.h>
#include <PvGenericLoaderExtended.h>
#include <ActionDateTimeHelper.h>

CPrescriptionScheduleManager::CPrescriptionScheduleManager(const HPATCON hPatCon)
	: m_hPatCon(hPatCon)
{

}

void CPrescriptionScheduleManager::UpdatePrescriptionSchedule(IComponent& component, PvOrderObj& orderObj,
		const Cerner::Foundations::Calendar& phaseStartDateTime)
{
	PvOrderFld* pRequestedStartDateTimeFld = orderObj.m_orderFldArr.GetFieldFromMeanId(eDetailReqStartDtTm);

	if (NULL != pRequestedStartDateTimeFld)
	{
		UpdateStartDateTime(component, orderObj, phaseStartDateTime, *pRequestedStartDateTimeFld);
		UpdateStopDateTime(component, orderObj, phaseStartDateTime);

		PvOrderFld::OeFieldValue requestedStartDateTimeFieldValue;
		pRequestedStartDateTimeFld->GetLastOeFieldValue(requestedStartDateTimeFieldValue);

		ERelatedFldsStatus status = CPvGenericLoaderExtended(m_hPatCon).SetSpecificRelatedFieldsWithoutUpdatingSchedule(
										orderObj, eDetailReqStartDtTm, requestedStartDateTimeFieldValue);
	}
}

void CPrescriptionScheduleManager::UpdateStopDateTime(IComponent& component, PvOrderObj& orderObj,
		const Cerner::Foundations::Calendar& phaseStartDateTime)
{
	PvOrderFld* pStopDateTimeFld = orderObj.m_orderFldArr.GetFieldFromMeanId(eDetailOrdStopDtTm);

	if (NULL != pStopDateTimeFld)
	{
		Cerner::Foundations::Calendar stopDateTime = pStopDateTimeFld->GetLastOeFldDtTmValue();

		if (stopDateTime.IsNull())
		{
			const CString sStopDateTimeOffset = pStopDateTimeFld->GetLastOeFldDisplayValue();

			if (!sStopDateTimeOffset.IsEmpty())
			{
				Cerner::Foundations::Calendar calculatedStopDateTime = CPvDateTimeOffsetUtil::AddOffsetToDateTime(phaseStartDateTime,
						sStopDateTimeOffset);
				pStopDateTimeFld->AddOeFieldDtTmValue(calculatedStopDateTime);
			}
		}
	}
}

void CPrescriptionScheduleManager::UpdateStartDateTime(IComponent& component, PvOrderObj& orderObj,
		const Cerner::Foundations::Calendar& phaseStartDateTime, PvOrderFld& requestedStartDateTimeFld)
{
	const Cerner::Foundations::Calendar currentRequestedStartDateTime = requestedStartDateTimeFld.GetLastOeFldDtTmValue();
	Cerner::Foundations::Calendar actionDateTime = CActionDateTimeHelper::GetActionDateTime(orderObj);

	if (currentRequestedStartDateTime.IsNull())
	{
		const CString sRequestedStartDateTimeOffset = requestedStartDateTimeFld.GetLastOeFldDisplayValue();

		if (sRequestedStartDateTimeOffset.IsEmpty())
		{
			requestedStartDateTimeFld.AddOeFieldDtTmValue((Cerner::Foundations::Calendar)phaseStartDateTime);
			component.PutLinkedToPhaseStartDateTime(TRUE);
		}
		else
		{
			Cerner::Foundations::Calendar requestedStartDateTime = CPvDateTimeOffsetUtil::AddOffsetToDateTime(phaseStartDateTime,
					sRequestedStartDateTimeOffset);
			requestedStartDateTimeFld.AddOeFieldDtTmValue(requestedStartDateTime);
			component.PutLinkedToPhaseStartDateTime(FALSE);
		}

		if (actionDateTime.Compare(requestedStartDateTimeFld.GetLastOeFldDtTmValue()) > 0)
		{
			requestedStartDateTimeFld.AddOeFieldDtTmValue((Cerner::Foundations::Calendar)actionDateTime);
			component.PutLinkedToPhaseStartDateTime(FALSE);
		}
	}
	else if (component.GetLinkedToPhaseStartDateTime() == TRUE || currentRequestedStartDateTime.Compare(actionDateTime) < 0)
	{
		if (actionDateTime.Compare(phaseStartDateTime) > 0)
		{
			requestedStartDateTimeFld.AddOeFieldDtTmValue((Cerner::Foundations::Calendar)actionDateTime);
			component.PutLinkedToPhaseStartDateTime(FALSE);
		}
		else
		{
			requestedStartDateTimeFld.AddOeFieldDtTmValue((Cerner::Foundations::Calendar)phaseStartDateTime);
			component.PutLinkedToPhaseStartDateTime(TRUE);
		}
	}
}