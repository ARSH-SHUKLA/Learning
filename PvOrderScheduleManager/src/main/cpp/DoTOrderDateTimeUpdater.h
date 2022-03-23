#pragma once

#include <srvcalendar.h>

// Forward Declarations
class PvOrderObj;

class CDoTOrderDateTimeUpdater
{
public:
	void UpdateDoTOrderDateTimes(PvOrderObj& orderObj) const;

private:
	void UpdateDoTOrderStopDateTime(PvOrderObj& orderObj, Cerner::Foundations::Calendar treatmentPeriodStopDateTime) const;
	void UpdateDoTOrderNextDoseDateTime(PvOrderObj& orderObj) const;
	void UpdateDoTOrderValidDoseDateTime(PvOrderObj& orderObj) const;
};