/*  It was named God Mode commands before.  However, god mode is suitable
 *  for FPP games.  Obviously PRIME is not one of them.  NetHack uses wizard
 *  mode fits much better.  Developers needed something more compatible with
 *  science fiction setting.  Hence Bastard Operator From Hell mode.
 */

#include "Global.h"
#include "Hero.h"
#include "Mutant.h"

void /* FIXME: XXX: BROKEN! */
showRooms ()
{
    for (int y = 0; y < MAPMAXROWS; y++) {
        for (int x = 0; x < MAPMAXCOLUMNS; x++) {
            int c = ' ';
            if (!Level->isInRoom (x, y)) {
                continue;
            }
            int room = Level->getRoomID (x, y);
            if (room < 10) {
                c = '0' + room;
            } else if (room < 36) {
                c = 'a' + room - 10;
            } else if (room < 62) {
                c = 'A' + room - 36;
            } else {
                c = '*';
            }

            if (Level->stairsOK (x, y))
                c |= ColorMap[kPink];

            Level->drawSq (x, y);
        }
    }
    I->pauseXY (Hero.mX, Hero.mY);
}

void
switchDarkness ()
{
    int room = Level->getRoomID (Hero.mX, Hero.mY);
    int sx, sy, ex, ey;
    if (room == 0) return;  /* We're in corridor or something. */
    Level->getRoomDimensions (room, &sx, &sy, &ex, &ey);
    /* Switch darkness. */
    if (Level->isLit (Hero.mX, Hero.mY, Hero.mX, Hero.mY)) {
        Level->darkRoom (sx, sy, ex, ey);
    } else {
        for (int y = sy; y <= ey; ++y) {
            for (int x = sx; x <= ex; ++x) {
                Level->setLit (x, y, 1, 1, 1, 1);
            }
        }
    }
}

static shFeature *
buildDoor (const int x, const int y)
{
    shFeature *door = shFeature::newDoor ();
    shMenu *doorFeatures = I->newMenu ("Please select door features:", shMenu::kMultiPick);
    doorFeatures->addIntItem ('c', "closed", 0xFFFF);
    doorFeatures->addIntItem ('a', "automatic opening", shFeature::kAutomatic);
    doorFeatures->addIntItem ('i', "inverted logic", shFeature::kInverted);
    doorFeatures->addIntItem ('m', "malfunction", shFeature::kBerserk);
    doorFeatures->addIntItem ('u', "unknown malfunction", 0xFFFE);
    doorFeatures->addIntItem ('l', "locked", shFeature::kLocked);
    doorFeatures->addIntItem ('b', "broken lock", shFeature::kLockBroken);
    doorFeatures->addIntItem ('f', "force field", shFeature::kMagneticallySealed);
    doorFeatures->addIntItem ('s', "alarm system", shFeature::kAlarmed);
    doorFeatures->addIntItem ('h', "horizontal", shFeature::kHorizontal);
    doorFeatures->addIntItem ('S', "retina scanner", shFeature::kLockRetina);
    doorFeatures->addIntItem ('A', "lock color: A", shFeature::kLock1);
    doorFeatures->addIntItem ('B', "lock color: B", shFeature::kLock2);
    doorFeatures->addIntItem ('C', "lock color: C", shFeature::kLock3);
    doorFeatures->addIntItem ('D', "lock color: D", shFeature::kLock4);
    doorFeatures->addIntItem ('M', "lock color: Master", shFeature::kLockMaster);

    int result, flags = 0;
    while (doorFeatures->getIntResult (&result)) {
        if (result == 0xFFFF) {
            door->mType = shFeature::kDoorClosed;
        } else if (result == 0xFFFE) {
            door->mTrapUnknown = 1;
        } else {
            flags |= result;
        }
    }
    delete doorFeatures;
    door->mDoor.mFlags = flags;
    door->mX = x;
    door->mY = y;
    return door;
}

void
shHero::useBOFHPower ()
{
    shMenu *menu = I->newMenu ("[~]# _", 0);
    int choice;

    menu->addIntItem ('b', "buff up", 8);
    menu->addIntItem ('d', "diagnostics", 12);
    menu->addIntItem ('i', "identify item", 1);
    menu->addIntItem ('l', "gain level", 2);
    menu->addIntItem ('h', "fully heal", 18);
    menu->addIntItem ('f', "create feature", 10);
    menu->addIntItem ('m', "create monster", 3);
    menu->addIntItem ('o', "create object", 4);
    menu->addIntItem ('r', "reveal map", 5);
    menu->addIntItem ('s', "monster spoilers", 6);
    menu->addIntItem ('p', "probability tables", 17);
    menu->addIntItem ('t', "transport", 7);
    menu->addIntItem ('T', "level transport", 9);
    menu->addIntItem ('e', "excavate", 19);
    menu->addIntItem ('R', "show RoomIDs", 11);
    menu->addIntItem ('D', "switch room darkness", 16);
    menu->addIntItem ('M', "choose mutations", 15);
    // TODO: Collapse both flag manipulation choices into single option.
    menu->addIntItem ('F', "check story flag", 13);
    menu->addIntItem ('S', "alter story flag", 14);
    menu->addIntItem ('v', "toggle invisibility", 20);

    menu->getIntResult (&choice);
    delete menu;

    if (1 == choice) {
        identifyObjects (-1, 0);
    } else if (2 == choice) {
        mXP += 1000;
        levelUp ();
    } else if (3 == choice) {
        char desc[100];
        shMonsterIlk *ilk = NULL;
        shMonster *monster;
        int x = mX;
        int y = mY;

        I->getStr (desc, 80, "Create what kind of monster?");
        for (int i = 0; i < kMonNumberOf; ++i) {
            if (0 == strcasecmp (MonIlks[i].mName, desc)) {
                ilk = &MonIlks[i];
                break;
            }
        }
        if (NULL == ilk) {
            I->p ("%s not found in monster list.", desc);
            return;
        }
        Level->findNearbyUnoccupiedSquare (&x, &y);
        if (0 == I->getSquare ("Where should it spawn? (select a location)",
                               &x, &y, -1))
        {
            return;
        }
        monster = new shMonster (ilk->mId);
        if (0 != Level->putCreature (monster, x, y)) {
            //FIXME: might not have been deleted
            //delete monster;
        }
    } else if (4 == choice) {
        char desc[100];
        shObject *obj;
        I->getStr (desc, 80, "Create what object?");
        obj = createObject (desc, 0);
        if (obj) {
            if (!Hero.isBlind ()) {
                obj->setAppearanceKnown ();
            }
            if (0 == addObjectToInventory (obj)) {
                I->p ("The object slips from your hands!");
                Level->putObject (obj, mX, mY);
            }
        } else {
            I->p ("%s not found in object list.", desc);
        }
    } else if (5 == choice) {
        Level->reveal (0);
        for (int x = 0; x < MAPMAXCOLUMNS; ++x) {
            for (int y = 0; y < MAPMAXROWS; ++y) {
                Level->mVisibility[x][y] = 1;
                Level->setLit (x, y);
            }
        }
        I->drawScreen ();
        I->p ("Map revealed.");
    } else if (6 == choice) {
        monsterSpoilers ();
    } else if (7 == choice) {
        int x = -1, y = -1;
        if (I->getSquare ("Transport to what location?", &x, &y, -1)) {
            Hero.transport (x, y, 100, 1, 1);
        }
    } else if (8 == choice) {
        Hero.mMaxHP += 100;
        Hero.mHP = Hero.mMaxHP;
        addObjectToInventory (
            createObject ("optimized (20 charges) disintegration ray gun", 0));
    } else if (9 == choice) {
        shMenu *lmenu = I->newMenu ("Warp to what level?", 0);
        shMapLevel *L;
        for (int i = 1; i < Maze.count (); i++) {
            L = Maze.get (i);
            lmenu->addPtrItem (i <= 26 ? i + 'a' - 1 : i + 'A' - 27,
                               L->mName, L, 1);
        }
        lmenu->getPtrResult ((const void **) &L);
        delete lmenu;
        if (L) {
            Level->warpCreature (&Hero, L);
        }
    } else if (10 == choice) {
        shFeature::Type t;
        int x = Hero.mX, y = Hero.mY;
        shMenu *fmenu = I->newMenu ("Create what kind of feature?", 0);
        fmenu->addIntItem ('a', "acid pit trap", shFeature::kAcidPit, 1);
        fmenu->addIntItem ('p', "pit trap", shFeature::kPit, 1);
        fmenu->addIntItem ('s', "sewage pit trap", shFeature::kSewagePit, 1);
        fmenu->addIntItem ('r', "radiation trap", shFeature::kRadTrap, 1);
        fmenu->addIntItem ('h', "hole", shFeature::kHole, 1);
        fmenu->addIntItem ('t', "trap door", shFeature::kTrapDoor, 1);
        fmenu->addIntItem ('v', "vat of sludge", shFeature::kVat, 1);
        fmenu->addIntItem ('d', "door", shFeature::kDoorOpen, 1);
        fmenu->getIntResult ((int *) &t);
        delete fmenu;
        if (!t) {
            return;
        }
        if (0 == I->getSquare ("Put it where? (select a location)",
                               &x, &y, -1))
        {
            return;
        }
        switch (t) {
            case shFeature::kVat:
                Level->addVat (x, y);
                break;
            case shFeature::kDoorOpen:
                Level->addFeature (buildDoor (x, y));
                break;
            default:
                Level->addTrap (x, y, t);
                break;
        }
    } else if (11 == choice) {
        showRooms ();
        return;
    } else if (12 == choice) {
        shCreature *c;
        int x = Hero.mX, y = Hero.mY;
        if (I->getSquare ("Diagnose whom?", &x, &y, -1)) {
            if ((c = Level->getCreature (x, y))) {
                c->doDiagnostics (0);
            } else {
                I->p ("There is no one here.");
            }
        }
        return;
    } else if (13 == choice) {
        char name[50];
        int val;
        I->getStr (name, 30, "Check what story flag?");
        val = Hero.getStoryFlag(name);
        if (!val) I->p ("Such story flag has not been set.");
        else      I->p ("Flag has value %d.", val);
    } else if (14 == choice) {
        char name[50], number[15];
        int val;
        I->getStr (name, 30, "Alter what story flag?");
        if (strlen (name) == 0) {
            I->p ("Cancelled.");
            return;
        }
        I->getStr (number, 10, "To what value?");
        int read = sscanf(number, "%d", &val);
        if (strlen (number) == 0 or !read) {
            I->p ("Cancelled.");
            return;
        }
        Hero.setStoryFlag(name, val);
        I->p ("Flag set.");
    } else if (15 == choice) {
        shMenu *fmenu = I->newMenu ("Toggle mutations:", shMenu::kMultiPick);
        char letter = 'a';
        for (int power = kNoMutantPower; power <= kMaxHeroPower; ++power)
            /* list implemented but not activated persistent powers */
            if (MutantPowers[power].mFunc and Hero.mMutantPowers[power] != 2) {
                fmenu->addIntItem (letter++, MutantPowers[power].mName,
                    power, 1, Hero.mMutantPowers[power] > 0);
                /* remove by default */
                Hero.mMutantPowers[power] = 0;
            }
        int input;
        do {
            int index;
            input = fmenu->getIntResult (&index);
            if (index)
            {
                Hero.mMutantPowers[index] = 1;
            }
        } while (input);
        delete fmenu;
    } else if (16 == choice) {
        switchDarkness ();
    } else if (17 == choice) {
        itemSpoilers ();
    } else if (18 == choice) {
        Hero.fullHealing (100, 0);
    } else if (19 == choice) {
        int x = Hero.mX, y = Hero.mY;
        if (!I->getSquare ("Where? (select a location)", &x, &y, -1)) return;
        Level->dig (x, y);
    } else if (20 == choice) {
        Hero.mInnateIntrinsics ^= kInvisible;
        Hero.computeIntrinsics ();
    }
    I->drawScreen ();
}
