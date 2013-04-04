#include "Global.h"
#include "Util.h"
#include "Object.h"
#include "ObjectIlk.h"
#include "Creature.h"
#include "Hero.h"

const char *
shObjectIlk::getRayGunColor ()
{
    char *buf = GetBuf ();
    char *p;
    strcpy (buf, mAppearance.mName);
    for (p = &buf[0]; ' ' != *p; ++p)
        ;
    *p = 0;
    return buf;
}


static const struct shCanToRayGun {
    shObjId mCanIlk, mGunIlk;
    int mDice, mSides;
} fillTable[] = {
    {kObjMutagenCanister,       kObjGammaRayGun,            2, 10},
    {kObjFullHealingCanister,   kObjHealingRayGun,          6,  3},
    {kObjRadAway,               kObjDecontaminationRayGun,  5,  3},
    {kObjWater,                 kObjSquirtRayGun,           3,  4},
    {kObjLNO,                   kObjFreezeRayGun,           3,  4},
    {kObjNapalm,                kObjHeatRayGun,             3,  4},
    {kObjAntimatter,            kObjDisintegrationRayGun,   3,  3},
    {kObjSuperGlue,             kObjStasisRayGun,           2,  6},
    {kObjPoisonCanister,        kObjPoisonRayGun,           2,  6},
    {kObjPlasmaCanister,        kObjGaussRayGun,            2,  6},
    {kObjSpiceMelange,          kObjTransporterRayGun,      2,  6},
    {kObjHealingCanister,       kObjHealingRayGun,          2,  6},
    {kObjSpeedCanister,         kObjAccelerationRayGun,     2,  5},
    {kObjSpeedCanister,         kObjDecelerationRayGun,     2,  5},
    {kObjRestorationCanister,   kObjRestorationRayGun,      2,  4}
};
static const int nfills = sizeof (fillTable) / sizeof (struct shCanToRayGun);

/* Returns time elapsed. */
static int
loadRayGun (shObject *gun)
{
    shObjectVector v;
    shObject *can;

    selectObjects (&v, Hero.mInventory, kCanister);
    can = Hero.quickPickItem (&v, "reload with", 0);
    if (!can)  return 0;

    int index;
    for (index = 0; index < nfills; ++index)
        if (fillTable[index].mCanIlk == can->mIlkId)
            break;

    bool loaded = false;

    if (can->isA (kObjGainAbilityCanister) and !can->isBuggy ()) {
        /* Buggy cans will ruin the ray gun. */
        gun->mIlkId = kObjAugmentationRayGun;
        gun->mCharges += can->isOptimized () ? NUM_ABIL : 1;
        loaded = true;
    } else if (index == nfills) {
        /* Remaining canisters that *should* ruin ray guns:
           beverages (beer, nano/nuka cola, bbb, water, coffee)
           universal solvent - damages player besides ruining ray gun?
           canned alien embryo
           brain
        */
        I->p ("Your ray gun is %s!",
              can->isA (kObjUniversalSolvent) ? "dissolved" : "ruined");
        can->maybeName ();
        Hero.useUpOneObjectFromInventory (can);
        Hero.useUpOneObjectFromInventory (gun);
        return FULLTURN;
    } else {
        int mod = 0;
        if (fillTable[index].mCanIlk == kObjSpeedCanister) {
            if (can->isBuggy ())  ++index; /* Use deceleration instead. */
        } else {
            mod = can->mBugginess;
        }
        gun->mIlkId = fillTable[index].mGunIlk;
        gun->mCharges += NDX (fillTable[index].mDice + mod, fillTable[index].mSides);
        loaded = true;
    }

    if (loaded) {
        if (!Hero.isBlind ()) {
            I->p ("The light on the ray gun is %s now.", gun->myIlk ()->getRayGunColor ());
        } else {
            gun->resetAppearanceKnown ();
        }
        if (can->isKnown ()) {
            gun->setKnown ();
        } else if (gun->isKnown () and !Hero.isBlind ()) {
            can->setKnown ();
        } else {
            can->maybeName ();
        }
    }

    //if (can->isKnown () and gun->isIlkKnown ())  gun->setAppearanceKnown ();
    Hero.useUpOneObjectFromInventory (can);

    return FULLTURN;
}

int
getChargesForRayGun (shObjId raygun)
{
    if (raygun == kObjAugmentationRayGun)
        return 1;
    for (int i = 0; i < nfills; ++i)
        if (fillTable[i].mGunIlk == raygun)
            return NDX (fillTable[i].mDice, fillTable[i].mSides);
    return 0;
}

void
initializeRayGuns (void)
{
    AllIlks[kObjEmptyRayGun].mUseFunc = loadRayGun;
}
