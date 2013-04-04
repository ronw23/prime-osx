#include <stdio.h>
#include <ctype.h>
#include "Interface.h"


shMenu::shMenu (const char *prompt, int flags)
    : mChoices ()
{
    strncpy (mPrompt, prompt, 79); mPrompt[79] = 0;
    mFlags = flags;
    mResultIterator = 0;
    mOffset = 0;
    mDone = 0;
    mNum = 0;
    mObjTypeHack = kMaxObjectType;
    mHelpFileName = NULL;
}


shMenu::~shMenu ()
{
    for (int i = 0; i < mChoices.count (); ++i) {
        delete mChoices.get (i);
    }
}


void
shMenu::addIntItem (char letter, const char *text, int value,
                    int count /* = 1 */, int selected /* = 0 */)
{
    shMenuChoice *c;

    if (0 == letter) {
        letter = ' ';
    }

    c = new shMenuChoice (letter, text, value, count, selected);
    mChoices.add (c);
}

void
shMenu::addPtrItem (char letter, const char *text, const void *value,
                    int count /* = 1 */, int selected /* = 0 */)
{
    const char *typeHeader[kMaxObjectType] =
    {
        "UNINITIALIZED       (! please report !)",
        "Money               (toggle all with $)",
        "Bionic Implants     (toggle all with :)",
        "Floppy Disks        (toggle all with ?)",
        "Canisters           (toggle all with !)",
        "Tools               (toggle all with ()",
        "Armor               (toggle all with [)",
        "Weapons             (toggle all with ))",
        "Ammunition          (toggle all with =)",
        "Other               (toggle all with &)",
        "Ray Guns            (toggle all with /)",
        "Energy Cells        (toggle all with *)"
    };

    shMenuChoice *c;

    if (0 == letter) {
        letter = ' ';
    }
    if (mFlags & kCategorizeObjects and value) {
        shObjectType t = ((shObject *) value) -> apparent ()->mType;
        if (t != mObjTypeHack) {
            if (t >= kUninitialized and t <= kEnergyCell) {
                if (mFlags & kMultiPick) { /* Show toggle key. */
                    addHeader (typeHeader[t]);       /* From above. */
                } else {
                    addHeader (objectTypeHeader[t]); /* Default. */
                }
            } else {
                addHeader ("------");
            }
            mObjTypeHack = t;
        }
    }

    c = new shMenuChoice (letter, text, value, count, selected);
    mChoices.add (c);
}

void
shMenu::addHeader (const char *text)
{   /* prints out the header */
    addPtrItem (-1, text, NULL, -1);
}

void
shMenu::addPageBreak ()
{   /* Just a prettifier. */
    addPtrItem (0, "", NULL, -1);
    /* -2 is signal to adjust space on both sides of text. */
    addPtrItem (-2, "---..oo..--=oOo=--..oo..---", NULL, -1);
    addPtrItem (0, "", NULL, -1);
}

void
shMenu::addText (const char *text)
{
    addPtrItem (' ', text, NULL);
}

void
shMenu::finish () {
    assert (kNoPick & mFlags);
    accumulateResults ();
}

shMenuChoice *
shMenu::getResultChoice ()
{
    if (!mDone)
        accumulateResults ();

    while (mResultIterator < mChoices.count ()) {
        shMenuChoice *choice = mChoices.get (mResultIterator++);
        if ((choice->mSelected)) {
            return choice;
        }
    }
    return NULL;
}

/* call this repeatedly to store the selected results into value and count.
  RETURNS: 0 if there are no (more) results, 1 o/w
*/

int
shMenu::getIntResult (int *value, int *count /* = NULL */)
{
    shMenuChoice *choice = getResultChoice ();

    if (choice) {
        *value = choice->mValue.mInt;
        if (NULL != count) {
            *count = choice->mSelected;
        }
        return 1;
    }
    *value = 0;
    return 0;
}

int
shMenu::getPtrResult (const void **value, int *count /* = NULL */)
{
    shMenuChoice *choice = getResultChoice ();
    if (mDone == DELETE_FILTER_SIGNAL)
        return DELETE_FILTER_SIGNAL;

    if (choice) {
        *value = choice->mValue.mPtr;
        if (NULL != count) {
            *count = choice->mSelected;
        }
        return 1;
    }
    *value = NULL;
    return 0;
}

void
shMenu::getRandIntResult (int *value, int *count /* = NULL */)
{
    shMenuChoice *choice = mChoices.get (RNG (mChoices.count ()));

    *value = choice->mValue.mInt;
    if (NULL != count) {
        *count = choice->mSelected;
    }
}

void
shMenu::getRandPtrResult (const void **value, int *count /* = NULL */)
{
    shMenuChoice *choice = mChoices.get (RNG (mChoices.count ()));

    *value = choice->mValue.mPtr;
    if (NULL != count) {
        *count = choice->mSelected;
    }
}

void
shMenu::attachHelp (const char *fname)
{
    mHelpFileName = fname;
}

void
shMenu::select (int i1, int i2,   /* mChoices[i1..i2) */
                int action,       /* 0 unselect, 1 select, 2 toggle */
                shObjectType t)   /* = kUninitialized */
{
    int i;
    if (!(mFlags & kMultiPick)) {
        return;
    }

    for (i = i1; i < i2; i++) {
        shMenuChoice *choice = mChoices.get (i);
        if (choice->mCount < 0) {
            continue;
        }
        if (t and
            mFlags & kCategorizeObjects and
            choice->mValue.mPtr and
            t != ((shObject *) choice->mValue.mPtr) -> apparent ()->mType)
        {
            continue;
        }
        if (0 == action) {
            choice->mSelected = 0;
        } else if (1 == action) {
            choice->mSelected = choice->mCount;
        } else if (2 == action) {
            if (!choice->mSelected)
                choice->mSelected = choice->mCount;
            else
                choice->mSelected = 0;
        }
    }
}

const char **
shMenu::prepareHelp (int *lines)
{   /* Building blocks for help. */
    const char *navigation =
"ARROWS, PAGE UP, PAGE DOWN  navigate    SPACE, ENTER, ESCAPE  finish";
    static char count[] =
"Quantity: [     ]   press NUMBERS to change quantity of selected items";
    const char *multipick =
"Items:  , select all  - deselect all  @ toggle all  LETTER toggle single";

    const char *help = "TAB  show help file   ";
    const char *filter = "BACKSPACE  disable filter   ";
    const char *singlepick = "LETTER  choose and accept";

    static char lastline[81];

    /* Choose appropriate blocks. */
    *lines = 0;
    const char **text = (const char **) calloc (4, sizeof (char *));
    if (mFlags & kNoHelp)  return text;

    text[(*lines)++] = navigation;

    if (mFlags & kCountAllowed) {
        text[(*lines)++] = count;
        if (mNum == 0) {
            strncpy (count + 11, " all ", 5); /* Does not place \0 anywhere. */
        } else {
            sprintf (count + 11, "%5d", mNum < 99999 ? mNum : 99999);
            count[16] = ']'; /* Replace \0 added by sprintf. */
        }
    }

    if (mFlags & kMultiPick)
        text[(*lines)++] = multipick;

    if (mHelpFileName or (mFlags & kFiltered) or !(mFlags & kMultiPick)) {
        snprintf (lastline, 81, "%s%s%s",
            mHelpFileName ? help : "",
            mFlags & kFiltered ? filter : "",
            !(mFlags & kMultiPick) ? singlepick : "");
        text[(*lines)++] = lastline;
    }

    return text;
}

void
shMenu::dropResults ()
{
    mDone = 0;
    mResultIterator = 0;
    for (int i = 0; i < mChoices.count (); ++i) {
        mChoices.get (i) -> mSelected = 0;
    }
}

/* Returns true when key pressed was valid. */
bool
shMenu::interpretKey (int key, shInterface::Command cmd)
{
    if (cmd) switch (cmd) {
    case shInterface::kDrop: /* Drop item filter. */
        if (kFiltered & mFlags) {
            mDone = DELETE_FILTER_SIGNAL;
            return true;
        }
        return false;
    case shInterface::kMoveNW: /* Home */
        mLast -= mOffset;
        mOffset = 0;
        return true;
    case shInterface::kMoveSW: /* End */
        mOffset += mChoices.count () - mLast;
        mLast = mChoices.count ();
        return true;
    case shInterface::kMoveUp:
        if (mOffset > 0) {
            --mOffset;
            --mLast;
        }
        return true;
    case shInterface::kMoveDown:
        if (mLast < mChoices.count ()) {
            ++mOffset;
            ++mLast;
        }
        return true;
    case shInterface::kMoveNE: /* Page up */
        mOffset -= mItemHeight / 2;
        mLast -= mItemHeight / 2;
        if (mOffset < 0) {
            mLast -= mOffset;
            mOffset = 0;
        }
        return true;
    case shInterface::kMoveSE: /* Page down */
        mLast += mItemHeight / 2;
        mOffset += mItemHeight / 2;
        if (mLast >= mChoices.count ()) {
            mOffset -= mLast - mChoices.count ();
            mLast = mChoices.count ();
        }
        return true;
    case shInterface::kHelp:
        {
            shTextViewer *viewer = new shTextViewer (mHelpFileName);
            viewer->show ();
            delete viewer;
        }
        return true;
    default: break;
    }
    switch (key) {
    case '@': /* toggle all */
        select (0, mChoices.count(), 2);
        return true;
    case '-': /* deselect all */
        select (0, mChoices.count(), 0);
        return true;
    case ',': /* select all */
        select (0, mChoices.count(), 1);
        return true;
    case '$': /* toggle all cash */
        select (0, mChoices.count(), 2, kMoney);
        return true;
    case ':': /* toggle all implants */
        select (0, mChoices.count(), 2, kImplant);
        return true;
    case '?': /* toggle all floppies */
        select (0, mChoices.count(), 2, kFloppyDisk);
        return true;
    case '!': /* toggle all cans */
        select (0, mChoices.count(), 2, kCanister);
        return true;
    case '(': /* toggle all tools */
        select (0, mChoices.count(), 2, kTool);
        return true;
    case '[': /* toggle all armor */
        select (0, mChoices.count(), 2, kArmor);
        return true;
    case ')': /* toggle all weapons */
        select (0, mChoices.count(), 2, kWeapon);
        return true;
    case '=': /* toggle all ammo */
        select (0, mChoices.count(), 2, kProjectile);
        return true;
    case '/': /* toggle all ray guns */
        select (0, mChoices.count(), 2, kRayGun);
        return true;
    case '*': /* toggle all energy */
        select (0, mChoices.count(), 2, kEnergyCell);
        return true;
    case '&': /* toggle all other things */
        select (0, mChoices.count(), 2, kOther);
        return true;
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
        if (mFlags & kCountAllowed) {
            mNum = mNum * 10 + key - '0';
            if (mNum >= 1000000) {
                mNum = 1000000;
            }
            return true;
        } else {
            return false;
        }
    default:
        for (int i = 0; i < mChoices.count (); ++i) {
            shMenuChoice *item = mChoices.get (i);
            if (item->mLetter == key) {
                if ((mFlags & kCountAllowed) and mNum) { /* Set exact number. */
                    if (mNum > item->mCount) {
                        /* TODO: Warn user not that many items are available. */
                        mNum = item->mCount;
                    }
                } else if (mFlags & kSelectIsPlusOne) {
                    mNum = item->mSelected == item->mCount ? 0 :
                           item->mSelected + 1;
                } else { /* Toggle all or none. */
                    mNum = item->mSelected ? 0 : item->mCount;
                }
                item->mSelected = mNum;
                mNum = 0;
                if (!(mFlags & kMultiPick)) {
                    mDone = 1;
                }
                return true;
            }
        }
        return false;
    }
    return false;
}

/* Build status bar featuring numbers of lines shown and scroll information. */
const char *
shMenu::bottomLine (int curpos)
{
    static char buf[60];
    const char *indicator = NULL;
    if (mLast == mChoices.count () and mOffset == 0) {
        return "--All shown--";
    } else if (curpos == mChoices.count ()) {
        indicator = "--End--";
    } else if (0 == mOffset) {
        indicator = "--Top--";
    } else {
        indicator = "--More--";
    }
    sprintf (buf, "%-8s         displaying lines %d - %d of %d     ",
             indicator, mOffset + 1, curpos + 1, mChoices.count () + 1);
    return buf;
}


shTextViewer::shTextViewer (const char *fname)
{   /* Try opening target file. */
    assert (fname);
    mFree = true;
    mLines = NULL;
    mNumLines = 0;
    mMaxWidth = 0;
    char path[PRIME_PATH_LENGTH];
    snprintf (path, sizeof (path)-1, "%s/%s", DATADIR, fname);
    FILE *text = fopen (path, "r");
    if (!text) return;
    /* Count lines and check max width. */
    char *buf = GetBuf ();
    while (fgets (buf, SHBUFLEN, text)) {
        mMaxWidth = maxi (mMaxWidth, strlen (buf));
        ++mNumLines;
    }
    ++mNumLines; /* For adding " --End-- " at the bottom. */
    mMaxWidth = maxi (mMaxWidth, 9 /* --End-- */); /* Ok, this is paranoidal. */
    ++mMaxWidth; /* Compensate for \0. */
    /* Copy file to memory. */
    mLines = (char *) calloc (mNumLines * mMaxWidth, sizeof (char));
    rewind (text);
    int line = 0;
    while (fgets (mLines + line * mMaxWidth, mMaxWidth, text)) {
        ++line;
    }
    strcpy (mLines + line * mMaxWidth, " --End-- ");
    /* Done! Now clean up. */
    fclose (text);
}

shTextViewer::shTextViewer (char *lines, int num, int max)
{   /* Import text from some other procedure. */
    mLines = lines;
    mNumLines = num;
    mMaxWidth = max;
    mFree = false;
}

shTextViewer::~shTextViewer (void)
{
    if (mFree)  free (mLines);
}

void /* Color change sequences: @X where X stands for a color code. */
shTextViewer::print (int winline, int fileline)
{
    int control = 0, idx = 0;
    char *line = mLines + fileline * mMaxWidth;
    I->winPrint (shInterface::kTemp, winline, 0, ""); /* Just to move cursor. */
    while (line[idx]) { /* Parse line. */
        if (control and !isupper (line[idx])) {
            I->winPutchar (shInterface::kTemp, '@');
            control = 0; /* Avoid choking on our email addresses. */
        }
        if (control) {
            I->setWinColor (shInterface::kTemp, shColor (line[idx] - 'A'));
            control = 0;
        } else if (line[idx] == '@') {
            control = 1;
        } else {
            I->winPutchar (shInterface::kTemp, line[idx]);
        }
        ++idx;
    }
}
     /* Displays loaded file to the screen. This procedure */
void /* should have no reason to modify member variables.  */
shTextViewer::show (bool bottom)
{
    int height = I->getMaxLines () - 1;
    int first = 0;
    if (bottom)  first = maxi (mNumLines - height, 0);
    I->newTempWin ();

    shInterface::SpecialKey sp;
    while (sp != shInterface::kEscape) {
        /* Show content. */
        I->clearWin (shInterface::kTemp);
        I->setWinColor (shInterface::kTemp, kGray);
        int lines_drawn = mini (mNumLines - first, height);
        for (int i = first; i < first + lines_drawn; ++i) {
            print (i - first, i);
        }
        /* Show help at bottom. */
        I->setWinColor (shInterface::kTemp, kWhite);
        I->winPrint (shInterface::kTemp, height, 0,
"    SPACE  ENTER  ESCAPE          ARROWS  PAGE UP  PAGE DOWN");
        I->setWinColor (shInterface::kTemp, kGray);
        I->winPrint (shInterface::kTemp, height, 9, ",");
        I->winPrint (shInterface::kTemp, height, 16, ",");
        I->winPrint (shInterface::kTemp, height, 24, " - exit");
        I->winPrint (shInterface::kTemp, height, 40, ",");
        I->winPrint (shInterface::kTemp, height, 49, ",");
        I->winPrint (shInterface::kTemp, height, 60, " - navigation");
        I->refreshWin (shInterface::kTemp);
        I->getSpecialChar (&sp);
        /* Navigation. */
        switch (sp) {
        case shInterface::kHome:
            first = 0;
            break;
        case shInterface::kEnd:
            first = mNumLines - height;
            break;
        case shInterface::kUpArrow:
            if (first > 0) --first;
            break;
        case shInterface::kDownArrow:
            if (first + height < mNumLines) ++first;
            break;
        case shInterface::kPgUp: case shInterface::kLeftArrow:
            first -= height / 2;
            if (first < 0) first = 0;
            break;
        case shInterface::kPgDn: case shInterface::kRightArrow:
            first += height / 2;
            if (first + height > mNumLines) first = mNumLines - height;
            break;
        case shInterface::kEnter: case shInterface::kSpace:
            sp = shInterface::kEscape;
            break;
        default: break;
        }
    }
    I->delTempWin ();
}
