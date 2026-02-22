#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

class Application;

enum class DragTarget { None, Object, Collider };

class InputHandler
{
private:
	Application* m_App;
	double m_LastX = 0.0;
	double m_LastY = 0.0;
	int m_ButtonDown = -1;

	// Viewport drag-picking state
	DragTarget m_DragTarget = DragTarget::None;
	glm::vec3 m_DragPlaneNormal = glm::vec3(0.0f);
	glm::vec3 m_DragPlanePoint = glm::vec3(0.0f);
	glm::vec3 m_DragOffset = glm::vec3(0.0f);

public:
	InputHandler(GLFWwindow* window, Application* app);

	void ProcessInput(GLFWwindow* window, double deltaTime);

private:
	static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
	static void CursorCallback(GLFWwindow* window, double xpos, double ypos);
	static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
	static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
	static void FramebufferSizeCallback(GLFWwindow* window, int width, int height);

	static glm::vec3 ScreenToRay(GLFWwindow* window, double mouseX, double mouseY, Application* app);
	static bool RayAABB(const glm::vec3& origin, const glm::vec3& dir,
	                     const glm::vec3& bmin, const glm::vec3& bmax, float& tOut);
	static bool RayPlane(const glm::vec3& origin, const glm::vec3& dir,
	                      const glm::vec3& planePoint, const glm::vec3& planeNormal,
	                      glm::vec3& hitPoint);
};
