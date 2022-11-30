#include "mxc_input.h"
#include "mxc_log.h"

mxcInputEvent keyEvents[mxcInputEventBufferCount];
mxcInputEvent heldEvents[mxcInputEventBufferCount];
int currentEvent;

double lastXPos;
double lastYPos;

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    mxcLogDebugInfo2("mxcKeyEvent", %d, key, %d, action);
    keyEvents[currentEvent].type = MXC_KEY_INPUT;
    keyEvents[currentEvent].keyInput.key = key;
    keyEvents[currentEvent].keyInput.action = action;
    currentEvent++;
}

static void cursor_position_callback(GLFWwindow* window, double xPos, double yPos)
{
//    mxcLogDebugInfo2("mxcMouseEvent position", %f, xPos, %f, yPos);
    keyEvents[currentEvent].type = MXC_MOUSE_POS_INPUT;
    keyEvents[currentEvent].mousePosInput.xPos = xPos;
    keyEvents[currentEvent].mousePosInput.yPos = yPos;
    keyEvents[currentEvent].mousePosInput.xDelta = xPos - lastXPos;
    keyEvents[currentEvent].mousePosInput.yDelta = yPos - lastYPos;
    lastXPos = xPos;
    lastYPos = yPos;
    currentEvent++;
}

static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    mxcLogDebugInfo2("mxcMouseEvent button", %d, button, %d, action);
    keyEvents[currentEvent].type = MXC_MOUSE_BUTTON_INPUT;
    keyEvents[currentEvent].mouseButtonInput.button = button;
    keyEvents[currentEvent].mouseButtonInput.action = action;
    currentEvent++;
}

int mxcInputEventCount() {
    return currentEvent;
}

mxcInputEvent mxcGetKeyEvent(int index){
    return keyEvents[index];
}

void mxcInitInput(mxcAppState *pAppState) {
    glfwSetKeyCallback(pAppState->pWindow, key_callback);
    glfwSetCursorPosCallback(pAppState->pWindow, cursor_position_callback);
    glfwSetMouseButtonCallback(pAppState->pWindow, mouse_button_callback);

    // set initial mouse pos so first delta calc is good
    glfwGetCursorPos(pAppState->pWindow, &lastXPos, &lastYPos);

    glfwSetInputMode(pAppState->pWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void mxcProcessInput(){
    currentEvent = 0;
    glfwPollEvents();
}