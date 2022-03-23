#pragma once

// Forward Declarations
class PvOrderObj;

class COrderDurationHelper
{
public:
	static bool IsDurationInDoses(const PvOrderObj& orderObj);
};