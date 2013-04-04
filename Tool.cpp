#include <ctype.h>
#include "Global.h"
#include "Util.h"
#include "Object.h"
#include "Hero.h"

int
shCreature::attemptRestraining (shObject *bolt)
{
    if (isA (kMonClerkbot) or isA (kMonDocbot)) { /* nice try */
        I->p ("The restraining bolt is vaporized by %s's "
              "anti-shoplifting circuits!", the ());
        newEnemy (&Hero);
        delete bolt;
        return 0;
    }
    addObjectToInventory (bolt);
    if (!isPet ()) ((shMonster *) this) -> makePet ();
    return 1;
}

static int
useRestrainingBolt (shObject *bolt)
{
    shDirection dir;
    int x = Hero.mX;
    int y = Hero.mY;
    shMonster *c;
    int difficulty;

    dir = I->getDirection ();
    switch (dir) {
    case kNoDirection:
    case kUp:
    case kDown:
        return 0;
    default:
        if (!Level->moveForward (dir, &x, &y)) {
            return 0;
        }
        c = (shMonster *) Level->getCreature (x, y);
        if (!c or !c->isRobot ()) {
            I->p ("Restraining bolts only work on bots and droids.");
            return 0;
        }
        if (c->isPet ()) {
            bool bolt = false;
            for (int i = 0; i < c->mInventory->count (); ++i)
                if (c->mInventory->get (i)->isA (kObjRestrainingBolt)) {
                    bolt = true;
                    break;
                }
            if (bolt) {
                I->p ("You've already affixed a restraining bolt to %s.", THE (c));
            } else {
                I->p ("There is no need to restrain %s.", THE (c));
            }
            return 0;
        }

        /* low-level bots should be automatic, warbots should be quite
           difficult to tame! */
        difficulty = bolt->isOptimized () ? 6 :
                     bolt->isBuggy ()     ? 2 :
                                            3;

        if (c->mCLevel > difficulty and RNG (c->mCLevel) > difficulty) {
            I->p ("You miss %s.", THE (c));
            c->newEnemy (&Hero);
        } else {
            Hero.usedUpItem (bolt, 1, "attach");
            bolt = Hero.removeOneObjectFromInventory (bolt);
            I->p ("You attach the restraining bolt to %s.", THE (c));
            c->attemptRestraining (bolt);
        }
        return FULLTURN;
    }
}


static void
flail (shObject *md, const char *what)
{
    I->p ("You flail %s with your limp %s.", what, md->getShortDescription ());
    I->p ("For some reason it doesn't appear to accomplish anything.");
}


static int
repairObject (shObject *tool)
{
    shObjectVector v;
    selectObjectsByFunction (&v, Hero.mInventory, &shObject::isDamaged);
    if (!v.count ()) {
        I->p ("You have nothing to repair.");
        return 0;
    }
    shObject *obj = Hero.quickPickItem (&v, "repair", 0);
    shObject *material;
    if (!obj) return 0;
    if (obj->myIlk ()->mMaterial == kFleshy) {
        I->p ("Repair a piece of meat?  How?");
        return 0;
    }
    if (!obj->isKnown () or !obj->isAppearanceKnown ()) {
        I->p ("But you don't surely know what it really is.");
        return 0;
    }
    if (obj->isWorn ()) {
        I->p ("Take it off first.");
        return 0;
    }
    if (tool->isA (kObjMechaDendrites) and !tool->isWorn ()) {
        flail (tool, obj->theQuick ());
        return FULLTURN;
    }
    material = Hero.quickPickItem (Hero.mInventory, "get material from", 0);
    if (!material) {
        I->p ("You cannot repair without spare parts.");
        return 0;
    }
    if (obj == material) {
        I->p ("You cannot repair %s with itself.", THE (obj));
        return 0;
    } else if (material->isWorn ()) {
        I->p ("Take it off first.");
        return 0;
    } else if (!material->isAppearanceKnown ()) {
        I->p ("But you don't surely know what the material is made of.");
        return 0;
    } else if (material->myIlk ()->mMaterial == kFleshy) {
        I->p ("Piece of meat as material? What are you thinking?");
        return 0;
    } /* Now the complex rules begin. */
    if (obj->isThrownWeapon ()) {
        if (!material->isThrownWeapon ()) {
            I->p ("%s is too dissimilar.", THE (material));
            return 0;
        }
    } else if (obj->isMeleeWeapon ()) {
        if (obj->myIlk ()->mMaterial != material->myIlk ()->mMaterial) {
            I->p ("To repair a melee weapon you need more of the material its made of.");
            return 0;
        }
    } else if (obj->isAimedWeapon ()) {
        if (!(material->isAimedWeapon ())) {
            I->p ("You need to repair it with another ranged weapon.");
            return 0;
        }
        if ((obj->isA (kObjLightFlamer) or obj->isA (kObjFlamer)) and
            (!material->isA (kObjLightFlamer) and !material->isA (kObjFlamer)))
        {
            I->p ("To repair a flamethrower you need another one.");
            return 0;
        } else {
            if (obj->myIlk ()->mParent != material->myIlk ()->mParent) {
                I->p ("These weapons belong to different class.");
                return 0;
            }
        }
    } else if (obj->isA (kArmor)) {
        if (obj->isA (kObjGenericJumpsuit)) {
            if (!material->isA (kObjGenericJumpsuit)) {
                I->p ("You need another jumpsuit.");
                return 0;
            }
            if (obj->isA (kObjChameleonSuit) and
                !material->isA (kObjChameleonSuit))
            {
                I->p ("Chameleon suits must be repaired using"
                      " patches from another one.");
                I->p ("Otherwise its camouflaging properties"
                      " would be diminished.");
                return 0;
            } else if (obj->isA (kObjRadiationSuit) and
                       !material->isA (kObjRadiationSuit))
            {
                I->p ("Radiation suits must be repaired using"
                      " patches from another one.");
                I->p ("Otherwise its protective properties"
                      " would be diminished.");
                return 0;
            } /* Otherwise ok. */
        } else if (obj->isA (kObjGenericArmor) or
                   obj->isA (kObjGenericHelmet))
        {   /* Compatibility rules:
                - type match: helmet <-> helmet or body armor <-> body armor
                - powered match: both powered or none
                - material type match */
            if (obj->myIlk ()->mParent != material->myIlk ()->mParent) {
                I->p ("Armor types do not match.");
                return 0;
            }
            int powered = obj->isPoweredArmor () + material->isPoweredArmor ();
            if (powered % 2) {
                I->p ("One of those armor pieces is powered.");
                I->p ("Either both of those should be powered or none of them.");
                return 0;
            }
            if (obj->myIlk ()->mMaterial != material->myIlk ()->mMaterial) {
                I->p ("The material types do not match.");
                return 0;
            }
        } else {
            I->p ("This armor cannot be repaired.");
            return 0;
        }
    } else {
        I->p ("This object does not have repair handle. Plase report this bug.");
        return 0;
    }
    int rad = obj->isRadioactive () + material->isRadioactive ();
    /* Finally, repair. */
    Hero.removeObjectFromInventory (material);
    delete material;
    if (rad) {
        shObjectIlk *rad_gr = &AllIlks[kObjRadGrenade];
        for (int i = 0; i < rad; ++i) {
            Hero.sufferDamage (rad_gr->mMissileAttack);
        }
    }
    if (sportingD20 () + Hero.getSkillModifier (kRepair) > 15) {
        obj->mDamage = 0;
        I->p ("Now %s looks as good as new!", THE (obj));
    } else {
        I->p ("Your repair attempt is a failure.");
    }
    return 1;
}


static int
makeRepair (shObject *tool)
{
    int x = Hero.mX, y = Hero.mY;
    shFeature *f;
    shMonster *c;
    shObjectVector *v;
    int flailing = 0;
    bool ducttape = false;
    int basescore = Hero.getSkillModifier (kRepair) + 2 * tool->mBugginess;

    if (tool->isA (kObjDuctTape)) {
        basescore += 8;
        ducttape = true;
    } else if (tool->isA (kObjMonkeyWrench)) {
        basescore += tool->mEnhancement;
    }
    if (tool->isA (kObjMechaDendrites) and !tool->isWorn ()) {
        flailing = 1;
    }
    int score = RNG (1, 20) + basescore;
    I->diag ("Make a repair score: base %d, with roll %d.", basescore, score);

    if (ducttape)  I->p ("Select yourself to repair items in inventory.");
    shDirection dir = I->getDirection ();
    switch (dir) {
    case kNoDirection:
        return 0;
    case kUp:
        if (Level->mSquares[Hero.mX][Hero.mY].mTerr == kBrokenLightAbove) {
            if (flailing) {
                flail (tool, "the broken light");
                return FULLTURN;
            } else if (Hero.mProfession == Janitor) {
                int room = Level->getRoomID (Hero.mX, Hero.mY);
                int sx = Hero.mX, sy = Hero.mY, ex = Hero.mX, ey = Hero.mY;
                Level->getRoomDimensions (room, &sx, &sy, &ex, &ey);
                for (int y = sy; y <= ey; ++y)
                    for (int x = sx; x <= ex; ++x)
                        Level->setLit (x, y);
                Level->mSquares[Hero.mX][Hero.mY].mTerr = kFloor;
                I->p ("You replace the missing lightbulb.");
                if (tool->isA (kObjMechaDendrites)) {
                    I->p ("Screwing it with %s was fun.", YOUR (tool));
                }
                Hero.earnXP (Level->mDLevel);
                break;
            } else {
                I->p ("The lightbulb is missing.  Call janitor to bring a spare.");
                return 0;
            }
        }
        I->p ("There's nothing to repair on the ceiling.");
        return 0;
    case kOrigin:
        if (!ducttape) {
            if (!repairObject (tool)) {
                return 0;
            } else {
                break;
            }
        }
    default:
        if (!Level->moveForward (dir, &x, &y)) {
            I->p ("There's nothing there to repair.");
            return 0;
        }
        c = (shMonster *) Level->getCreature (x, y);
        if (c and dir != kDown and ducttape) {
            I->p ("You tape %s.", c->isHero () ? "yourself" : THE (c));
            c->mTrapped.mTaped += NDX (2, 2) + tool->mBugginess;
            break;
        }
        if (c and dir != kDown) {
            /* repair a pet droid */
            const char *who = THE (c);
            if (c->isHero ()) {
                I->p ("You can't make repairs to yourself!");
                return 0;
            } else if (!c->isRobot ()) {
                I->p ("You can't make repairs to %s!", who);
                return 0;
            } else if (c->mHP == c->mMaxHP) {
                I->p ("%s doesn't appear to be damaged.", who);
                return 0;
            } else if (!c->isPet ()) { /* Replace with mDisposition check? */
                I->p ("%s won't cooperate with your attempt to repair it.",
                      who);
                return FULLTURN;
            } else if (flailing) {
                flail (tool, who);
                return FULLTURN;
            } else if (score < 15) {
                I->p ("Your repair attempt is a failure.");
            } else {
                c->mHP += score - 15 + RNG (1, 3);
                if (c->mHP >= c->mMaxHP) {
                    c->mHP = c->mMaxHP;
                    I->p ("Now %s looks as good as new!", who);
                } else {
                    I->p ("You repair some of the damage on %s.", who);
                }
            }
            break;
        }
        if ((v = Level->getObjects (x, y))) {
            int i;
            shObject *obj;
            int nx = x;
            int ny = y;
            int repaired = 0;

            for (i = 0; i < v->count (); i++) {
                obj = v->get (i);
                if (obj->isA (kObjWreck)) {
                    if (nx == Hero.mX and ny == Hero.mY and
                        -1 == Level->findNearbyUnoccupiedSquare (&nx, &ny))
                    {
                        I->p ("There is %s here, but you need more room "
                              "to repair it.", AN (obj));
                        break;
                    }
                    if (I->yn ("Repair %s?", THE (obj))) {
                        if (flailing) {
                            flail (tool, THE (obj));
                            return FULLTURN;
                        } else if (score < 20) {
                            I->p ("Your repair attempt is a failure.");
                        } else {
                            shMonster *bot = new shMonster (obj->mWreckIlk);
                            /* Prevent XP farming. */
                            bot->mConditions |= kNoExp;
                            /* Prevent drop farming. */
                            for (int i = bot->mInventory->count () -1; i >= 0; --i) {
                                shObject *obj = bot->mInventory->get (i);
                                bot->removeObjectFromInventory (obj);
                                delete obj;
                            }
                            Level->putCreature (bot, nx, ny);
                            I->p ("You bring %s back on-line!", THE (obj));
                            v->remove (obj);
                            delete obj;
                        }
                        repaired = 1;
                        break;
                    }
                }
            }
            if (repaired) break;
        }

        if ((f = Level->getFeature (x, y))) {
            if (f->isAutomaticDoor () and f->isBerserkDoor () and
                !f->mTrapUnknown)
            {
                if (flailing) {
                    flail (tool, "the door");
                } else if (score >= 12) {
                    f->mDoor.mFlags &= ~shFeature::kBerserk;
                    I->p ("You repair the malfunctioning door.");
                    if (Hero.mProfession == Janitor) {
                        /* A shout out to ADOM. :) */
                        I->p ("You feel like behaving properly.");
                        /* For doing what you should be. */
                        Hero.earnXP (Level->mDLevel);
                    }
                    Level->remember (x, y, f); /* For blind chars. */
                } else {
                    I->p ("You repair attempt is a failure.");
                }
                break;
            } else if (f->isLockBrokenDoor ()) {
                if (flailing) {
                    flail (tool, "the broken lock");
                } else if (score >= 15) {
                    f->mDoor.mFlags &= ~shFeature::kLockBroken;
                    I->p ("You repair the broken lock.");
                    /* No Janitor bonus.  This could be abused. */
                } else {
                    I->p ("You repair attempt is a failure.");
                }
                break;
            } else if (f->isAutomaticDoor () and f->isInvertedDoor ()) {
                if (flailing) {
                    flail (tool, "the inverted logic circuits");
                } else if (score >= 15) {
                    f->mDoor.mFlags &= ~shFeature::kInverted;
                    I->p ("You rewire the inverted logic circuits.");
                    if (Hero.mProfession == Janitor) {
                        Hero.earnXP (Level->mDLevel);
                    }
                } else {
                    I->p ("You repair attempt is a failure.");
                }
                break;
            } else if (shFeature::kRadTrap == f->mType and !f->mTrapUnknown) {
                const int raddiff = 24;
                if (dir != kDown) {
                    I->p ("You need to stand closer.");
                    return 0;
                } else if (flailing) {
                    flail (tool, "the poor rad trap");
                } else if (score >= raddiff) {
                    I->p ("You dismantle the trap.");
                    Level->removeFeature (f);
                } else {
                    int chance = (basescore - raddiff + 20) * 5;
                    I->p ("Whoops!  This task is %s",
                          chance <= 0 ? "impossible!" :
                          chance <= 25 ? "difficult." :
                          chance <= 50 ? "supposed to be doable." :
                          chance <= 75 ? "supposed to be easy." :
                                         "supposed to be trivial matter!");
                    Level->checkTraps (Hero.mX, Hero.mY, 100);
                }
                break;
            } else if ((shFeature::kPit == f->mType or
                        shFeature::kAcidPit == f->mType or
                        shFeature::kHole == f->mType) and ducttape)
            {
                if (Hero.mZ == -1) {
                    I->p ("Climb out first.");
                    return 0;
                }
                I->p ("You tape %s opening.  Looks almost like a solid ground.",
                    f->mType == shFeature::kHole ? "hole" : "pit");
                /* Make sure no intelligent enemy saw you at work. */
                int observers = 0;
                for (int i = 0; i < Level->mCrList.count (); ++i) {
                    shCreature *c = Level->mCrList.get (i);
                    if (c and c->mAbil.mInt > 6 and c->canSee (&Hero) and
                        !c->isHero () and !c->isPet ())
                    {
                        ++observers;
                        if (Hero.canSee (c))
                            I->p ("%s watches you closely.", THE (c));
                    }
                } /* It shall be conveniently assumed monsters had plenty
                     of time to forget the trap existed in the first place. */
                if (!observers)  f->mTrapMonUnknown = 2;
                break;
            } else {
                I->p ("It ain't broke!");
                return 0;
            }
        }
        I->p ("There's nothing there to repair.");
        return 0;
    }
    if (tool->isA (kObjMonkeyWrench) or
        (tool->isA (kObjMechaDendrites) and !flailing)) {
        /* This is intentionally a ripoff in case of wrench. */
        Hero.employedItem (tool, 10);
    }
    if (flailing) {
        Hero.employedItem (tool, 0); /* This leads to a YAFM. */
    }
    /* Superglue will get used up in the useCanister () function. */
    if (ducttape)
        Hero.useUpOneObjectFromInventory (tool);

    /* Yeah, realistically this should take longer, but that wouldn't
       be very fun for the player I think. -- CADV */
    return FULLTURN;
}


static int
useDuctTape (shObject *tape)
{
    if (Hero.mWeapon and Hero.mWeapon->isWeldedWeapon ()) {
        I->p ("You cannot free both your hands.");
        if (!Hero.mWeapon->isBugginessKnown ()) {
            Hero.mWeapon->setBugginessKnown ();
            Hero.mWeapon->announce ();
        }
        return 0;
    }
    return makeRepair (tape);
}

static int
useMechaDendrites (shObject *dendrites)
{
    return makeRepair (dendrites);
}

static int
useMonkeyWrench (shObject *wrench)
{
    if (Hero.mWeapon and Hero.mWeapon != wrench and
        Hero.mWeapon->isWeldedWeapon ())
    {
        I->p ("You cannot free both your hands.");
        if (!Hero.mWeapon->isBugginessKnown ()) {
            Hero.mWeapon->setBugginessKnown ();
            Hero.mWeapon->announce ();
        }
        if (Hero.mWeapon->isA (kObjMonkeyWrench))
            I->p ("But why not use the wrench you are holding instead?");
        return 0;
    }
    int elapsed = makeRepair (wrench);
    if (elapsed and Hero.mWeapon != wrench and wrench->isBuggy () and
        !Hero.isOrc ())
    {
        if (Hero.mWeapon)  Hero.unwield (Hero.mWeapon);
        Hero.wield (wrench, 1);
        bool wasknown = wrench->isBugginessKnown ();
        wrench->setBugginessKnown ();
        I->p ("You cannot put away %s%c", YOUR (wrench), wasknown ? "." : "!");
    }
    return elapsed;
}


int
shHero::useKey (shObject *key, shFeature *door)
{
    int elapsed = 0;
    int locked = door->isLockedDoor ();
    const char *does_what = locked ? "unlocks" : "locks";
    shObjectIlk *cardneeded = door->keyNeededForDoor ();

    if (shFeature::kDoorHiddenHoriz == door->mType or
        shFeature::kDoorHiddenVert == door->mType)
    {
        I->p ("There's no lock there.");
        return 0;
    }
    if (door->isLockBrokenDoor ()) {
        I->p ("The lock on this door is broken.");
        return QUICKTURN;
    }
    if (!cardneeded and !door->isRetinaDoor ()) {
        I->p ("There is no lock on this door.");
        return QUICKTURN;
    }
    if (door->isOpenDoor ()) {
        I->p ("You have to close it first.");
        return 0;
    }

    if (cardneeded and key->isA (kObjGenericKeycard)) {
        const char *key_d = key->getShortDescription ();
        if (isBlind ()) {
            if (!key->isBuggy () and
                (key->isA (kObjMasterKeycard) or
                 key->myIlk () == cardneeded or
                 key->isCracked ()))
            {
                I->p ("You swipe your %s and the door lock accepts it.", key_d);
                if (key->isAppearanceKnown () and /* You know keycard color. */
                /* The keycard is not cracked or you are unaware of the fact. */
                    (!key->isCracked () or !key->isCrackedKnown ()) and
                /* This is not master keycard or you are unaware of the fact. */
                    (!key->isA (kObjMasterKeycard) or !key->isKnown ()))
                { /* You infer the door lock must match color of keycard. */
                    int codeflag = door->mDoor.mFlags & shFeature::kColorCode;
                    Level->mRemembered[door->mX][door->mY].mDoor &= ~shFeature::kColorCode;
                    Level->mRemembered[door->mX][door->mY].mDoor |= codeflag;
                }
            } else {
                I->p ("You swipe your %s but it seems not to work.", key_d);
                return QUICKTURN;
            }
        } else if (key->isBuggy () and key->isBugginessKnown ()) {
            I->p ("But your %s is buggy.", key_d);
            return 0;
        } else if (key->myIlk () == cardneeded) {
            if (key->isBuggy ()) {
                I->p ("You swipe %s but the door does not react.", key_d);
                I->p ("It must be buggy.");
                key->setBugginessKnown ();
                return QUICKTURN;
            }
            I->p ("You swipe your %s and the door %s.", key_d, does_what);
        } else if (key->isBuggy ()) {
            I->p ("You swipe your %s but nothing happens.", key_d);
            if ((key->isA (kObjMasterKeycard) and key->isKnown ()) or
                (key->isCracked () and key->isCrackedKnown ()))
            {
                I->p ("Damn it!  This keycard must be buggy.");
                key->setBugginessKnown ();
            } else {
                I->p ("Obviously the keycard and lock colors do not match.");
            }
            return QUICKTURN;
        } else if (key->isA (kObjMasterKeycard)) {
            I->p ("You swipe your %s and the door %s.", key_d, does_what);
            if (!key->isKnown () and key->myIlk () != cardneeded) {
                I->p ("This must be the master keycard.");
                key->setKnown ();
            }
        } else if (key->isCracked ()) {
            if (key->isCrackedKnown ()) {
                I->p ("You swipe your %s and the door %s.", key_d, does_what);
            } else {
                I->p ("You swipe your %s.  To your surprise the door %s.",
                    key_d, does_what);
                I->p ("This keycard must be cracked.");
                key->setCrackedKnown ();
            }
        } else {
            I->p ("You swipe your %s but nothing happens.", key_d);
            I->p ("Obviously the keycard and lock colors do not match.");
            if (key->isBugginessKnown ()) key->setCrackedKnown ();
            return QUICKTURN;
        }
        elapsed = QUICKTURN;
    } else if (key->isA (kObjLockPick)) {
        int score = sportingD20 () + Hero.getSkillModifier (kOpenLock);
        if (door->isRetinaDoor ())
            score -= 10;
        if (!locked)
            score += 4;
        if (score >= 20) {
            if (locked) {
                I->p ("You run a bypass on the locking mechanism.");
                if (door->isAlarmedDoor () and mProfession == Ninja) {
                    I->p ("You also disable the door's alarm system.  Ninja'd!");
                    door->mDoor.mFlags &= ~shFeature::kAlarmed;
                    Hero.earnXP (Level->mDLevel);
                }
                if (mProfession == Ninja and !key->isBugginessKnown ()) {
                    key->setBugginessKnown ();
                    key->announce ();
                }
            } else {
                I->p ("You lock the door.");
            }
            elapsed = FULLTURN;
        } else {
            if (door->isRetinaDoor ())
                I->p ("It's very difficult to bypass "
                      "the retina scanner.");
            if (door->isAlarmedDoor () and score <= 10) {
                I->p ("You set off an alarm!");
                Level->doorAlarm (door);
            } else {
                I->p ("You fail to defeat the lock.");
            }
            return FULLTURN;
        }
    } else if (door->isRetinaDoor ()) {
        I->p ("You need the proper retina to unlock this door.");
    } else {
        if (isBlind ()) {
            I->p ("Nothing happens.");
        } else {
            char *buf = GetBuf ();
            strcpy (buf, door->getDoorLockName ());
            /* Only orange key needs to be preceded by 'an'. */
            const char *an = buf[0] == 'o' ? "an" : "a";
            I->p ("You need %s %s keycard to %s this door.", an, buf,
                  locked ? "unlock" : "lock");
        }
        return QUICKTURN;
    }
    if (locked) {
        door->unlockDoor ();
    } else {
        door->lockDoor ();
    }
    return elapsed;
}


static int
useKeyTool (shObject *key)
{
    shDirection dir;
    int x = Hero.mX;
    int y = Hero.mY;
    shFeature *f;

    dir = I->getDirection ();
    switch (dir) {
    case kNoDirection:
        return 0;
    case kUp:
        I->p ("There's no lock on the ceiling.");
        return 0;
    default:
        if (!Level->moveForward (dir, &x, &y)) {
            I->p ("There's no lock there.");
            return 0;
        }
        if ((f = Level->getFeature (x, y)) and f->isDoor ()) {
            return Hero.useKey (key, f);
        }
        I->p ("There's no lock there.");
        return 0;
    }
}


static int
useComputer (shObject *computer)
{
    if (Hero.is (kUnableCompute)) {
        I->p ("You dare to read the Nam-Shub?!  Defiler.");
        return 0;
    }

    shObjectVector v;
    selectObjects (&v, Hero.mInventory, kFloppyDisk);
    shObject *obj = Hero.quickPickItem (&v, "execute", 0);
    if (!obj)  return 0;

    return computer->executeProgram (obj);
}


static int
useMedicomp (shObject *medicomp)
{
    const int PLAGUE_COST = 10;
    const int RESTORE_COST = 4;
    if (!medicomp->mCharges) {
        I->p ("The medicomp is out of charges.");
        int known = medicomp->isChargeKnown ();
        medicomp->setChargeKnown ();
        return known ? 0 : HALFTURN;
    }
    if (Hero.mProfession == Yautja)  medicomp->setChargeKnown ();
    if (Hero.is (kPlagued)) {
        if (medicomp->mCharges >= PLAGUE_COST) {
            medicomp->mCharges -= PLAGUE_COST;
            Hero.restoration (1);
            return LONGTURN;
        } else {
            I->p ("Not enough charges to cure plague.");
            int known = medicomp->isChargeKnown ();
            medicomp->setChargeKnown ();
            return known ? 0 : HALFTURN;
        }
    }
    if (Hero.mHP < Hero.mMaxHP or Hero.is (kViolated) or Hero.is (kStunned) or
        Hero.is (kConfused) or Hero.getStoryFlag ("brain incision"))
    {
        --medicomp->mCharges;
        Hero.healing (NDX (5 + medicomp->mBugginess, 4), 0);
        return LONGTURN;
    }
    if (Hero.needsRestoration ()) {
        if (medicomp->mCharges >= RESTORE_COST) {
            medicomp->mCharges -= RESTORE_COST;
            Hero.restoration (2 + medicomp->mBugginess);
            return LONGTURN;
        } else {
            I->p ("Not enough charges to restore abilities.");
            int known = medicomp->isChargeKnown ();
            medicomp->setChargeKnown ();
            return known ? 0 : HALFTURN;
        }
    }
    I->p ("Nothing to cure.");
    return 0;
}

static int
scanVat (shFeature *f, shObject *tricorder)
{
    const int vatScanCost = 5;
    if (f->mVat.mAnalyzed) {
        I->p ("You already know this is %s.", f->getDescription ());
        return 0;
    }
    if (Hero.countEnergy () < vatScanCost) {
        I->p ("You have not enough energy to scan this vat.");
        return 0;
    }
    Hero.loseEnergy (vatScanCost);
    /* Intentionally says nothing about radioactiveness.
       Use Geiger counter for this purpose. */
    f->mVat.mAnalyzed = 1;
    I->p ("According to %s this is %s.",
        YOUR (tricorder), f->getDescription ());
    I->drawSideWin ();
    return LONGTURN;
}

static int
useTricorder (shObject *tricorder)
{   /* If you change values below remember to update Tricorder lore. */
    const int quickScanCost = 1;
    const int diagnosisCost = 9;
    static const char *creatureTypeName[kMaxCreatureType] =
    {
        "unliving",
        "unliving",
        "unliving",
        "unliving",
        "egg",
        "ooze",
        "cyborg",
        "aberration",
        "animal",
        "alien",
        "beast",
        "humanoid",
        "mutant humanoid",
        "insect",
        "unknown lifeform", /* outsider */
        "vermin",
        "zerg"
    };

    if (!tricorder->isKnown ()) {
        I->p ("It is a tricorder.");
        tricorder->setKnown ();
        I->pause ();
    }
    shDirection dir;
    int x = Hero.mX;
    int y = Hero.mY;
    //shMonster *c;
    shCreature *c;
    shFeature *f;

    if (tricorder->isBuggy ()) {
        I->p ("It appears to be broken.");
        tricorder->setBugginessKnown ();
        return 0;
    }
    dir = I->getDirection ();
    switch (dir) {
    case kNoDirection:
    case kUp:
        return 0;
    case kDown:
        f = Level->getFeature (x, y);
        if (f and f->mType == shFeature::kVat) {
            return scanVat (f, tricorder);
        }
        return 0;
    default:
        if (!Level->moveForward (dir, &x, &y)) {
            return 0;
        }
        if (Hero.countEnergy () < quickScanCost) {
            I->p ("You have no energy for %s.", YOUR (tricorder));
            return 0;
        }
        f = Level->getFeature (x, y);
        //c = (shMonster *) Level->getCreature (x, y);
        c = Level->getCreature (x, y);
        if (!c and Hero.isBlind ()) {
            Hero.loseEnergy (quickScanCost);
            I->p ("You detect no creature there.");
            I->drawSideWin ();
            return 0;
        } else if (c) {
            const char *desc;
            if (Hero.isBlind ()) {
                desc = creatureTypeName[c->mType];
            } else {
                desc = c->getDescription ();
            }
            c->mHidden = 0;

            Hero.loseEnergy (quickScanCost);
            I->p ("%s HP:%d/%d AC: %d", desc, c->mHP, c->mMaxHP, c->getAC ());
            I->drawSideWin ();
            /* Creature diagnosed must stay still. */
            if (c->isHero () or c->isPet () or c->isSessile () or
                c->is (kParalyzed) or c->is (kAsleep) or c->isTrapped ())
            {
                bool wasbugknown = tricorder->isBugginessKnown ();
                tricorder->setBugginessKnown ();
                /* Tell player if most other conditions are fulfilled. */
                if (tricorder->isDebugged ()) {
                    if (!wasbugknown) {
                        I->p ("Optimize your tricorder to perform full diangostic scans.");
                    }
                } else if (Hero.countEnergy () < diagnosisCost) {
                    I->p ("You have no energy to perform full scan.");
                } else if (I->yn ("Perform full diagnostic scan?")) {
                    Hero.loseEnergy (diagnosisCost);
                    /* Shopkeepers charge only for using diagnosis.  On the other
                       hand it is cheaper than docbot service so may be worth it. */
                    Hero.employedItem (tricorder, 80);
                    I->drawSideWin ();
                    /* Diagnosis screen would obscure this information. */
                    I->pause ();
                    c->doDiagnostics (tricorder->mBugginess);
                    return LONGTURN;
                }
            }
        } else if (f and f->mType == shFeature::kVat) {
            return scanVat (f, tricorder);
        } else {
            I->p ("There is nothing to analyze.");
        }
    } /* Takes no time to use to be useful in combat. */
    return 0;
}


static int
useDroidCaller (shObject *obj)
{
    shCreature *c;
    shVector <shCreature *> clist;

    if (Level->isInGarbageCompactor (Hero.mX, Hero.mY) and
        !obj->isBuggy () and Level->mCompactorState < 10 and
        Level->mCompactorState > 0)
    {
        for (int i = 0; i < Hero.mPets.count (); ++i) {
            shCreature *pet = Hero.mPets.get (i);
            /* R2D2! Turn off the garbage compactor at the sewer plant! */
            if (pet->isA (kMonAstromechDroid)) {
                I->p ("You call upon your astromech droid for help.");
                if (Hero.tryToTranslate (pet)) {
                    if (Level->mCompactorHacked == 0) {
                        I->p ("The R2 unit on %s %d acknowledges your request.",
                              pet->mLevel->mName, pet->mLevel->mDLevel);
                    } else {
                        I->p ("The R2 unit on %s %d is working on it.",
                              pet->mLevel->mName, pet->mLevel->mDLevel);
                    }
                } else {
                    I->p ("You receive a torrent of beeps and chirps"
                          " in response.");
                }
                Level->mCompactorHacked = 1; /* Hero is safe. */
                return HALFTURN;
            }
        }
    }
    I->p ("%s produces a strange whistling sound.", THE (obj));

    for (int i = 0; i < Level->mCrList.count (); ++i) {
        c = Level->mCrList.get (i);
        if (   (obj->isBuggy () and c->isRobot ())
            or (!obj->isBuggy () and c->isPet ()))
        {
            int x = Hero.mX;
            int y = Hero.mY;
            if (!obj->isOptimized ()) {
                x = x + RNG(7) - 3;
                y = y + RNG(5) - 2;
            }

            if (!Level->findNearbyUnoccupiedSquare (&x, &y)) {
                c->transport (x, y, 100, 1);
                if (Hero.canSee (c) and !c->isPet ()) {
                    obj->setBugginessKnown ();
                }
                if (c->isA (kMonClerkbot) or
                    c->isA (kMonGuardbot) or
                    c->isA (kMonSecuritron) or
                    c->isA (kMonDocbot) or
                    c->isA (kMonWarbot))
                {
                    clist.add (c);
                }
            }
        }
    }
    if (!obj->isKnown () or !obj->isAppearanceKnown ()) {
        obj->setKnown ();
        I->p ("%c - %s", obj->mLetter, obj->inv ());
    }
    I->drawScreen ();
    for (int i = 0; i < clist.count (); i++)
        clist.get (i) -> newEnemy (&Hero);

    return HALFTURN;
}


static int
useGrenade (shObject *grenade)   /* That means set it off. */
{
    I->p ("You push the button.");
    switch (grenade->mIlkId) {
    case kObjFlare: {
        int savecnt = grenade->mCount; /* Ugly hack. */
        grenade->mCount = 1;
        if (!Hero.isBlind ()) {
            I->p ("%s lights.", YOUR (grenade));
        } else {
            I->p ("%s becomes very warm.", YOUR (grenade));
        }
        grenade->mCount = savecnt;
        grenade->setKnown ();
        break;
    }
    case kObjFlashbang:
        I->p ("Poof!");
        grenade->setKnown ();
        break;
    default:
        I->p ("Boom!");
    }
    Level->areaEffect (grenade->myIlk ()->mMissileAttack, grenade,
                       Hero.mX, Hero.mY, kOrigin, &Hero, grenade->mEnhancement);
    Hero.useUpOneObjectFromInventory (grenade);
    return FULLTURN;
}


static int
useProximityMine (shObject *obj)
{
    shDirection dir;
    int x = Hero.mX;
    int y = Hero.mY;

    if (!obj->isKnown ()) {
        I->p ("You push the button.");
        obj->setKnown ();
        obj->announce ();
        I->pause ();
    }
    dir = I->getDirection ();
    switch (dir) {
    case kNoDirection:
        return 0;
    case kUp:
        I->p ("You can't reach the ceiling.");
        return 0;
    case kDown:
        I->p ("Move away from this spot to place it here.");
        return 0;
    default:
        if (!Level->moveForward (dir, &x, &y)) {
            return 0;
        }
        if (Level->getCreature (x, y)) {
            I->p ("There is something standing there already.");
            return 0;
        }
        if (Level->isObstacle (x, y)) {
            I->p ("There is not enough space to place it.");
            return 0;
        }
        shMonster *mine = new shMonster (kMonSmartBomb);
        mine->mConditions |= kNoExp;
        if (!obj->isBuggy ()) {
            mine->makePet ();
            obj->setBugginessKnown ();  /* Rest in stack known. */
        }
        Hero.useUpOneObjectFromInventory (obj);
        Level->putCreature (mine, x, y);
        Level->checkTraps (x, y, 100);
    }
    return LONGTURN;
}

/* returns ms elapsed */
static int
usePortableHole (shObject *obj)
{
    shDirection dir;
    int x = Hero.mX;
    int y = Hero.mY;
    shFeature *f;
    shCreature *c;

    dir = I->getDirection ();
    switch (dir) {
    case kNoDirection:
        return 0;
    case kUp:
        I->p ("You can't reach the ceiling.");
        return 0;
    case kDown:
    default:
        if (!Level->moveForward (dir, &x, &y)) {
            return 0;
        }
        f = (shFeature *) Level->getFeature (x, y);
        if (!Level->isFloor (x, y)) {
            if (f) {
                I->p ("But there is a portable hole already.");
                return 0;
            }
            int instant = 0, nx = x, ny = y;
            shTerrainType middle = kVoid;
            /* For low depth hole activates automatically. */
            /* Patterns .#. .##. .# #. only. Pattern ### is not permitted. */
            for (int i = 0; i < 3; ++i) {
                if (!Level->moveForward (dir, &nx, &ny))
                    break;
                if (i == 0)
                    middle = Level->getSquare (nx, ny)->mTerr;
                if (Level->isFloor (nx, ny)) {
                    instant = (i < 2) or (middle == kStone);
                    break;
                }
            }
            Hero.useUpOneObjectFromInventory (obj);
            if (instant) { /* Dig immediately. */
                I->p ("As you lay it on %s it immediately activates.",
                    THE (Level->getSquare (x, y)));
                while (!Level->isFloor (x, y)) {
                    Level->dig (x, y);
                    Level->moveForward (dir, &x, &y);
                } /* Check only for hero position. */
                if (Hero.isInShop ()) {
                    Hero.damagedShop (Hero.mX, Hero.mY);
                }
                break;
            }
            shFeature *f = new shFeature ();
            f->mType = shFeature::kPortableHole;
            f->mX = x;
            f->mY = y;
            Level->addFeature (f);
            I->p ("You lay it on %s.", THE (Level->getSquare (x, y)));
            /* Laying it from outside is okay. Doing it from inside is not. */
            if (Hero.isInShop ()) {
                Hero.damagedShop (Hero.mX, Hero.mY);
            }
            break;
        }
        if (f) {
            /* It is scummable to detect traps for free otherwise. */
            I->p ("It failed to activate properly %s. It is wasted.",
                  x == Hero.mX and y == Hero.mY ? "here" : "there");
            Hero.useUpOneObjectFromInventory (obj);
            return 0;
        }
        Hero.useUpOneObjectFromInventory (obj);
        Level->addTrap (x, y, shFeature::kHole);
        if (Level->isInShop (x, y)) {
            Hero.damagedShop (x, y);
        }
        c = Level->getCreature (x, y);
        if (c and !c->isHero ()) {
            c->newEnemy (&Hero);
        }
        Level->checkTraps (x, y, 100);
    }
    return FULLTURN;
}

/* returns ms elapsed */
int
useOnOffTool (shObject *tool)
{
    if (tool->isActive ()) {
        if (tool->isBuggy ()) {
            I->p ("You can't turn off %s.  It is jammed!", YOUR (tool));
            tool->setBugginessKnown ();
        } else {
            I->p ("You turn off %s.", YOUR (tool));
            tool->resetActive ();
            if (tool->isA (kObjGeigerCounter) and
                Hero.getStoryFlag ("geiger counter message"))
            {
                I->p ("It stops clicking.");
            }
        }
    } else if (Hero.countEnergy () <= 0 and tool->myIlk ()->mEnergyUse) {
        I->p ("You're out of juice!");
    } else {
        I->p ("You turn on %s.", YOUR (tool));
        tool->setActive ();
        if (tool->isA (kObjGeigerCounter) and
            Hero.getStoryFlag ("geiger counter message"))
        {
            if (!tool->isKnown ()) {
                I->p ("It starts clicking.  It must be a Geiger counter.");
                tool->setKnown ();
            } else {
                I->p ("It starts clicking.");
            }
            tool->setAppearanceKnown ();
        }
    }
    Hero.computeIntrinsics ();
    return HALFTURN;
}


static int
useEnergyTank (shObject *tank)
{
    int capacity = tank->myIlk ()->mMaxCharges;
    shObject *cells = NULL;
    if (tank->mCharges == capacity) {
        I->p ("%s is full already.", YOUR (tank));
        return 0;
    }
    for (int i = 0; i < Hero.mInventory->count (); ++i) {
        cells = Hero.mInventory->get (i);
        if (cells->mIlkId == kObjEnergyCell) break;
    }
    if (!cells or cells->mIlkId != kObjEnergyCell) {
        I->p ("You have no energy cells to discharge into %s.", tank->theQuick ());
        return 0;
    }
    int eat = mini (cells->mCount, capacity - tank->mCharges);
    Hero.useUpSomeObjectsFromInventory (cells, eat);
    tank->mCharges += eat;
    bool full = tank->mCharges == capacity;
    I->p ("%s absorbs energy of %d energy cell%s%s.", YOUR (tank),
          eat, eat > 1 ? "s" : "", full ? " and is now full" : "");
    return FULLTURN;
}


static int
useKhaydarinCrystal (shObject *crystal)
{
    int worked = 0;
    if (!crystal->mCharges) {
        I->p ("Nothing happens.");
        int elapsed = crystal->isChargeKnown () ? 0 : 200;
        crystal->setChargeKnown ();
        return elapsed;
    }
    if (Hero.mPsionicStormFatigue) {
        --Hero.mPsionicStormFatigue;
    }
    if (Hero.mAbil.mPsi < Hero.mMaxAbil.mPsi) {
        I->p ("You focus your mental powers.");
        Hero.mPsiDrain = 0;
        Hero.mAbil.mPsi = Hero.mMaxAbil.mPsi;
        ++worked;
    }
    if (Hero.is (kConfused) or Hero.is (kStunned)) {
        I->p ("Your thoughts become crystal clear.");
        Hero.cure (kConfused);
        Hero.cure (kStunned);
        ++worked;
    }
    if (!worked) {
        I->p ("%s glows faintly.", YOUR (crystal));
    }
    --crystal->mCharges;
    Hero.computeSkills (); /* Charges give bonus to concentration. */
    return HALFTURN;
}

#if 0
static int
useLicense (shObject *license)
{
    const char *interrogate[] =
    {
        "Orgasmatron?! What are you talking about?",
        "The Orgasmatron is rumored to be somewhere in the Mainframe.",
        "There is more than one Orgasmatron but only one true Orgasmatron.",
        "I heard someone left an Orgasmatron in a garbage compactor.",
        "Follow the white rabbit. It will lead you to the Orgasmatron.",
        "Entrance to the Mainframe is protected by a door with retina scanner.",
        "The key to retina scanner is held by the Bastard Operator From Hell.",
        "You can find the Operator at the bottom of Gamma Caves.",
        "You most probably will need to visit Gamma Caves.",
        "Some say that a skilled burglar can bypass retina scanner door.",
        "Need protection against radiation? Search the waste treatment plant.",
        "I am afraid you may need to confront Shodan about it."
    };
    const int numblurbs = sizeof (interrogate) / sizeof (char *);
    shDirection dir = I->getDirection ();
    switch (dir) {
        case kNoDirection: case kUp: case kDown:
            I->p ("No idea what to do with %s there.", YOUR (license));
            return 0;
        case kOrigin: {
            char *buf = GetBuf ();
            strcpy (buf, Hero.mName);
            buf[0] = toupper (buf[0]);
            if (!Hero.isBlind ()) {
                /* What if it is generated randomly? */
                I->p ("It reads: \"Detective %s\".", buf);
            } else {
                I->p ("You cannot read while blind.");
            }
            return 0;
        }
        default: {
            int x = Hero.mX, y = Hero.mY;
            if (!Level->moveForward (dir, &x, &y)) {
                return 0;
            }
            shCreature *c = Level->getCreature (x, y);
            shFeature *f = Level->getFeature (x, y);
            if (c) {
                if (c->isBlind ()) {
                    I->p ("%s seems to be blind.", THE (c));
                } else if (c->is (kAsleep)) {
                    I->p ("%s snores loudly.", THE (c));
                } else if (c->isA (kMonGuardbot) and !c->isHostile ()) {
                    I->p ("You present %s to %s.", YOUR (license), THE (c));
                    shMonster *guard = (shMonster *) c;
                    if (guard->mGuard.mToll == 0) {
                        I->p ("%s ignores you.", THE (guard));
                    } else {
                        if (license->isBuggy ()) {
                            if (Hero.tryToTranslate (guard)) {
                                I->p ("\"Your license has expired.  You must pay like everyone else.\"");
                            } else {
                                I->p ("%s whirs disappointingly.", THE (guard));
                            }
                        } else {
                            if (Hero.tryToTranslate (guard)) {
                                I->p ("\"You may pass.\"");
                            } else {
                                I->p ("%s beeps calmly.", THE (guard));
                            }
                            guard->mGuard.mToll = 0;
                        }
                    }
                } else if (c->isA (kMonLawyer) and c->isHostile ()) {
                    int bribe = 200 + RNG (101);
                    if (I->yn ("%s nods with understanding.  "
                               "Pay $%d to buy cooperation?",
                               THE (c), bribe))
                    {
                        if (Hero.countMoney () >= bribe) {
                            I->p ("%s takes your money.", THE (c));
                            Hero.loseMoney (bribe);
                            c->gainMoney (bribe);
                            ((shMonster *) c) ->mDisposition =
                                shMonster::kIndifferent;
                        } else {
                            I->p ("You don't have that much money.  %s disregards your license.", THE (c));
                        }
                    }
                } else if (c->mType != kHumanoid and c->mType != kMutant and
                           c->mType != kOutsider and c->mType != kCyborg)
                {
                    I->p ("%s seems strangely uncooperative.", THE (c));
                } else if (!c->isHostile () or c->is (kParalyzed)) {
                    I->pageLog ();
                    I->p ("You present %s to %s.", YOUR (license), THE (c));
                    I->p ("You interrogate about the Bizarro Orgasmatron.");
                    /* This could be replaced by get random fortune message. */
                    int reply = c->mMaxHP % numblurbs;
                    if (Hero.tryToTranslate (c)) {
                        I->p ("\"%s\"", interrogate[reply]);
                    } else {
                        I->p ("You fail to understand the reply.");
                    }
                } else {
                    I->p ("%s seems more interested in killing you.", THE (c));
                }
            } else if (f and f->isDoor ()) {
                return Hero.useKey (license, f);
            } else {
                I->p ("No idea what to do with %s there.", YOUR (license));
                return 0;
            }
        }
    }
    return FULLTURN;
}
#endif

static int
swallowBluePill (shObject *pill)
{
    I->p ("You swallow the pill.");
    if (Level->isMainframe ()) return 0;
    Hero.useUpOneObjectFromInventory (pill);
    for (int i = 1; i < Maze.count (); ++i) {
        shMapLevel *L = Maze.get (i);
        if (L->isMainframe ()) {
            Level->warpCreature (&Hero, L);
            break;
        }
    }
    return 0;
}

static int
swallowRedPill (shObject *pill)
{
    I->p ("You swallow the pill.");
    if (!Level->isMainframe ()) return 0;
    Hero.useUpOneObjectFromInventory (pill);
    for (int i = 1; i < Maze.count (); ++i) {
        shMapLevel *L = Maze.get (i);
        if (L->mType == shMapLevel::kRabbit) {
            Level->warpCreature (&Hero, L);
            break;
        }
    }
    return 0;
}

static int
useTransferCable (shObject *cable)
{
    shObjectVector v;
    selectObjects (&v, Hero.mInventory, kObjGenericEnergyTank);
    selectObjects (&v, Hero.mInventory, kObjEnergyBelt);
    if (v.count () < 2) {
        I->p ("You need at least two energy cell storage items to use this.");
        return 0;
    }
    shObject *src = Hero.quickPickItem (&v, "transfer from", 0);
    if (!src) return 0;
    v.remove (src);
    shObject *dest = Hero.quickPickItem (&v, "transfer to", 0);
    if (!dest) return 0;
    int space = dest->myIlk ()->mMaxCharges - dest->mCharges;
    int transfer = 0;
    if (space > src->mCharges) {
        transfer = src->mCharges;
        src->mCharges = 0;
    } else {
        transfer = space;
        src->mCharges -= transfer;
    }
    int loss = cable->isOptimized () ? transfer / 100 : /* 1% */
               cable->isDebugged () ? transfer * 3 / 100 : /* 3% */
               transfer / 10; /* 10% */
    if (loss > src->mCharges) {
        transfer -= (loss - src->mCharges);
        src->mCharges = 0;
    } else {
        src->mCharges -= loss;
    }
    dest->mCharges += transfer;
    int action = dest->isUnpaid () - src->isUnpaid ();
    const char *verb =
        action <= -1 ? "steal" : action >= +1 ? "donate" : "transfer";
    if (action <= -1 and Hero.mProfession == Ninja) verb = "ninja'd";
    I->p ("You %s energy of %d cell%s.", verb, transfer + loss,
        loss > 1 ? "s" : "");
    I->p ("Energy of %d cell%s was lost in the process.", loss,
        loss > 1 ? "s" : "");
    return FULLTURN;
}

static int
igniteLightsaber (shObject *saber)
{
    if (saber->isKnown ()) {
        I->p ("Wield this %s to fight with it.", saber->getShortDescription ());
        return 0;
    }  /* Applying unknown lightsaber activates it. */
    saber->setKnown ();
    Hero.reorganizeInventory ();
    const char *color = "";
    if (saber->isAppearanceKnown ()) { /* You now see blade color. */
        char *buf = GetBuf ();
        strcpy (buf, saber->getShortDescription ());
        strchr (buf, ' ')[1] = 0; /* Cut description after space. */
        color = buf;
    }
    I->p ("Kzzzcht!  The flashlight produces %senergy blade!", color);
    if (RNG (2)) { /* You held the wrong way. */
        I->p ("Ooowww!!  You happened to hold it the wrong way.");
        if (Hero.sufferDamage (saber->myIlk ()->mMeleeAttack)) {
            Hero.shCreature::die (kKilled, "igniting a lightsaber while holding it the wrong way");
        }
    } else { /* You held it outwards.  Lucky you. */
        I->p ("Luckily you held it outwards.");
    }
    return FULLTURN;
}

static int
switchMaskMode (shObject *mask)
{
    if (mask->isToggled ()) {
        I->p ("You switch %s to thermal vision mode.", YOUR (mask));
        mask->resetToggled ();
    } else {
        I->p ("You switch %s to EM field vision mode.", YOUR (mask));
        mask->setToggled ();
    }
    Hero.computeIntrinsics ();
    return HALFTURN;
}

static int
bribeSomeone (shObject *with_money)
{
    shDirection dir = I->getDirection ();
    switch (dir) {
        case kNoDirection: case kUp: case kDown:
            return 0;
        case kOrigin:
            I->p ("You count your money. You have $%d.", Hero.countMoney ());
            return 0;
        default: {
            int x = Hero.mX, y = Hero.mY;
            shCreature *c = NULL;
            if (!Level->moveForward (dir, &x, &y) or
                NULL == (c = Level->getCreature (x, y)))
            {
                I->p ("There is nobody to pay.");
                return 0;
            }
            Hero.bribe ((shMonster *) c);
            return FULLTURN;
        }
    }
}

static int
useBizarroOrgasmatron (shObject *obj)
{
    if (Level->isMainframe ()) {
        I->p ("Indeed, this is the Bizarro Orgasmatron!");
        I->p ("However, to make real use of its power you must escape the Mainframe.");
        obj->setKnown ();
        return FULLTURN;
    }
    I->p ("Hot dog!  You finally got your paws on the Bizarro Orgasmatron!");
    I->p ("You use its awesome power to beam yourself off this two-bit space hulk.");
    I->p ("");

    Hero.earnScore (10000);
    Hero.shCreature::die (kWonGame);
    return FULLTURN;
}


static int
useBizarreOrgasmatron (shObject *obj)
{
    I->p ("Hot dog!  You finally got your paws on the Bizarre Orgasmatron!");
    I->p ("You use its awesome power to beam yourself off this two-bit space hulk.");
    I->p ("");
    I->pause ();

    if (Hero.countEnergy () > 15) {
        Hero.loseEnergy (15);
        if (Hero.transport (-1, -1, 100, 1)) {
            Hero.shCreature::die (kKilled, "the power of the Bizarre Orgasmatron");
            return FULLTURN;
        }
    }
    I->p ("Hrmm... that's bizarre...");
    obj->setKnown ();
    return FULLTURN;
}


static int
useBizaaroOrgasmatron (shObject *obj)
{
    I->p ("You activate the Bizaaro Orgasmatron...");
    I->pause ();
    I->p ("An invisible choir sings, and you are bathed in radiance...");
    I->pause ();
    I->p ("The voice of eit_cyrus booms out:");
    I->setColor (kLime);
    I->p ("\"CoGNRATULATOINSS, n00B!!!\"");
    I->pause ();
    I->p ("\"In retrun  for thy sevrice, I grants thee teh gift of a SPELLCHEECKER!!!\"");
    I->p ("\"HAHAHAHAHOHOHOAHAHAHA!!! !!!!!1\"");
    I->setColor (kGray);

    obj->setKnown ();
    return FULLTURN;
}


static int
useBizzaroOrgasmatron (shObject *obj)
{
    I->p ("This makes you feel great!");
    obj->setKnown ();
    return FULLTURN;
}


static int
useBazaaroOrgasmatron (shObject *obj)
{
    I->p ("Nothing happens.");
    obj->setKnown ();
    return FULLTURN;
}


static int
useBazarroOrgasmatron (shObject *obj)
{
    char *capsname = strdup (Hero.mName);
    char *p;

    for (p = capsname; *p; p++)
        *p = toupper (*p);
    I->p ("THANK YOU %s!", capsname);
    I->p ("BUT OUR PRINCESS IS IN ANOTHER CASTLE!");
    free (capsname);
    obj->setKnown ();
    return FULLTURN;
}

void
initializeTools ()
{
    AllIlks[kObjMoney].mUseFunc = bribeSomeone;
    AllIlks[kObjBioMask].mUseFunc = switchMaskMode;
    AllIlks[kObjDroidCaller].mUseFunc = useDroidCaller;
    AllIlks[kObjTricorder].mUseFunc = useTricorder;
    for (int i = kObjKeycard1; i <= kObjMasterKeycard; ++i)
        AllIlks[i].mUseFunc = useKeyTool;
    AllIlks[kObjLockPick].mUseFunc = useKeyTool;
    /* AllIlks[kObjLicense].mUseFunc = useLicense; */
    AllIlks[kObjRestrainingBolt].mUseFunc = useRestrainingBolt;
    AllIlks[kObjMedicomp].mUseFunc = useMedicomp;
    for (int i = kObjMiniComputer; i <= kObjSatCom; ++i)
        AllIlks[i].mUseFunc = useComputer;
    AllIlks[kObjPortableHole].mUseFunc = usePortableHole;
    AllIlks[kObjKhaydarin].mUseFunc = useKhaydarinCrystal;
    AllIlks[kObjBluePill].mUseFunc = swallowBluePill;
    AllIlks[kObjRedPill].mUseFunc = swallowRedPill;
    AllIlks[kObjTransferCable].mUseFunc = useTransferCable;
    AllIlks[kObjLightSaber].mUseFunc = igniteLightsaber;

    for (int i = kObjConcussionGrenade; i <= kObjFlare; ++i)
        AllIlks[i].mUseFunc = useGrenade;
    AllIlks[kObjProximityMine].mUseFunc = useProximityMine;

    AllIlks[kObjSmallEnergyTank].mUseFunc = useEnergyTank;
    AllIlks[kObjLargeEnergyTank].mUseFunc = useEnergyTank;
    AllIlks[kObjEnergyBelt].mUseFunc = useEnergyTank;

    AllIlks[kObjDuctTape].mUseFunc = useDuctTape;
    AllIlks[kObjMonkeyWrench].mUseFunc = useMonkeyWrench;
    AllIlks[kObjMechaDendrites].mUseFunc = useMechaDendrites;

    AllIlks[kObjFlashlight].mUseFunc = useOnOffTool;
    AllIlks[kObjDeepBluePA].mUseFunc = useOnOffTool;
    AllIlks[kObjGeigerCounter].mUseFunc = useOnOffTool;
    AllIlks[kObjMotionTracker].mUseFunc = useOnOffTool;
    AllIlks[kObjShieldBelt].mUseFunc = useOnOffTool;
    AllIlks[kObjCloakingBelt].mUseFunc = useOnOffTool;

    AllIlks[kObjTheOrgasmatron].mUseFunc = useBizarroOrgasmatron;
    AllIlks[kObjFakeOrgasmatron1].mUseFunc = useBizarreOrgasmatron;
    AllIlks[kObjFakeOrgasmatron2].mUseFunc = useBizaaroOrgasmatron;
    AllIlks[kObjFakeOrgasmatron3].mUseFunc = useBizzaroOrgasmatron;
    AllIlks[kObjFakeOrgasmatron4].mUseFunc = useBazaaroOrgasmatron;
    AllIlks[kObjFakeOrgasmatron5].mUseFunc = useBazarroOrgasmatron;
}
