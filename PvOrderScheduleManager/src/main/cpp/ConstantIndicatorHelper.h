#pragma once

#include <RelatedFldsStatus.h>

// Forward Declarations
class PvOrderObj;

class CConstantIndicatorHelper
{
public:
	static ERelatedFldsStatus UpdateConstantIndicatorAcceptFlag(PvOrderObj& orderObj);
};