#include "mxc_input.h"
#include "mxc_log.h"

#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

mxcInputEvent inputEvents[mxcInputEventBufferCount];
int currentEventIndex;

struct { int key; bool value; } *heldKeyMap = NULL;
struct { int key; bool value; } *heldMouseButtonMap = NULL;

double lastXPos;
double lastYPos;

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_REPEAT)
        return;

    mxcLogDebugInfo2("mxcKeyEvent", %d, key, %d, action);
    inputEvents[currentEventIndex].type = MXC_KEY_INPUT;
    inputEvents[currentEventIndex].keyInput.key = key;
    inputEvents[currentEventIndex].keyInput.action = action;
    currentEventIndex++;

    if (action == GLFW_RELEASE) {
        hmdel(heldKeyMap, key);
    }
}

static void cursor_position_callback(GLFWwindow* window, double xPos, double yPos)
{
//    mxcLogDebugInfo2("mxcMouseEvent position", %f, xPos, %f, yPos);
    inputEvents[currentEventIndex].type = MXC_MOUSE_POS_INPUT;
    inputEvents[currentEventIndex].mousePosInput.xPos = xPos;
    inputEvents[currentEventIndex].mousePosInput.yPos = yPos;
    inputEvents[currentEventIndex].mousePosInput.xDelta = xPos - lastXPos;
    inputEvents[currentEventIndex].mousePosInput.yDelta = yPos - lastYPos;
    lastXPos = xPos;
    lastYPos = yPos;
    currentEventIndex++;
}

static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (action == GLFW_REPEAT)
        return;

    mxcLogDebugInfo2("mxcMouseEvent button", %d, button, %d, action);
    inputEvents[currentEventIndex].type = MXC_MOUSE_BUTTON_INPUT;
    inputEvents[currentEventIndex].mouseButtonInput.button = button;
    inputEvents[currentEventIndex].mouseButtonInput.action = action;
    currentEventIndex++;

    if (action == GLFW_RELEASE) {
        hmdel(heldMouseButtonMap, button);
    }
}

int mxcInputEventCount() {
    return currentEventIndex;
}

mxcInputEvent mxcGetKeyEvent(int index){
    return inputEvents[index];
}

void mxcInitInput(mxcAppState *pAppState) {
    glfwSetKeyCallback(pAppState->pWindow, key_callback);
    glfwSetCursorPosCallback(pAppState->pWindow, cursor_position_callback);
    glfwSetMouseButtonCallback(pAppState->pWindow, mouse_button_callback);

    // set initial mouse pos so first delta calc is good
    glfwGetCursorPos(pAppState->pWindow, &lastXPos, &lastYPos);

    glfwSetInputMode(pAppState->pWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    hmdefault(heldKeyMap, false);
    hmdefault(heldMouseButtonMap, false);
}

void mxcProcessInput(){
    for (int i = 0; i < currentEventIndex; ++i) {
        switch (inputEvents[i].type) {
            case MXC_NO_INPUT:
                break;
            case MXC_KEY_INPUT:{
                if (inputEvents[i].keyInput.action == GLFW_PRESS) {
                    hmput(heldKeyMap, inputEvents[i].keyInput.key, true);
                }
                break;
            }
            case MXC_MOUSE_POS_INPUT:
                break;
            case MXC_MOUSE_BUTTON_INPUT: {
                if (inputEvents[i].mouseButtonInput.action == GLFW_PRESS){
                    hmput(heldMouseButtonMap, inputEvents[i].mouseButtonInput.button, true);
                }
                break;
            }
        }
    }

    currentEventIndex = 0;
    glfwPollEvents();

    for (int i = 0; i < hmlen(heldKeyMap); ++i){
        inputEvents[currentEventIndex].type = MXC_KEY_INPUT;
        inputEvents[currentEventIndex].keyInput.key = heldKeyMap[i].key;
        inputEvents[currentEventIndex].keyInput.action = MXC_HELD;
        currentEventIndex++;
    }

    for (int i = 0; i < hmlen(heldMouseButtonMap); ++i){
        inputEvents[currentEventIndex].type = MXC_KEY_INPUT;
        inputEvents[currentEventIndex].mouseButtonInput.button = heldMouseButtonMap[i].key;
        inputEvents[currentEventIndex].mouseButtonInput.action = MXC_HELD;
        currentEventIndex++;
    }
}