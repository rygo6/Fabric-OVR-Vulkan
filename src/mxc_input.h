//
// Created by rygo6 on 11/28/2022.
//

#ifndef MOXAIC_MXC_INPUT_H
#define MOXAIC_MXC_INPUT_H

#include "mxc_app.h"

#define mxcInputEventBufferCount 32
#define MXC_HELD 3

struct MxcKeyInputEvent {
    int key;
    int action;
};

struct MxcMousePosInputEvent {
    double xPos;
    double yPos;
    double xDelta;
    double yDelta;
};

struct MxcMouseButtonInputEvent {
    int button;
    int action;
};

typedef struct MxcInputEvent {
    enum {
        MXC_NO_INPUT,
        MXC_KEY_INPUT,
        MXC_MOUSE_POS_INPUT,
        MXC_MOUSE_BUTTON_INPUT,
    } type;
    union {
        struct MxcKeyInputEvent keyInput;
        struct MxcMousePosInputEvent mousePosInput;
        struct MxcMouseButtonInputEvent mouseButtonInput;
    };
} MxcInputEvent;

int mxcInputEventCount();

const MxcInputEvent* mxcGetKeyEvent(int index);

void mxcInitInput(MxcAppState *pAppState);

void mxcProcessInput();

#endif //MOXAIC_MXC_INPUT_H
