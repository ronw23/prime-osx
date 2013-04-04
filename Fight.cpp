/**************************************************
* This file is for weapon attack resolution code. *
**************************************************/

#ifndef _WIN32
#include <unistd.h>
#endif

#include "Global.h"
#include "Util.h"
#include "Hero.h"
#include "Game.h"
#include "ObjectType.h"
#include "Interface.h"

static const char *
beamHitsMesg (shAttack *atk)
{
    switch (atk->mType) {
        case shAttack::kNoAttack:
        case shAttack::kAccelerationRay:
        case shAttack::kAcidSplash:
        case shAttack::kAugmentationRay:
        case shAttack::kBlast:
        case shAttack::kCombi:
        case shAttack::kDecelerationRay:
        case shAttack::kDecontaminationRay:
        case shAttack::kEnsnare:
        case shAttack::kExplode:
        case shAttack::kFlash:
        case shAttack::kGammaRay:
        case shAttack::kGaussRay:
        case shAttack::kHealingRay:
        case shAttack::kIncendiary:
        case shAttack::kRestorationRay:
        case shAttack::kTransporterRay:
            return NULL;
        case shAttack::kBreatheFire: return "The fire engulfs";
        case shAttack::kBreatheBugs: return "The cloud of bugs envelopes";
        case shAttack::kBreatheViruses: return "The cloud of viruses envelopes";
        case shAttack::kBreatheTime: return "The time warp envelopes";
        case shAttack::kBreatheTraffic: return "The packet storm envelopes";
        //case shAttack::kCombi: return "The spear pierces";
        case shAttack::kDisintegrationRay: return "The disintegration ray hits";
        case shAttack::kFreezeRay: return "The freeze ray hits";
        case shAttack::kHeatRay: return "The heat ray hits";
        case shAttack::kLaserBeam:
        case shAttack::kOpticBlast:
            return "The laser beam hits";
        case shAttack::kLight: return "The blinding light affects";
        case shAttack::kMentalBlast: return "The mental blast hits";
        case shAttack::kPlague: return "The vomit hits";
        case shAttack::kPlasmaGlob: return "The plasma glob hits";
        case shAttack::kPoisonRay: return "The poison ray hits";
        case shAttack::kSpit: return "The green spit hits";
        case shAttack::kStasisRay: return "The stasis ray hits";
        case shAttack::kWaterRay: return "Water douses";
        default: return "It hits";
    }
}


static const char *
beamHitsBlindYouMesg (shAttack *atk)
{
    switch (atk->mType) {
        case shAttack::kNoAttack:
        case shAttack::kAccelerationRay:
        case shAttack::kAugmentationRay:
        case shAttack::kBlast:
        case shAttack::kDecelerationRay:
        case shAttack::kDecontaminationRay:
        case shAttack::kFlash:
        case shAttack::kLight:
        case shAttack::kGammaRay:
        case shAttack::kGaussRay:
        case shAttack::kStasisRay:
        case shAttack::kTransporterRay:
        case shAttack::kHealingRay:
        case shAttack::kRestorationRay:
            return NULL;
        case shAttack::kBreatheTime: return "You are eveloped by a time warp!";
        case shAttack::kBreatheTraffic: return "You are enveloped by a packet storm !";
        case shAttack::kBreatheBugs:
        case shAttack::kBreatheViruses:
            return "You are enveloped by a dense cloud!";
        case shAttack::kCombi: return "You are punctured!";
        case shAttack::kDisintegrationRay: return "You are caught in a disintegration zone!";
        case shAttack::kFreezeRay: return "You are frozen!";
        case shAttack::kBreatheFire:
        case shAttack::kHeatRay:
        case shAttack::kIncendiary:
        case shAttack::kPlasmaGlob:
            return "You are burned!";
        case shAttack::kMentalBlast: return "A mental blast hits you!";
        case shAttack::kPoisonRay: return "You feel poisoned!";
        case shAttack::kPsionicStorm: return "Something wrecks your mind!";
        case shAttack::kWaterRay: return "You are doused in water.";
        case shAttack::kPlague:
        case shAttack::kSpit:
            return "Something splashes against you!";
        default: return "You are hit!";
    }
}


static shSpecialEffect
beamSpecialEffect (shAttack *attack, shDirection dir, bool head = true)
{
    switch (attack->mType) {
        case shAttack::kSpit:
            return kSpitEffect;
        case shAttack::kBlast:  /* Check primary damage. */
            switch (attack->mDamage[0].mEnergy) {
                case kBurning: return kIncendiaryEffect;
                case kDisintegrating: return kDisintegrationEffect;
                case kFreezing: return kColdEffect;
                case kMagnetic: return kNone;
                case kRadiological: return kRadiationEffect;
                case kShrapnel: return kShrapnelEffect;
                case kToxic: return kPoisonEffect;
                case kWebbing: return kNone;
                default: return kExplosionEffect;
            }
        case shAttack::kExplode: /* Rossak sorceress. */
            if (attack->mDamage[0].mEnergy == kPsychic)
                return kPsionicStormEffect;
            return kExplosionEffect;
        case shAttack::kFlash:
            switch (attack->mDamage[0].mEnergy) {
                case kBlinding: return kBlindingEffect;
                case kRadiological: return kRadiationEffect;
                default: return kExplosionEffect;
            }
        case shAttack::kDisc:
            return kDiscEffect;
        case shAttack::kPlasmaGlob:
            return dir == kOrigin ? kPlasmaHitEffect : kPlasmaEffect;
        case shAttack::kFreezeRay:
            return kColdEffect;
        case shAttack::kPlague:
            //return kVomitEffect; fall through instead
            //FIXME: There is nice glyph painted by Psiweapon.  Use it!
        case shAttack::kPoisonRay:
            return kPoisonEffect;
        case shAttack::kLight:
            return kBlindingEffect;
        case shAttack::kIncendiary:
        case shAttack::kHeatRay:
        case shAttack::kBreatheFire:
            return kIncendiaryEffect;
        case shAttack::kPsionicStorm:
            return kPsionicStormEffect;
        case shAttack::kDisintegrationRay:
            return kDisintegrationEffect;
        case shAttack::kBreatheTraffic:
            return kBinaryEffect;
        case shAttack::kBreatheBugs:
            return kBugsEffect;
        case shAttack::kBreatheViruses:
            return kVirusesEffect;
        case shAttack::kWaterRay:
            return dir == kOrigin ? kWaterEffect : kNone;
        case shAttack::kAcidSplash:
            return kAcidEffect;
        case shAttack::kLaserBeam:
            switch (dir) {
                case kNorthWest: case kSouthEast: return kLaserBeamBDiagEffect;
                case kSouthWest: case kNorthEast: return kLaserBeamFDiagEffect;
                case kEast: case kWest: return kLaserBeamHorizEffect;
                case kNorth: case kSouth: return kLaserBeamVertEffect;
                case kOrigin: default: return kLaserBeamEffect;
            }
        case shAttack::kOpticBlast:
            switch (dir) {
                case kNorthWest: case kSouthEast: return kOpticBlastBDiagEffect;
                case kSouthWest: case kNorthEast: return kOpticBlastFDiagEffect;
                case kEast: case kWest: return kOpticBlastHorizEffect;
                case kNorth: case kSouth: return kOpticBlastVertEffect;
                case kOrigin: default: return kOpticBlastEffect;
            }
        case shAttack::kBolt:
            switch (dir) {
                case kNorthWest: case kSouthEast: return kBoltBDiagEffect;
                case kSouthWest: case kNorthEast: return kBoltFDiagEffect;
                case kEast: case kWest: return kBoltHorizEffect;
                case kNorth: case kSouth: return kBoltVertEffect;
                case kOrigin: default: return kBoltEffect;
            }
        case shAttack::kCombi:
            if (head)  switch (dir) {
                case kNorthWest: return kCombiNorthWestEffect;
                case kSouthEast: return kCombiSouthEastEffect;
                case kSouthWest: return kCombiSouthWestEffect;
                case kNorthEast: return kCombiNorthEastEffect;
                case kEast: return kCombiEastEffect;
                case kWest: return kCombiWestEffect;
                case kNorth: return kCombiNorthEffect;
                case kSouth: return kCombiSouthEffect;
                case kOrigin: default: return kNone;
            } else  switch (dir) {
                case kNorthWest: case kSouthEast: return kCombiBDiagEffect;
                case kSouthWest: case kNorthEast: return kCombiFDiagEffect;
                case kEast: case kWest: return kCombiHorizEffect;
                case kNorth: case kSouth: return kCombiVertEffect;
                case kOrigin: default: return kNone;
            }
        case shAttack::kPea:
            return kPeaEffect;
        case shAttack::kSpear: /* Speargun effct might look like railgun in
                    ASCII, but when tiles get drawn it should get separate. */
        case shAttack::kRail:
            switch (dir) {
                case kNorthWest: return kRailNorthWestEffect;
                case kSouthEast: return kRailSouthEastEffect;
                case kSouthWest: return kRailSouthWestEffect;
                case kNorthEast: return kRailNorthEastEffect;
                case kEast: return kRailEastEffect;
                case kWest: return kRailWestEffect;
                case kNorth: return kRailNorthEffect;
                case kSouth: return kRailSouthEffect;
                case kOrigin: default: return kRailEffect;
            }
        default:
            return kNone;
    }
}


const char *
youHitMonMesg (shAttack::Type type)
{
    switch (type) {
        case shAttack::kNoAttack: return " don't attack";
        case shAttack::kAnalProbe: return " probe";
        case shAttack::kBite: return " bite";
        case shAttack::kBlast: return " blast";
        case shAttack::kClaw: return " claw";
        case shAttack::kClub: return " club";
        case shAttack::kChoke: return " choke";
        case shAttack::kCombi: return " puncture";
        case shAttack::kCrush: return " crush";
        case shAttack::kCut: return " cut";
        case shAttack::kDrill: return " drill";
        case shAttack::kHeadButt: return " head butt";
        case shAttack::kImpact: return " smack";
        case shAttack::kKick: return " kick";
        case shAttack::kPlague: return " vomit at";
        case shAttack::kPrick: return " prick";
        case shAttack::kPunch: return " punch";
        case shAttack::kRake: return " rake";
        case shAttack::kSaw: return " saw";
        case shAttack::kScorch: return " scorch";
        case shAttack::kSlash: return " slash";
        case shAttack::kSlime: return " slime";
        case shAttack::kSmash: return " smack";
        case shAttack::kStab: return " stab";
        case shAttack::kSting: return " sting";
        case shAttack::kStomp: return " stomp";
        case shAttack::kSword: return " slice";
        case shAttack::kTailSlap: return " tail-slap";
        case shAttack::kTouch: return " touch";
        case shAttack::kWhip: return " whip";
        case shAttack::kZap: return " zap";
        default: return " think deadly thoughts about";
    }
}


const char *
youShootMonMesg (shAttack::Type type)
{
    switch (type) {
        case shAttack::kWaterRay: return "splash";
        default: return "hit";
    }
}


static const char *
monHitsYouMesg (shAttack *atk)
{
    switch (atk->mType) {
        case shAttack::kNoAttack: return " doesn't attack";
        case shAttack::kAttach: return " attaches an object to you";
        case shAttack::kAnalProbe: return " probes you";
        case shAttack::kBite: return " bites you";
        case shAttack::kBlast: return " blasts you";
        case shAttack::kBolt: return " blasts you";
        case shAttack::kClaw: return " claws you";
        case shAttack::kClub: return " clubs you";
        case shAttack::kChoke: return " chokes you";
        case shAttack::kCrush: return " crushes you";
        case shAttack::kCut: return " cuts you";
        case shAttack::kDisintegrationRay: return " zaps you with a disintegration ray";
        case shAttack::kDrill: return " drills you";
        case shAttack::kExtractBrain: return NULL;
        case shAttack::kFaceHug: return " attacks your face";
        case shAttack::kFlash: return " blasts you with a bright light";
        case shAttack::kFreezeRay: return " zaps you with a freeze ray";
        case shAttack::kGammaRay: return " zaps you with a gamma ray";
        case shAttack::kGaussRay: return " zaps you with a gauss ray";
        case shAttack::kGun:
        case shAttack::kPea:
        case shAttack::kShot:
            return " shoots you";
        case shAttack::kHeadButt: return " head butts you";
        case shAttack::kHealingRay: return " zaps you with a healing ray";
        case shAttack::kHeatRay: return " zaps you with a heat ray";
        case shAttack::kIncendiary: return " sets you on fire";
        case shAttack::kKick: return " kicks you";
        case shAttack::kLaserBeam:
        case shAttack::kOpticBlast:
            return " zaps you with a laser beam";
        case shAttack::kLight: return " flashes you with blinding light";
        case shAttack::kLegalThreat: return " raises an objection";
        case shAttack::kMentalBlast: return " blasts your mind";
        case shAttack::kPlague: return NULL;
        case shAttack::kPoisonRay: return "zaps you with a poison ray";
        case shAttack::kPrick: return " pricks you";
        case shAttack::kPunch: return " punches you";
        case shAttack::kQuill: return " sticks you with a quill";
        case shAttack::kRail: return " rails you";
        case shAttack::kRake: return " rakes you";
        case shAttack::kRestorationRay: return " zaps you with a restoration ray";
        case shAttack::kSaw: return " saws you";
        case shAttack::kScorch: return " scorches you";
        case shAttack::kSlash: return " slashes you";
        case shAttack::kSlime: return " slimes you";
        case shAttack::kSmash: return " smacks you";
        case shAttack::kStab: return " stabs you";
        case shAttack::kStasisRay: return " zaps you with a stasis ray";
        case shAttack::kSting: return " stings you";
        case shAttack::kStomp: return " stomps you";
        case shAttack::kSword: return " slices you";
        case shAttack::kTailSlap: return "'s tail whips you";
        case shAttack::kTouch: return " touches you";
        case shAttack::kTransporterRay: return " zaps you with a transporter ray";
        case shAttack::kWhip: return " whips you";
        case shAttack::kZap: return " zaps you";
        default: return " hits you";
    }
}


static const char *
monHitsMonMesg (shAttack *atk)
{
    switch (atk->mType) {
        case shAttack::kNoAttack: return " doesn't attack";
        case shAttack::kAnalProbe: return " probes";
        case shAttack::kAttach: return " attaches an object to";
        case shAttack::kBite: return " bites";
        case shAttack::kBlast: return " blasts";
        case shAttack::kBolt: return " blasts";
        case shAttack::kClaw: return " claws";
        case shAttack::kClub: return " clubs";
        case shAttack::kChoke: return " chokes";
        case shAttack::kCrush: return " crushes";
        case shAttack::kCut: return " cuts";
        case shAttack::kDisintegrationRay: return " zaps a disintegration ray at";
        case shAttack::kDrill: return " drills";
        case shAttack::kExtractBrain: return " extracts the brain of";
        case shAttack::kFaceHug: return " attacks the face of";
        case shAttack::kFlash: return " flashes a bright light at";
        case shAttack::kFreezeRay: return " zaps a freeze ray at";
        case shAttack::kGammaRay: return " zaps a gamma ray at";
        case shAttack::kGaussRay: return " zaps a gauss ray at";
        case shAttack::kGun:
        case shAttack::kPea:
        case shAttack::kShot:
            return " shoots";
        case shAttack::kHeadButt: return " head butts";
        case shAttack::kHealingRay: return " zaps a healing ray at";
        case shAttack::kIncendiary: return " sets fire to";
        case shAttack::kHeatRay: return " zaps a heat ray at";
        case shAttack::kKick: return " kicks";
        case shAttack::kLaserBeam:
        case shAttack::kOpticBlast:
            return " zaps a laser beam at";
        case shAttack::kLight: return " emits blinding light at";
        case shAttack::kMentalBlast: return " blasts the mind of";
        case shAttack::kPlague: return " vomits at";
        case shAttack::kPoisonRay: return " zaps a poison ray at";
        case shAttack::kPrick: return " pricks";
        case shAttack::kPunch: return " punches";
        case shAttack::kQuill: return " quills";
        case shAttack::kRail: return " rails";
        case shAttack::kRake: return " rakes";
        case shAttack::kRestorationRay: return " zaps a restoration ray at";
        case shAttack::kSaw: return " saws";
        case shAttack::kScorch: return " scorches";
        case shAttack::kSlash: return " slashes";
        case shAttack::kSlime: return " slimes";
        case shAttack::kSmash: return " smacks";
        case shAttack::kSpit: return " spits";
        case shAttack::kStab: return " stabs";
        case shAttack::kStasisRay: return " zaps a stasis ray at";
        case shAttack::kSting: return " stings";
        case shAttack::kStomp: return " stomps";
        case shAttack::kSword: return " slices";
        case shAttack::kTailSlap: return "'s tail whips";
        case shAttack::kTouch: return " touches";
        case shAttack::kTransporterRay: return " zaps a transporter ray at";
        case shAttack::kWhip: return " whips";
        case shAttack::kZap: return " zaps";
        default: return " hits";
    }
}


static shDirection
reflectBackDir (shDirection dir) {
    dir = uTurn (dir);
    switch (RNG (4)) {
    case 0: return leftTurn (dir);
    case 1: return rightTurn (dir);
    default: return dir;
    }
}


shSkillCode
shObjectIlk::getSkillForAttack (shAttack *attack)
{
    if (attack->isMissileAttack ()) {
        return kGrenade;
    } else if (attack->isMeleeAttack ()) {
        return mMeleeSkill;
    } else if (attack->isAimedAttack ()) {
        return mGunSkill;
    } else {
        return kNoSkillCode;
    }
}

/* roll to save against the attack
   returns: 0 if save failed, true if made */
int
shCreature::reflexSave (shAttack *attack, int DC)
{
    int result = RNG (1, 20) + mReflexSaveBonus + ABILITY_MODIFIER (getAgi ());

    if (is (kAsleep) or isSessile ()) {
        return 0;
    }
    if (Hero.isLucky ()) {
        if (isHero () or isPet ()) {
            result += RNG (1, 7);
        } else {
            result -= RNG (1, 7);
        }
    }

    return result >= DC;
}

int
shCreature::misfire (shObject *weapon, shAttack *attack)
{
    if (attack->mEffect == shAttack::kExtend) {
        if (isHero ()) {
            weapon->setBugginessKnown ();
            I->p ("Your weapon jams!");
        } else if (Hero.canSee (this)) {
            I->p ("%s's weapon jams!", the ());
            weapon->setBugginessKnown ();
        }
        return attack->mAttackTime;
    } else if (weapon->isA (kRayGun) and RNG (3)) {
        int died;
        if (isHero ()) {
            I->p ("Your ray gun explodes!");
        } else if (Hero.canSee (this)) {
            I->p ("%s's ray gun explodes!", the ());
        } else {
            I->p ("You hear an explosion");
        }
        /* For all weapons that may explode, these caveats apply:
           1. One needs to remove the weapon from inventory first, so no
              risk of double-deletion due to it somehow getting destroyed
              by a secondary explosion effect.
           2. Deletion must be done after any potential clerkbot has had
              a look at to determine if it was unpaid.
           3. Remember to return -2 if the attacker is killed in the
              explosion, because higher level code will handle deletion.

           For blowing up ray guns the following is done:
              Kludgily borrow and modify the weapon's own attack structure
              to save typing in new exploding ray gun attacks.
        */
        removeObjectFromInventory (weapon); /* Handles unwielding. */
        /* Turn the thing into an explosion. */
        shAttack::Type saved_type = attack->mType;
        attack->mType = shAttack::kBlast;
        shAttack::Effect saved_effect = attack->mEffect;
        attack->mEffect = shAttack::kBurst;
        died = mLevel->areaEffect (attack, NULL, mX, mY, kNoDirection, this, 0);
        /* All right, repair the attack. */
        attack->mType = saved_type;
        attack->mEffect = saved_effect;
        if (isHero () and mState != kDead)
            Hero.usedUpItem (weapon, 1, "destroy");
        delete weapon;
        return died ? -2 : attack->mAttackTime;
    }
    /* Weapon misfire wastes ammo and turn. */
    if (weapon->vulnerability () == kCorrosive and
        weapon->mDamage == 3 and RNG (2))
    {
        if (weapon->myIlk ()->mAmmoType == kObjEnergyCell) {
            /* Thoroughly corroded energy weapons may explode. */
            if (isHero ()) {
                I->p ("Your weapon misfires and explodes!");
            } else if (Hero.canSee (this)) {
                I->p ("%s's weapon misfires and explodes!", the ());
                weapon->setBugginessKnown ();
            } else {
                I->p ("You hear a weapon misfire.");
            }
            removeObjectFromInventory (weapon);
            int died = mLevel->areaEffect (kAttConcussionGrenade, NULL,
                 mX, mY, kNoDirection, this, 0);
            if (isHero () and mState != kDead)
                Hero.usedUpItem (weapon, 1, "destroy");
            delete weapon;
            return died ? -2 : attack->mAttackTime;
        } else {
            /* Thoroughly corroded conventional weapons may fall apart. */
            if (isHero ()) {
                I->p ("Your weapon misfires and falls apart!");
            } else if (Hero.canSee (this)) {
                I->p ("%s's weapon misfires and falls apart!", the ());
            } else {
                I->p ("You hear a weapon misfire.");
            }
            /* Since a weapon falling apart has no side effects one can
               just delete it after clerkbot has complained about it. */
            removeObjectFromInventory (weapon);
            if (isHero ())  Hero.usedUpItem (weapon, 1, "destroy");
            delete weapon;
            return attack->mAttackTime;
        }
    } else { /* Usual misfire. */
        if (isHero ()) {
            weapon->setBugginessKnown ();
            I->p ("Your weapon misfires!");
        } else if (Hero.canSee (this)) {
            I->p ("%s's weapon misfires!", the ());
            weapon->setBugginessKnown ();
        } else {
            I->p ("You hear a weapon misfire.");
        }
    }
    return attack->mAttackTime;
}

/* roll to hit the target
   returns: -x for a miss (returns the amount missed by)
            1 for a hit
            2+ for a critical (returns the appropriate damage multiplier)
*/

int
shCreature::attackRoll (shAttack *attack, shObject *weapon,
                        int attackmod, int AC, shCreature *target)
{
    int result = RNG (1, 20);
    int dmul = 1;
    int threatrange = weapon ? weapon->getThreatRange (target) : 20;
    int critmult = weapon ? weapon->getCriticalMultiplier () : 2;
    const char *thetarget = target->the ();

    /* Rolling a 1 always results in a miss unless you shot yourself. */
    if (1 == result and this != target) {
        I->diag ("attacking %s: rolled a 1 against AC %d.", thetarget, AC);
        return -99;
    }
    if (20 == result or result + attackmod >= AC) { /* Hit. */
        if ((!target->isImmuneToCriticalHits ()) and
            (result >= threatrange))
        {   /* critical threat */
            int threat = RNG (1, 20);
            if ((1 != threat) and
                ((20 == threat) or (threat + attackmod >= AC)))
            {   /* critical hit */
                dmul = critmult;
                I->diag ("attacking %s: rolled %d, then %d+%d=%d against "
                         "AC %d: critical hit, dmg mult %d!",
                         thetarget, result, threat, attackmod,
                         threat + attackmod, AC, dmul);
            } else {
                I->diag ("attacking %s: rolled %d, then %d+%d=%d against AC %d",
                         thetarget, result, threat, attackmod,
                         threat + attackmod, AC);
            }
        } else {
            I->diag ("attacking %s: rolled a %d+%d=%d against AC %d",
                     thetarget, result, attackmod, result + attackmod, AC);
        } /* Skilled users can learn weapon enhancement sometimes. */
        if (20 == result and isHero () and weapon and weapon->isA (kWeapon) and
            !weapon->isEnhancementKnown ())
        {
            shSkillCode s = weapon->myIlk ()->getSkillForAttack (attack);
            if (s != kNoSkillCode) {
                int skill = getSkill (s) -> mRanks;
                if (RNG (20) < skill) {
                    weapon->setEnhancementKnown ();
                    I->p ("You are using %s.", THE (weapon));
                    I->pause ();
                }
            }
        }
        return dmul;
    }
    I->diag ("attacking %s: rolled a %d+%d=%d against AC %d",
             thetarget, result, attackmod, result + attackmod, AC);
    return result + attackmod - AC;
}


/* determine if a ranged attack hits.
   returns: -x for a miss (returns amount missed by)
             1 for a hit
             2+ for a critical hit (returns damage multiplier)
   modifies: adds calculated damage bonuses to *dbonus
 */


int
shCreature::rangedAttackHits (shAttack *attack,
                              shObject *weapon,
                              int attackmod,
                              shCreature *target,
                              int *dbonus)
{
    int AC;
    int flatfooted = 0;   /* set to 1 if the target is unable to react to the
                             attack due to unawareness or other disability */
    int range = distance (target, this);
    int wrange = attack->mRange;
    int maxrange;

    *dbonus += mDamageModifier;

    if (weapon) {
        *dbonus += weapon->mEnhancement;
        attackmod += weapon->myIlk ()->mToHitModifier;
    }

    if (!attack->isMeleeAttack () and
        target->hasShield () and
        target->countEnergy ())
    {   /* almost always hits the targets shield and the target gets no
           benefit from concealment */
        attackmod += 100;
    }

    canSee (target);

    if (attack->isMissileAttack ()) {
        maxrange = 10 * wrange; /* for thrown weapons, it's actually 2d4
                                   squares.  see throwObject() for details */
        if (weapon) {
            attackmod += getWeaponSkillModifier (weapon->myIlk (), attack);
            if (weapon->myIlk ()->mMissileAttack) {
                attackmod += weapon->mEnhancement;
                attackmod -= 2 * weapon->mDamage;
            } else { /* improvised missile weapon */
            //FIXME: assess stiff penalties for large or impractical missiles
            }
        } /* Else a mutant power generating missiles or some such. */
    } else {
        maxrange = 10 * wrange;
        if (!weapon) {
            /* psionic or innate attack - caller should have already
               added relevant skill bonus to attackmod */
        } else if (weapon->isA (kRayGun)) {
            attackmod += mToHitModifier;
            attackmod += 4; /* ray guns are normally pretty reliable */
            maxrange = 99999;      /* range checking handled by beam code */
        } else {
            attackmod += getWeaponSkillModifier (weapon->myIlk (), attack);
            attackmod += weapon->mEnhancement;
            attackmod -= 2 * weapon->mDamage;
        }
        if (weapon and weapon->isOptimized ()) {
            attackmod += 2;
        }
    }

    if (0 == target->canSee (this)) {
        attackmod += 2;
        flatfooted = 1;
    }
    if (target->is (kStunned)) {
        flatfooted = 1;
        attackmod += 2;
    }
    if (target->is (kAsleep) or target->is (kParalyzed) or
        target->isSessile ())
    {
        flatfooted = 1;
        attackmod += 4;
    }

    if (range > maxrange) {
        return -20;
    }

    if (wrange) {
        if (shAttack::kShot == attack->mType) {
            attackmod += 2 * (range / wrange);
            *dbonus -= 4 * (range / wrange);
        } else {
            attackmod -= 2 * (range / 30);
        }
    } else if (target == this) { /* Throwing at self? */
        attackmod = 100;
    }

    if (attack->isTouchAttack ()) {
        AC = target->getTouchAC (flatfooted);
    } else {
        AC = target->getAC (flatfooted);
    }

    /* roll to hit */
    return attackRoll (attack, weapon, attackmod, AC, target);
}


/* works out the ranged attack against the target
   returns: 1 if the target is eliminated (e.g. it dies, teleports away, etc.)
            0 if the target is hit but not eliminated
           -1 if the attack was a complete miss
           -2 if the attack was reflected
*/

int
shCreature::resolveRangedAttack (shAttack *attack,
                                 shObject *weapon,
                                 int attackmod,
                                 shCreature *target,
                                 shObject *stack)
{
    const char *n_target;
    const char *n_weapon;

    int dbonus = 0;
    int dmul = -1;

    int cantsee = 1;

    if (target->isHero ()) {
        n_target = "you";
        cantsee = 0;
    } else if (Hero.canSee (target) or Hero.canHearThoughts (target)) {
        n_target = target->the ();
        cantsee = 0;
    } else {
        n_target = "something";
    }

    if (weapon) {
        n_weapon = weapon->theQuick ();
    } else if (&Attacks[kAttOpticBlast] == attack) {
        if (isHero ())
            n_weapon = "your laser beam";
        else
            n_weapon = "the laser blast";
    } else {
        n_weapon = "it";
    }

    /* work out hit/miss */
    dmul = rangedAttackHits (attack, weapon, attackmod, target, &dbonus);

    if (dmul <= 0) {
        //FIXME: determine if the attack hit armor or just plain missed
        dmul = -1;
        goto youmiss;
    }

    /* successful hit */
    if (target->mHidden) {
        target->revealSelf ();
    }

    if (attack->mEffect == shAttack::kFarBurst) {
        Level->areaEffect (attack, weapon, target->mX, target->mY, kOrigin,
            this, weapon->mEnhancement);
        return target->mState == kDead;
    }

    if (attack->isMissileAttack () and weapon and !weapon->isA (kObjSmartDisc)) {
        weapon->impact (target,
                        vectorDirection (mX, mY, target->mX, target->mY),
                        this, stack);
        return target->mState == kDead;
    }

    if (isHero () and !target->isHero ()) {
        /* SufferDamage will handle message and getting angry. */
        if (target->sufferDamage (attack, this, weapon, dbonus, dmul)) {
            target->die (kSlain, this, weapon, attack);
            return 1;
        }
        return 0;
    } else if (target->isHero ()) {
        if (target->sufferDamage (attack, this, weapon, dbonus, dmul)) {
            shCauseOfDeath reason;
            if (attack->dealsEnergyType (kDisintegrating)) {
                reason = kAnnihilated;
            } else if (isHero ()) {
                reason = kSuicide;
            } else {
                reason = kSlain;
            }
            target->die (reason, this, weapon, attack);
            return 1;
        }
        target->interrupt ();
        return 0;
    } else {
        if (target->sufferDamage (attack, this, weapon, dbonus, dmul)) {
            shCauseOfDeath reason;
            if (attack->dealsEnergyType (kDisintegrating)) {
                reason = kAnnihilated;
            } else {
                reason = kSlain;
            }
            if (!cantsee)
                target->pDeathMessage (n_target, reason);
            target->die (reason, this, weapon, attack);
            return 1;
        } else if (dmul > 1) {
            if (!cantsee) {
                I->p ("%s is hit!", n_target);
            }
        } else {
            if (!cantsee) {
                I->p ("%s is hit.", n_target);
            }
        }
        if (isPet ()) {
            /* monsters will tolerate friendly-fire */
            target->newEnemy (this);
        }
        target->interrupt ();
        return 0;
    }

 youmiss:
    if (attack->isMissileAttack ()) {
        if (target->isHero ()) {
            if (!Hero.isBlind ())
                I->p ("%s misses you!", n_weapon);
            else
                I->p ("Something flies past you!");
        } else {
            I->p ("%s misses %s.", n_weapon, n_target);
        }
    } else if (&Attacks[kAttOpticBlast] == attack) {
        ;   /* No message because the ray itself is invisible. */
    } else {
        if (kNone == beamSpecialEffect (attack, kOrigin)) {
            if (isHero () and target->mHidden <= 0 and
                canSee (target) and Hero.canHearThoughts (target)) {
                I->p ("You miss %s.", n_target);
            } /* else if (target->isHero ()) {
                I->p ("%s misses.", n_attacker);
            } else {
                I->p ("%s misses %s", n_attacker, n_target);
            } */
        }

    }
    target->interrupt ();
    target->newEnemy (this);

    return -1;
}

/*  Work out the result of firing the weapon in the given direction.
    Can be called with weapon == NULL for special monster ranged attacks.
    returns: ms elapsed, -2 if attacker dies
*/
int
shCreature::shootWeapon (shObject *weapon, shDirection dir, shAttack *attack)
{
    int numrounds = 0;
    int x, y;
    int firsttarget = 1;
    int setemptyraygun = 0;
    int basehitmod = 0;

    Hero.interrupt ();

    if (!weapon) {
        numrounds = 1; /* Only single round for monster attacks. */
        basehitmod = mCLevel * 2;
    } else {
        attack = NULL;
        if (weapon->isA (kObjGenericComputer)) { /* Control shoulder cannon. */
            if (weapon->mEnhancement == -5) { /* No OS, no shooting. */
                if (isHero ()) {
                    I->p ("%s does not respond.", YOUR (weapon));
                }
                weapon->setEnhancementKnown ();
                return HALFTURN;
            }
            basehitmod -= mCloak->myIlk ()->mToHitModifier; /* Negate recoil. */
            basehitmod += weapon->mEnhancement; /* OS modifier. */
            weapon = mCloak; /* Consider only the plasma caster from now on. */
        }
        if (weapon->isA (kObjPlasmaCaster)) {
            if (weapon->mCharges) {
                attack = &Attacks[weapon->myIlk ()->mZapAttack];
                attack->mRadius = weapon->mCharges;
                weapon->mCharges = 0;
                numrounds = 1;
            } else {
                attack = &Attacks[weapon->myIlk ()->mGunAttack];
                numrounds = expendAmmo (weapon);
            }
        } else if (weapon->isA (kWeapon)) {
            attack = &Attacks[weapon->myIlk ()->mGunAttack];
            numrounds = expendAmmo (weapon);
        } else if (weapon->isA (kRayGun)) {
            attack = &Attacks[weapon->myIlk ()->mZapAttack];
            if (weapon->mCharges) {
                numrounds = 1;
                --weapon->mCharges;
                if (!weapon->mCharges and weapon->isChargeKnown ()) {
                    ++setemptyraygun;
                }
            } else {
                numrounds = 0;
            }
        }

        if (0 == numrounds) {
            int elapsed = FULLTURN;
            if (isHero ()) {
                if (weapon->isA (kRayGun)) {
                    weapon->mIlkId = kObjEmptyRayGun;
                    weapon->setChargeKnown ();
                    weapon->setKnown ();
                    weapon->setAppearanceKnown ();
                    I->p ("Nothing happens.");
                    I->p ("%c - %s", weapon->mLetter, weapon->inv ());
                } else {
                    I->p ("You're out of ammo!");
                    elapsed = 0;
                }
            }
            return elapsed;
        }

        /* Some weapons identify themselves when shot. */
        if (!isBlind () and (isHero () or Hero.canSee (this))) {
            if (weapon->isA (kObjHeatRayGun) or
                weapon->isA (kObjFreezeRayGun) or
                weapon->isA (kObjDisintegrationRayGun) or
                weapon->isA (kObjPoisonRayGun) or
                weapon->isA (kObjStasisRayGun) or
                weapon->isA (kObjSquirtRayGun) or
                weapon->isA (kObjAccelerationRayGun) or
                weapon->isA (kObjDecelerationRayGun))
            {
                weapon->setKnown ();
            }
        }

        if (!isHero ()) {
            int knownappearance = weapon->isAppearanceKnown ();
            if (Hero.canSee (this)) {
                weapon->setAppearanceKnown ();
                I->p ("%s shoots %s!", the (), weapon->her (this));
            } else if (Hero.canHearThoughts (this)) {
                I->p ("%s shoots %s!", the (), weapon->anVague ());
            } else {
                const char *a_weap = weapon->anVague ();
                if (!Hero.isBlind () and Level->isInLOS (mX, mY)) {
                    /* muzzle flash gives away position */
                    I->p ("Someone shoots %s!", a_weap);
                    Level->feelSq (mX, mY);
                } else {
                    I->p ("You hear someone shooting %s!", a_weap);
                }
                if (!knownappearance) {
                    weapon->resetAppearanceKnown ();
                }
            }
        }

        if (weapon->isBuggy () and !isOrc () and !RNG (5)) {
            return misfire (weapon, attack);
        }
    } /* End of shoot with weapon block. */

    switch (attack->mEffect) {
    case shAttack::kSingle:
    case shAttack::kFarBurst:
        if (-1 == numrounds) { /* pea shooter */
            numrounds = 1;
        }

        if (kOrigin == dir) {
            while (numrounds--) {
                if (sufferDamage (attack, this, weapon, 0, 1)) {
                    die (kSuicide);
                    return -2;
                }
            }
            return attack->mAttackTime;
        } else if (kUp == dir) {
            if (isHero ()) {
                I->p ("You shoot at the ceiling.");
            }
            return attack->mAttackTime;
        } else if (kDown == dir) {
            if (isHero ()) {
                I->p ("You shoot at %s.", Level->mSquares[mX][mY].the ());
            }
            return attack->mAttackTime;
        }

        while (numrounds--) {
            int timeout = 100;
            x = mX;
            y = mY;
            firsttarget = 1;
            while (Level->moveForward (dir, &x, &y) and --timeout)
            {
                shSpecialEffect eff = beamSpecialEffect (attack, dir);

                if (eff and !Hero.isBlind ()) {
                    Level->setSpecialEffect (x, y, eff);
                    Level->drawSq (x, y);
                }

                if (Level->isOccupied (x, y)) {
                    shCreature *c = Level->getCreature (x, y);
                    int tohitmod = basehitmod;
                    if (!isHero () and !c->isHero ())
                        tohitmod -= 8; /* Avoid friendly fire. */
                    else if (!firsttarget)
                        tohitmod -= 4;

                    firsttarget = 0;
                    if (c->reflectAttack (attack, &dir)) {
                        Level->setSpecialEffect
                            (x, y, beamSpecialEffect (attack, kOrigin));
                        I->pauseXY (Hero.mX, Hero.mY, 10);
                        Level->setSpecialEffect (x, y, kNone);
                        continue;
                    }

                    int r = resolveRangedAttack (attack, weapon, tohitmod,
                                                 Level->getCreature (x, y));
                    if (r >= 0 and /* Not a piercing attack. */
                        shAttack::kRail != attack->mType and
                        (shAttack::kSpear != attack->mType or RNG (2)))
                    {
                        Level->setSpecialEffect
                            (x, y, beamSpecialEffect (attack, kOrigin));
                        I->pauseXY (Hero.mX, Hero.mY, 10);
                        Level->setSpecialEffect (x, y, kNone);
                        break;
                    }
                }
                if (Level->isOcclusive (x, y)) {
                    shFeature *f = Level->getFeature (x, y);
                    if (f) {
                        switch (f->mType) {
                        case shFeature::kDoorClosed:
                            if (f->isMagneticallySealed ()) {
                                if (kParticle == attack->mDamage[0].mEnergy) {
                                    I->p ("The %s bounces off the door!",
                                          attack->noun ());
                                    dir = reflectBackDir (dir);
                                    Level->setSpecialEffect (x, y,
                                        beamSpecialEffect (attack, kOrigin));
                                    I->pauseXY (Hero.mX, Hero.mY, 10);
                                    Level->setSpecialEffect (x, y, kNone);
                                    continue;
                                } else {
                                    I->p ("Your shot hits a force field!");
                                }
                                break;
                            }
                            if (!weapon->isA (kRayGun)) {
                                shootLock (weapon, attack, f);
                            }
                            break;
                        default:
                            /* TODO: shoot other types of features */
                            break;
                        }
                    }
                    Level->setSpecialEffect (x, y, kNone);
                    if (attack->mEffect == shAttack::kFarBurst) {
                        Level->moveForward (uTurn (dir), &x, &y);
                        Level->areaEffect (attack, weapon, x, y, kOrigin, this,
                            weapon->mEnhancement);
                    }
                    break;
                }
                if (eff) {
                    if (timeout > 85 or firsttarget or
                        distance (x, y, Hero.mX, Hero.mY) < 40 or
                        !RNG (5))
                    { /* trying not to pause too much */
                        I->pauseXY (Hero.mX, Hero.mY, 5);
                    }
                    Level->setSpecialEffect (x, y, kNone);
                }
            }
        }
        break;
    case shAttack::kBeam:
    case shAttack::kExtend:
        Level->areaEffect (attack, weapon, mX, mY, dir, this, 0);
        break;
    case shAttack::kCone:
        Level->areaEffect (attack, weapon, mX, mY, dir, this, 0);
        break;
    default:
        I->p ("Unknown weapon type!");
        break;
    }

    if (weapon and setemptyraygun) {
        weapon->mIlkId = kObjEmptyRayGun;
        weapon->setKnown ();
        I->p ("%c - %s", weapon->mLetter, weapon->inv ());
    }

    return attack->mAttackTime;
}


void
shCreature::projectile (shObject *obj, shObject *stack,
                        int x, int y, shDirection dir,
                        shAttack *attack, int range)
{
    int firsttarget = 1;
    while (range--) {
        shFeature *f;

        if (!mLevel->moveForward (dir, &x, &y)) {
            mLevel->moveForward (uTurn (dir), &x, &y);
            obj->impact (x, y, dir, this, stack);
            return;
        }

        if (mLevel->isOccupied (x, y)) {
            shCreature *c = mLevel->getCreature (x, y);
            int tohitmod = 0;
            /* M41 Pulse Rifle has integrated grenade launcher. */
            if (mWeapon and mWeapon->isA (kObjPulseRifle)) {
                tohitmod += 2 + mWeapon->mEnhancement;
            } else if (isRobot ()) {
                tohitmod += 2; /* Robo-launcher. */
            }
            if (!isHero () and !c->isHero ()) {
                tohitmod -= 8; /* avoid friendly fire */
            } else if (!firsttarget) {
                tohitmod -= 4;
            }

            int r = resolveRangedAttack (attack, obj, tohitmod, c, stack);
            firsttarget = 0;
            if (r >= 0) {
                /* a hit - resolveRangedAttack will have called obj->impact */
                return;
            } else if (Hero.canSee (c)) {
                /* a miss - assuming we were aiming at this creature, the
                   object shouldn't land too far away */
                range = mini (range, RNG (1, 3) - 1);
                if (-1 == range) { /* land in the square in front */
                    mLevel->moveForward (uTurn (dir), &x, &y);
                    obj->impact (x, y, dir, this, stack);
                }
            }
        }

        f = mLevel->getFeature (x, y);
        if (f) {
            switch (f->mType) {
            case shFeature::kDoorHiddenVert:
            case shFeature::kDoorHiddenHoriz:
            case shFeature::kDoorClosed:
            case shFeature::kComputerTerminal:
            case shFeature::kPortal:
                /* The thrown object will hit these solid features. */
                obj->impact (f, dir, this, stack);
                return;
            default: /* It will fly right past other features. */
                break;
            }
        }

        if (mLevel->isObstacle (x, y)) {
            obj->impact (x, y, dir, this, stack);
            return;
        }
    }
    /* maximum range */
    obj->impact (x, y, dir, this, stack);
}

/*  work out the result of throwing the object in the given direction
    returns: ms elapsed
*/
int
shCreature::throwObject (shObject *obj, shObject *stack, shDirection dir)
{
    obj->mOwner = NULL; /* Needed to get correct destruction messages. */
    if (isHero ()) {
        Hero.usedUpItem (obj, obj->mCount, "throw");
        obj->resetUnpaid ();
    }

    shAttack *attack;
    if (obj->isThrownWeapon ()) {
        attack = &Attacks[obj->myIlk ()->mMissileAttack];
    } else {
        attack = &Attacks[kAttImprovisedThrow];
    }
    if (obj->isA (kObjSmartDisc)) {
        Level->areaEffect (attack, obj, mX, mY, dir, this, obj->mEnhancement);
        return attack->mAttackTime;
    }

    bool launch = isRobot () or (mWeapon and mWeapon->isA (kObjPulseRifle));
    if (!isHero () and Hero.canSee (this)) {
        if (launch) {
            I->p ("%s launches %s!", the (), obj->anQuick ());
        } else {
            I->p ("%s throws %s!", the (), obj->anQuick ());
        }
    }
    if (isHero () and launch) {
        I->p ("You launch %s.", obj->anQuick ());
    }

    if (kUp == dir) {
        if (isHero ()) {
            I->p ("It bounces off the ceiling and lands on your head!");
        }
        obj->impact (this, kDown, this, stack);
        return attack->mAttackTime;
    } else if (kDown == dir) {
        obj->impact (mX, mY, kDown, this, stack);
        return attack->mAttackTime;
    }

    int maxrange;
    if (launch) {
        maxrange = 60; /* Grenade launcher. */
    } else { /* 5 range increments @ 5 ft per increment. */
        maxrange = 4 + ABILITY_MODIFIER (getStr ()) + NDX (2, 4);
    }

    projectile (obj, stack, mX, mY, dir, attack, maxrange);

    return attack->mAttackTime;
}


int
shHero::kick (shDirection dir)
{
    const char *verb = "kick";
    const char *gerund = "kicking";

    /* Sealed body armor makes tail slaps impossible. */
    if (!mBodyArmor or !mBodyArmor->isSealedArmor ())
        for (int i = 0; i < MAXATTACKS; ++i) {
            shAttack *atk = &Attacks[myIlk ()->mAttacks[i].mAttId];
            /* Sometimes replace kick with tail slap for flavor. */
            if (atk and atk->mType == shAttack::kTailSlap and RNG (2)) {
                verb = "tail slap";
                gerund = "tail slapping";
                break;
            }
        }

    shFeature *f;
    int x = mX;
    int y = mY;
    bool known = mLevel->getMemory (x, y).mTerr != kMaxTerrainType;
    feel (x, y);

    if (mLevel->isWatery (x, y)) {
        I->p ("You slosh around uselessly.");
        return HALFTURN;
    }

    if (kUp == dir) {
        I->p ("You %s the air.", verb);
        return FULLTURN;
    }

    if (mZ < 0) {
        f = mLevel->getFeature (x, y);
        if (f) {
            switch (f->mType) {
            case shFeature::kPit:
                I->p ("You %s the pit wall!", verb);
                if (sufferDamage (kAttKickedWall)) {
                    char *reason = GetBuf ();
                    snprintf (reason, SHBUFLEN, "%s a pit wall", gerund);
                    shCreature::die (kKilled, reason);
                }
                return FULLTURN;
            case shFeature::kAcidPit:
                I->p ("Your useless %s splashes acid everywhere.", verb);
                if (sufferDamage (kAttAcidPitBath)) {
                    shCreature::die (kMisc, "Dissolved in acid");
                }
                return FULLTURN;
            case shFeature::kSewagePit:
                I->p ("You slosh around uselessly.");
                return FULLTURN;
            default:
                I->p ("UNEXPECTED: don't know how to handle %s from here.", verb);
                I->p ("(Please file a bug report!)");
                return 0;
            }
        } else {
            I->p ("UNEXPECTED: don't know how to handle %s from here.", verb);
            I->p ("(Please file a bug report!)");
            return 0;
        }
    }

    if (!mLevel->moveForward (dir, &x, &y)) {

    } else if (kDown != dir and mLevel->isOccupied (x, y)) {
        /* Treat as unarmed attack. */
        shCreature *c = Level->getCreature (x, y);
        /* Find a kick.  Chance to replace with tail slap 20% if available. */
        shAttack *atk = NULL;
        for (int i = 0; i < MAXATTACKS; ++i) {
            shAttack *a = &Attacks[myIlk ()->mAttacks[i].mAttId];
            if (a and (a->mType == shAttack::kKick or
                       a->mType == shAttack::kRake))
                atk = a;
            if (a and a->mType == shAttack::kTailSlap and !RNG (5))
                atk = a;
        }
        if (!atk) {
            I->p ("Oddly, you seem to have nothing to kick with.");
            return FULLTURN;
        }
        resolveMeleeAttack (atk, NULL, c);
    } else if (mLevel->countObjects (x, y)) {
        int maxrange = ABILITY_MODIFIER (getStr ()) + NDX (2, 4);
        shObject *obj;
        shAttack *atk;
        int dbonus = 0;

        obj = mLevel->removeObject (x, y, kObjFootball);
        if (obj) {
            atk = &Attacks[kAttKickedFootball];
            dbonus = ABILITY_MODIFIER (getStr ());
            dbonus += obj->mEnhancement;
        } else { /* Remove top object ourselves. */
            shObjectVector *v = mLevel->getObjects (x, y);

            obj = findBestObject (v);
            v->remove (obj);
            if (0 == v->count ()) {
                delete v;
                mLevel->setObjects (x, y, NULL);
            }
            atk = &Attacks[kAttImprovisedThrow];
            maxrange = maxi (2, maxrange);
        }

        if (kDown == dir) {
            I->p ("You stomp on %s.", obj->theQuick ());
            if (obj->isUnpaid ()) {
                usedUpItem (obj, obj->mCount, "kick");
                obj->resetUnpaid ();
            }
            obj->impact (x, y, kDown, this, NULL);
        } else {
            if (obj->isUnpaid ()) {
                usedUpItem (obj, obj->mCount, verb);
                obj->resetUnpaid ();
            }
            projectile (obj, NULL, x, y, dir, atk, maxrange);
        }
    } else if ((f = mLevel->getFeature (x, y))) {
        int score = D20 () + getWeaponSkillModifier (NULL);
        if (mProfession == Quarterback)  score += 5;
        switch (f->mType) {
        case shFeature::kDoorHiddenVert:
        case shFeature::kDoorHiddenHoriz:
            score += f->mSportingChance++;
            if (score >= 20 and
                !(shFeature::kLocked & f->mDoor.mFlags))
            {
                I->p ("Your %s opens a secret door!", verb);
                f->mType = shFeature::kDoorOpen;
                f->mSportingChance = 0;
            } else if (score > 18) {
                I->p ("Your %s uncovers a secret door!", verb);
                f->mType = shFeature::kDoorClosed;
                f->mSportingChance = 0;
            } else {
                I->p ("You %s %s wall!", verb, known ? "the" : "a");
                if (sufferDamage (kAttKickedWall)) {
                    char *reason = GetBuf ();
                    snprintf (reason, SHBUFLEN, "%s a secret door", gerund);
                    shCreature::die (kKilled, reason);
                }
            }
            break;
        case shFeature::kDoorClosed:
            if (f->isMagneticallySealed ()) {
                I->p ("Your %s a force field!  Ow!", verb);
                if (sufferDamage (kAttKickedWall)) {
                    char *reason = GetBuf ();
                    snprintf (reason, SHBUFLEN, "%s a magnetically sealed door", gerund);
                    shCreature::die (kKilled, reason);
                }
                break;
            }
            score += f->mSportingChance;
            f->mSportingChance += RNG (1, 3);
            if (score >= (shFeature::kLocked & f->mDoor.mFlags ? 22 : 20)) {
                f->mType = shFeature::kDoorOpen;
                f->mDoor.mFlags &= ~shFeature::kLocked;
                if (!f->isLockBrokenDoor () and f->keyNeededForDoor ()) {
                    I->p ("You smash the lock as you %s the door open!", verb);
                    f->mDoor.mFlags |= shFeature::kLockBroken;
                } else {
                    I->p ("You %s the door open!", verb);
                }
                f->mSportingChance = 0;
            } else {
                I->p ("The door shudders.%s",
                      mProfession == Quarterback ? "" : "  Ow!");
                if (sufferDamage (kAttKickedWall)) {
                    char *reason = GetBuf ();
                    snprintf (reason, SHBUFLEN, "%s a door", gerund);
                    shCreature::die (kKilled, reason);
                }
            }
            if (f->isAlarmedDoor ()) {
                I->p ("You set off an alarm!");
                Level->doorAlarm (f);
            }
            break;

        case shFeature::kComputerTerminal:
        case shFeature::kPortal:
        case shFeature::kStairsUp:
        case shFeature::kStairsDown:
        case shFeature::kRadTrap:
        case shFeature::kVat:
        case shFeature::kMaxFeatureType:
            I->p ("You %s %s!", verb, known ? f->the () : f->an ());
            if (sufferDamage (kAttKickedWall)) {
                char *reason = GetBuf ();
                snprintf (reason, SHBUFLEN, "%s %s", gerund, f->an ());
                shCreature::die (kKilled, reason);
            }
            break;
        case shFeature::kDoorOpen:
        default:
            I->p ("You %s the air!", verb);
            break;

        }
    } else if (mLevel->isObstacle (x, y)) {
        I->p ("You %s %s!", verb,
              known ? mLevel->the (x, y) : mLevel->an (x, y));
        if (sufferDamage (kAttKickedWall)) {
            char *reason = GetBuf ();
            snprintf (reason, SHBUFLEN, "%s %s", gerund, mLevel->an (x, y));
            shCreature::die (kKilled, reason);
        }
    } else {
        I->p ("You %s the air!", verb);
    }
    feel (x, y);
    return FULLTURN;
}

/* work out the melee attack against the target
   returns: 1 if the target is eliminated (e.g. it dies, telports away, etc.)
            0 if the target is hit but not eliminated
           -1 if the attack was a complete miss
           -2 if the attacker dies
*/
int
shCreature::resolveMeleeAttack (shAttack *attack,
                                shObject *weapon,
                                shCreature *target)
{
    int attackmod;
    int AC;
    int flatfooted = 0;   /* set to 1 if the target is unable to react to the
                             attack due to unawareness or other disability */
    int vis;

    const char *n_attacker;
    const char *n_target;

    int dbonus = 0;
    int dmul = 0;
    int cantsee = 1;

    if (isHero ()) {
        n_attacker = "you";
        cantsee = 0;
    } else if (Hero.canSee (this) or Hero.canHearThoughts (this)) {
        n_attacker = the ();
        cantsee = 0;
    } else {
        n_attacker = "something";
    }
    if (target->isHero ()) {
        n_target = "you";
        cantsee = 0;
    } else if (Hero.canSee (target)) {
        n_target = target->the ();
        cantsee = 0;
    } else {
        n_target = "something";
    }

    vis = canSee (target);

    if (is (kFrightened) and vis) {
        if (isHero ()) {
            I->p ("You are too afraid to attack %s!", n_target);
        }
        return -1;
    }
    if (is (kUnableAttack)) {
        if (isHero ()) {
            I->p ("What a blatant display of belligerence.");
        }
        return -1;
    }
    if (target->isScary () and isHero () and
        !isBlind () and !is (kConfused) and !is (kStunned) and
        !willSave (target->getPsionicDC (3)))
    {   /* brain shield doesn't help in this case... */
        I->p ("You are suddenly too afraid to %s %s!",
              weapon ? "attack" : "touch", n_target);
        inflict (kFrightened, 1000 * NDX (2, 10));
        return -1;
    }

    if (weapon) {
        if (weapon->isMeleeWeapon ()) {
            expendAmmo (weapon);
            attackmod = getWeaponSkillModifier (weapon->myIlk (), attack);
            attackmod += weapon->mEnhancement;
            attackmod -= 2 * weapon->mDamage;
            dbonus = weapon->mEnhancement;
            dbonus -= weapon->mDamage;
        } else {
            if (weapon->isA (kWeapon)) {
                /* e.g. pistol whipping, use unarmed combat skill */
                attackmod = getWeaponSkillModifier (NULL);
                attackmod += weapon->mEnhancement;
                attackmod -= 2 * weapon->mDamage;
            } else {
                /* improvised melee weapon */
                attackmod = ABILITY_MODIFIER (getStr ()) - 4;
                dbonus = ABILITY_MODIFIER (getStr ()) / 2;
            }
            attackmod += mToHitModifier;
        }
    } else { /* Basic hand to hand attack. */
        attackmod = getWeaponSkillModifier (NULL);
    }

    dbonus += mDamageModifier;

    if (0 == target->canSee (this)) {
        attackmod += 2;
        flatfooted = 1;
    }
    if (target->is (kStunned)) {
        flatfooted = 1;
        attackmod += 2;
    }

    if (target->is (kAsleep) or target->is (kParalyzed) or
        target->isSessile ())
    {
        flatfooted = 1;
        attackmod += 8;
    }

    if (attack->isTouchAttack ()) {
        AC = target->getTouchAC (flatfooted);
    } else {
        AC = target->getAC (flatfooted);
    }

    /* roll to hit */
    dmul = attackRoll (attack, weapon, attackmod, AC, target);


    if (dmul <= 0) { /* Miss. */
        dmul = -1;
        if (target->mHidden <= 0) {
            if (isHero ()) {
                I->p ("You miss %s.", n_target);
            } else if (target->isHero ()) {
                I->p ("%s misses.", n_attacker);
            } else if (!cantsee) {
                I->p ("%s misses %s.", n_attacker, n_target);
            }
            target->newEnemy (this);
        }
        target->interrupt ();
        return dmul;
    } else if (isHero ()) {
        if (target->isA (kMonMonolith) and !target->isHostile () and
            (
             (attack->mType == shAttack::kPunch and !mWeapon) or /* Humanoid. */
             (attack->mType == shAttack::kClaw and !mWeapon) or /* Xel'Naga. */
             (attack->mType == shAttack::kKick and !mBoots)
            ))
        {
            I->p ("You touch the monolith!");
            I->p ("You feel more experienced!");
            Hero.earnXP (-1);
            I->p ("The monolith mysteriously vanishes!");
            target->die (kSuicide);
            return 1;
        }
        /* SufferDamage will handle hit message. */
        if (target->sufferDamage (attack, this, weapon, dbonus, dmul)) {
            target->die (kSlain, this, weapon, attack);
            return 1;
        }
        target->newEnemy (this);
        target->interrupt ();
        return 0;
    } else if (target->isHero ()) {
        const char *hitsmesg = monHitsYouMesg (attack);
        if (hitsmesg)
            I->p ("%s%s!", n_attacker, hitsmesg);
        if (shAttack::kFaceHug == attack->mType and
            !Hero.getStoryFlag ("impregnation"))
        {
            int chance;
            if (Hero.is (kAsleep) or Hero.is (kParalyzed)) {
                chance = 1; /* Guaranteed. */
            } else if (Hero.mHelmet and Hero.mHelmet->isSealedArmor ()) {
                chance = 3; /* Sealed helmet protects somewhat. */
            } else {
                chance = 2;
            }
            int result = RNG (chance);
            if (!result) {
                I->p ("The facehugger impregnates you with an alien embryo!");
                Hero.setStoryFlag ("impregnation", 1);
                die (kSuicide);
                if (chance == 3) { /* Damage helmet. */
                    int *enh = &Hero.mHelmet->mEnhancement;
                    if (*enh < -Hero.mHelmet->myIlk ()->mMaxEnhancement) {
                        I->p ("%s was destroyed.", YOUR (Hero.mHelmet));
                        shObject *obj = Hero.mHelmet;
                        Hero.removeObjectFromInventory (obj);
                        delete obj;
                    } else {
                        I->p ("%s has been damaged.", YOUR (Hero.mHelmet));
                        --(*enh);
                    }
                    I->drawSideWin (); /* Possible AC change. */
                }
                return -2;
            } else if (result == 2) {
                I->p ("The facehugger bounces off %s.", YOUR (Hero.mHelmet));
                return 0;
            } else {
                I->nonlOnce ();
                if (RNG (2)) {
                    I->p ("  You dodge.");
                } else {
                    I->p ("  It misses.");
                }
                return -1;
            }
        }
        if (target->sufferDamage (attack, this, weapon, dbonus, dmul)) {
            if (shAttack::kExtractBrain == attack->mType) {
                target->die (kBrainJarred, this, weapon, attack);
            } else {
                target->die (kSlain, this, weapon, attack);
            }
            return 1;
        }
        return 0;
    } else {
        if (!cantsee) {
            I->p ("%s%s %s!", n_attacker, monHitsMonMesg (attack), n_target);
        }
        if (target->sufferDamage (attack, this, weapon, dbonus, dmul)) {
            if (!cantsee) {
                target->pDeathMessage (n_target, kSlain);
            }
            target->die (kSlain, this, weapon, attack);
            return 1;
        }
        target->newEnemy (this);
        target->interrupt ();
        return 0;
    }
}


/*  work out the result of melee attacking in the given direction
    returns: ms elapsed
*/
int
shHero::meleeAttack (shObject *weapon, shDirection dir)
{
    int x = mX;
    int y = mY;
    shAttack *attack;
    shCreature *target;

    if (!Level->moveForward (dir, &x, &y)) {
        /* impossible to attack off the edge of the map? */
        I->p ("There's nothing there!");
        return 0;
    }

    target = Level->getCreature (x, y);

    if (target and target->mHidden) {
        target->revealSelf ();
    }

    if (NULL == weapon) {
        if (0 == getStoryFlag ("strange weapon message")) {
            if (mProfession == XelNaga) {
                I->p ("You start ripping your foes with your clawed hands.");
            } else {
                I->p ("You start pummeling your foes with your bare hands.");
            }
            setStoryFlag ("strange weapon message", 1);
        }
        if (target and target->isA (kMonMonolith)) {
            /* Always punch monoliths.  FIXME: Find punch. */
            attack = &Attacks[myIlk ()->mAttacks[0].mAttId];
        } else {
            // FIXME: Use probabilities instead of choosing at random.
            // Reuse choosing melee attack procedure from MonsterAI.cpp
            attack = &Attacks[myIlk ()->mAttacks[0].mAttId];
        }
    } else if (weapon->isMeleeWeapon () and
               (!weapon->isA (kObjLightSaber) or weapon->isKnown ()))
    {
        if (hasAmmo (weapon)) {
            attack = &Attacks[weapon->myIlk ()->mMeleeAttack];
        } else {
            if (!getStoryFlag ("strange weapon message")) {
                I->p ("You are out of power for %s.", weapon->your ());
                I->p ("You start clumsily smashing your enemies with it.");
                setStoryFlag ("strange weapon message", 1);
            }
            attack = &Attacks[kAttImprovisedMelee];
        }
    } else {
        if (!getStoryFlag ("strange weapon message")) {
            I->p ("You start smashing enemies with %s.", weapon->your ());
            setStoryFlag ("strange weapon message", 1);
        }
        attack = &Attacks[kAttImprovisedMelee];
    }

    if (!target) {
        I->p ("You attack thin air!");
    } else {
        resolveMeleeAttack (attack, weapon, target);
    }
    feel (x, y);

    return attack->mAttackTime;
}


/*
  RETURNS:  1 if target is eliminated (dies, teleports, etc)
            0 if target was attacked
           -1 if target was missed
           -2 if the attacking monster dies
*/
int
shMonster::meleeAttack (shObject *weapon, shAttack *attack, int x, int y)
{
    shCreature *target;

    target = mLevel->getCreature (x, y);
    if (NULL == target) {
        if (Hero.canSee (this)) {
            I->p ("%s attacks thin air!", the ());
        }
        return -1;
    }
    if (Hero.isBlind ()) {
        Hero.feel (mX, mY);
    }
    return resolveMeleeAttack (attack, weapon, target);
}


void
shObject::breakAt (int x, int y)
{
    if (isA (kFloppyDisk) and isBuggy () and RNG (2)) {
        bool found_site = false;
        if (!Level->isOccupied (x, y) and Level->isFloor (x, y))
            found_site = true;
        if (!found_site and !Level->findAdjacentUnoccupiedSquare (&x, &y)) {
            found_site = true;
        }
        if (found_site) {
            shMonId mon;
            if (myIlk ()->mCost >= 300)
                mon = kMonRadbug;
            else
                mon = kMonGiantCockroach;
            shMonster *bug = new shMonster (mon);
            Level->putCreature (bug, x, y);
            if (Hero.canSee (x, y)) {
                I->p ("A black cloud rises forth from floppy disk remains!");
                I->p ("It coalesces into a bug.");
            } else if (distance (x, y, Hero.mX, Hero.mY) <= 40) {
                I->p ("You hear an evil hissing.");
            }
        }
    }
    delete this;
}


void
shObject::impact (int x, int y, shDirection dir,
                  shCreature *thrower, shObject *stack)
{
    if (Level->isObstacle (x, y)) { /* bounce back one square */
        Level->moveForward (uTurn (dir), &x, &y);
    }

    if (myIlk ()->mMissileAttack) {
        shAttack *atk = &Attacks[myIlk ()->mMissileAttack];
        if (shAttack::kBurst == atk->mEffect) { /* a grenade */
            if (Hero.isBlind ())  I->p ("Boom!");
            Level->areaEffect (atk, this, x, y, dir, thrower, mEnhancement, 0);
            /* Identifying thrown grenade identifies whole stack. */
            if (stack)  stack->mFlags = this->mFlags;
            if (thrower->isHero () and thrower->isBlind ()) {
                this->maybeName (); /* Heard this item explode. */
            }
            delete this;
            return;
        }
    }

    if (sufferDamage (kAttGroundCollision, x, y)) {
        breakAt (x, y);
    } else {
        Level->putObject (this, x, y);
    }
}


void
shObject::impact (shCreature *c, shDirection dir,
                  shCreature *thrower, shObject *stack)
{
    int dbonus = 0;
    const char *the_monster = c->the ();

    int x = c->mX;
    int y = c->mY;

    if (mIlkId == kObjRestrainingBolt and c->isRobot ()) {
        I->p ("%s clings to %s.", theQuick (), the_monster);
        c->attemptRestraining (this);
        return;
    }

    I->p ("%s hits %s.", theQuick (), the_monster);

    shAttackId attid;
    if (myIlk ()->mMissileAttack) {
        attid = myIlk ()->mMissileAttack;
        dbonus += mEnhancement;
        if (shAttack::kSingle == Attacks[attid].mEffect) {
            dbonus += ABILITY_MODIFIER (thrower->getStr ());
        }
    } else {
        attid = kAttImprovisedThrow;
    }

    shAttack *atk = &Attacks[attid];
    if (shAttack::kBurst != atk->mEffect) {
        if (c->sufferDamage (atk, thrower, NULL, dbonus)) {
            shCauseOfDeath reason;
            if (atk->dealsEnergyType (kDisintegrating)) {
                reason = kAnnihilated;
            } else {
                reason = kSlain;
            }
            c->pDeathMessage (THE (c), reason);
            c->die (reason, thrower, this, atk);
        } else {
            if (thrower->isHero () or
                thrower->isPet ())
            {
                c->newEnemy (thrower);
            }
            c->interrupt ();
        }
        /* Hits the monster and then falls to the ground. */
        if (sufferDamage (kAttGroundCollision, x, y)) {
            breakAt (x, y);
        } else {
            Level->putObject (this, x, y);
        }
    } else { /* a grenade */
        if (Hero.isBlind ())  I->p ("Boom!");
        Level->areaEffect (atk, this, c->mX, c->mY, dir, thrower, 0, dbonus);
        if (stack)  stack->mFlags = this->mFlags;
        if (thrower->isHero () and thrower->isBlind ()) {
            this->maybeName ();
        }
        delete this;
    }

}


void
shObject::impact (shFeature *f, shDirection dir,
                  shCreature *thrower, shObject *stack)
{
    int x = f->mX;
    int y = f->mY;

    if (myIlk ()->mMissileAttack) {
        shAttack *atk = &Attacks[myIlk ()->mMissileAttack];
        if (shAttack::kBurst == atk->mEffect) { /* a grenade */
            if (f->isObstacle () and atk->mRadius > 0) {
                /* bounce back one square */
                Level->moveForward (uTurn (dir), &x, &y);
            }
            if (Hero.isBlind ())  I->p ("Boom!");
            Level->areaEffect (atk, this, x, y, dir, thrower, 0, mEnhancement);
            if (stack)  stack->mFlags = this->mFlags;
            if (thrower->isHero () and thrower->isBlind ()) {
                this->maybeName ();
            }
            delete this;
            return;
        }
    }
    if (sufferDamage (kAttGroundCollision, x, y)) {
        breakAt (x, y);
    } else {
        if (f->isObstacle ()) {
            /* bounce back one square */
            Level->moveForward (uTurn (dir), &x, &y);
        }
        Level->putObject (this, x, y);
    }

}



void
shCreature::shootLock (shObject *weapon, shAttack *attack, shFeature *door)
{
    if (!isHero ())
        return;

    if (!weapon->myIlk ()->mGunAttack)  /* Ray guns. */
        return;

    if (areAdjacent (Hero.mX, Hero.mY, door->mX, door->mY) and
        Hero.canSee (door->mX, door->mY))
    {
        int score =
            D20 () + 4 +
            /* Might be without weapon. For example Optic Blast. */
            (weapon ? getWeaponSkillModifier(weapon->myIlk (), attack) : 0) +
            attack->mDamage[0].mHigh +
            door->mSportingChance;
        door->mSportingChance += RNG (2, 6);

        if ((door->isLockBrokenDoor () and !door->isLockedDoor ())
            or !door->isLockDoor ())
        {   /* Silent. */
        } else if (score < 22) {
            I->p ("You damage the lock.");
        } else if (door->isLockBrokenDoor ()) {
            I->p ("The broken lock falls apart!");
            door->mDoor.mFlags &= ~shFeature::kLocked;
        } else if (RNG (3) or score > 32) {
            I->p ("The lock falls apart!");
            door->mDoor.mFlags |= shFeature::kLockBroken;
            door->mDoor.mFlags &= ~shFeature::kLocked;
        } else {
            I->p ("You break the lock!");
            door->mDoor.mFlags |= shFeature::kLockBroken;
        }

        if (door->isAlarmedDoor ()) {
            I->p ("You set off an alarm!");
            Level->doorAlarm (door);
        }
    }
}


/* returns: 1 if the feature should block further effect
            0 if effect should continue
            -1 if effect should be reflected (message printed)
 */
int
shMapLevel::areaEffectFeature (shAttack *atk, shObject *weapon, int x, int y,
                               shCreature *attacker, int dbonus /* = 0 */)
{
    shFeature *f = getFeature (x, y);
    int destroy = 0;
    int block = 1;
    if (atk->mType == shAttack::kDisintegrationRay) {
        block = 0;
    }

    if (atk->mType == shAttack::kFlare)  setLit (x, y);

    if (!f or GameOver)
        return 0;

    switch (f->mType) {
    case shFeature::kDoorOpen:
        if (atk->mType == shAttack::kIncendiary) {
            if (Hero.canSee (x, y)) {
                I->p ("The flames burn down the door!");
            }
            destroy = 1;
        } else if (atk->mType == shAttack::kBlast and
                   atk->mDamage[0].mEnergy == kDisintegrating)
        {
            if (Hero.canSee (x, y)) {
                I->p ("The door is annihilated!");
            } else {
                I->p ("You hear a loud bang!");
                if (weapon) weapon->maybeName ();
            }
            destroy = 1;
        } else if (atk->mType == shAttack::kBlast and
                   atk->mDamage[0].mEnergy == kBurning)
        {
            if (Hero.canSee (x, y)) {
                I->p ("The door melts down!");
            }
            destroy = 1;
        }
        block = 0;
        break;
    case shFeature::kDoorClosed:
        if (f->isMagneticallySealed ()) {
            if (kLaser == atk->mDamage[0].mEnergy or
                kParticle == atk->mDamage[0].mEnergy )
            {
                if (Hero.canSee (x, y))
                    I->p ("The %s bounces off the door!", atk->noun ());
                return -1;
            }
            if (Hero.canSee (x, y) and shAttack::kGaussRay != atk->mType)
                I->p ("The %s is absorbed by a force field!", atk->noun ());
            return 1;
        }
        switch (atk->mType) {
        case shAttack::kBlast:
            if (atk->mDamage[0].mEnergy == kBurning) {
                if (Hero.canSee (x, y)) {
                    I->p ("The door melts down!");
                }
                destroy = 1;
                break;
            } else if (atk->mDamage[0].mEnergy == kDisintegrating) {
                /* Fall through to disintegration ray. */
            } else {
                break;
            }
        case shAttack::kDisintegrationRay:
            if (Hero.canSee (x, y)) {
                I->p ("The door is annihilated!");
            } else {
                I->p ("You hear a loud bang!");
                if (weapon) weapon->maybeName ();
            }
            destroy = 1;
            break;
        case shAttack::kGaussRay:
            f->mDoor.mFlags &= ~shFeature::kAutomatic;
            f->mDoor.mFlags &= ~shFeature::kBerserk;
            if (f->isLockedDoor ()) {
                f->mDoor.mFlags &= ~shFeature::kLocked;
                f->mDoor.mFlags |= shFeature::kLockBroken;
            }
            block = 0;
            break;
        case shAttack::kHeatRay:
            if (Hero.canSee (x, y)) {
                setSpecialEffect (x, y, beamSpecialEffect (atk, kOrigin));
                drawSq (x, y);
                I->p ("The heat ray melts a hole through the door!");
            }
            destroy = 1;
            break;
        case shAttack::kIncendiary:
            if (Hero.canSee (x, y)) {
                I->p ("The flames burn down the door!");
            }
            destroy = 1;
            break;
        case shAttack::kGammaRay: /* no message */
        case shAttack::kTransporterRay:
        case shAttack::kStasisRay:
        case shAttack::kHealingRay:
        case shAttack::kRestorationRay:
        case shAttack::kFlash:
            break;
        case shAttack::kPoisonRay:
        case shAttack::kFreezeRay:
        default:
            break;
        }
        if (!destroy and atk->mEffect != shAttack::kExtend) {
            attacker->shootLock (weapon, atk, f);
        }
        break;
    case shFeature::kDoorHiddenVert:
    case shFeature::kDoorHiddenHoriz:
        switch (atk->mType) {
        case shAttack::kBlast:
            if (atk->mDamage[0].mEnergy != kDisintegrating)
                break;
        case shAttack::kDisintegrationRay:
            if (Hero.canSee (x, y)) {
                setSpecialEffect (x, y, beamSpecialEffect (atk, kOrigin));
                drawSq (x, y);
            } /* Silently because antimatter may dig. */
            destroy = 1;
            break;
        case shAttack::kGaussRay:
            if (f->isLockedDoor ()) {
                f->mDoor.mFlags &= ~shFeature::kLocked;
                f->mDoor.mFlags |= shFeature::kLockBroken;
            }
            block = 0;
            break;
        case shAttack::kHeatRay:
            if (Hero.canSee (x, y)) {
                setSpecialEffect (x, y, beamSpecialEffect (atk, kOrigin));
                drawSq (x, y);
                I->p ("The heat ray melts a hole through a secret door!");
            }
            destroy = 1;
            break;
        case shAttack::kIncendiary:
            if (Hero.canSee (x, y)) {
                setSpecialEffect (x, y, beamSpecialEffect (atk, kOrigin));
                drawSq (x, y);
                I->p ("The flames burn down a secret door!");
            }
            destroy = 1;
            break;
        case shAttack::kPoisonRay:
        case shAttack::kFreezeRay:
        case shAttack::kGammaRay:
        case shAttack::kStasisRay:
        case shAttack::kHealingRay:
        case shAttack::kRestorationRay:
        default:
            break;
        }
        break;
    case shFeature::kMovingHWall:
        switch (atk->mType) {
        case shAttack::kBlast:
            if (atk->mDamage[0].mEnergy != kDisintegrating)
                break;
        case shAttack::kDisintegrationRay:
            if (Hero.canSee (x, y)) {
                I->p ("The moving wall section is annihilated!");
            } else {
                I->p ("You hear a loud bang!");
                if (weapon) weapon->maybeName ();
            }
            destroy = 1;
            break;
        default:
            break;
        }
        break;
    case shFeature::kMachinery:
        switch (atk->mType) {
        case shAttack::kBlast:
            if (atk->mDamage[0].mEnergy != kDisintegrating)
                break;
        case shAttack::kDisintegrationRay:
            if (Hero.canSee (x, y)) {
                I->p ("The machinery is annihilated!");
            } else {
                I->p ("You hear a loud bang!");
                if (weapon) weapon->maybeName ();
            }
            destroy = 1;

            /* KLUDGE: stop the wall from moving */
            int w;
            for (w = y+1; w <= y +8; w++) {
                shFeature *g = Level->getFeature (x, w);
                if (!g)
                    break;
                if (shFeature::kMovingHWall == g->mType) {
                    g->mType = shFeature::kMachinery;
                    if (Hero.canSee (x, y)) {
                        I->p ("The wall segment lurches and halts.");
                    } else {
                        I->p ("You hear gears lurching and grinding.");
                    }
                    break;
                }
                if (shFeature::kMovingHWall != g->mType)
                    break;
            }
            for (w = y-1; w <= y - 8; w--) {
                shFeature *g = Level->getFeature (x, w);
                if (!g)
                    break;
                if (shFeature::kMovingHWall == g->mType) {
                    g->mType = shFeature::kMachinery;
                    if (Hero.canSee (x, y)) {
                        I->p ("The wall segment lurches and halts.");
                    } else {
                        I->p ("You hear gears lurching and grinding.");
                    }
                    break;
                }
                if (shFeature::kMovingHWall != g->mType)
                    break;
            }
            break;
        default:
            break;
        }
        break;
    case shFeature::kVat:
        switch (atk->mType) {
        case shAttack::kBlast:
            if (atk->mDamage[0].mEnergy != kDisintegrating)
                break;
        case shAttack::kDisintegrationRay:
            if (Hero.canSee (x, y)) {
                I->p ("The vat is annihilated!");
            } else {
                I->p ("You hear a gurgling hiss!");
                if (weapon) weapon->maybeName ();
            }
            destroy = 1;
            break;
        //TODO: Transporter ray.
        default:
            block = 0;
            break;
        }
    case shFeature::kRadTrap:
        switch (atk->mType) {
        case shAttack::kBlast:
            if (atk->mDamage[0].mEnergy != kDisintegrating)
                break;
            if (Hero.canSee (x, y)) {
                I->p ("The radiation trap is annihilated!");
            } else {
                I->p ("You hear a loud bang!");
                if (weapon) weapon->maybeName ();
            }
            destroy = 1;
            break;
        //TODO: Transporter ray.
        default:
            block = 0;
            break;
        }
    case shFeature::kPit:
        if (atk->mType == shAttack::kAcidSplash and !RNG (4)) {
            if (Hero.canSee (x, y)) {
                I->p ("The pit is filled with acidic blood.");
            }
            f->mType = shFeature::kAcidPit;
        }
        block = 0;
        break;
    default:
        /* no effect */
        block = 0;
        break;
    }

    if (destroy) {
        Level->removeFeature (f);
        Level->computeVisibility ();
        if (attacker and Level->isInShop (x, y)) {
            if (attacker->isHero ()) {
                Hero.damagedShop (x, y);
            } // TODO: else make shopkeeper complain about attacker
            // "That damned *monname* just wrecked my door!
            // And I have to put up with it because *monname* is *not*
            // the player character! Thats sooo lame."
        }
    }
    return block;
}


void
shMapLevel::areaEffectObjects (shAttack *atk, shObject *weapon, int x, int y,
                               shCreature *attacker, int dbonus /* = 0 */)
{
    shObjectVector *v = Level->getObjects (x, y);

    if (v and !GameOver) {
        for (int i = 0; i < v->count (); ++i) {
            shObject *obj = v->get (i);
            int numdestroyed = obj->sufferDamage (atk, x, y, 1, 1);
            if (numdestroyed) {
                if (obj->isUnpaid () and attacker->isHero ()) {
                    if (atk->mDamage[0].mEnergy == kDisintegrating) {
                        Hero.usedUpItem (obj, numdestroyed, "annihilate");
                    } else {
                        Hero.usedUpItem (obj, numdestroyed, "destroy");
                    }
                }
                if (numdestroyed == obj->mCount) {
                    v->remove (obj);
                    --i;
                } else {
                    obj->mCount -= numdestroyed;
                }
            }
        }
        if (!v->count ()) {
            delete v;
            Level->setObjects (x, y, NULL);
        }
    }
}


/* returns: -2 if attacker dies,
             0 if effect should keep going,
             1 if effect should stop
 */

int
shMapLevel::areaEffectCreature (shAttack *atk, shObject *weapon,
                                int x, int y, shCreature *attacker,
                                int attackmod, int dbonus /* = 0 */)
{
    const char *msg = beamHitsMesg (atk);
    int died = 0;
    int dmul = 0;
    int block = 0;
    int divider = 1;
    int dc = 15 + attackmod;
    shCauseOfDeath howdead = kSlain;

    shCreature *c = getCreature (x, y);
    if (!c or GameOver or kDead == c->mState) return 0;

    const char *the_monster = c->the ();

    if (atk->mType == shAttack::kTransporterRay) {
        int self = c == attacker;
        if (c->mHidden <= 0 and weapon) weapon->setKnown ();
        if (!self) c->newEnemy (attacker);
        died = c->transport (-1, -1, 100, 1);
        if (1 == died and self) return -2;
        return 0;
    }


    if (atk->mType == shAttack::kNoAttack) {
        /* Special things. */
    } else if (atk->mType == shAttack::kFlare) {
        msg = NULL;
    } else if (atk->mType == shAttack::kLight) {
        /* Shielding does not protect against flashbang grenade. */
        if (c->isBlind ()) { /* Does not even notice. */
            return 0;
        }
        if (c->reflexSave (atk, dc)) {
            if (c->isHero ()) {
                I->p ("You close your eyes just in time!");
            } else if (Hero.canSee (c)) {
                if (!c->isRobot ()) {
                    I->p ("%s managed to close eyes in time.", the_monster);
                }
            }
            return 0;
        }
    } else if (atk->mType == shAttack::kAcidSplash) {
        /* Acid splash is a slow projectile and is not stopped by shield. */
        dmul = 1;
        if (c == attacker) {
            return 0; /* That's your own acidic blood. */
        }
        if (c->isHero ()) {
            I->p ("You are splashed by acid!");
        } else if (Hero.canSee (c)) {
            I->p ("%s is splashed by acid!", the_monster);
        }
    } else if (atk->mType == shAttack::kFlash) {
        /* Shield does not protect against irradiation. */
        dmul = 1;
    } else if (atk->mType == shAttack::kEnsnare) {
        if (c->isHero ()) {
            I->p ("You are ensnared within webs.");
        } else if (Hero.canSee (c)) {
            I->p ("%s is ensnared within webs.", the_monster);
        }
        dmul = 1;
    } else if (c->hasShield () and c->countEnergy ()) {
        /* The shield is big, so it's always hit. */
        block = 1;
        dmul = 1;
        if (atk->mType == shAttack::kBlast) {
            if (c->isHero ()) {
                I->p ("You are caught in the blast!");
            } else {
                I->p ("%s is caught in the blast!", the_monster);
            }
        }
    } else if (atk->dealsEnergyType (kDisintegrating)) {
        dmul = 1; /* Means death anyway, no point in dodging. */
        if (c->isA (kMonMonolith) and
            atk->mType == shAttack::kDisintegrationRay)
        {
            if (Hero.canSee (c))
                I->p ("The antimatter stream harmlessly enters the monolith.");
            return 1; /* Block the effect. */
        }
    } else if (atk->mType == shAttack::kBlast or
               (atk->mType == shAttack::kExplode and
                atk->mDamage[0].mEnergy != kPsychic) or
               atk->mType == shAttack::kIncendiary)
    {
        /* save for half damage */
        dmul = 1;
        if (c != attacker and !c->is (kParalyzed) and !c->is (kAsleep) and
            c->reflexSave (atk, dc))
        {
            ++divider;
            const char *what = (atk->mType == shAttack::kIncendiary) ?
                "flames" : "blast";
            if (c->isHero ()) {
                I->p ("You duck some of the %s.", what);
            } else {
                I->p ("%s ducks some of the %s.", the_monster, what);
            }
        } else {
            const char *what = (atk->mType == shAttack::kIncendiary) ?
                "engulfed by the flames" : "caught in the blast";
            if (c->isHero ()) {
                I->p ("You are %s!", what);
            } else {
                I->p ("%s is %s!", the_monster, what);
            }
        }
    } else if (atk->mType == shAttack::kOpticBlast or
               atk->mType == shAttack::kLaserBeam)
    {
        dmul = attacker->rangedAttackHits (atk, weapon, attackmod, c, &dbonus);
        if (dmul <= 0) {
            if (!c->isHero () and attacker and
                (attacker->isHero () or attacker->isPet ()))
            {
                c->newEnemy (attacker);
            }
            return 0;
        }
    } else if (atk->mType == shAttack::kShot) {
        dmul = 1; /* Shrapnel of sawn-off shotgun can't be dodged. */
    } else if (atk->mType == shAttack::kPlague) {
        dmul = 1;
    } else if (atk->mType == shAttack::kPsionicStorm) {
        dmul = 1;
    } else if (c != attacker and !c->isSessile () and !c->is (kParalyzed) and
              !c->is (kAsleep) and c->reflexSave (atk, dc))
    {
        const char *attack_desc;
        if (weapon and weapon->isA (kRayGun) and
            (!weapon->isKnown () or !weapon->isAppearanceKnown () or
             Hero.isBlind ()))
        {
            attack_desc = "ray";
        } else {
            attack_desc = atk->noun ();
        }
        /* save for no damage */
        if (c->isHero ()) {
            I->p ("You dodge the %s!", attack_desc);
        } else {
            if (Hero.canSee (c) or Hero.canHearThoughts (c)) {
                I->p ("%s dodges the %s.", the_monster, attack_desc);
            }
            if (attacker and
                (attacker->isHero () or attacker->isPet ()))
            {
                c->newEnemy (attacker);
            }
        }
        return 0;
    } else {
        dmul = 1;
        if (atk->mType == shAttack::kBlast) {
            if (c->isHero ()) {
                I->p ("You are caught in the blast!");
            } else {
                I->p ("%s is caught in the blast!", the_monster);
            }
        }
    }

    if (Hero.canSee (x, y)) {
        setSpecialEffect (x, y, beamSpecialEffect (atk, kOrigin));
        drawSq (x, y);
    }
    //I->cursorOnHero ();

    if (shAttack::kLaserBeam == atk->mType or
        shAttack::kOpticBlast == atk->mType)
        block = 1;

    if (c->isHero ()) {
        if (msg) {
            if (!Hero.isBlind()) {
                I->p ("%s you!", msg);
                if (weapon) {
                    weapon->setKnown ();
                    weapon->setAppearanceKnown ();
                }
            } else {
                msg = beamHitsBlindYouMesg (atk);
                if (msg) I->p ("%s", msg);
            }
        } else if (weapon and weapon->mOwner == &Hero) {
            if (weapon->isA (kObjHealingRayGun) or
                weapon->isA (kObjRestorationRayGun) or
                weapon->isA (kObjStasisRayGun))
            {
                weapon->setKnown ();
                weapon->setAppearanceKnown ();
            }
        }
        draw ();
        //I->pauseXY (Hero.mX, Hero.mY);
        if (Hero.sufferDamage (atk, attacker, weapon, dbonus, dmul, divider)) {
            if (attacker == c) {
                died = -2;
            }
            if (atk->mType == shAttack::kDisintegrationRay) {
                c->die (kAnnihilated, "a disintegration ray");
            } else {
                char deathbuf[50];
                if (attacker and !attacker->isHero () and c->isHero ()) {
                    Hero.resetBlind ();
                    Level->setLit (attacker->mX, attacker->mY, 1, 1, 1, 1);
                    Level->mVisibility [attacker->mX][attacker->mY] = 1;
                    if (attacker->isA (kMonDalek)) {
                        strncpy (deathbuf, AN (attacker), 50);
                    } else {
                        snprintf (deathbuf, 50, "%s's %s",
                                 AN (attacker),
                                  weapon ? weapon->getShortDescription ()
                                         : atk->noun ());
                    }
                } else {
                    snprintf (deathbuf, 50, "%s %s",
                              isvowel (atk->noun ()[0]) ? "an" : "a",
                              atk->noun ());
                }
                c->die (kSlain, attacker, weapon, atk, deathbuf);
            }
        }
    } else {
        if (Hero.canSee (c) or Hero.canHearThoughts (c)) {
            if (msg) {
                if (Hero.canSee (c)) {
                    draw ();
                }
                I->p ("%s %s!", beamHitsMesg (atk), the_monster);
            }
        }
        if (c->sufferDamage (atk, attacker, weapon, dbonus, dmul, divider)) {
            if (Hero.canSee (c) or Hero.canHearThoughts (c)) {
                if (shAttack::kDisintegrationRay == atk->mType or
                   (shAttack::kBlast == atk->mType and
                    kDisintegrating == atk->mDamage[0].mEnergy))
                {
                    howdead = kAnnihilated;
                } else if (shAttack::kGaussRay == atk->mType) {
                    howdead = kMisc;
                }
                if (atk->mEffect != shAttack::kExtend)
                    c->pDeathMessage (the_monster, howdead);
            }
            if (attacker == c) {
                died = -2;
            }
            c->die (howdead, attacker, weapon, atk);
        } else { /* Creature survived the attack. */
            if (attacker and atk->mType != shAttack::kNoAttack and
                (attacker->isHero () or attacker->isPet ()))
            { /* Get angry at player or pets unless the attack was harmless. */
                c->newEnemy (attacker);
            }
            c->interrupt ();
        }
    }

    return died ? -2 : block;
}

/*
static void
zapBeamAtCeiling (shCreature *attacker, shAttack *atk, shObject *weapon,
    int x, int y)
*/
static void
zapBeamAtFloor (shCreature *attacker, shAttack *atk, shObject *weapon,
    int x, int y)
{
    shObjectVector *v;
    if (shAttack::kDisintegrationRay == atk->mType) {
        if (!Level->isInGarbageCompactor (x, y))
        {
            shFeature *f = Level->getFeature (x, y);
            if (f) Level->removeFeature (f);
            Level->addTrap (x, y, shFeature::kHole);
            Level->checkTraps (x, y, 100);
        }
    } else if (shAttack::kTransporterRay == atk->mType) {
        if (Level->noTransport ()) return;
        v = Level->getObjects (x, y);
        if (v) {
            weapon->setKnown ();
            for (int i = v->count () - 1; i >= 0; --i) {
                shObject *obj = v->removeByIndex (i);
                int total_qty = obj->mCount;
                int unpaid = obj->isUnpaid ();
                int nx, ny;
                obj->resetUnpaid ();
                while (obj->mCount > 1) {
                    shObject *one = obj->split (1);
                    Level->findLandingSquare (&nx, &ny);
                    Level->putObject (one, nx, ny);
                }
                Level->findLandingSquare (&nx, &ny);
                /* Do check before putting last object since it may get
                   destroyed by landing in acid pit or other such place. */
                if (unpaid) {
                    obj->setUnpaid ();
                    Hero.usedUpItem (obj, total_qty, "transport");
                    obj->resetUnpaid ();
                }
                Level->putObject (obj, nx, ny);
            }
            /* Keep in mind objects may land in the original square too. */
            if (!v->count ()) {
                delete v;
                Level->setObjects (x, y, NULL);
            }
        }
    } else if (shAttack::kGammaRay == atk->mType) {
        Level->getSquare (x, y)->mFlags |= shSquare::kRadioactive;
    } else if (shAttack::kDecontaminationRay == atk->mType) {
        if (!Level->isPermaRadioactive ())
            Level->getSquare (x, y)->mFlags &= ~shSquare::kRadioactive;
        shFeature *f = Level->getFeature (x, y);
        if (f and f->mType == shFeature::kVat) {
            f->mVat.mRadioactive = 0;
        }
    }

    bool dig = RNG (100) < atk->dealsEnergyType (kDigging);
    bool disin = RNG (100) < atk->dealsEnergyType (kDisintegrating);
    if (dig or disin) {
        bool hole = disin;
        shFeature *f = Level->getFeature (x, y);
        if (f) { /* TODO */
            if (Level->isBottomLevel ())
                return;
            if (f->mType != shFeature::kPit and
                f->mType != shFeature::kAcidPit and
                f->mType != shFeature::kSewagePit)
                return; /* TODO */
            Level->removeFeature (f);
            hole = true;
        }
        if (hole) {
            Level->addTrap (x, y, shFeature::kHole);
        } else {
            Level->addTrap (x, y, shFeature::kPit);
        }
        f = Level->getFeature (x, y);
        f->mTrapUnknown = 0;
        if (Hero.canSee (x, y)) {
            I->p ("%s dig%s a %s.", attacker->the (),
                attacker->isHero () ? "" : "s", hole ? "hole" : "pit");
        }
    }
}

static int
smartDiscPath (shAttack *attack, shObject *weapon, int *x, int *y,
               shDirection dir, shCreature *attacker, int attackmod, int dbonus)
{
    while (Level->moveForward (dir, x, y)) {
        shSpecialEffect eff = beamSpecialEffect (attack, dir);
        if (eff and !Hero.isBlind ()) {
            Level->setSpecialEffect (*x, *y, eff);
            Level->drawSq (*x, *y);
        }

        if (Level->isOccupied (*x, *y)) {
            shCreature *c = Level->getCreature (*x, *y);
            if (c == attacker) { /* Get back to hand. */
                if (attacker->addObjectToInventory (weapon, 1)) {
                    if (!attacker->mWeapon) {
                        attacker->wield (weapon, 1);
                    }
                } else {
                    Level->putObject (weapon, *x, *y);
                }
                Level->setSpecialEffect (*x, *y, kNone);
                return 0;
            }
            if (!c->isFriendly (attacker) or !weapon->isOptimized ()) {
                int r = attacker->resolveRangedAttack (attack, weapon, 0, c);
                if (r >= 0 and c->hasShield ()) { /* Shields stop discs. */
                    Level->putObject (weapon, *x, *y);
                    return 0;
                }
            }
        }
        if (Level->isObstacle (*x, *y)) {
            break;
        }
        if (eff) {
            I->pauseXY (Hero.mX, Hero.mY, 10);
            Level->setSpecialEffect (*x, *y, kNone);
        }
    }
    Level->setSpecialEffect (*x, *y, kNone);
    Level->moveForward (uTurn (dir), x, y);
    return 1;
}


static int
extendAttackHitObstacle (shObject *weapon, shCreature *attacker)
{
    if (weapon->isOptimized () and RNG (4)) {
        if (attacker->isHero ())  I->p ("Bonk!");
        return 0;
    }
    if (weapon->mEnhancement == -5) {
        if (attacker->isHero ())
            I->p ("%s breaks!", YOUR (weapon));
        attacker->doff (weapon);
        attacker->removeObjectFromInventory (weapon);
        delete weapon;
        if (attacker->isHero () and Hero.mProfession == Yautja) {
            Level->clearSpecialEffects ();
            I->p ("THE SACRILEGE!!!  Having defiled your honor you take your life.");
            attacker->die (kSuicide);
            return -2;
        }
    } else {
        if (attacker->isHero ())
            I->p ("You damaged %s.", YOUR (weapon));
        --weapon->mEnhancement;
    }
    return 0;
}

static void
explosionMessage (shObject *weapon, int x, int y)
{
    bool seen = Hero.canSee (x, y);
    bool los = Level->existsLOS (Hero.mX, Hero.mY, x, y);
    if (los and weapon->mIlkId == kObjFlare) {
        I->p ("%s lights up.", THE (weapon));
        return;
    }
    if (weapon->mIlkId == kObjFlashbang) {
        I->p ("Poof!");
        return;
    }
    if (weapon->isA (kObjGenericGrenade)) {
        I->p ("Boom!");
        return;
    }
    /* Effects that require sight. */
    if (!seen)  return;
}

/* returns -2 if attacker dies */
int
shMapLevel::areaEffect (shAttack *atk, shObject *weapon, int x, int y,
                        shDirection dir, shCreature *attacker,
                        int attackmod, int dbonus /* = 0 */)
{
    if (weapon)  explosionMessage (weapon, x, y);
    int u, v;
    int res;
    int died = 0;
    int seen = 0;
    int firsttarget = 1;
    shMapLevel *savelev = Level; /* Some area effects may cause hero to fall
    down a hole.  Then any leftover flashy things are not to be drawn. */


    /* General rule: affect any features first, then creatures,
       because the death of a monster might cause an automatic door to
       close and get blown up, when it shouldn't have been affected.

       Next affect any objects because most probably you don't want to
       affect dropped inventories by dead monsters.  Creatures go last.
    */

    switch (atk->mEffect) {
    case shAttack::kSingle: /*something is wrong */
    case shAttack::kOther:
        abort ();
    case shAttack::kBeam:
    case shAttack::kExtend:
    {
        int range = atk->mRange;
        if (atk->mEffect == shAttack::kBeam)
            range += RNG (1, 6);

        for (int i = range; i > 0; --i) {
            if (!moveForward (dir, &x, &y)) {
                break; /* Out of map. */
            }
            if (kUp == dir and attacker->isHero ()) {
                break;
            }
            if (!isFloor (x, y)) { /* Obstacles stop most such attacks. */
                if (atk->mType == shAttack::kDisintegrationRay or
                    RNG (100) < atk->dealsEnergyType (kDigging))
                {
                    Level->dig (x, y);
                } else {
                    if (atk->mEffect == shAttack::kExtend and weapon) {
                        int res = extendAttackHitObstacle (weapon, attacker);
                        if (res) return res;
                    }
                    break;
                }
            }
            /* Draw beam effect if it can be seen. */
            if (!Hero.isBlind () and
                beamSpecialEffect (atk, dir) and kDown != dir and
                appearsToBeFloor (x, y) and
                (Hero.canSee (x, y) or
                 (atk->isLightGenerating () and
                  existsLOS (Hero.mX, Hero.mY, x, y))))
            {
                shFeature *f = Level->getFeature (x, y);
                if (f and f->isObstacle ())
                    setSpecialEffect (x, y, beamSpecialEffect (atk, kOrigin));
                else
                    setSpecialEffect (x, y, beamSpecialEffect (atk, dir));
                draw ();
                seen++;
            }

            /* Bounce off creature having reflection? */
            shCreature *c = getCreature (x, y);
            if (c and c->reflectAttack (atk, &dir)) {
                if (!Hero.isBlind ()) {
                    setSpecialEffect (x, y, beamSpecialEffect (atk, kOrigin));
                    draw ();
                    seen++;
                }
                continue;
            }

            /* Bounce off force field or some such feature? */
            res = areaEffectFeature (atk, weapon, x, y, attacker, dbonus);
            if (res < 0) {
                dir = reflectBackDir (dir);
                if (!Hero.isBlind ()) {
                    setSpecialEffect (x, y, beamSpecialEffect (atk, kOrigin));
                    draw ();
                    seen++;
                }
                continue;
            } else if (res > 0) {
                if (atk->mEffect == shAttack::kExtend and weapon) {
                    int res = extendAttackHitObstacle (weapon, attacker);
                    if (res) return res;
                }
                break;
            }

            areaEffectObjects (atk, weapon, x, y, attacker, dbonus);
            if (atk->mEffect == shAttack::kExtend) {
                I->pauseXY (Hero.mX, Hero.mY, 33);
                if (!Hero.isBlind ()) {
                    setSpecialEffect (x, y, beamSpecialEffect (atk, dir, false));
                    draw ();
                }
            }
            /* Did we hit that creature in beam path? */
            if (c and kDown != dir) {
                int tohitmod = 0;
                if (!attacker->isHero () and !c->isHero ()) {
                    tohitmod -= 8; /* Avoid friendly fire. */
                } else if (!firsttarget) {
                    tohitmod -= 4;
                }
                firsttarget = 0;
                if (c == attacker and dir == kOrigin) /* Shooting self. */
                    tohitmod = +100;
                int what =
                    areaEffectCreature (atk, weapon, x, y, attacker,
                                        attackmod + tohitmod, dbonus);
                if (-2 == what) {
                    died = -2;
                } else if (1 == what) {
                    break;
                }
            }
            if (kDown == dir) {
                zapBeamAtFloor (attacker, atk, weapon, x, y);
                break;
            }
            if (kOrigin == dir or kNoDirection == dir or kUp == dir) {
                break;
            }
            if (seen and (atk->mType == shAttack::kLaserBeam or
                          atk->mType == shAttack::kOpticBlast))
            {
                /* In NotEye mode this requires corners to be drawn. */
                int cr1x, cr1y, cr2x, cr2y;
                shSpecialEffect cr1 = kNone, cr2 = kNone;
                switch (dir) {
                case kNorthWest:
                    cr1x = x; cr1y = y - 1; cr1 = kLaserBeamSWCEffect;
                    cr2x = x - 1; cr2y = y; cr2 = kLaserBeamNECEffect;
                    break;
                case kNorthEast:
                    cr1x = x; cr1y = y - 1; cr1 = kLaserBeamSECEffect;
                    cr2x = x + 1; cr2y = y; cr2 = kLaserBeamNWCEffect;
                    break;
                case kSouthEast:
                    cr1x = x; cr1y = y + 1; cr1 = kLaserBeamNECEffect;
                    cr2x = x + 1; cr2y = y; cr2 = kLaserBeamSWCEffect;
                    break;
                case kSouthWest:
                    cr1x = x; cr1y = y + 1; cr1 = kLaserBeamNWCEffect;
                    cr2x = x - 1; cr2y = y; cr2 = kLaserBeamSECEffect;
                    break;
                default: break;
                }
                if (atk->mType == shAttack::kOpticBlast) {
                    /* The fact optic blast is listed directly under
                       laser beam is abused here. */
                    if (cr1)  cr1 = shSpecialEffect (int (cr1) << 1);
                    if (cr2)  cr2 = shSpecialEffect (int (cr2) << 1);
                }
                if (cr1 and Level->isFloor (cr1x, cr1y))
                    setSpecialEffect (cr1x, cr1y, cr1);
                if (cr2 and Level->isFloor (cr2x, cr2y))
                    setSpecialEffect (cr2x, cr2y, cr2);
                if (cr1)  draw ();
            }
        }
        if (seen) {
            if (Level == savelev and kDead != Hero.mState) {
                int times100 = 0;
                draw ();
                if (atk->mEffect == shAttack::kBeam) {
                    times100 = 3;
                } else if (atk->mEffect == shAttack::kExtend) {
                    /* no additional delay */
                }
                for (int i = 0; i < times100; ++i) {
                    draw ();
                    I->pauseXY (Hero.mX, Hero.mY, 100);
                }
            }
            savelev->clearSpecialEffects ();
            draw ();
        }
        return died;
    }
    /* Radius means how many squares to travel before widening. */
    /*      kCone area effect attack picture for
     *          radius = 2       range = 7
     *
     * gggg
     * gfff
     * gfeee
     * gfedd
     *   edcc        g
     *     cb      efg
     *       a   cdefg
     *        @abcdefg
     *           cdefg
     *             efg
     *               g
     */
    case shAttack::kCone:
    {
        struct shFlameTongue {
            int x;
            int y;
            int present;
        } flame[20];
        int flames = 1;
        flame[0].x = x;
        flame[0].y = y;
        flame[0].present = 1;
        for (int r = 0; r < atk->mRange; ++r) {
            if (r % atk->mRadius == 0 and r != 0) { /* Widen cone. */
                /* Only middle stream may expand. */
                if (flame[0].present) {
                    flame[flames].x = flame[0].x;
                    flame[flames].y = flame[0].y;
                    flame[flames++].present = 1;
                    flame[flames].x = flame[0].x;
                    flame[flames].y = flame[0].y;
                    flame[flames++].present = 1;
                }
            }   /* Process all flames. */
            for (int fl = 0; fl < flames; ++fl) {
                shDirection newdir;
                /* Turn to side? */
                if (r % atk->mRadius == 0 and r != 0 and fl != 0) {
                    newdir = fl % 2 ? leftTurn (dir) : rightTurn (dir);
                } else { /* Or push forward. */
                    newdir = dir;
                }
                if (flame[fl].present and
                    moveForward (newdir, &flame[fl].x, &flame[fl].y))
                {
                    if (!isFloor (flame[fl].x, flame[fl].y)) {
                        continue;
                    }
                    if (!Hero.isBlind () and
                        appearsToBeFloor (flame[fl].x, flame[fl].y) and
                        (Hero.canSee (x, y) or atk->isLightGenerating ()))
                    {
                        shSpecialEffect eff =
                            beamSpecialEffect(atk, newdir);
                        setSpecialEffect (flame[fl].x, flame[fl].y, eff);
                        draw ();
                        seen++;
                    }
                    int feature_blocks =
                        areaEffectFeature (atk, weapon,
                            flame[fl].x, flame[fl].y, attacker, dbonus);
                    areaEffectObjects (atk, weapon,
                        flame[fl].x, flame[fl].y, attacker, dbonus);
                    int creature_blocks =
                    areaEffectCreature (atk, weapon,
                        flame[fl].x, flame[fl].y, attacker, dbonus);
                    /* Flame tongue might be blocked or absorbed. */
                    flame[fl].present = !feature_blocks and !creature_blocks;
                } else { /* Hit obstacle. */
                    flame[fl].present = 0;
                }
            }
        }
        if (seen and kDead != Hero.mState and effectsInEffect ()) {
            for (int i = 0; i < 6; ++i) {
                draw ();
                I->pauseXY (Hero.mX, Hero.mY, 50);
            }
            clearSpecialEffects ();
            draw ();
        }
        return died;
    }
    case shAttack::kBurst:
    case shAttack::kFarBurst:
    {
        bool disintegrating = atk->dealsEnergyType (kDisintegrating);
        for (u = x - atk->mRadius; u <= x + atk->mRadius; u++) {
            for (v = y - atk->mRadius; v <= y + atk->mRadius; v++) {
                if (isInBounds (u, v)
                    and distance (u, v, x, y) <= 5 * atk->mRadius + 2
                    /* Only flares and antimatter affect walls. */
                    and (isFloor (u, v) or
                         (weapon and weapon->isA (kObjFlare)) or
                         disintegrating)
                    and (existsLOS (x, y, u, v) or disintegrating))
                {
                    if (disintegrating and !isFloor (u, v))  dig (u, v);
                    if (!Hero.isBlind () and appearsToBeFloor (u, v) and
                        (Hero.canSee (x, y) or atk->isLightGenerating ()))
                    {
                        shSpecialEffect eff =
                            beamSpecialEffect(atk, kNoDirection);
                        setSpecialEffect (u, v, eff);
                        draw ();
                        seen++;
                    }
                    areaEffectFeature (atk, weapon, u, v, attacker, dbonus);
                }
            }
        }
        for (u = x - atk->mRadius; u <= x + atk->mRadius; u++) {
            for (v = y - atk->mRadius; v <= y + atk->mRadius; v++) {
                if (isInBounds (u, v)
                    and distance (u, v, x, y) <= 5 * atk->mRadius + 2
                    and (existsLOS (x, y, u, v) or disintegrating))
                {
                    areaEffectObjects (atk, weapon, u, v, attacker, dbonus);
                    if (-2 == areaEffectCreature (atk, weapon, u, v, attacker,
                                                  dbonus))
                    {
                        died = -2;
                    }
                }
            }
        }
        if (seen and kDead != Hero.mState) {
            /* Do not move effectsInEffect check to outer conditional
               check because nested calling (a chain of explosions) may
               disable this and prevent identification of grenades! -- MB */
            if (effectsInEffect ()) {
                for (int i = 0; i < 6; ++i) {
                    draw ();
                    I->pauseXY (Hero.mX, Hero.mY, 50);
                }
            } /* Was the item causing explosion a grenade/canister? */
            if (weapon and weapon->myIlk ()->mMissileAttack) {
                weapon->setKnown ();
            }
            clearSpecialEffects ();
            draw ();
        }
        return died;
    }
    case shAttack::kSmartDisc:
    {
        int start_x = x, start_y = y;
        if (kOrigin == dir or kUp == dir) {
            if (attacker->isHero ()) {
                if (kOrigin == dir) {
                    I->p ("You toss the disc from hand to hand.");
                } else {
                    I->p ("You throw the disc up and catch it as it falls.");
                }
            }
            if (attacker->addObjectToInventory (weapon)) {
                if (!attacker->mWeapon) {
                    attacker->wield (weapon, 1);
                }
            } else {
                Level->putObject (weapon, x, y);
            }
            return 0;
        } else if (kDown == dir) {
            Level->putObject (weapon, x, y);
            return 0;
        }

        /* First phase: fly forward until obstacle is hit. */
        if (!smartDiscPath (atk, weapon, &x, &y, dir, attacker,
                            attackmod, dbonus))
        {
            return 0;
        }
        if (x == start_x and y == start_y) { /* Immediately hit a wall. */
            Level->putObject (weapon, x, y);
            return 0;
        }
        /* Second phase: determine direction to bounce. */
        int bnc_x = x, bnc_y = y;
        struct shScoreDir {
            int pts;
            shDirection dir;
        } score[8];
        for (int i = 0; i < 8; ++i) {
            score[i].pts = 0;
            score[i].dir = (shDirection) i;
            if (i == dir or i == uTurn (dir) or
                i == leftTurn (dir) or i == rightTurn (dir))
            {
                continue;
            }
            x = bnc_x; y = bnc_y;
            /* Check chosen direction. */
            while (Level->moveForward ((shDirection) i, &x, &y)) {
                if (Level->isOccupied (x, y)) {
                    shCreature *c = Level->getCreature (x, y);
                    if (!c->isFriendly (attacker) or !weapon->isOptimized ()) {
                        score[i].pts += 2; /* Count targets on this line. */
                    }
                }
                if (Level->isOcclusive (x, y)) {
                    break;
                }
            }
            Level->moveForward (uTurn (i), &x, &y);
            /* Can thrower be reached from this point? */
            shDirection ricochet = linedUpDirection (x, y, start_x, start_y);
            if (ricochet != kNoDirection and ricochet != uTurn (dir) and
                existsLOS (x, y, start_x, start_y)) /* No obstacles? */
            {
                /* That plus one will matter when someone throws disc
                   into empty area for practice or toying around. */
                score[i].pts = score[i].pts * 100 + 1;
            }
        }
        x = bnc_x; y = bnc_y;
        /* Third phase: choose direction. */
        shuffle (score, 8, sizeof (shScoreDir)); /* Break ties randomly. */
        int best = 0;
        for (int i = 1; i < 8; ++i) {
            if (score[i].pts > score[best].pts) best = i;
        }
        if (!score[best].pts) { /* Thud! No enemies or way to return found. */
            Level->putObject (weapon, x, y);
            break;
        }
        dir = score[best].dir;
        /* Fourth phase: fly next part. */
        if (attacker->isHero () or Hero.canSee (x, y)) {
            weapon->setKnown (); /* Saw disc bounce and continue flight. */
        }
        if (!smartDiscPath (atk, weapon, &x, &y, dir, attacker,
                            attackmod, dbonus))
        {
            return 0;
        }
        /* Fifth phase: fly until point of origin is reached if possible. */
        if ((dir = linedUpDirection (x, y, start_x, start_y)) != kNoDirection) {
            if (smartDiscPath (atk, weapon, &x, &y, dir, attacker,
                           attackmod, dbonus))
                Level->putObject (weapon, x, y);
        } else {
            Level->putObject (weapon, x, y);
        }
        break;
    }
    }
    return 0;
}
