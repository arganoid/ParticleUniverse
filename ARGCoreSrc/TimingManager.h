#pragma once

#include <string>
#include <map>

class TimingManager
{
private:
	static TimingManager* s_instance;

public:

	struct AccumulatedData
	{
		unsigned count;
		double time;
	};

	TimingManager();
	~TimingManager();

	void Init();

	float GetDeltaTime();

	void RunTest();

	void BeginProfileSection(std::string const& name);
	double EndProfileSection(std::string const& name);

	static void BeginAccumulatedProfileSection(std::string const& name);
	static void EndAccumulatedProfileSection(std::string const& name);

	static std::map<std::string, AccumulatedData>& GetAccumulatedTimes() { return s_instance->m_accumulatedTimes; }

private:
	__int64 m_lastTimerReading;

	std::map<std::string, __int64> m_profileStartTimes;

	std::map<std::string, AccumulatedData> m_accumulatedTimes;
};
