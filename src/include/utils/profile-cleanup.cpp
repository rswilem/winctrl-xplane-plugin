#include "profile-cleanup.h"

#include "appstate.h"
#include "dataref.h"

void cleanupProfile(void *profile) {
    AppState::getInstance()->cancelTasksForOwner(profile);
    Dataref::getInstance()->unbindAll(profile);
}
