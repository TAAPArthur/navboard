#define _POSIX_C_SOURCE 200112L
#include <stdlib.h>
#include <stdio.h>
#include "navboard.h"
#include "xutil.h"
#include "util.h"

void shiftKeys(KeyGroup*keyGroup, Key*key) {
    keyGroup->level = key->pressed;
}

static void pressAllModifiers(KeyGroup*keyGroup, int press) {
    for(int i = 0; i < keyGroup->numKeys; i++) {
        if(keyGroup->keys[i].pressed && isModifier(&keyGroup->keys[i])) {
            sendKeyEvent(press, keyGroup->keys[i].keyCode);
            if(!press && (keyGroup->keys[i].flags & LATCH)&& !(keyGroup->keys[i].flags & LOCK)) {
                keyGroup->keys[i].pressed = 0;
            }
        }
    }
}

void sendKeyReleaseWithModifiers(KeyGroup*keyGroup, Key* key) {
    sendKeyEvent(0, key->keyCode);
    pressAllModifiers(keyGroup, 0);
    if(key->flags & SHIFT)
        sendShiftKeyEvent(0);
}

void sendKeyPressWithModifiers(KeyGroup*keyGroup, Key* key) {
    if(key->flags & SHIFT)
        sendShiftKeyEvent(1);
    pressAllModifiers(keyGroup, 1);
    sendKeyEvent(1, key->keyCode);
}

void typeKey(KeyGroup*keyGroup, Key* key) {
    sendKeyPressWithModifiers(keyGroup, key);
    sendKeyReleaseWithModifiers(keyGroup, key);
}

void activateBoard(KeyGroup*keyGroup, Key*key) {
    activateBoardByName(key->arg.s ? key->arg.s: key->label);
}

void setKeyEnv(const Key* key) {
    char buffer[8];
    sprintf(buffer, "%d", key->value);
    setenv("KEY_VALUE", buffer, 1);
    sprintf(buffer, "%d", key->pressed);
    setenv("KEY_PRESSED", buffer, 1);
    if(key->label)
        setenv("KEY_LABEL", key->label, 1);
}
void spawnCmd(KeyGroup*keyGroup, Key*key) {
    setKeyEnv(key);
    spawn(key->arg.s);
}
void setPressedFromCmd(KeyGroup*keyGroup, Key*key) {
    setKeyEnv(key);
    key->pressed = !spawn(key->arg.s);
}

void readValueFromCmd(KeyGroup*keyGroup, Key*key){
    char buffer[8]={0};
    setKeyEnv(key);
    if(readCmd(key->arg.s, buffer, sizeof(buffer)) == 0)
        key->value = atoi(buffer);
}
