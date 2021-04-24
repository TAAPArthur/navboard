#ifndef NAVBOARD_FUNCTIONS_H
#define NAVBOARD_FUNCTIONS_H
#include "navboard.h"

void shiftKeys(KeyGroup*keyGroup, Key*key);

/**
 * Sends a KeyEvent for all held modifiers and then for the key.
 * Eventually sendKeyReleaseWithModifiers should be called
 */
void sendKeyPressWithModifiers(KeyGroup*keyGroup, Key*key);
void sendKeyReleaseWithModifiers(KeyGroup*keyGroup, Key*key);

/**
 * Calls sendKeyPressWithModifiers and sendKeyReleaseWithModifiers
 */
void typeKey(KeyGroup*keyGroup, Key*key);
#endif
