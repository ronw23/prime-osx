#ifndef OBJECTTYPE_H
#define OBJECTTYPE_H

#include "AttackType.h"
#include "Flavor.h"
#include "ObjectIlk.h"

/* Various types of objects in the game.
   ObjectType is the most general classification of an object,
   ObjectIlk is a more specific subclassification.
*/

extern shGlyph ObjectGlyphs[];
extern int ColorMap[22];

enum shThingSize
{                 /* creature size */
    kFine,        /* 6 in or less */
    kDiminutive,
    kTiny,
    kSmall,
    kMedium,
    kLarge,
    kHuge,
    kGigantic,
    kColossal
};

enum shMaterialType  /* Based on Nethack materials. */
{
    kFleshy,
    kPaper,
    kCloth,
    kPlastic,
    kLeather,
    kWood,
    kBone,
    kIron,
    kBrass,
    kTin,
    kBronze,
    kLead,
    kSteel,
    kAluminum,
    kAsbestos,
    kTitanium,
    kKevlar,
    kCarbonFiber,
    kPlasteel,
    kAdamantium,
    kDepletedUranium,
    kCopper,
    kSilver,
    kGold,
    kPlatinum,
    kSilicon,
    kGlass,
    kCrystal,
    kLastMaterial
};
extern const char *matname[kLastMaterial];

/* This one is full.  Should think what to do about it sometime. */
enum shIntrinsics {
    kTelepathy =           0x1,
    kMotionDetection =     0x2,
    kTremorsense =         0x4,
    kScent =               0x8,
    kXRayVision =          0x10,
    kReflection =          0x20,
    kAutoSearching =       0x40,
    kTranslation =         0x80,  /* babel fish */
    kHealthMonitoring =    0x100, /* low HP warning */
    kRadiationProcessing = 0x200,
    kSenseLife =           0x400, /* Xel'Naga and aliens' racial ability */
    kAutoRegeneration =    0x800,
    kLightSource =         0x1000,
    kPerilSensing =        0x2000,
    kBlind =               0x4000,
    kShielded =            0x8000, /* protected by a shield generator */
    kBugSensing =          0x10000, /* software engineer */
    kFlying =              0x20000,
    kLucky =               0x40000,
    kNarcolepsy =          0x80000,
    kBrainShielded =       0x100000,
    kScary =               0x200000,
    kAcidBlood =           0x400000, /* splash damage on death */
    kMultiplier =          0x800000,
    kAirSupply =           0x1000000, /* has air supply */
    kBreathless =          0x2000000, /* doesn't need to breathe */
    kCanSwim =             0x4000000,
    kNightVision =         0x8000000,
    kCamouflaged =         0x10000000, /* Invisible next to obstacles. */
    kInvisible =           0x20000000, /* Covered by cloaking field. */
    kEMFieldVision =       0x40000000, /* Like night vision but for aliens. */
    kJumpy =               0x80000000  /* One-turn escape from pits. */
};


enum shObjectIlkFlags {
    kMergeable = 0x1,
    /* UNUSED = 0x2     was kChargeable */
    kIdentified = 0x4,
    /* UNUSED = 0x8     was kEnhanceable */
    kBugProof = 0x10,
    kUsuallyBuggy = 0x20,
    kUnique = 0x40,  /* Use "the" instead of a/an indefinite article. */
    kIndestructible = 0x80,
    kRadioactive = 0x100, /* Always emits radiation. */
    kPowered = 0x200, /* The weight of worn powered armor doesn't encumber. */
    kSelectiveFire = 0x400, /* Gun has burst and single shot modes. */
    kSealed = 0x800, /* Protects against vacuum. */
    kPreidentified = 0x1004  /* Do not show in discoveries table. */
};
/* NOTE: If you want to add something that emits radiation only
   under certain conditions do it through shObject::isRadioactive
   and shObject::isRadioactiveKnown instead.  The flag is for
   objects that irradiate surroundings no matter what. */


void randomizeIlkNames ();
void randomizeServiceNames ();

enum shExecResult
{
    kNormal = 0x0,
    kNoExpiry = 0x1,
    kNoEat = 0x2,
    kDestruct = 0x4, /* destruct */
    kFakeExpiry = 0x8, /* always pretend license ended and then destruct */
    kGoneAlready = 0x10, /* assume the pointer is invalid */
    kDestroyComputer = 0x20 /* delete computer and floppy disk */
};

/* following return ms elapsed */
typedef int shToolFunc (shObject *tool);
typedef int shCanisterFunc (shObject *canister);
typedef int shCanisterUseFunc (shObject *canister);
typedef shExecResult shFloppyDiskFunc (shObject *computer, shObject *disk);

struct shDescription {
    shObjectType mType;
    const char *mName;
    shGlyph mGlyph;
};

struct shObjectIlk
{   /* base properties */
    shObjId mId;
    shObjId mParent; // parent ilk, useful for isA ()
                     // e.g. bullet is parent of silver bullet

    /* Description data for item.
        real - precise name and correct object type
        appearance - ignorant description
        vague - blind description
    */
    shDescription mReal, mAppearance, mVague;
    const char *mUserName;   /* name given by user, e.g.: "tastes like crap" */

    shMaterialType mMaterial;

    int mCost;         /* base cost in shop */

    int mFlags;
    unsigned int mCarriedIntrinsics; /* conferred to the carrier */
    unsigned int mWornIntrinsics;    /* conferred only when worn */
    unsigned int mWieldedIntrinsics; /* conferred only when wielded */
    unsigned int mActiveIntrinsics;  /* conferred only when active */

    int mProbability; /* relative probability when randomly generating;
                         the constant ABSTRACT indicates an abstract ilk */
    shFlavorId mFlavor; /* assigned random description */

    int mWeight;       // in grams
    shThingSize mSize; // physical size / unwieldiness.
    int mHardness;     // flat incoming damage reduction
    int mHP;           // base HP
    int mEnergyUse;    /* for items that continuously use energy (e.g. geiger
                          counters, flashlights), ticks per cell consumed
                          for items that use energy in bursts (e.g. weapons),
                          cells consumed per use */

    /* weapon properties */
    int mRange;   // in ft, 0 indicates melee weapon
    shObjId mAmmoType;
    int mAmmoBurst; /* no of rounds of ammo /energy cells consumed per burst */
    shSkillCode mMeleeSkill;
    shSkillCode mGunSkill;
    shAttackId mMeleeAttack;   /* bump with this wielded */
    shAttackId mMissileAttack; /* throw it */
    shAttackId mGunAttack;     /* fire it */
    shAttackId mZapAttack;     /* zap it like ray gun */

    /* implant properties */
    enum Site {
        kFrontalLobe,
        kParietalLobe,
        kOccipitalLobe,
        kTemporalLobe,
        kCerebellum,
        kNeck,
        kLeftEar,
        kRightEyeball,

        kMaxSite,
        kAnyEar,
        kAnyEye,
        kAnyBrain,
        kNoSite
    };
    Site mSite;
    shSkillCode mBoostedSkill;

    /* armor properties */
    char mArmorBonus;
    unsigned char mMaxEnhancement;
    signed char mResistances[kMaxEnergyType]; /* resistances granted by this armor */

    int mToHitModifier;
    int mDamageModifier;
    int mSpeedBoost;

    int mPsiModifier;
    int mMaxCharges;

    /* Command hooks. */
    shToolFunc *mUseFunc;
    shCanisterFunc *mQuaffFunc;
    shFloppyDiskFunc *mRunFunc;

    /* association with monster type */
    //shMonId mMonsterIlk;

    /* leftover stuff */
    bool isA (shObjId ilk);
    bool isAbstract (void);
    const char *getRayGunColor ();
    shSkillCode getSkillForAttack (shAttack *attack);
};

struct shStaticDescription {
    shObjectType mType;
    char mName[40];
    shGlyph mGlyph;
};

/* Describes random appearances. */
struct shFlavor
{
    shStaticDescription mAppearance, mVague;
    shMaterialType mMaterial;
    int mCostMod;
    int mWeightMod;
};

const char *describeImplantSite (shObjectIlk::Site site);

enum shTileRow {
    kRowProfession,
    kRowDefault,
    kRowAt,
    kRowDeprecated1,
    kRowDeprecated2,
    kRowLittleA,
    kRowBigA = kRowLittleA + 26,
    kRowHandgun = kRowBigA + 26,
    kRowLightGun,
    kRowHeavyGun,
    kRowUnarmed,
    kRowMelee,
    kRowSword,
    kRowMissile,
    kRowGrenade,
    kRowAmmunition,
    kRowRayGun,
    kRowHelmet,
    kRowGoggles,
    kRowBelt,
    kRowCloak,
    kRowBoots,
    kRowJumpsuit,
    kRowBodyArmor,
    kRowKeycard,
    kRowPowerPlant,
    kRowGizmo,
    kRowComputer,
    kRowGenTool,
    kRowImplant,
    kRowCanister,
    kRowFloppyDisk,
    kRowULabel,   /* [U]nidentified */
    kRowILabel,   /* [I]dentified */
    kRowMoney,
    kRowEnergy,
    kRowWreck,
    kRowTag1,
    kRowCursor,
    kRow4DirAtt,
    kRow8DirAtt,
    kRow0DirAtt,
    kRowSplash,
    kRowBoom,
    kRowBreath,
    kRowNotes1,
    kRowNotes2,
    kRowASCII,
    kRowSpaceBase,
    kRowSewers,
    kRowGammaCaves,
    kRowMainframe,
    kRowReserved1,
    kRowReserved2,
    kRowReserved3,
    kRowReserved4,
    kRowTrap
};

shTileRow wpnRow (shSkillCode skill);
shTileRow mslRow (shObjId parent);
shTileRow armRow (shObjId parent);
shTileRow toolRow (shObjId parent);

/* Probability value for abstract ilks. */
#define ABSTRACT -1

#endif
