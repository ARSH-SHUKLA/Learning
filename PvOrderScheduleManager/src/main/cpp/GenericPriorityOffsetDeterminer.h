#pragma once

#include <afxstr.h>

// Forward Declarations
class PvOrderObj;

class CGenericPriorityOffsetDeterminer
{
public:
	static CString DetermineGenericPriorityOffset(PvOrderObj& orderObj);
};