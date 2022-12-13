//
// Created by rygo6 on 11/28/2022.
//

#ifndef FABRIC_INPUT_H
#define FABRIC_INPUT_H

#include "fbr_app.h"

#define fbrInputEventBufferCount 32
#define FBR_HELD 3

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
        FBR_NO_INPUT,
        FBR_KEY_INPUT,
        FBR_MOUSE_POS_INPUT,
        FBR_MOUSE_BUTTON_INPUT,
    } type;
    union {
        struct FbrKeyInputEvent keyInput;
        struct FbrMousePosInputEvent mousePosInput;
        struct FbrMouseButtonInputEvent mouseButtonInput;
    };
} FbrInputEvent;

int fbrInputEventCount();

const FbrInputEvent *fbrGetKeyEvent(int index);

void fbrInitInput(FbrApp *pApp);

void fbrProcessInput();

#endif //FABRIC_INPUT_H
