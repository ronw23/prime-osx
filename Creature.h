#ifndef CREATURE_H
#define CREATURE_H

#define ABILITY_MODIFIER(score) ((score) / 2 - 5)
#define ABILITY_BONUS(score) ((score) < 12 ? 0 : (score) / 2 - 5)
#define ABILITY_PENALTY(score) ((score) > 9 ? 0 : (score) / 2 - 5)
#define ABILITY_MAX 25

enum shActionState
{
    kActing,   /* actively taking turns */
    kWaiting,  /* don't process, on another level */
    kDead      /* don't process, will be cleaned up soon */
};


enum shGender {
    kFemale =    0x1,
    kMale =      0x2
};

enum shCreatureType {
    /* non-living: */
    kBot,
    kDroid,
    kProgram,
    kConstruct,
    /* living: */
    kEgg,
    kOoze,
    /* living, and has mind: */
    kCyborg,
    kAberration,
    kAnimal,
    kAlien,
    kBeast,
    kHumanoid,
    kMutant,      /* mutant humanoid */
    kInsect,
    kOutsider,
    kVermin,
    kZerg,
    kMaxCreatureType
};
#define IS_ROBOT(_crtype) (_crtype <= kDroid)
#define IS_PROGRAM(_crtype) (_crtype == kProgram)
#define IS_ALIVE(_crtype) (_crtype > kConstruct)
#define HAS_MIND(_crtype) (_crtype > kOoze)
#define RADIATES(_crtype) (_crtype <= kDroid or \
                           (_crtype >= kCyborg and _crtype != kAlien))

enum shCreatureSymbols {
    kSymHero = '@',
    kSymBot = 'x',
    kSymDroid = 'X',
    kSymHumanoid = 'h',
    kSymCritter = 'c',
    kSymWorm = 'w',
    kSymInsect = 'i',
    kSymGray = 'g',
    kSymZerg = 'Z'
};


enum shCondition {
    kSpeedy =        0x1,       /* from temporary speed boost item */
    kSlowed =        0x2,       /* also from temporary item */
    kConfused =      0x4,       /* mental blast, canister of beer, etc. */
/*  unused           0x8,
    unused           0x10, */
    kHosed =         0x20,      /* traffic */
    kViolated =      0x40,      /* anal probe */
    kXenosHunter =   0x80,      /* wears marine stuff */
/*  unused           0x100, */
    kSickened =      0x200,     /* virus attack */
    kParalyzed =     0x400,     /* stasis ray, some melee attacks, etc. */
/*  unused           0x800,
    unused           0x1000, */
    kStunned =       0x2000,    /* stun grenades, etc */
    kAsleep =        0x4000,    /* Hypnosis, narcoleptor. */
    kPlagued =       0x8000,    /* Defiler plague. */

    kNoExp =         0x10000,   /* Do not give experience upon death. */
/*  unused           0x20000,
    unused           0x40000,
    unused           0x80000,*/

    kUnableCompute = 0x100000,  /* The Voice said not to read the Nam-Shub. */
    kUnableUsePsi =  0x200000,  /* The Voice said no mutant power usage. */
    kUnableAttack =  0x400000,  /* The Voice said not to show belligerence. */
    kGenerous =      0x800000,  /* The Voice said to lower prices. */

    kFleeing =       0x1000000, /* but not necessarily frightened - for mons */
    kFrightened =    0x2000000,
/*  unused           0x4000000,
    unused           0x8000000,*/

    kBurdened =      0x10000000,
    kStrained =      0x30000000,
    kOvertaxed =     0x70000000,
    kOverloaded =    0xf0000000
};
const unsigned int kEncumbrance = kOverloaded;


#define SKILL_KEY_ABILITY(_code) \
    ((shAbilityIndex) (((_code) & kSkillAbilityMask) >> 8))

enum shFeat {
    kNoFeat =           0x000,
    kMimicMoney =       0x001,
    kMimicObject =      0x002,
    kMimicFeature =     0x004,
    kHideUnderObject =  0x008,
    kExplodes =         0x010,
    kSessile =          0x020,
    kNoTreasure =       0x040,
    kUniqueMonster =    0x080,
    kWarpish =          0x100,
    kBurrow =           0x200,
    kMaxFeat
};


#include "Map.h"
#include "Profession.h"

int sportingD20 ();


enum shAbilityIndex {
    kStr = 1,
    kCon = 2,
    kAgi = 3,
    kDex = 4,
    kInt = 5,
    kPsi = 6
};
#define NUM_ABIL kPsi


struct shAbilities {
    int mStr, mCon, mAgi, mDex, mInt, mPsi;

    int getByIndex (shAbilityIndex idx) const;
    void setByIndex (shAbilityIndex idx, int val);
    void changeByIndex (shAbilityIndex idx, int delta);
    static const char *abilName (shAbilityIndex i);
    static const char *shortAbilName (shAbilityIndex i);
    static shAbilityIndex getRandomIndex ();
};
#define FOR_ALL_ABILITIES(x) \
    for (shAbilityIndex x = kStr; \
         x <= kPsi; \
         x = (shAbilityIndex) ((int)(x)+1))

/* What are kSlain and kKilled for:
 * kSlain means whacked, cut or shot to death.  Aliens dying that way splash
 * acid everywhere.  Smart bombs, smart missiles and other explosive bots go
 * boom.
 * kKilled means death comes through some non-violent way.  Aliens do not splash
 * acid, smart bombs leave proximity mine item.
 */
enum shCauseOfDeath {
    kSlain,
    kKilled,
    kAnnihilated,  /* Took disintegration damage. */
    kSuddenDecompression,
    kTransporterAccident,
    kSuicide,
    kDrowned,
    kBrainJarred,
    kMisc, /* Miscellaneous stupid stuff, use killer text. */
    kQuitGame,
    kWonGame
};

/* timeout keys */

enum shTimeOutType {
    NOTIMEOUT,
    ASLEEP,
    BLINDED,
    CAFFEINE,
    CONFUSED,
    FLEEING,
    FRIGHTENED,
    HOSED,
    PARALYZED,
    PLAGUED,
    SICKENED,
    SLOWED,
    SPEEDY,
    STUNNED,
    TELEPATHY,
    TRAPPED,
    VIOLATED,
    VOICE_GENEROSITY,
    VOICE_NAMSHUB,
    VOICE_MUTANT,
    VOICE_VIOLENCE,
    XRAYVISION
};

class shCreature
{
 public:

    int mX;
    int mY;
    int mZ; /* -1, 0, 1  for in pit, on ground, flying */
    class shMapLevel *mLevel;
    shMonId mIlkId;
    shMonsterIlk *myIlk ();

    struct TimeOut {
        shTimeOutType mKey;
        shTime mWhen;

        TimeOut () { };
        TimeOut (shTimeOutType key, shTime when) {
            mKey = key;
            mWhen = when;
        }
    };

    shCreatureType mType; /* TODO: Can be read from ilk, remove. */
    shProfession *mProfession;

    char mName[30];
    int mGender;

    int mAP;        /* action points */

    int mCLevel;    // character level

    int mAC;        // armor class
    int mConcealment;

    int mHP;        // hit points
    int mMaxHP;
    shTime mLastRegen; //time of last HP regen

    int mNaturalArmorBonus;
    shAbilities mAbil;
    shAbilities mMaxAbil;
    shAbilities mExtraAbil; // component of maxabil from items
    //TODO: The two fields below are relevant only for the Hero.
    int mPsiDrain;  // temporary drain
    int mPsionicStormFatigue;  // fatigue exclusive to psionic storm

    int mSpeed;
    int mSpeedBoost; /* e.g. psionic speed boost */

    int mReflexSaveBonus; /* when dodging area effects */
    int mWillSaveBonus;   /* when saving against psychic attacks */

    /* resistances represented as damage reduction */
    int mInnateResistances[kMaxEnergyType]; /* permanent */
    int mResistances[kMaxEnergyType];      /* inate + that from equip */

    int mFeats;
    char mMutantPowers[kMaxAllPower];
    int mHidden;             /* hide score (negative iff Hero has spotted, but
                                monster doesn't know) */
    enum { kNothing, kObject, kFeature, kMonster } mMimic;
    shTime mSpotAttempted;   /* last time hero tried to spot creature */
    int mInnateIntrinsics;    /* permanent + those from mutant powers */
    int mIntrinsics;         /* inate + those gained from equipment/implants */
    int mConditions;
    int mRad;                /* radiation exposure */
    int mCarryingCapacity;   /* all weights are in grams */
    int mPsiModifier;        /* effect of equipment on psionic attacks */
    int mToHitModifier;      /* effect of equipment on hit chance */
    int mDamageModifier;     /* effect of eqiupment on damage rolls */

    int mWeight;

    struct {
        int mInPit;          /* how badly stuck in pit */
        int mDrowning;       /* air left in lungs */
        int mWebbed;         /* sticky webs impeding movement */
        int mTaped;          /* getting immobilized with duct tape */
    } mTrapped;

    shTime mLastMoveTime;    /* time of last movement */
    shDirection mDir;        /* direction of current movement */
    int mLastX;              /* previous x,y coordinates */
    int mLastY;
    shActionState mState;
    shGlyph mGlyph;
#define TRACKLEN 5
    shCoord mTrack[TRACKLEN]; /* mTrack[0] == our last position */


    shObject *mWeapon;       /* must be the first ptr field (see save ()) */

    shObject *mJumpsuit;     /* worn under body armor */
    shObject *mBodyArmor;
    shObject *mHelmet;
    shObject *mBoots;
    shObject *mGoggles;
    shObject *mBelt;
	shObject *mCloak;

    shObject *mImplants[shObjectIlk::kMaxSite];

    union {
        shObjectIlk *mMimickedObject;
        shFeature::Type mMimickedFeature;
        shMonsterIlk *mMimickedMonster;
    };

    shVector <TimeOut *> mTimeOuts;
    shVector <shSkill *> mSkills;
    shObjectVector *mInventory;
    shMapLevel *mLastLevel;  /* previous level */
    shCauseOfDeath mHowDead; /* Set when mState is set to kDead. */


    //constructor:
    shCreature ();
    //destructor:
    virtual ~shCreature ();

    virtual void saveState (int fd);
    virtual void loadState (int fd);

    int isA (shMonId id);

    virtual int isHero () { return 0; }
    virtual const char *the () = 0;
    virtual const char *an () = 0;
    virtual const char *your () = 0;
    virtual const char *getDescription () = 0;
    virtual const char *herself ();

    virtual int interrupt () { return 0; }

    virtual const char *her (const char *thing);

    int reflexSave (shAttack *attack, int DC);
    int willSave (int DC);

    int reflectAttack (shAttack *attack, shDirection *dir);
    //RETURNS: 1 if attack kills us; o/w 0
    int sufferAbilityDamage (shAbilityIndex idx, int amount,
                             int ispermanent = 0);

    //RETURNS: 1 if attack kills us; o/w 0
    int sufferDamage (shAttack *attack, shCreature *attacker = NULL,
        shObject *weapon = NULL,
        int bonus = 0, int multiplier = 1, int divisor = 1);
    int sufferDamage (shAttackId attid, shCreature *atkr = NULL,
        shObject *weapon = NULL, int bonus = 0, int mult = 1, int divd = 1)
    {
        return sufferDamage (&Attacks[attid], atkr, weapon, bonus, mult, divd);
    }
    //RETURNS: 1 if the creature really dies; o/w 0
    virtual int die (shCauseOfDeath how, shCreature *killer,
        shObject *implement, shAttack *attack, const char *killstr = NULL);
    int die (shCauseOfDeath how, const char *killstr = NULL) {
        return die (how, NULL, NULL, NULL, killstr);
    }
    void pDeathMessage (const char *monname, shCauseOfDeath how,
                               int presenttense = 0);

    virtual int checkRadiation ();

    virtual shTime setTimeOut (shTimeOutType key, shTime howlong,
                               int additive = 1);
    virtual TimeOut *getTimeOut (shTimeOutType key);
    inline void clearTimeOut (shTimeOutType key) { setTimeOut (key, 0, 0); }
    int checkTimeOuts ();
    void showLore ();


    //RETURNS: 1 if successful
    virtual int addObjectToInventory (shObject *obj, int quiet = 0);
    void removeObjectFromInventory (shObject *obj);
    shObject *removeOneObjectFromInventory (shObject *obj);
    shObject *removeSomeObjectsFromInventory (shObject *obj, int cnt);
    void useUpOneObjectFromInventory (shObject *obj);
    //RETURNS: number of objects used
    int useUpSomeObjectsFromInventory (shObject *obj, int cnt);
    int owns (shObject *obj);

    //RETURNS: 1 if successful, 0 o/w
    virtual int wield (shObject *obj, int quiet = 0);
    //RETURNS: 1 if successful, 0 o/w
    virtual int unwield (shObject *obj, int quiet = 0);
    int expendAmmo (shObject *weapon, int cnt = 0);
    int hasAmmo (shObject *weapon);

    inline int isAlive () { return IS_ALIVE (mType); }
    inline int isProgram () { return IS_PROGRAM (mType); }
    inline int isRobot () { return kBot == mType or kDroid == mType; }
    inline int isInsect () { return kInsect == mType; }
    inline int isAlien () { return kAlien == mType; }
    inline int radiates () { return RADIATES (mType); }
    inline int isOrc () { return mGlyph.mSym == 'O'; }
    inline int isXenos () {
        return mGlyph.mSym == 'E' or
               mGlyph.mSym == 'O' or
               mGlyph.mSym == 'g' or
               mGlyph.mSym == 'k';
    }
    inline int isHeretic () {
        return isA (kMonBeneGesserit) or isA (kMonSorceress) or
               isA (kMonMutantHuman);
    }


    virtual int isHostile () { return 0; }

    virtual int don (shObject *obj, int quiet = 0);
    virtual int doff (shObject *obj, int quiet = 0);
    void damageEquipment (shAttack *attack, shEnergyType energy);
    void damageEquipment (shAttackId attid, shEnergyType energy)
    {
        damageEquipment (&Attacks[attid], energy);
    }
    int transport (int x, int y, int tries, int safe, int override = 0);

    virtual int useMutantPower () = 0;


    //RETURNS: 1 if successful, 0 o/w
    int openDoor (int x, int y);
    int closeDoor (int x, int y);

    void shootLock (shObject *weapon, shAttack *attack, shFeature *door);

    void doDiagnostics (int quality, int present = 1, FILE *file = NULL);

    virtual int numHands () { return 0; }

    int getStr () { return mAbil.mStr; }
    int getCon () { return mAbil.mCon; }
    int getAgi () { return mAbil.mAgi; }
    int getDex () { return mAbil.mDex; }
    int getInt () { return mAbil.mInt; }
    int getPsi () { return mAbil.mPsi; }
    void gainAbility (bool controlled, int num);
    int needsRestoration ();

    int getPsionicDC (int powerlevel);

    void computeIntrinsics (bool quiet = false);
    void computeSkills ();
    void computeAC ();
    int sewerSmells ();

    int getAC (int flatfooted = 0)
    {
        return flatfooted ? mAC - ABILITY_BONUS (getAgi ()) : mAC;
    }

    int getTouchAC (int flatfooted = 0)
    {
        return flatfooted ? 10 + ABILITY_PENALTY (getAgi ())
                          : 10 + ABILITY_MODIFIER (getAgi ());
    }

    shThingSize getSize ();
    int getSizeACMod (shThingSize size);

    int isImmuneToCriticalHits () { return isHero (); }
    int getResistance (shEnergyType etype) {
        return mResistances[etype] > 100 ? 1000 : mResistances[etype];
    }
    void addSkill (shSkillCode c, int access, shMutantPower power = kNoMutantPower);
    shSkill *getSkill (shSkillCode c, shMutantPower power = kNoMutantPower);

    int getSkillModifier (shSkillCode c, shMutantPower power = kNoMutantPower);
    int gainRank (shSkillCode c, int howmany = 1,
                  shMutantPower power = kNoMutantPower);

    int getWeaponSkillModifier (shObjectIlk *ilk, shAttack *atk = NULL);

    void levelUp ();

    int countEnergy (int *tankamount = NULL);
    int loseEnergy (int amount);
    void gainEnergy (int amount);

    int countMoney ();
    int loseMoney (int amount);
    int gainMoney (int amount);

    /* conditions */

    int getEncumbrance () { return mConditions & kEncumbrance; }

    int hasMind () { return HAS_MIND (mType) or isA (kMonBrainMold); }
    int isMoving ();
    virtual int isPet () { return 0; }
    int attemptRestraining (shObject *bolt);

    int hasTelepathy () { return mIntrinsics & kTelepathy; }
    int hasTremorsense () { return mIntrinsics & kTremorsense; }
    int hasSenseLife () { return mIntrinsics & kSenseLife; }
    int hasMotionDetection () { return mIntrinsics & kMotionDetection; }
    int hasScent () { return mIntrinsics & kScent; }
    int hasXRayVision () { return mIntrinsics & kXRayVision; }
    int hasReflection () { return mIntrinsics & kReflection; }
    int hasAutoSearching () { return mIntrinsics & kAutoSearching; }
    int hasTranslation () { return mIntrinsics & kTranslation; }
    int hasHealthMonitoring () { return mIntrinsics & kHealthMonitoring; }
    int hasRadiationProcessing () { return mIntrinsics & kRadiationProcessing;}
    int hasAutoRegeneration () { return mIntrinsics & kAutoRegeneration; }
    int hasLightSource () { return mIntrinsics & kLightSource; }
    int hasPerilSensing () { return mIntrinsics & kPerilSensing; }
    int hasShield () { return mIntrinsics & kShielded; }
    int hasBugSensing () { return mIntrinsics & kBugSensing; }
    int isFlying () { return mIntrinsics & kFlying; }
    int isBlind () { return mIntrinsics & kBlind; }
    int isLucky () { return mIntrinsics & kLucky; }
    int hasNarcolepsy () { return mIntrinsics & kNarcolepsy; }
    int hasBrainShield () { return mIntrinsics & kBrainShielded; }
    int isScary () { return mIntrinsics & kScary; }
    int hasAcidBlood () { return mIntrinsics & kAcidBlood; }
    int isMultiplier () { return mIntrinsics & kMultiplier; }
    int hasAirSupply () { return mIntrinsics & kAirSupply; }
    int isBreathless () { return mIntrinsics & kBreathless; }
    int canSwim () { return mIntrinsics & kCanSwim; }
    int hasNightVision () { return mIntrinsics & kNightVision; }
    int hasEMFieldVision () { return mIntrinsics & kEMFieldVision; }
    int canJump () { return mIntrinsics & kJumpy; }
    int isInvisible () { return mIntrinsics & kInvisible; }
    int canHideUnder (shObject *o) { return mFeats & kHideUnderObject
                                         and o->myIlk ()->mSize > getSize (); }
    int canHideUnder (shFeature *f) { return mFeats & kHideUnderObject
                                          and f->isHidingSpot (); }
    int canHideUnderObjects () { return mFeats & kHideUnderObject; }
    int canHideUnderWater () { return mFeats & kHideUnderObject; }
    int canBurrow () { return mFeats & kBurrow; }
    int canMimicMoney () { return mFeats & kMimicMoney; }
    int canMimicObjects () { return mFeats & kMimicObject; }
    int isExplosive () { return mFeats & kExplodes; }
    int isSessile () { return mFeats & kSessile; }
    int noTreasure () { return mFeats & kNoTreasure; }
    int isUnique () { return mFeats & kUniqueMonster; }
    int isWarpish () { return mFeats & kWarpish; }

    void checkConcentration ();

    int is (shCondition cond);
    void inflict (shCondition cond, int howlong);
    void cure (shCondition cond);

    void setBlind () { mIntrinsics |= kBlind; }
    void resetBlind () { mIntrinsics &= ~kBlind; }
    void makeBlinded (int howlong);
    int isTrapped ();
    int tryToFreeSelf ();
    int isUnderwater () {
        if (mZ >= 0 or !mLevel) return 0;
        shFeature *f = mLevel->getFeature (mX, mY);
        if (f and shFeature::kSewagePit == f->mType) return 1;
        return 0;
    }

    void sterilize ();
    int revealSelf ();

    int isAdjacent (int x, int y) { return areAdjacent (mX, mY, x, y); }
    int isInShop () { return mLevel->isInShop (mX, mY); }
    int isInPit ();

    bool isFriendly (shCreature *c) {
        return (c->isHero () or c->isPet ()) == (isHero () or isPet ());
    }

    int canSmell (shCreature *c);
    int canSenseLife (shCreature *c) {
        return (hasSenseLife () and c->isAlive () and
                distance (this, c->mX, c->mY) < 5 * (mCLevel * 4 + 4));
    }
    //RETURNS:  0 for completely concealed target, 100 for visible target
    int canSee (int x, int y);
    int canSee (shCreature *c);
    int hasCamouflage ();

    int hpWarningThreshold () { return mini (mMaxHP-1, maxi (mMaxHP/6, 5)); }

    virtual void newEnemy (shCreature *c) = 0;

    virtual int doMove (shDirection dir);

    /* fighting functions; see Fight.cpp */

    int attackRoll (shAttack *attack, shObject *weapon,
                    int attackmod, int AC, shCreature *target);
    int rangedAttackHits (shAttack *attack, shObject *weapon, int attackmod,
                          shCreature *target, int *dbonus);
    int resolveRangedAttack (shAttack *attack, shObject *weapon,
                             int attackmod, shCreature *target,
                             shObject *stack = NULL);
    int shootWeapon (shObject *weapon, shDirection dir,
                     shAttack *attack = NULL);
    int resolveMeleeAttack (shAttack *attack, shObject *weapon,
                            shCreature *target);
    void projectile (shObject *obj, shObject *stack,
                     int x, int y, shDirection dir,
                     shAttack *attack, int range);
    int throwObject (shObject *obj, shObject *stack, shDirection dir);
    int utilizeWreck (shObject *obj);
    int misfire (shObject *weapon, shAttack *attack);

    /* Can effects: */
    int healing (int hp, int hpmaxdice);
    int fullHealing (int hp, int hpmaxdice);

    /* Mutant powers: */
    int telepathy (int on);
    int tremorsense (int on);
    int digestion (shObject *obj);
    int opticBlast (shDirection dir);
    int shootWeb (shDirection dir);
    int hypnosis (shDirection dir);
    int xRayVision (int on);
    int mentalBlast (int x, int y);
    int regeneration ();
    int restoration (int howmuch);
    int adrenalineControl (int on);
    int haste (int on);
    int teleportation ();
    int psionicStorm (int x, int y);
    int theVoice (int x, int y);
    int ceaseAndDesist ();

    /* Initialization crap: */

    void rollAbilityScores (int strbas, int conbas, int agibas, int dexbas,
                            int intbas, int psibas);
    int statsViability ();
    void rollHitPoints (int hitdice, int diesize);

    /* Taking action: */
    virtual void takeTurn () = 0;
 private:
    shObject *getKeyForDoor (shFeature *f);
};

#endif
