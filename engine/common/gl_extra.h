#pragma

// Some GL enum values. GLTF uses these values, but we don't use full OpenGL
// or ever include gl.h. So we're replicate some here for convenience.
enum GL {
    GLArrayBuffer = 0x8892,
    GLElementArrayBuffer = 0x8893,
};

inline char const * glString(int target) {
    return
    (target == GLArrayBuffer)           ? "GL_ARRAY_BUFFER" :
    (target == GLElementArrayBuffer)    ? "GL_ELEMENT_ARRAY_BUFFER" :
    "unknown";
}

