#pragma once

struct GLFWwindow;

class ImGuiLayer
{
public:
	ImGuiLayer(GLFWwindow* window);
	~ImGuiLayer();

	void BeginFrame();
	void EndFrame();
};
