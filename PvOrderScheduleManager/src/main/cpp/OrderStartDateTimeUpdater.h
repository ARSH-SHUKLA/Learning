#pragma once

// Forward Declarations
class PvOrderObj;

namespace Cerner
{
	namespace Foundations
	{
		class Calendar;
	}
}

class COrderStartDateTimeUpdater
{
public:
	static void SetRequestedStartDateTime(PvOrderObj& orderObj, const Cerner::Foundations::Calendar& requestedStartDateTime,
										  const bool bClearReference);
	static void SetReferenceStartDateTime(PvOrderObj& orderObj, const Cerner::Foundations::Calendar& referenceStartDateTime,
										  const bool bClearRequested);
};