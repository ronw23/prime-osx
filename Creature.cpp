#include "Global.h"
#include "Util.h"
#include "Creature.h"
#include "Object.h"
#include "AttackType.h"
#include "Mutant.h"
#include "Hero.h"
#include <ctype.h>

int
shAbilities::getByIndex (shAbilityIndex idx) const
{
    switch (idx) {
    case kStr: return mStr;
    case kCon: return mCon;
    case kAgi: return mAgi;
    case kDex: return mDex;
    case kInt: return mInt;
    case kPsi: return mPsi;
    default: abort (); return -1;
    }
}

void
shAbilities::setByIndex (shAbilityIndex idx, int val)
{
    switch (idx) {
    case kStr: mStr = val; break;
    case kCon: mCon = val; break;
    case kAgi: mAgi = val; break;
    case kDex: mDex = val; break;
    case kInt: mInt = val; break;
    case kPsi: mPsi = val; break;
    default:
        debug.log ("Error: Trying to set ability %d to %d.", idx, val);
        break;
    }
}

void
shAbilities::changeByIndex (shAbilityIndex idx, int delta)
{
    setByIndex (idx, getByIndex (idx) + delta);
}

const char *
shAbilities::abilName (shAbilityIndex i)
{
    switch (i) {
    case kStr: return "strength";
    case kCon: return "constitution";
    case kAgi: return "agility";
    case kDex: return "dexterity";
    case kInt: return "intelligence";
    case kPsi: return "psionics";
    default:   return "bug affinity";
    }
}

const char *
shAbilities::shortAbilName (shAbilityIndex i)
{
    switch (i) {
    case kStr: return "Str";
    case kCon: return "Con";
    case kAgi: return "Agi";
    case kDex: return "Dex";
    case kInt: return "Int";
    case kPsi: return "Psi";
    default:   return "BUG";
    }
}

shAbilityIndex
shAbilities::getRandomIndex ()
{
    return (shAbilityIndex) RNG (1, 6);
}



shCreature::shCreature ()
    : mTimeOuts (), mSkills ()
{
    mX = -10;
    mY = -10;
    mZ = 0;
    mType = kHumanoid;
    mProfession = NULL;
    mName[0] = 0;
    mAP = -1000;
    mCLevel = 0;
    mAC = 0;
    mConcealment = 0;
    mToHitModifier = 0;
    mDamageModifier = 0;
    mPsiDrain = 0;
    mPsionicStormFatigue = 0;
    mLastRegen = MAXTIME;
    mLastLevel = NULL;
    mReflexSaveBonus = 0;
    mInnateIntrinsics = 0;
    mFeats = 0;
    mLevel = NULL;
    for (int i = 0; i < kMaxEnergyType; ++i) {
        mInnateResistances[i] = 0;
        mResistances[i] = 0;
    }
    for (int i = 0; i < kMaxAllPower; ++i) {
        mMutantPowers[i] = 0;
    }
    for (int i = 0; i < shObjectIlk::kMaxSite; ++i) {
        mImplants[i] = NULL;
    }
    for (int i = 0; i < TRACKLEN; ++i) {
        mTrack[i].mX = -1;
    }
    mConditions = 0;
    mRad = 0;

    mInventory = new shVector <shObject *> (8);
    mWeight = 0;
    mPsiModifier = 0;

    mSpeed = 100;
    mSpeedBoost = 0;
    mBodyArmor = NULL;
    mJumpsuit = NULL;
    mHelmet = NULL;
    mBoots = NULL;
    mBelt = NULL;
    mGoggles = NULL;
    mCloak = NULL;
    mWeapon = NULL;
    mState = kWaiting;
    mHidden = 0;
    mMimic = kNothing;
    mSpotAttempted = 0;
    mTrapped.mInPit = 0;
    mTrapped.mDrowning = 0;
    mTrapped.mWebbed = 0;
    mTrapped.mTaped = 0;
    /* Prevent motion tracker from picking up things it should not. */
    mLastMoveTime = -10000;
}


shCreature::~shCreature ()
{
    delete mInventory;
}


shMonsterIlk *
shCreature::myIlk ()
{
    return &MonIlks[mIlkId];
}


int
shCreature::isA (shMonId id)
{
    return mIlkId == id;
}


const char *
shCreature::herself ()
{
    if (!hasMind ())
        return "itself";
    if (kFemale == mGender)
        return "herself";
    else if (kMale == mGender)
        return "himself";
    else
        return "itself";
}


const char *
shCreature::her (const char *thing)
{
    char *buf = GetBuf ();

    snprintf (buf, SHBUFLEN, "%s %s",
              !hasMind () ? "its" :
              kFemale == mGender ? "her" :
              kMale   == mGender ? "his" : "its", thing);
    return buf;
}



shCreature::TimeOut *
shCreature::getTimeOut (shTimeOutType key)
{
    for (int i = 0; i < mTimeOuts.count (); ++i)
        if (key == mTimeOuts.get (i) -> mKey)
            return mTimeOuts.get (i);
    return NULL;
}


shTime
shCreature::setTimeOut (shTimeOutType key, shTime howlong, int additive)
{
    TimeOut *t = getTimeOut (key);

    if (NULL == t) {
        mTimeOuts.add (new TimeOut (key, howlong + Clock));
        return howlong + Clock;
    } else {
        if (additive) {
            t->mWhen += howlong;
        } else if (0 == howlong) {
            t->mWhen = 0;
        } else {
            t->mWhen = maxi (howlong + Clock, t->mWhen);
        }
        return t->mWhen;
    }
}


/* returns non-zero iff the creature dies */
int
shCreature::checkTimeOuts ()
{
    TimeOut *t;
    int donotremove = 0;
    int oldcond = mConditions;

    for (int i = mTimeOuts.count () - 1; i >= 0; --i) {
        t = mTimeOuts.get (i);
        if (t->mWhen <= Clock) {
            if (XRAYVISION == t->mKey) {
                mInnateIntrinsics &= ~kXRayVision;
                computeIntrinsics ();
                if (isHero () and
                    hasPerilSensing () and
                    mGoggles->isToggled ())
                {
                    I->p ("You can no longer see through "
                          "your darkened goggles.");
                }
                interrupt ();
            } else if (TELEPATHY == t->mKey) {
                mInnateIntrinsics &= ~kTelepathy;
                computeIntrinsics ();
                interrupt ();
            } else if (STUNNED == t->mKey) {
                mConditions &= ~kStunned;
                if (isHero ()) {
                    I->drawSideWin ();
                    I->p ("You feel steadier.");
                }
                interrupt ();
            } else if (CAFFEINE == t->mKey) {
                mInnateResistances[kMesmerizing] -= 25;
                if (isHero ()) {
                    I->p ("You feel caffeine effects decline.");
                }
                interrupt ();
            } else if (CONFUSED == t->mKey) {
                mConditions &= ~kConfused;
                if (isHero ()) {
                    I->drawSideWin ();
                    I->p ("You feel less confused.");
                }
                interrupt ();
            } else if (HOSED == t->mKey) {
                mConditions &= ~kHosed;
                computeIntrinsics ();
                if (isHero ()) {
                    I->drawSideWin ();
                    I->p ("You are no longer hosed.");
                }
                interrupt ();
            } else if (PARALYZED == t->mKey) {
                mConditions &= ~kParalyzed;
                if (isHero ()) {
                    I->drawSideWin ();
                    I->p ("You can move again.");
                }
                interrupt ();
            } else if (FLEEING == t->mKey) {
                mConditions &= ~kFleeing;
            } else if (FRIGHTENED == t->mKey) {
                mConditions &= ~kFrightened;
                if (isHero ()) {
                    I->drawSideWin ();
                    if (oldcond & kFrightened)
                        I->p ("You regain your courage.");
                }
                interrupt ();
            } else if (BLINDED == t->mKey) {
                mInnateIntrinsics &= ~kBlind;
                computeIntrinsics ();
                if (isHero () and !isBlind ()) {
                    I->drawSideWin ();
                    I->p ("You can see again.");
                }
                interrupt ();
            } else if (VIOLATED == t->mKey) {
                mConditions &= ~kViolated;
                if (isHero ()) {
                    I->drawSideWin ();
                    I->p ("You don't feel so sore anymore.");
                }
                computeIntrinsics ();
                interrupt ();
            } else if (PLAGUED == t->mKey) {
                if (!is (kPlagued)) {
                    /* Hero bought some time. Now it ends. */
                    if (isHero ()) {
                        I->p ("The ilness is coming back.");
                        interrupt ();
                    }
                    mConditions |= kPlagued;
                    t->mWhen += 3000;
                } else {
                    int dmg = 0, statloss = 0;
                    shAbilityIndex stat = (shAbilityIndex) RNG (1, 6);
                    if (stat != kPsi or !isHero ()) {
                        if (mAbil.getByIndex (stat) -
                            mExtraAbil.getByIndex (stat) >
                            mMaxAbil.getByIndex (stat) / 2)
                        {
                            sufferAbilityDamage (stat, 1);
                            statloss = 1;
                        }
                    } else {
                        int upkeep = 0;
                        for (int i = kNoMutantPower; i <= kMaxHeroPower; ++i) {
                            if (mMutantPowers[i] > 1) {
                                upkeep += MutantPowers[i].mLevel;
                            }
                        }
                        if (mAbil.getByIndex (stat) -
                            mExtraAbil.getByIndex (stat) +
                            Hero.mPsiDrain + upkeep >
                            (mMaxAbil.getByIndex (stat) + upkeep) / 2)
                        {
                            sufferAbilityDamage (stat, 1);
                            statloss = 1;
                        }
                    }
                    if (mHP > mMaxHP / 2) {
                        int diff = mHP - mMaxHP / 2;
                        if (diff > 5) diff = 5;
                        dmg = RNG (1, diff);
                        mHP -= dmg;
                    }
                    if (dmg or statloss) { /* Print ilness message. */
                        if (statloss) {
                            I->p ("Plague tortures your %s.",
                                stat >= kInt ? "mind" : "body");
                        } else {
                            I->p ("You feel sick.");
                        }
                        /*if (dmg) {
                            I->nonlOnce ();
                            I->p ("  (%d dam)", dmg);
                        } disabled - too much spamming */
                        if (statloss) {
                            I->nonlOnce ();
                            char *buf = GetBuf ();
                            strcpy (buf, shAbilities::abilName (stat));
                            buf[3] = 0;
                            buf[0] = toupper (buf[0]);
                            I->p ("  (-1 %s)", buf);
                        }
                    }
                    /* This must be cured. */
                    t->mWhen += 5000 + RNG (6) * 1000;
                }
                donotremove = 1;
            } else if (SICKENED == t->mKey) {
                if (sewerSmells ()) {
                    /* keep sick */
                    t->mWhen += FULLTURN;
                    donotremove = 1;
                } else {
                    interrupt ();
                    mConditions &= ~kSickened;
                }
            } else if (SPEEDY == t->mKey) {
                mConditions &= ~kSpeedy;
                if (isHero ()) {
                    I->p ("You slow down.");
                }
                computeIntrinsics ();
                interrupt ();
            } else if (SLOWED == t->mKey) {
                mConditions &= ~kSlowed;
                if (isHero ()) {
                    I->p ("You speed up.");
                }
                computeIntrinsics ();
                interrupt ();
            } else if (ASLEEP == t->mKey) {
                if (is (kAsleep)) {
                    mConditions &= ~kAsleep;
                    if (isHero ()) {
                        I->drawSideWin ();
                        I->p ("You wake up.");
                    }
                }
                interrupt ();
            } else if (VOICE_NAMSHUB == t->mKey) {
                mConditions &= ~kUnableCompute;
                if (isHero ()) {
                    I->p ("The command not to use the Nam-Shub has weakened.");
                }
                computeIntrinsics ();
                interrupt ();
            } else if (VOICE_MUTANT == t->mKey) {
                mConditions &= ~kUnableUsePsi;
                if (isHero ()) {
                    I->p ("You overcome distaste towards your mutant nature.");
                }
                computeIntrinsics ();
                interrupt ();
            } else if (VOICE_VIOLENCE == t->mKey) {
                mConditions &= ~kUnableAttack;
                if (isHero ()) {
                    I->p ("You again will fight when it is necessary.");
                }
                computeIntrinsics ();
                interrupt ();
            /* Silently remove this one. */
            //} else if (VOICE_GENEROSITY == t->mKey) {
            } else if (TRAPPED == t->mKey) {
                if (isTrapped ()) {
                    shFeature *f = mLevel->getFeature (mX, mY);
                    if (f and shFeature::kAcidPit == f->mType) {
                        bool resists = mResistances[kCorrosive] >
                            Attacks[kAttAcidPitBath].mDamage[0].mHigh;
                        if (!resists) {
                            if (isHero ()) {
                                I->p ("You are bathing in acid!");
                            } else if (Hero.canSee (this)) {
                                if (!isSessile ()) {
                                    I->p ("%s splashes in the acid!", the ());
                                } else {
                                    I->p ("%s is being slowly dissolved "
                                        "by the acid.", the ());
                                }
                            }
                        }
                        if (sufferDamage (kAttAcidPitBath)) {
                            if (!isHero () and Hero.canSee (this)) {
                                pDeathMessage (the (), kSlain);
                            }
                            if (isHero ()) {
                                die (kMisc, "Dissolved in acid");
                            } else {
                                die (kSlain, "acid");
                            }
                            return -1;
                        }
                    }

                    if (mTrapped.mDrowning) {
                        --mTrapped.mDrowning;
                        if (0 == mTrapped.mDrowning) {
                            if (!isHero ()) {
                                I->p ("%s drowns!", the ());
                            }
                            if (kSewage == mLevel->getSquare (mX, mY) ->mTerr) {
                                die (kDrowned, "in a pool of sewage");
                            } else {
                                die (kDrowned, "in a pool");
                            }
                            return -1;
                        } else if (2 == mTrapped.mDrowning) {
                            if (isHero ()) I->p ("You're drowning!");
                        } else if (5 == mTrapped.mDrowning) {
                            if (isHero ()) I->p ("You're almost out of air!");
                        }
                    }
                    t->mWhen += FULLTURN;
                    ++i;
                    donotremove = 1;
                }
            }

            if (!donotremove) {
                mTimeOuts.remove (t);
                delete t;
            }
        }
    }
    return 0;
}


/* Relies on an innate intrinsic instead of condition. Not good ... */
void
shCreature::makeBlinded (int howlong)
{
    mInnateIntrinsics |= kBlind;
    computeIntrinsics ();
    setTimeOut (BLINDED, howlong);
}


int
isAdditiveTimeOut (shTimeOutType tm_out)
{
    switch (tm_out) {
    case ASLEEP: case FLEEING: case FRIGHTENED: case PARALYZED: case VIOLATED:
        return 0;
    default:
        return 1;
    }
}

shTimeOutType
conditionToTimeOut (shCondition cond)
{
    switch (cond) {
    case kAsleep:        return ASLEEP;
    case kConfused:      return CONFUSED;
    case kFleeing:       return FLEEING;
    case kFrightened:    return FRIGHTENED;
    case kGenerous:      return VOICE_GENEROSITY;
    case kHosed:         return HOSED;
    case kParalyzed:     return PARALYZED;
    case kPlagued:       return PLAGUED;
    case kSickened:      return SICKENED;
    case kSlowed:        return SLOWED;
    case kSpeedy:        return SPEEDY;
    case kStunned:       return STUNNED;
    case kUnableCompute: return VOICE_NAMSHUB;
    case kUnableUsePsi:  return VOICE_MUTANT;
    case kUnableAttack:  return VOICE_VIOLENCE;
    case kViolated:      return VIOLATED;
    default:             return NOTIMEOUT;
    }
}

int
shCreature::is (shCondition cond)
{
    return mConditions & cond;
}

void
shCreature::inflict (shCondition cond, int howlong)
{
    shTimeOutType tm_out = conditionToTimeOut (cond);
    assert (tm_out != NOTIMEOUT);

    switch (cond) {
    case kSpeedy:
        if (is (kSpeedy)) {
            howlong /= 2;
            if (isHero ())  I->p ("You feel a bit speedier.");
        } else {
            if (isHero ())
                I->p ("You feel speedy!");
            else if (Hero.canSee (this))
                I->p ("%s speeds up!", the ());
        }
        break;
    case kSlowed:
        if (is (kSlowed)) {
            howlong /= 2;
            if (isHero ())  I->p ("You feel a bit slower.");
        } else {
            if (isHero ())
                I->p ("You feel sluggish...");
            else if (Hero.canSee (this))
                I->p ("%s slows down.", the ());
        }
        break;
    case kHosed:
        if (is (kHosed)) howlong /= 2;
        if (isHero ()) I->p ("Your connection is hosed!");
        break;
    case kParalyzed:
        if (is (kParalyzed)) return;
        if (isHero ()) {
            I->p ("You can't move!");
        } else if (Hero.canSee (this)) {
            I->p ("%s can't move!", the ());
        }
        break;
    case kPlagued:
        if (isHero ()) {
            if (is (kPlagued)) {
                /* Assure imminent stat degrade. */
                clearTimeOut (PLAGUED);
                setTimeOut (PLAGUED, 100, 0);
                return;
            } else {
                I->p ("You feel terribly sick.");
            }
        }
        break;
    case kSickened:
        if (isHero ()) {
            if (is (kSickened)) {
                I->p ("You feel worse.");
                howlong /= 2;
            } else {
                I->p ("You feel sick.");
            }
        }
        break;
    case kStunned:
        if (isHero ()) {
            if (is (kStunned)) {
                I->p ("You reel...");
                howlong /= 2;
            } else {
                I->p ("You stagger...");
            }
        }
        checkConcentration ();

        if (mWeapon and RNG (2) and
            /* No dropping weapons like power fist. */
            mWeapon->myIlk ()->mMeleeSkill != kUnarmedCombat)
        {
            shObject *obj = mWeapon;
            unwield (obj);
            removeObjectFromInventory (obj);
            if (isHero ()) {
                I->drawSideWin ();
                I->p ("You drop %s!", obj->your ());
            } else if (Hero.canSee (this)) {
                I->p ("%s drops %s!", the (), obj->her (this));
                if (obj->isUnpaid () and !Level->isInShop (mX, mY)) {
                    obj->resetUnpaid ();
                }
            }
            Level->putObject (obj, mX, mY);
        }
        break;
    case kViolated:
        if (isHero ()) {
            if (is (kAsleep)) {
                I->p ("You have an unpleasant dream!");
            } else {
                I->p ("You feel violated!");
                checkConcentration ();
            }
        } else if (Hero.canSee (this)) {
            I->p ("%s looks a little uncomfortable.", the ());
        }
        break;
    default: break;
    }

    mConditions |= cond;
    computeIntrinsics ();
    setTimeOut (tm_out, howlong, isAdditiveTimeOut (tm_out));
}

void
shCreature::cure (shCondition cond)
{
    shTimeOutType tm_out = conditionToTimeOut (cond);
    assert (tm_out != NOTIMEOUT);
    mConditions &= ~cond;
    clearTimeOut (tm_out);

    switch (cond) {
    case kAsleep:
        if (isHero () and mHP > 0) {
            I->drawSideWin ();
            I->p ("You wake up.");
        }
        break;
    case kPlagued:
        /* Plague timeout must be found and cut out.  Clearing is not enough. */
        for (int i = mTimeOuts.count () - 1; i >= 0; --i) {
            TimeOut *t = mTimeOuts.get (i);
            if (t->mKey == PLAGUED) {
                mTimeOuts.remove (t);
                delete t;
                break;
            }
        }
        break;
    default: break;
    }
}


void
shCreature::sterilize ()
{
    mIntrinsics &= ~kMultiplier;
}

/* Some items modify character abilities. */
const shAbilities *
shObject::getAbilityModifiers ()
{
    static const shAbilities
         /* STR CON AGI DEX INT PSI */
    BRPH = { 0,  0,  0,  0,  0, +1},
    BRPA = {+2,  0, -2,  0,  0,  0},
    PCPA = { 0,  0, +3, -4,  0,  0},
    RCPA = {+3,  0,  0,  0, -4,  0},
    BCPA = { 0,  0, -4,  0,  0, +3},
    GCPA = { 0, +3,  0,  0,  0, -4},
    MBA2 = {+1, +1, +1, +1, +1, +1},
    EBA2 = { 0, +1, +1,  0,  0,  0},
    EBA3 = { 0,  0,  0,  0,  0, +1},
    SBA3 = { 0, +2, +2, +1,  0,  0},
    SBA4 = { 0,  0,  0,  0,  0, +2},
    PSIB = { 0,  0,  0,  0,  0, -2};

    switch (mIlkId) {
    case kObjBrotherhoodPH:   return &BRPH;
    case kObjBrotherhoodPA:   return &BRPA;
    case kObjPinkCPA:         return &PCPA;
    case kObjRedCPA:          return &RCPA;
    case kObjBlueCPA:         return &BCPA;
    case kObjGreenCPA:        return &GCPA;
    case kObjMBioArmor2:      return &MBA2;
    case kObjEBioArmor2:      return &EBA2;
    case kObjEBioArmor3:      return &EBA3;
    case kObjSBioArmor3:      return &SBA3;
    case kObjSBioArmor4:      return &SBA4;
    case kObjPsiBlades:       return &PSIB;
    default: return NULL;
    }
}


void
shObject::applyConferredResistances (shCreature *target)
{
    if (isA (kArmor) and isWorn ()) {
        target->mSpeedBoost += myIlk ()->mSpeedBoost;
        for (int i = 0; i < kMaxEnergyType; ++i) {
            target->mResistances[i] += myIlk ()->mResistances[i];
        }
        const shAbilities *a = getAbilityModifiers ();
        if (a) {
            setAppearanceKnown ();
            setKnown (); /* Enhancement of abilities gives the item away. */
            FOR_ALL_ABILITIES (i) {
                target->mExtraAbil.changeByIndex (i, a->getByIndex (i));
            }
        }
        target->mPsiModifier += getPsiModifier ();
        target->mToHitModifier += myIlk ()->mToHitModifier;
        target->mDamageModifier += myIlk ()->mDamageModifier;
    } else if (isA (kImplant) and isWorn ()) {
        target->mPsiModifier += getPsiModifier ();
        if (isA (kObjCerebralCoprocessor)) {
            target->mExtraAbil.mInt += mEnhancement;
        } else if (isA (kObjMotoricCoordinator)) {
            target->mExtraAbil.mDex += mEnhancement;
        } else if (isA (kObjReflexCoordinator)) {
            target->mExtraAbil.mAgi += mEnhancement;
        } else if (isA (kObjAdrenalineGenerator)) {
            target->mExtraAbil.mStr += mEnhancement;
        } else if (isA (kObjPoisonResistor)) {
            target->mResistances[kToxic] += 10;
        } else if (isA (kObjGloryDevice)) {
            target->mFeats |= kExplodes;
        }
    }
}

/* Adds skill bonus unless target does not have the skill at all. */
void
shObject::applyConferredSkills (shCreature *target)
{
    shSkill *s;
    if (isA (kObjKhaydarin) and isWielded ()) {
        s = target->getSkill (kConcentration);
        if (s) s->mBonus += mCharges;
    } else if (isA (kObjM57Smartgun) and isWielded ()) {
        s = target->getSkill (myIlk ()->mGunSkill);
        int base = 7 + 2 * isOptimized ();
        /* Yes, this hurts when hero is better at heavy guns. */
        if (s) s->mBonus += base - s->mRanks;
    } else if (isA (kObjM56Smartgun) and isWielded ()) {
        s = target->getSkill (myIlk ()->mGunSkill);
        int base = 3 + 2 * mBugginess;  /* Smaller bonus but reliable. */
        if (s) s->mBonus += base;
    } else if (isA (kObjStabilizerBelt) and isWorn () and
               isKnown () and isAppearanceKnown ())
    {   /* You must be aware of its function to benefit from the belt. */
        s = target->getSkill (kHeavyGun);
        if (s) s->mBonus += 1 + isOptimized ();
    } else if (isA (kObjGenericSkillsoft) and isWorn ()) {
        shSkillCode c = myIlk ()->mBoostedSkill;
        if (c == kNoSkillCode) return;
        /* buggy:+0  debugged:+4  optimized:+8 */
        int bonus = (mBugginess + 1) * 4;
        s = target->getSkill (c);
        /* Two skillsoft implants should not add up. Best should count. */
        if (s) s->mBonus = maxi (s->mBonus, bonus);
    } else if (isA (kObjMechaDendrites) and isWorn ()) {
        /* Mecha-dendrites are implanted in cerebellum and thus need not
           worry about duplicating skill bonus effect of some other implant
           of same kind.  Bonuses are added without maxi () guard. */
        s = target->getSkill (kRepair);
        /* Grants +1,+3,+5 to repair skill. */
        if (s) s->mBonus = 4 + 2 * mBugginess;
        /* Grants +0,+1,+2 to programming skill if hero knows any of it. */
        s = target->getSkill (kHacking);
        if (s and s->mRanks) s->mBonus += 1 + mBugginess;
        /* Same for lock pick skill. */
        s = target->getSkill (kOpenLock);
        if (s and s->mRanks) s->mBonus += 1 + mBugginess;
    } else if (isA (kObjSBioArmor4) and isWorn ()) {
        for (int i = kIllumination; i <= kMaxHeroPower; ++i) {
            s = target->getSkill (kMutantPower, (shMutantPower) i);
            if (s) s->mBonus += maxi (1, 7 - MutantPowers[i].mLevel);
        }
    } else if (isA (kObjTorc) and isWorn () and !isBuggy ()) {
        static const shMutantPower psiPowers[] =
        {
            kIllumination, kHypnosis, kOpticBlast, kTelepathyPower,
            kMentalBlast, kTeleport, kPsionicStorm
        };
        const int numPsiPowers = sizeof (psiPowers) / sizeof (shMutantPower);
        int boost = 1 + isOptimized ();
        /* Torc amplifies psionic powers better than metabolic ones. */
        for (int i = 0; i <= numPsiPowers; ++i) {
            s = target->getSkill (kMutantPower, psiPowers[i]);
            if (s) s->mBonus +=
                MutantPowers[psiPowers[i]].mLevel <= 3 ? boost + 1 : boost;
        }
        for (int i = kIllumination; i <= kMaxHeroPower; ++i) {
            s = target->getSkill (kMutantPower, (shMutantPower) i);
            if (s) s->mBonus +=
                MutantPowers[i].mLevel <= 3 ? boost : 1;
        }
    }
}


int
shCreature::hasCamouflage ()
{   /* Smaller worms hide well in sludge. */
    if (mType == kVermin and myIlk ()->mSize <= kSmall and
        Level->getSquare (mX, mY)->mTerr == kSewage)
            return 1;
    if (!(kCamouflaged & mIntrinsics))
        return 0;
    if (!mLevel->isLit (mX, mY, mX, mY))  return 1;
    if (isInPit ())  return 1;
    for (int i = 0; i < 4; ++i) { /* Test orthogonal spaces. */
        int xmod = (i < 2) ? i*2 - 1 : 0;
        int ymod = (i > 1) ? i*2 - 5 : 0;
        if (mLevel->isInBounds (mX + xmod, mY + ymod) and
            mLevel->isObstacle (mX + xmod, mY + ymod))
            return 1; /* One wall is enough. */
    }
    return 0;
}


int
shCreature::canSee (int x, int y)
{
    if (isBlind ()) {
        return 0;
    } else if (isHero ()) {
        if (areAdjacent (x, y, mX, mY)) {
            return 100;
        } else if (mLevel->isInLOS (x, y)) {
            if (mLevel->isLit (x, y, mX, mY) or
                (hasLightSource () and distance (this, x, y) <= 25))
            {
                return 100;
            }
        } else if (mLevel->isLit (x, y, mX, mY) and
                   hasXRayVision () and distance (this, x, y) < 25)
        {
            return 100;
        }
    } else {
        return 100;
    }
    return 0;
}


int
shCreature::canSee (shCreature *c)
{
    if (isBlind ()) {
        return (c->mX == mX and c->mY == mY) ? 100 : 0;
    } else if (c->isHero ()) {
        if (areAdjacent (mX, mY, c->mX, c->mY))
            return 100;
        if ((Level->isInLOS (mX, mY) and !c->isInvisible () and
             (!c->hasCamouflage () or (!isHero () and
              /* Lurking monsters can be fooled by camouflage. */
              ((shMonster *) this)->mStrategy != shMonster::kLurk))) or
            (hasXRayVision () and distance (this, c->mX, c->mY) < 25 and
             !c->isInvisible ()))
        {
            return 100;
        }
    } else if (isHero ()) {
        if (areAdjacent (mX, mY, c->mX, c->mY))
            return 100;
        if (mLevel->isInLOS (c->mX, c->mY)) {
            if ((mLevel->isLit (c->mX, c->mY, mX, mY) and !c->hasCamouflage ()
                and !c->isInvisible ()) or
                (hasLightSource () and distance (this, c->mX, c->mY) <= 25 and
                 !c->isInvisible ()) or
                (hasEMFieldVision () and c->myIlk ()->mType == kAlien and
                 distance (this, c->mX, c->mY) <= 100) or
                (hasNightVision () and c->radiates () and !c->hasCamouflage ()
                 and distance (this, c->mX, c->mY) <= 100))
            {
                return 100;
            }
        } else if (mLevel->isLit (c->mX, c->mY, mX, mY) and hasXRayVision () and
                   distance (this, c->mX, c->mY) < 25 and !c->isInvisible ())
        {
            return 100;
        }
    } else {
        return canSee (c->mX, c->mY);
    }
    return 0;
}


void
shCreature::levelUp ()
{
    if (mState == kDead) return;

    ++mCLevel;

    mReflexSaveBonus = mProfession->mReflexSaveBonus * mCLevel / 4;
    mWillSaveBonus = mProfession->mWillSaveBonus * mCLevel / 4;

    if (isHero ()) {
        I->p ("You've obtained level %d!", mCLevel);
        if (mCLevel % 3 == 0) {
            I->p ("Your professional rank is now %s.", Hero.getTitle ());
        }
        I->drawSideWin ();
        I->pause ();
    }
    /* Gain hit points. */
    int x = ABILITY_MODIFIER (getCon ()) + RNG (1, mProfession->mHitDieSize);
    if (x < 1) x = 1;
    if (isHero ()) ++x; /* Fudge things slightly in favor of the hero... */
    mMaxHP += x;
    mHP += x;

    if (isOrc () and needsRestoration ())
        restoration (RNG (1, 2));

    if (isHero ()) {
        if (0 == mCLevel % 4) {
            I->drawSideWin ();
            Hero.gainAbility (true, 1);
            Hero.computeAC ();
        }
        Hero.addSkillPoints (Hero.mProfession->mNumPracticedSkills);
        I->p ("You may advance your skills with the '%s' command.",
              I->getKeyForCommand (shInterface::kEditSkills));
        if (mProfession == XelNaga and mCLevel == 6) {
            mInnateIntrinsics |= kJumpy;
            I->p ("You feel like jumping around.");
        }
        if (mProfession == XelNaga and mCLevel == 12) {
            mInnateIntrinsics |= kTelepathy;
            I->p ("Your dormant instincts awake.");
        }
        if (mProfession == Janitor and mCLevel == 10) {
            mIlkId = kMonSpaceOrcBoss;
            I->p ("Ur da boss now!");
            mGlyph = myIlk ()->mGlyph;
        }
        Hero.computeIntrinsics ();
    }
}


//RETURNS: 1 if attack is deadly (caller responsible for calling die ()); o/w 0
int
shCreature::sufferAbilityDamage (shAbilityIndex idx, int amount,
                                 int ispermanent /* = 0 */)
{
    int oldscore = mAbil.getByIndex (idx);
    int oldmodifier = ABILITY_MODIFIER (oldscore);
    mAbil.setByIndex (idx, mAbil.getByIndex (idx) - amount);
    if (ispermanent) {
        int newamount = mMaxAbil.getByIndex (idx) - amount;
        if (newamount < 0) {
            newamount = 0;
        }
        mMaxAbil.setByIndex (idx, newamount);
    }

    if (kCon == idx) {
        /* constitution damage results in lost HP */
        int hploss = mCLevel *
            (oldmodifier - ABILITY_MODIFIER (mAbil.getByIndex (idx)));

        mMaxHP -= hploss;
        if (mMaxHP < mCLevel) {
            hploss -= (mCLevel - mMaxHP);
            mMaxHP = mCLevel;
        }
        mHP -= hploss;
        if (mHP <= 0) {
            mHP = 0;
            if (&Hero == this) {
                I->drawSideWin ();
                I->drawLog ();
            }
            return 1;
        }
    } else if (kPsi == idx) {
        /* May lose the strength to maintain powers. */
        if (isHero ()) {
            if (mPsiDrain > 0) { /* Lose drained points first. */
                --mPsiDrain;
                ++mAbil.mPsi;
            } else if (0 <= getPsi () + Hero.mPsiDrain) {
                shMutantPower powers[kMaxHeroPower];
                int on = 0;
                for (int i = kNoMutantPower; i <= kMaxHeroPower; ++i) {
                    if (mMutantPowers[i] > 1) {
                        powers[on++] = (shMutantPower) i;
                    }
                }
                if (on) {
                    int chosen = RNG (on);
                    Hero.stopMutantPower (powers[chosen]);
                    --Hero.mPsiDrain;
                    ++mAbil.mPsi;
                    I->p ("You fail to maintain a power.");
                }
            }
        }
    }

    computeIntrinsics ();

    if (mAbil.getByIndex (idx) <= 0) {
        mAbil.setByIndex (idx, 0);
        if (&Hero == this) {
            I->drawSideWin ();
            I->drawLog ();
        }
        return 1;
    }
    return 0;
}


static int
reflectDir (shDirection *dir, int scatter)
{
    switch (RNG(scatter)) {
    case 0: *dir = uTurn (*dir); return 1;
    case 1: *dir = rightTurn (*dir); return 0;
    case 2: *dir = leftTurn (*dir); return 0;
    case 3: *dir = rightTurn (rightTurn (*dir)); return 0;
    case 4: *dir = leftTurn (leftTurn (*dir)); return 0;
    case 5: *dir = rightTurn (uTurn (*dir)); return 0;
    case 6: *dir = leftTurn (uTurn (*dir)); return 0;
    default: return 0;
    }
}


/* if the attack is reflected, prints a message, modifies dir, and returns 1
   o/w returns 0

*/
int
shCreature::reflectAttack (shAttack *attack, shDirection *dir)
{
    if ((kLaser == attack->mDamage[0].mEnergy and hasReflection ()) or
        (kParticle == attack->mDamage[0].mEnergy and
         mWeapon and mWeapon->isA (kObjLightSaber)))
    {
        const char *who = the ();
        int refl = 0;

        if (mWeapon and mWeapon->isA (kObjLightSaber)) {
            /* This could possibly involve a skill check?
               Note you can parry even when blind! */
            refl = reflectDir (dir, 7);
            if (isHero ()) {
                I->p ("You parry the %s with %s!",
                      attack->noun (), mWeapon->your ());
            } else {
                I->p ("%s parries the %s with %s!", who, attack->noun (),
                      mWeapon->her (this));
            }
        } else if (Hero.isBlind ()) { /* silently reflect */
            refl = reflectDir (dir, 7);
            return 1;
        } else if (mHelmet and mHelmet->isA (kObjBrainShield)) {
            refl = reflectDir (dir, 7);
            if (isHero ()) {
                I->p ("The %s is %s by your shiny hat!", attack->noun (),
                      refl ? "reflected" : "deflected");
            } else {
                I->p ("The %s is %s by %s's shiny hat!", attack->noun (),
                      refl ? "reflected" : "deflected", who);
            }
            mHelmet->setKnown ();
        } else if (mBodyArmor and mBodyArmor->isA (kObjReflecSuit)) {
            refl = reflectDir (dir, 7);
            if (isHero ()) {
                I->p ("The %s is %s by your shiny armor!", attack->noun (),
                      refl ? "reflected" : "deflected");
            } else {
                I->p ("The %s is %s by %s's shiny armor!", attack->noun (),
                      refl ? "reflected" : "deflected", who);
            }
            mBodyArmor->setKnown ();
        } else {
            refl = reflectDir (dir, 7);
            if (isHero ()) {
                I->p ("The %s is %s by your shiny skin!", attack->noun (),
                      refl ? "reflected" : "deflected");
            } else {
                I->p ("The %s is %s by %s's shiny skin!", attack->noun (),
                      refl ? "reflected" : "deflected", who);
            }
        }
        return 1;
    } else {
        return 0;
    }
}

/* Imported from Fight.cpp: */
extern const char *youHitMonMesg (shAttack::Type);
extern const char *youShootMonMesg (shAttack::Type);
/* works out damage done by an attack that has hit.  applies resistances,
   special defences, special damage (e.g. stunning), and subtracts HP.
   returns: 1 if attack kills us; o/w 0
*/
int
shCreature::sufferDamage (shAttack *attack, shCreature *attacker,
    shObject *weapon, int bonus, int multiplier, int divisor)
{
    if (kDead == mState)
        return 0;

    interrupt ();

    /* Quarterbacks are expert at kicking and take no damage. */
    if (isHero () and attack == &Attacks[kAttKickedWall] and
        mProfession == Quarterback)
        return 0;

    int totaldamage = 0;
    int shieldworked = 0;
    int absorbed = 0;
    int torcdiff = 0;
    int lethal = 0; /* At least one point of lethal damage was sent. */
    int oldhp = mHP;
    int dmgtable[3] = {0, 0, 0};
    bool process[3];

    const char *thewho = the ();

    /* Recognize this type of flaslight wielded by monster as dangerous. */
    if (weapon and weapon->isA (kObjLightSaber) and isHero () and !isBlind ())
        weapon->setKnown ();

    if (attacker and attacker->isHero () and mHidden) {
        mHidden = 0; /* Reveal hit (accidentally or not) targets. */
    }

    /* Iterate through three possible types of damage. */
    /* Phase 1: Collect damage. */
    for (int i = 0; i < 3; ++i) {
        shEnergyType energy = (shEnergyType) attack->mDamage[i].mEnergy;
        if (kNoEnergy == energy)
            break;

        /* Some damages do not kick in always. */
        if (RNG (100) >= attack->mDamage[i].mChance) {
            process[i] = false;
            continue;
        }
        process[i] = true;

        int psiattack = 0;
        int damage = 0;

        if (!shAttack::isLethal (energy))
            multiplier = divisor = 1;

        /* Compute base damage. */
        if (energy != kPsychic or !attacker or !weapon) {
            damage = multiplier *
                RNG (attack->mDamage[i].mLow, attack->mDamage[i].mHigh);
        } else { /* Attack with weapon dealing psychic damage. */
            damage = multiplier * RNG (1, attacker->mAbil.mPsi);
            if (mImplants[shObjectIlk::kNeck] and attacker and
                attacker->mImplants[shObjectIlk::kNeck])
            {
                torcdiff = this->mImplants[shObjectIlk::kNeck]->mBugginess -
                       attacker->mImplants[shObjectIlk::kNeck]->mBugginess;
                /* Difference can be only -1 or +1. */
                if (torcdiff) torcdiff /= torcdiff > 0 ? torcdiff : -torcdiff;
                bonus += damage * -torcdiff * 3 / 4;
            }
            psiattack = 1;
        }

        /* Whacking with things in melee is influenced by strength applied. */
        if (attacker and attack->isMeleeAttack () and
            (energy == kConcussive or energy == kHackNSlash))
        {
            bonus += ABILITY_BONUS (attacker->mAbil.mStr);
        }

        I->diag ("rolling damage %d*(%d-%d)%+d = %d",
                 multiplier, psiattack ? 1 : attack->mDamage[i].mLow,
                 psiattack ? attacker->mAbil.mPsi : attack->mDamage[i].mHigh,
                 bonus, damage + bonus);

        /* Global damage bonus influences many ways of doing harm but should
           not affect poisons, effect durations and other such things. */
        if (!shAttack::isSpecial (energy)) {
            damage += bonus;
            /* Full bonus is applied only to first eligible energy type. */
            bonus = 0;
        }

        /* This is applied always because it represents partially evading an
           attack and thus getting affected less by all its components. */
        damage /= divisor;

        /* No energy type should deal less than one damage point by itself. */
        if (damage <= 0)  damage = 1;

        /* Possibly absorb some of the damage with a shield. */
        if (hasShield () and
            (attack->isMissileAttack () or attack->isAimedAttack ()) and
            !attack->bypassesShield ())
        {
            if (attack->isAbsorbedForFree (i)) {
                I->diag ("shield absorbed %d for free", damage);
                damage = 0;
            } else {
                absorbed += loseEnergy (damage);
                if (absorbed) {
                    shieldworked = 1;
                }
                damage -= absorbed;
                I->diag ("shield absorbed %d", absorbed);
            }
        }

        /* Reduce damage according to resistances. */
        int resist = getResistance (energy);
        damage = damage - resist;
        if (damage < 0)  damage = 0;
        dmgtable[i] = damage; /* Save for later effect processing. */

        /* If you successfully hurt yourself with your own grenade
           you now know what it does. */
        if (damage and attacker == this and isHero () and
            weapon and weapon->isThrownWeapon () and
            energy != kRadiological) /* Hard to be certain in this case. */
        { /* TODO: Should also identify stack the grenade was part of. */
            weapon->setKnown ();
        }

        /* Count lethal damage. */
        if (damage and shAttack::isLethal (energy)) {
            totaldamage += damage;
            lethal = 1;
        }
    }

    /* Phase 2: Special attack effects. */
    int idx = attack->findEnergyType (kSpecial);
    int spcdmg = idx != -1 ? dmgtable[idx] : 0;
    switch (attack->mType) {
    case shAttack::kAugmentationRay:
    {   /* Gain ability ray. */
        if (!isAlive ())  break;
        bool control = weapon and weapon->isOptimized () and !RNG (3);
        gainAbility (control, 1);
        if (weapon and isHero () and attacker->isHero ())  weapon->setKnown ();
        break;
    }
    case shAttack::kCreditDraining:
    {
        int amount = loseMoney (spcdmg);
        if (amount) {
            I->p ("Your wallet feels lighter.  (-$%d)", amount);
            /* Some is money is permanently lost so the attack
               actually is harmful instead of being 100% joke. */
            attacker->gainMoney (amount * 2 / 3);
            attacker->mMaxHP += amount / 10;
            attacker->mHP += amount / 5;
            if (attacker->mHP > attacker->mMaxHP) {
                attacker->mHP = attacker->mMaxHP;
            }
            attacker->transport (-1, -1, 100, 1);
            attacker->mHidden = 0;
        }
        return 0; /* Attacker is away already. */
    }
    case shAttack::kDecontaminationRay:
        if (isHero ()) {
            int prev = Hero.mRad;
            Hero.mRad -= spcdmg;
            if (Hero.mRad < 0)  Hero.mRad = 0;
            if (Hero.mRad) {
                I->p ("You feel a bit less contaminated.");
            } else if (prev) {
                I->p ("You feel purified.");
            }
            if (weapon and (Hero.mRad or prev)) {
                weapon->setKnown ();
                if (weapon->mOwner == this)  weapon->announce ();
            }
        }
        break;
    case shAttack::kExtractBrain:
    {
        if (!attacker) {
            I->p ("You hear the program bugs craving for your gray matter.");
            break;
        }
        if (!isHero ())  break; /* Not implemented for monsters. */
        /* TODO: Implement for monsters.  Not a big problem. */
        if (mHelmet) {
            shObject *helmet = mHelmet;
            I->p ("%s removes %s!", THE (attacker), YOUR (helmet));
            doff (helmet);
            removeObjectFromInventory (helmet);
            if (!attacker->addObjectToInventory (helmet))
                Level->putObject (helmet, attacker->mX, attacker->mY);
            break;
        }
        int i;
        /* Only implants in frontal, parietal and occipital lobes
           need to be extracted in order to get at your brain. */
        for (i = shObjectIlk::kFrontalLobe;
             i <= shObjectIlk::kOccipitalLobe; ++i)
        {
            if (mImplants[i]) {
                shObject *impl = mImplants[i];
                I->p ("%s extracts your %s!", THE (attacker), YOUR (impl));
                doff (impl);
                removeObjectFromInventory (impl);
                if (!attacker->addObjectToInventory (impl))
                    Level->putObject (impl, attacker->mX, attacker->mY);
                break;
            }
        }
        /* Extraction took place this attack.  This is enough. */
        if (i <= shObjectIlk::kOccipitalLobe)  break;
        if (Hero.getStoryFlag ("brain incision")) {
            I->p ("%s extracts your brain!", THE (attacker));
            mGlyph.mSym = '!';
            mGlyph.mColor = kWhite;
            mGlyph.mTileX = 0;
            mGlyph.mTileY = kRowCanister;
            mMaxAbil.mStr = mAbil.mStr = 0; /* Stats are set this way */
            mMaxAbil.mCon = mAbil.mCon = 0; /* because you have just  */
            mMaxAbil.mDex = mAbil.mDex = 0; /* been separated from    */
            mMaxAbil.mAgi = mAbil.mAgi = 0; /* the rest of your body. */
            return 1;
        } else { /* This is warning an instakill is coming your way. */
            I->p ("%s makes an incision into your skull!", THE (attacker));
            Hero.setStoryFlag ("brain incision", 1);
        }
    }
    case shAttack::kFlare:
        if (isA (kMonSmartMissile))  return 1;  /* Fooled by heat. */
        break;
    case shAttack::kHealingRay:
        if (isAlive ())  healing (spcdmg, 0);
        break;
    case shAttack::kBreatheTraffic:
        inflict (kHosed, FULLTURN * spcdmg);
        break;
    case shAttack::kPlague:
        if (mBodyArmor
            and (mBodyArmor->isA (kObjMBioArmor1)
                 or mBodyArmor->isA (kObjGreenCPA)))
        {
            break; /* These armors grant immunity to plague. */
        }
        /* Some leeway before first effect. */
        inflict (kPlagued, RNG (6, 16) * FULLTURN);
        break;
    case shAttack::kRestorationRay:
        restoration (1);
        break;
    case shAttack::kWaterRay:
        if (isA (kMonManEatingPlant) or
            isA (kMonGreenTomato) or isA (kMonRedTomato))
        {
            if (attacker and attacker->isHero () and attacker->canSee (this)) {
                I->p ("You water %s.  It appears to be grateful.", the ());
            }
            /* Pacify. */
            ((shMonster *) this)->mDisposition = shMonster::kIndifferent;
            if (isA (kMonManEatingPlant) and attacker and attacker->isHero ()
                and Hero.canHearThoughts (this))
            {
                I->p ("You sense %s thinking blood would be more welcome.", the ());
            }
        }
        break;
    default: break;
    }

    /* Phase 3: Messages. */
    if (torcdiff and isHero ()) {
        I->p ("%s %s incoming psionic force.",
            YOUR (mImplants[shObjectIlk::kNeck]),
            torcdiff < 0 ? "amplifies" : "dampens");
    }
    /* Print a mesage about your shield. */
    if (shieldworked) {
        bool fizzle = (0 == countEnergy ());
        if (isHero ()) {
            I->p ("Your force shield absorbs %s damage%s (%d dam)",
                  totaldamage ? "some of the" : "the",
                  fizzle ? " and fizzles out!" : "!", absorbed);
        } else if (Hero.canSee (this)) {
            I->p ("%s's force shield absorbs %s damage%s (%d dam)", thewho,
                  totaldamage ? "some of the" : "the",
                  fizzle ? " and fizzles out!" : "!", absorbed);
        }
        if (fizzle and mBelt and mBelt->isA (kObjShieldBelt))
            mBelt->resetActive ();
    }

    bool showdam = isHero () or Hero.canSee (this) or
        (attacker and attacker->isHero () and attack->isMeleeAttack ());
    showdam = showdam and lethal;
    if (totaldamage >= mHP and !isA (kMonUsenetTroll)) {
        if (attack->isMeleeAttack () and attacker and attacker->isHero ()) {
            if (weapon and weapon->myIlk ()->mMeleeAttack and
                Attacks[weapon->myIlk ()->mMeleeAttack].mType == shAttack::kClub
                and (isA (kMonGreenTomato) or isA (kMonRedTomato)))
            { /* YAFM for clubbed to death killer tomatoes. */
                I->p ("*SPLAT*");
            } else {
                pDeathMessage (the (), kSlain, 1);
            }
        } else if (attack->isMissileAttack () and attacker and
                   attacker->isHero () and attack->mEffect != shAttack::kBurst)
        {
            pDeathMessage (the (), kSlain, 1);
        } else if (attack->isAimedAttack () and attacker and
            attacker->isHero () and weapon and weapon->myIlk ()->mGunAttack and
            Attacks[weapon->myIlk ()->mGunAttack].mEffect == shAttack::kSingle)
        {
            pDeathMessage (isHero () ? "yourself" : the (), kSlain, 1);
        }
        mHP = 0;
    } else {
        /* Attack has only nonlethal energies. */
        bool specialonly = !totaldamage and
            (dmgtable[0] + dmgtable[1] + dmgtable[2]);
        char punct = totaldamage ? (multiplier > 1 ? '!' : '.') : '?';
        /* Hit in melee. */
        if (attack->isMeleeAttack () and attacker and attacker->isHero ()) {
            const char *hitmesg = youHitMonMesg (attack->mType);
            /* It is a little difficult to bite with sealed helmet on. */
            /* TODO: Same for raking with boots on. */
            if (Hero.mHelmet and Hero.mHelmet->isSealedArmor () and
                attack->mType == shAttack::kBite)
            {
                hitmesg = youHitMonMesg (shAttack::kHeadButt);
            }
            /* Hits granting experience are termed abductions. */
            if (Hero.mProfession == Abductor and !is (kNoExp) and
                attack->dealsEnergyType (kViolating) and
                dmgtable[attack->findEnergyType (kViolating)])
            {
                hitmesg = " abduct";
                punct = '!';
            } else if (attack->dealsEnergyType (kViolating) and
                dmgtable[attack->findEnergyType (kViolating)])
            {
                punct = '.';
            }

            I->p ("You%s %s%c", hitmesg, the (), punct);
        }
        /* Hit with thrown weapon. */
        if (attack->isMissileAttack () and attacker and attacker->isHero () and
            !specialonly and (attack->mEffect == shAttack::kSingle or
                              attack->mType == shAttack::kDisc))
        {
            I->p ("You hit %s%c", the (), punct);
        }
        /* Ranged hit from a gun shooting individual bullets. */
        if (attack->isAimedAttack () and attacker and attacker->isHero () and
            lethal and weapon and
            (weapon->myIlk ()->mGunAttack or weapon->myIlk ()->mZapAttack) and
            attack->mEffect == shAttack::kSingle)
        {
            const char *hit = youShootMonMesg (attack->mType);
            I->p ("You %s %s%c", hit, isHero () ? "yourself" : the (), punct);
        }

        if (lethal and attack->isAimedAttack () and isHero () and
            attack->mEffect == shAttack::kSingle)
        {
            I->p ("You are hit%c", punct);
        }
    }
    if (showdam) {
        I->nonlOnce ();
        I->p ("  (%d dam)", totaldamage);
    }
    if (totaldamage >= mHP and isA (kMonUsenetTroll)) {
        if (Hero.canSee (this) or Hero.canHearThoughts (this)) {
            I->p ("%s rage quits!", the ());
        } else {
            I->p ("You hear someone loudly exclaim \"I quit!\"");
        }
        mHP = 0;
        //return 1;
    }

    /* Phase 4: Deal damage. */
    if (mHP == 0)  goto skip_phase_four;

    damageEquipment (attack, kNoEnergy); /* Apply attack type effects. */
    totaldamage = 0; /* Will be counted again more accurately. */
    for (int i = 0; i < 3; ++i) {
        shEnergyType energy = (shEnergyType) attack->mDamage[i].mEnergy;
        if (kNoEnergy == energy)
            break;

        if (!process[i])  continue; /* Damage type not triggered. */

        int damage = dmgtable[i];
        int resist = getResistance (energy);

        /* Possibly damage some equipment.  This occurs even
           if you suffer no harm yourself.  */
        if (shAttack::kOther == attack->mEffect) {
            /* Special attack methods won't affect your equipment */
        } else if (kElectrical == energy or kMagnetic == energy or
                   kBurning == energy    or kBugging == energy or
                   kPlasma == energy     or kCorrosive == energy or
                   kToxic == energy)
        {
            damageEquipment (attack, energy);
        }

        if (!dmgtable[i])  continue; /* Damage completely resisted. */

        /* Apply energy-specific special effects */

        switch (energy) {
        case kToxic:
            if (isHero ()) {
                if (!(shAttack::kPoisonRay == attack->mType or
                    shAttack::kPrick == attack->mType or /* Too obvious. */
                    shAttack::kNoAttack == attack->mType))
                {
                    I->p ("%s was poisonous!  ", attacker->her (attack->noun ()));
                    I->nonlOnce ();
                }
                if (0 == damage) {
                    I->p ("You resist.");
                } else if (resist) {
                    I->p ("You resist partially. (-%d Str)", damage);
                } else {
                    I->p ("You feel weak! (-%d Str)", damage);
                }
            }
            if (sufferAbilityDamage (kStr, damage, 0)) {
                /* Dead, dead, dead! */
                return 1;
            }
            if (isHero ()) {
                if (attacker and attacker->isA (kMonRadspider) and damage and
                    !mMutantPowers[kShootWebs] and !RNG (10))
                {
                    Hero.getMutantPower (kShootWebs);
                }
            }
            /* For balance, monsters take double HP damage in addition to str drain. */
            if (!isHero ()) damage *= 2; else damage = 0;
            totaldamage += damage;
            break;
        case kBlinding:
            if (!isBlind () and damage) {
                if (isHero ()) {
                    I->p ("You are blinded!");
                    I->pauseXY(Hero.mX, Hero.mY);
                }
                makeBlinded (FULLTURN * damage);
            }
            break;
        case kMesmerizing:
            if (!is (kAsleep)) {
                if (isHero ()) {
                    if (damage) {
                        I->p ("You fall asleep!");
                        I->pauseXY(Hero.mX, Hero.mY);
                    } else {
                        I->p ("You yawn.");
                    }
                } else {
                    if (damage) {
                        if (Hero.canSee (this) or Hero.canHearThoughts (this)) {
                            I->p ("%s falls asleep!", THE (this));
                        }
                    } else if (Hero.canSee (this)) {
                        I->p ("%s yawns.", THE (this));
                    }
                }
                if (damage) {
                    inflict (kAsleep, FULLTURN * damage);
                }
            }
            break;
        case kSickening:
            inflict (kSickened, FULLTURN * damage);
            break;
        case kStunning:
            inflict (kStunned, FULLTURN * damage);
            break;
        case kConfusing:
            inflict (kConfused, FULLTURN * damage);
            break;
        case kViolating:
            if (!damage and Hero.canSee (this)) {
                if (isHero ()) {
                    I->p ("You aren't affected.");
                } else {
                    I->p ("%s doesn't seem to notice.", thewho);
                }
            }
            if (damage) {
                inflict (kViolated, FULLTURN * damage);
                if (attacker and attacker->isHero () and
                    attacker->mProfession == Abductor)
                { /* Make an abduction! */
                    mConditions |= kNoExp;
                    Hero.earnXP (mCLevel);
                }
            }
            break;
        case kParalyzing:
            inflict (kParalyzed, FULLTURN * damage);
            break;
        case kDisintegrating:
            if (isHero ()) {
                shObject *obj;
                if (NULL != (obj = mCloak) or
                    NULL != (obj = mBodyArmor) or
                    NULL != (obj = mJumpsuit))
                {
                    I->p ("%s is annihilated!", obj->your ());
                    removeObjectFromInventory (obj);
                    delete obj;
                    continue;
                }
                return 1;
            } else if (damage) {
                return 1;
            } else if (Hero.canSee (this)) {
                I->p ("%s resists!", thewho);
            }
            break;
        case kMagnetic:
            /* An easy way to identify gauss ray gun: fire it at a robot. */
            if (damage and Hero.canSee (this)) {
                if (weapon and weapon->isA (kObjGaussRayGun)) {
                    weapon->setKnown ();
                    weapon->setAppearanceKnown ();
                }
            }
            break;
        case kRadiological:
            if (isHero ()) {
                mRad += damage * 2;
            } else if (damage) { /* treat like poison */
                if (sufferAbilityDamage (kCon, RNG (6), 0)) {
                    /* dead, dead, dead! */
                    return 1;
                } else if (Hero.canSee (this)) {
                    I->p ("%s seems weakened.", thewho);
                    if (weapon and weapon->isA (kObjGammaRayGun)) {
                        weapon->setKnown ();
                        weapon->setAppearanceKnown ();
                    }
                }
                sterilize ();
            }
            break;
        case kWebbing:
            if (attack->mEffect != shAttack::kBurst) {
                if (isHero ())
                    I->p ("You are entangled in a web!");
                else if (Hero.canSee (this))
                    I->p ("%s is entangled in the web!", thewho);
            }
            if (mTrapped.mWebbed)  damage /= 2;
            mTrapped.mWebbed += damage;
            break;
        case kAccelerating:
            inflict (kSpeedy, FULLTURN * damage);
            break;
        case kDecelerating:
            inflict (kSlowed, FULLTURN * damage);
            break;
        case kSpecial:
            /* Do not add up damage. */
            break;
        default: /* Various plainly harmful energy types. */
            totaldamage += damage;
            break;
        }
    }

    /* TODO: system shock check for massive damage */
    skip_phase_four:

    /* Reduce hit points. */
    mHP -= totaldamage;
    if (mHP < 0) {
        mHP = 0;
    }
    I->diag ("dealt %d damage to %s (%p), now has %d / %d HP",
             totaldamage, the (), this, mHP, mMaxHP);

    /* Pain may wake up asleep creatures. */
    if (isHero ()) {
        if (is (kAsleep) and sportingD20 () + totaldamage > 16) {
            cure (kAsleep);
        }
        if (hasHealthMonitoring () and
            mHP < hpWarningThreshold () and
            mHP > 0 and
            oldhp >= hpWarningThreshold ())
        {
            I->p ("You are about to die!");
        }
        I->drawSideWin ();
        I->drawLog ();
    } else {
        if (is (kAsleep) and RNG (1, 20) > 16) {
            cure (kAsleep);
        }
    }

    if (mHP <= 0) { /* Die if HP <= 0. */
        if (isA (kMonBorg) and attacker and attacker->isHero ()) {
            /* The borg adapt to defend against the attacks you use
               against them!  I am so evil!! -- CADV */

            /* Too bad this is not saved. -- MB */
            ++myIlk ()->mNaturalArmorBonus;
#if 0
            for (i = 0; i < 2; i++) {
                shEnergyType energy = (shEnergyType) attack->mDamage[i].mEnergy;
                if (kNoEnergy = energy) {
                    break;
                }
                if (getResistance (energy) < 10) {
                    myIlk ()->mInnateResistances[energy] += 10;
                    break;
                }
            }
#endif
        }

        return 1;
    }
    if (attacker and !isHero ()) { /* Provoke attacked monsters. */
        shMonster *m = (shMonster *) this;
        if (m->mStrategy == shMonster::kLurk) { /* Go hunting! */
            m->mTactic = shMonster::kNewEnemy;
        }
    }
    bool anydamage = (dmgtable[0] + dmgtable[1] + dmgtable[2]);
    if (attacker and attacker->isHero () and !isHero () and anydamage) {
        newEnemy (attacker); /* Get angry! */
    }
    return 0;
}


void
shCreature::pDeathMessage (const char *monname, shCauseOfDeath how,
                           int presenttense /* = 0 */)
{
    if (kAnnihilated == how) {
        I->p ("%s is annihilated!", monname);
        return;
    }
    if (isExplosive () and kSlain == how) {
        if (Hero.canSee (this)) {
            I->p ("%s explodes!", the ());
        } else if (distance (&Hero, mX, mY) <= 100) {
            I->p ("You hear an explosion!");
        } else {
            I->p ("You hear an explosion in the distance.");
        }
        return;
    }

    if (presenttense) {
        if (Hero.is (kXenosHunter) and isXenos ())
            I->p ("You purge %s!", monname);
        if (Hero.is (kXenosHunter) and isHeretic ())
            I->p ("You cleanse %s!", monname);
        else if (isAlive ())
            I->p ("You kill %s!", monname);
        else if (isRobot ())
            I->p ("You disable %s!", monname);
        else if (isProgram ())
            I->p ("You derez %s!", monname);
        else
            I->p ("You destroy %s!", monname);
    } else {
        if (Hero.is (kXenosHunter) and isXenos ())
            I->p ("%s is purged!", monname);
        else if (Hero.is (kXenosHunter) and isHeretic ())
            I->p ("%s is cleansed!", monname);
        else if (isAlive ())
            I->p ("%s is killed!", monname);
        else if (isRobot ())
            I->p ("%s is disabled!", monname);
        else if (isProgram ())
            I->p ("%s is derezzed!", monname);
        else
            I->p ("%s is destroyed!", monname);
    }
}


int
shCreature::die (shCauseOfDeath how, shCreature *killer,
    shObject *implement, shAttack *attack, const char *killstr)
{
    debug.log ("%s (%p) is dead.", the (), this);
    mHowDead = how;
    if (kSuicide != how) {
        ++myIlk ()->mKills;
        if (killer == &Hero and Hero.mProfession == Yautja and
            (isA (kMonAlienWarrior) or isA (kMonAlienQueen)) and
            Hero.canSee (this))
        {
            I->p ("RAAWWWRRRR!");
        }
    }

    /* Assume any death is hero's fault.  It almost always is.  Moreover this
       solution is easy to code, kludgeless and invites to use pets and kill
       monsters using tricky tactics.  Giving experience when that stupid
       tribble falls into a pit and dies on its own is a minor price to pay. */
    if (!(mConditions & kNoExp) and !isHero () and !isPet () and kSuicide != how) {
        /* Multipliers can be too easily farmed for XP.  Arbitrary cap. */
        if (!isMultiplier () or mCLevel + 3 >= Hero.mCLevel) {
            Hero.earnXP (mCLevel);
        }
    }

    if (isPet ()) {
        Hero.mPets.remove (this);
        if (how != kSuicide and !Hero.canSee (this)) {
            I->p ("You have a sad feeling for a moment, and then it passes.");
        }
    }

    if (hasAcidBlood () and kSlain == how) {
        if (mZ == -1) {
            shFeature *f = mLevel->getFeature (mX, mY);
            if (f and f->mType == shFeature::kPit) {
                f->mType = shFeature::kAcidPit;
                if (Hero.canSee (mX, mY)) {
                    I->p ("Acidic blood fills %s pit.",
                        f->mTrapUnknown ? "a" : "the");
                    f->mTrapUnknown = 0;
                }
            }
        } else { /* Alien Queens spray much more blood than everyone else. */
            shAttackId blood = isA (kMonAlienQueen) ? kAttAcidBlood2 : kAttAcidBlood1;
            Level->areaEffect (blood, NULL, mX, mY, kOrigin, this);
        }
    }

    int rbolt = 0;
    int glory_devices = 0;
    for (int i = 0; i < mInventory->count (); ++i) {
        shObject *obj = mInventory->get (i);
        if (obj->isA (kObjGloryDevice) and obj->isWorn ()) {
            ++glory_devices;
            delete obj; /* It explodes. */
            continue;
        }
        obj->resetWorn ();
        obj->resetWielded ();
        if (obj->isUnpaid () and !Level->isInShop (mX, mY)) {
            obj->resetUnpaid ();
        }
        if (!rbolt and obj->isA (kObjRestrainingBolt)) {
            rbolt = 1;
            if (!RNG (3) or obj->sufferDamage (kAttRestrainingBoltWear,
                                               Hero.mX, Hero.mY, 1, 1))
            { /* Buggy bolts go awry surely.  Non-buggy sometimes do too. */
                delete obj;
                continue;
            }
        }
        if (kAnnihilated == how and !obj->isIndestructible ()) {
            delete obj;
        } else {
            Level->putObject (obj, mX, mY);
        }
    }
    int goesboom = isExplosive () and (kSlain == how or kSuicide == how);
    if (kAnnihilated == how) {
        /* Nothing remains. */
    } else if (isA (kMonKillerRabbit)) {
        Level->putObject (createObject ("rabbit's foot", 1), mX, mY);
    } else if (isRobot () and !goesboom) {
        shObject *wreck;
        if (!isA (kMonSmartBomb)) {
            wreck = createWreck (this);
        } else {
            wreck = createObject ("proximity mine", 0);
        }
        Level->putObject (wreck, mX, mY);
    }

    mState = kDead;
    Level->removeCreature (this);

    if (goesboom) {
        shAttackId atk = kAttDummy;
        if (glory_devices) { /* The more the merrier. */
            atk = shAttackId (kAttGlory1 + glory_devices - 1);
        } else if (how == kSuicide and attack) { /* Find explode attack. */
            /* Check melee attacks first. */
            for (int i = 0; i < MAXATTACKS; ++i)
                if (&Attacks[myIlk ()->mAttacks[i].mAttId] == attack) {
                    atk = myIlk ()->mAttacks[i].mAttId;
                    break;
                }
            /* Not found?  Then it must be ranged attack. */
            if (!atk)  for (int i = 0; i < MAXATTACKS; ++i)
                if (&Attacks[myIlk ()->mRangedAttacks[i].mAttId] == attack) {
                    atk = myIlk ()->mAttacks[i].mAttId;
                    break;
                }
            /* Still not here?  That's a bug, use default. */
            if (!atk) {
                atk = kAttExplodingMonster;
                debug.log ("Error: attack not found for suiciding %s.",
                    getDescription ());
            }
        } else {
            atk = kAttExplodingMonster;
        }
        Level->areaEffect (atk, NULL, mX, mY, kOrigin, this, mCLevel);
    }
    if (mLevel->rememberedCreature (mX, mY)) {
        mLevel->forgetCreature (mX, mY);
    }
    return 1;
}

/* Returns elapsed time in miliseconds.  Usually is instaneous. */
int
shCreature::revealSelf ()
{
    const char *a_thing;
    int mimic = 0;
    bool iswatery = mLevel->isWatery (mX, mY);
    shFeature *f = mLevel->getFeature (mX, mY);
    shObjectVector *v = mLevel->getObjects (mX, mY);

    bool known = mHidden < 0;
    mHidden = 0;
    if (canBurrow () and !iswatery) { /* Unburrowing takes time. */
        if (Hero.canSee (this)) {
            I->p ("%s unburrows%c", known ? the () : an (), known ? '.' : '!');
            Hero.interrupt (); /* Needed because the creature most likely
                                  will not attack immediately. */
        }
        return LONGTURN;
    }
    if (known)
        return 0; /* No need to print a message since we knew it was there. */

    if (!Hero.canSee (this))  return 0; /* Not seen - no message. */

    const char *a_monster = an ();

    if (mMimic == kObject) {
        mimic = 1;
        mMimic = kNothing;
        a_thing = mMimickedObject->mVague.mName;
    } else {
        mimic = 0;
        int found = 0;
        if (v and canHideUnderObjects ()) {
            for (int i = 0; i < v->count (); ++i) {
                shObject *obj = v->get (i);
                if (canHideUnder (obj)) {
                    a_thing = obj->an ();
                    found = 1;
                    break;
                }
            }
        }
        if (found) {
            /* Skip. */
        } else if (f and canHideUnder (f)) {
            a_thing = f->the ();
        } else if (iswatery and canHideUnderWater ()) {
            a_thing = mLevel->getSquare (mX, mY) ->the ();
        } else { /* Impossible! */
            a_thing = "shroud of bugginess";
        }
    }

    if (mimic) {
        I->p ("The %s morphs into %s!", a_thing, a_monster);
    } else {
        I->p ("%s was hiding under %s!", a_monster, a_thing);
    }
    return 0;
}


/* check to see if the creature is getting irradiated
   returns: number of rads taken
   modifies: nothing
*/
int
shCreature::checkRadiation ()
{
    int i;
    int res = 0;
    shObjectVector *v = Level->getObjects (mX, mY);
    if (v) {
        for (i = 0; i < v->count (); i++) {
            shObject *obj = v->get (i);
            if (obj->isRadioactive ()) {
                res += obj->mCount;
            }
        }
    }
    for (i = 0; i < mInventory->count (); i++) {
        shObject *obj = mInventory->get (i);
        if (obj->isRadioactive ()) {
            res += 2 * obj->mCount;
        }
    }
    res += Level->isRadioactive (mX, mY);

    return res;
}


int
shCreature::isInPit (void)
{
    if (mZ == -1 and mTrapped.mInPit)
        return canJump () ? -1 : mTrapped.mInPit;
    return 0;
}


int
shCreature::isTrapped ()
{
    return mTrapped.mInPit + mTrapped.mDrowning +
           mTrapped.mTaped + mTrapped.mWebbed;
}


int
shCreature::owns (shObject *obj)
{
    return mInventory->find (obj) >= 0;
}


int
shCreature::tryToFreeSelf ()
{
    if (isSessile ())  return FULLTURN;
    const char *s = !isHero () ? "s" : "";
    shFeature *f = mLevel->getFeature (mX, mY);
    bool seen = Hero.canSee (this);

    if (f and isInPit ()) {
        f->mTrapMonUnknown = 0;
        if (seen)  f->mTrapUnknown = 0;
    }

    /* Problem to try take care about now.
       0 - none, 1 - webbing, 2 - taping, 3 - being in a pit */
    int problem = 0;

    /* One cannot get out of pit if webbed or taped. */
    if (mTrapped.mWebbed and mTrapped.mTaped) {
        problem = RNG (1, 2);
    } else if (mTrapped.mWebbed) {
        problem = 1;
    } else if (mTrapped.mTaped) {
        problem = 2;
    } else if (mTrapped.mInPit) {
        problem = 3;
    }

    switch (problem) {
    default:
    case 0:
        return 0;
    case 1:
        --mTrapped.mWebbed;
        if (!mTrapped.mWebbed and seen) {
            if (f and f->mType == shFeature::kWeb) {
                I->p ("%s free%s %s from the web.",
                      the (), s, herself ());
            } else {
                I->p ("%s break%s the webs holding %s!",
                      the (), s, herself ());
            }
        }
        return FULLTURN;
    case 2:
        --mTrapped.mTaped;
        if (!mTrapped.mTaped and seen) {
            I->p ("%s rip%s duct tape holding %s to pieces!",
                  the (), s, herself ());
        }
        return FULLTURN;
    case 3:
        --mTrapped.mInPit;
        if (!mTrapped.mInPit)  mTrapped.mDrowning = 0;
        if (canJump ()) {
            mTrapped.mInPit = 0;
            mTrapped.mDrowning = 0;
            if (Hero.canSee (this))
                I->p ("%s jump%s out of %s.", the (), s,
                      f ? f->getShortDescription () : "the bugginess zone");
            return FULLTURN;
        } else if (!mTrapped.mInPit and Hero.canSee (this)) {
            if (!f) {
                I->p ("%s free%s %s from the bugginess zone.");
                return FULLTURN;
            }
            switch (f->mType) {
            case shFeature::kPit:
            case shFeature::kAcidPit:
                I->p ("%s climb%s out of %s.", the (), s,
                      f->getShortDescription ());
                mTrapped.mInPit = 0;
                break;
            case shFeature::kSewagePit:
                I->p ("%s swim%s to shallow sewage.", the (), s);
                mTrapped.mInPit = 0;
                mTrapped.mDrowning = 0;
                break;
            default:
                I->p ("%s free%s %s from %s.",
                      the (), s, herself (), f->getShortDescription ());
                break;
            }
            return FULLTURN;
        }
    }

    /* It is error if this point is reached so make it long to be noticeable. */
    return LONGTURN * 5;
}

int
shCreature::addObjectToInventory (shObject *obj, int quiet)
{
    mInventory->add (obj);

    obj->mLocation = shObject::kInventory;
    obj->mOwner = this;
    mWeight += obj->getMass ();
    computeIntrinsics ();
    return 1;
}


void
shCreature::removeObjectFromInventory (shObject *obj)
{
    mInventory->remove (obj);
    mWeight -= obj->getMass ();
    if (obj->isWorn ()) {
        doff (obj);
    }
    obj->resetWorn ();
    obj->resetWielded ();
    obj->resetActive ();
    if (mWeapon == obj) { mWeapon = NULL; }
    computeIntrinsics ();
    computeSkills ();
    computeAC ();
}


shObject *
shCreature::removeOneObjectFromInventory (shObject *obj)
{
    if (1 == obj->mCount) {
        removeObjectFromInventory (obj);
        return obj;
    } else {
        mWeight -= obj->myIlk ()->mWeight;
        return obj->split (1);
    }
}


void
shCreature::useUpOneObjectFromInventory (shObject *obj)
{
    if (isHero () and obj->isUnpaid ()) {
        Hero.usedUpItem (obj, 1, "use");
    }
    if (1 == obj->mCount) {
        removeObjectFromInventory (obj);
        delete obj;
    } else {
        --obj->mCount;
        mWeight -= obj->myIlk ()->mWeight;
    }
}


//RETURNS: number of objects used
int
shCreature::useUpSomeObjectsFromInventory (shObject *obj, int cnt)
{
    if (obj->mCount > cnt) {
        obj->mCount -= cnt;
        mWeight -= cnt * obj->myIlk ()->mWeight;
        if (isHero () and obj->isUnpaid ()) {
            Hero.usedUpItem (obj, cnt, "use");
        }
        return cnt;
    } else {
        cnt = obj->mCount;
        if (isHero () and obj->isUnpaid ()) {
            Hero.usedUpItem (obj, cnt, "use");
        }
        removeObjectFromInventory (obj);
        delete obj;
        return cnt;
    }
}


shObject *
shCreature::removeSomeObjectsFromInventory (shObject *obj, int cnt)
{
    if (obj->mCount > cnt) {
        obj = obj->split (cnt);
        mWeight -= cnt * obj->myIlk ()->mWeight;
        return obj;
    } else {
        removeObjectFromInventory (obj);
        return obj;
    }
}


int
shCreature::countEnergy (int *tankamount)
{
    shObjectVector v;
    int res = 0;

    selectObjects (&v, mInventory, kObjEnergyCell);
    for (int i = 0; i < v.count (); i++) {
        res += v.get (i) -> mCount;
    }

    v.reset ();
    selectObjects (&v, mInventory, kObjGenericPlant);
    selectObjects (&v, mInventory, kObjGenericEnergyTank);
    if (mBelt and mBelt->isA (kObjEnergyBelt)) v.add (mBelt);
    if (tankamount) *tankamount = 0;
    for (int i = 0; i < v.count (); i++) {
        shObject *obj = v.get (i);
        res += obj->mCharges;
        if (tankamount) {
            *tankamount += obj->myIlk ()->mMaxCharges;
        }
    }

    return res;
}

/* expends some energy from the following sources, in order of preference:
   1. power plants
   2. energy cells
   3. energy tanks
   4. unpaid power plants
   5. unpaid cells
   6. unpaid energy tanks
   returns: # of energy units spent
*/

int
shCreature::loseEnergy (int cnt)
{
    shObjectVector v;
    int res = 0;
    int i;
    int pass;

    for (pass = 0; pass < 2; pass++) {
        selectObjects (&v, mInventory, kObjGenericPlant);
        for (i = 0; i < v.count (); i++) {
            shObject *obj = v.get (i);
            int unpaid = isHero () and obj->isUnpaid ();
            if (unpaid) {
                if (0 == pass) {
                    continue;
                } else if (obj->mCharges) {
                    /* easy way to deal with this is to make 'em pay for the
                       whole damn tank */ //FIXME
                    Hero.usedUpItem (obj, 1, "use");
                    obj->resetUnpaid ();
                }
            }

            if (obj->mCharges > cnt) {
                obj->mCharges -= cnt;
                res += cnt;
                return res;
            } else {
                res += obj->mCharges;
                obj->mCharges = 0;
            }
            cnt -= res;
        }
        v.reset ();

        selectObjects (&v, mInventory, kObjEnergyCell);
        for (i = 0; i < v.count (); i++) {
            shObject *obj = v.get (i);
            int unpaid = isHero () and obj->isUnpaid ();
            if (0 == pass and unpaid) {
                continue;
            }
            int tmp = useUpSomeObjectsFromInventory (obj, cnt);
            cnt -= tmp;
            res += tmp;
            if (0 == cnt) {
                return res;
            }
        }
        v.reset ();

        if (mBelt and mBelt->isA (kObjEnergyBelt)) v.add (mBelt);
        selectObjects (&v, mInventory, kObjGenericEnergyTank);
        for (i = 0; i < v.count (); i++) {
            shObject *obj = v.get (i);
            int unpaid = isHero () and obj->isUnpaid ();
            if (0 == pass and unpaid) {
                continue;
            } else if (obj->mCharges) {
                /* easy way to deal with this is to make 'em pay for the
                   whole damn tank */
                Hero.usedUpItem (obj, 1, "use");
                obj->resetUnpaid ();
            }
            if (obj->mCharges > cnt) {
                obj->mCharges -= cnt;
                res += cnt;
                return res;
            } else {
                res += obj->mCharges;
                obj->mCharges = 0;
            }
            cnt -= res;
        }
        v.reset ();
    }
    return res;
}


void
shCreature::gainEnergy (int amount)
{
    shObjectVector v;
    int i;

    selectObjects (&v, mInventory, kObjGenericPlant);
    for (i = 0; i < v.count (); i++) {
        shObject *obj = v.get (i);

        if (obj->mCharges < 100) {
            obj->mCharges += amount;
            if (obj->mCharges > 100) {
                amount -= (obj->mCharges - 100);
                obj->mCharges = 100;
            } else {
                return;
            }
        }
    }
}


int
shCreature::countMoney ()
{
    shObjectVector v;
    int res = 0;
    int i;

    selectObjects (&v, mInventory, kMoney);
    for (i = 0; i < v.count (); i++) {
        res += v.get (i) -> mCount;
    }
    return res;
}


int
shCreature::loseMoney (int amount)
{
    shObjectVector v;
    int res = 0;
    int i;

    selectObjects (&v, mInventory, kMoney);
    for (i = 0; i < v.count (); i++) {
        shObject *obj = v.get (i);
        int tmp = useUpSomeObjectsFromInventory (obj, amount);
        amount -= tmp;
        res += tmp;
    }
    return res;
}


int
shCreature::gainMoney (int amount)
{
    shObject *obj = new shObject (kObjMoney);
    obj->mCount = amount;
    addObjectToInventory (obj, 1);
    return amount;
}


/* Returns 0 if failed to unwield buggy weapon. */
int
shCreature::wield (shObject *obj, int quiet /* = 0 */ )
{
    if (mWeapon and 0 == unwield (mWeapon)) {
        return 0;
    }
    mWeapon = obj;
    if (0 == quiet and !mHidden and Hero.canSee (this) and mWeapon) {
        mWeapon->setAppearanceKnown ();
        if (isRobot () and mWeapon->isThrownWeapon ()) {
            I->p ("%s loads %s!", the (), mWeapon->an ());
        } else {
            if (mWeapon->isA (kObjLightSaber))
                mWeapon->setKnown ();
            I->p ("%s wields %s!", the (), mWeapon->an ());
        }
        Hero.interrupt ();
    }
    computeIntrinsics ();
    computeSkills ();
    return 1;
}


int
shCreature::unwield (shObject *obj, int quiet /* = 0 */ )
{
    assert (mWeapon == obj);
    mWeapon = NULL;
    obj->resetWielded ();
    computeIntrinsics ();
    computeSkills ();
    return 1;
}


int
shCreature::don (shObject *obj, int quiet /* = 0 */ )
{
    int before = mIntrinsics;
    unsigned int encumbrance = mConditions & kOverloaded;

    if (obj->isA (kObjGenericArmor) and NULL == mBodyArmor) {
        mBodyArmor = obj;
    } else if (obj->isA (kObjGenericJumpsuit) and NULL == mJumpsuit) {
        mJumpsuit = obj;
    } else if (obj->isA (kObjGenericHelmet) and NULL == mHelmet) {
        shObject *imp = mImplants[shObjectIlk::kCerebellum];
        if (imp and imp->isA (kObjMechaDendrites) and obj->isSealedArmor ()) {
            if (!quiet and isHero ()) {
                I->p ("%s conflict with %s.", YOUR (imp), obj->theQuick ());
            }
            return 0;
        }
        if (mGoggles and obj->isA (kObjBioMask)) {
            if (!quiet and isHero ()) {
                I->p ("%s conflict with %s.", YOUR (mGoggles), obj->theQuick ());
            }
            return 0;
        }
        mHelmet = obj;
    } else if (obj->isA (kObjGenericGoggles) and NULL == mGoggles) {
        if (mHelmet and mHelmet->isA (kObjBioMask)) {
            if (!quiet and isHero ()) {
                I->p ("%s conflicts with %s.", YOUR (mHelmet), obj->theQuick ());
            }
            return 0;
        }
        mGoggles = obj;
    } else if ((obj->isA (kObjGenericCloak) or obj->isA (kObjPlasmaCaster)) and
               NULL == mCloak)
    {
        mCloak = obj;
    } else if (obj->isA (kObjGenericBoots) and NULL == mBoots) {
        mBoots = obj;
    }  else if (obj->isA (kObjGenericBelt) and NULL == mBelt) {
        mBelt = obj;
    } else if (obj->isA (kImplant)) {
        shObjectIlk::Site site = obj->myIlk ()->mSite;
        if (shObjectIlk::kAnyBrain == site) {
            int i;
            for (i = 0; i <= shObjectIlk::kCerebellum; ++i) {
                if (!mImplants[i]) {
                    site = (shObjectIlk::Site) i;
                    break;;
                }
            }
            if (i > shObjectIlk::kCerebellum) {
                if (isHero () and !quiet) {
                    I->p ("There's no more room in your brain for this implant!");
                }
                return 0;
            }
        } else if (shObjectIlk::kAnyEar == site) {
            site = shObjectIlk::kLeftEar;
        } else if (shObjectIlk::kAnyEye == site) {
            site = shObjectIlk::kRightEyeball;
        }

        if (mImplants[site]) {
            if (isHero () and !quiet) {
                if (site == shObjectIlk::kNeck) {
                    I->p ("%s's field repels the new %s!",
                        YOUR(mImplants[site]), obj->inv ());
                } else {
                    I->p ("You already have an implant installed in your %s.",
                          describeImplantSite (site));
                }
            }
            return 0;
        }
        if (obj->isA (kObjMechaDendrites) and
            mHelmet and mHelmet->isSealedArmor ())
        {
            return 0; /* Quietly because this condition can only be triggered */
        }             /* by automatic donning at the start of new game. */
        mImplants[site] = obj;
        if (isHero ()) {
            if (!quiet) {
                I->p ("You install %s in your %s.", THE (obj),
                    describeImplantSite (site));
            }
            obj->mImplantSite = site;
        }
    } else {
        return 0;
    }
    obj->setWorn ();
    computeIntrinsics (quiet);
    computeSkills ();
    computeAC ();
    if (isHero ()) {
        if (!(mIntrinsics & kCamouflaged) and (before & kCamouflaged)) {
            if (!quiet) {
                I->p ("Covering %s deprives you of camouflage.",
                    obj != mJumpsuit and mJumpsuit ? YOUR (mJumpsuit) : "your body");
            }
        }
        if (obj->isA (kArmor) and obj->isEnhanceable ()) {
                obj->setEnhancementKnown ();
        }
        if (obj->isA (kObjHealthMonitor) and !(before & kHealthMonitoring)) {
            obj->setKnown ();
        } else if (obj->isA (kObjTissueRegenerator)) {
            mLastRegen = Clock;
        } else if (obj->isA (kObjMechaDendrites)) {
            obj->setBugginessKnown ();
        } else if (obj->isA (kObjGenericSkillsoft) and !obj->isBuggy ()) {
            if (!quiet) {
                I->p ("You gain access to technical knowledge database.");
            }
            obj->setKnown ();
            obj->setBugginessKnown ();
        } else if (obj->isA (kObjChameleonSuit)) {
            if ((mIntrinsics & kCamouflaged) and !(before & kCamouflaged)) {
                if (!mBodyArmor and !mCloak) {
                    if (!quiet)  I->p ("You blend in with your surroundings.");
                    obj->setKnown ();
                } else if (obj->isKnown () and !quiet) {
                    /* FIXME: Above condition is not enough.
                              This is never printed. */
                    I->p ("You would blend in with your surroundings if you hadn't covered your suit.");
                }
            }
        } else if (obj->isA (kObjStabilizerBelt)) {
            if (encumbrance != (mConditions & kOverloaded)) {
                obj->setKnown ();
            }
            /* +2 to heavy guns is noticeable.  If it gives +1 it could be
               either buggy or debugged so bugginess is not revealed yet. */
            if (obj->isKnown () and obj->isOptimized ())
                obj->setBugginessKnown ();
        } else if (obj->isA (kObjEnergyBelt)) {
            /* No message. Energy display indicates what item is this. */
            obj->setKnown ();
            obj->setChargeKnown ();
        } else if (obj->isA (kObjShieldBelt) and !(before & kShielded)) {
            if (!quiet) {
                if (!isBlind ()) {
                    I->p ("You are surrounded by a force field.");
                } else {
                    I->p ("There is a feeling of stasis around you.");
                }
            }
            obj->setKnown ();
            obj->setActive ();
        /*
        } else if (obj->isA ("jetpack") and !(before & kFlying)) {
            if (!quiet) {
                I->p ("You zoom into the air!");
            }
            obj->setKnown ();
            mTrapped.mInPit = 0;
            mTrapped.mDrowning = 0;
        */
        } else if (obj->isA (kObjXRayGoggles) and !isBlind () and
                   !(before & kXRayVision))
        {
            if (!quiet) {
                I->p ("You can see through things!");
            }
            obj->setKnown ();
        } else if (obj->isA (kObjCerebralCoprocessor) or
                   obj->isA (kObjMotoricCoordinator) or
                   obj->isA (kObjReflexCoordinator) or
                   obj->isA (kObjAdrenalineGenerator) or
                  (obj->isA (kObjPsiAmp) and mProfession == Psion))
        {
            if (obj->mEnhancement) { /* Stat mod gives it away. */
                obj->setKnown ();
                obj->setEnhancementKnown ();
            } else if (obj->isKnown ()) { /* Identify +0 if you know type. */
                obj->setEnhancementKnown ();
            }
            /*  Psionic amplifier:
               Usually identifying a +0 would require one to check percentage
               chances of mutant powers.  Psions are considered so expert at
               noticing psionic aura disturbances they can know psionic
               amplifiers just by having them interact with their brain. */
        } else if (obj->isA (kObjStormtrooperHelmet) and !isBlind ()) {
            if (!quiet) {
                I->p ("It's hard to see out of this helmet.");
            }
            obj->setKnown ();
        } else if (obj->isA (kObjRedCPA)) {
            if (!quiet) {
                I->p ("A voice roars in your head:");
                I->p ("Blood for the blood god, %s!", Hero.getTitle ());
            }
            obj->identify ();
            obj->setBuggy ();
        } else if (obj->isA (kObjPinkCPA)) {
            if (!quiet) {
                I->p ("A voice whispers in your ear:");
                I->p ("Pleasure me, %s!", Hero.getTitle ());
            }
            obj->identify ();
            obj->setBuggy ();
        } else if (obj->isA (kObjBlueCPA)) {
            if (!quiet) {
                I->p ("You hear a thousand silent voices.");
            }
            obj->identify ();
            obj->setBuggy ();
        } else if (obj->isA (kObjGreenCPA)) {
            if (!quiet) {
                I->p ("%s gurgles and speaks:", obj->the());
                I->p ("Come bask in my filth, lowly %s.", Hero.getTitle ());
            }
            obj->identify ();
            obj->setBuggy ();
        } else if (obj->isA (kObjBioArmor)) {
            if (!quiet) {
                I->p ("%s quivers and envelops your body!.", obj->the());
            }
            obj->setKnown ();
        } else if (obj->isA (kObjTorc) and !obj->isBuggy ()) {
            Hero.torcCheck ();
        }
        obj->resetWorn ();
        if (!quiet)  obj->announce ();
        obj->setWorn ();
    }
    return 1;
}


int
shCreature::doff (shObject *obj, int quiet /* = 0 */ )
{
    int before = mIntrinsics;
    unsigned int encumbrance = mConditions & kOverloaded;

    if (obj == mBodyArmor) {
        mBodyArmor = NULL;
    } else if (obj == mJumpsuit) {
        mJumpsuit = NULL;
    } else if (obj == mHelmet) {
        mHelmet = NULL;
    } else if (obj == mGoggles) {
        mGoggles = NULL;
	} else if (obj == mCloak) {
		mCloak = NULL;
    } else if (obj == mBelt) {
        mBelt = NULL;
	} else if (obj == mBoots) {
		mBoots = NULL;
    } else {
        int i;
        for (i = 0; i < shObjectIlk::kMaxSite; i++) {
            if (obj == mImplants[i]) {
                mImplants[i] = NULL;
                break;
            }
        }
        if (i == shObjectIlk::kMaxSite) return 0;
    }

    obj->resetWorn ();
    computeIntrinsics (quiet);
    computeSkills ();
    computeAC ();
    if (isHero ()) {
        if (obj->isA (kObjTissueRegenerator)) {
            mLastRegen = MAXTIME;
        } else if (obj->isA (kObjChameleonSuit)) {
            if (!quiet and (kCamouflaged and mIntrinsics))
            I->p ("You no longer blend in with your surroundings.");
        /*
        } else if (obj->isA ("jetpack")) {
            if (!isFlying ()) {
                obj->setKnown ();
                I->p ("You float down.");
            }
        */
        } else if (obj->isA (kObjStabilizerBelt)) {
            if (encumbrance != (mConditions & kOverloaded)) {
                obj->setKnown ();
            }
            /* Bugginess revealed always because having the ability to remove
               the belt means it has to be debugged if it grants +1 to heavy
               guns.  Unless you are an orc in which case only optimized belts
               status can be deduced by taking it off. */
            if (obj->isKnown () and
                (!isOrc () or obj->isOptimized ()))  obj->setBugginessKnown ();
        } else if (obj->isA (kObjStormtrooperHelmet) and !isBlind ()) {
            I->p ("You can see much better now.");
        } else if (obj->isA (kObjTorc) and !obj->isBuggy ()) {
            I->p ("The torc's rare earth circuits are singed as you remove it from your neck!");
            if (!isBlind ())  I->p ("Its color deadens.");
            --obj->mBugginess;
            if (sufferDamage (kAttTorcRemoval)) {
                die (kKilled, "removing a torc"); /* TODO: What color torc? */
            }
        }
        if ((mIntrinsics & kCamouflaged) and !(before & kCamouflaged)) {
            if (!quiet) {
                I->p ("By uncovering %s you %sgain camouflage.",
                    mJumpsuit ? YOUR (mJumpsuit) : "your body",
                    mJumpsuit and !mJumpsuit->isKnown () ? "" : "re");
                if (mJumpsuit and !mJumpsuit->isKnown ()) {
                    mJumpsuit->setKnown ();
                    mJumpsuit->announce ();
                }
            }
        }
    }
    return 1;
}

/* If a creature gets hit with certain energy type what items get affected? */
void
shCreature::damageEquipment (shAttack *attack, shEnergyType energy)
{
    shObjectVector v;
    shObject *obj = NULL;

    if (kCorrosive == energy) {
        if (mWeapon)  v.add (mWeapon);
        if (mCloak) {
            v.add (mCloak);
        } else if (mBodyArmor) {
            v.add (mBodyArmor);
        } else if (mJumpsuit) {
            v.add (mJumpsuit);
        }
        if (mBoots)  v.add (mBoots);
        if (mBelt)  v.add (mBelt);
		if (mCloak)  v.add (mCloak);
        if (mHelmet)  v.add (mHelmet);
        if (mGoggles)  v.add (mGoggles);
    } else if (kElectrical == energy) {
        if (RNG (2))
            /* Only unprotected energy cells are subject to shortening out. */
            for (int i = 0; i < mInventory->count (); ++i) {
                shObject *obj = mInventory->get (i);
                if (obj and obj->isA (kObjEnergyCell)) {
                    v.add (obj);
                    /* Be content with finding a single stack of cells because
                       if all of them were picked up for attack players might
                       be tempted to split their cells to protect implants.
                       This would be boring. */
                    break;
                }
            }
        /* Helmet granting resistance to electricity protects implants. */
        if (!mHelmet or mHelmet->myIlk ()->mResistances[kElectrical] <= 0) {
            for (int i = 0; i < shObjectIlk::kMaxSite; i++)
                if (mImplants[i])  v.add (mImplants[i]);
        }
    } else if (kMagnetic == energy) { /* Get everything. */
        for (int i = 0; i < mInventory->count (); ++i)
            v.add (mInventory->get (i));
    } else if (kBurning == energy or kPlasma == energy) {
        if (mCloak) {
            v.add (mCloak);
        } else if (mBodyArmor) {
            v.add (mBodyArmor);
        } else if (mJumpsuit) {
            v.add (mJumpsuit);
        }
        if (mHelmet) v.add (mHelmet);
        if (mBelt) v.add (mBelt);
        if (mBoots) v.add (mBoots);
		if (mCloak) v.add (mCloak);
        if (mGoggles) v.add (mGoggles);
        if (!RNG (3))  selectObjects (&v, mInventory, kFloppyDisk);
    } else if (kFreezing == energy) {
        if (!RNG (3))  selectObjects (&v, mInventory, kCanister);
    } else if (kBugging == energy) {
        selectObjectsByFunction (&v, mInventory, &shObject::isBuggy, 1);
    } else if (kToxic == energy) {
      /* For reticulan bio armor and green chaos power armor. */
        if (mBodyArmor)  v.add (mBodyArmor);
    }

    /* Bio armors respond to restoration, healing and plague. */
    if (attack->mType == shAttack::kRestorationRay or
        attack->mType == shAttack::kHealingRay or
        attack->mType == shAttack::kPlague)
    {
        if (mBodyArmor)  v.add (mBodyArmor);
    }

    /* Choose one victim randomly. */
    if (v.count ()) {
        obj = v.get (RNG (v.count ()));
        int num;
        if ((num = obj->sufferDamage (energy, attack, NULL, mX, mY))) {
            if (isHero())  Hero.usedUpItem (obj, num, "destroy");
            delete removeOneObjectFromInventory (obj);
        }
    }
    computeIntrinsics ();
    computeAC ();
}


int
shCreature::canSmell (shCreature *c)
{
    if (!hasScent ()) return 0;
    if (distance (this, c) <= 25) return 1;
    return 0;
}


shObject *
shCreature::getKeyForDoor (shFeature *f)
{
    shObjectIlk *keyneeded = f->keyNeededForDoor ();
    shObjectVector mykeys;

    /* Try your keycards first. */
    for (int i = 0; i < mInventory->count (); ++i) {
        shObject *obj = mInventory->get (i);
        if (obj->isA (kObjGenericKeycard) and /* A keycard. */
            /* It is in working condition or you are not sure. */
            (!obj->isBuggy () or !obj->isBugginessKnown ()) and
           ( /* Unlocks all doors. */
            (obj->isA (kObjMasterKeycard) and obj->isKnown ()) or
             /* Unlawfully unlocks all doors. */
            (obj->isCracked () and obj->isCrackedKnown ()) or
             /* Is right key */
            (obj->isA (keyneeded->mId) and
            /* You see door lock color and keycard color or... */
             (!isBlind () or
            /* ...you know both lock color and this keycard color. */
              (obj->isAppearanceKnown () and
               (Level->mRemembered[f->mX][f->mY].mDoor & shFeature::kColorCode))
             ))
           ))
            mykeys.add (obj);
    }

    if (mykeys.count ()) { /* Pick random fitting key for fun. */
        int choose = RNG (mykeys.count ());
        return mykeys.get (choose);
    }

    mykeys.reset (); /* Time to try breaching security. */
    selectObjects (&mykeys, mInventory, kObjLockPick);

    if (mykeys.count ()) {
        int choose = RNG (mykeys.count ());
        return mykeys.get (choose);
    }
    return NULL;
}


//RETURNS: 1 if successful, 0 o/w
int
shCreature::openDoor (int x, int y)
{
    shFeature *f = mLevel->getFeature (x,y);
    if (shFeature::kDoorClosed != f->mType or
        !isAdjacent (x, y))
    {
        return 0;
    }
    if (0 == numHands () and !isA (kMonDalek)) {
        return 0;
    }
    if (shFeature::kLocked & f->mDoor.mFlags) {
        if (isHero ()) {
            if (f->isLockBrokenDoor ()) {
                I->p ("The door is held shut by a broken lock!");
            } else if (f->isRetinaDoor ()) {
                I->p ("\"Initiating retina scan...\"");
                I->pause ();
                if (mImplants[shObjectIlk::kRightEyeball] and
                    mImplants[shObjectIlk::kRightEyeball]
                    ->isA (kObjBOFHEye))
                {
                    I->p ("\"Identification positive.  "
                          "Welcome, Mr. Operator!\"");
                    mImplants[shObjectIlk::kRightEyeball]->setKnown ();
                    f->unlockDoor ();
                    f->mType = shFeature::kDoorOpen;
                    f->mDoor.mMoveTime = Clock;
                    return 1;
                } else {
                    I->p ("\"Identification negative.  Access denied.\"");
                }
            } else {
                /* Safe, will not return NULL because lock broken
                   case is handled above. */
                shObjectIlk *keyneeded = f->keyNeededForDoor ();
                shObject *key = getKeyForDoor (f);
                if (key) {
                    return Hero.useKey (key, f);
                }
                I->p ("The door is locked.  There is %s lock.",
                    isBlind () ? "some kind of keycard"
                               : keyneeded->mAppearance.mName);
                I->p ("You do not seem to have anything to unlock it with.");
                I->p ("Maybe try kicking it (with '%s' key) or shooting it?",
                      I->getKeyForCommand (shInterface::kKick));
            }
        } else if (isA (kMonDalek)) {
            /* Daleks can lock pick doors with intact locks but not retina
               scanners.  Daleks are said to perform a million of operations
               per second according to some Doctor Who episode. */
            if (!f->isRetinaDoor () and !f->isLockBrokenDoor ())
            { /* Unlock! */
                f->unlockDoor ();
                if (Hero.canSee (x, y)) {
                    I->p ("The lock on the door emits a cascade of lights and unlocks!");
                    I->pauseXY (x, y);
                }
                return 1;
            }
        }
        return 0;
    }
    f->mType = shFeature::kDoorOpen;
    f->mDoor.mMoveTime = Clock;
    if (mLevel == Level)
        Level->computeVisibility ();
    return 1;
}


//RETURNS: 1 if successful, 0 o/w
int
shCreature::closeDoor (int x, int y)
{
    shFeature *f = mLevel->getFeature (x,y);
    if ((shFeature::kDoorOpen != f->mType and !isHero ()) or
        !isAdjacent (x, y) or
        0 != mLevel->countObjects (x, y))
    {
        return 0;
    }
    if (0 == numHands ()) {
        return 0;
    }
    if (shFeature::kDoorOpen == f->mType) { /* Close a door. */
        f->mType = shFeature::kDoorClosed;
        f->mDoor.mMoveTime = Clock;
        if (f->isRetinaDoor ())
            f->lockDoor ();
        if (mLevel == Level)
            Level->computeVisibility ();
    } else { /* Hero wishes to lock a closed door. */
        if (!f->isLockDoor ()) {
            I->p ("It's already closed.");
            return 0;
        }
        if (f->isLockBrokenDoor ()) {
            I->p ("Its lock is broken.  You need to fix it before you can lock it.");
            return 0;
        }
        if (f->isLockedDoor ()) {
            I->p ("It's already closed and locked.");
            return 0;
        }
        if (f->isRetinaDoor ()) {
            I->p ("You reactivate the door's retina scanner.");
            f->lockDoor ();
            return 1;
        }
        shObject *key = getKeyForDoor (f);
        if (key) {
            return Hero.useKey (key, f);
        }

        I->p ("You do not seem to have any means to lock this door.");
        shObjectIlk *keyneeded = f->keyNeededForDoor ();
        I->p ("It has %s lock.", isBlind () ? "some kind of keycard"
                                            : keyneeded->mAppearance.mName);
    }
    return 1;
}


/* returns true if the creature's movement's would trigger a motion detector */
int
shCreature::isMoving ()
{
    if (mHidden) {
        return 0;
    }
    if (mConcealment > 15) {
        return 0;
    }
    if (mLastMoveTime + 2500 > Clock) {
        return 1;
    }
    switch (mType) {
    case kBot:
    case kConstruct:
    case kDroid:
    case kEgg:
    case kAlien:
    case kProgram:
    case kOoze:
        /* these creatures have the ability to remain perfectly still */
        return 0;
    case kCyborg:
    case kAberration:
    case kAnimal:
    case kBeast:
    case kHumanoid:
    case kMutant:
    case kInsect:
    case kOutsider:
    case kVermin:
    case kZerg:
        /* the creatures shift around enough when lurking that they are
           still detectable */
        return 1;
    case kMaxCreatureType:
        break;
    }
    return 0;
}


static
int
permuteScore (int base, int score)
{
    score += (base - 10);
    return score < 3 ? 3 : score;
}


void
shCreature::rollAbilityScores (int strbas, int conbas, int agibas, int dexbas,
                               int intbas, int psibas)
{
    if (isHero ()) {
        int totalpoints = strbas + conbas + agibas + dexbas +
                          intbas + psibas;

        mAbil.mStr = strbas;
        mAbil.mCon = conbas;
        mAbil.mAgi = agibas;
        mAbil.mDex = dexbas;
        mAbil.mInt = intbas;
        mAbil.mPsi = psibas;

        while (totalpoints < 71) {
            shAbilityIndex idx = shAbilities::getRandomIndex ();
            int score = mAbil.getByIndex (idx);

            if (RNG (3, 18) <= score
                and score < 17)
            {
                /* prefer to increase already high scores */
                int boost = RNG (1, 2);
                mAbil.setByIndex (idx, score + boost);
                totalpoints += boost;
            }
        }
    } else if (mCLevel < 4 and RNG (6)) {
        mAbil.mStr = permuteScore (strbas, 6 + NDX (2, 3));
        mAbil.mCon = permuteScore (conbas, 6 + NDX (2, 3));
        mAbil.mAgi = permuteScore (agibas, 6 + NDX (2, 3));
        mAbil.mDex = permuteScore (dexbas, 6 + NDX (2, 3));
        mAbil.mInt = permuteScore (intbas, 6 + NDX (2, 3));
        mAbil.mPsi = permuteScore (psibas, 6 + NDX (2, 3));
    } else {
        mAbil.mStr = permuteScore (strbas, 7 + NDX (2, 2));
        mAbil.mCon = permuteScore (conbas, 7 + NDX (2, 2));
        mAbil.mAgi = permuteScore (agibas, 5 + NDX (2, 4));
        mAbil.mDex = permuteScore (dexbas, 5 + NDX (2, 4));
        mAbil.mInt = permuteScore (intbas, 5 + NDX (2, 4));
        mAbil.mPsi = permuteScore (psibas, 7 + NDX (2, 2));
    }

    FOR_ALL_ABILITIES (i) {
        mMaxAbil.setByIndex (i, mAbil.getByIndex (i));
        mExtraAbil.setByIndex (i, 0);
    }
}


void
shCreature::rollHitPoints (int hitdice, int diesize)
{
    mMaxHP = 0;
    for (int i = 0; i < hitdice; i++) {
        int roll = (0 == i and isHero ()) ? diesize : RNG (1, diesize)
            + ABILITY_MODIFIER (getCon ());;
        mMaxHP += roll > 1 ? roll : 1;
    }
    mHP = mMaxHP;
}


shThingSize
shCreature::getSize ()
{
    return myIlk ()->mSize;
}

int
shCreature::getSizeACMod (shThingSize size)
{
    switch (getSize ()) {
        case kFine: return +8;
        case kDiminutive: return +4;
        case kTiny: return +2;
        case kSmall: return +1;
        case kMedium: return 0;
        case kLarge: return -1;
        case kHuge: return -2;
        case kGigantic: return -4;
        case kColossal: return -8;
        default:
            debug.log ("computeAC: unknown size: (%d)!", size);
            return 0;
    }
}

void
shCreature::computeAC ()
{
    mAC = 10 + mNaturalArmorBonus;
    mAC += ABILITY_MODIFIER (getAgi ());
    mAC += getSizeACMod (getSize());

    if (mBodyArmor) {
        mAC += mBodyArmor->getArmorBonus ();
    }
    if (mJumpsuit) {
        mAC += mJumpsuit->getArmorBonus ();
    }
    if (mHelmet) {
        mAC += mHelmet->getArmorBonus ();
    }
    if (mBelt) {
        mAC += mBelt->getArmorBonus ();
    }
    if (mBoots) {
        mAC += mBoots->getArmorBonus ();
    }
    if (mCloak) {
        mAC += mCloak->getArmorBonus ();
    }
}


/* returns non-zero if the creature should be sick from the sewer smell */
int
shCreature::sewerSmells ()
{
    if (mLevel and (shMapLevel::kSewer == mLevel->mType or
                   shMapLevel::kSewerPlant == mLevel->mType)
        and !hasAirSupply () and !isBreathless () and !mResistances[kSickening])
    {
        return 1;
    }
    return 0;
}


/*
  According to this article I just read, US Soldiers carry 54
  kilograms of gear, and this load encumbers them.
*/

static int CarryingCapacityByStrength[30] = {
0,      4535,   5210,   5985,   6875,   7897,   9071,   10420,  11970,  13750,
15795,  18143,  20841,  23940,  27500,  31590,  36287,  41683,  47881,  55001,
63180,  72574,  83366,  95762,  110002, 126360, 145149, 166733, 191525, 220005
};

void
shCreature::computeIntrinsics (bool quiet)
{
    shObject *obj;
    static unsigned int oldburden = 0;
    unsigned int newburden;
    int weight;
    int smelled = 0;

    smelled = sewerSmells ();

    mFeats = myIlk ()->mFeats;
    if (mProfession == XelNaga and mCLevel >= 10) {
        mInnateIntrinsics |= kTelepathy;
    }
    mIntrinsics = mInnateIntrinsics;
    memcpy (&mResistances, &mInnateResistances, sizeof (mResistances));
    mConcealment = 0;
    mPsiModifier = 0;
    mToHitModifier = 0;
    mDamageModifier = 0;
    mSpeedBoost = 0;
    FOR_ALL_ABILITIES (i) {
        mMaxAbil.changeByIndex (i, - mExtraAbil.getByIndex (i));
        mAbil.changeByIndex (i, - mExtraAbil.getByIndex (i));
        mExtraAbil.setByIndex (i, 0);
    }
    for (int i = 0; i < mInventory->count (); i++) {
        obj = mInventory->get (i);
        mIntrinsics |= obj->getConferredIntrinsics ();
        obj->applyConferredResistances (this);
    }
    if (mMutantPowers[kHaste] == 2) {
        int skill = getSkillModifier (kMutantPower, kHaste);
        mSpeedBoost += 20 + 2 * mini (skill, 10);
    }
    mSpeed = myIlk ()->mSpeed + mSpeedBoost;
    FOR_ALL_ABILITIES (i) {
        mMaxAbil.changeByIndex (i, mExtraAbil.getByIndex (i));
        mAbil.changeByIndex (i, mExtraAbil.getByIndex (i));
    }
    if (mGoggles and
        mGoggles->isA (kObjPerilGoggles) and
        mGoggles->isToggled () and
        !hasXRayVision ())
    {
        setBlind ();
    }
    if (isBlind ()) {
        /* vision enhancements do no good (harm) when blind */
        if (mGoggles) {
            mToHitModifier -= mGoggles->myIlk ()->mToHitModifier;
            mDamageModifier -= mGoggles->myIlk ()->mDamageModifier;
        }
        if (mHelmet) {
            mToHitModifier -= mHelmet->myIlk ()->mToHitModifier;
            mDamageModifier -= mHelmet->myIlk ()->mDamageModifier;
        }
    }
    if (mHelmet and mHelmet->isA (kObjBioMask)) {
        if (mHelmet->isToggled ()) {
            mIntrinsics |= kEMFieldVision;
        } else {
            mIntrinsics |= kNightVision;
        }
    }

    /* How much you can carry? */
    int effStr = mAbil.mStr; /* Effective strength for carrying equipment. */
    if (mBelt and mBelt->isA (kObjStabilizerBelt)) {
        effStr += 2 + mBelt->mBugginess;
    }
    if (effStr <= 0) {
        mCarryingCapacity = 0;
    } else if (effStr < 30) {
        mCarryingCapacity = CarryingCapacityByStrength[effStr];
    } else {
        mCarryingCapacity = 64 * CarryingCapacityByStrength[29];
    }
    /* Self-powered armor does not encumber. */
    if (mBodyArmor and mBodyArmor->isPoweredArmor ()) {
        mCarryingCapacity += mBodyArmor->getMass ();
    }
    if (mHelmet and mHelmet->isPoweredArmor ()) {
        mCarryingCapacity += mHelmet->getMass ();
    }
    if (mBoots and mBoots->isPoweredArmor ()) {
        mCarryingCapacity += mBoots->getMass ();
    }

    if (smelled and hasAirSupply ()) {
        if (isHero () and !quiet)
            I->p ("Fresh air!");
    } else if (!smelled and sewerSmells ()) {
        if (isHero () and !quiet)
            I->p ("What a terrible smell!");
        inflict (kSickened, 2000);
    }

    if (mTrapped.mDrowning and hasAirSupply ()) {
        mTrapped.mDrowning = 0;
        if (isHero () and !quiet) {
            I->p ("You can breathe!");
        }
    } else if (!mTrapped.mDrowning and !hasAirSupply () and isUnderwater ()) {
        mTrapped.mDrowning = getCon ();
        if (isHero () and !quiet)
            I->p ("You're holding your breath!");
    }

    /* You need to wear nothing over chameleon suit to benefit from it. */
    if (kCamouflaged & mIntrinsics) {
        if (mBodyArmor or mCloak) {
            mIntrinsics &= ~kCamouflaged;
        }
    }

    weight = mWeight;

    if (is (kViolated)) {
        mSpeed -= 50;
    }
    if (is (kSlowed)) {
        mSpeed -= 50;
    }
    if (is (kSpeedy)) {
        mSpeed += 100;
    }
    if (is (kHosed)) {
        mSpeed -= 100;
    }
    mSpeed += 5 * ABILITY_BONUS (getAgi ());

    /* Are you out to purge xenos scum?  Let us see your attire. */
    if (mProfession == SpaceMarine) {
        mConditions &= ~kXenosHunter;
        if ((mHelmet and mHelmet->isXenosHunterGear ()) or
            (mBodyArmor and mBodyArmor->isXenosHunterGear ()) or
            (mCloak and mCloak->isXenosHunterGear ()))
            mConditions |= kXenosHunter;
    }

    if (weight <= mCarryingCapacity) {
        newburden = 0;
    } else if (weight <= mCarryingCapacity * 2) {
        newburden = kBurdened;
        mSpeed -= 50;
    } else if (weight <= mCarryingCapacity * 3) {
        newburden = kStrained;
        mSpeed -= 100;
    } else if (weight <= mCarryingCapacity * 4) {
        newburden = kOvertaxed;
        mSpeed -= 300;
    } else {
        newburden = kOverloaded;
        mSpeed -= 700;
    }

    if (isHero () and oldburden != newburden) {
        if (!quiet) {
            if ((unsigned) oldburden > (unsigned) newburden) {
                if (0 == newburden) {
                    I->p ("You movement is no longer encumbered.");
                } else {
                    I->p ("Your movement is somewhat less encumbered now.");
                }
            } else if (kBurdened == newburden) {
                I->p ("You are encumbered by your load.");
            } else if (kStrained == newburden) {
                I->p ("You strain to manage your load.");
            } else if (kOvertaxed == newburden) {
                I->p ("You can barely move under this load.");
            } else {
                I->p ("You collapse under your load.");
            }
        }
        I->drawSideWin ();
        oldburden = newburden;
    }
    mConditions &= ~kOverloaded;
    mConditions |= newburden;
}


void
shCreature::computeSkills ()
{
    for (int i = 0; i < mSkills.count (); ++i) {
        shSkill *s = mSkills.get (i);
        s->mBonus = 0;
    }
    for (int i = 0; i < mInventory->count (); ++i) {
        shObject *obj = mInventory->get (i);
        obj->applyConferredSkills (this);
    }
}

/*
  RETURNS: a d20 roll, with small possibility of rolling higher than 20,
           to give a sporting chance of achieving a high result
*/

int
sportingD20 ()
{
    int result;

    result = RNG (1, 21);
    if (21 == result or Hero.isLucky ()) {
        do {
            result += RNG (6);
        } while (! RNG (3));
    }
    return result;
}


shSkill *
shCreature::getSkill (shSkillCode c, shMutantPower power)
{
    for (int i = 0; i < mSkills.count (); ++i) {
        shSkill *s = mSkills.get (i);
        if (s->mCode == c)
            switch (c) {
            case kMutantPower:
                if (power == s->mPower)
                    return s;
                break;
            default:
                return s;
            }
    }
    return NULL;
}


int
shCreature::getSkillModifier (shSkillCode c,
                              shMutantPower power)
{
    shSkill *skill;
    shAbilityIndex ability;
    int result = 0;

    switch (c) {
    case kMutantPower:
        skill = getSkill (c, power);
        ability = kPsi;
        break;
    default:
        skill = getSkill (c);
        ability = SKILL_KEY_ABILITY (c);
        break;
    }
    result += ABILITY_MODIFIER (mAbil.getByIndex (ability));
    if (skill) {
        result += skill->mRanks + skill->mBonus;
    }
    if (is (kSickened)) {
        result -= 2;
    }
    return result;
}


int
shCreature::getWeaponSkillModifier (shObjectIlk *ilk, shAttack *atk)
{
    shSkillCode c = kNoSkillCode;
    shSkill *skill = NULL;
    shAbilityIndex ability = kDex;
    int result = 0;

    if (NULL == ilk) { /* Barehanded. */
        c = kUnarmedCombat;
    } else {
        c = ilk->getSkillForAttack (atk);
        if (c == kNoSkillCode and atk->isMeleeAttack ()) {
            /* Improvised melee weapon. */
            ability = kStr;
            result -= 4;
        }
    }
    if (c != kNoSkillCode) {
        ability = SKILL_KEY_ABILITY (c);
        skill = getSkill (c);
    }
    result += mToHitModifier;
    result += ABILITY_MODIFIER (mAbil.getByIndex (ability));
    /* I have no idea why deviations from kMedium give bonuses. -- MB */
    switch (getSize ()) {
    case kFine: result += 8; break;
    case kDiminutive: result += 4; break;
    case kTiny: result += 2; break;
    case kSmall: result += 1; break;
    case kMedium: break;
    case kLarge: result += 1; break;
    case kHuge: result += 2; break;
    case kGigantic: result += 4; break;
    case kColossal: result += 8; break;
    default:
        I->p ("getWeaponSkillModifier: unknown size!");
        I->p ("Please file a bug report.");
        return -4;
    }

    if (NULL == skill or (0 == skill->mRanks and 0 >= skill->mBonus)) {
        /* inflict a slight penalty for lacking weapon skill,
           since we're not using SRD Feats */
        result -= 2;
    } else {
        result += skill->mRanks + skill->mBonus;
    }
    if (is (kSickened)) {
        result -= 2;
    }
    return result;
}


int
shCreature::gainRank (shSkillCode c, int howmany, shMutantPower power)
{
    shSkill * skill;

    skill = getSkill (c, power);
    if (NULL == skill) {
        skill = new shSkill (c, power);
        mSkills.add (skill);
    }
    skill->mRanks += howmany;
    return 1;
}


void
shCreature::addSkill (shSkillCode c, int access, shMutantPower power)
{
    shSkill *skill = new shSkill (c, power);
    skill->mAccess = access;
    mSkills.add (skill);
}
