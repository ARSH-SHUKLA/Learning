#pragma once

#include <afxstr.h>

// Forward Declarations
class PvOrderObj;

class CSchedulingInstructionsHelper
{
public:
	static CString DetermineSchedulingInstructionsOffset(PvOrderObj& orderObj);
};