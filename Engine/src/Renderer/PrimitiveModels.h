#pragma once
#include "Renderer/Model.h"
#include <memory>

class PrimitiveModels 
{
public:
    // Create basic geometric shapes for physics visualization
    
    static std::shared_ptr<Model> CreateCube(float size = 1.0f) {
        auto model = std::make_shared<Model>();
        
        // Define vertices for a cube
        std::vector<Vertex3D> vertices = {
            // Front face
            {{-size, -size,  size}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
            {{ size, -size,  size}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
            {{ size,  size,  size}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
            {{-size,  size,  size}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}, {1.0f, 1.0f, 0.0f}},

            // Back face
            {{-size, -size, -size}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}, {1.0f, 0.0f, 1.0f}},
            {{ size, -size, -size}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}, {0.0f, 1.0f, 1.0f}},
            {{ size,  size, -size}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}, {0.5f, 0.5f, 0.5f}},
            {{-size,  size, -size}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}, {0.8f, 0.2f, 0.6f}},

            // Left face
            {{-size,  size,  size}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 0.5f, 0.0f}},
            {{-size,  size, -size}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}, {0.5f, 1.0f, 0.0f}},
            {{-size, -size, -size}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}, {0.0f, 0.5f, 1.0f}},
            {{-size, -size,  size}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.5f}},

            // Right face
            {{ size,  size,  size}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}, {0.8f, 0.3f, 0.1f}},
            {{ size,  size, -size}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}, {0.2f, 0.8f, 0.3f}},
            {{ size, -size, -size}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}, {0.6f, 0.2f, 0.8f}},
            {{ size, -size,  size}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.9f, 0.7f, 0.2f}},

            // Bottom face
            {{-size, -size, -size}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}, {0.4f, 0.6f, 0.9f}},
            {{ size, -size, -size}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}, {0.7f, 0.4f, 0.6f}},
            {{ size, -size,  size}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}, {0.3f, 0.9f, 0.5f}},
            {{-size, -size,  size}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}, {0.8f, 0.5f, 0.3f}},

            // Top face
            {{-size,  size, -size}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}, {0.6f, 0.8f, 0.2f}},
            {{ size,  size, -size}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}, {0.2f, 0.6f, 0.8f}},
            {{ size,  size,  size}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}, {0.9f, 0.3f, 0.7f}},
            {{-size,  size,  size}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}, {0.5f, 0.9f, 0.4f}}
        };

        // Define indices for cube faces
        std::vector<uint32_t> indices = {
            0,  1,  2,  2,  3,  0,   // front
            4,  5,  6,  6,  7,  4,   // back
            8,  9,  10, 10, 11, 8,   // left
            12, 13, 14, 14, 15, 12,  // right
            16, 17, 18, 18, 19, 16,  // bottom
            20, 21, 22, 22, 23, 20   // top
        };

        model->Meshes.emplace_back(vertices, indices);
        return model;
    }
    
    static std::shared_ptr<Model> CreateSphere(float radius = 1.0f, int segments = 16, int rings = 12) {
        auto model = std::make_shared<Model>();
        
        std::vector<Vertex3D> vertices;
        std::vector<uint32_t> indices;
        
        // Generate sphere vertices
        for (int ring = 0; ring <= rings; ++ring) {
            float phi = static_cast<float>(ring) * M_PI / static_cast<float>(rings);
            
            for (int segment = 0; segment <= segments; ++segment) {
                float theta = static_cast<float>(segment) * 2.0f * M_PI / static_cast<float>(segments);
                
                Vertex3D vertex;
                
                // Position
                vertex.Pos.x = radius * sin(phi) * cos(theta);
                vertex.Pos.y = radius * cos(phi);
                vertex.Pos.z = radius * sin(phi) * sin(theta);
                
                // Normal (normalized position for sphere)
                vertex.Normal = glm::normalize(vertex.Pos);
                
                // Texture coordinates
                vertex.TexCoord.x = static_cast<float>(segment) / static_cast<float>(segments);
                vertex.TexCoord.y = static_cast<float>(ring) / static_cast<float>(rings);
                
                // Color (can be customized)
                vertex.Color = glm::vec3(0.7f, 0.7f, 0.7f);
                
                vertices.push_back(vertex);
            }
        }
        
        // Generate sphere indices
        for (int ring = 0; ring < rings; ++ring) {
            for (int segment = 0; segment < segments; ++segment) {
                int current = ring * (segments + 1) + segment;
                int next = current + segments + 1;
                
                // Two triangles per quad
                indices.push_back(current);
                indices.push_back(next);
                indices.push_back(current + 1);
                
                indices.push_back(current + 1);
                indices.push_back(next);
                indices.push_back(next + 1);
            }
        }
        
        model->Meshes.emplace_back(vertices, indices);
        return model;
    }
    
    static std::shared_ptr<Model> CreatePlane(float size = 1.0f) {
        auto model = std::make_shared<Model>();
        
        std::vector<Vertex3D> vertices = {
            {{-size, 0.0f, -size}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}, {0.8f, 0.8f, 0.8f}},
            {{ size, 0.0f, -size}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}, {0.8f, 0.8f, 0.8f}},
            {{ size, 0.0f,  size}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}, {0.8f, 0.8f, 0.8f}},
            {{-size, 0.0f,  size}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}, {0.8f, 0.8f, 0.8f}}
        };
        
        std::vector<uint32_t> indices = {
            0, 1, 2, 2, 3, 0
        };
        
        model->Meshes.emplace_back(vertices, indices);
        return model;
    }
    
private:
    static constexpr float M_PI = 3.14159265358979323846f;
};