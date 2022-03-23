#pragma once

// Forward Declarations
class PvOrderObj;

class CPRNFlagUpdater
{
public:
	bool SetPRNFlagsForInpatientOrder(PvOrderObj& orderObj, const bool bSuppressWarning) const;

private:
	bool SetFlagsForPRNOrder(PvOrderObj& orderObj, const bool bSuppressWarning) const;
};