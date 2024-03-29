#include "Global.h"
#include "Creature.h"
#include "Object.h"


/* professions are basically character classes.  I didn't want to use the
   term class and cause confusion with C++ classes...  -CADV
*/

//PRIME adds here the Ninja and the Astronaut, the former being idea of Robot at the www.zapm.org board

struct shProfession;
extern shVector <shProfession *> Professions;

#ifndef CLASS_H
#define CLASS_H


struct shSkill {
    shSkillCode mCode;
    shMutantPower mPower; /* only set for mCode == kMutantPower */

    int mRanks;
    int mBonus;     /* bonus from items, race, and misc effects */
    int mAccess;    /* number of ranks earned per 4 levels */

    shSkill (shSkillCode code, shMutantPower power = kNoMutantPower) {
        mCode = code;
        if (kMutantPower == code) {
            mPower = power;
        } else {
            mPower = kNoMutantPower;
        }
        mAccess = 1;
        mRanks = 0;
        mBonus = 0;
    }

    const char *getName ();
    void getDesc (char *buf, int len);
    int getValue () { return mRanks + mBonus; }
};

typedef void (shHeroInitFunction) (shHero *);

struct shProfession
{
    int mId;
    const char *mName;        /* generic name, e.g. "janitor" */
    int mHitDieSize;
    const char *mTitles[10];

    int mNumPracticedSkills;  /* num skill points earned per level */
    int mReflexSaveBonus;     /* Reflex Save = level * mRSB / 4 */
    int mWillSaveBonus;       /* Reflex Save = level * mWSB / 4 */

    shHeroInitFunction *mInitFunction;

    void postInit (shHero *hero);
    shProfession (const char *name, int hitdiesize, int numskills,
                  int reflex, int will,
                  shHeroInitFunction *f,
                  const char *t1,
                  const char *t2,
                  const char *t3,
                  const char *t4,
                  const char *t5,
                  const char *t6,
                  const char *t7,
                  const char *t8,
                  const char *t9,
                  const char *t10);
};


void initializeProfessions ();

shProfession *chooseProfession ();

extern shProfession *SpaceMarine;
extern shProfession *Quarterback;
extern shProfession *Janitor;
extern shProfession *Psion;
extern shProfession *SoftwareEngineer;
extern shProfession *Ninja;
extern shProfession *Abductor;
extern shProfession *Yautja;
extern shProfession *XelNaga;

#endif
