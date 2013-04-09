#define NOAUDIO
#include "noteye.h"
#include "noteye-curses.h"
#undef NOAUDIO

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#else
#include <unistd.h> // write
#endif
#include <ctype.h>
#include <string.h>

#include "NEUI.h"
#include "Hero.h"
#ifdef __APPLE__
extern char** NXArgv;
#endif

shInterface *
startNotEye (int argc, char **argv)
{
    return new shNotEyeInterface (argc, argv);
}

bool mapOn;
bool altHeld;

int
altstate (lua_State *ls)
{
    checkArg (ls, 1, "altstate");
    int alt = noteye_int (ls, 1);
    altHeld = !!alt;
    /* FIXME: Part of "solution" to bug blanking PC glyph.
              For more information see shNotEyeInterface::cursorOnXY () */
    if (altHeld)
        I->cursorAt (80, 0);
    return 0;
}

int
ismapon (lua_State *ls)
{
    if (I) {
        retBool (ls, mapOn and !altHeld);
    } else {
        retBool (ls, false);
    }
    return 1;
}

int
pcloc (lua_State *ls)
{
    noteye_lua_newtable (ls);
    setfield (ls, "x", Hero.mX);
    setfield (ls, "y", Hero.mY);
    return 1;
}

int
mapcontents (lua_State *ls)
{
    checkArg (ls, 2, "map_contents");
    int x = noteye_int (ls, 1);
    int y = noteye_int (ls, 2);
    noteye_lua_newtable (ls);

    shNotEyeInterface *UI = (shNotEyeInterface *) (I);
    /* Tile contents not changed from last call.  Use cached image. */
    if (!UI->mCache[x][y].needs_update) {
        setfield (ls, "usecache", 1);
        return 1;
    }

    char buf[10];
    for (int i = 0; i < UI->mCache[x][y].tiledata->count (); ++i) {
        sprintf (buf, "%d", i+1);
        setfield (ls, buf, UI->mCache[x][y].tiledata->get (i));
    }
    UI->mCache[x][y].needs_update = false;
    return 1;
}

int
minimap (lua_State *ls)
{
    checkArg (ls, 2, "minimap");
    int x = noteye_int (ls, 1);
    int y = noteye_int (ls, 2);

    if (!Level) { /* Wait a minute, NotEye! */
        retInt (ls, kBlack);
        return 1;
    }

    shCreature *c = Level->getCreature (x, y);
    shFeature *f = Level->getMemory (x, y).mFeat ?
        Level->getKnownFeature (x, y) : NULL;
    bool seen = Level->getMemory (x, y).mTerr != kMaxTerrainType;
    if (c and c->isHero ()) {
        retInt (ls, kYellow);
    } else if (c and Hero.canSee (c) and c->isPet ()) {
        retInt (ls, kGreen);
    } else if (c and Hero.canSee (c)) {
        retInt (ls, kOrange);
    } else if (c and Hero.isAwareOf (c)) {
        retInt (ls, kRed);
    } else if (seen) {
        if (f and f->isStairs ()) {
            retInt (ls, kWhite);
        } else if (f and f->isTrap () and !f->mTrapUnknown) {
            retInt (ls, kMagenta);
        } else if (f and f->isDoor () and !f->mTrapUnknown) {
            retInt (ls, kNavy);
        } else if (f and !f->mTrapUnknown) {
            retInt (ls, kLime);
        } else if (Level->getMemory (x, y).mObj) {
            retInt (ls, kCyan);
        } else if (!Level->appearsToBeFloor (x, y)) {
            retInt (ls, kGray);
        } else {
            retInt (ls, kDarkGray);
        }
    } else {
        retInt (ls, kBlack);
    }

    return 1;
}

int
gamecore (lua_State *ls)
{
    checkArg (ls, 2, "game_core");
    InternalProcess *IP = byId<InternalProcess> (noteye_int (ls, 1), ls);
    noteye_setinternal (IP, ls, 2);
    mainLoop ();
    IP->exitcode = 0;
    IP->isActive = false;
    return 0;
}

shNotEyeInterface::shNotEyeInterface (int argc, char **argv)
{
    noteye_args (argc, argv);
    noteye_init ();

    noteye_globalfun ("map_contents", mapcontents);
    noteye_globalfun ("minimap", minimap);
    noteye_globalfun ("get_pc_coords", pcloc);
    noteye_globalfun ("is_map_on", ismapon);
    noteye_globalfun ("alt_state", altstate);
    noteye_globalfun ("game_core", gamecore);

    ColorMap[kBlack] = 0;
    ColorMap[kBlue] = 1;
    ColorMap[kGreen] = 2;
    ColorMap[kCyan] = 3;
    ColorMap[kRed] = 4;
    ColorMap[kMagenta] = 5;
    ColorMap[kBrown] = 6;
    ColorMap[kGray] = 7;
    ColorMap[kDarkGray] = 7;
    ColorMap[kNavy] = 9;
    ColorMap[kLime] = 10;
    ColorMap[kAqua] = 11;
    ColorMap[kOrange] = 12;
    ColorMap[kPink] = 13;
    ColorMap[kYellow] = 14;
    ColorMap[kWhite] = 15;

    mWin[kMain].x1 = 0;
    mWin[kMain].y1 = 0;
    mWin[kMain].x1 = 64;
    mWin[kMain].y1 = 20;
    mWin[kSide].x1 = 64;
    mWin[kSide].y1 = 0;
    mWin[kSide].x2 = 80;
    mWin[kSide].y2 = 20;
    mWin[kLog].x1 = 0;
    mWin[kLog].y1 = 20;
    mWin[kLog].x2 = 80;
    mWin[kLog].y2 = 25;
    mWin[kTemp].x1 = 0;
    mWin[kTemp].y1 = 0;
    mWin[kTemp].x2 = 80;
    mWin[kTemp].y2 = 25;

    for (int y = 0; y < MAPMAXROWS; ++y)
        for (int x = 0; x < MAPMAXCOLUMNS; ++x) {
            mCache[x][y].needs_update = true;
            mCache[x][y].tiledata = new shVector<int>;
        }

    for (int i = 0; i < kMaxWin; ++i) {
        mWin[i].cx = mWin[i].x1;
        mWin[i].cy = mWin[i].y1;
    }

    mColor = kGray;

    mLogSize = 5;
    mLogRow = 0;
    mHistoryIdx = 0;
    mHistoryWrapped = 0;
    mNoNewline = 0;
    mLogSCount = 0;
    mPause = 0;

    mapOn = true;
}

shNotEyeInterface::~shNotEyeInterface ()
{
    noteye_finishinternal (1);
    //noteye_uifinish ();
}

void
shNotEyeInterface::runMainLoop ()
{
#ifdef OSX
    setenv("NOTEYEDIR", DATADIR, 1);
    setenv("LUA_PATH", USERDIR, 1);
#endif
    noteye_run ("lua/prime.lua", true);
}

void
shNotEyeInterface::shCache::add (int x, int y, int spatial, int recolor)
{
    tiledata->add (x);
    tiledata->add (y);
    tiledata->add (spatial);
    tiledata->add (recolor);
}

static bool tempWinClear;

void
shNotEyeInterface::newTempWin ()
{
    tempWinClear = mapOn;
    mapOn = false;
    clearWin (kTemp);
}


void
shNotEyeInterface::delTempWin ()
{
    clearWin (kTemp);
    if (tempWinClear)  mapOn = true;
    drawScreen ();
}


void
shNotEyeInterface::clearWin (Window win)
{
    for (int y = mWin[win].y1; y < mWin[win].y2; ++y)
        for (int x = mWin[win].x1; x < mWin[win].x2; ++x)
            noteye_mvaddch (y, x, ' ');
}


void
shNotEyeInterface::refreshWin (Window win)
{}


int
shNotEyeInterface::getMaxLines ()
{
    return 25;
}


int
shNotEyeInterface::getMaxColumns ()
{
    return 80;
}


void
shNotEyeInterface::assign (int key, Command cmd)
{
    mKey2Cmd[key] = cmd;
}


void
shNotEyeInterface::resetKeybindings ()
{
    for (int i = 0; i < MAXKEYS; i++) {
        mKey2Cmd[i] = kNoCommand;
    }
}


static const struct shNameToKey KeyNames[] =
{
    {"space", ' '},
    {"tab", '\t'},
    {"backspace", 8},
    {"enter", 10},
    {"escape", 27},
    {"down arrow", D_DOWN},
    {"up arrow", D_UP},
    {"left arrow", D_LEFT},
    {"right arrow", D_RIGHT},
    {"keypad 1", D_END},
    {"keypad 2", D_DOWN},
    {"keypad 3", D_PGDN},
    {"keypad 4", D_LEFT},
    {"keypad 5", D_CTR},
    {"keypad 6", D_RIGHT},
    {"keypad 7", D_HOME},
    {"keypad 8", D_UP},
    {"keypad 9", D_PGUP},
    {"home", D_HOME},
    {"end", D_END},
    {"page up", D_PGUP},
    {"page down", D_PGDN},
    {"fn 1", KEY_F0 + 1},
    {"fn 2", KEY_F0 + 2},
    {"fn 3", KEY_F0 + 3},
    {"fn 4", KEY_F0 + 4},
    {"fn 5", KEY_F0 + 5},
    {"fn 6", KEY_F0 + 6},
    {"fn 7", KEY_F0 + 7},
    {"fn 8", KEY_F0 + 8},
    {"fn 9", KEY_F0 + 9},
    {"fn 10", KEY_F0 + 10},
    {"fn 11", KEY_F0 + 11},
    {"fn 12", KEY_F0 + 12}
};
const int n_keys = sizeof (KeyNames) / sizeof (shNameToKey);

void
shNotEyeInterface::readKeybindings (const char *fname)
{
    I->readKeybindings (fname, n_keys, KeyNames);
}


static const char *
keyCodeToString (int code)
{
    if (code >= KEY_F0 + 1 and code <= KEY_F0 + 12) {
        char *buf = GetBuf ();
        sprintf (buf, "F%d", code - KEY_F0 + 2);
        return buf;
    }

    for (int i = 0; i < n_keys; ++i)
        if (KeyNames[i].key == code)
            return KeyNames[i].name;

    if (code < ' ') {
        char *buf = GetBuf ();
        sprintf (buf, "ctrl %c", code + 64);
        return buf;
    } else {
        char *buf = GetBuf ();
        sprintf (buf, "%c", code);
        return buf;
    }
    return "(unkn)";
}


const char *
shNotEyeInterface::getKeyForCommand (Command cmd)
{   /* Find key corresponding to command. */
    int i;
    for (i = 0; i < MAXKEYS and cmd != mKey2Cmd[i]; ++i)
        ;
    if (i == MAXKEYS) /* Unassigned? This is bad. */
        return NULL;  /* Safe, will not lead to crash. */
    return keyCodeToString (i);
}


shVector<const char *> *
shNotEyeInterface::getKeysForCommand (Command cmd)
{
    shVector<const char *> *keys = new shVector<const char *> ();
    for (int i = 0; i < MAXKEYS; ++i) {
        if (cmd == mKey2Cmd[i])  keys->add (keyCodeToString (i));
    }
    return keys;
}


shInterface::Command
shNotEyeInterface::keyToCommand (int key)
{
    return mKey2Cmd[key];
}


bool
shNotEyeInterface::has_vi_keys ()
{
    return mKey2Cmd['l'] == kMoveE;
}

bool
shNotEyeInterface::has_vi_fire_keys ()
{
    return mKey2Cmd[0x1f & 'l'] == kFireE;
}


int
shNotEyeInterface::getChar ()
{
    return noteye_getch ();
}


int
shNotEyeInterface::getSpecialChar (SpecialKey *sk)
{
    int key = getChar ();
    switch (key) {
    case 27: *sk = kEscape; break;
    case 10: *sk = kEnter; break;
    case ' ': *sk = kSpace; break;
    case 8: *sk = kBackSpace; break;
    case D_HOME: *sk = kHome; break;
    case D_END:  *sk = kEnd; break;
    case D_PGUP: *sk = kPgUp; break;
    case D_PGDN: *sk = kPgDn; break;
    case D_UP: *sk = kUpArrow; break;
    case D_DOWN: *sk = kDownArrow; break;
    case D_LEFT: *sk = kLeftArrow; break;
    case D_RIGHT: *sk = kRightArrow; break;
    case D_CTR: *sk = kCenter; break;
    default: *sk = kNoSpecialKey; break;
    }
    return key;
}


int
shNotEyeInterface::getStr (char *buf, int len, const char *prompt,
                    const char *dflt /* = NULL */ ) /* Default suggestion. */
{
    char msg[80];
    int pos = 0;
    int savehistidx = mHistoryIdx;

    snprintf (msg, 80, "%s ", prompt);
    msg[79] = 0;
    p (msg);
    buf[0] = 0;

    if (dflt) {
        strncpy (buf, dflt, len);
        buf[len-1] = 0;
        winPrint (kLog, buf);
        pos = strlen (buf);
    }

    while (1) {
        drawLog ();
        int c = getChar ();
        if (isprint (c)) {
            if (pos >= len - 1) {
                continue;
            }
            buf[pos++] = c;
            winPutchar (kLog, c);
        } else if ('\n' == c or '\r' == c) {
            break;
        } else if (8 == c and pos) {
            pos--;
            int y, x;
            winGetYX (kLog, &y, &x);
            winPrint (kLog, y, x - 1, " ");
            winGoToYX (kLog, y, x - 1);
        } else if (27 == c) { /* Escape. */
            pos = 0;
            break;
        }/* else {
            debug.log ("getstr: unhandled char %d", c);
        }*/
    }

    buf[pos] = 0;
    snprintf (msg, 80, "%s %s", prompt, buf);
    msg[79] = 0;
    strcpy (&mLogHistory[savehistidx*80], msg);

    return pos;
}


int
shNotEyeInterface::diag (const char *format, ...)
{
    va_list ap;
    va_start (ap, format);
    char dbgbuf[100];
    vsnprintf (dbgbuf, 100, format, ap);
    debug.log ("%s", dbgbuf);
    va_end (ap);
    return 0;
}


void
shNotEyeInterface::cursorOnXY (int x, int y, int curstype)
{
    const int spCursor = spFlat + spMonst + spICeil + spIWallR + spIWallL;
    static bool curson = false; /* Is special cursor being displayed? */
    static int crx, cry; /* Last special cursor position on map. */
    if (curson) {
        /* Any cursors are assumed to be last on stack. */
        int cnt = mCache[crx][cry].tiledata->count ();
        for (int i = 1; i <= 4; ++i) /* This has to reverse shCache::add. */
            mCache[crx][cry].tiledata->removeByIndex (--cnt);
        mCache[crx][cry].needs_update = true;
    }
    if (curstype) {
        curson = true;
        mCache[x][y].add (curstype, kRowCursor, spCursor);
        mCache[x][y].needs_update = true;
        crx = x;  cry = y;
    } else {
        curson = false;
    }
    /* FIXME: There is a bug causing the spot under cursor to be blanked out.
       I cannot find it at the moment, so here is a workaround. -- MB */
    if (!altHeld)
        noteye_move (y, x);
    else
        noteye_move (0, 80);
}


void
shNotEyeInterface::winGoToYX (Window win, int y, int x)
{
    noteye_move (mWin[win].cy = mWin[win].y1 + y, mWin[win].cx = mWin[win].x1 + x);
}


void
shNotEyeInterface::winGetYX (Window win, int *y, int *x)
{
    *y = mWin[win].cy - mWin[win].y1;  *x = mWin[win].cx - mWin[win].x1;
}


void
shNotEyeInterface::pageLog ()
{
    mLogSCount = 0;
    mLogRow = 0;
    for (int i = 0; i < 5; ++i) {
        winGoToYX (kLog, i, 0);
        noteye_clrtoeol ();
    }
    winGoToYX (kLog, 0, 0);
    drawLog ();
}


void
shNotEyeInterface::winPutchar (Window win, const char c)
{
    if (c == '\n') {
        noteye_move (mWin[win].cy += 1, mWin[win].cx = 0);
    } else {
        noteye_mvaddch (mWin[win].cy, mWin[win].cx++, c);
    }
}


void
shNotEyeInterface::winPrint (Window win, const char *fmt, ...)
{
    va_list ap;
    char linebuf[80];
    va_start (ap, fmt);
    vsnprintf (linebuf, 80, fmt, ap);
    va_end (ap);
    noteye_mvaddstr (mWin[win].cy, mWin[win].cx, linebuf);
    mWin[win].cx += strlen (linebuf);
}


void
shNotEyeInterface::winPrint (Window win, int y, int x, const char *fmt, ...)
{
    va_list ap;
    char linebuf[80];
    va_start (ap, fmt);
    vsnprintf (linebuf, 80, fmt, ap);
    va_end (ap);
    noteye_mvaddstr (mWin[win].y1 + y, mWin[win].x1 + x, linebuf);
    mWin[win].cy = mWin[win].y1 + y;
    mWin[win].cx = mWin[win].x1 + x + strlen (linebuf);
}


void
shNotEyeInterface::setWinColor (Window win, shColor color)
{
    setTextAttr (ColorMap[color], 0);
}


void
shNotEyeInterface::doScreenShot (FILE *file)
{   /* TODO */   }

void
shNotEyeInterface::showVersion ()
{
    const int BX = 11, EX = 23;
    const int BY = 44, EY = 50;
    int hx = Hero.mX, hy = Hero.mY; /* Hack: center the screen on logo. */

    for (int y = BY; y <= EY; ++y) {
        for (int x = BX; x <= EX; ++x) {
            mCache[x - BX][y - BY].needs_update = true;
            mCache[x - BX][y - BY].add (x, y, spFlat + spIFloor);
        }
    }
    I->p ("PRIME "PRIME_VERSION" - Asbestos and webs");
    I->p ("Necklace of the Eye version "NOTEYEVERSION);
    Hero.mX = (EX - BX) * 3 / 4;
    Hero.mY = (EY - BY) * 3 / 4;
    noteye_getch ();
    pageLog ();
    for (int y = BY; y <= EY; ++y) {
        for (int x = BX; x <= EX; ++x) {
            mCache[x - BX][y - BY].needs_update = true;
            mCache[x - BX][y - BY].tiledata-> reset ();
        }
    }
    Hero.mX = hx;  Hero.mY = hy;
}


void
shNotEyeInterface::drawScreen ()
{
    Level->draw ();
    drawSideWin ();
}


void shNotEyeInterface::drawLog ()
{ //noteye_refresh ();
}

void shNotEyeInterface::refreshScreen ()
{
    noteye_refresh ();
}


const int Floor = spFlat + spFloor + spIFloor + spCeil;
const int spWall = spWallN + spWallE + spWallS + spWallW;
const int Wall = spFlat + spWall + spIWallL + spIWallR + spICeil;
const int spFeature = spFlat + spMonst + spIItem;
const int WayDown = spFlat + spFloor + spIFloor;

void
drawSpecialEffect (shSpecialEffect effect, shNotEyeInterface::shCache *cache)
{
    const int OPT = 14; /* Difference between laser and optic blast. */
    const int eff2tile[kLastEffect][2] =
    {   /* None: */ {5, kRowDefault},
        /* Laser: f-slash, horiz, vert, b-slash, hit. */
        {1, kRow4DirAtt}, {2, kRow4DirAtt}, {3, kRow4DirAtt}, {4, kRow4DirAtt},
        {5, kRow4DirAtt},
        /* Optic blast: f-slash, horiz, vert, b-slash, hit. */
        {1+OPT, kRow4DirAtt}, {2+OPT, kRow4DirAtt}, {3+OPT, kRow4DirAtt},
        {4+OPT, kRow4DirAtt}, {5+OPT, kRow4DirAtt},
        /* Bolt: f-slash, horiz, vert, b-slash, hit. */
        {10, kRow4DirAtt}, {11, kRow4DirAtt}, {12, kRow4DirAtt},
        {13, kRow4DirAtt}, {14, kRow4DirAtt},
        /* Railgun: N, NE, E, SE directions. */
        {3, kRow8DirAtt}, {8, kRow8DirAtt}, {6, kRow8DirAtt}, {1, kRow8DirAtt},
        /* Railgun: S, SW, W, NW directions. */
        {7, kRow8DirAtt}, {5, kRow8DirAtt}, {2, kRow8DirAtt}, {4, kRow8DirAtt},
        /* Rail hit. */
        {5, kRowDefault},
        /* Combi-stick: N, NE, E, SE directions. */
        {11, kRow8DirAtt},{9, kRow8DirAtt},{14, kRow8DirAtt},{16, kRow8DirAtt},
        /* Combi-stick: S, SW, W, NW directions. */
        {15, kRow8DirAtt},{13, kRow8DirAtt},{10, kRow8DirAtt},{12, kRow8DirAtt},
        /* Combi-stick: horz, vert, f-diag, b-diag directions. */
        {18, kRow8DirAtt},{19, kRow8DirAtt},{17, kRow8DirAtt},{20, kRow8DirAtt},
        /* Invis (should not be used), pea pellet, plasma glob, plasma hit. */
        {5, kRowDefault}, {1, kRow0DirAtt}, {2, kRow0DirAtt}, {3, kRowBoom},
        /* Explosion, frost, poison, radiation. */
        {1, kRowBoom}, {4, kRow0DirAtt}, {6, kRow0DirAtt}, {2, kRowBoom},
        /* Flashbang, incendiary, psi storm, disintegration. */
        {4, kRowBoom}, {5, kRow0DirAtt}, {5, kRowDefault}, {3, kRow0DirAtt},
        /* Shrapnel/frag, acid splash, water splash. */
        {5, kRowBoom}, {2, kRowSplash}, {5, kRowSplash},
        /* Binary, bugs, viruses, hydralisk spit. */
        {5, kRowDefault}, {5, kRowDefault}, {6, kRow0DirAtt}, {5, kRowDefault},
        /* Smart-Disc, radar blip, sensed life, sensed tremor. */
        {5, kRowMissile}, {4, kRowCursor}, {5, kRowCursor}, {6, kRowCursor}
    };

    const int Bolt = spFlat + spFree + spIItem;
    const int BoltHit = spFlat + spMonst + spIItem;
    const int Marker = spFlat + spIItem;

    /* Laser corners are weird special case.  Handle them now.  This has
       additional advantage that they are overwritten by any other effect
       that might be assigned to the tile. */
    if (effect & kLaserBeamCEffect) {
        if ((effect & kLaserBeamNWCEffect) == kLaserBeamNWCEffect)
            cache->add (6, kRow4DirAtt, Bolt);
        if ((effect & kLaserBeamNECEffect) == kLaserBeamNECEffect)
            cache->add (7, kRow4DirAtt, Bolt);
        if ((effect & kLaserBeamSWCEffect) == kLaserBeamSWCEffect)
            cache->add (8, kRow4DirAtt, Bolt);
        if ((effect & kLaserBeamSECEffect) == kLaserBeamSECEffect)
            cache->add (9, kRow4DirAtt, Bolt);
        effect = shSpecialEffect (effect & ~kLaserBeamCEffect);
    }

    switch (effect) {
    case kNone: /* Never drawn effects. */
    case kInvisibleEffect:
        return;
    /* Here go effects without tiles at the moment.  Don't draw anything. */
    case kPsionicStormEffect:
    case kRailEffect:
    case kBinaryEffect:
    case kBugsEffect:
    case kSpitEffect:
        return;
    /* Hit tiles in bolt row. */
    case kLaserBeamEffect:  case kBoltEffect:
        cache->add (eff2tile[effect][0], eff2tile[effect][1], BoltHit);
        break;
    default:
        break;
    }
    int col = eff2tile[effect][0];
    int row = eff2tile[effect][1];
    switch (row) {
    case kRow8DirAtt:  case kRow4DirAtt:
        cache->add (col, row, Bolt);
        break;
    case kRow0DirAtt:  case kRowSplash:  case kRowBoom:
        cache->add (col, row, BoltHit);
        break;
    case kRowCursor: case kRowMissile:
        cache->add (col, row, Marker);
        break;
    }
}


void
shNotEyeInterface::terr2tile (shTerrainType terr, shCache *cache, int odd)
{
    switch (terr) {
    case kStone:
        break;
    case kSewerWall1:
        cache->add (1, kRowSewers, Wall);
        break;
    case kSewerWall2:
        cache->add (2, kRowSewers, Wall);
        break;
    case kCavernWall1:
        cache->add (1, kRowGammaCaves, Wall);
        break;
    case kCavernWall2:
        cache->add (2, kRowGammaCaves, Wall);
        break;
    case kVWall: case kHWall: case kNTee: case kSTee: case kWTee: case kETee:
        cache->add (1, kRowSpaceBase, Wall);
        break;
    case kNWCorner: case kNECorner: case kSWCorner: case kSECorner:
        cache->add (2, kRowSpaceBase, spFlat + spWall + spIWallL + spIWallR);
        cache->add (1, kRowSpaceBase, spICeil);
        break;
    case kVirtualWall1:
        cache->add (1, kRowMainframe, Wall);
        break;
    case kVirtualWall2:
        cache->add (2, kRowMainframe, Wall);
        break;
    case kFloor:
    case kBrokenLightAbove:
        cache->add (5, kRowSpaceBase, Floor);
        break;
    case kCavernFloor: /* No ceiling as long as ASCII dot is used. */
        cache->add (5, kRowGammaCaves, spFlat + spFloor + spIFloor);
        break;
    case kSewerFloor:
        cache->add (5, kRowSewers, Floor);
        break;
    case kVirtualFloor:
        cache->add (5, kRowMainframe, Floor);
        break;
    case kSewage:
        cache->add (35 + odd, kRowSewers, Floor);
        break;
    case kGlassPanel:
        cache->add (5, kRowSpaceBase, Floor);
        cache->add (4, kRowSpaceBase, Wall);
        break;
    case kVoid:
    case kMaxTerrainType:
        break;
    }
}


void
shNotEyeInterface::feat2tile (shFeature *feat, shCache *cache)
{
    int featrow = kRowASCII;
    switch (Level->mMapType) {
    case shMapLevel::kBunkerRooms:
    case shMapLevel::kTown:
    case shMapLevel::kRabbit:
        featrow = kRowSpaceBase; break;
    case shMapLevel::kSewer:
    case shMapLevel::kSewerPlant:
        featrow = kRowSewers; break;
    case shMapLevel::kRadiationCave:
        featrow = kRowGammaCaves; break;
    case shMapLevel::kMainframe:
        featrow = kRowMainframe; break;
    case shMapLevel::kTest:
        featrow = kRowASCII; break;
    }
    switch (feat->mType) {
    case shFeature::kDoorHiddenVert:
    case shFeature::kDoorHiddenHoriz:
        cache->add (1, featrow, Wall);
        break;
    case shFeature::kDoorClosed:
    case shFeature::kDoorOpen:
    {   /* Doors are very detailed. */
        int spDoor = spFlat + spCenter;
        spDoor += (feat->isHorizontalDoor ()) ? spIWallL : spIWallR;
        /* Order is: closed, open, -, -, berserk closed, berserk open. */
        int x = 11; /* 11 is "plain" closed door. */
        if (feat->isOpenDoor ())  ++x;
        if (feat->isBerserkDoor () and !feat->mTrapUnknown)  x += 4;
        cache->add (x, featrow, spDoor);
        if (feat->isClosedDoor ()) { /* Show ceiling only if closed. */
            cache->add (1, featrow, spICeil);
            if (feat->isMagneticallySealed ()) {
                cache->add (14, featrow, spDoor); /* Indicate force field. */
            }
            if (feat->isInvertedDoor ()) { /* Troll face. */
                cache->add (17, featrow, spDoor);
            }
        }
        if (feat->isAutomaticDoor ()) { /* Pictures a detector above. */
            cache->add (13, featrow, spDoor);
        }
        if (feat->isClosedDoor () and feat->isLockDoor ()) {
            x = 19; /* No lock. */
            if (feat->isRetinaDoor ()) {
                x = 30;
            } else { /* Order is: open, closed, broken open, broken closed. */
                x = 26; /* 26 is "plain" open. */
                if (feat->isLockedDoor ())  ++x;
                if (feat->isLockBrokenDoor ())  x += 2;
            }
            cache->add (x, featrow, spDoor);
        }
        if (feat->isCodeLockDoor ()) { /* Color markings. */
            switch (feat->keyNeededForDoor ()->mAppearance.mGlyph.mColor) {
            case kNavy: x = 20; break;
            case kGreen: x = 21; break;
            case kRed: x = 22; break;
            case kOrange: x = 23; break;
            case kMagenta: x = 24; break;
            default: x = 25; break;
            }
            cache->add (x, featrow, spDoor);
        }
    }
    break;
    case shFeature::kStairsUp:
        cache->add (10, featrow, spFeature);
        break;
    case shFeature::kStairsDown:
        /* Most areas have way down in the floor. */
        if (featrow != kRowMainframe)
            cache->add (9, featrow, WayDown);
        else /* Sight obstructing way down. */
            cache->add (9, featrow, spFeature);
        break;
    case shFeature::kVat:
        cache->add (31, featrow, spFeature);
        break;
    case shFeature::kMovingHWall:
        cache->add (34, featrow, spFeature);
        break;
    case shFeature::kMachinery:
        cache->add (34, featrow, spFeature);
        break;
    case shFeature::kRadTrap:
        cache->add (2, kRowTrap, spFeature);
        break;
    case shFeature::kPit:
        cache->add (feat->mTrapMonUnknown == 2 ? 6 : 3, kRowTrap, spFeature);
        break;
    case shFeature::kAcidPit:
        cache->add (feat->mTrapMonUnknown == 2 ? 6 : 4, kRowTrap, spFeature);
        break;
    case shFeature::kHole:
        cache->add (5, kRowTrap, spFeature);
        break;
    case shFeature::kSewagePit:
    case shFeature::kTrapDoor:
    case shFeature::kWeb:
    case shFeature::kPortal:
    case shFeature::kPortableHole:
        cache->add (0, kRowTrap, spFeature);
        break;
    case shFeature::kComputerTerminal:
    case shFeature::kMaxFeatureType:
        break;
    }
}

const int spObj = spFlat + spItem + spIItem;
const int spCre = spFlat + spMonst + spIItem;

static void
putOverlay (shObject *obj, shNotEyeInterface::shCache *cache)
{
    shGlyph *g = NULL;
    if (obj->isA (kObjGenericComputer)) {
        /* Show virus or antivirus status in lower right corner. */
        if (obj->isFooproof () and obj->isFooproofKnown ()) {
            cache->add (22, kRowTag1, spObj);
        } else if (obj->isInfectedKnown ()) {
            cache->add (obj->isInfected () ? 23 : 37, kRowTag1, spObj);
        }
    } else if (obj->isA (kFloppyDisk)) {
        /* Add label to floppy disks. */
        if (obj->isKnown ()) {
            g = &obj->myIlk ()->mReal.mGlyph;
        } else if (obj->isAppearanceKnown ()) {
            g = &obj->myIlk ()->mAppearance.mGlyph;
        }
        if (g) cache->add (g->mTileX, g->mTileY, spObj);
        /* Show virus status. */
        if (obj->isInfectedKnown ()) {
            cache->add (obj->isInfected () ? 23 : 37, kRowTag1, spObj);
        }
    } else if (obj->myIlk ()->mReal.mGlyph.mTileX == kRowTag1 and
               obj->isKnown ())
    {   /* Indicate item type in upper right corner. */
        g = &obj->myIlk ()->mReal.mGlyph;
        cache->add (g->mTileX, g->mTileY, spObj);
    } else if (obj->isA (kObjGenericJumpsuit)) {
        /* Put question mark on unidentified jumpsuit types. */
        g = &obj->myIlk ()->mReal.mGlyph;
        cache->add (obj->isKnown () ? g->mTileX : 7, g->mTileY, spObj);
    } else if ((obj->isA (kArmor) or obj->isA (kWeapon)) and obj->isFooproof ())
    {
        cache->add (21, kRowTag1, spObj);
    }
}

/* Necklace of the Eye needs to be told both what tiles reside at given */
void         /* (x, y) coordinate pair and what ASCII glyph to display. */
shNotEyeInterface::draw (int x, int y, shSpecialEffect e, shCreature *c,
                   shObjectIlk *oi, shObject *o, shFeature *f, shSquare *s)
{
    shGlyph g = getASCII (x, y, e, c, oi, o, f, s);
    noteye_move (y, x);
    setTextAttr (g.mColor, g.mBkgd);
    noteye_addch (g.mSym);

    mCache[x][y].needs_update = true;
    mCache[x][y].tiledata->reset ();
    if (s) {
        terr2tile (s->mTerr, &mCache[x][y], (x + y) % 2);
    }
    if (f) {
        feat2tile (f, &mCache[x][y]);
    }
    if (o or oi) {
        shGlyph g = o ? o->getGlyph () : oi->mVague.mGlyph;
        mCache[x][y].add (g.mTileX, g.mTileY, spObj);
        if (o)  putOverlay (o, &mCache[x][y]);
    }
    if (c) {
        shGlyph g = c->mGlyph;
        mCache[x][y].add (g.mTileX, g.mTileY, spCre);
    }
    if (e)  drawSpecialEffect (e, &mCache[x][y]);
}


void
shNotEyeInterface::drawMem (int x, int y, shSpecialEffect e, shCreature *c,
                   shObjectIlk *o, shFeature *f, shSquare *s)
{
    shGlyph g = getMemASCII (x, y, e, c, o, f, s);
    noteye_move (y, x);
    setTextAttr (g.mColor, g.mBkgd);
    noteye_addch (g.mSym);

    shMapLevel::shMemory mem = Level->getMemory (x, y);
    mCache[x][y].needs_update = true;
    mCache[x][y].tiledata->reset ();

    if (s) {
        terr2tile (s->mTerr, &mCache[x][y], (x + y) % 2);
    } else if (mem.mTerr) {
        terr2tile (mem.mTerr, &mCache[x][y], (x + y) % 2);
    }
    if (f) {
        feat2tile (f, &mCache[x][y]);
    } else if (mem.mFeat != shFeature::kMaxFeatureType) {
        shFeature *real = Level->getFeature (x, y);
        shFeature fake;
        fake.mType = mem.mFeat;
        fake.mDoor.mFlags = mem.mDoor;
        if (real)
            fake.mTrapUnknown = real->mTrapUnknown;
        feat2tile (&fake, &mCache[x][y]);
    }
    if (o or mem.mObj) {
        shGlyph g = o ? o->mVague.mGlyph : AllIlks[mem.mObj].mVague.mGlyph;
        mCache[x][y].add (g.mTileX, g.mTileY, spObj);
    }
    if (c or mem.mMon) {
        shGlyph g = c ? c->mGlyph : MonIlks[mem.mMon].mGlyph;
        mCache[x][y].add (g.mTileX, g.mTileY, spCre);
    }
    if (e)  drawSpecialEffect (e, &mCache[x][y]);
}
