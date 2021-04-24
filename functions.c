#include "navboard.h"
#include "xutil.h"

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
}

void sendKeyPressWithModifiers(KeyGroup*keyGroup, Key* key) {
    pressAllModifiers(keyGroup, 1);
    sendKeyEvent(1, key->keyCode);
}

void typeKey(KeyGroup*keyGroup, Key* key) {
    sendKeyPressWithModifiers(keyGroup, key);
    sendKeyReleaseWithModifiers(keyGroup, key);
}

void activateBoard(KeyGroup*keyGroup, Key*key) {
    activateBoardByName(key->label);
}
