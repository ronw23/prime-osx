#include <math.h>
#include "Global.h"
#include "Util.h"
#include "Hero.h"
#include "Profession.h"
#include "Creature.h"
#include "Interface.h"
#include "Game.h"

#include <ctype.h>

const char *
shHero::getDescription ()
{
    return "hero";
}


const char *
shHero::the ()
{
    return "you";
}


const char *
shHero::an ()
{
    return "you";
}


const char *
shHero::your ()
{
    return "your";
}


const char *
shHero::herself ()
{
    return "yourself";
}


void
shHero::earnScore (int points)
{
    mScore += points;
}


void
shHero::earnXP (int challenge)
{
    int old = mXP / 1000;
    int amount = challenge - mCLevel;

    if (-1 == challenge) {
        mXP = old * 1000 + 1000;
        levelUp ();
        return;
    }

    amount = (int) (125.0 * pow (2.0, (double) amount / 2.0));

    earnScore (amount);
    mXP += amount;

    if (mXP / 1000 >= mCLevel) {
        if (mXP / 1000 > mCLevel) {
            /* don't allow multiple level gain */
            mXP = mCLevel * 1000;
        }
        levelUp ();
    }
}

static int pickedupitem;


void
shHero::oldLocation (int newX, int newY, shMapLevel *newLevel)
{
    if (GameOver)
        return;

    shRoom *oldroom = Level->getRoom (mX, mY);
    shRoom *newroom = Level->getRoom (newX, newY);

    if (oldroom == newroom)
        return;

    if (mLevel->isInShop (mX, mY)) {
        leaveShop ();
    }
    if (Level->isInHospital (mX, mY)) {
        leaveHospital ();
    }
    if (Level->isInGarbageCompactor (mX, mY)) {
        leaveCompactor ();
    }
}


void
shHero::newLocation ()
{
    if (!mLastLevel) mLastLevel = Level;
    shRoom *oldroom = mLastLevel->getRoom (mLastX, mLastY);
    shRoom *newroom = Level->getRoom (mX, mY);

//    if (mLastLevel != Level or distance (this, mLastX, mLastY) > 10)
    Level->computeVisibility ();

    I->drawScreen ();

    if (sewerSmells () and !is (kSickened)) {
        I->p ("What a terrible smell!");
        inflict (kSickened, 2000);
    }

    if (newroom != oldroom) {
        if (Level->isInShop (mX, mY)) {
            enterShop ();
        }
        if (Level->isInHospital (mX, mY)) {
            enterHospital ();
        }
        if (Level->isInGarbageCompactor (mX, mY)) {
            enterCompactor ();
        }

        if (newroom->mType == shRoom::kNest and !isBlind ()) {
            I->p ("You enter an alien nest!");
            /* Avoid giving this message again. */
            newroom->mType = shRoom::kNormal;
        }
    }

    pickedupitem = 0;

    shObjectVector *v = Level->getObjects (mX, mY);
    if (v) {
        int n = v->count ();
        for (int i = 0; i < n; i++) {
            shObject *obj = v->get (i);
            shObjectType type = obj->apparent ()->mType;
            if (kMoney == type or /* Always pick up money. */
                (obj->isA (kObjFootball) and Hero.mProfession == Quarterback) or
                (Flags.mAutopickup and Flags.mAutopickupTypes[type]))
            {
                interrupt ();
                if (!isBlind ()) obj->setAppearanceKnown ();
                /* HACK: as discussed below (see case shInterface::kPickup:),
                   remove obj from floor before attempting to add to
                   inventory: */
                v->remove (obj);
                if (addObjectToInventory (obj)) {
                    --i;
                    --n;
                    pickedupitem++;
                    if (Level->isInShop (mX, mY)) {
                        pickedUpItem (obj);
                    }
                } else {
                    /* END HACK: add the object back to the end of the
                       vector, and don't look at it again: */
                    v->add (obj);
                    --n;
                }
            }
        }
        if (!v->count ()) {
            delete v;
            Level->setObjects (mX, mY, NULL);
        }
    }
}


void
shHero::lookAtFloor ()
{
    int objcnt = Level->countObjects (mX, mY);
    shObjectVector *v;
    shFeature *f = Level->getFeature (mX, mY);
    int feelobjs = 0;

    feel (mX, mY);

    /*
    if (kSewage == Level->getSquare (mX, mY) -> mTerr) {
        if (0 == mZ and 0 == objcnt) {
            I->p ("You are knee deep in sewage.");
        }
    }
    */
    if (f) {
        switch (f->mType) {
        case shFeature::kStairsUp:
            I->p ("There is a staircase up here.  Press '%s' to ascend.",
                  I->getKeyForCommand (shInterface::kMoveUp));
            break;
        case shFeature::kStairsDown:
            I->p ("There is a staircase down here.  Press '%s' to descend.",
                  I->getKeyForCommand (shInterface::kMoveDown));
            break;
        case shFeature::kVat:
            I->p ("There is %s here.", f->getDescription ()); break;
        default: break;
        }
        interrupt ();
    }
    if (0 == objcnt) return;
    if (Level->isWatery (mX, mY)) {
        if (kSewage == Level->getSquare (mX, mY) -> mTerr) {
            I->p ("You feel around in the sewage...");
            feelobjs = 1;
        }
    } else if (isBlind ()) {
        if (isFlying ()) return;
        I->p ("You feel around the floor...");
        feelobjs = 1;
    }
    interrupt ();
    v = Level->getObjects (mX, mY);

    for (int i = 0; i < v->count (); ++i) {
        shObject *obj = v->get (i);
        if (hasBugSensing ())
            obj->setBugginessKnown ();
        if (!feelobjs)
            obj->setAppearanceKnown ();
    }

    if (objcnt > I->freeLogLines () - 1 and objcnt > 1) {
        I->p ("There are several objects here.");
        return;
    }
    if (1 == objcnt) {
        I->p ("You %s %s.", feelobjs ? "find" : "see here",
              v->get (0) -> inv ());
        return;
    }
    I->p ("Things that are here:");
    for (int i = 0; i < objcnt; ++i) {
        I->p (v->get (i) -> inv ());
    }
}


/* Yep.  Just for wanton of ADOM-like fancy messaging.
   The "you find a trap!", "you find another trap!" and
   "you find yet another trap!" has impressed me so much
   I wanted to implement it in PRIME too.  -- MB */
static void
yetAnotherThing (int n, shFeature *f, const char *str)
{
    if (f)
        I->p (str, n > 1 ? "yet another " : n > 0 ? "another " : "",
              n > 0 ? f->getShortDescription () : AN (f));
    else
        I->p (str, n > 1 ? "yet another" : n > 0 ? "another" : "a");
}

static void
yetAnotherCreature (int n, shCreature *c, const char *str)
{
    I->p (str, n > 1 ? "yet another " : n > 0 ? "another " : "",
          n > 0 ? c->getDescription () : c->an ());
}

void
shHero::spotStuff ()
{
    if (isBlind ())  return;
    const int SPOT_INTERVAL = 160 * FULLTURN;
    int sk = getSkillModifier (kSpot);
    int score;

    I->drawScreen ();
    if (hasPerilSensing ()) {
        sensePeril ();
    }

    shVector<shMonId> spotted_c;
    for (int i = 0; i < Level->mCrList.count (); ++i) {
        shCreature *c = Level->mCrList.get (i);
        if (c == this or !c->isHostile () or !canSee (c->mX, c->mY)) {
            continue;
        }
        if (canSee (c->mX, c->mY) and
            c->mHidden > 0 and c->mSpotAttempted + SPOT_INTERVAL < Clock and
            distance (this, c->mX, c->mY) < 30)
        {
            c->mSpotAttempted = Clock;
            score = D20 () + sk;
            debug.log ("spot check: %d", score);
            if (score > c->mHidden) {
                int cnt = 0;
                for (int j = 0; j < spotted_c.count (); ++j)
                    if (spotted_c.get (j) == c->mIlkId)  ++cnt;
                yetAnotherCreature (cnt, c, "You spot %s%s!");
                spotted_c.add (c->mIlkId);
                c->mHidden *= -1;
                Level->drawSq (c->mX, c->mY, 1);
                I->pauseXY (c->mX, c->mY);
                interrupt ();
            }
        }
    }

    int sdoors = 0, malf = 0;
    shVector<shFeature::Type> spotted_f;
    for (int i = 0; i < Level->mFeatures.count (); ++i) {
        shFeature *f = Level->mFeatures.get (i);
        if (canSee (f->mX, f->mY) and
            (f->mSpotAttempted + SPOT_INTERVAL < Clock) and
            (distance (this, f->mX, f->mY) < 30))
        {
            f->mSpotAttempted = Clock;
            score = D20 () + sk;
            debug.log ("spot check: %d", score);
            if ((shFeature::kDoorHiddenHoriz == f->mType or
                 shFeature::kDoorHiddenVert == f->mType) and
                score >= 22)
            {
                yetAnotherThing (sdoors++, NULL, "You spot %s secret door!");
                f->mType = shFeature::kDoorClosed;
                f->mSportingChance = 0;
                f->mTrapUnknown = 0;
                Level->drawSq (f->mX, f->mY);
                I->pauseXY (f->mX, f->mY);
                interrupt ();
            } else if (f->isTrap () and f->mTrapUnknown and score >= 22) {
                if (f->isBerserkDoor ()) {
                    yetAnotherThing (malf++, NULL,
                                     "You spot %s malfunctioning door!");
                } else {
                    int cnt = 0;
                    for (int j = 0; j < spotted_f.count (); ++j)
                        if (spotted_f.get (j) == f->mType)  ++cnt;
                    yetAnotherThing (cnt, f, "You spot %s%s!");
                    spotted_f.add (f->mType);
                }
                f->mTrapUnknown = 0;
                f->mSportingChance = 0;
                Level->drawSq (f->mX, f->mY);
                I->pauseXY (f->mX, f->mY);
                interrupt ();
            }
        }
    }
}


static void
checkFeature (shFeature *f, int sk, shVector<shFeature::Type> *found,
              int *sdoor, int *malf)
{
    int score = D20 () + sk + f->mSportingChance++;
    if ((shFeature::kDoorHiddenHoriz == f->mType or
         shFeature::kDoorHiddenVert == f->mType) and
        score >= 20)
    {
        f->mType = shFeature::kDoorClosed;
        f->mTrapUnknown = 0;
        f->mSportingChance = 0;
        yetAnotherThing (*sdoor++, NULL,
                         "You find %s secret door!");
        Hero.feel (f->mX, f->mY);
        Level->drawSq (f->mX, f->mY);
        I->pauseXY (f->mX, f->mY);
        Hero.interrupt ();
    } else if ((shFeature::kDoorClosed == f->mType or
                shFeature::kDoorOpen == f->mType) and
               shFeature::kBerserk & f->mDoor.mFlags and
               f->mTrapUnknown and
               score >= 12)
    {
        yetAnotherThing (*malf++, NULL,
                         "You find malfunction in %s door.");
        f->mTrapUnknown = 0;
        f->mSportingChance = 0;
        Hero.feel (f->mX, f->mY);
        Level->drawSq (f->mX, f->mY);
        I->pauseXY (f->mX, f->mY);
        Hero.interrupt ();
    } else if (f->isTrap () and
               f->mTrapUnknown and
               score >= 20)
    {
        int cnt = 0;
        for (int j = 0; j < found->count (); ++j)
            if (found->get (j) == f->mType)  ++cnt;
        yetAnotherThing (cnt, f, "You find %s%s!");
        found->add (f->mType);
        f->mTrapUnknown = 0;
        f->mSportingChance = 0;
        Hero.feel (f->mX, f->mY);
        Level->drawSq (f->mX, f->mY);
        I->pauseXY (f->mX, f->mY);
        Hero.interrupt ();
    }
}

void
shHero::doSearch (void)
{
    int sk = getSkillModifier (kSearch);
    int sdoor = 0, malf = 0;
    shVector<shFeature::Type> found;
    for (int x = mX - 1; x <= mX + 1; ++x)
        for (int y = mY - 1; y <= mY + 1; ++y)
            if (Level->isInBounds (x, y)) {
                shFeature *f = Level->getFeature (x, y);
                feel (x, y);
                if (f)  checkFeature (f, sk, &found, &sdoor, &malf);
            }
}


int
shHero::tryToTranslate (shCreature *c)
{
#define TALKERIDLEN 20
    char talkerid[TALKERIDLEN];
    const char *talker = c->the ();
    int i;

    snprintf (talkerid, TALKERIDLEN, "translate %p", (void *) c);

    if (hasTranslation ()) {
        assert (mImplants[shObjectIlk::kLeftEar] or
                (mHelmet and mHelmet->isA (kObjBioMask)));
        shObject *translator;
        if (mImplants[shObjectIlk::kLeftEar]) {
            translator = mImplants[shObjectIlk::kLeftEar];
        } else {
            translator = mHelmet;
        }

        if (1 != getStoryFlag (talkerid)) {
            I->p ("%s translates %s's %s:", translator->your (), talker,
                  c->isRobot () ? "beeps and chirps" : "alien language");
            setStoryFlag (talkerid, 1);
            translator->setKnown ();
        }
        return 1;
    }

    for (i = 0; i < mPets.count (); i++) {
        shMonster *m = (shMonster *) mPets.get (i);

        if (m->isA (kMonProtocolDroid) and m->mLevel == mLevel and
            distance (m, c) < 100 and distance (this, m) < 100)
        {
            if (2 != getStoryFlag (talkerid)) {
                I->p ("%s translates %s's %s:", m->your (), talker,
                      c->isRobot () ? "beeps and chirps" : "alien language");
                setStoryFlag (talkerid, 2);
            }
            return 1;
        }
    }
    return 0;
}


int
shHero::looksLikeJanitor ()
{
    return (Hero.mWeapon and Hero.mWeapon->isA (kObjMop) and
            Hero.mJumpsuit and Hero.mJumpsuit->isA (kObjJanitorUniform) and
            (Janitor == Hero.mProfession or !Hero.mBodyArmor));
}


void
shHero::sensePeril ()
{
    int oldperil = mGoggles->isToggled ();
    int newperil = 0;
    shCreature *c;
    shFeature *f;
    int i;

    if (oldperil) {
        mGoggles->resetToggled ();
        computeIntrinsics ();
    }

    for (i = 0; i < Level->mCrList.count (); i++) {
        c = Level->mCrList.get (i);
        if (c == this or !c->isHostile () or !canSee (c->mX, c->mY)) {
            continue;
        }
        newperil++;
        goto done;
    }

    for (i = 0; i < Level->mFeatures.count (); i++) {
        f = Level->mFeatures.get (i);
        if (f->isTrap () and canSee (f->mX, f->mY)) {
            newperil++;
            goto done;
        }
    }

done:
    const char *your_goggles = mGoggles->your ();
    if (!oldperil and newperil) {
        mGoggles->setToggled ();
        interrupt ();
        computeIntrinsics ();
        if (hasXRayVision ()) {
            I->p ("%s have darkened a bit.", your_goggles);
        } else if (!isBlind ()) {
            I->p ("%s have turned black!", your_goggles);
        }
    } else if (oldperil and !newperil) {
        mGoggles->resetToggled ();
        computeIntrinsics ();
        if (!isBlind ())
            I->p ("%s have turned transparent.", your_goggles);
    } else if (oldperil) {
        mGoggles->setToggled ();
        computeIntrinsics ();
    }
}

/* returns 1 if interrupted, 0 if not busy anyway */
int
shHero::interrupt ()
{
    if (mBusy) {
        Level->computeVisibility ();
        I->drawScreen ();
        mBusy = 0;
        return 1;
    }
    return 0;
}


/* returns 1 if successful*/
int
shHero::wield (shObject *obj, int quiet /* = 0 */ )
{
    shObject *oldweapon = mWeapon;
    if (mWeapon) {
        if (mWeapon == obj) {
            if (0 == quiet) {
                I->p ("You're wielding that already!");
            }
            return 0;
        }
        if (mWeapon->isWeldedWeapon () and !isOrc ()) {
            I->p ("%s is welded to your hand!", YOUR(mWeapon));
            mWeapon->setBugginessKnown ();
            return 0;
        }
        if (0 == unwield (mWeapon, quiet)) {
            return 0;
        }
    }
    if (TheNothingObject == obj) {
        if (NULL == oldweapon) {
            if (0 == quiet) {
                I->p ("You're already unarmed.");
            }
            return 0;
        } else {
            if (0 == quiet) {
                I->p ("You are unarmed.");
            }
            resetStoryFlag ("strange weapon message");
            mWeapon = NULL;
            return 1;
        }
    } else if (obj->isWorn ()) {
        I->p ("You can't wield that because you're wearing it!");
        return 0;
    } else {
        mWeapon = obj;
        obj->resetPrepared ();
        resetStoryFlag ("strange weapon message");
        computeSkills (); /* Compute before checking for Smartgun. */
        if (!quiet) {
            I->p ("You now wield %s.", AN (obj));
        }
        if (obj->isA (kObjChaosSword)) {
            obj->setBuggy();
            if (!quiet) {
                I->p ("You hear maniacal laughter in the distance.");
            }
        }
        if (obj->isA (kObjM56Smartgun) or obj->isA (kObjM57Smartgun)) {
            if (!isBlind () and !obj->isKnown ()) {
                if (!quiet) {
                    I->p ("You find out this weapon has automatic aim module.");
                }
                obj->setKnown ();
            }
            if (!quiet and obj->isA (kObjM57Smartgun)) {
                shSkillCode c = obj->myIlk ()->mGunSkill;
                shSkill *skill = getSkill (c);
                if (skill and skill->mBonus < 0) {
                    I->p ("Auto-aim hampers you because your skill is greater.");
                }
            }
        } else if (obj->isA (kObjKhaydarin) and obj->mCharges) {
            if (!is (kConfused) and !is (kStunned)) {
                if (!quiet) {
                    I->p ("Your mind finds peace.");
                }
                obj->setChargeKnown ();
            }
        }
        obj->setWielded ();
        computeIntrinsics ();
        return 1;
    }
}


int
shHero::unwield (shObject *obj, int quiet /* = 0 */ )
{
    assert (mWeapon == obj);
    mWeapon = NULL;
    obj->resetWielded ();
    resetStoryFlag ("strange weapon message");
    computeIntrinsics ();
    computeSkills ();
    return 1;
}


// TODO: This should be split and partially moved to Vat.cpp.
void
shHero::drop (shObject *obj)
{
    obj->resetPrepared ();
    shFeature *f = Level->getFeature (mX, mY);
    int isimp = obj->isA (kImplant); //for hopefully not erasing implants.
    const char *an_obj = AN (obj);
    if (f and shFeature::kVat == f->mType and
        isimp and I->yn ("Dip %s into the vat?", an_obj))
    {
        if (isInShop ()) {
            dropIntoOwnedVat (obj);
        }
        if (obj->isA (kObjRadiationProcessor) and f->mVat.mRadioactive) {
            I->p ("%s overloads and explodes!", THE (obj));
            Level->areaEffect (kAttConcussionGrenade, obj, mX, mY, kOrigin, this, 0, -1);
            f->mVat.mRadioactive--;
            obj->setKnown ();
            delete obj;
            return;
        } else if (obj->isA (kObjPoisonResistor)) {
            I->p ("The sludge boils upon contact with %s!", YOUR (obj));
            obj->setKnown ();
            obj->announce ();
        } else if (obj->isA (kObjReflexCoordinator) or
                   obj->isA (kObjMotoricCoordinator))
        {
            if (!isBlind ()) {
                I->p ("The sludge squirms frantically.");
            } else {
                I->p ("The sludge seems to be moving.");
            }
            obj->maybeName ();
        } else if (obj->isA (kObjAdrenalineGenerator) and !isBlind ()) {
            I->p ("The sludge looks dangerous.");
            obj->setKnown ();
            obj->announce ();
        } else if (obj->isA (kObjNarcoleptor) and !isBlind ()) {
            I->p ("Whoa!  Trippy swirls!");
            if (mResistances[kMesmerizing]) {
                I->p ("You resist the mesmerizing sight.");
            } else {
                Hero.inflict (kAsleep, 4000);
            }
            obj->setKnown ();
            obj->announce ();
        } else if (obj->isA (kObjTissueRegenerator)) {
            I->p ("The sludge grows %smoss.", isBlind () ? "" : "purple ");
            obj->setKnown ();
            obj->announce ();
        } else if (obj->isA (kObjBabelFish)) {
            I->p ("Bye bye Willy, be free.");
            delete obj;
            return;
        } else {
            I->p ("Nothing happens.");
        }
        addObjectToInventory (obj, 1);
    } else {
        I->p ("You drop %s.", an_obj);
        obj->mOwner = NULL;
        obj->resetDesignated ();
        Level->putObject (obj, mX, mY);
        if (Level->isInShop (mX, mY)) {
            maybeSellItem (obj);
        }
    }
}


static void
listDiscoveries ()
{
    shObjectType old = kUninitialized;
    char *buf = GetBuf ();
    int known = 0;
    shMenu *menu = I->newMenu ("Discoveries:", shMenu::kNoPick);
    for (int i = 0; i < kObjNumIlks; ++i) {
        if (AllIlks[i].mFlags & kIdentified and
            !(AllIlks[i].mFlags & (kPreidentified ^ kIdentified)))
        {   /* Try using headers for clarity. */
            shObjectType t = AllIlks[i].mReal.mType;
            if (t != old and t >= kMoney and t <= kEnergyCell) {
                menu->addHeader (objectTypeHeader[t]);
                old = t;
            }
            snprintf (buf, SHBUFLEN, "* %-35s * %-35s",
                AllIlks[i].mAppearance.mName, AllIlks[i].mReal.mName);
            menu->addText (buf);
            ++known;
        }
    }
    if (known) {
        menu->finish ();
    } else {
        I->p ("You recognize nothing.");
    }
    delete menu;
}


int
shHero::listInventory ()
{
    if (0 == mInventory->count ()) {
        I->p ("You aren't carrying anything!");
    } else {
        shMenu *menu = I->newMenu ("Inventory", shMenu::kCategorizeObjects);
        shObject *obj;
        for (int i = 0; i < mInventory->count (); ++i) {
            obj = mInventory->get (i);
            menu->addPtrItem (obj->mLetter, obj->inv (), obj, obj->mCount);
        }
        do {
            menu->getPtrResult ((const void **) &obj);
            /* Only examination possible after death. */
            if (obj and GameOver) {
                obj->showLore ();
                menu->dropResults ();
            }
        } while (obj and GameOver);
        delete menu;
        if (GameOver) return 0;
        if (obj) {
            I->pageLog ();
            I->p ("Do what with %s?", obj->inv ());
            I->p ("%6s - adjust     %6s - apply      %6s - drop       %6s - examine",
                I->getKeyForCommand (shInterface::kAdjust),
                I->getKeyForCommand (shInterface::kUse),
                I->getKeyForCommand (shInterface::kDrop),
                I->getKeyForCommand (shInterface::kExamine));
            I->p ("%6s - execute    %6s - name       %6s - take off   %6s - throw",
                I->getKeyForCommand (shInterface::kExecute),
                I->getKeyForCommand (shInterface::kName),
                I->getKeyForCommand (shInterface::kTakeOff),
                I->getKeyForCommand (shInterface::kThrow));
            I->p ("%6s - pay for    %6s - quaff      %6s - wear       %6s - wield",
                I->getKeyForCommand (shInterface::kPay),
                I->getKeyForCommand (shInterface::kQuaff),
                I->getKeyForCommand (shInterface::kWear),
                I->getKeyForCommand (shInterface::kWield));
            I->p ("%6s - zap",
                I->getKeyForCommand (shInterface::kZapRayGun));
            return objectVerbCommand (obj);
        }
    }
    return 0;
}

void
shHero::adjust (shObject *obj)
{
    if (!obj) {
        obj = quickPickItem (mInventory, "adjust", shMenu::kCategorizeObjects);
    }
    if (!obj) return;
    I->p ("Adjust to what letter?");
    char c = I->getChar ();
    if (!isalpha (c) or c == obj->mLetter) {
        I->nevermind ();
        return;
    }
    for (int i = 0; i < mInventory->count (); ++i) {
        shObject *obj2 = mInventory->get (i);
        if (obj2->mLetter == c) { /* Swap letters with another item. */
            obj2->mLetter = obj->mLetter;
            obj->mLetter = c;
            I->p ("%c - %s", obj->mLetter, obj->inv ());
            I->p ("%c - %s", obj2->mLetter, obj2->inv ());
            return;
        }
    }
    obj->mLetter = c; /* Adjust letter to unused one. */
    I->p ("%c - %s", obj->mLetter, obj->inv ());
}

static void
nameObject (shObject *obj)
{
    int ilk = !I->yn ("Name an individual object?");
    if (!obj) {
        obj = Hero.quickPickItem (Hero.mInventory, "name", shMenu::kCategorizeObjects);
    }
    if (obj) {
        if (ilk) {
            obj->nameIlk ();
        } else {
            obj->name ();
        }
    }
}

static int
tryToThrowObject (shObject *obj)
{
    if (obj->isWeldedWeapon () and !Hero.isOrc ()) {
        obj->setBugginessKnown ();
        I->p ("Your weapon is welded to your hands!");
        return HALFTURN;
    } else if (obj->isWorn ()) {
        I->p ("You can't throw that because you're wearing it!");
    } else {
        shDirection dir = I->getDirection ();
        if (kNoDirection == dir) return 0;

        shObject *stack = obj;
        obj = Hero.removeOneObjectFromInventory (obj);
        return Hero.throwObject (obj, stack, dir);
    }
    return 0;
}

static shObject *
chooseComputer ()
{
    shObjectVector v;
    selectObjects (&v, Hero.mInventory, kObjGenericComputer);
    if (0 == v.count ()) {
        I->p ("You don't have a computer!");
        return NULL;
    } else if (Hero.is (kUnableCompute)) {
        I->p ("You will not employ thinking machines.");
        return NULL;
    } else if (1 == v.count ()) {
        return v.get (0);
    } else {
        for (int i = 0; i < v.count (); ++i) {
            if (v.get (i)->isDesignated ()) {
                return v.get (i);
            }
        }
        shObject *computer = Hero.quickPickItem (&v, "compute with", 0);
        return computer;
    }
}

static shObject *
tryToDrop (shObject *obj, int cnt)
{
    if (obj == Hero.mWeapon) {
        if (obj->isWeldedWeapon () and !Hero.isOrc ()) {
            I->p ("Your weapon is welded to your hands.");
            obj->setBugginessKnown ();
            return NULL;
        }
        if (0 == Hero.unwield (obj)) {
            return NULL;
        }
    }
    if (obj->isWorn ()) {
        I->p ("You can't drop that because you're wearing it.");
        return NULL;
    }
    return Hero.removeSomeObjectsFromInventory (obj, cnt);
}

int
shHero::doWear (shObject *obj)
{
    if (obj->isA (kObjGenericComputer)) {
        if (obj->isDesignated ()) {
            I->p ("It is already your designated machine for running software.");
            return 0;
        }
        for (int i = 0; i < mInventory->count (); ++i) {
            shObject *o = mInventory->get (i);
            if (o->isA (kObjGenericComputer) and o->isDesignated ())
                o->resetDesignated ();
        }
        I->p ("Whenever you have more than one computer to run software you will use:");
        I->p ("%c - %s", obj->mLetter, obj->inv ());
        obj->setDesignated ();
        I->drawSideWin (); /* Update equippy chars. */
        return 0; /* Meta action takes no time. */
    }

    int elapsed = 0;
    /* Many implants need the possibility to remove helmet temporarily. */
    if (obj->isA (kImplant) and mHelmet and mHelmet->isBuggy () and !isOrc () and
        obj->mIlkId != kObjTorc and obj->mIlkId != kObjMechaDendrites)
    {   /* FIXME: but what about in vacuum/poison gas cloud? CADV */
        I->p ("You can't remove your helmet to install the implant!");
        if (!mHelmet->isBugginessKnown ()) {
            I->p ("It must be buggy!");
            mHelmet->setBugginessKnown ();
            elapsed = FULLTURN;
        }
        return elapsed;
    }
    /* Mecha-dendrites and sealed helmets don't mix. */
    if (obj->isA (kObjMechaDendrites) and
        mHelmet and mHelmet->isSealedArmor ())
    {
        I->p ("%s conflict with %s.", obj->getShortDescription (),
            mHelmet->getShortDescription ());
        return elapsed;
    }
    /* Armor piece might need to be swapped out. */
    shObject *swap = NULL;
    if (obj->isA (kObjGenericJumpsuit) and mJumpsuit) {
        /* Complicated case first. */
        if (mBodyArmor and mBodyArmor->isBuggy () and !isOrc ()) {
            if (mBodyArmor->isBugginessKnown ()) {
                I->p ("Your armor is buggy.  You can't take it off to wear jumpsuit.");
            } else {
                I->p ("You can't seem to take off your armor.  It must be buggy!");
                mBodyArmor->setBugginessKnown ();
                elapsed = FULLTURN;
            }
            return elapsed;
        }
        swap = mJumpsuit;
    } else if (obj->isA (kObjGenericArmor) and mBodyArmor) {
        swap = mBodyArmor;
    } else if (obj->isA (kObjGenericHelmet) and mHelmet) {
        swap = mHelmet;
    } else if (obj->isA (kObjGenericGoggles) and mGoggles) {
        swap = mGoggles;
    } else if ((obj->isA (kObjGenericCloak) or obj->isA (kObjPlasmaCaster)) and mCloak) {
        swap = mCloak;
    } else if (obj->isA (kObjGenericBelt) and mBelt) {
        swap = mBelt;
    } else if (obj->isA (kObjGenericBoots) and mBoots) {
        swap = mBoots;
    }
    /* Swapped armor piece must be not buggy for non-orcs. */
    if (swap and swap->isBuggy () and !isOrc ()) {
        I->p ("You can't take off %s.", YOUR (swap));
        if (!swap->isBugginessKnown ()) {
            I->p ("It must be buggy!");
            swap->setBugginessKnown ();
            elapsed = FULLTURN;
        }
        return elapsed;
    }
    if (swap) {
        I->p ("You take off %s.", YOUR (swap));
        doff (swap);
    }
    /* You could have wielded the item previously. Not a problem. */
    if (obj->isWielded ()) {
        unwield (obj);
    }
    // FIXME: mEquipSpeed is unused
    elapsed = FULLTURN;
    int donned = don (obj);
    /* Thematic message for wearing equipment of xenophobic marines. */
    if (donned and mProfession == SpaceMarine and obj->isXenosHunterGear ())
        switch (RNG (5)) {
        case 0: I->p ("Blessed is the mind too small for doubt."); break;
        case 1: I->p ("Innocence proves nothing."); break;
        case 2: I->p ("Know the mutant; kill the mutant."); break;
        case 3: I->p ("There is no such thing as innocence, only degrees of guilt."); break;
        case 4: I->p ("It is better to die for the Emperor than to live for yourself."); break;

        }
    return elapsed;
}

int
shHero::doTakeOff (shObject *obj)
{
    int elapsed = 0;
    /* Weapon is a special case. */
    if (obj == mWeapon) {
        if (0 == wield (TheNothingObject)) {
            return 0;
        }
        return HALFTURN;
    } /* So is designated computing machine. */
    if (obj->isA (kObjGenericComputer)) {
        obj->resetDesignated ();
        I->p ("You will be asked to choose computer each time if you have more than one.");
        I->drawSideWin (); /* Update equippy chars. */
        return 0; /* Meta action takes no time. */
    }
    /* Body armor must not be buggy to manipulate jumpsuit. */
    if (obj == mJumpsuit and mBodyArmor) {
        if (mBodyArmor->isBuggy () and !isOrc ()) {
            if (!mBodyArmor->isBugginessKnown ()) {
                I->p ("You can't seem to take your armor off.  It must be buggy!");
                mBodyArmor->setBugginessKnown ();
                elapsed = FULLTURN;
            } else {
                I->p ("You can't take your armor off because it is buggy.");
                I->p ("You need to do so in order to take off your jumpsuit.");
            }
            return elapsed;
        }
    }
    /* Helmet must not be buggy to manipulate most implants. */
    if (obj->isA (kImplant) and mHelmet and mHelmet->isBuggy () and !isOrc ()
        and obj->mIlkId != kObjTorc and obj->mIlkId != kObjMechaDendrites)
    {   /* FIXME: but what about in vacuum/poison gas cloud? CADV */
        if (!mHelmet->isBugginessKnown ()) {
            I->p ("You can't remove your helmet!  It must be buggy!");
            mHelmet->setBugginessKnown ();
            elapsed = HALFTURN;
        } else {
            I->p ("You can't take your helmet off because it is buggy.");
            I->p ("You need to do to free way for implants leaving your cranium.");
        }
        return elapsed;
    }

    /* Buggy belts may be forcibly taken off. */
    if (obj->isBuggy () and obj->isA (kObjGenericBelt) and !isOrc ()) {
        if (!obj->isBugginessKnown ()) {
            I->p ("You find %s is buggy.", YOUR (obj));
            obj->setBugginessKnown ();
            elapsed = HALFTURN;
        }
        if (mAbil.mStr >= 16 or
            (mAbil.mStr >= 14 and !obj->isA (kObjArmoredBelt))) {
            I->p ("You force the belt open.");
            if (RNG (2)) {
                I->nonlOnce ();
                I->p ("  It was destroyed in the process.");
                doff (obj);
                removeObjectFromInventory (obj);
                delete obj;
                return FULLTURN;
            }
        } else {
            if (obj->isA (kObjArmoredBelt) and mAbil.mStr >= 14 and
                (!obj->isAppearanceKnown () or !obj->isKnown ())) {
                I->p ("Nnnggh!  This must be an armored belt.");
                I->p ("You are strong enough to force any other belt type open.");
                obj->setKnown ();
                elapsed += FULLTURN;
                return elapsed;
            } else {
                I->p ("You are too weak to forcibly open it.");
                return elapsed;
            }
        }
    /* Item itself must not be buggy. */
    } else if (obj->isBuggy () and (!isOrc () or obj->isA (kImplant))) {
        if (obj->mIlkId == kObjTorc) {
            I->p ("It won't come out!  It must be some kind of trap of those fey folks!");
            if (!obj->isAppearanceKnown ()) {
                obj->setAppearanceKnown ();
                elapsed = FULLTURN;
            }
        } else if (!obj->isBugginessKnown ()) {
            if (obj->isA (kImplant)) {
                I->p ("It won't come out!  It must be buggy!");
            } else {
                I->p ("You can't seem to take it off.  It must be buggy!");
            }
            obj->setBugginessKnown ();
            elapsed = FULLTURN;
        } else if (obj->isA (kImplant)) {
            I->p ("It is buggy and will not respond to recall command.");
        } else {
            I->p ("To take it off it mustn't be buggy.");
        }
        return elapsed;
    }
    const char *verb = obj->isA (kImplant) ? "uninstall" : "take off";
    I->p ("You %s %s.", verb, YOUR (obj));
    doff (obj);
    return FULLTURN;
}

static bool
HeroIsWebbedOrTaped ()
{
    if (Hero.mTrapped.mWebbed or Hero.mTrapped.mTaped) {
        const char *something = Hero.mTrapped.mWebbed and Hero.mTrapped.mTaped ?
            "webs and duct tape" : Hero.mTrapped.mTaped ? "duct tape" : "webs";
        I->p ("Not while you are held by %s.", something);
        return true;
    }
    return false;
}

static bool
HeroCanThrow ()
{
    if (HeroIsWebbedOrTaped ())
        return false;
    if (Hero.is (kFrightened)) {
        I->p ("You are too afraid to throw anything!");
        return false;
    }
    if (Hero.is (kUnableAttack)) {
        I->p ("What an insulting display of belligerence.");
        return false;
    }
    return true;
}

static bool
HeroCanZap ()
{
    if (HeroIsWebbedOrTaped ())
        return false;
    if (Hero.is (kFrightened)) {
        I->p ("You are too afraid to use a raygun!");
        return false;
    }
    if (Hero.is (kUnableAttack)) {
        I->p ("What an insulting display of belligerence.");
        return false;
    }
    return true;
}

static bool
HeroCanFire ()
{
    if (HeroIsWebbedOrTaped ())
        return false;
    int dothrow = !Hero.mWeapon->myIlk ()->mGunAttack
                  and !Hero.mWeapon->isA (kRayGun)
                  and !Hero.mWeapon->isA (kObjGenericComputer);
    if (Hero.is (kFrightened)) {
        I->p ("You are too afraid to %s your weapon!",
              dothrow ? "throw" : "fire");
        return false;
    }
    if (Hero.is (kUnableAttack)) {
        I->p ("What a blatant display of belligerence.");
        return false;
    }
    return true;
}

static bool
HeroCanExecute (shObject *computer)
{
    bool webortape = Hero.mTrapped.mWebbed or Hero.mTrapped.mTaped;
    if (!webortape)
        return true;
    if (computer->isA (kObjBioComputer)) {
        if (Hero.isMute ()) {
            I->p ("Being mute, you are unable to command %s.", YOUR (computer));
            return false;
        } else {
            return true;
        }
    }

    HeroIsWebbedOrTaped ();
    return false;
}

int /* Time elapsed. */
shHero::objectVerbCommand (shObject *obj)
{
    shInterface::Command cmd = I->getCommand ();
    switch (cmd) {
    case shInterface::kAdjust:
        adjust (obj);
        return 0;
    case shInterface::kDrop:
        obj = tryToDrop (obj, obj->mCount);
        if (!obj) return 0;
        drop (obj);
        feel (mX, mY);
        return HALFTURN;
    case shInterface::kExamine:
        obj->showLore ();
        return 0;
    case shInterface::kExecute:
    {
        if (!obj->myIlk ()->mRunFunc) {
            I->p ("You cannot execute this.");
            return 0;
        }
        shObject *computer = chooseComputer ();
        if (!computer)  return 0;
        if (!HeroCanExecute (computer))  return 0;
        return computer->executeProgram (obj);
    }
    case shInterface::kName:
        nameObject (obj);
        return 0;
    case shInterface::kPay:
    {
        int price = obj->mCount * obj->myIlk ()->mCost;
        shMonster *shopkeeper = Level->getShopKeeper (mX, mY);
        if (!obj->isUnpaid ()) {
            I->p ("You don't need to pay for it.");
        } else if (countMoney () < price) {
            I->p ("You do not have enough money to purchase it.");
        } else if (!shopkeeper) {
            I->p ("There's nobody around to pay.");
        } else {
            loseMoney (price);
            shopkeeper->gainMoney (price);
            obj->resetUnpaid ();
            I->p ("You buy %s for %d buckazoids.", THE (obj), price);
        }
        return 0;
    }
    case shInterface::kQuaff:
        if (!obj->myIlk ()->mQuaffFunc) {
            I->p ("You cannot drink this.");
            return 0;
        }
        return quaffCanister (obj);
    case shInterface::kTakeOff:
        if (!obj->isEquipped ()) {
            I->p ("You haven't equipped this item.");
            return 0;
        }
        return doTakeOff (obj);
    case shInterface::kThrow:
    {
        if (!HeroCanThrow ()) return 0;
        return tryToThrowObject (obj);
    }
    case shInterface::kUse:
        if (obj->myIlk ()->mUseFunc) {
            if (HeroIsWebbedOrTaped ())  return 0;
            return obj->myIlk ()->mUseFunc (obj);
        } else {
            I->p ("This object cannot be applied.");
            return 0;
        }
    case shInterface::kWear:
        if (!obj->canBeWorn ()) {
            I->p ("%s cannot be worn.", THE (obj));
            return 0;
        }
        if (obj->isWorn ()) {
            I->p ("You are wearing this already.");
            return 0;
        }
        return doWear (obj);
    case shInterface::kWield:
        if (!wield (obj)) {
            return 0;
        } else {
            return HALFTURN;
        }
    case shInterface::kZapRayGun:
    {
        if (!obj->myIlk ()->mZapAttack) {
            I->p ("You cannot zap this.");
            return 0;
        }
        if (!HeroCanZap ()) return 0;

        shDirection dir = I->getDirection ();
        if (kNoDirection == dir) {
            return 0;
        }
        return shootWeapon (obj, dir);
    }
    default:
        I->nevermind ();
        return 0;
    }
}

/* returns non-zero if the hero dies */
int
shHero::instantUpkeep ()
{
    for (int i = mInventory->count () - 1; i >= 0; --i) {
        shObject *obj = mInventory->get (i);
        if ((obj->isActive () and obj->myIlk ()->mEnergyUse)
            or obj->isA (kObjGenericPlant))
        {
            if (MAXTIME == obj->mLastEnergyBill) {
                /* unnecessary, should be handled by setActive () */
                obj->mLastEnergyBill = Clock;
            } else {
                while (Clock - obj->mLastEnergyBill > obj->myIlk ()->mEnergyUse) {
                    if (obj->isA (kObjGenericPlant)) {
                        gainEnergy (1);
                    } else if (0 == loseEnergy (1)) {
                        obj->resetActive ();
                        I->p ("%s has shut itself off.", YOUR(obj));
                    }
                    obj->mLastEnergyBill += obj->myIlk ()->mEnergyUse;
                }
            }
        }
    }
    if (hasAutoRegeneration ()) {
        while (Clock - mLastRegen > 1500) {
            if (mHP < mMaxHP) ++mHP;
            mLastRegen += 1500;
        }
    }
    if (checkTimeOuts ()) {
        return -1;
    }
    computeIntrinsics ();
    return 0;
}


void
shHero::feel (int x, int y, int force)
{
    if (!force and
        (!isBlind () or !isAdjacent (x, y)))
    {
        return;
    }
    Level->feelSq (x, y);
}


/* try to move the hero one square in the given direction
   RETURNS: AP expended
 */

int
shHero::doMove (shDirection dir)
{
    int x = mX;
    int y = mY;
    int tx, ty;
    static int leftsq, rightsq; /* this static crap is not tasty */
    shFeature *f;
    int speed;

    speed = isDiagonal (dir) ? DIAGTURN: FULLTURN;

    if (!Level->moveForward (dir, &x, &y)) {
        //I->p ("You can't move there!");
        return 0;
    }

    mDir = dir;

    if (kOrigin == dir) {
        /* No checks needed. */
    } else if (1 == mBusy) {
        tx = x; ty = y;
        if (!Level->moveForward (leftTurn (dir), &tx, &ty)) {
            leftsq = 0;
        } else {
            leftsq = Level->appearsToBeFloor (tx, ty);
        }
        tx = x; ty = y;
        if (!Level->moveForward (rightTurn (dir), &tx, &ty)) {
            rightsq = 0;
        } else {
            rightsq = Level->appearsToBeFloor (tx, ty);
        }
        ++mBusy;
    } else if (mBusy) {
        if (is (kStunned)) {
            interrupt ();
            return 0;
        }
        feel (x, y);
        if (Level->isObstacle (x, y)) {
            interrupt ();
            return 0;
        }
        if ((f = Level->getFeature (x, y))) {
            if (!f->mTrapUnknown or f->isObstacle ()) {
                interrupt ();
                return 0;
            }
        }
        if (isBlind ()) {
            tx = x;     ty = y;
            if (Level->moveForward (leftTurn (dir), &tx, &ty)) {
                feel (tx, ty);
            }
            if (Level->moveForward (rightTurn (dir), &tx, &ty)) {
                feel (tx, ty);
            }
            goto trymove;
        }

        tx = x; ty = y;
        if (!Level->moveForward (leftTurn (dir), &tx, &ty) or
            (leftsq != Level->appearsToBeFloor (tx, ty)) or
            (Level->countObjects (tx, ty) and !Level->isWatery (tx, ty)) or
            Level->getKnownFeature (tx, ty))
        {
            interrupt ();
        }
        tx = x; ty = y;
        if (!Level->moveForward (rightTurn (dir), &tx, &ty) or
            (rightsq != Level->appearsToBeFloor (tx, ty)) or
            (Level->countObjects (tx, ty) and !Level->isWatery (tx, ty)) or
            Level->getKnownFeature (tx, ty))
        {
            interrupt ();
        }
    }
trymove:

    if (Level->isObstacle (x, y)) {
        int ismapped = Level->getMemory (x, y).mTerr != kMaxTerrainType;
        if (!interrupt ()) {
            if (is (kStunned) or is (kConfused) or
                (isBlind () and /* Try to open remembered doors. */
                 Level->getMemory (x, y).mFeat != shFeature::kDoorClosed))
            {
                I->p ("You bump into %s!",
                    ismapped ? Level->the (x, y) : Level->an (x, y));
                feel (x, y);
                return speed;
            }
        }
        /* Auto open doors when you bump into them. */
        shFeature *f = Level->getFeature (x, y);
        if (f and f->isDoor ()) {
            openDoor (x, y);
            feel (x, y);
            return FULLTURN;
        }
    } else if (Level->isOccupied (x, y) and dir != kOrigin) {
        feel (x, y);
        if (!interrupt ()) {
            shCreature *c = Level->getCreature (x, y);
            if (is (kStunned) or is (kConfused) or
                (isBlind () and !canHearThoughts (c) and !canSenseLife (c)))
            {
                I->p ("You bump into %s!", THE (c));
                if (!c->isHostile ()) {
                    if (c->isRobot ()) {
                        I->p ("It beeps.");
                        if (hasTranslation ()) {
                            Level->remember (x, y, c->mIlkId);
                            I->refreshScreen ();
                            I->pauseXY (x, y, 1000, 0);
                        }
                    } else if (c->isA (kMonMelnorme)) {
                        if (tryToTranslate (c)) {
                            I->p ("\"Please be careful.\"");
                            Level->remember (x, y, c->mIlkId);
                            I->refreshScreen ();
                            I->pauseXY (x, y, 1000, 0);
                        } else {
                            I->p ("You hear annoyed mumbling.");
                        }
                    } else if (c->isA (kMonMonolith)) {
                        I->p ("It seems to be a solid block.");
                        Level->remember (x, y, c->mIlkId);
                    }
                }
                return speed;
            }
        }
    } else {
        /* Ask before entering square with a trap covered by items. */
        shFeature *f = Level->getFeature (x, y);
        if ((x != mX or y != mY) and /* This is not wait in place. */
            Level->getObjects (x, y) and /* Stuff is lying there. */
            f and !f->mTrapUnknown and /* There is a trap you know about. */
            ( /* List of bad stuff to prompt confirmation: */
             f->mType == shFeature::kAcidPit or
             f->mType == shFeature::kPit or
             f->mType == shFeature::kHole or
             f->mType == shFeature::kTrapDoor or
             f->mType == shFeature::kRadTrap
            ))
        {
            if (!I->yn ("Confirm entering place with an obscured %s?",
                        f->getShortDescription ()))
                return 0; /* Cancelled. */
        }
        Level->moveCreature (this, x, y);
        if (mBusy) {
            I->pauseXY (mX, mY, 10);
            if (kOrigin == dir) {
                ++mBusy;
                if (mBusy == 20+1)
                    interrupt (); /* Limit resting to 20 turns. */
            }
        }
        return speed;
    }

    return 0;
}


/* called every 10 seconds */
void
shHero::upkeep ()
{
    if (!is (kSickened) and (!is (kPlagued) or mHP < mMaxHP / 2)) {
        /* Recover hit points. */
        if (mHP < mMaxHP) {
            mHP += 1 + mCLevel / 3 + isOrc ();
            if (mHP > mMaxHP)  mHP = mMaxHP;
        }
    }
    /* recover a cha drain point */
    if (mPsiDrain > 0) {
        --mPsiDrain;
        ++mAbil.mPsi;
    } else if (mPsiDrain < 0) {
        ++mPsiDrain;
        --mAbil.mPsi;
    }

    /* superglued tongue */
    {
        int glue = getStoryFlag ("superglued tongue");
        if (glue > 0) {
            glue++;
            if (glue > 100) {
                I->p ("The canister of super glue has finally fallen off!");
                resetStoryFlag ("superglued tongue");
            } else {
                setStoryFlag ("superglued tongue", glue);
            }
        }
    }

    /* reduce police awareness of your software piracy crimes */
    {
        int danger = getStoryFlag ("software piracy");
        if (danger > 0) {
            setStoryFlag ("software piracy", --danger);
        }
    }

    /* check for alien impregnation */
    {
        int preggers = getStoryFlag ("impregnation");

        if (preggers > 0) {
            preggers++;
            if (preggers > 40) {
                int x = Hero.mX;
                int y = Hero.mY;
                int queen = !RNG (0, 17);
                // (v1.7) changed nearby to adjacent for better style -- MB
                int gotPlace = Level->findAdjacentUnoccupiedSquare (&x, &y);
                shMonster *baby = new shMonster (queen ? kMonAlienPrincess : kMonChestburster);
                if (baby and gotPlace) {
                    /* (v1.7) No place to put baby? Just wait but make sure
                       sickness continues. -- MB */
                    inflict (kSickened, 10000);
                    inflict (kStunned, 6000); /* periodical stun */
                    delete baby;
                } else {
                    Level->putCreature (baby, x, y);
                    I->drawScreen ();
                    resetStoryFlag ("impregnation"); /* for post mortem */
                    I->p ("The alien creature explodes from your chest!");
                    shCreature::die (kMisc, "Died during childbirth");
                }
            } else if (38 == preggers) {
                I->p ("You are violently ill!");
                inflict (kStunned, 35000);
                interrupt ();
            } else if (35 == preggers) {
                I->p ("You vomit!");
                inflict (kSickened, 100000);
                interrupt ();
            } else if (20 == preggers) {
                I->p ("You feel something moving inside you!");
                interrupt ();
            } else if (10 == preggers) {
                I->p ("You feel a little queasy.");
                interrupt ();
            }
            setStoryFlag ("impregnation", preggers);
        }
    }

    /* Check radiation. */
    int r = checkRadiation ();
    if (r) {
        r -= getResistance (kRadiological);
        if (r < 0) r = 0;
    }
    mRad += r;

    if (mRad > 300) {
        mRad -= RNG (1, 5);
    }

    if (!r and hasRadiationProcessing ()) {
        mRad -= 10;
    }

    if (mRad < 0) {
        mRad = 0;
    }

    if (mRad >= 400 and !(mInnateIntrinsics & kLightSource)) {
        if (!isBlind ()) {
            I->p ("You start glowing!");
        }
        mInnateIntrinsics |= kLightSource;
    }
    if (mRad < 300 and mInnateIntrinsics & kLightSource) {
        if (!isBlind ()) {
            I->p ("You no longer glow.");
        }
        mInnateIntrinsics &= ~kLightSource;
    }

    if (2 == (Clock / 10000) % 6) { /* Every minute. */
        int stage = getStoryFlag ("radsymptom");
        if (stage >= 10 and mRad > 1000 and !RNG (100)) {
            /* TODO: Mutation. */
        }
        if (stage == 0 and mRad >= 75 and mRad <= 150 and checkRadiation ()) {
            switch (RNG (3)) {
            case 0:
                I->p ("You have a slight headache."); break;
            case 1:
                I->p ("You feel fatigued."); break;
            case 2:
                I->p ("You feel somewhat sick."); break;
            }
        } else if (mRad > 150) {
            if (mRad / 12 > RNG (20) + getCon () - 10) {
                int more_dmg = stage / 7 + mRad / 300;
                if (sufferAbilityDamage (kCon, RNG (1, 2) + more_dmg)) {
                    I->p ("You are overcome by your illness.");
                    shCreature::die (kSlain, "radiation sickness");
                } else {
                    /* If hero's constitution is already damaged or very low
                       advance sickness to later stage. */
                    if (stage < 7 - getCon ()) {
                        stage = 7 - getCon ();
                    }
                    switch (stage) {
                    case 0:
                        I->p ("You feel tired."); break;
                    case 1:
                        I->p ("You feel feverish."); break;
                    case 2:
                        I->p ("You feel nauseated."); break;
                    case 3:
                        I->p ("You vomit!"); break;
                    case 4:
                        I->p ("Your gums are bleeding!"); break;
                    case 5:
                        I->p ("Your hair is falling out!"); break;
                    case 6:
                        I->p ("Your skin is melting!"); break;
                    default:
                        I->p ("Your body is wracked with convulsions.");
                        inflict (kStunned, stage * FULLTURN);
                    }
                    ++stage;
                    setStoryFlag ("radsymptom", stage);
                }
            } else if (0 == stage) {
                I->p ("You have a headache.");
                setStoryFlag ("radsymptom", 1);
            } else if (RNG (2)) {
                I->p ("You feel weary.");
            } else {
                I->p ("You feel quite sick.");
            }
        } else {
            if (getStoryFlag ("radsymptom")) {
                I->p ("You're beginning to feel better.");
                setStoryFlag ("radsymptom", 0);
            }
        }
        mRad -= 5; /* slowly recover */
        if (mRad < 0) {
            mRad = 0;
        }
    }

    if (hasNarcolepsy () and !is (kAsleep) and !RNG (10)) {
        if (Hero.mResistances[kMesmerizing]) {
            I->p ("You feel drowsy.");
        } else {
            I->p ("You suddenly fall asleep!");
            inflict (kAsleep, FULLTURN * RNG (20, 100));
        }
    }
}


void
shHero::takeTurn ()
{
    int dx;
    int dy;
    int glidemode = 0;
    int elapsed = 0;

    if (instantUpkeep ()) { /* hero died */
        return;
    }
    Level->computeVisibility ();
    I->drawScreen ();

    if (is (kParalyzed) or is (kAsleep)) {
        /* lose a turn */
        mAP -= 1000;
        return;
    }

    if (hasAutoSearching ()) {
        doSearch ();
        Level->drawSq (mX, mY);
    }

 getcmd:

    { /* Process geiger counters. */
        int rl = checkRadiation ();
        int ol = getStoryFlag ("geiger counter message");

        if (rl != ol) {
            int cnt = 0;
            shObject *counter = NULL;
            const char *what = NULL;

            for (int i = 0; i < mInventory->count (); ++i) {
                shObject *obj = mInventory->get (i);
                if (obj->isA (kObjGeigerCounter) and obj->isActive ()) {
                    ++cnt;
                    counter = obj;
                }
            }
            if (cnt > 1) {
                /* Kludge: temporarily change object quantity. */
                counter->mCount = cnt;
                what = YOUR (counter);
                counter->mCount = 1;
            } else if (cnt) {
                what = YOUR (counter);
            }
            if (cnt) { /* React to radiation level changes. */
                interrupt ();
                if (0 == ol) {
                    I->p ("%s %s making a clicking noise.", what,
                        cnt > 1 ? "are" : "is");
                    if (!counter->isKnown ()) {
                        I->p ("%s must be %sGeiger counter%s.", what,
                        cnt == 1 ? "a " : "", cnt > 1 ? "s" : "");
                        counter->setKnown ();
                    }
                    /* Reveal appearance of all active counters. */
                    for (int i = 0; i < mInventory->count (); ++i) {
                        shObject *obj = mInventory->get (i);
                        if (obj->isA (kObjGeigerCounter) and obj->isActive ()) {
                            obj->setAppearanceKnown ();
                        }
                    }
                } else if (0 == rl) {
                    I->p ("%s %s stopped clicking.", what,
                        cnt > 1 ? "have" : "has");
                } else if (ol < rl) {
                    I->p ("%s %s clicking more rapidly.", what,
                        cnt > 1 ? "are" : "is");
                } else {
                    I->p ("%s %s clicking more slowly.", what,
                        cnt > 1 ? "are" : "is");
                }
            }
        }
        setStoryFlag ("geiger counter message", rl);
    }


    if (mBusy and !isTrapped () and !is (kStunned) and !is (kConfused)) {
        elapsed = doMove (mDir);
        if (elapsed) {
            mAP -= elapsed;
            if (mDir == kOrigin) { /* Search around while resting. */
                doSearch ();
            }
            return;
        }
    }

    mBusy = 0;
    elapsed = 0;
    shInterface::Command cmd = I->getCommand ();

    dx = 0;
    dy = 0;
    if (cmd >= shInterface::kMoveN and cmd <= shInterface::kMoveNW) {
        /* Stun totally randomizes movement every turn. */
        if (is (kStunned)) {
            cmd = (shInterface::Command) (shInterface::kMoveN + RNG (8));
        /* Confusion tilts to left or right every third turn or so. */
        } else if (is (kConfused) and !RNG (3)) {
            if (RNG (2)) {
                cmd = shInterface::Command (int (cmd) - 1);
            } else {
                cmd = shInterface::Command (int (cmd) + 1);
            }
            if (cmd < shInterface::kMoveN)  cmd = shInterface::kMoveNW;
            if (cmd > shInterface::kMoveNW)  cmd = shInterface::kMoveN;
        }
    }

    switch (cmd) {
    case shInterface::kNoCommand:
        I->p ("Unknown command.  Type '%s' for help.",
              I->getKeyForCommand (shInterface::kHelp));
        break;
    case shInterface::kHelp:
        I->showHelp ();
        break;
    case shInterface::kHistory:
        I->showHistory ();
        break;
    case shInterface::kRest:
        doSearch ();
        elapsed = doMove (kOrigin);
        break;
    case shInterface::kGlide:
    {
        if (is (kConfused) or is (kStunned) or isTrapped ()) {
            break;
        }
        int dz = 0;
        shDirection d;
        d = I->getDirection (&dx, &dy, &dz, 1);
        if (kNoDirection == d or dz) {
            break;
        }
        glidemode = 1;
        mBusy = 1;
        I->pageLog ();

        if (kOrigin == d) {
            /* Do not attempt trap escape. Go straight to waiting instead. */
            elapsed = doMove (kOrigin);
            break;
        }
        goto domove;
    }
    case shInterface::kGlideN:
        glidemode = 1; mBusy = 1; /* fall through */
    case shInterface::kMoveN:
        --dy;  goto domove;
    case shInterface::kGlideNE:
        glidemode = 1; mBusy = 1; /* fall through */
    case shInterface::kMoveNE:
        ++dx; --dy; goto domove;
    case shInterface::kGlideE:
        glidemode = 1; mBusy = 1; /* fall through */
    case shInterface::kMoveE:
        ++dx; goto domove;
    case shInterface::kGlideSE:
        glidemode = 1; mBusy = 1; /* fall through */
    case shInterface::kMoveSE:
        ++dx; ++dy; goto domove;
    case shInterface::kGlideS:
        glidemode = 1; mBusy = 1; /* fall through */
    case shInterface::kMoveS:
        ++dy; goto domove;
    case shInterface::kGlideSW:
        glidemode = 1; mBusy = 1; /* fall through */
    case shInterface::kMoveSW:
        ++dy; --dx; goto domove;
    case shInterface::kGlideW:
        glidemode = 1; mBusy = 1; /* fall through */
    case shInterface::kMoveW:
        --dx; goto domove;
    case shInterface::kGlideNW:
        glidemode = 1; mBusy = 1; /* fall through */
    case shInterface::kMoveNW:
        --dx; --dy;

    domove:
        /* FIXME: Remove all logic pertraining when to untrap self from
            following block and place it first.  That should eliminate all
            gotos and make the code piece easier to read. */
        {
            int x = mX + dx;
            int y = mY + dy;
            shCreature *c;

            if (!Level->isInBounds (x, y))
                break;

            c = Level->getCreature (x, y);

            if (mTrapped.mWebbed or mTrapped.mTaped)  goto untrap;
            if (Level->rememberedCreature (x, y) or
                (c and (!isBlind () or isAwareOf (c))))
            { /* this must be an attack */
                if (glidemode) {
                    glidemode = 0;
                    break;
                }

                if (c and c->mZ < 0 and mLevel->isObstacle (x, y)) {
                    I->p ("You can't attack there!");
                    break;
                }
                if (mZ < 0 and mLevel->isObstacle (mX, mY)) {
                    if (isTrapped ())
                        goto untrap;
                    else
                        break;
                }
                if (c and c->isPet () and !is (kStunned) and !is (kConfused)
                    and !c->isSessile ())
                {
                    if (isTrapped ()) {
                        goto untrap;
                    }
                    elapsed = displace (c);
                } else {
                    if (c and is (kFrightened) and !is (kConfused) and
                             !is (kStunned) and !isBlind ())
                    {
                        I->p ("You are too afraid to attack %s!", THE (c));
                        break;
                    }
                    if (c and !c->isHostile () and !c->isA (kMonMonolith) and
                        !isBlind () and !is (kConfused) and !is (kStunned))
                    {
                        if (!I->yN ("Really attack %s?", THE (c))) {
                            I->nevermind ();
                            break;
                        }
                    }
                    if (c and c->isA (kMonFuelBarrel) and isAwareOf (c) and
                        mHP < Attacks[kAttExplodingMonster].mDamage[0].mHigh and
                        !getStoryFlag ("melee barrel"))
                    {   /* No point in killing players who like DoomRL very
                           much that way.  Expectations do carry over. */
                        I->p ("Be warned that you cannot push barrels.");
                        I->p ("You need %d hit points to surely survive the explosion.",
                            Attacks[kAttExplodingMonster].mDamage[0].mHigh + 1);
                        I->p ("There will be no futher warnings.");
                        setStoryFlag ("melee barrel", 1);
                        I->pause ();
                        break;
                    }
                    elapsed = meleeAttack (mWeapon, I->moveToDirection (cmd));
                }
                if (!elapsed) {
                    break;
                }
            } else if (isTrapped ()) {
            untrap:
                elapsed = tryToFreeSelf ();
            } else {
                elapsed = doMove (vectorDirection (dx, dy));
            }
        }
        break;
    {
        shDirection dir;

    case shInterface::kFireWeapon:
    case shInterface::kFireN:
    case shInterface::kFireNE:
    case shInterface::kFireE:
    case shInterface::kFireSE:
    case shInterface::kFireS:
    case shInterface::kFireSW:
    case shInterface::kFireW:
    case shInterface::kFireNW:
//    case shInterface::kFireDown:
//    case shInterface::kFireUp:
        if (cmd == shInterface::kFireWeapon) {
            dir = kNoDirection;
        } else {
            dir = (shDirection) ((int) cmd - (int) shInterface::kFireN);
        }

        if (!mWeapon or
            !(mWeapon->myIlk ()->mGunAttack or
              mWeapon->isThrownWeapon () or mWeapon->isA (kRayGun) or
              (mWeapon->isA (kObjGenericComputer) and mCloak and
               mCloak->isA (kObjPlasmaCaster))))
        {
            I->p ("You're not wielding a ranged weapon.");
            break;
        }

        if (!HeroCanFire ()) {
            break;
        }

        if (kNoDirection == dir) {
            dir = I->getDirection ();
            if (kNoDirection == dir) {
                break;
            }
        }
        if (kOrigin == dir) {
            if (!I->yn ("Really harm yourself?  Please confirm.")) break;
        }
        resetStoryFlag ("strange weapon message");

        int dothrow = !mWeapon->myIlk ()->mGunAttack
                      and !mWeapon->isA (kRayGun)
                      and !mWeapon->isA (kObjGenericComputer);
        if (dothrow) {
            shObject *obj = removeOneObjectFromInventory (mWeapon);
            elapsed = throwObject (obj, mWeapon, dir);
        } else {
            elapsed = shootWeapon (mWeapon, dir);
        }
        break;
    }
    case shInterface::kThrow:
    {
        shObject *obj;
        shObjectVector v;

        if (!HeroCanThrow ()) break;

        selectObjectsByFunction (&v, mInventory, &shObject::isThrownWeapon);
        obj = quickPickItem (&v, "throw", shMenu::kFiltered |
                             shMenu::kCategorizeObjects);
        if (!obj) break;
        elapsed = tryToThrowObject (obj);
        break;
    }
    case shInterface::kKick:
    {
        shDirection dir;

        dir = I->getDirection ();
        if (kNoDirection == dir) {
            break;
        } else if (kOrigin == dir) {
            I->p ("Kicking yourself isn't going to make things any better.");
            break;
        }

        elapsed = kick (dir);
        break;
    }

    case shInterface::kZapRayGun:
    {
        if (!HeroCanZap ()) break;
        shObjectVector v;
        selectObjects (&v, mInventory, kRayGun);
        shObject *obj = quickPickItem (&v, "zap", 0);
        if (!obj) break;
        shDirection dir = I->getDirection ();
        if (kNoDirection == dir) break;
        elapsed = shootWeapon (obj, dir);
        break;
    }
    case shInterface::kMoveUp:
    {
        int x, y;
        shMapLevel *oldlevel = Level;
        shFeature *stairs = Level->getFeature (mX, mY);

        if (isTrapped ()) {
            elapsed = tryToFreeSelf ();
            break;
        }

        if (!stairs or shFeature::kStairsUp != stairs->mType) {
            I->p ("You can't go up here.");
            break;
        }
        Level->removeCreature (this);
        Level = Maze.get (stairs->mDest.mLevel);

        if (-1 == stairs->mDest.mX) {
            Level->findUnoccupiedSquare (&x, &y);
        } else {
            x = stairs->mDest.mX;
            y = stairs->mDest.mY;
            if (Level->isOccupied (x, y)) {
                Level->findNearbyUnoccupiedSquare (&x, &y);
            }
        }
        Level->putCreature (this, x, y);
        checkForFollowers (oldlevel, stairs->mX, stairs->mY);
        elapsed = FULLTURN;
        break;
    }
    case shInterface::kMoveDown:
    {
        int x, y;
        shMapLevel *oldlevel = Level;
        shFeature *stairs = Level->getFeature (mX, mY);

        if (!stairs) {
            I->p ("You can't go down here.");
            break;
        }
        if (-1 == Hero.mZ) {
            I->p ("You're already at the bottom.");
            break;
        }

        if (isFlying ()) {
            I->p ("You are flying high above %s.", THE (stairs));
            break;
        }
        switch (stairs->mType) {
        case shFeature::kHole:
            I->p ("You jump into a hole!");
            break;
        case shFeature::kTrapDoor:
            I->p ("You jump into a trap door!");
            break;
        case shFeature::kPit:
            I->p ("You %s into the pit.", canJump () ? "jump" : "climb down");
            mZ = -1;
            mTrapped.mInPit = NDX (1, 6);
            elapsed = FULLTURN;
            break;
        case shFeature::kAcidPit:
            I->p ("You %s into the pit.", canJump () ? "jump" : "climb down");
            mZ = -1;
            mTrapped.mInPit = NDX (1, 6);
            if (sufferDamage (kAttAcidPitBath)) {
                shCreature::die (kMisc, "Dissolved in acid");
                return;
            }
            setTimeOut (TRAPPED, 1000, 0);
            elapsed = FULLTURN;
            break;
        case shFeature::kSewagePit:
            I->p ("You dive into the sewage.");
            Hero.mZ = -1;
            Hero.mTrapped.mInPit = NDX (1, 6);
            if (!hasAirSupply () and !isBreathless ()) {
                I->p ("You're holding your breath!");
                mTrapped.mDrowning = getCon ();
            }
            setTimeOut (TRAPPED, 1000, 0);
            elapsed = FULLTURN;
            break;
        case shFeature::kStairsDown:
            break;
        default:
            I->p ("You can't go down here.");
            break;
        }

        if (elapsed)
            break;

        /* o/w we are descending a level */
        Level->removeCreature (this);
        Level = Maze.get (stairs->mDest.mLevel);
        if (!Level) {
            Level = oldlevel->getLevelBelow ();
        }
        if (-1 == stairs->mDest.mX) {
            Level->findUnoccupiedSquare (&x, &y);
        } else {
            x = stairs->mDest.mX;
            y = stairs->mDest.mY;
            if (Level->isOccupied (x, y)) {
                Level->findNearbyUnoccupiedSquare (&x, &y);
            }
        }
        Level->putCreature (this, x, y);
        checkForFollowers (oldlevel, stairs->mX, stairs->mY);
        elapsed = FULLTURN;
        break;
    }
    case shInterface::kOpen:
    case shInterface::kClose:
    {
        int dz;
        int x, y;
        shFeature *f;
        shCreature *c;

        if (kNoDirection == I->getDirection (&dx, &dy, &dz)) {
            break;
        }
        x = dx + mX;
        y = dy + mY;
        if (!Level->isInBounds (x, y)) {
            I->p ("There is nothing to %s there.",
                  shInterface::kOpen == cmd ? "open" : "close");
            break;
        }
        feel (x, y);
        f = Level->getFeature (x, y);
        c = Level->getCreature (x, y);
        if (shInterface::kOpen == cmd) {
            if (c and c->isPet ()) {
                shObject *bolt = NULL;
                for (int i = 0; i < c->mInventory->count (); ++i) {
                    shObject *obj = c->mInventory->get (i);
                    if (obj->isA (kObjRestrainingBolt)) {
                        bolt = obj;
                        break;
                    }
                }
                bool tookaction = false;
                if (bolt) {
                    if (I->yn ("Remove restraining bolt from %s?", THE (c))) {
                        shMonster *m = (shMonster *) c;
                        c->mInventory->remove (bolt);
                        addObjectToInventory (bolt);
                        mPets.remove (c);
                        m->mTame = 0;
                        m->mTactic = shMonster::kNewEnemy;
                        m->mDisposition = shMonster::kHostile;
                        m->mStrategy = m->myIlk ()->mDefaultStrategy;
                        elapsed = FULLTURN;
                        tookaction = true;
                    }
                }
                if (c->isA (kMonSmartBomb) and !tookaction) {
                    if (I->yn ("Deactivate %s?", THE (c))) {
                        mPets.remove (c); /* Prevent sadness message. */
                        c->die (kMisc);
                        --c->myIlk ()->mKills; /* That was not a kill. */
                        elapsed = FULLTURN;
                        tookaction = true;
                    }
                }
                if (!tookaction)  break;
            } else if (f and shFeature::kDoorClosed == f->mType) {
                openDoor (x, y);
                feel (x, y, 1);
                elapsed = FULLTURN;
            } else if (f and shFeature::kDoorOpen == f->mType) {
                I->p ("It's already open.");
                break;
            } else if (f and shFeature::kPortableHole == f->mType) {
                int nx = x + dx, ny = y + dy, fail = 0;
                /* Probe for another hole. */
                while (Level->getSquare (nx, ny)->mTerr == kStone) {
                    nx += dx;
                    ny += dy;
                    if (!Level->isInBounds (nx, ny)) {
                        fail = 1;
                        break;
                    }
                }
                shFeature *end = NULL;
                if (!fail) {
                    end = Level->getFeature (nx, ny);
                    if (!end or end->mType != shFeature::kPortableHole) {
                        fail = 1;
                    }
                }
                Level->removeFeature (f);
                if (fail) {
                    I->p ("The portable hole breaks down.");
                } else { /* Dig a tunnel. */
                    Level->removeFeature (end);
                    Level->dig (x, y);
                    while (x != nx or y != ny) {
                        x += dx;
                        y += dy;
                        Level->dig (x, y);
                    }
                    I->p ("You open the portable hole.");
                }
                elapsed = FULLTURN;
            } else {
                I->p ("There is nothing there to open.");
                break;
            }
        } else {
            if (f and shFeature::kDoorOpen == f->mType) {
                if (0 != Level->countObjects (x, y)) {
                    I->p ("There's something obstructing it.");
                } else if (Level->isOccupied (x, y)) {
                    I->p ("There's a monster in the way!");
                } else if (0 == closeDoor (x, y)) {
                    I->p ("You fail to close the door.");
                }
                feel (x, y, 1);
                elapsed = HALFTURN; /* it takes less time to slam a door shut */
            } else if (f and shFeature::kDoorClosed == f->mType) {
                int res = closeDoor (x, y);
                if (res) {
                    feel (x, y, 1);
                    elapsed = FULLTURN;
                } else {
                    break;
                }
            } else {
                I->p ("There is nothing there to close.");
                break;
            }
        }
        break;
    }
    case shInterface::kPickup:
    case shInterface::kPickUpAll:
    {
        shObjectVector *v = Level->getObjects (mX, mY);
        shObject *obj;

        int objcnt = v ? v->count () : 0;
        if (0 == objcnt) {
            I->p ("There is nothing on the ground to pick up.");
            break;
        }
        if (isFlying ()) {
            I->p ("You are flying high above the floor.");
            break;
        }
        if (cmd == shInterface::kPickUpAll) {
            int num = objcnt;
            objcnt = 0;
            for (int i = 0; i < num; ++i) {
                obj = v->get (0);
                if (!isBlind()) obj->setAppearanceKnown ();
                /* HACK BEGIN: explained later. */
                v->remove (obj);
                if (!addObjectToInventory (obj)) {
                    /* HACK END: everything is explained in next comment. */
                    Level->putObject (obj, mX, mY);
                } else {
                    if (Level->isInShop (mX, mY)) {
                        pickedUpItem (obj);
                    }
                    ++objcnt;
                }
            }
            if (!v->count ()) {
                Level->setObjects (mX, mY, NULL);
                delete v;
            }
        } else if (1 == objcnt) {
            if (!isBlind ())
                v->get (0) -> setAppearanceKnown ();
            /* HACK: if the object is merged into an inventory object,
               the the floor will momentarily contain a deleted object
               which would be dereferenced during the screen redraw that
               occurs while printing the "you picked up an object"
               message.  So, we preemptively null out the floor objs: */
            Level->setObjects (mX, mY, NULL);
            if (1 == addObjectToInventory (v->get (0))) {
                if (Level->isInShop (mX, mY)) {
                    pickedUpItem (v->get (0));
                }
                delete v;
                v = NULL;
            }
            /* END HACK: Restore the floor. */
            Level->setObjects (mX, mY, v);
        } else {
            int cnt;
            shMenu *menu = I->newMenu ("Pick up what?",
                shMenu::kMultiPick | shMenu::kCountAllowed |
                shMenu::kCategorizeObjects);

            v->sort (&compareObjects);
            for (int i = 0; i < v->count (); ++i) {
                obj = v->get (i);
                if (!isBlind ()) obj->setAppearanceKnown ();
                int let = i % 52;
                menu->addPtrItem (let < 26 ? let + 'a' : let - 26 + 'A',
                                 obj->inv (), obj, obj->mCount);
            }
            objcnt = 0;
            while (menu->getPtrResult ((const void **) &obj, &cnt)) {
                assert (cnt);
                ++objcnt;
                if (!isBlind())
                    obj->setAppearanceKnown ();
                if (cnt != obj->mCount) {
                    obj = obj->split (cnt);
                } else {
                    /* HACK: as above, need to pre-emptively remove the
                       object from the floor vector even though we might
                       not actually pick it up: */
                    v->remove (obj);
                }
                if (0 == addObjectToInventory (obj)) {
                    /* END HACK: put it back. */
                    Level->putObject (obj, mX, mY);
                } else {
                    if (Level->isInShop (mX, mY)) {
                        pickedUpItem (obj);
                    }
                }
            }
            delete menu;
            if (!v->count ()) {
                Level->setObjects (mX, mY, NULL);
                delete v;
            }
        }
        if (!objcnt) { /* Didn't pick anything up means no time taken. */
            break;
        }
        feel (mX, mY);
        elapsed = FULLTURN;
        break;
    }
    case shInterface::kListInventory:
    {
        reorganizeInventory ();
        elapsed = listInventory ();
        break;
    }
    case shInterface::kAdjust:
    {
        adjust (NULL);
        break;
    }
    case shInterface::kDrop:
    {
        shObject *obj;
        int cnt = 1;
        if (0 == mInventory->count ()) {
            I->p ("You aren't carrying anything.");
            break;
        }
        reorganizeInventory ();
        obj = quickPickItem (mInventory, "drop",
                             shMenu::kCountAllowed |
                             shMenu::kCategorizeObjects, &cnt);
        if (!obj) break;
        obj = tryToDrop (obj, cnt);
        if (!obj) break;
        drop (obj);
        feel (mX, mY);
        elapsed = HALFTURN;
        break;
    }

    case shInterface::kDropMany:
    {
        shObject *obj;
        int cnt;
        int ndropped = 0;

        reorganizeInventory ();

        if (0 == mInventory->count ()) {
            I->p ("You aren't carrying anything.");
            goto donedropping;
        } else {
            shMenu *menu = I->newMenu ("Drop what?",
                shMenu::kMultiPick | shMenu::kCountAllowed |
                shMenu::kCategorizeObjects);
            for (int i = 0; i < mInventory->count (); ++i) {
                obj = mInventory->get (i);
                menu->addPtrItem (obj->mLetter, obj->inv (), obj, obj->mCount);
            }

            while (menu->getPtrResult ((const void **) &obj, &cnt)) {
                assert (cnt);
                if (0 == cnt) {
                    I->p ("Error popcorn delta: Menu fault.");
                    break;
                }
                if (obj == mWeapon) {
                    if (obj->isWeldedWeapon () and !isOrc ()) {
                        I->p ("Your weapon is welded to your hands.");
                        obj->setBugginessKnown ();
                        continue;
                    }
                    if (0 == unwield (obj)) {
                        continue;
                    }
                }
                if (obj->isWorn ()) {
                    I->p ("You can't drop %s because you're wearing it.",
                          YOUR (obj));
                    continue;
                }
                obj = removeSomeObjectsFromInventory (obj, cnt);
                drop (obj);
                ++ndropped;
            }
            delete menu;
        }
    donedropping:
        if (!ndropped) { /* didn't drop anything, no time penalty */
            break;
        } else if (1 == ndropped) {
            elapsed = HALFTURN;
        } else {
            elapsed = FULLTURN;
        }
        feel (mX, mY);
        break;
    }

    case shInterface::kLook:
    {
        char *buf = GetBuf ();
        snprintf (buf, SHBUFLEN,
            "Look at what (select a location, '%s' for help)",
            I->getKeyForCommand (shInterface::kHelp));
        int x = -1, y = -1;
        while (I->getSquare (buf, &x, &y, -1, 1))
        {
            shCreature *c = Level->getCreature (x, y);
            shFeature *f = Level->getFeature (x, y);
            int seen = 0;
            I->pageLog ();

            //FIXME: should remember things not in LOS

            if (!canSee (x, y) and Level->rememberedCreature (x, y)) {
                I->p ("You remember an unseen monster there.");
                seen++;
            } else if (c) {
                const char *pcf = "";
                const char *desc = NULL;
                if (c->isHostile ()) {
                    desc = AN (c);
                } else {
                    pcf = c->isPet () ? "a restrained " : "a peaceful ";
                    desc = c->getDescription ();
                }
                if (canSee (c) and c->mHidden <= 0) {
                    const char *wielding = "";
                    const char *weapon = "";
                    char armor[13] = "";
                    char shield[18] = "";
                    if (c->mWeapon) {
                        wielding = ", wielding ";
                        weapon = AN (c->mWeapon);
                    }
                    if (mGoggles and mGoggles->isA (kObjScouterGoggles)) {
                        int AC;
                        if (mGoggles->isBuggy ()) {
                            mGoggles->setBugginessKnown ();
                            AC = 9001 + RNG (999); /* Over nine thousand. :-P */
                        } else {
                            AC = c->mAC;
                        }
                        snprintf (armor, 13, " (AC: %d)", AC);
                        if (c->hasShield ()) {
                            if (mGoggles->mBugginess) {
                                sprintf (shield, " (shields: %d)",
                                    mGoggles->isOptimized () ?
                                    c->countEnergy () : 9001 + RNG (999));
                            } else {
                                sprintf (shield, " (shielded)");
                            }
                        }
                        if (!mGoggles->isKnown ()) {
                            I->p ("You notice a weird number in a corner of your goggles.");
                            mGoggles->setKnown ();
                        }
                    }
                    I->p ("You see %s%s%s%s.%s%s", pcf, desc, wielding, weapon, armor, shield);
                    if (mProfession == Abductor) {
                        if (c->mResistances[kViolating]) {
                            I->p ("It is not even worth probing this one.");
                        } else if (c->mConditions & kNoExp) {
                            I->p ("You have already performed abduction of this one.");
                        }
                    }
                    seen++;
                } else if (canHearThoughts (c)) {
                    I->p ("You sense %s%s.", pcf, desc);
                    seen++;
                } else if (canSenseLife (c)) {
                    I->p ("You feel the presence of a lifeform here.");
                    seen++;
                } else if (canFeelSteps (c)) {
                    I->p ("You feel something moving here.");
                    seen++;
                } else if (canTrackMotion (c)) {
                    I->p ("You see a blip on your motion tracker.");
                    seen++;
                } else if (canSee(c) and c->mHidden > 0) {
                    switch (c->mMimic) {
                    case shCreature::kObject:
                        I->p ("You see %s.", c->mMimickedObject->mVague.mName);
                        seen++;
                        break;
                    default:
                        /* nothing seen */
                        break;
                    }
                }
            }
            if (hasMotionDetection () and distance (this, x, y) < 50 and
                !isBlind () and f and f->isDoor () and
                f->mDoor.mMoveTime + SLOWTURN >= Clock)
            { /* Automatic doors also show up on motion tracker. */
                I->p ("You see a blip on your motion tracker.");
                seen++;
            }
            if (!canSee (x, y)) {
                if (isBlind ()) {
                    I->p ("You are blind and cannot see.");
                } else if (!seen) {
                    I->p ("You can't see that location from here.");
                }
                continue;
            }

            if (Level->countObjects (x, y) and !Level->isWatery (x, y)) {
                shObjectVector *objs = Level->getObjects (x, y);
                shObject *bestobj = findBestObject (objs);
                I->p ("You see %s.", AN (bestobj));
                int cnt = objs->count ();
                if (cnt > 1) {
                    I->p ("You see %d more object%s underneath.",
                          cnt-1, cnt > 2 ? "s" : "");
                }
                seen++;
            }
            if (f and !(f->isTrap () and f->mTrapUnknown)) {
                switch (f->mType) {
                case shFeature::kDoorHiddenVert:
                case shFeature::kDoorHiddenHoriz:
                case shFeature::kMovingHWall:
                    I->p ("You see a wall."); break;
                case shFeature::kStairsUp:
                    I->p ("You see a staircase leading upstairs."); break;
                case shFeature::kStairsDown:
                    I->p ("You see a staircase leading downstairs."); break;
                case shFeature::kDoorClosed:
                case shFeature::kDoorOpen:
                case shFeature::kVat:
                    I->p ("You see %s.", f->getDescription ()); break;
                case shFeature::kPit:
                    I->p ("You see a pit."); break;
                case shFeature::kAcidPit:
                    I->p ("You see an acid-filled pit."); break;
                case shFeature::kTrapDoor:
                    I->p ("You see a trap door."); break;
                case shFeature::kHole:
                    I->p ("You see a hole."); break;
                case shFeature::kWeb:
                    I->p ("You see a web."); break;
                case shFeature::kRadTrap:
                    I->p ("You see a radiation trap."); break;
                case shFeature::kSewagePit:
                    I->p ("You see a dangerously deep pool of sewage."); break;
                case shFeature::kMachinery:
                    I->p ("You see pistons and machinery."); break;
                case shFeature::kPortableHole:
                    I->p ("You see a closed portable hole."); break;
                case shFeature::kPortal:
                case shFeature::kComputerTerminal:
                case shFeature::kMaxFeatureType:
                    I->p ("You see something very buggy."); break;
                }
            } else {
                if (Level->isFloor (x, y)) {
                    if (kBrokenLightAbove == Level->getSquare(x,y)->mTerr) {
                        I->p ("You see a broken light under the ceiling.");
                    } else if (kSewage == Level->getSquare(x,y)->mTerr) {
                        I->p ("You see a pool of sewage.");
                    } else if (!seen) {
                        I->p ("You see the floor.");
                    }
                } else if (kVoid == Level->getSquare (x, y) ->mTerr) {
                    I->p ("You see a yawning void.");
                } else {
                    I->p ("You see a wall.");
                }
            }
        }
        I->p ("Done.");
        break;
    }

    case shInterface::kPay:
    {
        payShopkeeper ();
        break;
    }

    case shInterface::kQuaff:
    {
        shObject *obj;
        shObjectVector v;

        if (getStoryFlag ("superglued tongue")) {
            I->p ("You can't drink anything with this stupid canister "
                  "glued to your mouth!");
            break;
        }

        shFeature *f = Level->getFeature (mX, mY);
        if (f and shFeature::kVat == f->mType and !isFlying () and
            I->yn ("Drink from the vat?"))
        {
            quaffFromVat (f);
            elapsed = FULLTURN;
        } else if (f and shFeature::kAcidPit == f->mType and !isFlying () and
            I->yn ("Drink from the acid pit?"))
        {
            quaffFromAcidPit ();
            elapsed = FULLTURN;
        } else {
            selectObjectsByFunction (&v, mInventory, &shObject::canBeDrunk);
            obj = quickPickItem (&v, "quaff", 0);
            if (!obj) break;
            elapsed = quaffCanister (obj);
        }
        break;
    }
    case shInterface::kSwap:
    {
        shObject *swap = NULL;
        for (int i = 0; i < mInventory->count (); ++i) {
            if (mInventory->get (i)->isPrepared ()) {
                swap = mInventory->get (i);
                break;
            }
        }
        if (mWeapon and mWeapon->isWeldedWeapon () and !isOrc ()) {
            if (mWeapon->isBugginessKnown ()) {
                I->p ("You cannot switch away from buggy weapon.");
                break;
            }
            I->p ("You can't let go of %s!  It must be buggy.", YOUR (mWeapon));
            mWeapon->setBugginessKnown ();
            elapsed = HALFTURN;
            break;
        }
        elapsed = HALFTURN;
        shObject *weap = mWeapon;
        if (mWeapon and !Hero.unwield (mWeapon)) {
            I->p ("Somehow, you can't let go of %s.", YOUR (mWeapon));
            break;
        }
        /* You can have only one item prepared. */
        for (int i = 0; i < mInventory->count (); ++i) {
            mInventory->get (i)->resetPrepared ();
        }
        if (weap)  {
            weap->setPrepared ();
            weap->announce ();
        }
        if (swap) {
            swap->resetPrepared ();
            Hero.wield (swap, 1);
            swap->announce ();
        } else {
            I->p ("You are now unarmed.");
        }
        break;
    }
    case shInterface::kUse:
    {
        if (HeroIsWebbedOrTaped ())  break;
        shObjectVector v;

        selectObjectsByFunction (&v, mInventory, &shObject::isUseable);
        shObject *obj = quickPickItem (&v, "use", shMenu::kCategorizeObjects);
        if (!obj) {
            break;
        }
        elapsed = obj->myIlk ()->mUseFunc (obj);
        break;
    }
    case shInterface::kDiscoveries:
    {
        listDiscoveries ();
        break;
    }
    case shInterface::kEditSkills:
    {
        editSkills ();
        break;
    }
    case shInterface::kExamine:
    {
        shObject *obj = quickPickItem (mInventory, "examine",
            shMenu::kCategorizeObjects);
        if (obj) obj->showLore ();
        break;
    }
    case shInterface::kExecute:
    {
        shObject *computer = chooseComputer ();
        if (!computer) break;
        if (!HeroCanExecute (computer))  break;
        elapsed = computer->myIlk ()->mUseFunc (computer);
        break;
    }
    case shInterface::kWield:
    {
        shObject *obj;
        shObjectVector v1, v2;

        selectObjectsByFunction (&v1, mInventory, &shObject::isKnownWeapon);
        unselectObjectsByFunction (&v2, &v1, &shObject::isWielded);
        v1.reset ();
        unselectObjectsByFunction (&v1, &v2, &shObject::isWorn);
        obj = quickPickItem (&v1, "wield", shMenu::kFiltered |
                                           shMenu::kCategorizeObjects);
        if (!obj or !wield (obj)) {
            break;
        }
        elapsed = HALFTURN;
        break;
    }
    case shInterface::kWear:
    {   /* One can wear armor and install bionic implants. */
        shObjectVector v1, v2;
        selectObjectsByFunction (&v1, mInventory, &shObject::canBeWorn);
        unselectObjectsByFunction (&v2, &v1, &shObject::isWorn);
        v1.reset ();
        unselectObjectsByFunction (&v1, &v2, &shObject::isDesignated);
        /* Pick something. */
        shObject *obj = quickPickItem (&v1, "wear", shMenu::kCategorizeObjects);
        if (!obj) break;
        elapsed = doWear (obj);
        break;
    }
    case shInterface::kTakeOff:
    {   /* Collect worn, installed and wielded stuff. */
        shObjectVector v;
        selectObjectsByFunction (&v, mInventory, &shObject::isEquipped);
        /* Choose item to unequip. */
        shObject *obj = quickPickItem (&v, "remove", shMenu::kCategorizeObjects);
        if (!obj) break;
        elapsed = doTakeOff (obj);
        break;
    }
    case shInterface::kMutantPower:
    {
        elapsed = useMutantPower ();
        break;
    }
    case shInterface::kName:
    {
        nameObject (NULL); /* This will prompt player to choose one. */
        break;
    }
    case shInterface::kToggleAutopickup:
        Flags.mAutopickup = !Flags.mAutopickup;
        I->p ("Autopickup is now %s.", Flags.mAutopickup ? "ON" : "OFF");
        break;
    case shInterface::kMainMenu:
    {
        shMenu *menu = I->newMenu ("Game menu:", shMenu::kNoHelp);
        menu->addIntItem ('s', "Save and quit", 1);
        menu->addIntItem ('o', "Set options", 2);
        menu->addIntItem ('v', "See game version", 3);
        menu->addIntItem ('h', "See high score table", 4);
        menu->addIntItem ('Q', "Quit without saving", 5);
        int res;
        menu->getIntResult (&res);
        delete menu;
        switch (res) {
        case 1:
            if (0 == saveGame ()) {
                I->p ("Game saved.  Press enter.");
                GameOver = 1;
                I->pause ();
                return;
            } else {
                I->p ("Save game operation failed!");
            }
            break;
        case 2:
            I->editOptions (); break;
        case 3:
            I->showVersion (); break;
        case 4:
            I->showHighScores (); break;
        case 5:
            if (I->yn ("Really quit?")) {
                shCreature::die (kQuitGame, "quit");
                return;
            } else {
                I->nevermind ();
            }
            break;
        }
       break;
    }

    /* debugging commands */
    case shInterface::kBOFHPower:
        if (!BOFH) {
            I->p ("You have to be Bastard Operator From Hell to use this.");
            break;
        }
        useBOFHPower ();
        break;

    default:
//      I->p ("Impossible command.  The game is corrupt!");
        break;
    }

    if (0 == elapsed and !GameOver) {
        goto getcmd;
    } else if (elapsed > 0) {
        mAP -= elapsed;
    }

    return;
}

void
shHero::torcCheck ()
{
    if (!getStoryFlag ("worn torc")) {
        setStoryFlag ("worn torc", 1);
        if ((!RNG (10) or (mProfession == Psion and !RNG (3)))
            and getMutantPower ())
        {
            I->p ("The torc seems to have unlocked your latent powers.");
        } else {
            I->p ("A thrill passes down your spine.");
        }
    }
}

void
shHero::abortion (int quiet)
{
    if (kDead != mState and getStoryFlag ("impregnation")) {
        if (!quiet) {
            I->p ("The parasitic alien inside you is killed!");
        }
        setStoryFlag ("impregnation", 0);
        earnXP (MonIlks[kMonFacehugger].mBaseLevel);
        ++MonIlks[kMonAlienEmbryo].mKills;
    }
}

void
shHero::amputateTail (void)
{   /* This code fragment relies on the tail attack being number 4. */
    Hero.myIlk ()->mAttackSum -= Hero.myIlk ()->mAttacks[3].mProb;
    Hero.myIlk ()->mAttacks[3].mProb = 0;
    Hero.myIlk ()->mAttacks[3].mAttId = kAttDummy;
}


int
shHero::die (shCauseOfDeath how, shCreature *killer, shObject *implement,
    shAttack *attack, const char *killstr)
{
    int won = 0;

    char newreason[100];
    if (killer) {
        resetBlind (); /* Yes, do it for Xel'Naga too. */
        Level->setLit (killer->mX, killer->mY, 1, 1, 1, 1);
        if (this == killer) {
            how = kKilled;
            killer = NULL;
            strcpy (newreason, her ("own weapon"));
        } else {
            strcpy (newreason, AN (killer));
        }
        killstr = newreason;
    }

    mState = kDead;
    if (hasAcidBlood () and kSlain == how) {
        shAttackId attid = mCLevel >= 15 ? kAttAcidBlood2 : kAttAcidBlood1;
        Level->areaEffect (attid, NULL, mX, mY, kOrigin, this);
    }
    int glory_devices = 0;
    for (int i = 0; i < mInventory->count (); ++i) {
        shObject *obj = mInventory->get (i);
        if (obj->isA (kObjGloryDevice) and obj->isWorn ()) {
            ++glory_devices;
        }
    }
    if (isExplosive () and (kSlain == how or kSuicide == how)) {
        shAttackId atk;
        if (glory_devices) {
            atk = shAttackId (kAttGlory1 + glory_devices - 1);
        } else {
            atk = kAttExplodingMonster;
        }
        Level->areaEffect (atk, NULL, mX, mY, kOrigin, this, mCLevel);
    }

    Level->computeVisibility ();
    I->drawScreen ();
    switch (how) {
    case kSlain:
    case kKilled:
    case kMisc:
    case kSuicide:
        I->p ("You die."); break;
    case kBrainJarred:
        I->p ("You are now a brain in a jar."); break;
    case kDrowned:
        I->p ("You drown."); break;
    case kAnnihilated:
        I->p ("You are annihilated."); break;
    case kSuddenDecompression:
        switch (RNG (3)) { /* I just couldn't decide on one message. -- CADV */
        case 0:
            I->p ("Your lungs explode painfully."); break;
        case 1:
            I->p ("Your blood boils, enveloping your body in a crimson mist.");
            break;
        case 2:
            I->p ("Your internal organs rupture and your eyeballs pop."); break;
        }
        break;
    case kTransporterAccident:
        I->p ("Your molecules are scrambled into a pile of gray goo."); break;
    case kQuitGame:
        break;
    case kWonGame:
        won = 1;
        I->p ("Congratulations, you are the baddest motherfucker "
              "in the galaxy now!");
        break;
    }

    if (!won and SpaceMarine == mProfession) { /* YAFM */
        I->p ("Game over, man!  Game over!");
    }
    /* Do it now, before DYWYPI spoils the beautiful screen to be captured. */
    postMortem ();
    char message[200];
    epitaph (message, 200, how, killstr, killer);
    logGame (message);

    I->pause ();

    Level->clearSpecialEffects ();

    shMenu *query = I->newMenu ("Do you want ...", 0);
    query->addIntItem ('i', "... your possessions identified?", 1, 1);
    query->addIntItem ('m', "... to see complete map of current area?", 2, 1);
    query->addIntItem ('d', "... your diagnostics revealed?", 3, 1);
    query->addIntItem ('h', "... to see the console message history?", 4, 1);
    query->addIntItem ('k', "... to read the kill list?", 5, 1);
    query->addIntItem ('s', "... to view the high score table?", 6, 1);

    GameOver = 1;
    int r;
    while (query->getIntResult (&r)) {
        switch (r) {
        case 1:
            for (int i = 0; i < mInventory->count (); ++i) {
                mInventory->get (i) -> identify ();
            }
            listInventory ();
            break;
        case 2:
            for (int x = 0; x < MAPMAXCOLUMNS; ++x) {
                for (int y = 0; y < MAPMAXROWS; ++y) {
                    shFeature *f = Level->getFeature (x, y);
                    if (f)  f->mTrapUnknown = 0;
                    Level->mVisibility[x][y] = 1;
                    Level->setLit (x, y);
                }
            }
            I->drawScreen ();
            I->pause ();
            break;
        case 3:
            doDiagnostics (0, kWonGame == how);
            break;
        case 4:
            I->showHistory ();
            break;
        case 5:
            I->showKills ();
            break;
        case 6:
            I->showHighScores ();
            break;
        }
        query->dropResults ();
    }
    delete query;

    if (kWonGame != how and kQuitGame != how) tomb (message);

    I->p ("Goodbye...");

    if (kWonGame != how) {
        debug.log ("Hero is slain!");
    }
    Level->removeCreature (this);
    return 1;
}

//REQUIRES: monster types initialized already
void
shHero::init (const char *name, shProfession *profession)
{
    int x = 1, y = 1;

    strncpy (mName, name, HERO_NAME_LENGTH);
    mName[HERO_NAME_LENGTH] = 0;
    mCLevel = 1;

    profession->mInitFunction (this);
    profession->postInit (this);
    memcpy (mInnateResistances, myIlk ()->mInnateResistances,
            sizeof (mInnateResistances));
    mInnateIntrinsics |= myIlk ()->mInnateIntrinsics;
    mInnateResistances[kMagnetic] = 122;

    mAP = -1000;
    mHP = mMaxHP;

    computeIntrinsics (true);
    computeAC ();

    mScore = 0;
    mSkillPoints = 0;
    mXP = 0;

    do {
        Level->findUnoccupiedSquare (&x, &y);
    } while (Level->isInShop (x, y) or !Level->isInRoom (x, y));
    Level->putCreature (this, x, y);
}

shHero Hero;
