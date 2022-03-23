#include "StdAfx.h"

#include "ChangePhaseStopOrderScheduleManager.h"

#include <pvorderobj.h>
#include <srvcalendar.h>
#include <CPS_ImportPVCareCoordCom.h>
#include <ActionDateTimeHelper.h>

bool CChangePhaseStopOrderScheduleManager::UpdateOrderScheduleOnChangePhaseStop(IComponent& component,
		PvOrderObj& orderObj, const Cerner::Foundations::Calendar& phaseStopDateTime)
{
	const Cerner::Foundations::Calendar& actionDateTime = CActionDateTimeHelper::GetActionDateTime(orderObj);

	PvOrderFld* pOrderStopDateTimeFld = orderObj.m_orderFldArr.GetFieldFromMeanId(eDetailOrdStopDtTm);

	if (NULL != pOrderStopDateTimeFld)
	{
		Cerner::Foundations::Calendar updatedStopDateTime = phaseStopDateTime;
		pOrderStopDateTimeFld->AddOeFieldDtTmValue(updatedStopDateTime);

		return true;
	}

	return false;
}