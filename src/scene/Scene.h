#pragma once

#include <iostream>
#include <vector>
#include "Geometry.h"
#include "Mesh.h"
#include "Camera.h"

class Scene
{
private:
	std::unique_ptr<Shader> m_GridShader;
	std::unique_ptr<Mesh> m_GridMesh;
	bool m_DrawGrid = true;

public:
	Scene(std::shared_ptr<Camera> cam);
	~Scene();

	void SetGridUniforms(float aspectRatio, float nearPlane, float farPlane);
	void DrawGrid();
	void SetDrawGrid();

private:
	void InitGrid();

public:
	std::shared_ptr<Camera> camera;
};
