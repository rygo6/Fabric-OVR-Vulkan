//
// Created by rygo6 on 11/28/2022.
//

#ifndef MOXAIC_MXC_INPUT_H
#define MOXAIC_MXC_INPUT_H

#include "mxc_app.h"

#define mxcInputEventBufferCount 32

struct mxcKeyInputEvent {
    int key;
    int action;
};

struct mxcMousePosInputEvent {
    double xPos;
    double yPos;
    double xDelta;
    double yDelta;
};

struct mxcMouseButtonInputEvent {
    int button;
    int action;
};

typedef struct mxcInputEvent {
    enum {
        MXC_KEY_INPUT,
        MXC_MOUSE_POS_INPUT,
        MXC_MOUSE_BUTTON_INPUT,
    } type;
    union {
        struct mxcKeyInputEvent keyInput;
        struct mxcMousePosInputEvent mousePosInput;
        struct mxcMouseButtonInputEvent mouseButtonInput;
    };
} mxcInputEvent;

int mxcInputEventCount();

mxcInputEvent mxcGetKeyEvent(int index);

void mxcInitInput(mxcAppState *pAppState);

void mxcProcessInput();

#endif //MOXAIC_MXC_INPUT_H
