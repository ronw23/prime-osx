#include "Global.h"
#include "Util.h"
#include "Object.h"
#include "Hero.h"
#include "Creature.h"
#include <math.h>

static int
selectWeaponFireMode (shObject *gun)
{
    if (gun->isToggled ()) {
        I->p ("You switch your weapon to single fire mode.");
        gun->resetToggled ();
    } else {
        I->p ("You switch your weapon to burst mode.");
        gun->setToggled ();
    }
    return HALFTURN;
}

static int /* Mostly a YAFM. */
useAnalProbe (shObject *probe)
{
    if (Hero.mGlyph.mSym == 'g') { /* Reticulan. */
        Hero.sufferDamage (probe->myIlk ()->mMeleeAttack);
        probe->identify ();
        I->p ("Your experience tells you it is %s.", probe->inv ());
        return FULLTURN;
    } else {
        I->p ("You sick bastard! I am not going to do it!");
        return 0;
    }
}

static int
chargePlasmaCaster (shObject *caster)
{
    const int CHARGE_COST[2] = {24, 36}; /* Energy cells needed. */
    caster->setChargeKnown ();
    if (caster->mCharges == 2) {
        I->p ("You cannot charge it further.");
        return 0;
    } else {
        int next = caster->mCharges;
        if (Hero.countEnergy () >= CHARGE_COST[next]) {
            Hero.loseEnergy (CHARGE_COST[next]);
            ++caster->mCharges;
            caster->setChargeKnown ();
            I->p ("You charge the plasma caster.");
            return caster->mCharges * FULLTURN;
        } else {
            I->p ("You need %d energy to add %s charge.", CHARGE_COST[next],
                  next ? "second" : "first");
            return 0;
        }
    }
}


void
initializeWeapons ()
{
    AllIlks[kObjAnalProbe].mUseFunc = useAnalProbe;
    AllIlks[kObjPlasmaCaster].mUseFunc = chargePlasmaCaster;

    AllIlks[kObjPulseRifle].mUseFunc = selectWeaponFireMode;
    AllIlks[kObjM56Smartgun].mUseFunc = selectWeaponFireMode;
    AllIlks[kObjM57Smartgun].mUseFunc = selectWeaponFireMode;
}


int
shObject::isAmmo (shObject *weapon)
{
    if (NULL == weapon) {
        return isA (kProjectile);
    }
    return isA (weapon->myIlk ()->mAmmoType);
}


/* returns: 1 if the creature has enough ammo to shoot the weapon once,
              or if the weapon doesn't need ammo; 0 o/w
 */
int
shCreature::hasAmmo (shObject *weapon)
{
    shObjectIlk *ilk = weapon->myIlk ();
    shObjId ammo = weapon->isA (kRayGun) ? kObjNothing : ilk->mAmmoType;
    int i;
    int n = 0;

    if (kObjNothing == ammo) {
        if (weapon->isChargeable ()) {
            if (weapon->mCharges) {
                return 1;
            } else {
                if (isHero ()) {
                    weapon->setChargeKnown ();
                }
                return 0;
            }
        }
        return 1;
    }
    n = ilk->mAmmoBurst;
    if (kObjEnergyCell == ammo) {
        return countEnergy () >= n ? 1 : 0;
    }
    for (i = 0; n > 0 and i < mInventory->count (); i++) {
        shObject *obj = mInventory->get (i);
        if (obj->isA (ammo)) {
            n -= obj->mCount;
        }
    }
    return n <= 0 ? 1 : 0;
}


//RETURNS: number of rounds taken on success; 0 if no ammo available;
//         -1 if none were needed
//         For weapons that use energy cells, returns 1 or 0 depending on
//          whether or not enough cells were consumed

int
shCreature::expendAmmo (shObject *weapon, int cnt /* = 0 */ )
{
    shObjectIlk *ilk = weapon->myIlk ();
    shObjId ammo = ilk->mAmmoType;
    int i;
    int n = 0;

    if (0 == cnt) {
        if (weapon->isSelectiveFireWeapon () and !weapon->isToggled ()) {
            cnt = 1;
        } else {
            cnt = ilk->mAmmoBurst;
        }
    }
    if (weapon->isChargeable () and !weapon->isA (kObjPlasmaCaster)) {
        if (weapon->mCharges > cnt) {
            weapon->mCharges -= cnt;
        } else {
            cnt = weapon->mCharges;
            weapon->mCharges = 0;
        }
        return cnt;
    } else if (kObjNothing == ammo) {
        return -1;
    }
    if (kObjEnergyCell == ammo) {
        if (countEnergy () >= cnt) {
            loseEnergy (cnt);
            return 1;
        } else {
            return 0;
        }
    }

    for (i = 0; n < cnt and i < mInventory->count (); i++) {
        shObject *obj = mInventory->get (i);
        if (obj->isA (ammo)) {
            if (kObjBullet != ammo and obj->mCount < cnt) {
                return 0; /* Only bullet weapons may burst right now. */
            }
            int x = useUpSomeObjectsFromInventory (obj, cnt - n);
            if (x) {
                n += x;
                --i;
            }
        }
    }
    return n;
}
