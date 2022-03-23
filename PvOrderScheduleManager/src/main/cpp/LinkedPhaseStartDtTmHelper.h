#pragma once

// Forward Declarations
struct IComponent;
class PvOrderObj;

class CLinkedPhaseStartDtTmHelper
{
public:
	
	static void SetLinkedPhaseStartDtTmToAnchorOrder(PvOrderObj& orderObj, IComponent& component);
	static bool IsComponentLinkedToValidPhase(IComponent& component, IPlanPtr pIPlan);
	static bool IsRequestedStartDateTimeManuallyEntered(IComponent& component, PvOrderObj& orderObj);
	static bool IsProtocolReviewBeingAccepted(IComponent& component);
};

