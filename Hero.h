#ifndef HERO_H
#define HERO_H

#include "Global.h"
#include "Util.h"
#include "Creature.h"
#include "Monster.h"



struct shStoryFlag
{
    const char *mName;
    int mValue;
    shStoryFlag () {}
    shStoryFlag (const char *name, int value)
    {
        mName = strdup (name);
        mValue = value;
    }
};


class shHero : public shCreature
{
	int mXP;
    int mScore;
    int mBusy;
    int mSkillPoints;
    shVector <shStoryFlag *> mStoryFlags;

public:
    shVector <shCreature *> mPets;

    void saveState (int fd);
    void loadState (int fd);

    int isHero () { return 1; }

    void init (const char *name, shProfession *profession);

    const char *getTitle ()
    {
        if (mCLevel/3 < 10) {
            return mProfession->mTitles[mCLevel/3];
        } else {
            return mProfession->mTitles[9];
        }
    }
    const char *the ();
    const char *an ();
    const char *your ();
    const char *herself ();
    const char *getDescription ();

    int numHands () { return myIlk ()->mNumHands; }

    int die (shCauseOfDeath how, shCreature *killer,
        shObject *implement, shAttack *attack, const char *killstr = NULL);
    void epitaph (char *buf, int len, shCauseOfDeath how,
        const char *killstr, shCreature *killer);
    int tallyScore ();
    void tomb (char *message);
    void logGame (char *message);
    void postMortem ();

    int addObjectToInventory (shObject *obj, int quiet = 0);

    int wield (shObject *obj, int quiet = 0);
    int unwield (shObject *obj, int quiet = 0);

    void earnXP (int challenge);
    void earnScore (int points);

    void oldLocation (int newX, int newY, shMapLevel *newLevel);
    void newLocation ();
    void lookAtFloor ();
    void doSearch ();
    int getStoryFlag (const char *name) {
        int i;
        for (i = 0; i < mStoryFlags.count (); i++) {
            if (0 == strcmp (name, mStoryFlags.get (i) -> mName)) {
                return mStoryFlags.get (i) -> mValue;
            }
        }
        return 0;
    }

    void setStoryFlag (const char *name, int value) {
        int i;
        for (i = 0; i < mStoryFlags.count (); i++) {
            if (0 == strcmp (name, mStoryFlags.get (i) -> mName)) {
                mStoryFlags.get (i) -> mValue = value;
                return;
            }
        }
        mStoryFlags.add (new shStoryFlag (name, value));
    }

    void resetStoryFlag (const char *name) {
        setStoryFlag (name, 0);
    }

    int looksLikeJanitor ();
    int isMute() { return getStoryFlag ("superglued tongue"); };

    shObject *quickPickItem (shObjectVector *v, const char *action,
                             int flags, int *count = NULL);

    int interrupt ();  /* returns 0 if hero was busy */

    int instantUpkeep ();

    void useBOFHPower ();

    int tryToTranslate (shCreature *c);

    /* Shopping section. */
    void enterShop ();
    void leaveShop ();
    void damagedShop (int x, int y);
    void dropIntoOwnedVat (shObject *obj);
    void quaffFromOwnedVat (shFeature *vat);
    void movedVat ();
    void stolenVat ();
    void pickedUpItem (shObject *obj);
    void billableOffense (shObject *obj, const char *gerund, int fee);
    void employedItem (shObject *obj, int fee);
    void usedUpItem (shObject *obj, int cnt, const char *action);
    void maybeSellItem (shObject *obj);
    void payShopkeeper ();
    /* Trade section. */
    void enterHospital ();
    void leaveHospital ();
    void payDoctor (shMonster *doctor);
    void payMelnorme (shMonster *trader);
    void bribe (shMonster *lawyer);

    void enterCompactor ();
    void leaveCompactor ();

    void checkForFollowers (shMapLevel *level, int sx, int sy);
    int displace (shCreature *c);
    void newEnemy (shCreature *c) { };
    int doMove (shDirection dir);
    void drop (shObject *obj);
    int kick (shDirection dir);
    void reorganizeInventory ();
    int listInventory ();
    void feel (int x, int y, int force = 0);
    void spotStuff ();
    void sensePeril ();

    void addSkillPoints (int points) { mSkillPoints += points; }
    void editSkills ();
    int getXP () { return mXP; }

    void quaffFromVat (shFeature *vat);
    void quaffFromAcidPit ();
    void abortion (int quiet = 0);

    int useKey (shObject *key, shFeature *door);

    void torcCheck ();
    shMutantPower getMutantPower (shMutantPower power = kNoMutantPower,
                                  int silent = 0);

    int canHearThoughts (shCreature *c) {
        shSkill *s = getSkill (kMutantPower, kTelepathyPower);
        int bonus = (s ? s->mRanks * 5 : 0) + mCLevel;
        return (hasTelepathy () and c->hasMind () and
                distance (this, c->mX, c->mY) < 5 * (bonus + 15));
    }

    int canTrackMotion (shCreature *c) {
        return (hasMotionDetection () and c->isMoving () and
                distance (this, c->mX, c->mY) < 50);
    }

    int canFeelSteps (shCreature *c) {
        shSkill *s = getSkill (kMutantPower, kTremorsensePower);
        int bonus = (s ? s->mRanks * 5 : 0) + mCLevel;
        return (hasTremorsense () and c->isMoving () and
                distance (this, c->mX, c->mY) < 5 * (bonus + 12));
    }

    int isAwareOf (shCreature *c) {
        return canSee (c) or canHearThoughts (c) or canSenseLife (c)
            or canTrackMotion (c) or canFeelSteps (c);
    }

    const char *radLevels ();
    int getMutantPowerChance (int power);
    int useMutantPower ();
    int stopMutantPower (shMutantPower id);

    /* fighting functions; see Fight.cpp */
    int meleeAttack (shObject *weapon, shDirection dir);

    void upkeep ();
    void takeTurn ();
    void amputateTail ();
 private:
    /* Actions possibly invoked by takeTurn. */
    int objectVerbCommand (shObject *obj);
    void adjust (shObject *obj);
    int doWear (shObject *obj);
    int doTakeOff (shObject *obj);
};
#endif
