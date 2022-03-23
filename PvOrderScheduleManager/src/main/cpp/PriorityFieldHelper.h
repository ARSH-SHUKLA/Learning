#pragma once

// Forward Declarations
class PvOrderObj;

class CPriorityFieldHelper
{
public:
	static void DisablePriorityFieldsForIncompatibleOrderTypes(PvOrderObj& orderObj);
};