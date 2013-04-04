#include <ctype.h>
#include "Util.h"
#include "NEUI.h"
#include "Game.h"

extern bool mapOn;

shNotEyeMenu::shNotEyeMenu (const char *prompt, int flags)
    : shMenu (prompt, flags)
{
    mWidth = 80;
    mHeight = 25;
}

void
shNotEyeMenu::showHelp ()
{
    int lines;
    const char **text = prepareHelp (&lines);

    for (int i = 0; i < lines; ++i) {
        int len = strlen (text[i]);
        for (int j = 0; j < len; ++j) {
            if (islower (text[i][j]) or text[i][j] == ':' or
                (j == 0 and (text[i][j] == 'I' or text[i][j] == 'Q')))
            {
                setTextAttr (7, 0);
            } else {
                setTextAttr (15, 0);
            }
            noteye_mvaddch (25 - lines + i, j, text[i][j]);
        }
    }
    free (text);
}


void
shNotEyeMenu::accumulateResults ()
{
    const int map_end = 64; /* column map ends */
    mapOn = false;

    int helplines;
    free (prepareHelp (&helplines));

    /* Two additional lines besides all the choices are the menu title
       and --End-- or --More-- at the bottom. */
    int height = mini (mHeight - helplines, mChoices.count () + 2);
    int width = 10;
    int gap = 0; /* Used to position the menu. */
    for (int i = 0; i < mChoices.count (); ++i) {
        width = maxi (width, strlen (mChoices.get (i)->mText) + 1);
    }
    if (!(mFlags & kNoPick)) width += 10; /* Adjust for "( ) x - " prompts. */
    /* Main header might be still longer. */
    width = maxi (width, strlen (mPrompt) + 2);
    if (mFlags & kCategorizeObjects) { /* Do not center item lists. */
        /* Determine whether window can leave sidebar unobscured. */
        if (width <= map_end) { /* Yes! */
            width = map_end;
        } else {                /* No, so hide it whole. */
            width = mWidth;
        }
    } else {
        width = mini (mWidth, width);
        if (width > map_end) { /* Would obscure side bar window? */
            width = mWidth;    /* Then cover it whole. */
        } else { /* Place small gap between sidebar and menu if possible. */
            gap = mini (10, (map_end - width) / 2);
        }
    }
    int startX = maxi (0, map_end - width) - gap; /* Column menu starts at. */

    noteye_erase ();

    /* -2 lines to make space for header and --End-- or similar. */
    mLast = mini (mOffset + height - 2, mChoices.count ());
    while (1) { /* Menu loop. */
        /* Menu header: */
        setTextAttr (15, 0);
        noteye_mvaddstr (0, startX + 1, mPrompt);
        noteye_clrtoeol ();

        setTextAttr (7, 0);
        int i;
        for (i = mOffset; i < mLast; ++i) {
            /* First, clear line. */
            noteye_move (1 + i - mOffset, startX);
            noteye_clrtoeol ();
            /* Then draw. */
            char buf[100];
            shMenuChoice *item = mChoices.get (i);

            if (item->mLetter >= 0 and (shMenu::kCategorizeObjects & mFlags)
                and ((shObject *) item->mValue.mPtr)->isKnownRadioactive ())
            {
                setTextAttr (2, 0);
            }
            if (-2 == item->mLetter) { /* This is a pretty delimiter. */
                int len = strlen (item->mText);
                int j = (width - len) / 2;
                char *spaces = GetBuf ();
                for (int i = 0; i < j; ++i) {
                    spaces[i] = ' ';
                }
                spaces[j] = '\0';
                snprintf (buf, 100, "%s%s%s", spaces, item->mText, spaces);
            } else if (-1 == item->mLetter) { /* This is a header entry. */
                if ((kCategorizeObjects & mFlags) and (mFlags & kMultiPick)) {
                    /* Get category header. */
                    char part[50];
                    snprintf (part, 50, " %s ", item->mText);
                    char *gap = strstr (part, "  "); /* Find gap. */
                    gap[1] = 0; /* Truncate. */
                    setTextAttr (0, 7);
                    noteye_mvaddstr (1 + i - mOffset, startX + 1, part);
                    /* Get (toggle all with X) part. */
                    char *p2 = strstr (item->mText, "(t");
                    int len = strlen (p2);
                    snprintf (part, len-2, "%s", item->mText);
                    setTextAttr (1, 0);
                    noteye_mvaddstr (1 + i - mOffset, startX + width-21, p2);
                    /* The toggle key should stand out. */
                    snprintf (buf, len - 1, "%c", p2[len - 2]);
                    setTextAttr (15, 0);
                    noteye_mvaddch (1 + i - mOffset, startX + width-21+len-2, p2[len - 2]);
                    buf[0] = 0; /* Printing is done. */
                } else {
                    setTextAttr (0, 7);
                    snprintf (buf, 100, " %s ", item->mText);
                }
            } else if (mFlags & kNoPick) {
                if (' ' == item->mLetter) {
                    snprintf (buf, 100, "%s", item->mText);
                } else {
                    snprintf (buf, 100, "%c - %s", item->mLetter, item->mText);
                }
            } else {
                if (' ' == item->mLetter) {
                    snprintf (buf, 100, "        %s", item->mText);
                } else if (mFlags & kShowCount) {
                    if (item->mSelected) {
                        snprintf (buf, 100, "(%d) %c - %s",
                            item->mSelected, item->mLetter, item->mText);
                    } else {
                        snprintf (buf, 100, "( ) %c - %s",
                            item->mLetter, item->mText);
                    }
                } else {
                    snprintf (buf, 100, "(%c) %c - %s",
                              0 == item->mSelected ? ' ' :
                              item->mCount == item->mSelected ? 'X' : '#',
                              item->mLetter, item->mText);
                }
            }
            noteye_mvaddstr (1 + i - mOffset, startX + 1, buf);
            setTextAttr (7, 0);
        }
        noteye_mvaddstr (1 + i - mOffset, startX + 1, bottomLine (i));
        if (helplines)  showHelp ();
        if (width <= map_end)  I->drawSideWin ();

        while (1) {
            int key = I->getChar ();

            if (27 == key or 10 == key or 13 == key or ' ' == key) { /* done */
                mDone = 1; break;
            } else if (8 == key) {
                interpretKey (0, shInterface::kDrop); break;
            } else if (D_HOME == key) {
                interpretKey (0, shInterface::kMoveNW); break;
            } else if (D_END == key) {
                interpretKey (0, shInterface::kMoveSW); break;
            } else if (D_UP == key) {
                interpretKey (0, shInterface::kMoveUp); break;
            } else if (D_DOWN == key) {
                interpretKey (0, shInterface::kMoveDown); break;
            } else if (D_PGUP == key or D_LEFT == key) {
                interpretKey (0, shInterface::kMoveNE); break;
            } else if (D_PGDN == key or D_RIGHT == key) {
                interpretKey (0, shInterface::kMoveSE); break;
            } else if ('\t' == key and mHelpFileName) { /* invoke help */
                interpretKey (0, shInterface::kHelp); break;
            } else if (mFlags & kNoPick) {
                continue;
            } else {
                if (interpretKey (key)) break;
            }
            if (mDone) break;
        }
        if (mDone) break;
    }
    /* Clean up. */
    noteye_erase ();
    mapOn = true;
    I->drawScreen ();
}




shMenu *
shNotEyeInterface::newMenu (const char *prompt, int flags) {
    return new shNotEyeMenu (prompt, flags);
}
