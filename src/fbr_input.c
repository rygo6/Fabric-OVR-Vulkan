#include "fbr_input.h"
#include "fbr_log.h"
#include "stb_ds.h"

FbrInputEvent inputEvents[FBR_INPUT_BUFFER_COUNT];
int currentEventIndex;

struct {
    int key;
    bool value;
} *heldKeyMap = NULL;
struct {
    int key;
    bool value;
} *heldMouseButtonMap = NULL;

double lastXPos;
double lastYPos;

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (action == GLFW_REPEAT)
        return;

    inputEvents[currentEventIndex].type = FBR_KEY_INPUT;
    inputEvents[currentEventIndex].keyInput.key = key;
    inputEvents[currentEventIndex].keyInput.action = action;
    currentEventIndex++;

    if (action == GLFW_RELEASE) {
        hmdel(heldKeyMap, key);
    }
}

static void cursor_position_callback(GLFWwindow *window, double xPos, double yPos) {
    inputEvents[currentEventIndex].type = FBR_MOUSE_POS_INPUT;
    inputEvents[currentEventIndex].mousePosInput.xPos = xPos;
    inputEvents[currentEventIndex].mousePosInput.yPos = yPos;
    inputEvents[currentEventIndex].mousePosInput.xDelta = xPos - lastXPos;
    inputEvents[currentEventIndex].mousePosInput.yDelta = yPos - lastYPos;
    lastXPos = xPos;
    lastYPos = yPos;
    currentEventIndex++;
}

static void mouse_button_callback(GLFWwindow *window, int button, int action, int mods) {
    if (action == GLFW_REPEAT)
        return;

    inputEvents[currentEventIndex].type = FBR_MOUSE_BUTTON_INPUT;
    inputEvents[currentEventIndex].mouseButtonInput.button = button;
    inputEvents[currentEventIndex].mouseButtonInput.action = action;
    currentEventIndex++;

    if (action == GLFW_RELEASE) {
        hmdel(heldMouseButtonMap, button);
    }
}

int fbrInputEventCount() {
    return currentEventIndex;
}

const FbrInputEvent *fbrGetKeyEvent(int index) {
    return &inputEvents[index];
}

void fbrInitInput(FbrApp *pApp) {
    glfwSetKeyCallback(pApp->pWindow, key_callback);
    glfwSetCursorPosCallback(pApp->pWindow, cursor_position_callback);
    glfwSetMouseButtonCallback(pApp->pWindow, mouse_button_callback);

    // set initial mouse pos so first delta calc is good
    glfwGetCursorPos(pApp->pWindow, &lastXPos, &lastYPos);

    glfwSetInputMode(pApp->pWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    hmdefault(heldKeyMap, false);
    hmdefault(heldMouseButtonMap, false);
}

void fbrProcessInput() {
    // Get key presses from last frame and store them in hashmap to add to event buffer as held events
    for (int i = 0; i < currentEventIndex; ++i) {
        switch (inputEvents[i].type) {
            case FBR_NO_INPUT:
                break;
            case FBR_KEY_INPUT: {
                if (inputEvents[i].keyInput.action == GLFW_PRESS) {
                    hmput(heldKeyMap, inputEvents[i].keyInput.key, true);
                }
                break;
            }
            case FBR_MOUSE_POS_INPUT:
                break;
            case FBR_MOUSE_BUTTON_INPUT: {
                if (inputEvents[i].mouseButtonInput.action == GLFW_PRESS) {
                    hmput(heldMouseButtonMap, inputEvents[i].mouseButtonInput.button, true);
                }
                break;
            }
        }
    }

    currentEventIndex = 0;
    glfwPollEvents();

    for (int i = 0; i < hmlen(heldKeyMap); ++i) {
        inputEvents[currentEventIndex].type = FBR_KEY_INPUT;
        inputEvents[currentEventIndex].keyInput.key = heldKeyMap[i].key;
        inputEvents[currentEventIndex].keyInput.action = FBR_HELD;
        currentEventIndex++;
    }

    for (int i = 0; i < hmlen(heldMouseButtonMap); ++i) {
        inputEvents[currentEventIndex].type = FBR_MOUSE_BUTTON_INPUT;
        inputEvents[currentEventIndex].mouseButtonInput.button = heldMouseButtonMap[i].key;
        inputEvents[currentEventIndex].mouseButtonInput.action = FBR_HELD;
        currentEventIndex++;
    }
}