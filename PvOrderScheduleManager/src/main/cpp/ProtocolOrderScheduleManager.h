#pragma once

// Forward Declarations
struct IPhase;

class PvOrderProtocolObj;

class CProtocolOrderScheduleManager
{
public:
	CProtocolOrderScheduleManager(const HPATCON hPatCon);

public:
	void UpdateProtocolSchedule(IPhase& phase) const;
	void UpdateProtocolSchedule(PvOrderProtocolObj& protocolObj) const;

private:
	const HPATCON m_hPatCon;
};