#include <math.h>
#include <stdio.h>
#include "Global.h"
#include "Util.h"
#include "Monster.h"
#include "Creature.h"
#include "ObjectIlk.h"
#include "Object.h"
#include "Interface.h"
#include "Hero.h"
#include "MonsterIlk.h"

#include "MonsterIlk.cpp"

#define MAXMONSTERLEVEL 25
static shVector <shMonId> MonsterIlksGroupedByLevel[MAXMONSTERLEVEL];

shAttackId lizard[NUMLIZARDS];
void addLizardBreaths ();

static int
sym2TileY (char sym)
{
    if (sym == '@') {
        return 2;
    } else if (sym <= 'Z') {
        return sym - 'A' + kRowBigA;
    } else {
        return sym - 'a' + kRowLittleA;
    }
}

void
initializeMonsters ()
{
    for (int i = 0; i < kMonNumberOf; ++i) {
        shMonsterIlk *ilk = &MonIlks[i];
        ilk->mId = shMonId (i);
        if (ilk->mGlyph.mTileY == -1)
            ilk->mGlyph.mTileY = sym2TileY (ilk->mGlyph.mSym);

        if (ilk->mGlyph.mTileX == 0) { /* White ASCII character. */
            /* NotEye will repaint the glyph to right color.
               24 is simply far enough to right to be safely assumed free. */
            ilk->mGlyph.mTileX = 24 + ilk->mGlyph.mColor;
        }

        for (int j = 0; j < MAXPOWERS; ++j) {
            ilk->mPowers[j]  and  ++ilk->mNumPowers;
        }

        for (int j = 0; j < MAXATTACKS; ++j) {
            ilk->mAttackSum += ilk->mAttacks[j].mProb;
        }

        // Temporarily, until monster generation mechanism is improved
        MonsterIlksGroupedByLevel[ilk->mBaseLevel].add (ilk->mId);
    }


    int roll = RNG (3);
    lizard[0] = roll == 0 ? kAttHeatRay :
                roll == 1 ? kAttFreezeRay :
                            kAttPoisonRay;
    roll = RNG (6);
    lizard[1] = roll == 0 ? kAttHeatRay :
                roll == 1 ? kAttFreezeRay :
                roll == 2 ? kAttPoisonRay :
                roll == 3 ? kAttTransporterRay :
                roll == 4 ? kAttGammaRay :
                            kAttStasisRay;
    roll = RNG (6);
    lizard[2] = roll == 0 ? kAttPoisonRay :
                roll == 1 ? kAttGammaRay :
                roll == 2 ? kAttTransporterRay :
                roll == 3 ? kAttGaussRay :
                roll == 4 ? kAttStasisRay :
                            kAttDisintegrationRay;
}

void addLizardBreaths ()
{   /* Clone and modify ray gun zap effect. */
    for (int i = 0; i < NUMLIZARDS; ++i) {
        shAttack *atk = &Attacks[shAttackId (kAttLizardBreath1 + i)];
        memcpy (atk, &Attacks[lizard[i]], sizeof (shAttack));
        atk->mRange = 8;
        atk->mAttackTime = LONGTURN - HALFTURN * i;
        atk->mDamage[0].mHigh = atk->mDamage[0].mHigh * (i+1) / 2;
    }
}

/* Equipment.cpp defines regular monster posessions. */
struct shEquip {
    const char **mPtr;
    int mCnt;
};
extern shEquip equipment[kMonNumberOf];

//constructor:
shMonster::shMonster (shMonId id, int extralevels /* = 0 */ )
    :shCreature ()
{
    int do_or = 0;
    int do_and = 0;
    int gotweapon = 0;
    shMonsterIlk *ilk = &MonIlks[id];
    mIlkId = id;

    mType = ilk->mType;
    strncpy (mName, ilk->mName, 30);
    mCLevel = ilk->mBaseLevel;
    mNaturalArmorBonus = ilk->mNaturalArmorBonus;
    mReflexSaveBonus = 0;

    mDir = kNoDirection;
    mTame = 0;
    mStrategy = ilk->mDefaultStrategy;
    mDisposition = ilk->mDefaultDisposition;
    mTactic = kReady;
    mDestX = -1;
    mEnemyX = -1;
    mPlannedMoveIndex = -1;
    mSpellTimer = 0;

    memcpy (mInnateResistances, ilk->mInnateResistances,
            sizeof (mInnateResistances));
    mInnateIntrinsics = ilk->mInnateIntrinsics;
    for (int i = 0; i < ilk->mNumPowers; ++i)
        if (ilk->mPowers[i] == kTelepathyPower)
            mInnateIntrinsics |= kTelepathy; /* Assume always on. */
    mFeats = ilk->mFeats;

#define IMMUNE 122

    switch (mType) {
    case kMutant:
        mInnateResistances[kRadiological] = IMMUNE;
        mInnateResistances[kMagnetic] = IMMUNE;
        break;
    case kHumanoid:
        mInnateResistances[kMagnetic] = IMMUNE;
        break;
    case kAnimal:
        mInnateResistances[kMagnetic] = IMMUNE;
        mInnateIntrinsics |= kScent;
        break;
    case kInsect:
        mInnateResistances[kMagnetic] = IMMUNE;
        mInnateResistances[kRadiological] = IMMUNE;
        mInnateIntrinsics |= kScent;
        break;
    case kOutsider:
        mInnateResistances[kMagnetic] = IMMUNE;
        break;
    case kBot:
        mInnateResistances[kToxic] = IMMUNE;
        mInnateResistances[kPsychic] = IMMUNE;
        mInnateResistances[kRadiological] = IMMUNE;
        mInnateResistances[kViolating] = IMMUNE;
        mInnateResistances[kMesmerizing] = IMMUNE;
        mInnateResistances[kBlinding] = IMMUNE;
        mInnateIntrinsics |= kBreathless;
        break;
    case kDroid:
        mInnateResistances[kToxic] = IMMUNE;
        mInnateResistances[kPsychic] = IMMUNE;
        mInnateResistances[kRadiological] = IMMUNE;
        mInnateResistances[kViolating] = IMMUNE;
        mInnateResistances[kMesmerizing] = IMMUNE;
        mInnateResistances[kBlinding] = IMMUNE;
        mInnateIntrinsics |= kBreathless;
        break;
    case kProgram:
        mInnateResistances[kToxic] = IMMUNE;
        mInnateResistances[kRadiological] = IMMUNE;
        mInnateResistances[kViolating] = IMMUNE;
        /* Processes can be put to sleep so no kMesmerizing immunity. */
        mInnateIntrinsics |= kBreathless;
        break;
    case kConstruct:
        mInnateResistances[kToxic] = IMMUNE;
        mInnateResistances[kPsychic] = IMMUNE;
        mInnateResistances[kRadiological] = IMMUNE;
        mInnateResistances[kStunning] = IMMUNE;
        mInnateResistances[kConfusing] = IMMUNE;
        mInnateResistances[kViolating] = IMMUNE;
        mInnateResistances[kParalyzing] = IMMUNE;
        mInnateResistances[kMesmerizing] = IMMUNE;
        mInnateResistances[kBlinding] = IMMUNE;
        mInnateIntrinsics |= kBreathless;
        break;
    case kOoze:
        mInnateResistances[kToxic] = 3;
        mInnateResistances[kMagnetic] = IMMUNE;
        mInnateResistances[kRadiological] = IMMUNE;
        mInnateResistances[kViolating] = IMMUNE;
        mInnateResistances[kWebbing] = IMMUNE;
        mInnateIntrinsics |= kBreathless;
        break;
    case kAberration:
        mInnateResistances[kMagnetic] = IMMUNE;
        mInnateResistances[kRadiological] = IMMUNE;
        mInnateResistances[kViolating] = IMMUNE;
        break;
    case kCyborg:
        mInnateResistances[kToxic] = 2;
        mInnateResistances[kRadiological] = 10;
        mInnateIntrinsics |= kBreathless;
        break;
    case kEgg:
        mInnateResistances[kViolating] = IMMUNE;
        mInnateIntrinsics |= kBreathless;
        /* fall through */
    case kBeast:
        mInnateResistances[kMagnetic] = IMMUNE;
        mInnateIntrinsics |= kScent;
        break;
    case kVermin:
        mInnateResistances[kMagnetic] = IMMUNE;
        mInnateIntrinsics |= kScent;
        mInnateIntrinsics |= kCanSwim;
        break;
    case kAlien:
        mAlienEgg.mHatchChance = 0;
        mInnateResistances[kMagnetic] = IMMUNE;
        mInnateResistances[kRadiological] = 10;
        mInnateResistances[kCorrosive] = IMMUNE;
        mInnateIntrinsics |= kScent;
        if (!isA (kMonAlienQueen)) /* Too heavy and bloated. */
            mInnateIntrinsics |= kJumpy;
        gainRank (kUnarmedCombat, mCLevel/2);
        break;
    default:
        debug.log ("Alert! Unknown monster type");
        mInnateResistances[kMagnetic] = IMMUNE;
    }

#undef IMMUNE

    if (3 == ilk->mGender) {
        mGender = RNG (1, 2);
    } else {
        mGender = ilk->mGender;
    }

    /* roll ability scores */
    rollAbilityScores (ilk->mStr, ilk->mCon, ilk->mAgi,
                       ilk->mDex, ilk->mInt, ilk->mPsi);

    /* roll hit points */
    rollHitPoints (ilk->mHitDice, 8);

    /* setup speed */
    mSpeed = ilk->mSpeed;
    computeAC ();

    mGlyph = ilk->mGlyph;
    if (isA (kMonMutantNinjaTurtle))
        mGlyph.mTileX += RNG (4); /* Got four of those. */
    mState = kActing;

    if (equipment[id].mPtr) /* Does monster get standard equipment? */
    for (int i = 0; i < equipment[id].mCnt; ++i) {
        const char *str = equipment[id].mPtr[i];

        if ('|' == str[0]) { /* the or symbol indicates don't try to create
                                the object unless the previous obj failed */
            if (0 == do_or) {
                do_or = 1;
                do_and = 0;
                continue;
            } else {
                ++str;
            }
        } else if (',' == str[0]) { /* the comma symbol indicates create the
                                       object only if the prev obj was made */
            if (1 == do_and) {
                ++str;
            } else {
                continue;
            }
        }

        shObject *obj = createObject (str, 0);

        if (NULL == obj) {
            do_or = 1;
            do_and = 0;
            debug.log ("unable to equip %s", str);
            continue;
        }
        do_or = 0;
        do_and = 1;
        addObjectToInventory (obj);
        if (!mWeapon and obj->isA (kWeapon)) {
            if (obj->myIlk ()->mMeleeSkill != kNoSkillCode) {
                gainRank (obj->myIlk ()->mMeleeSkill, 1 + mCLevel * 2/3);
            }
            if (obj->myIlk ()->mGunSkill != kNoSkillCode) {
                gainRank (obj->myIlk ()->mGunSkill, 1 + mCLevel * 2/3);
            }
            ++gotweapon;
            /* don't wield until hero is in sight so he can see message */
            //wield (obj, 1);
        } else if (obj->isA (kArmor) or obj->isA (kImplant)) {
            don (obj, 1);
        }
    }

    if (!gotweapon) {
        gainRank (kUnarmedCombat, 1 + mCLevel);
    }

    /* Maybe monster gets some more treasure. */
    if (noTreasure () or
        kAnimal == mType  or kBeast == mType    or kAberration == mType or
        kInsect == mType  or kVermin == mType   or kOutsider == mType or
        kEgg == mType     or kOoze == mType     or kAlien == mType or
        kBot == mType     or kConstruct == mType)
    { /* No treasure requested or a monster does not want treasure. */
    } else {
        if (RNG (50) <= 5 + mCLevel) {
            shObject *cash = new shObject (kObjMoney);
            cash->mCount = NDX (mCLevel + 1, 10);
            addObjectToInventory (cash);
        }
        if (RNG (80) <= 5 + mCLevel) {
            shObject *obj = generateObject (-1);

            while (obj->getMass () > 5000) {
                delete obj;
                obj = generateObject (-1);
            }

            addObjectToInventory (obj);
        }
    }

    computeIntrinsics ();

    if (RNG (100) < ilk->mPeacefulChance) {
        mDisposition = kIndifferent;
    }

    debug.log ("spawned %s with %d HP speed %d", mName, mHP, mSpeed);
}


shMonId
pickAMonsterIlk (int level)
{
    int baselevel = level;
    int n = 0;
    shVector <shMonId> v;
    shMonId ilk;
    int i;
    int c;

//    rarity = RNG (3) ? 1 : RNG (3) ? 2 : 3;

    while (0 == (n = MonsterIlksGroupedByLevel[baselevel].count ())) {
        baselevel--;
    }


    for (i = 0; i < n; i++) {
        ilk = MonsterIlksGroupedByLevel[baselevel].get (i);
        c = MonIlks[ilk].mProbability;
        while (c--) {
            v.add (ilk);
        }
    }
    if (!v.count ()) {
        return shMonId (0);
    }

    return v.get (RNG (v.count ()));
}


shMonster *
generateMonster (int level)
{
    /* generates random monster of the given level */

    int baselevel = level;
    int n;

    while (0 == (n = MonsterIlksGroupedByLevel[baselevel].count ())) {
        baselevel--;
    }

    return new shMonster (MonsterIlksGroupedByLevel[baselevel].get (RNG (n)),
                          baselevel - level);
}


const char *
shMonster::the ()
{
    char *buf = GetBuf ();

    if (!Hero.canSee (this) and !Hero.canHearThoughts (this)) {
        strncpy (buf, hasMind () ? "someone" : "something", SHBUFLEN);
    } else if (Hero.is (kXenosHunter) and isXenos ()) {
        snprintf (buf, SHBUFLEN, "the xenos scum");
    } else if (Hero.is (kXenosHunter) and isHeretic ()) {
        snprintf (buf, SHBUFLEN, "the heretic");
    } else {
        snprintf (buf, SHBUFLEN, "the %s", myIlk ()->mName);
    }
    return buf;
}

const char *
shMonster::an ()
{
    char *buf = GetBuf ();

    if (!Hero.canSee (this) and !Hero.canHearThoughts (this)) {
        strncpy (buf, hasMind () ? "someone" : "something", SHBUFLEN);
    } else if (isUnique ()) {
        return the ();
    } else if (Hero.is (kXenosHunter) and isXenos ()) {
        snprintf (buf, SHBUFLEN, "a xenos scum");
    } else if (Hero.is (kXenosHunter) and isHeretic ()) {
        snprintf (buf, SHBUFLEN, "a heretic");
    } else {
        snprintf (buf, SHBUFLEN,
                  isvowel (myIlk ()->mName[0]) ? "an %s" : "a %s",
                  myIlk ()->mName);
    }
    return buf;
}

const char *
shMonster::your ()
{
    char *buf = GetBuf ();

    if (Hero.isBlind () and !Hero.canHearThoughts (this)) {
        strncpy (buf, hasMind () ? "someone" : "something", SHBUFLEN);
    } else {
        snprintf (buf, SHBUFLEN, "your %s", myIlk ()->mName);
    }
    return buf;
}


const char *
shMonster::getDescription ()
{
    return myIlk ()->mName;
}

int
shMonster::numHands ()
{
    return myIlk ()->mNumHands;
}

/*     SPOILER GENERATION     */

int speedToAP (int speed); /* Game.cpp */

void
shMonsterIlk::spoilers (FILE *spoilerfile)
{
    int i, n = 100;
    int AC = 0;
    int speed = 0;
    int HP = 0;
    int ap = 0;

    int acfoo = 4 * mBaseLevel / 3  + 10;

    int attacks = 0;
    int hits = 0;
    int damagefoo = 0;
    int damage15 = 0;
    int damage20 = 0;
    int damage25 = 0;

    int rhits = 0;
    int rattacks = 0;
    int rdamagefoo = 0;
    int zhits = 0;

    shMonster *m;

    for (i = 0; i < n; i++) {
        m = new shMonster (mId);
        int clk = 0;

        AC += m->getAC ();
        speed += m->mSpeed;
        HP += m->mMaxHP;

        m->readyWeapon ();

        if (m->mWeapon and m->mWeapon->isA (kRayGun)) {
            zhits += 1;
            continue;
        }

        while (clk < 60000) {
            shAttack *atk = NULL;

            int attack = 0;
            int damage = 0;

            if (!mAttackSum) {
                break;
            }

            clk += 100;
            ap += speedToAP (m->mSpeed);
            if (ap < 0) {
                continue;
            }

            attack += RNG (1, 20);
            if (20 == attack) attack = 100;

            if (m->mWeapon) {
                if (m->mWeapon->isMeleeWeapon ()) {
                    atk = &Attacks[m->mWeapon->myIlk ()->mMeleeAttack];
                    damage = ABILITY_MODIFIER (m->getStr ());
                } else if (m->mWeapon->isThrownWeapon ()) {
                    atk = &Attacks[m->mWeapon->myIlk ()->mMissileAttack];
                    damage = ABILITY_MODIFIER (m->getStr ());
                } else {
                    atk = &Attacks[m->mWeapon->myIlk ()->mGunAttack];
                    damage = 0;
                    --attack; /* assume range penalty */
                }
                attack += m->mWeapon->myIlk ()->mToHitModifier;
                attack += m->getWeaponSkillModifier (m->mWeapon->myIlk (), atk);
                attack += m->mWeapon->mEnhancement;
                damage += m->mWeapon->mEnhancement;
                if (!m->mWeapon->isMeleeWeapon ()) {
                    rattacks++;
                } else {
                    attacks++;
                }
            } else {
                // FIXME: Use attack chooser here.
                atk = &Attacks[mAttacks[0].mAttId];
                damage = ABILITY_MODIFIER (m->getStr ());
                ++attacks;
            }

            ap -= atk->mAttackTime;


            attack += m->mToHitModifier;
            damage += RNG (atk->mDamage[0].mLow,
                           atk->mDamage[0].mHigh);
            if (acfoo > 22) { /* spot hero +2 agi bonus */
                damage -= (acfoo - 21) / 2;
            }
            if (damage < 1) damage = 1;
            if (attack > acfoo) {
                if (m->mWeapon and !m->mWeapon->isMeleeWeapon ()) {
                    rdamagefoo += damage;
                    ++rhits;
                } else {
                    damagefoo += damage;
                    ++hits;
                }

            }
            if (attack >= 15) {
                damage15 += damage;
            }
            if (attack >= 20) {
                damage20 += damage;
            }
            if (attack >= 25) {
                damage25 += damage;
            }
        }

        delete m;
    }



    fprintf (spoilerfile,
             "%28s %2d %3d %4d %4d %3d/%3d %4d %3d/%3d %4d %2d\n",
             mName,
             AC / n, HP / n, speed / n,
             acfoo,
             rhits / (n),
             rattacks / (n),
             rdamagefoo / (n),
             hits / (n),
             attacks / (n),
             damagefoo / (n),
             zhits);

/*
    I->p ("                                     %4d/15 %4d/20 %4d/25",
          damage15 / (6 * n),
          damage20 / (6 * n),
          damage25 / (6 * n));
*/
}





void
monsterSpoilers ()
{
    FILE *spoilerfile = fopen ("monsters.txt", "w");

    I->p ("Generating spoilers, please be patient...");

    fprintf (spoilerfile, "%28s %2s %3s %4s %2s %3s/%3s %4s %3s/%3s %4s\n",
             "Monster", "AC", "HP", "Spd", "vsAC", "hit", "rgd", "dmg", "hit", "atk", "dmg");

    for (int i = 0; i < kMonNumberOf; ++i) {
        shMonsterIlk *ilk = &MonIlks[i];
        ilk->spoilers (spoilerfile);
    }
    fclose (spoilerfile);
    I->p ("Spoilers saved in monsters.txt.");
}
