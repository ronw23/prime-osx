#ifndef MONSTER_H
#define MONSTER_H

#include "Util.h"
#include "Creature.h"
#include "AttackType.h"
#include "Interface.h"
#include "Services.h"

class shMonster : public shCreature
{
public:
    enum Strategy {
        kWander,      /* wander around looking for a fight */
        kLurk,        /* stay put, if Hero approaches, fight and pursue */
        kSitStill,    /* stay put, attack any adjacent creature */
        kHide,        /* same as sitstill, but pursue if hero finds us */
        kHatch,       /* alien egg strategy */
        kPet,         /* pet strategy */
        kShopKeep,
        kAngryShopKeep,
        kGuard,
        kDoctor,
        kMelnorme
    };

    enum Tactic {
        kNewEnemy,  /* respond to new threat */
        kReady,     /* ready for next tactic */
        kFight,     /* attempt close combat */
        kShoot,     /* attempt ranged combat */
        kMoveTo     /* move to a location */
    };

    enum Disposition {
        kHelpful,
        kFriendly,
        kIndifferent,
        kUnfriendly,
        kHostile
    };

    int mTame; /* 0 = wild, 1 = loyal */

    int mEnemyX;  /* set if monster is threatened by enemy other than hero */
    int mEnemyY;  /* (right now only used for pets) */
    int mDestX;
    int mDestY;
    union {
        struct {
            int mShopId;
            int mHomeX;  /* coordinates of the "home spot" in front of */
            int mHomeY;  /* the shop door */
            int mBill;   /* amount owed for destroyed items */
        } mShopKeeper;
        struct {
            int mSX;       /* coordinates of guarded rectangle */
            int mSY;
            int mEX;
            int mEY;
            int mToll;     /* toll: -1 for none, 0 for already paid */
            int mChallengeIssued;
        } mGuard;
        struct {
            int mHatchChance;
        } mAlienEgg;
        struct {
            int mHomeX;
            int mHomeY;
            int mRoomID;
            int mPermute[kMedMaxService];
        } mDoctor;
        struct {
            int mKnowledgeExhausted;
            int mSX;     /* Coordinates of current room. */
            int mSY;
            int mEX;
            int mEY;
            int mPermute[kMelnMaxService];
        } mMelnorme;
    };
    Strategy mStrategy;
    Tactic mTactic;
    Disposition mDisposition;
    shDirection mPlannedMoves[100];
    shTime mSpellTimer;
    int mPlannedMoveIndex;

    char mPlaceHolder; /* kludge saves me 22 seconds */

    shMonster () {}
    shMonster (shMonId id, int extralevels = 0);

    void saveState (int fd);
    void loadState (int fd);

    void takeTurn ();
    const char *the ();
    const char *an ();
    const char *your ();
    const char *getDescription ();

    int numHands ();

    void followHeroToNewLevel ();

    void makeAngry ();
    void newEnemy (shCreature *c);
    void newDest (int x, int y);

    int checkThreats ();
    void findPetGoal ();
    void findTreasure ();

    int likesMoney ();
    int likesWeapons ();
    int needsWeapon ();

    int readyWeapon ();
    int setupPath ();
    int doQuickMoveTo (shDirection dir = kNoDirection);
    int clearObstacle (int x, int y);
    int drinkBeer ();

    enum SquareFlags {
        kHero =       0x1,
        kMonster =    0x2,
        kEnemy =      0x4,
        kLinedUp =    0x10,
        kDoor =       0x100,
        kWall =       0x200,
        kTrap =       0x400,
        kFreeMoney =  0x1000,
        kFreeWeapon = 0x2000,
        kFreeArmor =  0x4000,
        kFreeEnergy = 0x8000,
        kFreeWreck =  0x10000,
        kFreeItem =   0x1f000,
        kHidingSpot = 0x20000
    };

    int findSquares (int flags, shCoord *coords, int *info);

    int doAttack (shCreature *target, int *elapsed);
    void doRangedAttack (shAttack *attack, shDirection dir);
    int useMutantPower ();
    /* Strategies: */
    int doSitStill ();
    int doHide ();
    int doLurk ();
    int doHatch ();
    int doWander ();
    int doPet ();
    int doShopKeep ();
    int doAngryShopKeep ();
    int doGuard ();
    int doDoctor ();
    int doMelnorme ();

    int doRetaliate ();

    int doFight ();
    int doMoveTo ();

    int mimicSomething ();
    int meleeAttack (shObject *weapon, shAttack *attack, int x, int y);
    /* The two functions below return kAttDummy on no attack was chosen or
       available.  On success attack identifier is returned. */
    shAttackId pickMeleeAttack ();
    shAttackId pickRangedAttack ();

    int isHostile () { return kHostile == mDisposition; }
    int isPet () { return mTame; }
    void makePet ();
};


struct shMonsterIlk
{
public:
    shMonId mId;
    // TODO: This does not belong in monsterIlk. More like in herodata.
    int mKills;             // how many died during game
    shCreatureType mType;

    const char *mName;

    shThingSize mSize;
    int mHitDice;
    int mBaseLevel;

    int mStr;
    int mCon;
    int mAgi;
    int mDex;
    int mInt;
    int mPsi;

    int mSpeed;

    int mGender;
    int mNumHands;    // number of hands that can wield weapons

    int mNaturalArmorBonus;
    int mFeats;
    unsigned int mInnateIntrinsics;
    int mInnateResistances[kMaxEnergyType];

#define MAXPOWERS 5
    shMutantPower mPowers[MAXPOWERS];
    int mNumPowers;
#define MAXATTACKS 5
    struct shAttackData {
        shAttackId mAttId;
        int mProb;
    } mAttacks[MAXATTACKS], mRangedAttacks[MAXATTACKS];
    int mAttackSum;

    int mNumAppearingDice;
    int mNumAppearingDieSides;
    int mProbability;

    shGlyph mGlyph;

    shMonster::Strategy mDefaultStrategy;
    shMonster::Disposition mDefaultDisposition;
    int mPeacefulChance;

    void spoilers (FILE *);
};


void initializeMonsters ();

shMonId pickAMonsterIlk (int level);
shMonster *generateMonster (int level);

void monsterSpoilers ();

#define NUMLIZARDS 3
extern shAttackId lizard[NUMLIZARDS];
extern void addLizardBreaths ();

#endif
