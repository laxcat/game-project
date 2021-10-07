#pragma once
#include <../../bx/tests/dbg.h>
#include <bx/file.h>


class Memory {
public:

    bx::DefaultAllocator allocator;
    bx::FileReader * reader;

    void init() {
        reader = BX_NEW(&allocator, bx::FileReader);

    }

    void shutdown() {
        BX_DELETE(&allocator, reader);        
    }

    bgfx::Memory const * loadMem(const char* _filePath)
    {
        if (bx::open(reader, _filePath) )
        {
            uint32_t size = (uint32_t)bx::getSize(reader);
            const bgfx::Memory* mem = bgfx::alloc(size+1);
            bx::read(reader, mem->data, size);
            bx::close(reader);
            mem->data[mem->size-1] = '\0';
            return mem;
        }

        DBG("Failed to load %s.", _filePath);
        return NULL;
    }

    bgfx::ShaderHandle loadShader(const char* _name)
    {
        char filePath[512];

        const char* shaderPath = "???";

        switch (bgfx::getRendererType())
        {
        case bgfx::RendererType::Noop:
        case bgfx::RendererType::Direct3D9:  shaderPath = "shaders/dx9/";   break;
        case bgfx::RendererType::Direct3D11:
        case bgfx::RendererType::Direct3D12: shaderPath = "shaders/dx11/";  break;
        case bgfx::RendererType::Agc:
        case bgfx::RendererType::Gnm:        shaderPath = "shaders/pssl/";  break;
        case bgfx::RendererType::Metal:      shaderPath = "shaders/metal/"; break;
        case bgfx::RendererType::Nvn:        shaderPath = "shaders/nvn/";   break;
        case bgfx::RendererType::OpenGL:     shaderPath = "shaders/glsl/";  break;
        case bgfx::RendererType::OpenGLES:   shaderPath = "shaders/essl/";  break;
        case bgfx::RendererType::Vulkan:     shaderPath = "shaders/spirv/"; break;
        case bgfx::RendererType::WebGPU:     shaderPath = "shaders/spirv/"; break;

        case bgfx::RendererType::Count:
            BX_ASSERT(false, "You should not be here!");
            break;
        }

        bx::strCopy(filePath, BX_COUNTOF(filePath), shaderPath);
        bx::strCat(filePath, BX_COUNTOF(filePath), _name);
        bx::strCat(filePath, BX_COUNTOF(filePath), ".bin");

        bgfx::ShaderHandle handle = bgfx::createShader(loadMem(filePath) );
        bgfx::setName(handle, _name);

        return handle;
    }


    bgfx::ProgramHandle loadProgram(const char* _vsName, const char* _fsName)
    {
        bgfx::ShaderHandle vsh = loadShader(_vsName);
        bgfx::ShaderHandle fsh = BGFX_INVALID_HANDLE;
        if (NULL != _fsName)
        {
            fsh = loadShader(_fsName);
        }

        return bgfx::createProgram(vsh, fsh, true /* destroy shaders when program is destroyed */);
    }

};
