#pragma once

// Forward Declarations
class PvOrderObj;

class CPlannedComponentDeterminer
{
public:
	static bool IsPlannedOrderComponent(PvOrderObj& orderObj);
};