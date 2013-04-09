#ifndef GLOBAL_H
#define GLOBAL_H

#define ZAPM_VERSION "0.8.2"
#define PRIME_VERSION "2.2"

#ifdef DJGPP
#include <stdarg.h>
#include <stdio.h>
int snprintf (char *str, size_t size, const char *format, ...);
int vsnprintf (char *str, size_t size, const char *format, va_list ap);
#endif

#include <assert.h>
#ifdef _WIN32
#define DATADIR "user"
#define USERDIR "user"
#define SCOREDIR "user"
#else
#include "config.h"
#endif

typedef signed int shTime;    //ms
#define MAXTIME 99999999 /*so sloppy*/

#define LONGTURN  2000
#define SLOWTURN  1500
#define DIAGTURN  1414
#define FULLTURN  1000
#define QUICKTURN 750
#define HALFTURN  500

struct shInterface;
struct shMenu;
struct shAbilities;
class shCreature;
struct shMonsterIlk;
class shMonster;
struct shObjectIlk;
struct shObject;
class shHero;
class shMapLevel;
struct shFeature;

struct shCoord { char mX; char mY; };

extern shObjectIlk AllIlks[];
extern shMonsterIlk MonIlks[];

enum shObjectType
{
    kUninitialized = 0,
    kMoney,
    kImplant,
    kFloppyDisk,
    kCanister,
    kTool,
    kArmor,
    kWeapon,
    kProjectile,
    kOther,
    kRayGun,
    kEnergyCell,
    kMaxObjectType
};

const char objectTypeHeader[kMaxObjectType][20] =
{
    "UNINITIALIZED",
    "Money",
    "Bionic Implants",
    "Floppy Disks",
    "Canisters",
    "Tools",
    "Armor",
    "Weapons",
    "Ammunition",
    "Other",
    "Ray Guns",
    "Energy Cells"
};

/* Game options. */
struct shFlags {
    //float mDelayMult;
    char mKeySet[40];
    int mFadeLog;
    int mShowLOS;
    int mAutopickup;
    int mAutopickupTypes[kMaxObjectType];
};

enum shDirection {
    kNorth = 0,
    kNorthEast = 1,
    kEast = 2,
    kSouthEast = 3,
    kSouth = 4,
    kSouthWest = 5,
    kWest = 6,
    kNorthWest = 7,
    kUp = 8,
    kDown = 9,
    kOrigin = 10,
    kNoDirection = 11
};

extern shTime Clock;         // global clock
extern shTime MonsterClock;


enum shColor {
    kBlack = 0,
    kBlue,
    kGreen,
    kCyan,
    kRed,
    kMagenta,
    kBrown,
    kGray,
    kDarkGray,       /* unused color */
    kNavy,           /* bright blue */
    kLime,           /* bright green */
    kAqua,           /* bright cyan */
    kOrange,         /* bright red */
    kPink,           /* bright magenta */
    kYellow,
    kWhite
};

struct shGlyph {
    char mSym;
    shColor mColor, mBkgd;
    int mTileX, mTileY;
};


enum shSkillCode {
    kNoSkillCode =        0x0000,

    kSkillAbilityMask =   0x0f00,

    kWeaponSkill =        0x0010,
    kAdventureSkill =     0x0020,

    kStrSkill =           0x0100, /* Must be kStr << 2 */
    kUnarmedCombat =      0x0111,
    kHeavyGun =           0x0112,

    kConSkill =           0x0200, /* Must be kCon << 2 */
    kConcentration =      0x0221,

    kAgiSkill =           0x0300, /* Must be kAgi << 2 */
    kMeleeWeapon =        0x0310,
    kSword =              0x0311,

    kDexSkill =           0x0400, /* Must be kDex << 2 */
    kOpenLock =           0x0421,
    kRepair =             0x0423,

    kGrenade =            0x0410,
    kHandgun =            0x0411,
    kLightGun =           0x0412,

    kIntSkill =           0x0500, /* Must be kInt << 2 */
    kSearch =             0x0525,
    kHacking =            0x0526,
    kSpot =               0x0527,

    kPsiSkill =           0x0600, /* Must be kPsi << 2 */

    kMutantPower =        0x1601,

    kUninitializedSkill = 0xffff
};


enum shMutantPower {
    kNoMutantPower = 0x0,
    kIllumination,
    kDigestion,
    kHypnosis,
    kRegeneration,
    kOpticBlast,
    kHaste,
    kTelepathyPower,
    kTremorsensePower,
    kShootWebs,
    kMentalBlast,
    kRestoration,
    kAdrenalineControl,
    kXRayVisionPower,
    kTheVoice,
    kTeleport,
    kAutolysis,
    kPsionicStorm,
    kMaxHeroPower = kPsionicStorm,
/* monster only powers*/
    kHallucinate,
    kTerrify,
    kDarkness,
    kMaxMutantPower = kDarkness,
    kCeaseAndDesist, /* lawyer first */
    kSeizeEvidence,
    kSueForDamages,
    kSummonWitness,  /* lawyer last */
    kLaunchMissile,  /* rocket turret */
    kMaxAllPower
};


const char *getMutantPowerName (shMutantPower id);

#include "Util.h"

void exitPRIME (const int code); /* defined in Game.cpp */

extern shHero Hero;
extern shMapLevel *Level;  /* points to the current level */
extern shVector <shMapLevel *> Maze;  /* all the levels */

extern shFlags Flags;


#ifndef OSX
#ifndef DATADIR
#define DATADIR "user"
#define USERDIR "user"
#define SCOREDIR "user"
#endif
#endif

#define HERO_NAME_LENGTH 14
#endif
