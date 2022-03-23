#pragma once

#include <RelatedFldsStatus.h>

// Forward Declarations
class PvOrderObj;

class CDurationDisplayHelper
{
public:
	static ERelatedFldsStatus SetDurationDisplay(PvOrderObj& orderObj);

private:
	static ERelatedFldsStatus SetDurationDisplayForNonOneTimeOrder(PvOrderObj& orderObj);
};