#pragma once

// Forward Declarations
struct IPhase;
struct IComponent;
class PvOrderObj;

class CLinkedToPhaseDurationUpdater
{
public:
	void UpdateLinkedToPhaseDurationOrders(const std::list<PvOrderObj*>& orders) const;

private:
	bool UpdateLinkOnCurrentlyLinkedOrder(IPhase& phase, IComponent& component, PvOrderObj& orderObj) const;

	void CallInpatientOrdersScheduleService(std::list<PvOrderObj*>& orders) const;
};