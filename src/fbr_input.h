//
// Created by rygo6 on 11/28/2022.
//

#ifndef FABRIC_MXC_INPUT_H
#define FABRIC_MXC_INPUT_H

#include "fbr_app.h"

#define fbrInputEventBufferCount 32
#define MXC_HELD 3

struct FbrKeyInputEvent {
    int key;
    int action;
};

struct FbrMousePosInputEvent {
    double xPos;
    double yPos;
    double xDelta;
    double yDelta;
};

struct FbrMouseButtonInputEvent {
    int button;
    int action;
};

typedef struct FbrInputEvent {
    enum {
        MXC_NO_INPUT,
        MXC_KEY_INPUT,
        MXC_MOUSE_POS_INPUT,
        MXC_MOUSE_BUTTON_INPUT,
    } type;
    union {
        struct FbrKeyInputEvent keyInput;
        struct FbrMousePosInputEvent mousePosInput;
        struct FbrMouseButtonInputEvent mouseButtonInput;
    };
} FbrInputEvent;

int fbrInputEventCount();

const FbrInputEvent* fbrGetKeyEvent(int index);

void fbrInitInput(FbrAppState *pAppState);

void fbrProcessInput();

#endif //FABRIC_MXC_INPUT_H
