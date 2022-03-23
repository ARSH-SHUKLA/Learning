#include "StdAfx.h"

#include "SchedulingInstructionsHelper.h"

#include <pvorderobj.h>
#include <pvorderdataexp.h>

CString CSchedulingInstructionsHelper::DetermineSchedulingInstructionsOffset(PvOrderObj& orderObj)
{
	const double dScheduleInstructionsCd = orderObj.m_orderFldArr.GetFieldValFromMeanId(eDetailSchedInstructions);

	if (dScheduleInstructionsCd > 0.0)
	{
		SSchdlInstrs scheduleInstructions;

		if (GetSchdlInstr(dScheduleInstructionsCd, scheduleInstructions))
		{
			return scheduleInstructions.defaultStartDtTm;
		}
	}

	return "";
}