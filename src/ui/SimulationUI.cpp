#include "SimulationUI.h"
#include "SimulationParams.h"
#include "Application.h"
#include "Softbody.h"
#include "Model.h"
#include <imgui.h>
#include <cstdio>
#include <cmath>
#include <algorithm>

std::string SimulationUI::OpenFileDialog()
{
#ifdef __APPLE__
	FILE* pipe = popen(
		"osascript -e 'set theFile to POSIX path of (choose file of type "
		"{\"obj\", \"fbx\", \"gltf\", \"glb\", \"dae\", \"3ds\", \"ply\", \"stl\"} "
		"with prompt \"Select a 3D Model\")' 2>/dev/null",
		"r");
	if (!pipe) return "";

	char buffer[512];
	std::string result;
	while (fgets(buffer, sizeof(buffer), pipe))
		result += buffer;
	pclose(pipe);

	// Trim trailing newline
	while (!result.empty() && (result.back() == '\n' || result.back() == '\r'))
		result.pop_back();

	return result;
#else
	return "";
#endif
}

void SimulationUI::Draw(SimulationParams& params, SimulationMetrics& metrics,
                        bool& simRunning, bool& wireframe,
                        bool& stepOnce, bool& resetRequested, float fps,
                        Application* app)
{
	ImGui::Begin("Simulation Controls");

	ImGui::Text("FPS: %.1f", fps);
	ImGui::Separator();

	// Physics parameters
	ImGui::Text("Physics");
	ImGui::SliderFloat("Particle Mass", &params.particleMass, 0.01f, 5.0f, "%.3f");
	ImGui::SliderFloat("Spring Constant", &params.springConstant, 50.0f, 500.0f);
	ImGui::SliderFloat("Damping", &params.dampingConstant, 0.1f, 10.0f);
	ImGui::SliderFloat("Gravity", &params.gravityStrength, -20.0f, 0.0f);

	float extForce[3] = { params.externalForce.x, params.externalForce.y, params.externalForce.z };
	if (ImGui::DragFloat3("External Force", extForce, 0.1f, -50.0f, 50.0f))
		params.externalForce = glm::vec3(extForce[0], extForce[1], extForce[2]);

	int moles = static_cast<int>(params.moles);
	if (ImGui::SliderInt("Moles", &moles, 10, 1000))
		params.moles = static_cast<unsigned int>(moles);

	ImGui::SliderFloat("Time Step", &params.integrationStep, 0.001f, 0.05f, "%.4f");

	const char* integrationMethods[] = { "Forward Euler", "Midpoint (2nd Order)", "Implicit Euler" };
	int currentMethod = static_cast<int>(params.integrationMethod);
	if (ImGui::Combo("Integration", &currentMethod, integrationMethods, 3))
		params.integrationMethod = static_cast<IntegrationMethod>(currentMethod);

	const char* volumeMethods[] = { "AABB", "Bounding Sphere", "Bounding Ellipsoid", "Exact (Divergence Thm)" };
	int currentVolMethod = static_cast<int>(params.volumeMethod);
	if (ImGui::Combo("Volume Method", &currentVolMethod, volumeMethods, 4))
		params.volumeMethod = static_cast<VolumeMethod>(currentVolMethod);

	// Volume comparison (Addition 1)
	if (app)
	{
		const auto& softbodies = app->GetSoftbodies();
		if (!softbodies.empty())
		{
			float vExact = softbodies[0]->GetVolumeExact();
			float vAABB = softbodies[0]->GetVolumeAABB();
			float vSphere = softbodies[0]->GetVolumeSphere();
			float vEllipsoid = softbodies[0]->GetVolumeEllipsoid();

			if (ImGui::CollapsingHeader("Volume Comparison"))
			{
				ImGui::Text("V_exact:     %.4f", vExact);
				ImGui::Text("V_AABB:      %.4f", vAABB);
				ImGui::Text("V_sphere:    %.4f", vSphere);
				ImGui::Text("V_ellipsoid: %.4f", vEllipsoid);

				if (vExact > 1e-6f)
				{
					float errAABB = ((vAABB - vExact) / vExact) * 100.0f;
					float errSphere = ((vSphere - vExact) / vExact) * 100.0f;
					float errEllipsoid = ((vEllipsoid - vExact) / vExact) * 100.0f;

					ImGui::Separator();
					ImGui::Text("Relative Error vs Exact:");
					ImGui::Text("  AABB:      %+.1f%%", errAABB);
					ImGui::Text("  Sphere:    %+.1f%%", errSphere);
					ImGui::Text("  Ellipsoid: %+.1f%%", errEllipsoid);
				}

				ImGui::Text("Active: %s", volumeMethods[currentVolMethod]);
				ImGui::Text("Pressure: %.2f", softbodies[0]->GetPressure());
			}
		}
	}

	ImGui::Separator();

	// Object Position
	ImGui::Text("Object Position");
	float objPos[3] = { params.objectPosition.x, params.objectPosition.y, params.objectPosition.z };
	if (ImGui::DragFloat3("Obj Pos", objPos, 0.1f, -20.0f, 20.0f))
		params.objectPosition = glm::vec3(objPos[0], objPos[1], objPos[2]);

	ImGui::Separator();

	// Rendering
	ImGui::Checkbox("Wireframe (E)", &wireframe);
	ImGui::Checkbox("Show Bounding Box", &params.showBoundingBox);

	ImGui::Separator();

	// Simulation controls
	if (simRunning)
	{
		if (ImGui::Button("Pause (Q)"))
			simRunning = false;
	}
	else
	{
		if (ImGui::Button("Play (Q)"))
			simRunning = true;
	}

	ImGui::SameLine();

	if (!simRunning)
	{
		if (ImGui::Button("Step (N)"))
			stepOnce = true;
	}
	else
	{
		ImGui::BeginDisabled();
		ImGui::Button("Step (N)");
		ImGui::EndDisabled();
	}

	ImGui::SameLine();

	if (ImGui::Button("Reset (Bksp)"))
		resetRequested = true;

	ImGui::Separator();

	// Status
	if (metrics.diverged)
		ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "DIVERGED");
	else if (simRunning)
		ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "RUNNING");
	else
		ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "PAUSED");

	ImGui::Separator();

	// Metrics
	if (ImGui::CollapsingHeader("Metrics", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Text("Frame: %d  |  Step: %.3f ms  |  Avg: %.3f ms",
			metrics.simFrameCount, metrics.physicsStepMs, metrics.avgPhysicsStepMs);
		ImGui::Text("Max Dist: %.2f", metrics.maxParticleDist);

		if (metrics.diverged)
			ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "Simulation unstable!");

		if (ImGui::Button("Snapshot (P)"))
		{
			if (app) app->CaptureSnapshot();
		}
		ImGui::SameLine();
		if (ImGui::Button("Reset Metrics"))
		{
			metrics.avgPhysicsStepMs = 0.0f;
			metrics.simFrameCount = 0;
			metrics.diverged = false;
		}
	}

	ImGui::Separator();

	// Collider Box (paper Section 3.2.4)
	ImGui::Text("Collider Box");
	ImGui::Checkbox("Enable Collider", &params.collider.enabled);
	ImGui::Checkbox("Show Collider Box", &params.showColliderBox);

	float colPos[3] = { params.colliderPosition.x, params.colliderPosition.y, params.colliderPosition.z };
	if (ImGui::DragFloat3("Collider Pos", colPos, 0.1f, -20.0f, 20.0f))
	{
		glm::vec3 newPos(colPos[0], colPos[1], colPos[2]);
		glm::vec3 delta = newPos - params.colliderPosition;
		params.collider.min += delta;
		params.collider.max += delta;
		params.colliderPosition = newPos;
	}

	float colMin[3] = { params.collider.min.x, params.collider.min.y, params.collider.min.z };
	float colMax[3] = { params.collider.max.x, params.collider.max.y, params.collider.max.z };

	if (ImGui::DragFloat3("Col Min", colMin, 0.1f, -40.0f, 20.0f))
		params.collider.min = glm::vec3(colMin[0], colMin[1], colMin[2]);

	if (ImGui::DragFloat3("Col Max", colMax, 0.1f, -20.0f, 40.0f))
		params.collider.max = glm::vec3(colMax[0], colMax[1], colMax[2]);

	ImGui::SliderFloat("Restitution", &params.collider.restitution, 0.0f, 1.0f);

	ImGui::Separator();

	// --- Model Loading ---
	ImGui::Text("3D Models");
	ImGui::Spacing();

	ImGui::InputText("##modelpath", m_ModelPath, sizeof(m_ModelPath));
	ImGui::SameLine();
	if (ImGui::Button("Browse..."))
	{
		std::string path = OpenFileDialog();
		if (!path.empty())
		{
			strncpy(m_ModelPath, path.c_str(), sizeof(m_ModelPath) - 1);
			m_ModelPath[sizeof(m_ModelPath) - 1] = '\0';
		}
	}

	if (ImGui::Button("Load Model"))
	{
		if (m_ModelPath[0] != '\0' && app)
		{
			app->LoadModel(std::string(m_ModelPath));
			m_StatusMsg = "Loaded: " + std::string(m_ModelPath);
			m_StatusTimer = 3.0f;
			m_ModelPath[0] = '\0';
		}
	}

	// Status message
	if (m_StatusTimer > 0.0f)
	{
		ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "%s", m_StatusMsg.c_str());
		m_StatusTimer -= ImGui::GetIO().DeltaTime;
	}

	// --- Loaded Models List ---
	if (app)
	{
		const auto& models = app->GetModels();
		int removeIndex = -1;

		for (int i = 0; i < static_cast<int>(models.size()); i++)
		{
			ImGui::PushID(i);
			ImGui::Separator();

			// Show filename from path
			const std::string& path = models[i]->GetPath();
			std::string filename = path;
			auto lastSlash = path.find_last_of('/');
			if (lastSlash != std::string::npos)
				filename = path.substr(lastSlash + 1);

			if (ImGui::CollapsingHeader(filename.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
			{
				Transform& t = models[i]->GetTransform();

				// Read current values from transform
				glm::vec3 translation = t.GetTranslation();
				glm::vec3 scale = t.GetScale();
				glm::vec3 rotation = t.GetRotation();

				float pos[3] = { translation.x, translation.y, translation.z };
				float scl[3] = { scale.x, scale.y, scale.z };
				float rot[3] = { rotation.x, rotation.y, rotation.z };

				if (ImGui::DragFloat3("Position", pos, 0.1f))
					t.SetTranslation(glm::vec3(pos[0], pos[1], pos[2]));

				if (ImGui::DragFloat3("Scale", scl, 0.01f, 0.001f, 100.0f))
					t.SetScale(glm::vec3(scl[0], scl[1], scl[2]));

				if (ImGui::DragFloat3("Rotation", rot, 1.0f, -360.0f, 360.0f))
					t.SetRotation(glm::vec3(rot[0], rot[1], rot[2]));

				if (ImGui::Button("Remove"))
					removeIndex = i;
			}

			ImGui::PopID();
		}

		if (removeIndex >= 0)
			app->RemoveModel(removeIndex);
	}

	ImGui::End();
}
