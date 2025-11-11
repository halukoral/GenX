#include "graphics/GraphicsAPI.h"
#include "graphics/ShaderProgram.h"
#include <iostream>

namespace eng
{
    std::shared_ptr<ShaderProgram> GraphicsAPI::CreateShaderProgram(const std::string& vertexSource,
        const std::string& fragmentSource)
    {
        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        const char* vertexShaderCStr = vertexSource.c_str();
        glShaderSource(vertexShader, 1, &vertexShaderCStr, nullptr);
        glCompileShader(vertexShader);

        GLint success;
        glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            char infoLog[512];
            glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
            std::cerr << "ERROR:VERTEX_SHADER_COMPILATION_FAILED: " << infoLog << std::endl;
            return nullptr;
        }

        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        const char* fragmentShaderSourceCStr = fragmentSource.c_str();
        glShaderSource(fragmentShader, 1, &fragmentShaderSourceCStr, nullptr);
        glCompileShader(fragmentShader);

        glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            char infoLog[512];
            glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
            std::cerr << "ERROR:FRAGMENT_SHADER_COMPILATION_FAILED: " << infoLog << std::endl;
            return nullptr;
        }

        GLuint shaderProgramID = glCreateProgram();
        glAttachShader(shaderProgramID, vertexShader);
        glAttachShader(shaderProgramID, fragmentShader);
        glLinkProgram(shaderProgramID);

        glGetProgramiv(shaderProgramID, GL_LINK_STATUS, &success);
        if (!success)
        {
            char infoLog[512];
            glGetProgramInfoLog(shaderProgramID, 512, nullptr, infoLog);
            std::cerr << "ERROR:SHADER_PROGRAM_LINKING_FAILED: " << infoLog << std::endl;
            return nullptr;
        }

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        return std::make_shared<ShaderProgram>(shaderProgramID);
    }

    void GraphicsAPI::BindShaderProgram(ShaderProgram* shaderProgram)
    {
        shaderProgram->Bind();
    }
}