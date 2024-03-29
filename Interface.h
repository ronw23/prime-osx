#ifndef INTERFACE_H
#define INTERFACE_H

#include <stdarg.h>
#include "Map.h"
#include "Menu.h"

extern bool BOFH;
extern void mainLoop (void);

struct shMenu;
struct shTextViewer;
struct shNameToKey
{
    const char *name;
    int key;
};
struct shInterface
{
    enum Window {
        kMain,
        kSide,
        kLog,
        kDiag,
        kTemp,
        kMaxWin
    };
    enum Command {
        kNoCommand,

        kMoveN,
        kMoveNE,
        kMoveE,
        kMoveSE,
        kMoveS,
        kMoveSW,
        kMoveW,
        kMoveNW,
        kMoveDown,
        kMoveUp,

        kGlideN,
        kGlideNE,
        kGlideE,
        kGlideSE,
        kGlideS,
        kGlideSW,
        kGlideW,
        kGlideNW,
        kGlideDown,
        kGlideUp,

        kFireN,
        kFireNE,
        kFireE,
        kFireSE,
        kFireS,
        kFireSW,
        kFireW,
        kFireNW,
        kFireDown,
        kFireUp,

        kAdjust,
        kClose,
        kDiscoveries,
        kDrop,
        kDropMany,
        kEditSkills,
        kExamine,
        kExecute,
        kFireWeapon,
        kGlide,
        kHelp,
        kHistory,
        kKick,
        kListInventory,
        kLook,
        kMainMenu,
        kMutantPower,
        kName,
        kOpen,
        kPay,
        kPickup,
        kPickUpAll,
        kQuaff,
        kRest,
        kSwap,
        kTakeOff,
        kToggleAutopickup,
        kThrow,
        kUse,
        kWear,
        kWield,
        kZapRayGun,

        kBOFHPower, /* Debug commands. */

        kMaxCommand
    };
    enum SpecialKey {
        kNoSpecialKey,
        kEscape,
        kSpace,
        kBackSpace,
        kEnter,
        kPgUp,
        kPgDn,
        kHome,
        kEnd,
        kInsert,
        kDelete,
        kUpArrow,
        kDownArrow,
        kLeftArrow,
        kRightArrow,
        kCenter
    };

    static shDirection
    moveToDirection (Command cmd)
    {
        if (cmd < kMoveN or cmd > kMoveUp) {
            return kNoDirection;
        } else {
            return (shDirection) (cmd - kMoveN);
        }
    }

    //virtual shInterface ();
    virtual ~shInterface () = 0;

    virtual int getChar () = 0;
    virtual int getSpecialChar (SpecialKey *sk) = 0;
    virtual int getStr (char *buf, int len, const char *prompt,
                const char *dflt = NULL) = 0;
    int getSquare (const char *prompt, int *x, int *y, int maxradius,
                   int instant = 0, int curstype = 0);
    Command getCommand ();
    virtual const char *getKeyForCommand (Command cmd) = 0;
    virtual shVector<const char *> *getKeysForCommand (Command cmd) = 0;
    shDirection getDirection (int *x = NULL, int *y = NULL,
                              int *z = NULL, int silent = 0);

    void cursorOnHero ();
    void cursorAt (int x, int y);
    void pauseXY (int x, int y, int ms = 0, int draw = 1);
    void drawSideWin ();
    virtual void drawScreen () = 0;
    virtual void drawLog () = 0;
    virtual void refreshScreen () = 0;

    /* Draws everything specified unquestionably. */
    virtual void draw (int x, int y, shSpecialEffect e, shCreature *c,
               shObjectIlk *oi, shObject *o, shFeature *f, shSquare *s) = 0;
    /* Draws everything specified.  What is not given in parameters
       is pulled from hero's memory of square contents. */
    virtual void drawMem (int x, int y, shSpecialEffect e, shCreature *c,
                  shObjectIlk *o, shFeature *f, shSquare *s) = 0;
    void drawMem (int x, int y) {
        drawMem (x, y, kNone, NULL, NULL, NULL, NULL);
    };


    int yn (const char *prompt, ...); /* Yes or no prompt. */
    int yN (const char *prompt, ...); /* Yes or no prompt; default to no. */
    int p (const char *format, ...);
    int vp (const char *format, va_list ap);
    /* Diagnostic message output to log file and diag window if present. */
    virtual int diag (const char *format, ...) = 0;
    /* Lines that can be printed until "more" appears. */
    int freeLogLines ();
    void nonlOnce ();
    void nevermind ();
    void pause ();
    void smallPause ();
    virtual void pageLog () = 0;
    void showHistory ();
    void showKills ();
    void showHelp ();
    void showHighScores ();
    virtual void showVersion () = 0;
    void editOptions ();
    void loadOptions ();
    void saveOptions ();
    void keymapChoice ();
    void doMorePrompt ();
    virtual void doScreenShot (FILE *file) = 0;
    void readKeybindings (const char *fname, int n, const shNameToKey table[]);

    void setColor (shColor c) { mColor = c; };

    virtual int getMaxLines () = 0;
    virtual int getMaxColumns () = 0;
    virtual void newTempWin () = 0;
    virtual void delTempWin () = 0;
    virtual void clearWin (Window win) = 0;
    virtual void refreshWin (Window win) = 0;
    virtual void setWinColor (Window win, shColor color) = 0;
    virtual void winGoToYX (Window win, int y, int x) = 0;
    virtual void winGetYX (Window win, int *y, int *x) = 0;
    virtual void winPutchar (Window win, const char c) = 0;
    virtual void winPrint (Window win, const char *fmt, ...) = 0;
    virtual void winPrint (Window win, int y, int x, const char *fmt, ...) = 0;

    virtual shMenu *newMenu (const char *prompt, int flags) = 0;
    virtual void runMainLoop () = 0; /* Calls the game loop. */

 protected:
    /* Functions that isolate whatever screen and keyboard handling
       each inheriting class employs. */

    virtual void cursorOnXY (int x, int y, int curstype) = 0;
    virtual Command keyToCommand (int key) = 0;
    virtual void readKeybindings (const char *fname) = 0;
    virtual void assign (int key, Command cmd) = 0;
    virtual void resetKeybindings () = 0;

    /* Implemented internal helper functions. */
    void delay (int ms);
    /* Computing ASCII symbol and color pair is
       useful for both NCurses and NotEye modes. */
    shGlyph getASCII (int x, int y, shSpecialEffect e, shCreature *c,
        shObjectIlk *oi, shObject *o, shFeature *f, shSquare *s);
    shGlyph getMemASCII (int x, int y, shSpecialEffect e, shCreature *c,
        shObjectIlk *oi, shFeature *f, shSquare *s);

    /* Variables needed for the stuff generic interface implements. */
#define HISTORY_ROWS 300
    char mLogHistory[HISTORY_ROWS * 80];
    int mHistoryIdx;
    int mHistoryWrapped;
    int mNoNewline;
    int mPause;

    int mLogRow;      //first empty row in log window
    int mLogSize;     //num rows in LogWin
    int mLogSCount;   //num rows output since last keyboard input

    shColor mColor;

 private:
    /* Used by showHelp: */
    virtual bool has_vi_keys () = 0;
    virtual bool has_vi_fire_keys () = 0;
    /* Used by drawSideWin: */
    void drawEquippyChar (shObject *obj);
    /* Used by pauseXY and doMorePrompt: */
    void moreBegin ();
    void moreEnd ();
    /* For yn and yN. */
    int yninternal (int dflt, const char *prompt, va_list va);
};
inline shInterface::~shInterface (void) {}


/* Value used for determining player's choice to show full inventory
   instead of a subset.  Examples: wielding items, throwing items. */
#define DELETE_FILTER_SIGNAL -1
struct shMenu
{
 public:
    enum MenuFlags {
        kNoPick = 0x1,        /* display only */
        kMultiPick = 0x2,
        kCountAllowed = 0x4,
        kFiltered = 0x8,
        /*UNUSED = 0x10,*/
        kCategorizeObjects = 0x20, /* hack */
        kNoHelp = 0x40, /* Do not show available keys. */
        kShowCount = 0x80, /* for skills */
        kSelectIsPlusOne = 0x100 /* for skills */
    };

    shMenu (const char *prompt, int flags);
    virtual ~shMenu ();

    void addPtrItem (char letter, const char *text, const void *value,
                     int count = 1, int selected = 0);
    void addIntItem (char letter, const char *text, const int value,
                     int count = 1, int selected = 0);
    void addText (const char *text);
    void addHeader (const char *text);
    void addPageBreak ();
    int getPtrResult (const void **value, int *count = NULL);
    int getIntResult (int *value, int *count = NULL);
    void getRandPtrResult (const void **value, int *count = NULL);
    void getRandIntResult (int *value, int *count = NULL);
    void dropResults ();
    void finish ();
    void attachHelp (const char *fname);

 protected:
    shVector <shMenuChoice *> mChoices;
    char mPrompt[80];
    int mFlags;
    int mResultIterator;
    shObjectType mObjTypeHack;
    const char *mHelpFileName;

    int mItemHeight; /* Number of choices visible. */
    int mOffset;     /* First choice. */
    int mLast;       /* Last choice. */
    int mNum;        /* Holds number of items to select. */
    int mDone;

    void select (int i1, int i2, int action, shObjectType t = kUninitialized);
    shMenuChoice *getResultChoice ();
    const char **prepareHelp (int *lines);
    bool interpretKey (int key, shInterface::Command cmd = shInterface::kNoCommand);
    const char *bottomLine (int curpos);

 private:
    virtual void accumulateResults () = 0;
};



struct shTextViewer
{
 public:
    shTextViewer (const char *fname);
    shTextViewer (char *lines, int num, int max);
    ~shTextViewer ();
    void show (bool bottom = false);
 private:
    void print (int winline, int fileline);
    char *mLines;
    int mNumLines;
    int mMaxWidth;
    bool mFree;
};

extern shInterface *I;
shInterface *startNCurses (void);
shInterface *startNotEye (int argc, char **argv);
#endif
