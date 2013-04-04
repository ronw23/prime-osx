#ifndef NEUI_H
#define NEUI_H
#include "Interface.h"
#include "noteye-curses.h"

#define MAXKEYS DBASE+32

class shNotEyeInterface : public shInterface
{
 public:
    shNotEyeInterface (int argc, char **argv);
    ~shNotEyeInterface ();

    int getChar ();
    int getSpecialChar (SpecialKey *sk);
    int getStr (char *buf, int len, const char *prompt,
                const char *dflt = NULL);
    const char *getKeyForCommand (Command cmd);
    shVector<const char *> *getKeysForCommand (Command cmd);

    void cursorOnXY (int x, int y, int curstype);
    void draw (int x, int y, shSpecialEffect e, shCreature *c,
               shObjectIlk *oi, shObject *o, shFeature *f, shSquare *s);
    void drawMem (int x, int y, shSpecialEffect e, shCreature *c,
                  shObjectIlk *o, shFeature *f, shSquare *s);

    int diag (const char *format, ...);
    void drawScreen ();
    void refreshScreen ();
    void drawLog ();
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

    struct shCache {
        bool needs_update;
        shVector<int> *tiledata;
        void add (int x, int y, int spatial, int recolor = 0);
    } mCache[MAPMAXCOLUMNS][MAPMAXROWS];

 private:
    Command mKey2Cmd[MAXKEYS];
    struct {
        int x1, y1, x2, y2; /* Window absolute position on screen. */
        int cx, cy;         /* Cursor coordinates. */
    } mWin[kMaxWin];

    Command keyToCommand (int key);
    void readKeybindings (const char *fname);
    void assign (int key, Command cmd);
    void resetKeybindings ();

    bool has_vi_keys ();
    bool has_vi_fire_keys ();

    void terr2tile (shTerrainType terr, shCache *cache, int odd);
    void feat2tile (shFeature *feat, shCache *cache);
};

class shNotEyeMenu : public shMenu
{
 public:
    shNotEyeMenu (const char *prompt, int flags);
 private:
    int mHeight;     /* Viewable rows. */
    int mWidth;      /* Viewable cols. */

    void accumulateResults ();
    void showHelp ();
};
extern bool mapOn;
#endif
