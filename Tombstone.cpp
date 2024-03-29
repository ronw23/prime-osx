#include <ctype.h>

#include "Global.h"
#include "Util.h"
#include "Map.h"
#include "NCUI.h" // argh, but Hero::tomb uses it
#include "Hero.h"
#include "Mutant.h"
#include "Game.h"

#include <stdio.h>
#include <time.h>

#ifndef _WIN32
#include <unistd.h>
#endif

extern int GameOver;
/* calculation of score:

1 point per XP earned
500 points per level visited besides level 1
10000 for finding the bizarro orgasmatron

*/

static int
copystr (char *dest, char *src, int len)
{
    int i;
    for (i = 0; i < len; i++) {
        dest[i] = src[i];
        if (0 == src[i]) {
            return -1;
        }
    }
    while (i > 0) {
        --i;
        if (isspace (src[i])) {
            dest[i] = 0;
            return i+1;
        }
    }
    dest[i] = 0;
    return len;
}

int
shHero::tallyScore ()
{
    return mScore;
}


const char *
shMapLevel::getDescription ()
{
    switch (Level->mType) {
    case shMapLevel::kTown: return "in Robot Town";
    case shMapLevel::kRadiationCave: return "in the Gamma Caves";
    case shMapLevel::kMainframe: return "in the Mainframe";
    case shMapLevel::kRabbit: return "in the Rabbit's Hole";
    case shMapLevel::kSewer: return "in the sewer";
    case shMapLevel::kSewerPlant:
        return "in the waste treatment plant";
    default:
        return "in the space base";
    }
}


void
shHero::epitaph (char *buf, int len, shCauseOfDeath how, const char *killstr,
    shCreature *killer)
{
    switch (how) {
    case kSlain:
    case kKilled:
        if (killer) switch (killer->mIlkId) {
        case kMonDalek:
            snprintf (buf, len, "Exterminated by a dalek");
            break;
        case kMonManEatingPlant:
            snprintf (buf, len, "Devoured by a man eating plant");
            break;
        default:
            snprintf (buf, len, "Killed by %s", killer->an ());
            break;
        } else {
            snprintf (buf, len, "Killed by %s", killstr ? killstr : "a monster");
        }
        break;
    case kAnnihilated:
        snprintf (buf, len, "Annihilated by %s",
                  killstr ? killstr : "a monster");
        break;
    case kSuddenDecompression:
        snprintf (buf, len, "Died of vaccuum exposure");
        break;
    case kTransporterAccident:
        snprintf (buf, len, "Died in a transporter accident");
        break;
    case kSuicide:
        snprintf (buf, len, "Committed suicide");
        break;
    case kDrowned:
        snprintf (buf, len, "Drowned %s", killstr);
        break;
    case kBrainJarred:
        snprintf (buf, len, "Brain-napped by %s", killstr);
        break;
    case kMisc:
        snprintf (buf, len, "%s", killstr);
        break;
    case kQuitGame:
        snprintf (buf, len, "Quit");
        break;
    case kWonGame:
        snprintf (buf, len, "Activated the Bizarro Orgasmatron");
        break;
    }

    if (Hero.is (kAsleep) or Hero.is (kParalyzed)) {
        snprintf (&buf[strlen (buf)], len, " while helpless");
    }
    snprintf (&buf[strlen (buf)], len, " %s at depth %d.",
              Level->getDescription (), Level->mDLevel);
    if (killer and killer->mState == kDead and killer != &Hero and
        killer->mHowDead != kSuicide)
    {
        const char *insult;
        if (killer->mType == kHumanoid) {
            insult = killer->mGender == kFemale ? "bitch" : "bastard";
        } else {
            insult = "foul creature";
        }
        const char *he, *him;
        if (RNG (2)) {
            he = "He";   him = "him";
        } else {
            he = "She";  him = "her";
        }
        snprintf (&buf[strlen (buf)], len, " %s took the %s with %s.",
            he, insult, him);
    }
}



void
shHero::tomb (char *message)
{
    char buf1[80];
    char buf2[80];
    int n, l;
    int row;
    WINDOW *win;
    PANEL *panel;

    win = newwin (25, 60, 0, 0);
    if (!win) {
        I->p ("Couldn't create tombstone.");
        return;
    }
    panel = new_panel (win);

    wattrset (win, ColorMap[kAqua]);

    row = 3;

    mvwaddstr (win, row++, 6, "            _________            ");
    mvwaddstr (win, row++, 6, "           /         \\           ");
    mvwaddstr (win, row++, 6, "          /  _     _  \\          ");
    mvwaddstr (win, row++, 6, "         /  | \\ | | \\  \\         ");
    mvwaddstr (win, row++, 6, "        /   |_/ | |_/   \\        ");
    mvwaddstr (win, row++, 6, "       /    | \\ | |      \\       ");
    mvwaddstr (win, row++, 6, "      |                   |      ");
    mvwaddstr (win, row++, 6, "      |                   |      ");
    mvwaddstr (win, row++, 6, "      |                   |      ");
    mvwaddstr (win, row++, 6, "      |                   |      ");
    mvwaddstr (win, row++, 6, "      |                   |      ");
    mvwaddstr (win, row++, 6, "      |                   |      ");
    mvwaddstr (win, row++, 6, "      |                   |      ");
    mvwaddstr (win, row++, 6, "      |                   |      ");
    mvwaddstr (win, row++, 6, "      |                   |      ");
    mvwaddstr (win, row++, 6, "      |                   |      ");
    mvwaddstr (win, row++, 2,
               "----------+-------------------+--------------------------");

    row -= 10;

    wattrset (win, ColorMap[kWhite]);
    snprintf (buf1, 20, "%s", mName);
    mvwaddstr (win, row, 22 - strlen (buf1) / 2, buf1);

    snprintf (buf1, 18, "%s", getTitle ());
    wattrset (win, ColorMap[kGray]);
    mvwaddstr (win, ++row, 22 - strlen (buf1) / 2, buf1);

    row += 2;
    n = 0;
    do {
        l = copystr (buf2, &message[n], 18);
        n += l;
        mvwaddstr (win, row, 22 - strlen (buf2) / 2, buf2);
        row++;
    } while (l >= 0);

    wmove (win, 0, 0);

    update_panels ();
    doupdate();
    I->getChar ();
    hide_panel (panel);
    del_panel (panel);
    delwin (win);
    update_panels ();
    I->drawScreen ();
}


void
readHighScoreTable (HighScoreEntry scores[NUMHIGHSCORES], int *nscores, int *entry)
{
    char filename[PRIME_PATH_LENGTH];
    snprintf (filename, sizeof(filename)-1, "%s/highscores.dat", SCOREDIR);
    FILE *fd = fopen (filename, "r");

    *nscores = 0;
    *entry = 0;
    if (fd) {
        bool reading_second_line = false;
        const int SIZE = 250;
        char buf[SIZE];
        while (fgets (buf, SIZE, fd)) {
            if (!reading_second_line) {
                sscanf (buf, "%d %d %[^\n]", &scores[*nscores].mScore,
                    &scores[*nscores].mUid, scores[*nscores].mName);
            } else {
                sscanf (buf, "%[^\n]", scores[*nscores].mMessage);
                if (Hero.tallyScore () < scores[*nscores].mScore) {
                    *entry = *nscores + 1;
                }
                ++(*nscores);
            }
            reading_second_line = !reading_second_line;
        }
        fclose (fd);
    } else {
        I->p ("The high score table is corrupt and will be made anew.");
    }
    /* Otherwise a new high score table is created. */
}


void
shInterface::showHighScores ()
{ /* Read the high score data. */
    HighScoreEntry score[NUMHIGHSCORES+1];
    int nscores = 0, entry = 0;
    readHighScoreTable (score, &nscores, &entry);

    if (!nscores) {
        p ("The high score table is empty.");
        return;
    }
    const int LINELEN = 80;

    char *lines = (char *) calloc ((nscores * 3 + 5) * LINELEN, sizeof (char));
    int row = 0;
    if (GameOver and !BOFH) {
        if (NUMHIGHSCORES == entry) {
            strcpy (&lines[row++ * LINELEN], "You didn't make the high score list.");
        } else {
            strcpy (&lines[row++ * LINELEN], "You made the high score list!");
        }
        lines[row++ * LINELEN] = 0;
    }

    if (BOFH) entry = NUMHIGHSCORES; /* Disable highlight. */
    for (int i = 0; i < nscores; ++i) {
        char buf[200] = ""; /* Full message.  Will be split. */
        char buf2[78] = ""; /* Message part. */
        snprintf (buf, 200, "%s %3d. %8d ", /* 17 characters total, 15 shown. */
                  entry == i ? "@I" : "@H", /* Highlight score in bright red. */
                  i + 1, score[i].mScore);
        strcpy (&lines[row * LINELEN], buf);
        snprintf (buf, 200, "%s, %s@H", score[i].mName, score[i].mMessage);
        /* Move part of message to second line if needed. */
        int n = 0, l = -1;
        bool clear = false;
        do {
            l = copystr (buf2, &buf[n], 60);
            n += l;
            if (clear) { /* 15 spaces preceding non-first lines of epitaph. */
                strcpy (&lines[row * LINELEN], "               ");
            }
            strcpy (&lines[row++ * LINELEN + 15 + (clear ? 0 : 2)], buf2);
            clear = true;
        } while (l >= 0);
        lines[++row * LINELEN] = 0;
    }

    shTextViewer *hiscore = new shTextViewer (lines, row, LINELEN);
    hiscore->show ();
    delete hiscore;
    free (lines);
}


void
shHero::logGame (char *message)
{
    char logname[PRIME_PATH_LENGTH];
    time_t clocktime;
    struct tm *when;

    time (&clocktime);
    when = localtime (&clocktime);

    snprintf (logname, sizeof(logname)-1, "%s/logfile.txt", SCOREDIR);

    FILE *fd = fopen (logname, "a");

    if (!fd) {
        I->p ("Couldn't open logfile.");
        I->pause ();
    } else {
        fprintf (fd, "%s %04d-%02d-%02d %10d %2d %6d %s the %s %s%s\n",
                  PRIME_VERSION,
                  when->tm_year + 1900, when->tm_mon + 1, when->tm_mday,
                  mScore, mCLevel, mXP, mName,
                  getTitle (),
                  BOFH? "(BOFH) " : "", message);
        fclose (fd);
    }

    /* check high score file */

    if (BOFH) {
        I->p ("As BOFH, you are ineligible for the highscore list.");
        return;
    }

    HighScoreEntry scores[NUMHIGHSCORES+1];
    int nscores = 0;
    int entry = 0;

    readHighScoreTable (scores, &nscores, &entry);

    if (NUMHIGHSCORES == entry) {
        /* didn't make a high score */
        scores[entry].mScore = mScore;
#ifdef _WIN32
        scores[entry].mUid = 0;
#else
        scores[entry].mUid = getuid ();
#endif
        strncpy (scores[entry].mName, mName, 30);
        strncpy (scores[entry].mMessage, message, 200);
    } else {
        if (++nscores > NUMHIGHSCORES) {
            nscores = NUMHIGHSCORES;
        }
        for (int i = nscores - 1; i > entry; --i) {
            memcpy (&scores[i], &scores[i-1], sizeof (HighScoreEntry));
        }
        scores[entry].mScore = mScore;
#ifndef _WIN32
        scores[entry].mUid = getuid ();
#else
        scores[entry].mUid = 0;
#endif
        strncpy (scores[entry].mName, mName, 30);
        strncpy (scores[entry].mMessage, message, 200);

        char filename[PRIME_PATH_LENGTH];
        snprintf (filename, sizeof(filename)-1, "%s/highscores.dat", SCOREDIR);
        FILE *fd = fopen (filename, "w");
        if (!fd) {
            I->p ("Couldn't write high score file.");
            return;
        }
        for (int i = 0; i < nscores; ++i) {
            int prt = fprintf (fd, "%d %d %s\n%s\n", scores[i].mScore, scores[i].mUid,
            scores[i].mName, scores[i].mMessage);
            if (prt < 0) {
                I->p ("Error writing high score file.");
                break;
            }
        }
        fclose (fd);
    }
}

void
shHero::postMortem ()
{
    char mortemname[PRIME_PATH_LENGTH];

    snprintf (mortemname, sizeof(mortemname)-1, "%s/mortem.txt", UserDir);

    const int SIZE = 80;
    char buf[SIZE];
    FILE *mortem = fopen (mortemname, "w");
    if (!mortem) {
        I->p ("Couldn't open mortem file.");
        I->pause ();
        // TODO: ask for alternate path and filename?
        return;
    }

    I->doScreenShot (mortem);

    if (Hero.mInventory->count ()) {
        fprintf (mortem, "\n--- INVENTORY ---\n\n");
    }
    shObjectType lastType = kUninitialized;
    shObject *obj;
    int n = Hero.mInventory->count ();
    for (int i = 0; i < n; ++i) {
        obj = Hero.mInventory->get (i);
        if (obj->apparent ()->mType != lastType) { /* Insert header. */
            lastType = obj->apparent ()->mType;
            fprintf (mortem, "  %s\n", objectTypeHeader[lastType]);
        }
        obj->identify ();
        fprintf (mortem, "%c - %s\n", obj->mLetter, obj->inv ());
    }

    int powers = 0;
    for (int i = kNoMutantPower; i < kMaxHeroPower; ++i) {
        if (mMutantPowers[i]) {
            if (powers == 0) {
                fprintf (mortem, "\n--- MUTANT POWERS ---\n\n");
                fprintf (mortem, "  Power            Level Chance Status\n");
                powers = 1;
            }
            fprintf (mortem, "%-18s  %2d    %3d%%   %s\n",
                MutantPowers[i].mName, MutantPowers[i].mLevel,
                getMutantPowerChance (i),
                mMutantPowers[i] > 1 ? "(on)" : "");
        }
    }

    int skills = 0;
    shSkillCode lastcode = kNoSkillCode;
    n = mSkills.count ();
    for (int i = 0; i < n; ++i) {
        shSkill *skill = mSkills.get (i);
        if (skill->mAccess == 0 or (skill->getValue () == 0)) continue;
        if (!skills) {
            fprintf (mortem, "\n--- SKILLS ---\n\n");
        }
        ++skills;
        if (lastcode != kMutantPower and skill->mCode == kMutantPower) {
            fprintf (mortem, "Mutant Power Skills\n");
        } else if (!(lastcode & kWeaponSkill) and
                   (skill->mCode & kWeaponSkill))
        {
            fprintf (mortem, "Combat Skills\n");
        } else if (!(lastcode & kAdventureSkill) and
                    (skill->mCode & kAdventureSkill))
        {
            fprintf (mortem, "Adventuring Skills\n");
        }
        skill->getDesc (buf, SIZE);
        fprintf (mortem, "%s\n", buf);
        lastcode = skill->mCode;
    }
    if (mSkillPoints) {
        fprintf (mortem, "\nYou had %d unused skill point%s.\n",
            mSkillPoints, mSkillPoints > 1 ? "s" : "");
    }

    fprintf (mortem, "\n--- DIAGNOSTICS ---\n\n");
    Hero.doDiagnostics (0, 0, mortem);

    fprintf (mortem, "\n--- KILL LIST ---\n\n");
    for (int i = 0; i < kMonNumberOf; ++i) {
        shMonsterIlk *ilk = &MonIlks[i];
        if (ilk->mKills) {
            if (ilk->mFeats & kUniqueMonster) {
                snprintf (buf, SIZE, "the %s\n", ilk->mName);
            } else {
                /* Need another buffer just for monster name because
                   number of slain creatures would confuse makePlural. */
                char monname[SIZE];
                snprintf (monname, SIZE, "%s", ilk->mName);
                if (ilk->mKills > 1) {
                    makePlural (monname, SIZE);
                }
                snprintf (buf, SIZE, "%d %s\n", ilk->mKills, monname);
            }
            fprintf (mortem, buf);
        }
    }

    fprintf (mortem, "\n--- SUMMARY ---\n\n");
    time_t clocktime;
    time (&clocktime);
    struct tm *when = localtime (&clocktime);

    fprintf (mortem, "%s finished adventuring on %d-%d-%d.\n",
        mName, when->tm_year + 1900, when->tm_mon + 1, when->tm_mday);
    fprintf (mortem, "%s earned %d experience points and achieved level %d.\n",
        mName, mXP, mCLevel);
    fprintf (mortem, "%s attained the professional rank of %s.\n",
        mName, getTitle ());
    if (BOFH) {
        fprintf (mortem, "Games in BOFH mode are not scored.  "
						 "%d score points go down the drain.\n", mScore);
    } else {
        fprintf (mortem, "%s collected %d score points.\n", mName, mScore);
    }
    fclose (mortem);
}
