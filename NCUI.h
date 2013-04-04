#ifndef NCUI_H
#define NCUI_H
#include "Interface.h"
#include <panel.h>

class shNCursesInterface : public shInterface
{
 public:
    shNCursesInterface ();
    ~shNCursesInterface ();

    int getChar ();
    int getSpecialChar (SpecialKey *sk);
    int getStr (char *buf, int len, const char *prompt,
                const char *dflt = NULL);
    const char *getKeyForCommand (Command cmd);
    shVector<const char *> *getKeysForCommand (Command cmd);

    void cursorOnXY (int x, int y, int curstype);
    void drawScreen ();
    void drawLog ();
    void refreshScreen ();

    /* Draws everything specified unquestionably. */
    void draw (int x, int y, shSpecialEffect e, shCreature *c,
               shObjectIlk *oi, shObject *o, shFeature *f, shSquare *s);
    /* Draws everything specified. What is not given is pulled from
       hero's memory of square contents. */
    void drawMem (int x, int y, shSpecialEffect e, shCreature *c,
                  shObjectIlk *o, shFeature *f, shSquare *s);
    void drawMem (int x, int y) {
        drawMem (x, y, kNone, NULL, NULL, NULL, NULL);
    };

    /* Diagnostic message output to window under log. */
    int diag (const char *format, ...);
    void pageLog ();
    void showVersion ();
    void doScreenShot (FILE *file);

    int getMaxLines ();
    int getMaxColumns ();
    void newTempWin ();
    void delTempWin ();
    void clearWin (Window win);
    void refreshWin (Window win);
    void setWinColor (Window win, shColor color);
    void winGoToYX (Window win, int y, int x);
    void winGetYX (Window win, int *y, int *x);
    void winPutchar (Window win, const char c);
    void winPrint (Window win, const char *fmt, ...);
    void winPrint (Window win, int y, int x, const char *fmt, ...);

    shMenu *newMenu (const char *prompt, int flags);
    void runMainLoop ();

 private:

    int mX0;
    int mY0;
    int mXMax;
    int mYMax;

    WINDOW *mWin[kMaxWin];
    PANEL *mPan[kMaxWin];

    Command mKey2Cmd[KEY_MAX];
    const char *mCommandHelp[kMaxCommand];

    //helper functions:
    Command keyToCommand (int key);
    void readKeybindings (const char *fname);
    void assign (int key, Command cmd);
    void resetKeybindings ();

    bool has_vi_keys ();
    bool has_vi_fire_keys ();
};

struct shMenuChoice;

class shNCursesMenu : public shMenu
{
 public:
    shNCursesMenu (const char *prompt, int flags);

 private:
    int mHeight;     /* Viewable rows. */
    int mWidth;      /* Viewable cols. */

    void accumulateResults ();
    void showHelp (WINDOW *win);
};
#endif
