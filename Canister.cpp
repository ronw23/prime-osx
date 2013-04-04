#include "Global.h"
#include "Util.h"
#include "Object.h"
#include "Hero.h"
#include <ctype.h>

static void
teleportVat (shFeature *vat)
{
    int newX; int newY;
    int prevroom = Level->getRoomID (Hero.mX, Hero.mY);
    Level->findSuitableStairSquare (&newX, &newY);
    if (!Level->getFeature (newX, newY)) {
        if (!Hero.isBlind ()) {
            I->p ("The vat flickers and disappears!");
        } else {
            I->p ("The vat seems to be gone.");
        }
        Level->addVat (newX, newY);
        shFeature *newvat = Level->getFeature (newX, newY);
        newvat->mVat = vat->mVat; /* Transfer all vat properties. */
        Level->removeFeature (vat);
        if (Hero.canSee (newX, newY)) {
            I->p ("Suddenly a vat materializes out of nowhere and falls down!");
        } /* See if the vat falls on someone. */
        shCreature *c = Level->getCreature (newX, newY);
        if (c) {
            if (c->sufferDamage (kAttFallingVat)) {
                if (Hero.canSee (c)) {
                    I->p ("It crushes %s into a pulp!", AN (c));
                    I->pauseXY (c->mX, c->mY);
                } else if (Hero.canHearThoughts (c)) {
                    I->p ("The mind of %s emanates pure terror for an instant.", AN (c));
                    Hero.checkConcentration (); /* Contact with terrified mind. */
                    I->p ("Then thoughts of %s abruptly vanish.", THE (c));
                    I->pauseXY (c->mX, c->mY);
                }
                c->die (kSlain);
            } else if (Hero.canSee (c)) {
                I->p ("It lands right on %s!", AN (c));
                I->pauseXY (c->mX, c->mY);
                if (c->canSee (&Hero)) {
                    c->newEnemy (&Hero);
                }
            } else if (Hero.canHearThoughts (c)) {
                I->p ("The mind of %s emanates fear for a moment.", AN (c));
                I->pauseXY (c->mX, c->mY);
                if (c->hasTelepathy ()) {
                    I->p("Somehow %s blames you for this.", THE (c));
                    c->newEnemy (&Hero);
                }
            }
        } /* Did the vat belong to a shopkeeper? */
        if (Hero.isInShop ()) {
            int newroom = Level->getRoomID (newX, newY);
            if (prevroom == newroom) {
                Hero.movedVat (); /* Harmless furniture rearrange. */
            } else {
                Hero.stolenVat (); /* Vat landed outside shop. */
            }
        }
    } else {
        I->p ("Nothing seems to happen.");
    }
}

/* Pouring a canister from a stack into a vat should always */
static void   /* ask the hero to name only a singular item. */
pourMaybeName (shObject *cans)
{
    int cnt = cans->mCount;
    cans->mCount = 1;
    cans->maybeName ();
    cans->mCount = cnt;
}

/* Splashes objects on floor with canister contents. */
static void
pourCanisterOnObjects (shObject *can, int x, int y)
{
    int seen = Hero.canSee (x, y);
    shObjectVector *v = Level->getObjects (x, y);
    if (v and can->myIlk ()->mMissileAttack) { /* Is explosive. */
        for (int i = v->count () - 1; i >= 0; --i) {
            shObject *obj = v->get (i);
            if (obj->sufferDamage (can->myIlk ()->mMissileAttack, x, y, 0, 0)) {
                if (seen) {
                    can->setKnown ();
                } else {
                    pourMaybeName (can);
                }
                v->remove (obj);
                delete obj;
            }
        }
        if (!v->count ()) {
            delete v;
            v = NULL;
            Level->setObjects (x, y, NULL);
        }
        return;
    } /* Try for special interaction. */
    if (v) for (int i = v->count () - 1; i >= 0; --i) {
        shObject *obj = v->get (i);
        switch (can->mIlkId) {
        case kObjBrain:
            if (obj->isA (kObjBioComputer)) {
                if (seen) {
                    I->p ("%s devours the brain!", THE (obj));
                    can->setKnown ();
                } else {
                    I->p ("You hear content munching sounds.");
                    pourMaybeName (can);
                }
                obj->mEnhancement = mini(+5, obj->mEnhancement + can->mBugginess + 1);
                return;
            } else if (obj->isA (kObjBioArmor)) {
                if (Hero.canSee (x, y)) {
                    I->p ("%s envelops the brain!", THE (obj));
                    can->setKnown ();
                } else {
                    I->p ("You hear a slurping sound.");
                    pourMaybeName (can);
                }
                obj->mIlkId = kObjBioComputer;
                obj->mEnhancement = RNG (-3, +3) + can->mBugginess;
                return;
            }
            break;
        case kObjMutagenCanister:
            if (obj->isA (kObjBioArmor)) {
                int die = RNG (2);
                if (seen) {
                    I->p ("%s soaks up the liquid!", THE (obj));
                    if (die) {
                        I->p ("It develops a fatal mutation and dies.");
                    } else {
                        I->p ("It develops a harmful mutation.");
                    }
                    can->setKnown ();
                }
                if (die) {
                    v->remove (obj);
                    delete obj;
                    if (!v->count ()) {
                        delete v;
                        v = NULL;
                        Level->setObjects (x, y, NULL);
                    }
                } else {
                    obj->mIlkId = kObjMBioArmor0;
                    obj->mEnhancement = maxi (-(obj->myIlk ()->mMaxEnhancement),
                        obj->mEnhancement - 1);
                }
                return;
            }
            break;
        case kObjGainAbilityCanister:
            if (!obj->isA (kObjBioComputer) and !obj->isA (kObjBioArmor)) {
                break;
            }
            if (seen) {
                I->p ("%s soaks up the liquid!", THE (obj));
            }
            if (obj->isA (kObjBioComputer)) {
                if (can->isBuggy ()) {
                    obj->mEnhancement = -5;
                    if (seen) {
                        I->p ("It loses its ability to process input.");
                        can->setKnown ();
                    }
                } else {
                    if (seen and (obj->mEnhancement == 5 or
                        (obj->mEnhancement == 4 and !can->isOptimized ())))
                    {
                        I->p("Nothing else seems to happen.");
                        pourMaybeName (can);
                    } else {
                        if (seen) {
                            I->p ("Its abilities improve!");
                            can->setKnown ();
                        }
                        ++obj->mEnhancement;
                    }
                }
            } else if (can->isBuggy ()) {
                if (obj->mIlkId == kObjBioArmor or
                    obj->isA (kObjMBioArmor0))
                {
                    if (obj->isEnhancementKnown () and seen) {
                        I->p ("It suffers.");
                        can->setKnown ();
                    }
                    obj->mEnhancement = maxi (-(obj->myIlk ()->mMaxEnhancement),
                        obj->mEnhancement - 1);
                } else {
                    if (seen) {
                        I->p ("It regresses in development!");
                        can->setKnown ();
                    }
                    obj->mIlkId = obj->myIlk ()->mParent;
                }
            } else {
                if (obj->mIlkId == kObjBioArmor) {
                    if (seen) {
                        I->p ("It gains a new ability!");
                        can->setKnown ();
                    }
                    switch (RNG (3)) {
                    case 0: obj->mIlkId = kObjEBioArmor1; break;
                    case 1: obj->mIlkId = kObjEBioArmor2; break;
                    case 2: obj->mIlkId = kObjEBioArmor3; break;
                    }
                } else if (obj->mIlkId == kObjMBioArmor0) {
                    if (seen) {
                        I->p ("Its mutation develops into somewhat beneficial one.");
                        can->setKnown ();
                    }
                    if (RNG (2)) {
                        obj->mIlkId = kObjMBioArmor1;
                    } else {
                        obj->mIlkId = kObjMBioArmor2;
                    }
                } else if (can->isOptimized () and
                       (obj->mIlkId == kObjEBioArmor1 or
                        obj->mIlkId == kObjEBioArmor2 or
                        obj->mIlkId == kObjEBioArmor3))
                {
                    if (seen) {
                        I->p ("It is further enhanced!");
                        can->setKnown ();
                    }
                    switch (obj->mIlkId) {
                    case kObjEBioArmor1:
                        if (RNG (2)) {
                            obj->mIlkId = kObjSBioArmor1;
                        } else {
                            obj->mIlkId = kObjSBioArmor2;
                        }
                        break;
                    case kObjEBioArmor2:
                        obj->mIlkId = kObjSBioArmor3; break;
                    case kObjEBioArmor3:
                        obj->mIlkId = kObjSBioArmor4; break;
                    default: break;
                    }
                } else {
                    if (obj->isEnhancementKnown () and seen) {
                        I->p ("Looks like %s enjoyed it.", THE (obj));
                        can->setKnown ();
                    }
                    obj->mEnhancement = mini (obj->myIlk ()->mMaxEnhancement,
                        obj->mEnhancement + 1);
                }
            }
            return;
        default: /* Has no special interaction. No point in iterating. */
            return;
        }
    } /* If it survived until this point it means the floor is contaminated. */
    if (can->isA (kObjMutagenCanister)) {
        Level->getSquare(x, y)->mFlags |= shSquare::kRadioactive;
    }
}

static int
pourCanisterAtFeature (shObject *can, shFeature *f, int x, int y)
{
    f->mTrapUnknown = 0; /* Better reveal it. */
    switch (f->mType) {
    case shFeature::kAcidPit:
        switch (can->mIlkId) {
        case kObjAntimatter:
            if (Level->getLevelBelow ()) {
                if (!Hero.isBlind ()) {
                    I->p ("The acid is annihilated.");
                }
                f->mType = shFeature::kHole;
                Level->checkTraps (x, y);
            } else {
                f->mType = shFeature::kPit;
            }
            return FULLTURN;
        case kObjWater:
            f->mType = shFeature::kPit;
            if (!Hero.isBlind ()) {
                I->p ("The acid boils violently as it is neutralized.");
                can->setKnown ();
            } else {
                I->p ("You hear boiling.");
                pourMaybeName (can);
            }
            return FULLTURN;
        default:
            if (!Hero.isBlind ()) {
                I->p ("The acid boils.");
            } else {
                I->p ("You hear boiling.");
            }
            pourMaybeName (can);
            return FULLTURN;
        }
    case shFeature::kPit:
        switch (can->mIlkId) {
        case kObjAntimatter:
            if (Level->getLevelBelow ()) {
                f->mType = shFeature::kHole;
                Level->checkTraps (x, y);
            }
            return FULLTURN;
        case kObjUniversalSolvent:
            f->mType = shFeature::kAcidPit;
            if (!Hero.isBlind ()) {
                I->p ("The pit is filled with acid!");
            } else {
                I->p ("You smell acidic odor.");
            }
            can->setKnown ();
            return FULLTURN;
        default:
            I->p ("You pour out the contents into %s.", THE (f));
            pourCanisterOnObjects (can, x, y);
            pourMaybeName (can);
            return FULLTURN;
        }
    case shFeature::kHole:
        I->p ("You pour out the contents into %s.", THE (f));
        return FULLTURN;
    case shFeature::kRadTrap:
        switch (can->mIlkId) {
        case kObjAntimatter:
            if (!Hero.isBlind ()) {
                I->p ("The rad trap is annihilated!");
            }
            if (Level->getLevelBelow ()) {
                f->mType = shFeature::kHole;
            } else {
                f->mType = shFeature::kPit;
            }
            Level->checkTraps (x, y, 100);
            break;
        case kObjUniversalSolvent:
            if (!Hero.isBlind ()) {
                I->p ("The rad trap dissolves!");
            } else {
                I->p ("You smell acidic odor.");
            }
            Level->removeFeature (f);
            can->setKnown ();
            break;
        default:
            I->p ("You pour out the contents into %s.", THE (f));
            pourMaybeName (can);
            break;
        }
        return FULLTURN;
    case shFeature::kVat:
        switch (can->mIlkId) {
        case kObjAntimatter:
            if (Hero.canSee (x, y)) {
                I->p ("The vat is annihilated!");
            }
            Level->removeFeature (f);
            can->setKnown ();
            Level->addTrap (x, y, shFeature::kHole);
            if (Hero.isInShop ()) {
                Hero.damagedShop (x, y);
            }
            Level->checkTraps (x, y, 100);
            return FULLTURN;
        case kObjLNO:
            I->p ("The sludge freezes!");
            can->setKnown ();
            return FULLTURN;
        case kObjNapalm: case kObjPlasmaCanister:
            if (!Hero.isBlind ()) {
                I->p ("The sludge boils!");
            } else {
                I->p ("You hear boiling.");
            }
            pourMaybeName (can);
            return FULLTURN;
        case kObjBeer: case kObjNanoCola: case kObjNukaCola: case kObjB3:
        case kObjCoffee:
            I->p ("The sludge fizzes.");
            pourMaybeName (can);
            return FULLTURN;
        case kObjSuperGlue:
            I->p ("The sludge thickens.");
            can->setKnown ();
            return FULLTURN;
        case kObjUniversalSolvent: case kObjWater:
            I->p ("The sludge thins.");
            pourMaybeName (can);
            return FULLTURN;
        case kObjMutagenCanister:
            I->p ("The sludge changes color.");
            f->mVat.mHealthy -= 2;
            f->mVat.mRadioactive++;
            can->setKnown ();
            return FULLTURN;
        case kObjPoisonCanister:
            I->p ("The sludge seems less nutritous now.");
            f->mVat.mHealthy -= 2;
            can->setKnown ();
            return FULLTURN;
        case kObjFullHealingCanister:
            f->mVat.mHealthy += 2; /* Fall through. */
        case kObjHealingCanister:
            I->p ("The sludge seems more nutritious now.");
            f->mVat.mHealthy += 1;
            pourMaybeName (can);
            return FULLTURN;
        case kObjGainAbilityCanister:
            f->mVat.mHealthy += 2; /* Fall through. */
        case kObjRestorationCanister:
            I->p ("You've improved the sludge recipe.");
            f->mVat.mHealthy += 2;
            pourMaybeName (can);
            return FULLTURN;
        case kObjBrain:
            I->p ("You drain a brain.");
            f->mVat.mHealthy += 2;
            can->setKnown ();
            return FULLTURN;
        case kObjRadAway:
            I->p ("The sludge seems purified.");
            f->mVat.mHealthy += 1;
            f->mVat.mRadioactive = 0;
            can->setKnown ();
            return FULLTURN;
        case kObjSpeedCanister:
            I->p ("The sludge is churning more rapidly now.");
            can->setKnown ();
            return FULLTURN;
        case kObjSpiceMelange:
            teleportVat (f);
            can->setKnown ();
            return FULLTURN;
        case kObjCannedEmbryo: {
            if (!Hero.isBlind ()) {
                I->p ("A larva falls out and drowns in the sludge!");
                can->setKnown ();
            }
            f->mVat.mHealthy -= 3;
            /* Give exp for killing alien. */
            Hero.earnXP (MonIlks[kMonFacehugger].mBaseLevel);
            /* Count wasted embryos too. */
            MonIlks[kMonAlienEmbryo].mKills += 1;
            if (Hero.isBlind ()) {
                I->p ("You feel more experienced.");
                pourMaybeName (can);
            }
            return FULLTURN;
        }
        default:
            can->setKnown (); /* This might help nail down any bugs. */
            I->p ("You almost breach space-time continuity by triggering unimplemented effect.");
            I->p ("You'd better report this bug and how you caused it.");
            return FULLTURN;
        }
    case shFeature::kPortableHole:
        switch (can->mIlkId) {
        case kObjAntimatter:
            if (!Hero.isBlind ()) {
                I->p ("The portable hole is annihilated along with %s!",
                    THE (Level->getSquare (x, y)));
            }
            Level->removeFeature (f);
            Level->dig (x, y);
            break;
        case kObjUniversalSolvent:
            if (!Hero.isBlind ()) {
                I->p ("The portable hole dissolves!");
            } else {
                I->p ("You smell acidic odor.");
            }
            Level->removeFeature (f);
            can->setKnown ();
            break;
        default:
            I->p ("You pour out the contents onto %s.", THE (Level->getSquare (x, y)));
        }
        return FULLTURN;
    case shFeature::kDoorOpen:
    default:
        I->p ("You pour out the contents onto %s.", THE (f));
        if (can->myIlk ()->mMissileAttack) {
            Level->areaEffectFeature (&Attacks[can->myIlk ()->mMissileAttack],
                NULL, x, y, &Hero);
        }
        pourCanisterOnObjects (can, x, y);
        pourMaybeName (can);
        return FULLTURN;
    }

}

static int useUniversalSolvent (shObject *can);

static int
useCommonCanister (shObject *can)
{
    shDirection where = I->getDirection ();
    if (kNoDirection == where) {
        return 0;
    }
    switch (where) {
    case kDown: {
        shFeature *f = Level->getFeature (Hero.mX, Hero.mY);
        if (f) return pourCanisterAtFeature (can, f, Hero.mX, Hero.mY);
        I->p ("You pour out the contents onto %s.",
            THE (Level->getSquare (Hero.mX, Hero.mY)));
        pourCanisterOnObjects (can, Hero.mX, Hero.mY);
        pourMaybeName (can);
        return FULLTURN;
    }
    case kOrigin:
        if (can->isA (kObjUniversalSolvent)) return useUniversalSolvent (can);
        /* Otherwise fall through. */
    case kUp:
        if (can->myIlk ()->mMissileAttack) {
            can->setKnown ();
            switch (can->mIlkId) {
            case kObjAntimatter: I->p ("You spill antimatter."); break;
            case kObjNapalm: I->p ("It ignites!"); break;
            case kObjLNO: I->p ("It freezes!"); break;
            case kObjPlasmaCanister: I->p ("It shocks you!"); break;
            case kObjUniversalSolvent: I->p ("It is acidic!"); break;
            case kObjSuperGlue: I->p ("This is sticky."); break;
            default: I->p ("You soak yourself."); break;
            }
            if (Hero.sufferDamage (can->myIlk ()->mMissileAttack)) {
                char *buf = GetBuf ();
                sprintf (buf, "spilling %s %s", can->an (1),
                    where == kUp ? "upwards" : "at self");
                Hero.shCreature::die (kKilled, buf);
            }
        }
        return FULLTURN;
    default: {
        int x = Hero.mX, y = Hero.mY;
        if (!Level->moveForward (where, &x, &y)) {
            I->p ("You stick the canister outside the map and pour it out.");
            I->p ("Its contents are harmlessly deallocated.");
            return FULLTURN;
        }
        shCreature *c = Level->getCreature (x, y);
        if (NULL == c) {
            shFeature *f = Level->getFeature (x, y);
            if (f) {
                return pourCanisterAtFeature (can, f, x, y);
            } else {
                I->p ("You pour out the contents onto %s.",
                    THE (Level->getSquare (x, y)));
            }
            if (can->isA (kObjAntimatter) and !Level->isFloor (x, y)) {
                I->p ("%s is annihilated!", THE (Level->getSquare (x, y)));
                Level->dig (x, y);
                return FULLTURN;
            }
            if (can->myIlk ()->mMissileAttack) {
                Level->areaEffectFeature (&Attacks[can->myIlk ()->mMissileAttack],
                    NULL, x, y, &Hero);
            }
            pourCanisterOnObjects (can, x, y);
            return FULLTURN;
        }
        I->p ("You splash the contents of canister at %s.", THE (c));
        /* Instakill because player expends whole canister. Yes, even Shodan. */
        if (can->isA (kObjAntimatter)) {
            I->p ("%s is annihilated!", THE (c));
            c->die (kAnnihilated);
            return FULLTURN;
        }
        if (can->myIlk ()->mMissileAttack) {
            switch (can->mIlkId) { /* TODO: Change to 3rd person. */
            case kObjNapalm: I->p ("You burn %s!", THE (c)); break;
            case kObjLNO: I->p ("You freeze %s!", THE (c)); break;
            case kObjPlasmaCanister: I->p ("You shock %s!", THE (c)); break;
            case kObjUniversalSolvent: I->p ("You dissolve %s!", THE (c)); break;
            case kObjSuperGlue: I->p ("You coat %s with glue.", THE (c)); break;
            default:
                I->p ("You do something evil and buggy to %s.", THE (c)); break;
            }
            if (c->sufferDamage (can->myIlk ()->mMissileAttack)) {
                c->die (kSlain);
            }
            can->setKnown ();
        } else {
            I->p ("You soak %s.", THE (c));
            pourMaybeName (can);
        }
        return FULLTURN;
    }
    }
    return FULLTURN; /* Not reached. */
}

static int
quaffAntimatter (shObject *can)
{
    I->p ("You drink antimatter.");
    can->setKnown ();
    Hero.shCreature::die (kAnnihilated, "drinking a canister of antimatter");
    return FULLTURN;
}

static int
quaffBeer (shObject *can)
{
    can->setKnown ();
    if (Hero.is (kFrightened)) {
        Hero.cure (kFrightened);
        I->p ("Liquid courage!");
    } else {
        I->p ("Mmmmm... beer!");
    }
    Hero.inflict (kConfused, NDX (2, 50) * FULLTURN);
    return FULLTURN;
}

static int
quaffNanoCola (shObject *can)
{
    int amt = RNG (2+can->mBugginess, 6);
    can->setKnown ();

    Hero.mPsiDrain -= amt;
    Hero.mAbil.mPsi += amt;
    I->p ("You feel invigorated!");
    return FULLTURN;
}

static int
quaffNukaCola (shObject *can)
{
    int amt = RNG (2+can->mBugginess, 6);
    can->setKnown ();

    Hero.mRad += RNG (1, 10);
    Hero.mPsiDrain -= amt;
    Hero.mAbil.mPsi += amt;
    I->p ("You feel refreshed!");
    return FULLTURN;
}

static int
quaffB3 (shObject *can)
{
    can->setKnown ();
    I->p ("You feel happier!");
    Hero.mRad += RNG (1, 10);
    if (RNG (2) and Hero.mHP >= 2) {
        --Hero.mHP;
        I->drawSideWin ();
    }
    if (!RNG (3)) {
        Hero.abortion ();
    }
    if (!RNG (40)) {
        I->p ("Your health suffers.");
        if (Hero.sufferAbilityDamage (kCon, 1)) {
            Hero.shCreature::die (kKilled, "drinking especially foul BBB canister");
        }
        I->drawSideWin ();
    }
    if (!RNG (100)) {
        Hero.getMutantPower ();
    }
    int amt = RNG (1, 5);
    Hero.mPsiDrain -= amt;
    Hero.mAbil.mPsi += amt;
    return FULLTURN;
}

static int
quaffCoffee (shObject *can)
{
    if (!can->isKnown ()) {
        I->p ("As you open the canister it heats itself up and you smell ... coffee?");
    } else {
        I->p ("You drink %s coffee.", can->isBuggy () ? "lukewarm" : "hot");
    }
    if (!Hero.getTimeOut (CAFFEINE)) {
        Hero.mInnateResistances[kMesmerizing] += 25;
    } /* Values for timeouts are those used by disk of hypnosis in reverse. */
    if (can->isBuggy ()) {
        Hero.setTimeOut (CAFFEINE, RNG (5, 25) * FULLTURN, 0);
    } else if (can->isDebugged ()) {
        Hero.setTimeOut (CAFFEINE, RNG (6, 40) * FULLTURN, 0);
    } else {
        Hero.setTimeOut (CAFFEINE, RNG (12, 80) * FULLTURN, 0);
    }
    can->setKnown ();
    return FULLTURN;
}

static int
quaffSuperGlue (shObject *can)
{
    can->setKnown ();
    I->p ("The canister sticks to your tongue!");
    I->p ("You look ridiculous!");
    Hero.setStoryFlag ("superglued tongue", 1);
    return FULLTURN;
}

static int
quaffUniversalSolvent (shObject *can)
{
    can->setKnown ();
    I->p ("This burns!");
    if (Hero.sufferDamage (kAttUniversalSolvent)) {
        Hero.shCreature::die (kKilled, "drinking a canister of universal solvent");
    }
    Hero.abortion ();
    return FULLTURN;
}

static int
useUniversalSolvent (shObject *can)
{
    int solved = 0;
    can->setKnown ();
    if (Hero.getStoryFlag ("superglued tongue")) {
        I->p ("You dissolve the super glue.");
        Hero.resetStoryFlag ("superglued tongue");
        Hero.mAbil.setByIndex (kPsi, 4 + Hero.mAbil.getByIndex (kPsi));
        ++solved;
    }
    if (Hero.mWeapon and Hero.mWeapon->isBuggy () and
        Hero.mWeapon->isBugginessKnown ())
    {
        I->p ("You unweld %s.", YOUR (Hero.mWeapon));
        Hero.mWeapon->setDebugged ();
        if (Hero.mWeapon->sufferDamage (kAttUniversalSolvent,
                                        Hero.mX, Hero.mY, 1, 1))
        {
            Hero.usedUpItem (Hero.mWeapon, 1, "dissolve");
            delete Hero.removeOneObjectFromInventory (Hero.mWeapon);
        }
        ++solved;
    }
    if (!solved) {
        I->p ("You dissolve some of your flesh.  Ouch!");
        if (Hero.sufferDamage (kAttUniversalSolvent)) {
            Hero.shCreature::die (kKilled, "bathing in universal solvent");
        }
    }
    return FULLTURN;
}

static int
quaffMutagen (shObject *can)
{
    can->setKnown ();
    I->p ("Ick!  That must have been toxic!");
    Hero.getMutantPower ();
    Hero.mRad += 125 + RNG (101);
    return FULLTURN;
}

static int
quaffWater (shObject *can)
{
    can->setKnown ();
    I->p ("This tastes like water.");
    return FULLTURN;
}

/* This routine and fullHealing return number of problems helped with. */
int
shCreature::healing (int hp, int hpmaxdice)
{
    if (hp == 0)  return 0;
    int id = 0;
    if (isHero () and mHP < mMaxHP) {
        int heal = mini (hp, mMaxHP - mHP);
        I->p ("Your wounds are rapidly healing!  (%+d HP)", heal);
        id++;
    }
    if (isHero () and Hero.getStoryFlag ("brain incision")) {
        Hero.resetStoryFlag ("brain incision");
        I->p ("Your head wound is closed.");
        id++;
    }
    mHP += hp;
    if (mHP > mMaxHP and hpmaxdice) {
        int bonus = NDX (hpmaxdice, 3);
        mMaxHP += bonus;
        mHP = mMaxHP;
        if (isHero ()) {
            I->p ("You feel much healthier.  (%+d max HP)", bonus);
            id++;
        }
    }
    mHP = mMaxHP;
    if (isHero () and is (kPlagued)) {
        I->p ("You have a feeling this will delay the plague somewhat.");
        mConditions &= ~kPlagued; /* Seemingly toggle it off. */
        setTimeOut (PLAGUED, 40 * FULLTURN, 0); /* Push next attack into future. */
        ++id;
    }
    if (is (kSickened) and !sewerSmells ()) {
        cure (kSickened);
        ++id;
    }
    static const shCondition remedy[] = {kViolated, kConfused, kStunned};
    const int size = sizeof (remedy) / sizeof (shCondition);
    for (int i = 0; i < size; ++i) {
        if (is (remedy[i])) {
            cure (remedy[i]);
            ++id;
        }
    }
    checkTimeOuts ();
    return id;
}

int
shCreature::fullHealing (int hp, int hpmaxdice)
{
    if (hp == 0)  return 0;
    int id = 0;
    if (isHero () and mHP < mMaxHP) {
        int heal = mini (hp, mMaxHP - mHP);
        I->p ("Your wounds are fully healed!  (%+d HP)", heal);
        id++;
    }
    if (isHero () and Hero.getStoryFlag ("brain incision")) {
        Hero.resetStoryFlag ("brain incision");
        I->p ("Your head wound is closed.");
        id++;
    }
    mHP += hp;
    if (mHP > mMaxHP and hpmaxdice) {
        int bonus = NDX (hpmaxdice, 6);
        mMaxHP += bonus;
        mHP = mMaxHP;
        if (isHero ()) {
            I->p ("You feel much healthier.  (%+d max HP)", bonus);
            id++;
        }
    }
    mHP = mMaxHP;
    static const shCondition remedy[] = {kPlagued, kViolated, kConfused, kStunned, kSickened};
    const int size = sizeof (remedy) / sizeof (shCondition);
    for (int i = 0; i < size; ++i) {
        if (is (remedy[i])) {
            cure (remedy[i]);
            ++id;
        }
    }
    if (is (kPlagued))  ++id;
    cure (kPlagued); /* Plague might be stalled by weak healing. */
    checkTimeOuts ();
    return id;
}

static int
quaffHealing (shObject *can)
{
    if (Hero.healing (NDX (3 + can->mBugginess, 6), 1))
        can->setKnown ();
    return FULLTURN;
}

static int
quaffFullHealing (shObject *can)
{
    if (Hero.fullHealing (NDX (4 + 2 * can->mBugginess, 8), 2))
        can->setKnown ();
    return FULLTURN;
}

static int
quaffRestoration (shObject *can)
{
    if (Hero.restoration (NDX (2, 2) + can->mBugginess)) {
        can->setKnown ();
    } else {
        can->maybeName ();
    }
    return FULLTURN;
}

static int
quaffBrain (shObject *can)
{
    can->setKnown ();
    can->setBugginessKnown ();

    if (!can->isBuggy ()) {
        I->p ("Brain food!");
        if (Hero.mMaxAbil.mInt - Hero.mExtraAbil.mInt < 25)
            ++Hero.mMaxAbil.mInt;
    } else {
        I->p ("Yuck!  Only zombies would enjoy such food.");
    }
    if (Hero.mAbil.mInt < Hero.mMaxAbil.mInt) {
        int bonus = 1;
        ++Hero.mAbil.mInt;
        if (can->isOptimized () and Hero.mAbil.mInt < Hero.mMaxAbil.mInt) {
            ++Hero.mAbil.mInt;
            ++bonus;
        }
        I->nonlOnce ();
        I->setColor (kLime);
        I->p ("  (%+d %s)", bonus, shAbilities::shortAbilName (kInt));
        I->setColor (kGray);
    }

    Hero.computeIntrinsics ();
    return FULLTURN;
}

static shAbilityIndex
chooseAbility (shCreature *c, bool restore)
{
    shMenu *menu = I->newMenu (restore ? "You may restore an ability:" :
        "You may increase an ability:", shMenu::kNoHelp);
    menu->attachHelp ("attribute.txt");
    int entries = 0;

    FOR_ALL_ABILITIES (a) { /* Prepare options: s - Strength. */
        if (!restore) {
            if (c->mMaxAbil.getByIndex (a) -
                c->mExtraAbil.getByIndex (a) >= ABILITY_MAX)
                continue;  /* Skip abilities already at ABILITY_MAX. */
        } else {
            int aval = c->mAbil.getByIndex (a);
            if (a == kPsi and c->isHero ())
                aval += Hero.mPsiDrain;
            if (aval >= c->mMaxAbil.getByIndex (a))
                continue;  /* Skil undamaged abilities. */
        }
        char *buf = GetBuf ();
        strncpy (buf, shAbilities::abilName (a), SHBUFLEN);
        buf[0] = toupper (buf[0]); /* Capitalize name. */
        menu->addIntItem (shAbilities::abilName (a)[0], buf, a);
        ++entries;
    }
    shAbilityIndex choice;

    if (entries == 0) { /* Choice does not matter but still has to be valid. */
        return shAbilityIndex (RNG (1, NUM_ABIL));
    }
    if (c != &Hero or entries == 1) {
        menu->getRandIntResult ((int *) &choice);
        return choice;
    }

    int menuresult;
    int tries = 5;
    do {
        menuresult = menu->getIntResult ((int *) &choice);
        menu->dropResults ();
    } while (!menuresult and --tries);
    if (tries == 0) { /* Undecided, eh?  We'll help. */
        menu->getRandIntResult ((int *) &choice);
    }
    delete menu;
    return choice;
}

void
shCreature::gainAbility (bool controlled, int num)
{   /* Spice Melange enables hero to control his or her biology
       like a Bene Gesserit reverend mother could. */
    controlled = controlled or Hero.getTimeOut (XRAYVISION);

    shAbilityIndex permute[NUM_ABIL] = {kStr, kCon, kAgi, kDex, kInt, kPsi};
    shuffle (permute, NUM_ABIL, sizeof (shAbilityIndex));

    for (int i = 0; i < num; ++i) {
        shAbilityIndex idx = permute[i];
        if (controlled) {
            idx = chooseAbility (this, false);
            controlled = false;
        }
        int a = mAbil.getByIndex (idx);
        int m = mMaxAbil.getByIndex (idx);
        if (m - mExtraAbil.getByIndex (idx) < ABILITY_MAX) {
            if (isHero ()) {
                I->setColor (kLime);
                I->p ("You feel %s!  (+1 %s)",
                      idx == kStr ? "strong" :
                      idx == kCon ? "tough" :
                      idx == kAgi ? "agile" :
                      idx == kDex ? "deft" :
                      idx == kInt ? "smart" :
                      idx == kPsi ? "weird" : "bugged",
                      shAbilities::shortAbilName (idx));
            }
            if (idx == kCon) {
                if (ABILITY_MODIFIER (m) != ABILITY_MODIFIER (m + 1)) {
                    mHP += mCLevel;
                    mMaxHP += mCLevel;
                }
            }
            mAbil.setByIndex (idx, a + 1);
            mMaxAbil.setByIndex (idx, m + 1);
            if (isHero ())  I->setColor (kGray);
        } else if (isHero ()) {
            I->p ("You are already as %s as you can get!",
                  idx == kStr ? "strong" :
                  idx == kCon ? "tough" :
                  idx == kAgi ? "agile" :
                  idx == kDex ? "deft" :
                  idx == kInt ? "smart" :
                  idx == kPsi ? "weird" : "bugged");
        }
    }
    computeIntrinsics ();
}

int
shCreature::needsRestoration ()
{
    FOR_ALL_ABILITIES (i) {
        int abil = mAbil.getByIndex (i);
        if (kPsi == i and isHero ()) {
            abil += Hero.mPsiDrain;
        }
        if (abil < mMaxAbil.getByIndex (i)) {
            return 1;
        }
    }
    return 0;
}

/* Returns: 1 if creature health was improved, 0 otherwise. */
int
shCreature::restoration (int howmuch)
{
    bool controlled = !!getTimeOut (XRAYVISION); /* Spice Melange effect. */
    if (is (kPlagued)) {
        cure (kPlagued);
        if (isHero ())  I->p ("You feel relieved!");
        return 1;
    } else {
        cure (kPlagued);
        /* No return.  Suppressed plague restores for free. */
    }

    if (howmuch == 0)  return 0;

    shAbilityIndex permute[NUM_ABIL] = {kStr, kCon, kAgi, kDex, kInt, kPsi};
    shuffle (permute, NUM_ABIL, sizeof (shAbilityIndex));
    int helped = 0;

    char statmod[20] = "";
    for (int i = 0; i < NUM_ABIL; ++i) {
        shAbilityIndex idx = permute[i];
        if (controlled) {
            idx = chooseAbility (this, true);
            controlled = false;
        }
        int a = mAbil.getByIndex (idx);
        int m = mMaxAbil.getByIndex (idx);
        int b = howmuch;

        if (isHero () and idx == kPsi) {
            /* Don't restore psionics drain. */
            a += Hero.mPsiDrain;
        }
        if (a < m) {
            if (helped) {
                computeIntrinsics ();
                if (isHero ()) {
                    I->p ("You feel restored.%s", statmod);
                }
                return 1;
            }
            if (a + b > m) { /* Is amount to restore too great? */
                b = m - a;
                helped = 1;
            }
            /* At this point it is known that 'b' points are restored. */
            snprintf (statmod, 20, "  (%+d %s)",
                      b, shAbilities::shortAbilName (idx));
            if (idx == kCon) {
                int hpgain = mCLevel *
                    (ABILITY_MODIFIER (a + b) - ABILITY_MODIFIER (a));
                mMaxHP += hpgain;
                mHP += hpgain;
            }
            mAbil.setByIndex (idx, mAbil.getByIndex (idx) + b);
            if (a + b == m) { /* Restored ability to full potential? */
                helped = 1;
                continue;
            } else { /* Ability is better but still damaged. */
                computeIntrinsics ();
                if (isHero ()) {
                    I->p ("You feel restored.%s", statmod);
                }
                return 1;
            }
            break;
        }
    }
    if (helped) {
        computeIntrinsics ();
        if (isHero ()) {
            I->p ("You feel fully restored.%s", statmod);
        }
        return 1;
    } else {
        if (isHero ())  I->p ("You feel great!");
        return 0;
    }
}

static int
quaffGainAbility (shObject *can)
{
    can->setKnown ();
    Hero.gainAbility (false, can->isOptimized () ? NUM_ABIL : 1);
    return FULLTURN;
}

static int
quaffRadAway (shObject *can)
{
    if (Hero.mRad > 0) {
        Hero.mRad -= 100 + NDX (3 + can->mBugginess, 50);
        if (Hero.mRad < 0) {
            Hero.mRad = 0;
        }
    }
    if (!Hero.mRad)
        I->p ("You feel purified.");
    else
        I->p ("You feel less contaminated.");

    can->setKnown ();
    return FULLTURN;
}

static int
quaffSpeed (shObject *can)
{
    int numdice = 12;
    if (can->isOptimized ()) {
        numdice = 20;
    } else if (can->isBuggy ()) {
        numdice = 4;
    }
    if (Hero.is (kHosed)) {
        Hero.cure (kHosed);
        Hero.checkTimeOuts ();
    }
    Hero.inflict (kSpeedy, FULLTURN * NDX (numdice, 20));
    can->setKnown ();
    return FULLTURN;
}

static int
quaffPlasma (shObject *can)
{
    can->setKnown ();
    I->p ("It shocks you!");
    if (Hero.sufferDamage (kAttPlasma)) {
        Hero.shCreature::die (kKilled, "drinking a canister of plasma");
    }
    Hero.abortion ();
    return FULLTURN;
}

static int
quaffNapalm (shObject *can)
{
    can->setKnown ();
    I->p ("It ignites!");
    if (Hero.sufferDamage (kAttNapalm)) {
        Hero.shCreature::die (kKilled, "drinking a canister of napalm");
    }
    Hero.abortion ();
    return FULLTURN;
}

static int
quaffLNO (shObject *can)
{
    can->setKnown ();
    I->p ("It freezes!");
    if (Hero.sufferDamage (kAttLNO)) {
        Hero.shCreature::die (kKilled, "drinking a canister of liquid nitrogen");
    }
    Hero.abortion ();
    return FULLTURN;
}

static int
quaffFuel (shObject *drum)
{   /* Poison resistance does not help against this. */
    int sdam = RNG (1, 3), cdam = RNG (1, 3);
    I->p ("Bleargh!  (-%d str, -%d con)", sdam, cdam);
    bool kill1 = Hero.sufferAbilityDamage (kStr, sdam);
    bool kill2 = Hero.sufferAbilityDamage (kCon, cdam);
    Hero.abortion ();
    if (kill1 or kill2) {
        Hero.shCreature::die (kKilled, "drinking flamethrower fuel");
    }
    return FULLTURN;
}

static int
quaffPoison (shObject *can)
{
    if (Hero.mBodyArmor and Hero.mBodyArmor->isA (kObjGreenCPA)) {
        I->p ("This nourishing drink has delicious taste!");
        Hero.healing (NDX (3 - can->mBugginess, 6), 1);
        int con = Hero.mAbil.mCon;
        int maxcon = Hero.mMaxAbil.mCon;
        if (con < maxcon) {
            if (can->isBuggy ()) {
                Hero.mAbil.mCon = maxcon;
                con = maxcon;
            } else {
                ++Hero.mAbil.mCon;
                ++con;
            }
            if (con == maxcon) {
                I->p ("You feel healthy.");
            } else {
                I->p ("You feel healthier.");
            }
        }
        can->maybeName ();
    } else {
        can->setKnown ();
        I->p ("This stuff must be poisonous.");
        int dmg = RNG (1, 3) - can->mBugginess;
        dmg -= Hero.mResistances[kToxic];
        if (dmg < 0)  dmg = 0;
        if (dmg and Hero.sufferAbilityDamage (kStr, dmg)) {
            Hero.shCreature::die (kKilled, "drinking a canister of poison");
        }
    }
    Hero.abortion ();
    return FULLTURN;
}

static int
quaffSpiceMelange (shObject *can)
{
    can->setKnown ();
    I->p ("You fold space.");
    if (1 == Hero.transport (-1, -1, 100, 1)) {
        Hero.shCreature::die (kKilled, "folding space");
    }
    I->p ("Your senses open to new dimensions.");
    Hero.computeIntrinsics ();
    Hero.setTimeOut (TELEPATHY, FULLTURN * NDX (10, 30));
    Hero.setTimeOut (XRAYVISION, FULLTURN * NDX (10, 30));
    Hero.inflict (kSpeedy, FULLTURN * NDX (5, 2));
    Hero.mInnateIntrinsics |= kTelepathy | kXRayVision;
    /* Add +20% chances for each Voice effect in power over you. */
    int chance = (Hero.is (kUnableCompute) > 0) * 20 +
        (Hero.is (kUnableUsePsi) > 0) * 20 + (Hero.is (kUnableAttack) > 0) * 20;
    if (RNG (100) < chance) {
        Hero.getMutantPower (kTheVoice);
    }
    return FULLTURN;
}

static int
quaffEmbryo (shObject *can)
{
    can->setKnown ();
	I->p ("You drink something writhing and slimy.");
	Hero.setStoryFlag ("impregnation", 1);
	return FULLTURN;
}

static int
useCanister (shObject *can)
{
    int t = useCommonCanister (can);
    if (t) {
        if (Hero.isInShop ()) {
            Hero.usedUpItem (can, 1, "use");
        }
        Hero.useUpOneObjectFromInventory (can);
    }
    return t;
}


void
initializeCanisters ()
{
    for (int i = kObjFirstCanister; i <= kObjLastCanister; ++i)
        AllIlks[i].mUseFunc = useCanister;
    AllIlks[kObjBeer].mQuaffFunc = quaffBeer;
    AllIlks[kObjSuperGlue].mQuaffFunc = quaffSuperGlue;
    AllIlks[kObjNanoCola].mQuaffFunc = quaffNanoCola;
    AllIlks[kObjNukaCola].mQuaffFunc = quaffNukaCola;
    AllIlks[kObjB3].mQuaffFunc = quaffB3;
    AllIlks[kObjCoffee].mQuaffFunc = quaffCoffee;
    AllIlks[kObjWater].mQuaffFunc = quaffWater;
    AllIlks[kObjRadAway].mQuaffFunc = quaffRadAway;
    AllIlks[kObjRestorationCanister].mQuaffFunc = quaffRestoration;
    AllIlks[kObjHealingCanister].mQuaffFunc = quaffHealing;
    AllIlks[kObjLNO].mQuaffFunc = quaffLNO;
    AllIlks[kObjNapalm].mQuaffFunc = quaffNapalm;
    AllIlks[kObjUniversalSolvent].mQuaffFunc = quaffUniversalSolvent;
    AllIlks[kObjSpeedCanister].mQuaffFunc = quaffSpeed;
    AllIlks[kObjPoisonCanister].mQuaffFunc = quaffPoison;
    AllIlks[kObjPlasmaCanister].mQuaffFunc = quaffPlasma;
    AllIlks[kObjMutagenCanister].mQuaffFunc = quaffMutagen;
    AllIlks[kObjFullHealingCanister].mQuaffFunc = quaffFullHealing;
    AllIlks[kObjGainAbilityCanister].mQuaffFunc = quaffGainAbility;
    AllIlks[kObjSpiceMelange].mQuaffFunc = quaffSpiceMelange;
    AllIlks[kObjAntimatter].mQuaffFunc = quaffAntimatter;
    AllIlks[kObjBrain].mQuaffFunc = quaffBrain;
    AllIlks[kObjCannedEmbryo].mQuaffFunc = quaffEmbryo;

    AllIlks[kObjFuelDrum].mQuaffFunc = quaffFuel;
}


int
quaffCanister (shObject *can)
{
    if (Hero.isInShop ()) {
        Hero.usedUpItem (can, 1, "drink");
        can->resetUnpaid ();
    }
    int elapsed = (can->myIlk ()->mQuaffFunc) (can);
    if (can->isKnown ()) { /* Identify rest of stack. */
        can->setAppearanceKnown ();
    }
    Hero.useUpOneObjectFromInventory (can);
    return elapsed;
}
