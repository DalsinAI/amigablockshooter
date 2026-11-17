/* The Amiga port's start: the stack and the version string.
 * MIT, like Block Shooter. Copyright (c) 2026 AmiCradle Team. */
unsigned long __stack = 64 * 1024;
static const char version[] __attribute__((used)) =
    "$VER: BlockShooter 1.0 (17.11.2026)";
