// PvOrderScheduleManager.h : main header file for the PvOrderScheduleManager DLL
//

#pragma once

#include "resource.h"		// main symbols

// CPvOrderScheduleManagerApp
// See PvOrderScheduleManager.cpp for the implementation of this class
//

class CPvOrderScheduleManagerApp : public CWinApp
{
public:
	virtual BOOL InitInstance();
	DECLARE_MESSAGE_MAP()
};
