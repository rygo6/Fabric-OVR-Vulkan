//
// Created by rygo6 on 11/28/2022.
//

#ifndef MOXAIC_MXC_INPUT_H
#define MOXAIC_MXC_INPUT_H

#include <GLFW/glfw3.h>

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
//    if (key == GLFW_KEY_W && action == GLFW_PRESS)
//        activate_airship();
}

static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{

}

void initInput(GLFWwindow *pWindow) {
    glfwSetKeyCallback(pWindow, key_callback);
    glfwSetCursorPosCallback(pWindow, cursor_position_callback);
}

#endif //MOXAIC_MXC_INPUT_H
