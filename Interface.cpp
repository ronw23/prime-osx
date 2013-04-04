/***************************************************

   - shInterface, shMenu, shTextViewer
   - Part of shMapLevel associated with drawing

***************************************************/

#include <ctype.h>
#include <stddef.h>
#include "Interface.h"
#include "Hero.h"

#define LINELEN 80

void
shInterface::cursorAt (int x, int y)
{
    cursorOnXY (x, y, 0);
}

void
shInterface::cursorOnHero ()
{
    cursorOnXY (Hero.mX, Hero.mY, 0);
}

void
shInterface::pause ()
{
    doMorePrompt ();
}

void
shInterface::nevermind ()
{
    p ("Never mind.");
}

void
shInterface::smallPause ()
{
    mPause = 2;
}

void
shInterface::nonlOnce ()
{
    mNoNewline = 1;
}

int
shInterface::freeLogLines ()
{
    return mLogSize - mLogSCount;
}


shInterface::Command
shInterface::getCommand ()
{
    mLogSCount = 0;
    cursorOnHero ();
    int res = getChar ();
    pageLog ();
    return keyToCommand (res);
}


int
shInterface::p (const char *format, ...)
{
    int res;
    va_list ap;
    va_start (ap, format);
    res = vp (format, ap);
    va_end (ap);
    return res;
}


int
shInterface::vp (const char *format, va_list ap)
{
    const int buflen = LINELEN;
    char strbuf[buflen];
    /* Is --More-- needed? */
    if ((!mNoNewline and 6 == ++mLogSCount) or mPause) {
        doMorePrompt ();
        mLogSCount = 1;
    }
    /* Capitalization rules. */
    int res = vsnprintf (&strbuf[0], buflen, format, ap);
    if (' ' != strbuf[1] or '-' != strbuf[2]) {
        /* For "c - pea shooter" messages. */
        strbuf[0] = toupper (strbuf[0]);
    }
    if ('"' == strbuf[0]) {
        strbuf[1] = toupper (strbuf[1]);
    }
    strbuf[buflen-1] = 0;
    debug.log ("%s", strbuf);
    /* Save message to history. */
    if (mNoNewline) {
        int y, x;
        winGetYX (kLog, &y, &x);
        --mHistoryIdx;
        if (mHistoryIdx < 0) mHistoryIdx = HISTORY_ROWS - 1;
        strcpy (&mLogHistory[mHistoryIdx * LINELEN] + x, strbuf);
    } else {
        strcpy (&mLogHistory[mHistoryIdx * LINELEN], strbuf);
    }
    mHistoryIdx = (mHistoryIdx + 1) % HISTORY_ROWS;
    if (0 == mHistoryIdx and !mNoNewline) {
        mHistoryWrapped = 1;
    }
    /* Actual printing. */
    if (mLogRow and !mNoNewline) {
        winPutchar (kLog, '\n');
    }
    if (!mNoNewline and mLogRow < mLogSize) {
        mLogRow++;
    }
    setWinColor (kLog, mColor);
    winPrint (kLog, strbuf);
    setWinColor (kLog, kGray);
    mNoNewline = 0;
    drawLog ();
    return res;
}


int
shInterface::yninternal (int dflt, const char *msg, va_list ap)
{
    vp (msg, ap);
    do {
        int res = getChar ();
        if ('y' == res or 'Y' == res) {
            return 1;
        } else if ('n' == res or 'N' == res) {
            return 0;
        }
    } while (dflt == -1);/* Wait indefinitely for decision if no default set. */
    return dflt;
}

int
shInterface::yn (const char *prompt, ...)
{
    char msg[LINELEN];

    snprintf (msg, LINELEN, "%s [y/n]", prompt);
    msg[LINELEN - 1] = 0;

    va_list ap;
    va_start (ap, prompt);
    int res = yninternal (-1, msg, ap);
    va_end (ap);
    return res;
}


int
shInterface::yN (const char *prompt, ...)
{
    char msg[LINELEN];

    snprintf (msg, LINELEN, "%s [y/n]", prompt);
    msg[LINELEN - 1] = 0;

    va_list ap;
    va_start (ap, prompt);
    int res = yninternal (0 /* Default answer: no. */, msg, ap);
    va_end (ap);
    return res;
}


void
shInterface::pauseXY (int x, int y, int ms /* = 0 */, int draw /* = 1 */)
{
    if (ms) {
        if (draw) drawScreen ();
    } else {
        moreBegin ();
    }
    cursorOnXY (x, y, 0);
    if (ms) { /* Wait specified time. */
        refreshScreen ();
        delay (ms);
    } else {  /* Wait until keypress. (space/enter/esc) */
        int ch;
        do {
            ch = getChar (); /* TODO: use getCharSpecial */
        } while (' ' != ch and 13 != ch and 10 != ch and 27 != ch);
    }
    Level->drawSq (x, y);
    if (!ms) {
        moreEnd ();
        mLogSCount = 0;
    }
}


void
shInterface::moreBegin ()
{
    static const char     more[] = "  --More--";
    winPrint (kLog, more);
    refreshWin (kLog); /* Show cursor after more.  Necessary for NCurses. */
    drawLog ();
}


void
shInterface::moreEnd ()
{
    static const char antimore[] = "          ";
    if (Flags.mFadeLog) {
        int x, y;
        winGetYX (kLog, &y, &x);
        winPrint (kLog, y, x - strlen (antimore), antimore);
        winGoToYX (kLog, y, x - strlen (antimore));
    }
    pageLog ();
}


void
shInterface::doMorePrompt ()
{
    moreBegin ();
    int ch;
    do {
        ch = getChar (); /* TODO: use getCharSpecial */
    } while (' ' != ch and 13 != ch and 10 != ch and 27 != ch);

    if (2 != mPause) {
        mLogSCount = 0;
        pageLog ();
    }
    mPause = 0;
    moreEnd ();
    if (Flags.mFadeLog) drawLog ();
}


shDirection
shInterface::getDirection (int *x, int *y, int *z, int silent)
{
    if (!silent) {
        pageLog ();
        p ("In what direction?");
    }
    drawLog ();
    cursorOnHero ();
    shDirection dir = kNoDirection;
    while (dir == kNoDirection) {
        SpecialKey sp;
        int res = getSpecialChar (&sp);
        Command cmd = keyToCommand (res);
        if (z) *z = 0;
        if (sp == kEscape or sp == kEnter or sp == kSpace) {
            pageLog ();
            nevermind ();
            return kNoDirection;
        }
        switch (cmd) {
        case kMoveSW:
            if (x) --*x; if (y) ++*y; pageLog (); dir = kSouthWest; break;
        case kMoveS: if (y) ++*y; pageLog (); dir = kSouth; break;
        case kMoveSE:
            if (x) ++*x; if (y) ++*y; pageLog (); dir = kSouthEast; break;
        case kMoveW: if (x) --*x; pageLog (); dir = kWest; break;
        case kRest: case kGlide:
            pageLog (); dir = kOrigin; break;
        case kMoveE: if (x) ++*x; pageLog (); dir = kEast; break;
        case kMoveNW:
            if (x) --*x; if (y) --*y; pageLog (); dir = kNorthWest; break;
        case kMoveN: if (y) --*y; pageLog (); dir = kNorth; break;
        case kMoveNE:
            if (x) ++*x; if (y) --*y; pageLog (); dir = kNorthEast; break;
        case kMoveUp: if (z) --*z; pageLog (); dir = kUp; break;
        case kMoveDown: if (z) ++*z; pageLog (); dir = kDown; break;
        default:
            pageLog ();
            p ("NW: %s N: %s NE: %s       up: %s",
                getKeyForCommand (kMoveNW), getKeyForCommand (kMoveN),
                getKeyForCommand (kMoveNE), getKeyForCommand (kMoveUp));
            p (" W: %s       E: %s     self: %s",
                getKeyForCommand (kMoveW), getKeyForCommand (kMoveE),
                getKeyForCommand (kRest));
            p ("SW: %s S: %s SE: %s     down: %s",
                getKeyForCommand (kMoveSW), getKeyForCommand (kMoveS),
                getKeyForCommand (kMoveSE), getKeyForCommand (kMoveDown));
            p ("Please select direction from above possibilities.");
            p ("Hit space, enter or esacape to cancel.");
            cursorOnHero ();
            break;
        }
    }
    if (Hero.is (kStunned)) { /* Be very helpful and ignore user choice. */
        if (dir == kUp or dir == kDown) {
            dir = RNG (2) ? kUp : kDown;
        } else {
            dir = shDirection (RNG (8));
        }
    } else if (Hero.is (kConfused) and !RNG (3)) {
        if (dir == kUp or dir == kDown) {
            dir = RNG (2) ? kUp : kDown;
        } else { /* Tilt to left or right. */
            if (RNG (2)) {
                dir = shDirection ((dir + 1) % 8);
            } else {
                dir = shDirection ((dir + 7) % 8);
            }
        }
    }
    return dir;
}

/*maxradius is specified in squares
  returns: 1 on success, 0 on abort
*/
int
shInterface::getSquare (const char *prompt, int *x, int *y, int maxradius,
                        int instant, int curstype)
{   /* This is supposed to be fully generic routine (shInterface)
       but here is incorporated knowledge of NotEye UI.  *sigh*  -- MB */
    if (!curstype) { /* Use default cursor if not set. */
        if (Hero.mHelmet and Hero.mHelmet->isA (kObjBioMask))
            curstype = 2; /* Predator targeting triangle. */
        else
            curstype = 1; /* Ordinary cursor. */
    }
    if (maxradius <= 0) maxradius = 99;
    if (prompt) p (prompt);

    if (-1 == *x or -1 == *y) {
        *x = Hero.mX;
        *y = Hero.mY;
    }
    int tx, ty;
    int step = 1;

    while (1) {
        cursorOnXY (*x, *y, curstype);
        tx = *x; ty = *y;
        SpecialKey sp;
        int res = getSpecialChar (&sp);
        Command cmd = keyToCommand (res);
        if (sp == kSpace or sp == kEnter) {
            /* Clearing special cursor on exit, if any. */
            cursorOnXY (*x, *y, 0);
            return 1 and !instant;
        }
        if (sp == kEscape) {
            cursorOnXY (*x, *y, 0);
            return 0;
        }
        switch (cmd) {
        case kMoveSW: tx = *x - step; ty = *y + step; break;
        case kMoveS:                  ty = *y + step; break;
        case kMoveSE: tx = *x + step; ty = *y + step; break;
        case kMoveW:  tx = *x - step;                 break;
        case kMoveE:  tx = *x + step;                 break;
        case kMoveNW: tx = *x - step; ty = *y - step; break;
        case kMoveN:                  ty = *y - step; break;
        case kMoveNE: tx = *x + step; ty = *y - step; break;
        case kRest:
            cursorOnXY (*x, *y, 0);
            return 1 and !instant;
        case kHelp:
            if (Level->isInBounds (*x, *y) and
                Level->isOccupied (*x, *y))
            {
                shCreature *c = Level->getCreature (*x, *y);
                if (Hero.canSee (c) or Hero.canHearThoughts (c))
                    c->showLore ();
            }
            continue;
        default:
            I->p ("Use the movement keys to position the cursor.");
            I->p ("Confirm the location with '%s' key, space or enter.",
                  I->getKeyForCommand (kRest));
            I->p ("Press escape to abort.  Press '%s' for creature lore.",
                  I->getKeyForCommand (kHelp));
            continue;
        }
        if (Level->isInBounds (tx, ty) and
            distance (&Hero, tx, ty) <= 5 * maxradius)
        {
            *x = tx;
            *y = ty;
        }
        if (instant) {
            cursorOnXY (*x, *y, 0);
            return 1;
        }
    }
}


void
shInterface::readKeybindings (const char *fname, int n_keys, const shNameToKey KeyNames[])
{
    static const struct shCommandName
    {
        const char *name;
        Command cmd;
    } CommandNames[] =
    {
        {"move n", kMoveN},
        {"move ne", kMoveNE},
        {"move e", kMoveE},
        {"move se", kMoveSE},
        {"move s", kMoveS},
        {"move sw", kMoveSW},
        {"move w", kMoveW},
        {"move nw", kMoveNW},
        {"move down", kMoveDown},
        {"move up", kMoveUp},

        {"run n", kGlideN},
        {"run ne", kGlideNE},
        {"run e", kGlideE},
        {"run se", kGlideSE},
        {"run s", kGlideS},
        {"run sw", kGlideSW},
        {"run w", kGlideW},
        {"run nw", kGlideNW},
        {"run down", kGlideDown},
        {"run up", kGlideUp},

        {"shoot n", kFireN},
        {"shoot ne", kFireNE},
        {"shoot e", kFireE},
        {"shoot se", kFireSE},
        {"shoot s", kFireS},
        {"shoot sw", kFireSW},
        {"shoot w", kFireW},
        {"shoot nw", kFireNW},
        {"shoot down", kFireDown},
        {"shoot up", kFireUp},

        {"adjust inventory", kAdjust},
        {"close", kClose},
        {"discoveries", kDiscoveries},
        {"drop", kDrop},
        {"multi drop", kDropMany},
        {"skills", kEditSkills},
        {"examine", kExamine},
        {"execute", kExecute},
        {"shoot", kFireWeapon},
        {"run", kGlide},
        {"help", kHelp},
        {"show message history", kHistory},
        {"kick", kKick},
        {"show inventory", kListInventory},
        {"look around", kLook},
        {"main menu", kMainMenu},
        {"use mutant power", kMutantPower},
        {"name", kName},
        {"open", kOpen},
        {"pay", kPay},
        {"pick up", kPickup},
        {"pick up all", kPickUpAll},
        {"quaff", kQuaff},
        {"rest", kRest},
        {"swap", kSwap},
        {"take off", kTakeOff},
        {"toggle autopickup", kToggleAutopickup},
        {"throw", kThrow},
        {"apply", kUse},
        {"wear", kWear},
        {"wield", kWield},
        {"zap ray gun", kZapRayGun},
        {"debug command", kBOFHPower}
    };
    const int n_commands = sizeof (CommandNames) / sizeof (shCommandName);

    char keymapname[PRIME_PATH_LENGTH];
    snprintf (keymapname, sizeof (keymapname)-1, "%s/%s", DATADIR, fname);
    FILE *keymap = fopen (keymapname, "r");
    if (!keymap) {
        p ("Could not read '%s'.", keymapname);
        return; /* Loading default keymap is in main.cpp. */
    } /* Save keymap file name. */
    strncpy (&Flags.mKeySet[0], fname, sizeof (Flags.mKeySet)-1);
    resetKeybindings ();
    int scan, errors = 0;
    do {
        char buf[200];
        char *cmd_str, *key_str;
        Command command;
        int key;
        buf[0] = 0;
        scan = NULL != fgets (&buf[0], sizeof (buf)-1, keymap);
        if (buf[0] == '\n' or buf[0] == 0) { /* Blank line. */
            continue;
        } else if (buf[0] == '#') { /* Comment character. */
            continue;
        } /* Parse 'command = key' style assignments. */
        cmd_str = &buf[0];
        key_str = strchr (&buf[0], '=');
        if (!key_str) {
            debug.log ("Expected '=' in key to command assignment:");
            debug.log ("%s", buf);
            ++errors;
            continue;
        } /* Remove whitespace from beginnings. */
        while (isspace (*cmd_str))
            ++cmd_str;
        if (*cmd_str == '=') {
            debug.log ("Invalid null command assignment:");
            debug.log ("%s", buf);
            ++errors;
            continue;
        }
        ++key_str;
        while (isspace (*key_str))
            ++key_str;
        if (*key_str == '\0') {
            debug.log ("Invalid null key assignment:");
            debug.log ("%s", buf);
            ++errors;
            continue;
        } /* Remove whitespace from endings. */
        char *work = strchr (&buf[0], '=') - 1;
        while (isspace (*work))
            *(work--) = 0;
        work = key_str + strlen (key_str) - 1;
        while (isspace (*work))
            *(work--) = 0;
        /* Ignore case. */
        for (int i = strlen (cmd_str) - 1; i >= 0; --i) {
            cmd_str[i] = tolower (cmd_str[i]);
        } /* Convert only if not letter (with control char). */
        int key_strlen = strlen (key_str);
        if (key_strlen > 2) {
            for (int i = strlen (key_str) - 1; i >= 0; --i) {
                key_str[i] = tolower (key_str[i]);
            }
        } /* Find command name in array. */
        int n = 0;
        while (n < n_commands and strcmp (CommandNames[n].name, cmd_str))
            ++n;
        if (n == n_commands) { /* Skip this line and continue. */
            debug.log ("Unknown command '%s'.", cmd_str);
            ++errors;
            continue;
        } /* Success. */
        command = CommandNames[n].cmd;
        if (key_strlen > 2) { /* Find key name in array. */
            int n = 0;
            while (n < n_keys and strcmp (KeyNames[n].name, key_str))
                ++n;
            if (n == n_keys) {
                debug.log ("Unknown key '%s'.", key_str);
                ++errors;
                continue;
            }
            key = KeyNames[n].key;
        } else {
            if (key_strlen == 2) {
                if (key_str[0] != '^' and key_str[0] != '~') {
                    debug.log ("Expected modifier character: %s.", key_str);
                    ++errors;
                    continue;
                } else if (key_str[0] == '^') {
                    key = 0x1f & key_str[1]; /* Control key. */
                } else {
                    key = 0x80 | key_str[1]; /* Alt key. */
                }
            } else {
                key = key_str[0];
            }
        } /* Success. Proceed to assignment. */
        debug.log ("Assigning '%c'(%d) to %s.", key, key, CommandNames[n].name);
        assign (key, command);
    } while (scan);
    if (errors) {
        p ("There %s %d parse error%s reading %s.",
            errors > 1 ? "were" : "was", errors,
            errors > 1 ? "s" : "", keymapname);
    }
    fclose (keymap);
}


void
shInterface::drawEquippyChar (shObject *obj)
{
    if (obj) {
        shGlyph g = obj->getGlyph ();
        setWinColor (kSide, g.mColor);
        winPutchar (kSide, g.mSym);
    } else {
        winPutchar (kSide, ' ');
    }
}

static void
printQuality (int row, const char *name, shTimeOutType key)
{
    const int BUFLEN = 17;
    char buf[BUFLEN];
    if (Hero.hasHealthMonitoring ()) {
        shCreature::TimeOut *t = Hero.getTimeOut (key);
        int turns = (t->mWhen - Clock) / FULLTURN;
        if ((t->mWhen - Clock) % FULLTURN)  ++turns; /* Round up. */
        snprintf (buf, BUFLEN, "%s (%d)", name, turns);
        I->winPrint (shInterface::kSide, row, 0, buf);
    } else
        I->winPrint (shInterface::kSide, row, 0, name);
}

void
shInterface::drawSideWin ()
{   /* Makes sense only if hero's profession is chosen because
       it tries to display his/her data and job title. */
    if (!Hero.mProfession) { return; }
    clearWin (kSide);

    setWinColor (kSide, kWhite);
    winPrint (kSide, 0, 0, Hero.mName);

    setWinColor (kSide, kGray);
    winPrint (kSide, 1, 0, Hero.getTitle ());
    winPrint (kSide, 2, 0, "XL %2d:%d", Hero.mCLevel, Hero.getXP ());

    { /* Equippy chars.  A pretty homage to Angband. */
        shObject *(shHero::*equip[]) =
        {
            &shHero::mWeapon,
            &shHero::mHelmet,
            &shHero::mGoggles,
            &shHero::mBelt,
            &shHero::mJumpsuit,
            &shHero::mBodyArmor,
            &shHero::mCloak,
            &shHero::mBoots
        };
        const int numchars = sizeof (equip) / sizeof (shObject *);
        winPrint (kSide, 3, 0, ""); /* For cursor move effect. */
        for (int i = 0; i < numchars; ++i) {
            drawEquippyChar (Hero.*equip[i]);
        }   /* Skip Eye of the BOFH slot.  It would be mostly empty. */
        for (int i = 0; i < shObjectIlk::kRightEyeball; ++i) {
            drawEquippyChar (Hero.mImplants[i]);
        }
        shObjectVector v;
        selectObjects (&v, Hero.mInventory, kObjGenericComputer);
        if (v.count ()) {
            shObject *piece = NULL;
            for (int i = 0; i < v.count (); ++i) {
                if (v.get (i)->isDesignated ()) {
                    piece = v.get (i);
                    break;
                }
            }
            drawEquippyChar (piece);
        }
    }
    setWinColor (kSide, kGray);
    winPrint (kSide, 4, 0, "Str     Con    ");
    winPrint (kSide, 5, 0, "Agi     Dex    ");
    winPrint (kSide, 6, 0, "Int     Psi    ");

    FOR_ALL_ABILITIES (i) {
        if (i == kPsi and Hero.mPsiDrain) {
            setWinColor (kSide, Hero.mPsiDrain > 0 ? kRed : kGreen);
        } else if (Hero.mAbil.getByIndex (i) < Hero.mMaxAbil.getByIndex (i)) {
            setWinColor (kSide, kYellow);
        } else {
            setWinColor (kSide, kGray);
        }
        winPrint (kSide, 4 + (i-1)/2, 12 - (i%2)*8, "%2d", Hero.mAbil.getByIndex (i));
    }

    setWinColor (kSide, kGray);
    winPrint (kSide, 8, 0, "Speed %+4d", Hero.mSpeed - 100);
    winPrint (kSide, 9, 0, "Armor  %3d", Hero.mAC);

    //TODO: Since bright red is officially named orange do it in four colors
    if (Hero.hasHealthMonitoring ()) { /* red < 1/6 >= yellow <= 1/2 > green */
        setWinColor (kSide, Hero.mHP < Hero.hpWarningThreshold () ? kOrange
                     : Hero.mHP <= Hero.mMaxHP / 2 ? kYellow : kGreen);
    }
    winPrint (kSide, 10, 0, "HitPts %3d(%d)", Hero.mHP, Hero.mMaxHP);
    {
        int en, enmax;
        en = Hero.countEnergy (&enmax);
        setWinColor (kSide, kCyan);
        if (enmax) {
            winPrint (kSide, 11, 0, "Energy%4d(%d)", en, enmax);
        } else {
            winPrint (kSide, 11, 0, "Energy%4d", en);
        }
    }
    setWinColor (kSide, kGreen);
    winPrint (kSide, 12, 0, "$%d", Hero.countMoney ());

    shObject *weapon = NULL;
    if (Hero.mWeapon) {
        if (Hero.mWeapon->isA (kObjGenericComputer) and Hero.mCloak and
            Hero.mCloak->isA (kObjPlasmaCaster))
        {
            weapon = Hero.mCloak;
        } else {
            weapon = Hero.mWeapon;
        }
    }
    if (weapon and weapon->isA (kWeapon)) {
        shObjectIlk *ilk = weapon->myIlk ();
        shObjectIlk *ammo = &AllIlks[ilk->mAmmoType];

        if (ilk->mAmmoType /* != kObjNothing */) {
            int n = 0;
            for (int i = 0; i < Hero.mInventory->count (); ++i) {
                shObject *obj = Hero.mInventory->get (i);
                if (obj->isA (ammo->mId)) {
                    n += obj->mCount;
                }
            }
            setWinColor (kSide, ammo->mReal.mGlyph.mColor);
            winPrint (kSide, 12, 7, "%c", ammo->mReal.mGlyph.mSym);
            setWinColor (kSide, kGray);
            winPrint (kSide, 12, 8, "%d", n);
        }
    } else if (weapon and weapon->isA (kRayGun) and weapon->isChargeKnown ()) {
        setWinColor (kSide, kGray);
        winPrint (kSide, 12, 7, "(%d)", weapon->mCharges);
    }
    { /* Conditions */
        int n = 13;
        const int BUFLEN = 17;
        char buf[BUFLEN];
        const char *condition = NULL;
        setWinColor (kSide, kBrown);
        /* Sort them in order of most likely to be temporary first because
           There is only space for six.  When conditions are rougly tied
           place more lethal one higher. */

        if (Hero.mTrapped.mDrowning) {
            snprintf (buf, BUFLEN, "DROWNING (%d)", Hero.mTrapped.mDrowning);
            winPrint (kSide, n++, 0, buf);
        }
        if (Hero.is (kUnableCompute) or Hero.is (kUnableUsePsi) or
            Hero.is (kUnableAttack))
        {
            winPrint (kSide, n++, 0, "Commanded");
        }
        if (Hero.isTrapped ()) {
            if (Hero.mTrapped.mWebbed) {
                snprintf (buf, BUFLEN, "Webbed (%d)", Hero.mTrapped.mWebbed);
                winPrint (kSide, n++, 0, buf);
            }
            if (Hero.mTrapped.mTaped) {
                snprintf (buf, BUFLEN, "Taped (%d)", Hero.mTrapped.mTaped);
                winPrint (kSide, n++, 0, buf);
            }
            int inpit = Hero.isInPit ();
            if (inpit) {
                if (inpit < 0) {
                    snprintf (buf, BUFLEN, "In a pit");
                } else {
                    snprintf (buf, BUFLEN, "In a pit (%d)", inpit);
                }
                winPrint (kSide, n++, 0, buf);
            }
        }
        if (Hero.is (kFrightened)) {
            winPrint (kSide, n++, 0, "Frightened");
        }
        if (Hero.is (kStunned))
            printQuality (n++, "Stunned", STUNNED);
        if (Hero.is (kConfused))
            printQuality (n++, "Confused", CONFUSED);
        if (Hero.isInvisible ()) {
            winPrint (kSide, n++, 0, "Cloaked");
        } else if (Hero.hasCamouflage ()) {
            winPrint (kSide, n++, 0, "Camouflaged");
        }
        if (Hero.is (kHosed)) {
            winPrint (kSide, n++, 0, "Hosed");
        }
        if (Hero.is (kSickened))
            printQuality (n++, "Sickened", SICKENED);
        if (Hero.is (kViolated))
            printQuality (n++, "Sore", VIOLATED);
        if (Hero.getStoryFlag ("impregnation") > 0) {
            winPrint (kSide, n++, 0, "Pregnant");
        }
        if (Hero.is (kPlagued)) {
            setWinColor (kSide, kRed);
            winPrint (kSide, n++, 0, "PLAGUED");
            setWinColor (kSide, kBrown);
        }
        if (Hero.getStoryFlag ("superglued tongue")) {
            winPrint (kSide, n++, 0, "Mute");
        }
        int radsymptom = Hero.getStoryFlag ("radsymptom");
        if (radsymptom > 1) {
            winPrint (kSide, n++, 0, radsymptom > 15 ? "Radiation Ghoul" :
                              radsymptom >  9 ? "Radiation Zombie" :
                                                "Radiation Sick");
        }
        if (Hero.isBlind ()) { /* Is obvious so goes last. */
            winPrint (kSide, n++, 0, "Blind");
        }

        switch (Hero.getEncumbrance ()) {
        case kOverloaded: condition = "Overloaded"; break;
        case kOvertaxed: condition = "Overtaxed"; break;
        case kStrained: condition = "Strained"; break;
        case kBurdened: condition = "Burdened";
        }
        if (condition) {
            winPrint (kSide, n++, 0, condition);
        }
    }
    setWinColor (kSide, kWhite);
    winPrint (kSide, BOFH ? 17 : 19, 0, "%5s %d", Level->mName, Level->mDLevel);
    if (BOFH) {
        setWinColor (kSide, kGray);
        //winPrint (kSide, 17, 0, "wt %5d/%5d", Hero.mWeight, Hero.mCarryingCapacity);
        winPrint (kSide, 18, 0, "rad %d ", Hero.mRad);
        winPrint (kSide, 19, 0, "%d ", Clock);
    }
    refreshWin (kSide);
}


void
shInterface::showHistory ()
{
    int entries; /* How many lines of history are there? */
    if (mHistoryWrapped) {
        entries = HISTORY_ROWS;
    } else {
        entries = mHistoryIdx;
    }
    char *lines = (char *) calloc (entries * LINELEN, sizeof (char));

    /* Copy history.  Might be in two parts if wrapped. */
    if (mHistoryWrapped) {
        int lines1 = mHistoryIdx;
        size_t chunk1 = LINELEN * lines1;
        int lines2 = (HISTORY_ROWS - mHistoryIdx) % HISTORY_ROWS;
        size_t chunk2 = LINELEN * lines2;
        memcpy (lines, mLogHistory + chunk1, chunk2);
        memcpy (lines + chunk2, mLogHistory, chunk1);
    } else {
        memcpy (lines, mLogHistory, (entries * LINELEN));
    }

    shTextViewer *history = new shTextViewer (lines, entries, LINELEN);
    history->show (true);
    delete history; /* Deletes char *lines. */
    free (lines);
}


void
shInterface::showKills ()
{
    int entries = 0, longest = 0;
    for (int i = 0; i < kMonNumberOf; ++i) {
        if (MonIlks[i].mKills) {
            ++entries;
            int len = strlen (MonIlks[i].mName);
            longest = maxi (len, longest);
        }
    }
    longest += 15; /* Should suffice for "@Xx@H - nn" part. */
    char *lines = (char *) calloc (entries * longest, sizeof (char));
    entries = 0;
    char *buf = GetBuf ();
    for (int i = 0; i < kMonNumberOf; ++i) {
        if (MonIlks[i].mKills) {
            snprintf (buf, LINELEN, "%s", MonIlks[i].mName);
            if (MonIlks[i].mKills > 1 and i != kMonCreepingCredits) {
                makePlural (buf, LINELEN);
            }
            sprintf (lines + entries * longest, "@%c%c@H - %d %s",
                MonIlks[i].mGlyph.mColor + 'A', MonIlks[i].mGlyph.mSym,
                MonIlks[i].mKills, buf);
            ++entries;
        }
    }
    shTextViewer *kill_list = new shTextViewer (lines, entries, longest);
    kill_list->show ();
    delete kill_list;
    free (lines);
}

struct shStaticEffect
{
    char ch;
    shColor cl, bg;
};

struct shStaticGlyph
{
    char ch;
    shColor cl;
};

static const shGlyph
terrainGlyph (shTerrainType t)
{
    static const struct shStaticGlyph terrain[kMaxTerrainType] =
    {
        {' ', kBlack},     /* kStone */
        {'#', kBlue},      /* kVWall */
        {'#', kBlue},      /* kHWall */
        {'#', kBrown},     /* kCavernWall1 */
        {'#', kBrown},     /* kCavernWall2 */
        {'#', kCyan},      /* kSewerWall1 */
        {'#', kCyan},      /* kSewerWall2 */
        {'#', kGreen},     /* kVirtualWall1 */
        {'#', kGreen},     /* kVirtualWall2 */
        {'#', kBlue},      /* kNWCorner */
        {'#', kBlue},      /* kNECorner */
        {'#', kBlue},      /* kSWCorner */
        {'#', kBlue},      /* kSECorner */
        {'#', kBlue},      /* kNTee */
        {'#', kBlue},      /* kSTee */
        {'#', kBlue},      /* kWTee */
        {'#', kBlue},      /* kETee */
        {'#', kAqua},      /* kGlassPanel */
        {'.', kBlue},      /* kFloor */
        {'.', kBrown},     /* kCavernFloor */
        {'.', kBrown},     /* kSewerFloor */
        {'.', kGreen},     /* kVirtualFloor */
        {'.', kNavy},      /* kBrokenLightAbove */
        {'~', kGreen},     /* kSewage */
        {'^', kBlue}       /* kVoid */
    };
    shGlyph g;
    g.mSym = terrain[t].ch;
    g.mColor = terrain[t].cl;
    g.mBkgd = kBlack;
    return g;
}

/* Returns glyph of a feature.  Handles fancy coloring of doors. */
static const shGlyph      /* Does not expect NULL pointer. */
featureGlyph (shFeature *f)
{
    static const struct shStaticGlyph feature[shFeature::kMaxFeatureType] =
    {
        {'#',  kBlue},      /* shFeature::kDoorHiddenVert */
        {'#',  kBlue},      /* shFeature::kDoorHiddenHoriz */
        {'+',  kCyan},      /* shFeature::kDoorClosed */
        {'=',  kCyan},      /* shFeature::kMovingHWall */
        {'&',  kCyan},      /* shFeature::kMachinery */
        {'<',  kWhite},     /* shFeature::kStairsUp */
        {'>',  kWhite},     /* shFeature::kStairsDown */
        {'{',  kLime},      /* shFeature::kVat */
        {'_',  kGreen},     /* shFeature::kComputerTerminal */
        {'^',  kBrown},     /* shFeature::kPit */
        {'^',  kYellow},    /* shFeature::kAcidPit */
        {'^',  kGreen},     /* shFeature::kSewagePit */
        {'^',  kLime},      /* shFeature::kRadTrap */
        {'^',  kRed},       /* shFeature::kTrapDoor */
        {'^',  kGray},      /* shFeature::kHole */
        {'"',  kCyan},      /* shFeature::kWeb */
        {'^',  kMagenta},   /* shFeature::kPortal */
        {'#',  kRed},       /* shFeature::kPortableHole */
       {'\'',  kCyan}       /* shFeature::kDoorOpen */
    };

    shGlyph g;
    g.mBkgd = kBlack;

    if (f->isDoor () and !f->isHiddenDoor ()) {
        if (!f->isOpenDoor ()) {
            g.mSym = f->isMagneticallySealed () ?
                (f->isHorizontalDoor () ? '-' : '|') :
                (f->isBerserkDoor () and !f->mTrapUnknown) ? '"' : '+';
        } else {
            g.mSym = (f->isBerserkDoor () and !f->mTrapUnknown) ? '`' : '\'';
        }
        if (f->isLockDoor ()) {
            if (f->isLockBrokenDoor ()) {
                if (f->isLockedDoor ()) { /* Damaged and jammed lock. */
                    g.mColor = kAqua;
                } else { /* Badly broken lock.  Can be still repaired. */
                    g.mColor = kGray;
                }
            } else if (f->isRetinaDoor ()) {
                g.mColor = kWhite;
            } else if (f->isCodeLockDoor ()) {
                shObjectIlk *ilk = f->keyNeededForDoor ();
                if (ilk) { /* Door color matches keycard color. */
                    g.mColor = ilk->mReal.mGlyph.mColor;
                } else { /* Lock color unknown. */
                    g.mColor = kBrown;
                }
            } else { /* Something not right. */
                g.mColor = kMagenta;
            }
        } else { /* Door without lock. */
            g.mColor = kCyan;
        }
        return g;
    } else {
        g.mSym = feature[f->mType].ch;
        g.mColor = feature[f->mType].cl;
        return g;
    }
}

static shGlyph *
specialEffectGlyph (shSpecialEffect e)
{
    static const struct shStaticEffect effects[kLastEffect] =
    {
        {' ', kBlack,   kBlack}, /* kNone */
        {'/', kAqua,    kBlack}, /* kLaserBeamFDiagEffect */
        {'-', kAqua,    kBlack}, /* kLaserBeamHorizEffect */
        {'|', kAqua,    kBlack}, /* kLaserBeamVertEffect */
       {'\\', kAqua,    kBlack}, /* kLaserBeamBDiagEffect */
        {'*', kAqua,    kBlack}, /* kLaserBeamEffect */
        {'/', kRed,     kBlack}, /* kOpticBlastFDiagEffect */
        {'-', kRed,     kBlack}, /* kOpticBlastHorizEffect */
        {'|', kRed,     kBlack}, /* kOpticBlastVertEffect */
       {'\\', kRed,     kBlack}, /* kOpticBlastBDiagEffect */
        {'*', kRed,     kBlack}, /* kOpticBlastEffect */
        {'/', kYellow,  kBlack}, /* kBoltFDiagEffect */
        {'-', kYellow,  kBlack}, /* kBoltHorizEffect */
        {'|', kYellow,  kBlack}, /* kBoltVertEffect */
       {'\\', kYellow,  kBlack}, /* kBoltBDiagEffect */
        {'*', kYellow,  kBlack}, /* kBoltEffect */
        {'|', kOrange,  kBlack}, /* kRailNorthEffect */
        {'/', kOrange,  kBlack}, /* kRailNorthEastEffect */
        {'-', kOrange,  kBlack}, /* kRailEastEffect */
       {'\\', kOrange,  kBlack}, /* kRailSouthEastEffect */
        {'|', kOrange,  kBlack}, /* kRailSouthEffect */
        {'/', kOrange,  kBlack}, /* kRailSouthWestEffect */
        {'-', kOrange,  kBlack}, /* kRailWestEffect */
       {'\\', kOrange,  kBlack}, /* kRailNorthWestEffect */
        {' ', kBlack,   kBlack}, /* kRailEffect */
        {'^', kWhite,   kBlack}, /* kCombiNorthEffect */
        {'/', kWhite,   kBlack}, /* kCombiNorthEastEffect */
        {'>', kWhite,   kBlack}, /* kCombiEastEffect */
       {'\\', kWhite,   kBlack}, /* kCombiSouthEastEffect */
        {'v', kWhite,   kBlack}, /* kCombiSouthEffect */
        {'/', kWhite,   kBlack}, /* kCombiSouthWestEffect */
        {'<', kWhite,   kBlack}, /* kCombiWestEffect */
       {'\\', kWhite,   kBlack}, /* kCombiNorthWestEffect */
        {'-', kGray,    kBlack}, /* kCombiHorizEffect */
        {'|', kGray,    kBlack}, /* kCombiVertEffect */
        {'/', kGray,    kBlack}, /* kCombiFDiagEffect */
       {'\\', kGray,    kBlack}, /* kCombiBDiagEffect */
        {' ', kBlack,   kBlack}, /* kInvisibleEffect */
        {'.', kWhite,   kBlack}, /* kPeaEffect */
        {'*', kLime,    kBlack}, /* kPlasmaEffect */
        {'*', kLime,    kBlack}, /* kPlasmaHitEffect */
        {'*', kGray,    kBlack}, /* kExplosionEffect */
        {'*', kBlue,    kBlack}, /* kColdEffect */
        {'*', kRed,     kBlack}, /* kPoisonEffect */
        {'*', kGreen,   kBlack}, /* kRadiationEffect */
        {'*', kWhite,   kBlack}, /* kBlindingEffect */
        {'*', kRed,     kBlack}, /* kIncendiaryEffect */
        {'+', kBlack,   kBlack}, /* kPsionicStormEffect */
        {'*', kBlack,   kBlue},  /* kDisintegrationEffect */
        {'*', kGray,    kBlack}, /* kShrapnelEffect */
        {'*', kYellow,  kBlack}, /* kAcidEffect */
        {'*', kAqua,    kBlack}, /* kWaterEffect */
        {'0', kBlack,   kGreen}, /* kBinaryEffect */
        {'&', kBlack,   kAqua},  /* kBugsEffect */
        {'*', kCyan,    kBlack}, /* kVirusesEffect */
        {'.', kGreen,   kBlack}, /* kSpitEffect */
        {'*', kGreen,   kBlack}, /* kDiscEffect */
        {'0', kRed,     kBlack}, /* kRadarBlip */
        {'I', kBrown,   kBlack}, /* kSensedLife */
        {'0', kOrange,  kBlack}  /* kSensedTremor */
    };
 /*
    static int spin_cnt = RNG (4);
    static int spin_dir = RNG (2) ? +1 : -1;
  */
    static shGlyph g;

    /* Laser corners are only needed in NotEye mode.  Ignore them. */
    e = shSpecialEffect (e & ~kLaserBeamCEffect);
    /* Use default values.  May be overridden. */
    g.mSym = effects[e].ch;
    g.mColor = effects[e].cl;
    g.mBkgd = effects[e].bg;
    switch (e) {
    case kNone:
    case kInvisibleEffect:
        return NULL;
    case kIncendiaryEffect:
    {
        const shColor cols[] = {kRed, kOrange, kYellow};
        g.mColor = cols[RNG (3)];
        break;
    }
    case kPsionicStormEffect: {
        const shColor cols[] = {kNavy, kAqua, kBlue, kWhite};
        g.mColor = cols[RNG (4)];
        const char syms[] = "/|-\\";
        g.mSym = syms[RNG (4)];
        break;
    }
    case kBinaryEffect:
        g.mSym += RNG (2);
        break;
    case kBugsEffect:
    {
        const char bugs[] = "~`@#$%^&*()-_=+:;'\"[]{}|\\/?>.<,";
        g.mSym = bugs[RNG (strlen (bugs))];
        break;
    }
    /*
    case kSpinEndEffect:
        spin_dir = RNG (2) ? +1 : -1;
        break;
    case kSpinEffect:
    {
        const char spin[] = "|/-\\";
        if (spin_dir > 0) {
            spin_cnt = ++spin_cnt > 3 ? 0 : spin_cnt;
        } else {
            spin_cnt = --spin_cnt < 0 ? 3 : spin_cnt;
        }
        waddch (mWin[kMain], spin[spin_cnt] | ColorMap[kGreen]);
        break;
    }
    */
    default:
        break;
    }

    return &g;
}


shGlyph
shInterface::getASCII (int x, int y, shSpecialEffect e, shCreature *c,
        shObjectIlk *oi, shObject *o, shFeature *f, shSquare *s)
{
    if (e) { /* Explosions and the like obscure everything. */
        shGlyph *g = specialEffectGlyph (e);
        if (g)  return *g;
    }

    if (c) { /* Creatures have highest priority. */
        if (c->isHero () and Hero.mBodyArmor and
            Hero.mBodyArmor->isPoweredArmor () and
            Hero.mBodyArmor->isAppearanceKnown ())
        {   /* Powered body armor changes hero glyph color. */
            shGlyph g;
            g.mSym = c->mGlyph.mSym;
            g.mColor = Hero.mBodyArmor->apparent ()->mGlyph.mColor;
            g.mBkgd = kBlack;
            return g;
        } else if (c->isPet ()) {
            shGlyph g;
            g.mSym = c->mGlyph.mSym;
            g.mColor = kBlack;
            g.mBkgd = c->mGlyph.mColor;
            return g;
        } else {
            return c->mGlyph;
        }
    }

    if (o) { /* Next is object on top of a pile. */
        return o->getGlyph ();
    } else if (oi) {
        return oi->mVague.mGlyph;
    }

    if (f) { /* Features unobscured by objects. */
        return featureGlyph (f);
    }

    if (s) { /* Finally, terrain. */
        shGlyph g = terrainGlyph (s->mTerr);
        if (Level->isFloor (x, y) and  /* Flashlight radius. */
            Hero.hasLightSource () and distance (&Hero, x, y) <= 25 and
            Level->getSquare (x, y)->mTerr != kSewage and Level->isInLOS (x, y))
        {
            g.mColor = kYellow;
        }
        return g;
    }
    return terrainGlyph (kVoid);
}

shGlyph
shInterface::getMemASCII (int x, int y, shSpecialEffect e, shCreature *c,
                      shObjectIlk *oi, shFeature *f, shSquare *s)
{
    shMapLevel::shMemory mem = Level->getMemory (x, y);

    if (e) {
        shGlyph *g = specialEffectGlyph (e);
        if (g)  return *g;
    }

    if (c) {
        if (c->isHero () and Hero.mBodyArmor and
            Hero.mBodyArmor->isPoweredArmor () and
            Hero.mBodyArmor->isAppearanceKnown ())
        {
            shGlyph g;
            g.mSym = c->mGlyph.mSym;
            g.mColor = Hero.mBodyArmor->apparent ()->mGlyph.mColor;
            g.mBkgd = kBlack;
            return g;
        } else {
            return c->mGlyph;
        }
    } else if (mem.mMon) {
        return MonIlks[mem.mMon].mGlyph;
    }

    if (oi) {
        return oi->mVague.mGlyph;
    } else if (mem.mObj) {
        return AllIlks[mem.mObj].mVague.mGlyph;
    }

    if (f) {
        return featureGlyph (f);
    } else if (mem.mFeat != shFeature::kMaxFeatureType) {
        /* Ugly kludge here.  Only door flags are saved, not whether
           door malfunctions are known by hero. */
        shFeature *real = Level->getFeature (x, y);
        shFeature fake;
        fake.mType = mem.mFeat;
        fake.mDoor.mFlags = mem.mDoor;
        if (real)
            fake.mTrapUnknown = real->mTrapUnknown;
        return featureGlyph (&fake);
    }

    if (s) {
        if (Hero.mZ < 0) { /* In a pit. */
            if (Level->isFloor (x, y)) {
                return terrainGlyph (kStone); /* Black area. */
            } else {
                return terrainGlyph (s->mTerr);
            }
        } else {
            if (Flags.mShowLOS and Level->isFloor (x, y)) {
                return terrainGlyph (kStone);
            } else {
                return terrainGlyph (s->mTerr);
            }
        }
    } else if (mem.mTerr != kMaxTerrainType) {
        if (Hero.mZ < 0 and Level->isFloor (x, y)) {
            return terrainGlyph (kStone);
        } else {
            return terrainGlyph (mem.mTerr);
        }
    }
    return terrainGlyph (kStone);
}


void
shMapLevel::draw ()
{
    for (int y = 0; y < MAPMAXROWS; ++y) {
        for (int x = 0; x < MAPMAXCOLUMNS; ++x) {
            drawSq (x, y);
        }
    }
}


int
shMapLevel::rememberedCreature (int x, int y)
{
    return mRemembered[x][y].mMon != kMonEarthling;
}

void
shMapLevel::feelSq (int x, int y)
{
    remember (x, y, getSquare (x, y)->mTerr);
    shFeature *f = getFeature (x, y);
    shFeature *kf = getKnownFeature (x, y);
    if (kf or (f and f->isHiddenDoor ())) {
        if (f->isDoor ()) {
            /* Feeling the door does not reveal what color is door lock. */
            mRemembered[x][y].mFeat = f->mType;
            /* On the other hand lock type can be checked easily. */
            mRemembered[x][y].mDoor |=
                (f->mDoor.mFlags & shFeature::kCanFeel);
        } else {
            remember (x, y, f);
        }
    } else {
        forgetFeature (x, y);
    }
    if (isOccupied (x, y) and (Hero.mX != x or Hero.mY != y) and
        !Hero.isAwareOf (getCreature (x, y)))
    {
        remember (x, y, kMonInvisible);
    } else {
        forgetCreature (x, y);
    }
    if (countObjects (x, y) > 0) {
        shObjectVector *objs = getObjects (x, y);
        shObject *bestobj = findBestObject (objs);
        remember (x, y, bestobj->mIlkId);
    } else {
        forgetObject (x, y);
    }
    Level->drawSq (x, y);
}

void
shMapLevel::drawSq (int x, int y, int forget /* = 0 */)
{
    shCreature *c = getCreature (x, y);
    shFeature *f = getFeature (x, y);
    shFeature *kf = getKnownFeature (x, y);
    shSpecialEffect e = getSpecialEffect (x, y);
    if (!Hero.canSee (x, y)) {
        /* Explosions and beams light up dark area for a moment. */
        if (!Hero.isBlind () and isInLOS (x, y) and e) {
            I->drawMem (x, y, e, NULL, NULL, NULL, NULL);
            return;
        }
        if (c == &Hero) { /* You know where you are.  Duh. */
            I->drawMem (x, y, kNone, c, NULL, NULL, NULL);
        } else if (c and Hero.canHearThoughts (c)) {
            forgetCreature (x, y);
            I->drawMem (x, y, kNone, c, NULL, NULL, NULL);
        } else if (c and (Hero.hasNightVision () or Hero.hasEMFieldVision ())
                   and Hero.canSee (c))
        {
            forgetCreature (x, y);
            I->drawMem (x, y, kNone, c, NULL, NULL, NULL);
        } else if (c and Hero.hasSenseLife () and Hero.canSenseLife (c)) {
            I->drawMem (x, y, kSensedLife, NULL, NULL, NULL, NULL);
        } else if (Hero.hasMotionDetection () and !Hero.isBlind () and
            (    /* Moving creature. */
             (c and c->isMoving ())
              or /* Moving automatic door. */
             (f and f->isDoor () and f->mDoor.mMoveTime + SLOWTURN >= Clock)
            ) and distance (&Hero, x, y) < 50)
        {
            I->drawMem (x, y, kRadarBlip, NULL, NULL, NULL, NULL);
            if (!(AllIlks[kObjMotionTracker].mFlags & kIdentified)) {
                AllIlks[kObjMotionTracker].mFlags |= kIdentified;
                I->p ("You now recognize motion trackers.");
                for (int i = 0; i < Hero.mInventory->count (); ++i) {
                    shObject *obj = Hero.mInventory->get (i);
                    if (obj->isA (kObjMotionTracker))  obj->setKnown ();
                }
            }
        } else if (c and Hero.hasTremorsense () and Hero.canFeelSteps (c)) {
            I->drawMem (x, y, kSensedTremor, NULL, NULL, NULL, NULL);
        } else if (Hero.hasNightVision () and isInLOS (x, y) and
                   kf and kf->isDoor () and kf->isOpenDoor () and
                   Level->getMemory (x, y).mFeat == shFeature::kDoorClosed)
        { /* Kludge: show doors because, e.g. night vision might have
             revealed a creature on the other side that hero might not
             expect to see if she remembers the door was closed. */
            I->drawMem (x, y, kNone, NULL, NULL, kf, NULL);
        } else { /* Everything from memory. */
            I->drawMem (x, y);
        }
        return;
    }

    shObjectVector *objs = getObjects (x, y);
    shObject *bestobj = NULL;
    shObjectIlk *objilk = NULL;
    if (objs and !Level->isWatery (x, y) and
        (bestobj = findBestObject (objs, true)))
    {
        objilk = bestobj->myIlk ();
        Level->remember (x, y, objilk->mId);
    } else {
        Level->forgetObject (x, y);
    }

    if (c) { /* Creatures may stay in hiding. */
        if (c->mHidden > 0 and Hero.canHearThoughts (c)) {
            c->mHidden *= -1; /* Show the hidden creature from now on. */
        }
        if (c->mHidden > 0) { /* Hero is not aware of the monster. */
            switch (c->mMimic) {
            case shCreature::kObject:
                Level->remember (x, y, c->mMimickedObject->mId);
                objilk = c->mMimickedObject; /* Obscure items. */
                bestobj = NULL;
                break;
            default:
                break; /* Invisible, or hiding under existing object. */
            }
            c = NULL;
        }
        if (c and !Hero.canSee (c)) {
            if (!e and Hero.hasMotionDetection () and !Hero.isBlind () and
                c->isMoving () and distance (&Hero, x, y) < 50)
            {
                e = kRadarBlip;
            }
            c = NULL;
        }
        if (c) {
            if (c->isSessile ()) { /* Mark non-moving monsters. */
                Level->remember (x, y, c->mIlkId);
            } else { /* Clear any remembered creature leftovers. */
                forgetCreature (x, y);
            }
        }
    }
    if (!c)  forgetCreature (x, y);

    if (f and f->isHiddenDoor ())  kf = f; /* Drawn as walls. */
    if (kf) {
        Level->remember (x, y, kf);
    } else {
        Level->forgetFeature (x, y);
    }

    shSquare *s = getSquare (x, y);
    if (isFloor (x, y) and !isLit (x, y, x, y) and !Hero.isBlind ()) {
        Level->forgetTerrain (x, y);
    } else {
        Level->remember (x, y, s->mTerr);
    }

    I->draw (x, y, e, c, objilk, bestobj, kf, s);
}

#ifndef _WIN32
#include <time.h>   // nanosleep

void
shInterface::delay (int ms)
{
    timespec wait;
    wait.tv_sec = ms / 1000;
    wait.tv_nsec = (ms % 1000) * 1000000; /* Nanoseconds. */
    nanosleep (&wait, NULL);
}
#else
#undef DATADIR /* The windows.h header also has such define. */
#include <windows.h>

void
shInterface::delay (int ms)
{
    Sleep (ms);
}
#endif
