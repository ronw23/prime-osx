#include <stdlib.h>

#include "AttackType.h"
#include "Global.h"
#include "Creature.h"
#include "Object.h"
#include "Hero.h"
#include "Attack.h"

#include "Attack.cpp"

const char *
shAttack::noun ()
{
    switch (mType) {
    case kNoAttack: return "non-attack";
    case kAcidBath: return "acid bath";
    case kAcidSplash: return "acid splash";
    case kAccelerationRay: return "acceleration ray";
    case kAnalProbe: return "probe";
    case kAttach: return "attachment";
    case kAugmentationRay: return "augmentation ray";
    case kBite: return "bite";
    case kBlast: return "explosion";
    case kBolt: return "bolt";
    case kBreatheBugs: return "cloud of bugs";
    case kBreatheFire: return "fiery breath";
    case kBreatheTime: return "time warp";
    case kBreatheTraffic: return "packet storm";
    case kBreatheViruses: return "cloud of viruses";
    case kChoke: return "choke";
    case kClaw: return "claw";
    case kClub: return "club";
    case kCombi: return "spearhead";
    case kCreditDraining: return "financial crises";
    case kCrush: return "grasp";
    case kCut: return "cut";
    case kDecelerationRay: return "deceleration ray";
    case kDecontaminationRay: return "decontamination ray";
    case kDisc: return "spinning edge";
    case kDisintegrationRay: return "disintegration ray";
    case kDrill: return "drill";
    case kEnsnare: return "ensnare";
    case kExplode:
        return mDamage[0].mEnergy == kPsychic ? "psychic wave" : "explosion";
    case kExtractBrain: return "neurosurgery";
    case kFaceHug: return "grasp";
    case kFlare: return "flare";
    case kFlash: return "flash";
    case kFreezeRay: return "freeze ray";
    case kGammaRay: return "gamma ray";
    case kGaussRay: return "gauss ray";
    case kGun: return "bullet";
    case kHeadButt: return "head butt";
    case kHealingRay: return "healing ray";
    case kHeatRay: return "heat ray";
    case kImpact: return "smash";
    case kIncendiary: return "flame burst";
    case kKick: return "kick";
    case kLightningBolt: return "lightning bolt";
    case kLaserBeam: return "laser beam";
    case kLegalThreat: return "legal threat";
    case kLight: return "blinding flash";
    case kMentalBlast: return "mental blast";
    case kOpticBlast: return "laser beam";
    case kPea: return "pea";
    case kPlague: return "plague vomit";
    case kPlasmaGlob: return "plasma glob";
    case kPoisonRay: return "poison ray";
    case kPrick: return "needle";
    case kPsionicStorm: return "psionic storm";
    case kPunch: return "punch";
    case kRestorationRay: return "restoration ray";
    case kQuill: return "quill";
    case kRail: return "rail";
    case kRake: return "rake";
    case kSaw: return "saw";
    case kScorch: return "scorch";
    case kShot: return "shot";
    case kSlash: return "edge";
    case kSlime: return "slime";
    case kSmash: return "smack";
    case kStab: return "stab";
    case kStomp: return "stomp";
    case kSpear: return "short spear";
    case kSpit: return "spittle";
    case kStasisRay: return "stasis ray";
    case kSting: return "stinger";
    case kSword: return "blade";
    case kTailSlap: return "tail";
    case kTouch: return "touch";
    case kTractorBeam: return "tractor beam";
    case kTransporterRay: return "transporter ray";
    case kWaterRay: return "water";
    case kWeb: return "web";
    case kWhip: return "whip";
    case kZap: return "zap";
    case kMaxAttackType: return "program bug";
    }
    return "attack";
}


static const char *
energyFormat (shEnergyType t) {
    switch (t) {
    case kHackNSlash: return "%d-%d hack 'n' slash";
    case kConcussive: return "%d-%d concussion";
    case kBullet: return "%d-%d bullet";
    case kExplosive: return "%d-%d explosion";
    case kShrapnel: return "%d-%d shrapnel";
    case kIrresistible: return "%d-%d damage";
    case kAccelerating: return "%d-%d speed up";
    case kBlinding: return "%d-%d blinding";
    case kBugging: return "%d-%d development anti-patterns";
    case kBurning: return "%d-%d fire";
    case kConfusing: return "%d-%d confusing";
    case kCorrosive: return "%d-%d acid";
    case kDigging: return "dig";
    case kDisintegrating: return "%d-%d atomization";
    case kDecelerating: return "%d-%d slow down";
    case kElectrical: return "%d-%d shock";
    case kFreezing: return "%d-%d ice";
    case kLaser: return "%d-%d laser";
    case kMagnetic: return "%d-%d gauss";
    case kMesmerizing: return "%d-%d hypnotic";
    case kParalyzing: return "%d-%d paralytic";
    case kParticle: return "%d-%d particle beam";
    case kPlasma: return "%d-%d plasma";
    case kRadiological: return "%d-%d rad";
    case kPsychic: return "%d-%d psi";
    case kSickening: return "%d-%d disease";
    case kSpecial: return "%d-%d";
    case kStunning: return "%d-%d stun";
    case kToxic: return "%d-%d toxin";
    case kTransporting: return "%d-%d transporting";
    case kViolating: return "%d-%d violating";
    case kWebbing: return "%d-%d webbing";
    case kNoEnergy: return NULL;
    case kMaxEnergyType: return "program bug";
    }
    return "%d-%d unknown";
}


const char *
energyDescription (shEnergyType t) {
    switch (t) {
    case kHackNSlash: return "hack and slash";
    case kConcussive: return "concussive damage";
    case kBullet: return "bullet damage";
    case kExplosive: return "explosive damage";
    case kShrapnel: return "shrapnel damage";
    case kAccelerating: return "acceleration";
    case kBlinding: return "blinding";
    case kBurning: return "fire";
    case kConfusing: return "confusion";
    case kCorrosive: return "acid";
    case kDecelerating: return "deceleration";
    case kDisintegrating: return "atomization";
    case kElectrical: return "electricity";
    case kLaser: return "laser damage";
    case kFreezing: return "freezing";
    case kMagnetic: return "magnetic effects";
    case kMesmerizing: return "falling asleep";
    case kParalyzing: return "paralysis";
    case kParticle: return "particle beams";
    case kPlasma: return "plasma";
    case kRadiological: return "radiation";
    case kPsychic: return "psychic attacks";
    case kSickening: return "sickness";
    case kStunning: return "stunning";
    case kToxic: return "toxins";
    case kViolating: return "violation";
    case kWebbing: return "spiderman";

    case kNoEnergy:
    case kBugging:
    case kDigging:
    case kSpecial:
    case kTransporting:
    case kIrresistible:
    case kMaxEnergyType:
        return NULL;
    }
    return NULL;
}

const char *
shAttack::damStr ()
{   /* (2-12 burning + 1-6 gauss), etc. */
    const char *format = energyFormat (mDamage[0].mEnergy);
    if (format) {
        char *buf = GetBuf ();
        int l = 0;
        l += snprintf (buf + l, SHBUFLEN - l, "(");
        for (int i = 0; i < 3 and format; ++i) {
            l += snprintf (buf + l, SHBUFLEN - l, format,
                           mDamage[i].mLow, mDamage[i].mHigh);
            if (i != 2) {
                format = energyFormat (mDamage[i+1].mEnergy);
                if (format) {
                    l += snprintf (buf + l, SHBUFLEN - l, " + ");
                }
            }
        }
        l += snprintf (buf + l, SHBUFLEN - l, ")");
        return buf;
    }
    return "";
}

int
shAttack::getThreatRange (shObject *weapon, shCreature *target)
{
    return weapon ? weapon->getThreatRange (target) : 20;
}

int
shAttack::getCriticalMultiplier (shObject *weapon, shCreature *target)
{
    return weapon ? weapon->getCriticalMultiplier () : 2;
}

int
shAttack::dealsEnergyType (shEnergyType type)
{
    int chance = 0;
    for (int i = 0; i < 3; ++i) {
        if (type == mDamage[i].mEnergy)
            chance = maxi (chance, mDamage[i].mChance);
    }
    return chance;
}

int
shAttack::findEnergyType (shEnergyType type)
{
    for (int i = 0; i < 3; ++i)
        if (type == mDamage[i].mEnergy)  return i;
    return -1;
}

int
shAttack::bypassesShield ()
{
    return mType == kRestorationRay or mType == kHealingRay or
           mType == kLight or mType == kFlash;
}

int
shAttack::isAbsorbedForFree (int damage)
{
    if (damage == 0) /* Primary damage always costs energy to absorb. */
        return 0;
    return mDamage[damage].mEnergy == kBlinding or
           mDamage[damage].mEnergy == kParalyzing;
}

int
shAttack::isLightGenerating ()
{ /* Should the attack be drawn over dark squares? */
    return mType == kHeatRay   or mType == kIncendiary
        or mType == kLaserBeam or mType == kLight
        or mType == kLightningBolt;
}

/* Returns false if damage of energy type 'e' has only a special effect.
 * In other words the energy type does not deal plain hit point damage.
 * Note that special effects includes annihilation which obliterates stuff
 * without reducing hit points. */
bool
shAttack::isLethal (shEnergyType e)
{
    switch (e) {
    case kHackNSlash: case kConcussive: case kBullet: case kExplosive:
    case kShrapnel: case kBurning: case kCorrosive: case kElectrical:
    case kLaser: case kFreezing: case kMagnetic: case kParticle: case kPsychic:
    case kPlasma: case kSickening: case kIrresistible:
        return true;
    case kNoEnergy: case kAccelerating: case kBlinding: case kBugging:
    case kConfusing: case kDecelerating: case kDigging: case kDisintegrating:
    case kMesmerizing: case kParalyzing: case kRadiological: case kSpecial:
    case kStunning: case kToxic: case kTransporting: case kViolating:
    case kWebbing: case kMaxEnergyType:
        return false;
    }
    /* Not reached unless someone adds new energy type and ignores
       compiler warning.  In which case it is just and right to crash. */
    assert (false);
    abort ();
    return false;
}

/* Where else could it belong? */
void
shHero::quaffFromAcidPit ()
{
    if (Hero.sufferDamage (kAttAcidPitSip)) {
        Hero.shCreature::die (kKilled, "drinking from an acid pit");
    }
    Hero.abortion ();
}
