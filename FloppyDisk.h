#ifndef FLOPPY_DISK_H
#define FLOPPY_DISK_H

#define MAXTRACES 4
struct shWarpTrace
{
    shMapLevel *level;
    int time;
    int srcX, srcY;
    int destX, destY;
    char *who;
};
extern shWarpTrace traces[MAXTRACES];
extern int lastTrace;

#endif
