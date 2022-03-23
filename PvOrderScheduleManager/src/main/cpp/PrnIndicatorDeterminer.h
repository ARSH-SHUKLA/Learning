// PrnIndicatorDeterminer.h- declarations of CPrnIndicatorDeterminer class.

#pragma once

// Forward Declarations
class PvOrderObj;

/////////////////////////////////////////////////////////////////////////////
/// \class	CPrnIndicatorDeterminer PrnIndicatorDeterminer.h "pvorderplanmanagercom/PrnIndicatorDeterminer.h"
/// \brief	Determiner class to get PRN indicator by using the PvOrderObj.
///
/// \owner	KM019227
/////////////////////////////////////////////////////////////////////////////
class CPrnIndicatorDeterminer
{
public:
	CPrnIndicatorDeterminer();
	virtual ~CPrnIndicatorDeterminer();

	bool IsPrnIndSetOnOrder(PvOrderObj& orderObj);
};