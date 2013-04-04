#ifndef ATTACKTYPE_H
#define ATTACKTYPE_H

#include "Attack.h"
#include "Global.h"
#include "ObjectIlk.h"

/* After adding any energy type here remember to add respective
   res_type field to resistances table in Items.txt and Monsters.txt. */
enum shEnergyType {
    kNoEnergy,

    /* The following ignore armor. */
    kIrresistible,   /* What it says on the tin.  Use judiciously. */
    kDisintegrating, /* Antimatter. */
    kPsychic,        /* Psi attacks. */

    /* Armor taken in normal way. */
    kBullet,         /* Guns and cannons. */
    kConcussive,     /* Smashing or bashing. */
    kExplosive,      /* Pure force of explosion. */
    kHackNSlash,     /* Cutting and hacking. */
    kLaser,          /* Many energy weapons. */
    kParticle,       /* For energy guns like blaster. */
    kShrapnel,       /* Shotguns, frag grenades, exploding canisters. */

    /* Armor is only half effective. */
    kBurning,        /* Flamethrowers, napalm. */
    kCorrosive,      /* Acid. */
    kElectrical,     /* Zappers, electric batons. */
    kFreezing,       /* Hardly used, but there. */
    kPlasma,         /* Plasma weapons.  High destructive potential. */

    /* Armor is only one third effective. */
    kMagnetic,       /* Gauss ray gun, plasma weapons side effect. */
    kRadiological,   /* Gamma Caves, rad traps, rad grenades, gamma ray guns. */

    /* Special energies not harming hit points. */
    kAccelerating,   /* Canister of speed. */
    kBlinding,       /* Side effect of laser weapons. */
    kBugging,        /* Decreases item statuses down to buggy. */
    kConfusing,      /* Mental blasts, drinking beer. */
    kDecelerating,
    kDigging,        /* Mining laser. */
    kMesmerizing,    /* Hypnotic gaze, narcolepsy. */
    kParalyzing,     /* No movement at all! */
    kSickening,      /* Suppresses regeneration. */
    kSpecial,        /* For quantifying special effects; does no direct harm. */
    kStunning,       /* Stun grenades, powerful blows. */
    kToxic,          /* Substances harmful to living organisms. */
    kTransporting,   /* Teleport equivalent. */
    kViolating,      /* You get sore butt by getting hit with this. */
    kWebbing,        /* Partial immobilization. */
    kMaxEnergyType
};

enum shAttackFlags {
    kMelee =           0x1,  /* mop, chainsaw */
    kMissile =         0x2,  /* dagger, grenade */
    kAimed =           0x4,  /* blaster, chaingun */
    kTouchAttack =     0x10  /* AC is 10 + bonus from agility */
};

struct shAttack
{
    enum Type {        /* primarily used for descriptive purposes */
        kNoAttack,
        kAccelerationRay,
        kAcidBath,     /* acid trap */
        kAcidSplash,   /* acid blood */
        kAnalProbe,
        kAttach,       /* e.g. attaching a restraining bolt */
        kAugmentationRay,
        kBite,
        kBlast,        /* explosion */
        kBolt,         /* energy bolt, e.g. pea shooter, blaster bolt */
        kBreatheBugs,
        kBreatheFire,
        kBreatheTime,
        kBreatheTraffic,
        kBreatheViruses,
        kClaw,
        kClub,
        kChoke,
        kCombi,        /* combi-stick */
        kCreditDraining, /* creeping credits :-) */
        kCrush,
        kCut,
        kDecelerationRay,
        kDecontaminationRay,
        kDisc,         /* Smart-Disc */
        kDisintegrationRay,
        kDrill,
        kEnsnare,      /* Webbing grenades. */
        kExplode,      /* Attack calls die (kSuicide) shortly after. */
        kExtractBrain, /* mi-go */
        kFaceHug,      /* Facehugger obviously. */
        kFlare,
        kFlash,        /* Radiation grenade. */
        kFreezeRay,
        kGammaRay,
        kGaussRay,
        kGun,
        kHeadButt,
        kHealingRay,
        kHeatRay,
        kImpact,       /* football, improvised thrown weapon */
        kIncendiary,
        kKick,
        kLaserBeam,
        kLight,        /* blinding flash */
        kLegalThreat,
        kLightningBolt,
        kMentalBlast,
        kOpticBlast,
        kPea,          /* Pea shooter.  Like kGun but different animation. */
        kPlague,       /* Defiler vomit. */
        kPlasmaGlob,   /* plasma guns */
        kPoisonRay,
        kPrick,        /* Gom Jabbar needle */
        kPsionicStorm,
        kPunch,
        kQuill,
        kRail,         /* Railgun slug. */
        kRake,         /* Rear claws of catbot. */
        kRestorationRay,
        kSaw,
        kScorch,       /* Burning in melee for NNTP daemon. */
        kShot,         /* Shotgun pellets. */
        kSlash,
        kSlime,
        kSmash,
        kSpear,        /* speargun short spear */
        kSpit,         /* For hydralisk. */
        kStab,
        kStasisRay,
        kSting,
        kStomp,
        kSword,
        kTailSlap,
        kTouch,
        kTransporterRay,
        kTractorBeam,
        kWaterRay,     /* Squirt ray gun. */
        kWeb,
        kWhip,
        kZap,
        kMaxAttackType
    };


    enum Effect {
        kSingle,    /* single target */
        kSmartDisc, /* for Yautja Smart-Discs */
        kCone,      /* a spreading area-effect attack */
        kExtend,    /* a straight line area-effect attack */
        kBeam,      /* as above but with random 1-6 bonus to range */
        kBurst,     /* a disc that doesn't penetrate walls */
        kFarBurst,  /* kBurst centered at target hit by kSingle */
        kOther
    };

    Type mType;
    Effect mEffect;
    int mFlags;
    int mRadius;             /* blast radius in squares */
    int mRange;              /* in squares for beams (+d6 will be added),
                                o/w in feet */
    int mHitMod;             /* difficulty of hitting with this attack */
    int mAttackTime;         /* AP spent recovering after attack */
    struct {
        shEnergyType mEnergy;
        int mLow, mHigh; /* damage ranges */
        int mChance;     /* percentage chance of causing this damage */
    } mDamage[3];

    int bypassesShield ();
    int isSingleAttack () { return kSingle == mEffect; }
    int isAimedAttack () { return kAimed & mFlags; }
    int isMeleeAttack () { return kMelee & mFlags; }
    int isMissileAttack () { return kMissile & mFlags; }
    int isTouchAttack () { return kTouchAttack & mFlags; }
    int isAbsorbedForFree (int damage);
    int isLightGenerating (); /* drawn over dark squares */
    int dealsEnergyType (shEnergyType type);
    int findEnergyType (shEnergyType type);

    int getThreatRange (shObject *weapon, shCreature *target);
    int getCriticalMultiplier (shObject *weapon, shCreature *target);

    const char *noun ();
    const char *damStr ();
    static bool isLethal (shEnergyType);
    static bool isSpecial (shEnergyType e) { return e >= kAccelerating; }
};

const char *energyDescription (shEnergyType t);

extern shAttack Attacks[];
#endif
