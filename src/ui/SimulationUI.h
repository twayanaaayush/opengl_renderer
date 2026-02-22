#pragma once

#include <string>

struct SimulationParams;
class Application;

class SimulationUI
{
public:
	void Draw(SimulationParams& params, bool& simRunning, bool& wireframe,
	          bool& stepOnce, bool& resetRequested, float fps,
	          Application* app);

private:
	char m_ModelPath[512] = "";
	std::string m_StatusMsg;
	float m_StatusTimer = 0.0f;

	std::string OpenFileDialog();
};
