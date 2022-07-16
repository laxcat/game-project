#pragma once



class MemorySystem {
public:

    void init() {
    }

    void shutdown() {
    }

    bgfx::Memory const * loadMem(char const * filePath) {
        FILE * fp = fopen(filePath, "r");
        if (!fp) {
            fprintf(stderr, "Failed to load %s.", filePath);
            return NULL;
        }

        fseek(fp, 0L, SEEK_END);
        size_t size = ftell(fp);
        rewind(fp);

        bgfx::Memory const * mem = bgfx::alloc(size + 1);
        fread(mem->data, size, 1, fp);
        fclose(fp);
        mem->data[size] = '\0';

        return mem;
    }

    bgfx::ShaderHandle loadShader(char const * assetsPath, char const * _name) {
        char filePath[512];

        const char* shaderPath = "???";

        switch (bgfx::getRendererType()) {
            case bgfx::RendererType::Noop:
            case bgfx::RendererType::Direct3D9:  shaderPath = "dx9";   break;
            case bgfx::RendererType::Direct3D11:
            case bgfx::RendererType::Direct3D12: shaderPath = "dx11";  break;
            case bgfx::RendererType::Agc:
            case bgfx::RendererType::Gnm:        shaderPath = "pssl";  break;
            case bgfx::RendererType::Metal:      shaderPath = "metal"; break;
            case bgfx::RendererType::Nvn:        shaderPath = "nvn";   break;
            case bgfx::RendererType::OpenGL:     shaderPath = "glsl";  break;
            case bgfx::RendererType::OpenGLES:   shaderPath = "essl";  break;
            case bgfx::RendererType::Vulkan:     shaderPath = "spirv"; break;
            case bgfx::RendererType::WebGPU:     shaderPath = "spirv"; break;

            case bgfx::RendererType::Count:
                assert(false && "You should not be here!");
                break;
        }

        sprintf(filePath, "%sshaders/%s/%s.bin", assetsPath, shaderPath, _name);

        bgfx::ShaderHandle handle = bgfx::createShader(loadMem(filePath));
        bgfx::setName(handle, _name);

        return handle;
    }

    bgfx::ProgramHandle loadProgram(char const * assetsPath, char const * _vsName, char const * _fsName) {
        bgfx::ShaderHandle vsh = loadShader(assetsPath, _vsName);
        bgfx::ShaderHandle fsh = loadShader(assetsPath, _fsName);
        return bgfx::createProgram(vsh, fsh, true);
    }

    void destroyProgram(bgfx::ProgramHandle const & ph) {
        bgfx::destroy(ph);
    }

};
