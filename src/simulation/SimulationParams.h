#pragma once

struct SimulationParams
{
	float springConstant  = 200.0f;
	float dampingConstant = 1.0f;
	float gravityStrength = -0.98f;
	unsigned int moles    = 200;
	float integrationStep = 0.011f;
};
