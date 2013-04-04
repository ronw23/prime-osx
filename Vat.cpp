/********************************

      Quaffing from Vats

********************************/

#include "Global.h"
#include "Util.h"
#include "Object.h"
#include "Hero.h"
#include "Interface.h"

/* Produces random vats for matter compiler. */
shFeature *
shFeature::newVat ()
{
    shFeature *vat = new shFeature ();
    vat->mType = shFeature::kVat;
    vat->mVat.mHealthy = RNG (-2, 2);
    vat->mVat.mRadioactive = RNG (10) > 0;
    vat->mVat.mAnalyzed = RNG (2);
    return vat;
}

/* Creates random vats for hero to find on map. */
void
shMapLevel::addVat (int x, int y)
{
    shFeature *vat = shFeature::newVat ();
    vat->mX = x;
    vat->mY = y;
    /* Vats on map usually are unhealthy. */
    if (RNG (7)) vat->mVat.mHealthy = 0;
    vat->mVat.mAnalyzed = 0;
    mFeatures.add (vat);
}

void
shHero::quaffFromVat (shFeature *vat)
{
    assert (vat->mX == Hero.mX and vat->mY == Hero.mY);

    int dryup = 3;
    int result;

    if (isInShop ()) {
        quaffFromOwnedVat (vat);
    }

    if (vat->mVat.mHealthy <= -3) {
        /* You need to pour some nasty stuff in to achieve this. */
        result = -2;
    } else if (vat->mVat.mRadioactive and !RNG (10 + vat->mVat.mHealthy)) {
        result = -1;
    } else if (RNG (2) < vat->mVat.mHealthy) {
        /* Healthy vats are quite likely to give a good effect, and they're
           the only way to get the gain ability effect. */
        result = RNG (0, 6);
    } else if (vat->mVat.mHealthy < 0) {
        /* Extraordinarily unhealthy vats may poison. */
        result = RNG (1, 10);
    } else {
        result = RNG (1, 9);
    }

    switch (result) {
        case -2:
            I->p ("Arrgghh!!  You vomit.");
            if (sufferDamage (kAttVatPlague)) {
                shCreature::die (kKilled, "a filthy sludge vat");
            }
            vat->mVat.mAnalyzed = 1;
            break;
        case -1:
            mRad += RNG (1, 100);
            I->p ("Ick!  That had a toxic taste!");
            break;
        case 0:
            Hero.gainAbility (false, 1);
            vat->mVat.mAnalyzed = 1;
            break;
        case 1:
            if (vat->mVat.mRadioactive) {
                getMutantPower ();
                dryup = 2;
                break;
            } /* else fall through... */
        case 2:
            mRad -= RNG (1, 200);
            mRad = maxi (0, mRad);
            if (!mRad)
                I->p ("You feel purified!");
            else
                I->p ("You feel less contaminated.");
            break;
        case 3:
            mHP += NDX (4, 6);
            mHP = mini (mHP, mMaxHP);
            I->p ("You feel better!");
            break;
        case 4:
        {
            int amt = RNG (1, 6);
            Hero.mPsiDrain -= amt;
            Hero.mAbil.mPsi += amt;
            I->p ("You feel invigorated!");
            break;
        }
        case 5:
            I->p ("Mmmm... bouncy bubbly beverage!");
            if (Hero.getStoryFlag ("impregnation")) {
                abortion (1); /* Here message given is slightly different. */
                I->p ("You feel the alien embryo inside you die.");
            }
            break;
        case 6:
            I->p ("Mmmm... hot fun!");
            if (Hero.needsRestoration ())
                Hero.restoration (RNG (2) + mini (3, vat->mVat.mHealthy));
            break;
        case 7:
            I->p ("Oops!  You fall in!  You are covered in slime!");
            damageEquipment (kAttVatCorrosion, kCorrosive);
            I->p ("You climb out of the vat.");
            break;
        case 8:
        {
            int x = mX;
            int y = mY;

            shMonster *monster = new shMonster (kMonVatSlime);
            if (0 != Level->findNearbyUnoccupiedSquare (&x, &y) or
                0 != Level->putCreature (monster, x, y))
            {
                I->p ("The sludge gurgles!");
            } else {
                I->p ("It's alive!");
            }
            break;
        }
        case 9:
            I->p ("You are jolted by an electric shock!");
            if (sufferDamage (kAttVatShock)) {
                shCreature::die (kKilled, "an improperly grounded sludge vat");
            }
            dryup = 2;
            break;
        case 10:
            I->p ("This stuff is poisonous!");
            Hero.abortion ();
            if (sufferDamage (kAttVatPoison)) {
                shCreature::die (kKilled, "drinking some unhealthy sludge");
            }
            dryup = 2;
            vat->mVat.mAnalyzed = 1;
            break;
    }

    if (!RNG (dryup)) {
        I->p ("The vat dries up!");
        Level->forgetFeature (vat->mX, vat->mY);
        Level->removeFeature (vat);
    }
}
