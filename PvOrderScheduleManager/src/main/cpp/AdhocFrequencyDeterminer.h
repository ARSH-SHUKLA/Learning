#pragma once

class CAdhocFrequencyDeterminer
{
public:
	CAdhocFrequencyDeterminer() = default;
	virtual ~CAdhocFrequencyDeterminer() = default;

	bool LoadAdhocIndFromFreqId(const double dFreqId) const;
};
