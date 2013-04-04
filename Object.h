#ifndef OBJECT_H
#define OBJECT_H
#include <stdlib.h>

#include "Global.h"
#include "ObjectType.h"
#include "Util.h"
#include "MonsterIlk.h"


struct shObject;
typedef shVector <shObject *> shObjectVector;

int selectObjectsByFlag (shObjectVector *dest, shObjectVector *src, int flag);
int selectObjects (shObjectVector *dest, shObjectVector *src, shObjId ilk);
int selectObjects (shObjectVector *dest, shObjectVector *src,
                   shObjectType type);
int selectObjectsByFunction (shObjectVector *dest, shObjectVector *src,
                             int (shObject::*idfunc) (), int neg = 0);
int unselectObjectsByFunction (shObjectVector *dest, shObjectVector *src,
                               int (shObject::*idfunc) ());
int selectObjectsByCallback (shObjectVector *dest, shObjectVector *src,
                             int (*objfunc) (shObject *), int neg = 0);
int unselectObjectsByCallback (shObjectVector *dest, shObjectVector *src,
                               int (*objfunc) (shObject *));


/* shOperatingSystem values are stored in computer enhancement value. */
enum shOperatingSystem {
    kNoSys = -5,         /* computer inoperable */
    kAbysmalSys     = -4,
    kTerribleSys    = -3,
    kBadSys         = -2,
    kPoorSys        = -1,
    kAverageSys     =  0,
    kFineSys        = +1,
    kGoodSys        = +2,
    kExcellentSys   = +3,
    kSuperbSys      = +4,
    kUniqueSys      = +5
};

enum shCompInterface {
    kNotAComputer,
    kIVisualOnly,
    kIVisualVoice,
    kIUniversal
};

struct shObject
{
    /* NOTE: don't forget to update constructor AND split () method
       when adding new fields to this class!
       Oh yeah and saveObject() and loadObject() too!
    */
    shObjId mIlkId;
    shObjectIlk *myIlk () { return &AllIlks[mIlkId]; }
    shDescription *apparent (void) {
        if (isKnown ()) return &myIlk ()->mReal;
        if (isAppearanceKnown ()) return &myIlk ()->mAppearance;
        return &myIlk ()->mVague;
    }

    int mCount;               // for mergeable objects
    int mCharges;             // for chargeable objects (maxhp for wreck)
    int mHP;                  // hit points TODO: get rid of this
    int mDamage;              // corrosion / fire damage
    int mEnhancement;         // operating system for computers
    char mLetter;             // letter in Hero's inventory display
    int mBugginess;           // optimized +1 / debugged 0 / buggy -1
    shTime mLastEnergyBill;   // last time energy was billed to the item
    enum Flags {
        kKnownType =        0x1,   // item true name revealed
        kKnownBugginess =   0x2,
        kKnownEnhancement = 0x4,   // so called plus
        kKnownCharges =     0x8,   // # of charges
        kKnownAppearance =  0x10,  // disk labels, canister types
        kKnownInfected =    0x20,  // disk/computer infection
        kKnownCracked =     0x40,  // hacked software
        kKnownFooproof =    0x80,  // only computer antivirus might be unknown
        kKnownExact =       0xff,  // sum of all above
        kFooproof =         0x100,  // acidproof, fireproof, etc.
                                    // also computers protected by antivirus
        kCracked =          0x200,  // floppy disks and keycards
        kInfected =         0x400,  // software contains virus
        /* 0x800 - unused yet */
        kWorn =             0x1000,
        kWielded =          0x2000,
        kActive =           0x4000, // turned on, for example
        kUnpaid =           0x8000, // belongs to shopkeeper
        kToggled =          0x10000,
        kDesignated =       0x20000,// primary item for some action
        kPrepared =         0x40000 // quick swap weapon or item
    };
    int mFlags;

    enum Location {
        kNowhere,
        kFloor,
        kInventory
    } mLocation;
    int mX;
    int mY;
    shCreature *mOwner;       /* Creature with obj in inventory (use kUnpaid
                                 for obj owned by shopkeeper). */

    const char *mUserName;    /* Name given by user with name command. */
    union {
        shMonId mWreckIlk;
        shObjectIlk::Site mImplantSite;
        int mColor;           /* For customizing floppies and lightsabers. */
    };

    //constructor:
    shObject () {
        mIlkId = kObjNothing; mCount = 0; mEnhancement = 0; mCharges = 0;
        mHP = 0; mLetter = 0; mBugginess = 0; mFlags = 0; mOwner = NULL;
        mDamage = 0; mLastEnergyBill = MAXTIME; mUserName = NULL;
    }
    shObject (shObjId id);

    void saveState (int fd);
    void loadState (int fd);

    const char *getDescription (int cnt = -1);
    const char *getShortDescription (int cnt = -1);
    const char *getVagueDescription ();
    int isA (shObjId ilk);
    inline int isA (shObjectType type) { return myIlk ()->mReal.mType == type; }
    inline int looksLikeA (shObjectType type) { return apparent ()->mType == type; }
    int getIlkFlag (int id) { return myIlk ()->mFlags & id; }
    int getFlag (int id) { return mFlags & id; }
    void setFlag (int id) { mFlags |= id; }
    void resetFlag (int id) { mFlags &= ~id; }
    shGlyph getGlyph ();
    int getMass () { return mCount * myIlk ()->mWeight; }
    int getCost () { return mCount * myIlk ()->mCost; }
    void name (const char *newname = NULL);
    void nameIlk ();
    void maybeName ();
    inline int isIdentified () {
        return kKnownExact == (kKnownExact & mFlags);
    }
    inline int isChargeable () { return myIlk()->mMaxCharges; }
    inline int isChargeKnown () { return getFlag (kKnownCharges); }
    inline void setChargeKnown ()
    {
        setFlag (kKnownCharges);
        if (isA (kRayGun) and !mCharges)  mIlkId = kObjEmptyRayGun;
        if (mIlkId == kObjEmptyRayGun)   setKnown ();
    }
    inline int isBugginessKnown () { return getFlag (kKnownBugginess); }
    inline void setBugginessKnown ()
    {
        setFlag (kKnownBugginess);
        if (mIlkId == kObjTorc) setFlag (kKnownAppearance);
    }
    inline void resetBugginessKnown ()
    {
        if (!isBugProof ())  resetFlag (kKnownBugginess);
    }
    inline int isAppearanceKnown () { return getFlag (kKnownAppearance); }
    inline void setAppearanceKnown ()
    {
        setFlag (kKnownAppearance);
        if (mIlkId == kObjTorc) setFlag (kKnownBugginess);
        if (isIlkKnown ())  {
            setIlkKnown ();
            setFlag (kKnownType);
        }
    }
    inline void resetAppearanceKnown () { resetFlag (kKnownAppearance); }
    inline int isFooproofable () {
        return vulnerability () != kNoEnergy and (isA (kArmor) or isA (kWeapon));
    }
    inline int isCrackedKnown () { return getFlag (kKnownCracked); }
    inline void setCrackedKnown () { setFlag (kKnownCracked); }
    inline void resetCrackedKnown () { resetFlag (kKnownCracked); }
    inline int isCrackable () {
        return isA (kFloppyDisk) or isA (kObjGenericKeycard);
    }
    inline int isInfectedKnown () { return getFlag (kKnownInfected); }
    inline void setInfectedKnown () { setFlag (kKnownInfected); }
    inline void resetInfectedKnown () { resetFlag (kKnownInfected); }
    inline int isInfectable () {
        return isA (kFloppyDisk) or isA (kObjGenericComputer);
    }
    inline int isUnique () { return getIlkFlag (kUnique); }
    inline int isUniqueKnown () { return getIlkFlag (kUnique) and getIlkFlag (kIdentified); }
    inline int isKnown () {
        if (isAppearanceKnown () and isIlkKnown ())  setFlag (kKnownType);
        return getFlag (kKnownType);
    }
    inline void setKnown () {
        if (isAppearanceKnown ())  setIlkKnown ();
        setFlag (kKnownType);
    }
    inline int isEnhancementKnown () { return getFlag (kKnownEnhancement); }
    inline void setEnhancementKnown () { setFlag (kKnownEnhancement); }
    inline void resetEnhancementKnown () { resetFlag (kKnownEnhancement); }
    inline int isEnhanceable () { return myIlk ()->mMaxEnhancement; }
    bool isRadioactive ();
    bool isKnownRadioactive ();

    inline void identify () {
        if (!isChargeKnown ())  setChargeKnown (); /* Important for ray guns. */
        setFlag (kKnownExact);
        setIlkKnown ();
    }

    shEnergyType vulnerability ();
    inline int isFooproof () { return getFlag (kFooproof); }
    inline void setFooproof () { setFlag (kFooproof); }
    inline void resetFooproof () { resetFlag (kFooproof); }
    inline int isFooproofKnown () { return getFlag (kKnownFooproof); }
    inline void setFooproofKnown () { setFlag (kKnownFooproof); }
    inline void resetFooproofKnown () { resetFlag (kKnownFooproof); }
    inline int isIndestructible () { return getIlkFlag (kIndestructible); }
    int isFlammable () { return isA (kFloppyDisk); }
    inline int isBugProof () { return getIlkFlag (kBugProof); }
    inline int isBuggy () { return -1 == mBugginess; }
    inline int isDebugged () { return 0 == mBugginess; }
    inline int isOptimized () { return 1 == mBugginess; }
    inline void setBuggy () { if (!isBugProof ()) mBugginess = -1; }
    inline void setOptimized () { if (!isBugProof ()) mBugginess = 1; }
    inline void setDebugged () { mBugginess = 0; }
    inline void setBugginess (int b) { if (!isBugProof ()) mBugginess = b; }
    /* TODO: Those functions below ask to be replaced with just is (), set ()
       and reset () just like conditions are for monsters. */
    inline int isInfected () { return getFlag (kInfected); }
    inline void setInfected () { setFlag (kInfected); }
    inline void resetInfected () { resetFlag (kInfected); }
    inline int isCracked () { return getFlag (kCracked); }
    inline void setCracked () { setFlag (kCracked); }
    inline void resetCracked () { resetFlag (kCracked); }
    inline int isActive () { return getFlag (kActive); }
    inline void setActive () { setFlag (kActive); mLastEnergyBill = Clock; }
    inline void resetActive () {
        resetFlag (kActive);
        mLastEnergyBill = MAXTIME;
    }
    inline int isWorn () { return getFlag (kWorn); }
    inline void setWorn () { setFlag (kWorn); }
    inline void resetWorn () { resetFlag (kWorn); }
    inline int isWielded () { return getFlag (kWielded); }
    inline void setWielded () { setFlag (kWielded); }
    inline void resetWielded () { resetFlag (kWielded); }
    inline int isUnpaid () { return getFlag (kUnpaid); }
    inline void setUnpaid () { setFlag (kUnpaid); }
    inline void resetUnpaid () { resetFlag (kUnpaid); }
    inline int isToggled () { return getFlag (kToggled); }
    inline void setToggled () { setFlag (kToggled); }
    inline void resetToggled () { resetFlag (kToggled); }
    inline int isDesignated () { return getFlag (kDesignated); }
    inline void setDesignated () { setFlag (kDesignated); }
    inline void resetDesignated () { resetFlag (kDesignated); }
    inline int isPrepared () { return getFlag (kPrepared); }
    inline void setPrepared () { setFlag (kPrepared); }
    inline void resetPrepared () { resetFlag (kPrepared); }

    inline int isDamaged () {
        return mDamage;
    }
    inline int isEquipped () {
        return getFlag (kWorn) or getFlag (kWielded) or getFlag (kDesignated);
    }

    inline int canBeWorn () {
        return looksLikeA (kArmor) or looksLikeA (kImplant) or
               (isA (kObjGenericComputer) and isKnown ()) or
               (isA (kObjPlasmaCaster) and isKnown ());
    }
    inline int isSealedArmor () {
        return isA (kArmor) and getIlkFlag (kSealed);
    }
    inline int isPoweredArmor () {
        return isA (kArmor) and getIlkFlag (kPowered);
    }
    inline int isMeleeWeapon () {
        return myIlk ()->mMeleeAttack;
    }
    inline int isThrownWeapon () {
        return ((isA (kWeapon) or isKnown ()) and myIlk ()->mMissileAttack) or
               (isA (kObjProximityMine) and !isKnown ());
    }
    inline int isAimedWeapon () {
        return myIlk ()->mGunAttack or myIlk ()->mZapAttack;
    }
    bool isWeldedWeapon (void);
    inline int isSelectiveFireWeapon () {
        return isA (kWeapon) and getIlkFlag (kSelectiveFire);
    }
    inline int isKnownWeapon () {
        return isA (kWeapon) or (isKnown () and
            (myIlk ()->mMeleeAttack or myIlk ()->mGunAttack or
             myIlk ()->mZapAttack));
    }
    inline int canBeDrunk () {
        return !!myIlk ()->mQuaffFunc;
    }
    int isUseable ();
    /* Computer properties section. */
    inline int isComputer () { /* For OS floppy disk. */
        return isA (kObjGenericComputer);
    }
    int hackingModifier (void);
    bool supportsInterface (shCompInterface);
    bool hasSystem (void);
    bool systemRunPOST (void); /* Power-On Self Test */
    bool systemCheckDisk (shObject *disk);
    bool isUpdateTime (void);
    void systemUpdateMessage (void);
    void kickComputer (void);
    int executeProgram (shObject *media);
    int execTime (void);
    /* End section. */

    int isAmmo (shObject *weapon);

    void impact (shCreature *c, shDirection dir,
                 shCreature *thrower = NULL, shObject *stack = NULL);
    void impact (int x, int y, shDirection dir,
                 shCreature *thrower = NULL, shObject *stack = NULL);
    void impact (shFeature *c, shDirection dir,
                 shCreature *thrower = NULL, shObject *stack = NULL);
    void breakAt (int x, int y);

    int sufferDamage (shAttack *attack, int x, int y, int multiplier = 1,
                      int specialonly = 0);
    int sufferDamage (shAttackId id, int x, int y, int mult = 1, int spc = 0) {
        return sufferDamage (&Attacks[id], x, y, mult, spc);
    }
    int sufferDamage (shEnergyType energy, shAttack *attack,
                      shCreature *attacker, int x, int y);

    const shAbilities *getAbilityModifiers ();
    inline int getConferredIntrinsics () {
        return myIlk ()->mCarriedIntrinsics |
            (isWorn () ? myIlk ()->mWornIntrinsics : 0) |
            (isWielded () ? myIlk ()->mWieldedIntrinsics : 0) |
            (isActive () ? myIlk ()->mActiveIntrinsics : 0);
    }
    void applyConferredResistances (shCreature *target);
    void applyConferredSkills (shCreature *target);

    void draw (shInterface *I);
    void showLore (int analyze = 0);
    int canMerge (shObject *obj = NULL);
    void merge (shObject *obj); /* obj will be deleted */
    shObject *split (int count);

    const char *those (int cnt = -1);
    const char *these (int cnt = -1);
    const char *the (int cnt = -1);
    const char *theQuick (int cnt = -1);
    const char *an (int cnt = -1);
    const char *anQuick (int cnt = -1);
    const char *anVague (int cnt = -1);
    const char *your (int cnt = -1);
    const char *her (shCreature *owner, int cnt = -1);
    const char *inv (void);

    void announce (void);

    /* any object can be used as a weapon, so we supply these routines: */
    int getThreatRange (shCreature *target);
    int getCriticalMultiplier ();

    int getArmorBonus ();
    int getPsiModifier ();

    int isXenosHunterGear ()
    {
        return mIlkId == kObjAquamarinePH or mIlkId == kObjMeanGreenPH or
               mIlkId == kObjAquamarinePA or mIlkId == kObjMeanGreenPA or
               mIlkId == kObjAdamantineCloak;
    }

    inline int isIlkKnown () { return getIlkFlag (kIdentified); }
    inline void setIlkKnown ()
    {
        if (!isIlkKnown ()) {
            if (myIlk ()->mUserName) {
                free ((void *) myIlk ()->mUserName);
                myIlk ()->mUserName = NULL;
            }
        }
        myIlk ()->mFlags |= kIdentified;
    }
 private:
    const char *stackDesc (int cnt, const char *pronoun, const char *desc);
    const char *anStackDesc (int cnt, const char *desc);
};


int compareObjects (shObject **obj1, shObject **obj2);


int quaffCanister (shObject *can);

// infected == 1 means tell looks only
void identifyObjects (int howmany, int infected);

shObject *findBestObject (shObjectVector *objs, bool markSeen = false);
shObject *createWreck (shCreature *m);

void makePlural (char *buf, int len);
shObject *createObject (const char *desc, int flags);

shObject *generateObject (int level = 1);
shObject *pickFromCategory (shObjId category);
shObject *createWeapon ();
shObject *createArmor ();
shObject *createTool ();
shObject *createCanister ();
shObject *createFloppyDisk ();
shObject *createImplant ();
shObject *createRayGun ();

void initializeWeapons ();
void initializeTools ();
void initializeRayGuns ();
void initializeCanisters ();
void initializeFloppyDisks ();
void initializeImplants ();
void initializeObjects ();
void initializeLore ();
void initializeParser ();
void initializeEquipment ();

void purgeDeletedObjects ();

void itemSpoilers ();

extern shObject *TheNothingObject;

#endif
