#include "SimulationUI.h"
#include "SimulationParams.h"
#include "Application.h"
#include "Model.h"
#include <imgui.h>
#include <cstdio>
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

void SimulationUI::Draw(SimulationParams& params, bool& simRunning, bool& wireframe,
                        bool& stepOnce, bool& resetRequested, float fps,
                        Application* app)
{
	ImGui::Begin("Simulation Controls");

	ImGui::Text("FPS: %.1f", fps);
	ImGui::Separator();

	// Physics parameters
	ImGui::Text("Physics");
	ImGui::SliderFloat("Spring Constant", &params.springConstant, 50.0f, 500.0f);
	ImGui::SliderFloat("Damping", &params.dampingConstant, 0.1f, 10.0f);
	ImGui::SliderFloat("Gravity", &params.gravityStrength, -5.0f, 0.0f);

	int moles = static_cast<int>(params.moles);
	if (ImGui::SliderInt("Moles", &moles, 10, 1000))
		params.moles = static_cast<unsigned int>(moles);

	ImGui::SliderFloat("Time Step", &params.integrationStep, 0.001f, 0.05f, "%.4f");

	ImGui::Separator();

	// Rendering
	ImGui::Checkbox("Wireframe (E)", &wireframe);

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
	if (simRunning)
		ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "RUNNING");
	else
		ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "PAUSED");

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
