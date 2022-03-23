#pragma once

#include <srvcalendar.h>
#include <afxstr.h>
#include <CPS_ImportPVCareCoordCom.h>

// Forward Declarations
struct IComponent;
class PvOrderObj;

class CInitiateOrderScheduleManager
{
public:
	bool UpdateOrderScheduleOnInitiate(IComponent& component, PvOrderObj& orderObj,
									   const Cerner::Foundations::Calendar& phaseStartDateTime, const Cerner::Foundations::Calendar& timeZeroDateTime,
									   const Cerner::Foundations::Calendar& phaseStopDateTime);

private:
	bool UpdateOrderStopDateTime(IComponent& component, PvOrderObj& orderObj,
								 const Cerner::Foundations::Calendar& phaseStartDateTime, const Cerner::Foundations::Calendar& phaseStopDateTime);
	void UpdateStartDateTime(IComponent& component, PvOrderObj& orderObj,
							 const Cerner::Foundations::Calendar& phaseStartDateTime, const Cerner::Foundations::Calendar& actionDateTime,
							 const Cerner::Foundations::Calendar& timeZeroDateTime);
};