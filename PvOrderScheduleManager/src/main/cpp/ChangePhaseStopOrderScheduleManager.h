#pragma once

#include <srvcalendar.h>
#include <afxstr.h>

// Forward Declarations
struct IComponent;
class PvOrderObj;

class CChangePhaseStopOrderScheduleManager
{
public:
	bool UpdateOrderScheduleOnChangePhaseStop(IComponent& component, PvOrderObj& orderObj,
			const Cerner::Foundations::Calendar& phaseStopDateTime);
};