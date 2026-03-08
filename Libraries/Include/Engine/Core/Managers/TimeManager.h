#pragma once

class TimeManager
{
	DECLARE_SINGLE(TimeManager);

public:
	void Init();
	void Update();

	uint32 GetFps() const			{ return _fps; }
	float GetDeltaTime() const		{ return _deltaTime; }
	float GetTotalTime() const		{ return _totalTime; }

private:
	uint64	_frequency = 0;
	uint64	_prevCount = 0;
	float	_deltaTime = 0.f;

	uint32	_frameCount = 0;
	float	_frameTime = 0.f;
	uint32	_fps = 0;

	float _totalTime;
};