#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

class Application;

class InputHandler
{
private:
	Application* m_App;
	double m_LastX = 0.0;
	double m_LastY = 0.0;
	int m_ButtonDown = -1;
	float m_RotXAngle = 0.0f;
	float m_RotYAngle = 0.0f;

public:
	InputHandler(GLFWwindow* window, Application* app);

	float GetRotXAngle() const { return m_RotXAngle; }
	float GetRotYAngle() const { return m_RotYAngle; }

	void ProcessInput(GLFWwindow* window, double deltaTime);

private:
	static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
	static void CursorCallback(GLFWwindow* window, double xpos, double ypos);
	static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
	static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
	static void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
};
