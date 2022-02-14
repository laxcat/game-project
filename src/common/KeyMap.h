struct KeyMap {
    int moveForward;
    int moveBackward;
    int moveLeft;
    int moveRight;
    int moveUp;
    int moveDown;

    void setDefaults() {
        moveForward     = GLFW_KEY_W;
        moveLeft        = GLFW_KEY_A;
        moveBackward    = GLFW_KEY_S;
        moveRight       = GLFW_KEY_D;
        moveUp          = GLFW_KEY_R;
        moveDown        = GLFW_KEY_F;
    }
};