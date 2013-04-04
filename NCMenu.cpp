/******************************************************

    Menu and viewer for NCurses user interface.
    High score table too for some reason.

******************************************************/

#include <stdio.h>
#include <ctype.h>
#include "Util.h"
#include "NCUI.h"
#include "Game.h"
#include "Menu.h"


shNCursesMenu::shNCursesMenu (const char *prompt, int flags)
    : shMenu (prompt, flags)
{
    getmaxyx(stdscr, mHeight, mWidth);
    if (!(mFlags & kNoPick)) {
        /* KLUDGE: avoid repeating a letter on the same page */
        mHeight = mini (52, mHeight);
    }
}


void
shNCursesMenu::showHelp (WINDOW *win)
{
    int lines;
    const char **text = prepareHelp (&lines);

    for (int i = 0; i < lines; ++i) {
        int len = strlen (text[i]);
        for (int j = 0; j < len; ++j) {
            if (islower (text[i][j]) or text[i][j] == ':' or
                (j == 0 and (text[i][j] == 'I' or text[i][j] == 'Q')))
            {
                wattrset (win, A_NORMAL);
            } else {
                wattrset (win, A_BOLD);
            }
            mvwaddnstr (win, i, j, &text[i][j], 1);
        }
    }
    free (text);
}


void
shNCursesMenu::accumulateResults ()
{
    const int map_end = 64; /* column map ends */
    WINDOW *win, *helpwin = NULL;
    PANEL *panel, *helppanel = NULL;

    int helplines;
    free (prepareHelp (&helplines));

    /* Two additional lines besides all the choices are the menu title
       and --End-- or --More-- at the bottom. */
    mItemHeight = mini (mHeight - helplines, mChoices.count () + 2);
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

    win = newwin (mItemHeight, width, 0, maxi (0, map_end - width) - gap);
    if (!win) {
        debug.log ("Unable to create window (%d, %d, %d, %d)",
                  mItemHeight, width, 0, maxi (0, map_end - width));
        I->p ("Uh oh!  Couldn't create window!!");
        mDone = 1;
        return;
    }
    keypad (win, TRUE);
    panel = new_panel (win);
    if (helplines) {
        helpwin = newwin (helplines, 80, mHeight - helplines, 0);
        if (!helpwin) {
            debug.log ("Unable to create help window (%d, %d, %d, %d)",
                      helplines, 80, mHeight - helplines, 0);
            I->p ("Uh oh!  Couldn't create help window!!");
            mDone = 1;
            return;
        }
        helppanel = new_panel (helpwin);
        showHelp (helpwin);
    }

    /* -2 lines to make space for header and --End-- or similar. */
    mLast = mini (mOffset + mItemHeight - 2, mChoices.count ());
    while (1) { /* Menu loop. */
        /* Menu header: */
        wattrset (win, A_BOLD);
        mvwaddnstr (win, 0, 1, mPrompt, width);
        wclrtoeol (win);

        wattrset (win, A_NORMAL);
        int i;
        for (i = mOffset; i < mLast; ++i) {
            /* First, clear line. */
            wmove (win, 1 + i - mOffset, 0);
            wclrtoeol (win);
            /* Then draw. */
            char buf[100];
            shMenuChoice *item = mChoices.get (i);

            if (item->mLetter >= 0 and (shMenu::kCategorizeObjects & mFlags)
                and ((shObject *) item->mValue.mPtr)->isKnownRadioactive ())
            {
                wattrset (win, ColorMap[kGreen]);
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
                if (kCategorizeObjects & mFlags and mFlags & kMultiPick) {
                    /* Get category header. */
                    char part[50];
                    snprintf (part, 50, " %s ", item->mText);
                    char *gap = strstr (part, "  "); /* Find gap. */
                    gap[1] = 0; /* Truncate. */
                    wattrset (win, A_REVERSE);
                    mvwaddnstr (win, 1 + i - mOffset, 1, part, width);
                    /* Get (toggle all with X) part. */
                    char *p2 = strstr (item->mText, "(t");
                    int len = strlen (p2);
                    snprintf (part, len-2, "%s", item->mText);
                    wattrset (win, ColorMap[kBlue]);
                    mvwaddnstr (win, 1 + i - mOffset, width-21, p2, width);
                    /* The toggle key should stand out. */
                    snprintf (buf, len - 1, "%c", p2[len - 2]);
                    mvwaddch (win, 1 + i - mOffset, width-21+len-2,
                              p2[len - 2] | ColorMap[kWhite]);
                    buf[0] = 0; /* Printing is done. */
                } else {
                    wattrset (win, A_REVERSE);
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
            mvwaddnstr (win, 1 + i - mOffset, 1, buf, width);
            wattrset (win, A_NORMAL);
        }
        mvwaddnstr (win, 1 + i - mOffset, 1, bottomLine (i), width);

        while (1) {
            if (helplines) {
                showHelp (helpwin);
                touchwin (helpwin);
            }
            update_panels ();
            doupdate();
            int key = I->getChar ();

            if (27 == key or 13 == key or ' ' == key) { /* done */
                mDone = 1; break;
            } else if (KEY_BACKSPACE == key) {
                interpretKey (0, shInterface::kDrop); break;
            } else if (KEY_HOME == key or KEY_A1 == key) {
                interpretKey (0, shInterface::kMoveNW); break;
            } else if (KEY_END == key or KEY_C1 == key) {
                interpretKey (0, shInterface::kMoveSW); break;
            } else if (KEY_UP == key) {
                interpretKey (0, shInterface::kMoveUp); break;
            } else if (KEY_DOWN == key) {
                interpretKey (0, shInterface::kMoveDown); break;
            } else if (KEY_PPAGE == key or KEY_LEFT == key or KEY_A3 == key) {
                interpretKey (0, shInterface::kMoveNE); break;
            } else if (KEY_NPAGE == key or KEY_RIGHT == key or KEY_C3 == key) {
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
    hide_panel (panel);
    del_panel (panel);
    delwin (win);
    if (helplines) {
        hide_panel (helppanel);
        del_panel (helppanel);
        delwin (helpwin);
    }
    update_panels ();
    I->drawScreen ();
}


shMenu *
shNCursesInterface::newMenu (const char *prompt, int flags) {
    return new shNCursesMenu (prompt, flags);
}
