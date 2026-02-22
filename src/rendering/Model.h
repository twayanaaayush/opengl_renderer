#pragma once

#include <string>
#include <vector>
#include <memory>
#include <assimp/scene.h>
#include "Mesh.h"
#include "Transform.h"

class Model
{
public:
    Model(const std::string& path);
    void Draw(Shader& shader) const;

    Transform& GetTransform() { return m_Transform; }
    const std::string& GetPath() const { return m_Path; }

private:
    std::vector<std::shared_ptr<Mesh>> m_Meshes;
    std::string m_Directory;
    std::string m_Path;
    std::vector<TextureInfo> m_TexturesLoaded;
    Transform m_Transform;

    void LoadModel(const std::string& path);
    void ProcessNode(aiNode* node, const aiScene* scene);
    std::shared_ptr<Mesh> ProcessMesh(aiMesh* mesh, const aiScene* scene);
    std::vector<TextureInfo> LoadMaterialTextures(
        aiMaterial* mat, aiTextureType type, const std::string& typeName);
};
