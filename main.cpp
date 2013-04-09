#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <io.h>
#define access _access
#include "win32/getopts/getopt.h"
#else
#include <unistd.h>
#endif
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#ifndef _WIN32
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>
#endif

#ifdef OSX
#include "osx/macsupport.h"
#endif

#include "Global.h"
#include "Util.h"
#include "Map.h"
#include "Interface.h"
#include "Monster.h"
#include "Hero.h"
#include "Game.h"

shInterface *I;
shMapLevel *Level;
bool BOFH = false;
char UserDir[PRIME_PATH_LENGTH];
shLogFile debug;

extern shFlags Flags;
#ifndef NOGUI
void noteye_halt (void);
#endif

static void
usage ()
{
    const char usage[] =
    "PRIME: A science fiction themed roguelike game with all kinds of aliens.\n"
    "       http://arcywidmo.republika.pl/prime/\n"
    "\n"
#ifndef NOGUI
    "usage: prime [-u username] [-a] [-bofh] [-N option]\n"
    "\n"
    "  -a   Force playing in ASCII mode.\n"
#else
    "usage: prime [-u username] [-bofh] [-N option]\n"
    "\n"
#endif
    "  -u   Specify username to play as.\n"
    "  -N   Pass an option to NotEye frontend.\n"
    "-bofh  Activate Bastard Operator From Hell mode.\n";
    fprintf (stdout, "%s", usage);
}


#ifndef NOGUI
static bool noteye = true; /* Use NotEye by default. */
#endif
static const char *name = NULL;
static char namebuf[HERO_NAME_LENGTH+2];

int
main (int argc, char **argv)
{
    int rlimitresult = 0;
#ifndef _WIN32
	struct rlimit coredumpsize;

    coredumpsize.rlim_cur = RLIM_INFINITY;
    coredumpsize.rlim_max = RLIM_INFINITY;
    rlimitresult = setrlimit (RLIMIT_CORE, &coredumpsize);
#endif

    //FIXME: Here follows some ugly code interspersed with ifdefs.
    //TODO:  Create source file responsible for hiding portability issues.
    //FIXME: should probably stat() it and make sure it's a directory...
    if (access (SCOREDIR, O_RDWR)) {
        fprintf (stderr, "Couldn't open \"%s\" directory: %s.\n", SCOREDIR, strerror (errno));
        fprintf (stderr, "PRIME needs this directory for high scores and log.\n");
        exitPRIME (-1);
    }

    if (access (DATADIR, O_RDONLY)) {
        fprintf (stderr, "Couldn't open \"%s\" directory: %s.\n", DATADIR, strerror (errno));
        fprintf (stderr, "PRIME needs this directory for keybindings and help files.\n");
        exitPRIME (-1);
    }

#ifndef _WIN32
    if (USERDIR[0] == '~') { /* Insert user home directory. */
        const char *home = getenv ("HOME");
        if (!home) {
            fprintf (stderr, "HOME environment variable not present.\n");
            fprintf (stderr, "PRIME was configured to search for it and cannot run unless it is set.\n");
            exitPRIME (-1);
        }
        snprintf (UserDir, PRIME_PATH_LENGTH, "%s%s", home, &USERDIR[1]);
    } else {
        snprintf (UserDir, PRIME_PATH_LENGTH, "%s", USERDIR);
    }
#else
    snprintf (UserDir, PRIME_PATH_LENGTH, "%s", USERDIR);
#endif
#ifndef _WIN32
    if (access (UserDir, O_RDWR)) {
        mkdir (UserDir, S_IRUSR | S_IWUSR | S_IXUSR);
    }
#endif
    if (access (UserDir, O_RDWR)) {
        fprintf (stderr, "Couldn't open \"%s\" directory: %s.\n", UserDir, strerror (errno));
        fprintf (stderr, "PRIME needs this directory for saved games and user preferences.\n");
        exitPRIME (-1);
    }

    if (!debug.open ()) {
        exitPRIME (-1);
    }

    int bofhopt = 0;
    int c;
    while (-1 != (c = getopt (argc, argv, "u:N:abofh"))) {
        switch (c) {
#ifndef NOGUI
        case 'a': noteye = false; break;
#endif
        case 'b': if (bofhopt == 0) bofhopt = 1; else bofhopt = 5; break;
        case 'o': if (bofhopt == 1) bofhopt = 2; else bofhopt = 5; break;
        case 'f': if (bofhopt == 2) bofhopt = 3; else bofhopt = 5; break;
        case 'h': if (bofhopt == 3) bofhopt = 4; else bofhopt = 5; break;
        case 'u':
            name = strdup (optarg);
            if (!nameOK (name)) {
                fprintf (stderr, "Sorry, can't play a game as \"%s\".\n",
                         name);
                return -1;
            }
            break;
        case 'N': break; /* Ignore.  This option is for NotEye frontend. */
        case '?':
        default:
#ifdef OSX
            if (strncmp("-psn_0_", argv[1], 7) == 0) {
                break;
            }
#endif
            debug.log("usage:%s", argv[1]);
            usage ();
            return 0;
        }
    }
    if (bofhopt == 4)
        BOFH = true;

    utilInitialize ();
    

#ifndef NOGUI
    if (noteye) {
#ifdef OSX
        OSXMain();
#endif
        I = startNotEye (argc, argv);
    }
    else
#endif
        I = startNCurses ();
    if (rlimitresult) {
        debug.log ("Couldn't adjust coredumpsize.");
    }

    initializeProfessions ();
    initializeMonsters ();

    I->runMainLoop ();

    delete I;

    fprintf (stdout, "Thanks for playing!\n");
    fprintf (stdout, "Please submit bug reports at psiweapon@gmail.com or michal@kast.net.pl\n");
#ifndef NOGUI
    if (noteye) {
        noteye_halt ();
    }
#endif
    exitPRIME (0);
}

void
mainLoop (void)
{
    I->showVersion ();

    if (!name) {
        bool invalid = false;

        name = getenv ("USER");
        if (!name)
            name = getenv ("USERNAME");

        do {
            I->p ("Welcome to PRIME!     (hit esc to quit now)");
            if (invalid) I->p ("Invalid name!");
            if (!I->getStr (namebuf, HERO_NAME_LENGTH+1,
                           "What is your name?", name))
            {
                I->p ("Goodbye.");
                return;
            }
            I->pageLog ();
            invalid = true;
        } while (!nameOK (namebuf));
        name = namebuf;
    }

    I->loadOptions ();

    if (0 == loadGame (name) ||
        0 == newGame (name))
    {
        if (Flags.mKeySet[0] == '\0') I->keymapChoice ();
        I->p ("Welcome to \"PRIME\".  Press '?' or F1 for help.");
        I->p ("Please submit bug reports at: psiweapon@gmail.com, michal@kast.net.pl");
        if (BOFH) {
            I->p ("Bastard Operator From Hell mode is on.");
        }
        gameLoop ();
    }

    I->saveOptions ();
}
