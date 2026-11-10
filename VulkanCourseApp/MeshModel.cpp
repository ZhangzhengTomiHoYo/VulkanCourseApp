
#include "MeshModel.h"
#include <utility>
#include <algorithm>

MeshModel::MeshModel() : modelMatrix(1.0f)
{
}

MeshModel::MeshModel(std::vector<Mesh> newMeshList)
    : meshList(std::move(newMeshList)), modelMatrix(1.0f)
{
}

size_t MeshModel::getMeshCount()
{
    return meshList.size();
}

Mesh* MeshModel::getMesh(size_t index)
{
    if (index >= meshList.size())
    {
        throw std::runtime_error("Failed to access invalid Mesh index");
    }

    return &meshList[index];
}

glm::mat4 MeshModel::getModelMatrix()
{
    return modelMatrix;
}

void MeshModel::setModel(glm::mat4 newModelMatrix)
{
    modelMatrix = newModelMatrix;
}

void MeshModel::destroyMeshModel()
{
    for (auto& mesh : meshList)
    {
        mesh.destroyBuffers();
    }
}

// 函数功能: 将 assimp库 读取bmp模型的纹理路径 放进 vector 中并返回
std::vector<std::string> MeshModel::LoadMaterials(const aiScene* scene)
{
    std::vector<std::string> textureList(scene->mNumMaterials);

    // 遍历每一个材质，读取它的纹理文件名（如果存在）
    for (size_t i = 0; i < scene->mNumMaterials; i++)
    {
        
        aiMaterial* material = scene->mMaterials[i]; // 获取当前材质
        textureList[i] = "";                         // 纹理初始化为空字符串，如果找到纹理会覆盖这个值            

        // 检查是否存在漫反射纹理（基础颜色纹理）
        if (material->GetTextureCount(aiTextureType_DIFFUSE))
        {
            // 获取纹理文件路径
            aiString path;
            if (material->GetTexture(aiTextureType_DIFFUSE, 0, &path) == AI_SUCCESS)
            {
                // 保留相对目录，统一 PMX 中混用的路径分隔符。
                std::string fileName = path.C_Str();
                std::replace(fileName.begin(), fileName.end(), '\\', '/');

                textureList[i] = fileName;
            }
        }
    }
    return textureList;
}

// 函数功能: 
std::vector<Mesh> MeshModel::LoadNode(VkPhysicalDevice newPhysicalDevice, VkDevice newDevice, VkQueue transferQueue, VkCommandPool transferCommandPool, aiNode* node, const aiScene* scene, std::vector<int> matToTex)
{
    std::vector<Mesh> meshList;

    // ① 加载当前节点挂载的所有Mesh
    for (size_t i = 0; i < node->mNumMeshes; i++)
    {
        // node->mMeshes[i] 只是索引！真正mesh存在scene->mMeshes[]数组
        meshList.push_back(
            LoadMesh(newPhysicalDevice, newDevice, transferQueue, transferCommandPool, scene->mMeshes[node->mMeshes[i]], scene, matToTex)
        );
    }

    // ② 递归处理每一个子节点
    for (size_t i = 0; i < node->mNumChildren; i++)
    {
        std::vector<Mesh> newList = LoadNode(newPhysicalDevice, newDevice, transferQueue, transferCommandPool, node->mChildren[i], scene, matToTex);
        // 把子节点返回的mesh追加到当前meshList
        meshList.insert(meshList.end(), newList.begin(), newList.end());
    }

    return meshList;
}

Mesh MeshModel::LoadMesh(VkPhysicalDevice newPhysicalDevice, VkDevice newDevice, VkQueue transferQueue, VkCommandPool transferCommandPool, aiMesh* mesh, const aiScene* scene, std::vector<int> matToTex)
{
    std::vector<Vertex> vertices;   // vbo
    std::vector<uint32_t> indices;  // ibo

    vertices.resize(mesh->mNumVertices);

    for (size_t i = 0; i < mesh->mNumVertices; i++)
    {
        vertices[i].pos = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };

        // Set tex coords (if they exist)
        if (mesh->mTextureCoords[0])
        {
            vertices[i].tex = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
        }
        else
        {
            vertices[i].tex = { 0.0f, 0.0f };
        }

        // Set colour (just use white for now)
        vertices[i].col = { 1.0f, 1.0f, 1.0f };
    }

    // Iterate over indices through faces and copy across
    for (size_t i = 0; i < mesh->mNumFaces; i++)
    {
        // Get a face
        aiFace face = mesh->mFaces[i];

        // Go through face's indices and add to list
        for (size_t j = 0; j < face.mNumIndices; j++)
        {
            indices.push_back(face.mIndices[j]);
        }
    }

    // Create new mesh with details and return it
    Mesh newMesh = Mesh(newPhysicalDevice, newDevice, transferQueue, transferCommandPool, &vertices, &indices, matToTex[mesh->mMaterialIndex]);

    return newMesh;
}

MeshModel::~MeshModel()
{
}
