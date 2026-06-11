#ifndef PROFILE_CLEANUP_H
#define PROFILE_CLEANUP_H

// Automatic cleanup for profile destructor: cancels any tasks or monitors
// scheduled with the profile instance as owner. Called from base destructors
// so derived profile classes cannot forget. Both operations are idempotent,
// so explicit cleanup in derived destructors remains harmless.
void cleanupProfile(void *profile);

#endif
