#pragma once

#include <afxstr.h>

// Forward Declarations
class PvOrderObj;

class CCollectionPriorityOffsetDeterminer
{
public:
	static CString DetermineCollectionPriorityOffset(PvOrderObj& orderObj);
};