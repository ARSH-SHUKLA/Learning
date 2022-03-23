#pragma once

// Forward Declarations
class PvOrderObj;

class CPharmacyPriorityHelper
{
public:
	static bool DoesMedOrderHavePharmacyPriorityOfStatOrNow(PvOrderObj& orderObj);
	static bool WasPharmacyPriorityChangedFromStatOrNowToRoutine(PvOrderObj& orderObj);
	static bool SetPharmacyPriorityToRoutineIfStatOrNow(PvOrderObj& orderObj);
};