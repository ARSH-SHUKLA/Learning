#include "StdAfx.h"

#include "DoTOrderDateTimeUpdater.h"

#include <PvOrdCalUtil.h>

/////////////////////////////////////////////////////////////////////////////
/// \fn		void CDoTOrderDateTimeUpdater::UpdateDoTOrderDateTimes(PvOrderObj& orderObj) const
/// \brief		Checks the following details/attributes of the supplied order object to insure
///				that it will remain eligible for the Manage Orders Service.  If they are invalid,
///				they will be adjusted to satisfy their relative validation requirements.
///					-- Next Dose Dt/Tm (must not occur before Start Dt/Tm)
///					-- Valid Dose Dt/Tm (must not be null, must not occur before Start Dt/Tm)
///					-- Stop Dt/Tm (must not be null, must not occur before Start Dt/Tm)
///
///				Normally these details are taken care of by the MDD service.  However,
///				it is currently unable to calculate them for certain types of orders
///             (like non-meds without a frequency). In these cases, we need to provide
///				these safeguards to avoid having orders stuck in a future status.
///				CRs: 1-7228170912, 1-7615119471, 1-7874899051
///
/// \return		void
///
/// \param[in]	PvOrderObj& orderObj - The DoT order to update the next, valid, and stop date/times for.
/// \owner		DB024248
/////////////////////////////////////////////////////////////////////////////
void CDoTOrderDateTimeUpdater::UpdateDoTOrderDateTimes(PvOrderObj& orderObj) const
{
	if (orderObj.IsDayOfTreatmentOrder() && !orderObj.IsFutureRecurringOrder())
	{
		IPhase* pITreatmentPeriodPhase = PowerPlanUtil::GetPhaseFromOrder(&orderObj);

		if (NULL != pITreatmentPeriodPhase)
		{
			ICalendarPtr pIPhaseStopDateTime = pITreatmentPeriodPhase->GetUTCCalcEndDtTm();
			Cerner::Foundations::Calendar treatmentPeriodStopDateTime = PvOrdCalUtils::GetDateOnlyCalendarFromInterface(
						pIPhaseStopDateTime);

			orderObj.SetTreatmentPeriodStopDtTm(treatmentPeriodStopDateTime);

			UpdateDoTOrderStopDateTime(orderObj, treatmentPeriodStopDateTime);
			UpdateDoTOrderNextDoseDateTime(orderObj);
			UpdateDoTOrderValidDoseDateTime(orderObj);
		}
	}
}

void CDoTOrderDateTimeUpdater::UpdateDoTOrderStopDateTime(PvOrderObj& orderObj,
		Cerner::Foundations::Calendar treatmentPeriodStopDateTime) const
{
	PvOrderFld* pStopDateTimeFld = orderObj.m_orderFldArr.GetFieldFromMeanId(eDetailOrdStopDtTm);

	if (NULL != pStopDateTimeFld)
	{
		const Cerner::Foundations::Calendar currentStopDateTime = pStopDateTimeFld->GetLastOeFldDtTmValue();

		if (currentStopDateTime.IsNull())
		{
			Cerner::Foundations::Calendar orderStartDateTime = orderObj.m_orderFldArr.GetFieldDtTmValFromMeanId(
						eDetailReqStartDtTm);

			if (!treatmentPeriodStopDateTime.IsNull() && treatmentPeriodStopDateTime.Compare(orderStartDateTime) >= 0)
			{
				IComponentPtr pIComponent = TransferDispatch(orderObj.GetPlanComponent());

				if (NULL != pIComponent)
				{
					pIComponent->PutLinkToPhase(TRUE);
				}

				pStopDateTimeFld->AddOeFieldDtTmValue(treatmentPeriodStopDateTime);
			}
			else
			{
				pStopDateTimeFld->AddOeFieldDtTmValue(orderStartDateTime);
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
/// \fn		void CDoTOrderDateTimeUpdater::UpdateDoTOrderNextDoseDateTime(PvOrderObj& orderObj) const
/// \brief		Prevent Next Dose Dt/Tm from occuring before start or after stop by setting it to equal start as needed.
///
/// \return		void
///
/// \param[out]	PvOrderObj& orderObj - The DoT order to update the next dose date/time of.
/// \owner		DB024248
/////////////////////////////////////////////////////////////////////////////
void CDoTOrderDateTimeUpdater::UpdateDoTOrderNextDoseDateTime(PvOrderObj& orderObj) const
{
	PvOrderFld* pNextDoseDateTimeFld = orderObj.m_orderFldArr.GetFieldFromMeanId(eDetailNextDoseDtTm);

	if (NULL != pNextDoseDateTimeFld)
	{
		Cerner::Foundations::Calendar nextDoseDateTime = pNextDoseDateTimeFld->GetLastOeFldDtTmValue();

		if (!nextDoseDateTime.IsNull())
		{
			Cerner::Foundations::Calendar startDateTime = orderObj.m_orderFldArr.GetFieldDtTmValFromMeanId(eDetailReqStartDtTm);

			if (!startDateTime.IsNull() && startDateTime > nextDoseDateTime)
			{
				pNextDoseDateTimeFld->AddOeFieldDtTmValue(startDateTime);
				return;
			}

			Cerner::Foundations::Calendar stopDateTime = orderObj.m_orderFldArr.GetFieldDtTmValFromMeanId(eDetailOrdStopDtTm);

			if (!stopDateTime.IsNull() && nextDoseDateTime > stopDateTime)
			{
				pNextDoseDateTimeFld->AddOeFieldDtTmValue(startDateTime);
				return;
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
/// \fn		void CDoTOrderDateTimeUpdater::UpdateDoTOrderValidDoseDateTime(PvOrderObj& orderObj) const
/// \brief		Prevent Valid Dose Dt/Tm from occuring before start by setting it to equal start as needed.
//				Also do the same for null because the order write service will write out a valid dose dt/tm
//				equal to original order dt/tm if the provided valid dose dt/tm is null.  This can cause
//				problems with orders set to start in the future because the start dt/tm will be later than
//				the original order dt/tm.  Since valid dose dt/tm will be set to match, it becomes invalid.
///
/// \return		void
///
/// \param[out]	PvOrderObj& orderObj - The DoT order to update the valid dose date/time of.
/// \owner		DB024248
/////////////////////////////////////////////////////////////////////////////
void CDoTOrderDateTimeUpdater::UpdateDoTOrderValidDoseDateTime(PvOrderObj& orderObj) const
{
	Cerner::Foundations::Calendar startDateTime = orderObj.m_orderFldArr.GetFieldDtTmValFromMeanId(eDetailReqStartDtTm);
	Cerner::Foundations::Calendar stopDateTime = orderObj.m_orderFldArr.GetFieldDtTmValFromMeanId(eDetailOrdStopDtTm);

	Cerner::Foundations::Calendar validDoseDateTime = orderObj.GetCurrentValidDoseDtTm();

	if (!startDateTime.IsNull())
	{
		if (validDoseDateTime.IsNull())
		{
			orderObj.SetValidDoseDtTm(startDateTime);
		}
		else if (startDateTime > validDoseDateTime)
		{
			orderObj.SetValidDoseDtTm(startDateTime);
		}
		else if (!stopDateTime.IsNull() && validDoseDateTime > stopDateTime)
		{
			orderObj.SetValidDoseDtTm(startDateTime);
		}
	}
}