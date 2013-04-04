/*******************************

        Mutant Power Code

how to add a mutant power:
1. edit shMutantPower enum in Global.h
2. edit the little table in Mutant.h, and make sure the order is consistent
   with the enum!
3. write a usePower function, or startPower/stopPower functions
4. possibly write shCreature::Power function if you want monsters to use it too
5. add the appropriate skill to the Psion and maybe other character classes

********************************/

#include "Global.h"
#include "Util.h"
#include "Object.h"
#include "Hero.h"
#include "Interface.h"
#include "Mutant.h"

#define MUTPOWER_CURSOR 3 /* Tile number in kRowCursor for targeting powers. */

static int
shortRange (shCreature *c)
{
    return 5 + c->mCLevel / 4;
}

int
shCreature::getPsionicDC (int powerlevel)
{
    return 10 + powerlevel + ABILITY_MODIFIER (getPsi () + mPsiDrain);
}


/* roll to save against the psionic attack
   returns: 0 if save failed, true if made
*/

int
shCreature::willSave (int DC)
{
    int modifier =  mWillSaveBonus + ABILITY_MODIFIER (getPsi () + mPsiDrain);
    int result = RNG (1, 20) + modifier;

    if (Hero.isLucky ()) {
        if (isHero () or isPet ()) {
            result += RNG (1, 3);
        }
    }

    debug.log ("Rolled will save %d+%d=%d, dc %d",
               result - modifier, modifier, result, DC);

    return result >= DC;
}

/* Returns: 1 if defender has better torc otherwise 0. */
static int /* Prints message if psionic attack was deflected. */
torcComparison (shCreature *attacker, shCreature *defender)
{
    int result = attacker->mImplants[shObjectIlk::kNeck] and
                 defender->mImplants[shObjectIlk::kNeck] and
                 defender->mImplants[shObjectIlk::kNeck]->mBugginess >
                 attacker->mImplants[shObjectIlk::kNeck]->mBugginess;
    if (result) {
        if (attacker->isHero ()) {
            I->p ("%s prevents you from making a psionic attack against %s.",
                YOUR (attacker->mImplants[shObjectIlk::kNeck]), THE (defender));
        } else if (defender->isHero ()) {
            I->p ("Circuitry of %s prevents a psionic assault against you.",
                YOUR (defender->mImplants[shObjectIlk::kNeck]));
            //TODO: reveal creature
            //Level->drawSqCreature (attacker->mX, attacker->mY);
        }
    }
    return result;
}

int
shCreature::digestion (shObject *obj)
{   /* No screwing yourself over a miskeypress. */
    if ((obj->isA (kObjTheOrgasmatron) or
         obj->isA (kObjFakeOrgasmatron1) or
         obj->isA (kObjFakeOrgasmatron2) or
         obj->isA (kObjFakeOrgasmatron3) or
         obj->isA (kObjFakeOrgasmatron4) or
         obj->isA (kObjFakeOrgasmatron5)) and
         !obj->isAppearanceKnown ()) {
        I->p ("Dude, it *might* be the Bizarro Orgasmatron.  You may need *that* to win!");
        I->p ("Go lose the game some other (and more creative) way.");
        return 0;
    }
    if (obj->isA (kObjTheOrgasmatron)) {
        I->p ("Dude, this is the Bizarro Orgasmatron!  You need *that* to win!");
        I->p ("Go lose the game some other (and more creative) way.");
        return 0;
    } /* Knowingly eating other Orgasmatrons is fine. */
    if (isHero ()) {
        shObject *stack = NULL;
        if (obj->mCount > 1) {
            stack = obj;
            obj = removeOneObjectFromInventory (obj);
        } else {
            removeObjectFromInventory (obj);
        }
        I->p ("You devour %s.", YOUR (obj));
        if (obj->isInfected () and !Hero.mResistances[kSickening]) {
            Hero.inflict (kSickened, RNG (15, 40) * FULLTURN);
            if (stack) {
                stack->setInfectedKnown ();
                stack->announce ();
            }
        }
    } else {
        abort ();
    }
    delete obj;

    return FULLTURN;
}

int
shCreature::telepathy (int on)
{
    if (on) {
        mInnateIntrinsics |= kTelepathy;
    } else {
        mInnateIntrinsics &= ~kTelepathy;
    }
    Hero.computeIntrinsics ();
    return FULLTURN;
}

int
shCreature::tremorsense (int on)
{
    if (on) {
        mInnateIntrinsics |= kTremorsense;
    } else {
        mInnateIntrinsics &= ~kTremorsense;
    }
    Hero.computeIntrinsics ();
    return FULLTURN;
}

int
shCreature::hypnosis (shDirection dir)
{
    int x = mX;
    int y = mY;
    shCreature *target;

    if (mLevel->moveForward (dir, &x, &y) and
        (target = mLevel->getCreature (x, y)))
    {
        if (torcComparison (this, target)) return HALFTURN;
        int saved = target->hasBrainShield () or
                    target->willSave (getPsionicDC (1));

        if (target->isHero ()) {
            if (!Hero.canSee (this)) {
                /* You can't see the hypnotic gaze. */
            } else if (saved) {
                I->p ("You blink.");
            } else { /* Message handled by sufferDamage. */
                target->sufferDamage (kAttHypnoGaze);
            }
        } else {
            const char *t_buf;
            if (Hero.canSee (target)) {
                t_buf = THE (target);
            } else {
                t_buf = "it";
            }
            if (!target->canSee (this)) {
                I->p ("%s doesn't seem to notice your gaze.", t_buf);
            } else if (!target->hasMind ()) {
                I->p ("%s is unaffected.", t_buf);
            } else if (saved) {
                I->p ("%s resists!", t_buf);
            } else {
                target->sufferDamage (kAttHypnoGaze);
            }
        }
    }
    return HALFTURN;
}

int
shCreature::shootWeb (shDirection dir)
{
    int x, y, r;
    int maxrange = shortRange (this);
    int elapsed = QUICKTURN;
    int attackmod;

    shSkill *skill = getSkill (kMutantPower, kShootWebs);

    shAttack *WebAttack = &Attacks[kAttShootWeb];
    WebAttack->mDamage[0].mLow = 1 + mCLevel / 2 + (skill ? skill->mRanks / 2 : 0);
    WebAttack->mDamage[0].mHigh = 6 + (skill ? skill->mRanks : 0) + mCLevel / 2;

    attackmod = mToHitModifier + getSkillModifier (kMutantPower, kShootWebs);

    if (kUp == dir) {
        return elapsed;
    } else if (kDown == dir or kOrigin == dir) {
        resolveRangedAttack (WebAttack, NULL, attackmod, this);
        return elapsed;
    }

    x = mX;    y = mY;

    bool immediate = true;
    while (maxrange--) {
        if (!Level->moveForward (dir, &x, &y)) {
            Level->moveForward (uTurn (dir), &x, &y);
            return elapsed;
        }

        if (Level->isOccupied (x, y)) {
            shCreature *c = Level->getCreature (x, y);
            /* May sail overhead unless web was shot from adjacent position. */
            if (c->mZ < 0 and RNG (2) and !immediate) {
                continue;
            }
            r = resolveRangedAttack (WebAttack, NULL, attackmod, c);
            if (r >= 0)
                return elapsed;
        }
        if (Level->isObstacle (x, y))  return elapsed;
        immediate = false;
    }

    return elapsed;
}

int
shCreature::opticBlast (shDirection dir)
{
    //int maxrange = shortRange (this);
    int elapsed = QUICKTURN;

    shAttack *OpticBlast = &Attacks[kAttOpticBlast];
    // FIXME: kludge
    OpticBlast->mDamage[0].mLow = mCLevel;
    OpticBlast->mDamage[0].mHigh = mCLevel * 6;

    if (kUp == dir) {
        /* TODO: maybe loosen debris from the ceiling? */
        return elapsed;
    } else if (kDown == dir) {
        return elapsed;
    }

    int attackmod = getSkillModifier (kMutantPower, kOpticBlast)
                  + mToHitModifier;

    int foo = Level->areaEffect (OpticBlast, NULL, mX, mY,
                                 dir, this, attackmod);
    if (foo < 0)
        return foo;
    return elapsed;
}

int
shCreature::xRayVision (int on)
{
    assert (isHero ());
    if (on) {
        if (!hasXRayVision () and
            hasPerilSensing () and
            mGoggles->isToggled ())
        {
            I->p ("You can see through your darkened goggles now.");
        }
        mInnateIntrinsics |= kXRayVision;
    } else {
        mInnateIntrinsics &= ~kXRayVision;
        if (isHero () and
            hasPerilSensing () and
            mGoggles->isToggled ())
        {
            I->p ("You can no longer see through your darkened goggles.");
        }
    }
    Hero.computeIntrinsics ();
    return HALFTURN;
}

int
shCreature::mentalBlast (int x, int y)
{
    shCreature *c = mLevel->getCreature (x, y);
    int elapsed = SLOWTURN;
    int shielded;

    if (!c) {
        return elapsed;
    }
    shielded = c->hasBrainShield ();

    const char *t_buf = THE (c);
    const char *an_buf = AN (c);

    if (c->isHero ()) {
        if (isHero ()) {
            I->p ("You blast yourself with psionic energy!");
            shielded = 0;
        } else {
            if (torcComparison (this, c)) {
                I->p ("%s tingles briefly.", YOUR (mImplants[shObjectIlk::kNeck]));
                return elapsed;
            }
            if (shielded) {
                I->p ("Your scalp tingles.");
                return elapsed;
            }
            I->p ("You are blasted by a wave of psionic energy!");
            if (!Hero.canSee (this)) {
                I->drawMem (mX, mY, kNone, this, NULL, NULL, NULL);
                I->refreshScreen ();
                I->pauseXY (mX, mY, 0, 0);
                if (isSessile ()) {
                    Level->remember (mX, mY, mIlkId);
                }
            }
        }
    } else if (isHero ()) {
        if (torcComparison (this, c)) return HALFTURN;
        if (c->hasMind ()) {
            if (!canSee (c) and !Hero.canHearThoughts (c)) {
                /* Make sure the creature is referred to by its actual name
                   because by making contact with creature's mind hero now
                   knows what it is.  For a brief moment at least.*/
                Level->setVisible (x, y, 1);
                an_buf = AN (c);
                t_buf = THE (c);
                Level->setVisible (x, y, 0);
                I->drawMem (mX, mY, kNone, this, NULL, NULL, NULL);
                I->refreshScreen ();
                I->p ("You blast %s with a wave of psionic energy.", an_buf);
                I->pauseXY (x, y);
            } else {
                I->p ("You blast %s with a wave of psionic energy.", t_buf);
            }
            c->newEnemy (this);
        } else if (!canSee (x, y)) {
            return elapsed;
        }
    }
    if (!c->hasMind () or c->hasBrainShield ()) {
        if (Hero.canSee (c)) {
            I->p ("%s is not affected.", t_buf);
        } else if (Hero.canHearThoughts (c)) {
            I->p ("You sense no effect.");
        }
        return elapsed;
    } else if (c->isHero ()) {
        I->p ("Ow!");
    }

    shAttack *atk;
    if (c->willSave (getPsionicDC (3))) {
        atk = &Attacks[kAttResistedMentalBlast];
        atk->mDamage[0].mLow = (mCLevel + 1) / 2;
        atk->mDamage[0].mHigh = atk->mDamage[0].mLow * 3;
    } else { /* confused if save not made */
        atk = &Attacks[kAttMentalBlast];
        atk->mDamage[0].mLow = (mCLevel + 1) / 2;
        atk->mDamage[0].mHigh = atk->mDamage[0].mLow * 6;
    }

    if (c->sufferDamage (atk, this, NULL, 1, 1)) {
        if (isHero () or Hero.canSee (c)) {
            I->p ("%s %s killed!", t_buf, c->isHero () ? "are" : "is");
        }
        c->die (kKilled, this, NULL, atk, "a mental blast");
    }
    return elapsed;
}

int
shCreature::regeneration ()
{
    if (isHero () and mHP == mMaxHP and Hero.getStoryFlag ("lost tail")) {
        Hero.resetStoryFlag ("lost tail");
        Hero.myIlk ()->mAttacks[3].mAttId = kAttXNTail;
        Hero.myIlk ()->mAttacks[3].mProb = 1;
        Hero.myIlk ()->mAttackSum += 1;
        I->p ("You regrow your tail!");
        return FULLTURN;
    }
    mHP += NDX (mCLevel, 4);
    if (mHP > mMaxHP) {
        mHP = mMaxHP;
    }
    return FULLTURN;
}

/* Is this balanced?  Trade -4 cha for +4 str - is awesome for non-psions! */
int
shCreature::adrenalineControl (int on)
{
    mAbil.mStr += on ? 4 : -4;
    mMaxAbil.mStr += on ? 4 : -4;
    computeIntrinsics ();
    return FULLTURN;
}

int
shCreature::haste (int on)
{   /* Implemented in shCreature::computeIntrinsics () */
    if (on) {
        if (isHero ()) {
            I->p ("Your actions accelerate.");
        } else if (Hero.canSee (this)) {
            I->p ("%s speeds up!", the ());
        }
    } else {
        if (isHero ()) {
            I->p ("Your actions decelerate.");
        } else if (Hero.canSee (this)) {
            I->p ("%s slows down!", the ());
        }
    }
    computeIntrinsics ();
    return FULLTURN;
}

int
shCreature::psionicStorm (int x, int y)
{
    if (isHero ()) {
        I->p ("You invoke psionic storm ...");
    }

    mLevel->areaEffect (kAttPsionicStorm, NULL, x, y, kNoDirection, this, 0);

    return SLOWTURN;
}

extern void pacifyMelnorme (shMonster *); /* Services.cpp */

int
shCreature::theVoice (int x, int y)
{
    const int elapsed = SLOWTURN;
    static const struct shVoiceEffect
    {
        const char *command;
        shCondition effect;
    } voiceData[] =
    {
        {"%s %s%s: \"%s will not read the Nam-Shub!\"", kUnableCompute},
        {"%s %s%s: \"%s will not indulge %s mutant nature!\"", kUnableUsePsi},
        {"%s %s%s: \"%s will not show belligerence!\"", kUnableAttack},
        {"%s %s%s: \"%s will not feast upon the poor!\"", kGenerous}
    };
    shCreature *c = Level->getCreature (x, y);
    if (!c) {
        if (isHero ()) I->p ("There is no one here.");
        return 0;
    }
    if (!Level->existsLOS (mX, mY, x, y)) {
        if (isHero ()) I->p ("%s cannot hear you.", THE (c));
        return 0;
    }
    if (c->mType != kHumanoid and c->mType != kMutant and
        c->mType != kOutsider and c->mType != kCyborg)
    {
        if (isHero ()) I->p ("The Voice has no effect.");
        return elapsed;
    }
    /* Determine exact The Voice command to use. */
    int choice, options;
    if (c->isA (kMonMelnorme) and c->isHostile ()) {
        I->p ("You speak: \"You will not bear petty grudges!\"");
        shMonster *m = (shMonster *) c;
        pacifyMelnorme (m);
        I->p ("%s complies.", THE (c));
        return elapsed;
    } else {
        int possible[3];
        options = 0;
        if (!c->is (kUnableCompute) and
            (c->isHero () or c->isA (kMonUsenetTroll) or c->isA (kMonBOFH)))
        {
            possible[options++] = 0;
        }
        if (!c->is (kUnableUsePsi)) {
            int haspowers = 0;
            if (c->isHero ()) {
                for (int i = kNoMutantPower + 1; i < kMaxHeroPower; ++i) {
                    if (c->mMutantPowers[i]) {
                        haspowers = 1;
                        break;
                    }
                }
            } else {
                for (int i = kNoMutantPower + 1; i < kMaxMutantPower; ++i) {
                    if (c->myIlk ()->mPowers[i]) {
                        haspowers = 1;
                        break;
                    }
                }
            }
            if (haspowers) possible[options++] = 1;
        }
        if (!c->is (kUnableAttack)) {
            possible[options++] = 2;
        }
        if ((c->isA (kMonLawyer) or c->isA (kMonMelnorme))
            and !c->is (kGenerous))
        {
            possible[options++] = 2;
            if (RNG (3) or !c->isHostile ()) {
                possible[0] = 3;
                choice = 3;
                options = 1;
            }
        }
        if (options)  choice = possible[RNG (options)];
    }
    if (!options) {
        if (isHero ()) {
            I->p ("You have fully used The Voice against %s already.",
                  c == this ? "yourself" : THE (c));
        }
        return elapsed;
    }
    bool self = this == c;
    I->p (voiceData[choice].command, the (), self ? "vow" : "speak",
        isHero () ? "" : "s", self ? "I" : "you", self ? "my" : "your");
    c->inflict (voiceData[choice].effect, FULLTURN * RNG (3, 13));
    return elapsed;
}

static int
useDigestion (shHero *h)
{
    if (Hero.getStoryFlag ("superglued tongue")) {
        I->p ("You can't eat anything with this stupid canister "
              "glued to your mouth!");
        return 0;
    }

    shObject *obj =
        h->quickPickItem (h->mInventory, "eat", shMenu::kCategorizeObjects);
    if (obj) {
        /* objects on your head can't be eaten conveniently, but otherwise
           we'll assume that digestion comes along with super flexibility
           required to eat belts, boots, armor, etc.
         */
        if (obj->isA (kImplant) and obj->isWorn () and !obj->isA (kObjTorc)) {
            I->p ("You'll have to uninstall that first.");
        } else if (h->mHelmet == obj or h->mGoggles == obj) {
            I->p ("You'll have to take it off first.");
        } else {
            return h->digestion (obj);
        }
    } else {
        I->nevermind ();
    }
    return 0;
}

static int
useAdrenalineControl (shHero *h)
{
    return h->adrenalineControl (1);
}

static int
stopAdrenalineControl (shHero *h)
{
    return h->adrenalineControl (0);
}

static int
useHaste (shHero *h)
{
    return h->haste (1);
}

static int
stopHaste (shHero *h)
{
    return h->haste (0);
}

static int
useTelepathy (shHero *h)
{
    I->p ("You extend the outer layers of your mind.");
    return h->telepathy (1);
}

static int
stopTelepathy (shHero *h)
{
    I->p ("Your consciousness contracts.");
    return h->telepathy (0);
}

static int
useTremorsense (shHero *h)
{
    return h->tremorsense (1);
}

static int
stopTremorsense (shHero *h)
{
    return h->tremorsense (0);
}

static int
useOpticBlast (shHero *h)
{
    if ((h->hasPerilSensing () and h->mGoggles->isToggled ()) or
        (h->mGoggles and h->mGoggles->isA (kObjBlindfold)))
    {
        if (!h->mGoggles->isFooproof ()) {
            I->p ("You incinerate %s!", YOUR (h->mGoggles));
            if (h->sufferDamage (kAttBurningGoggles)) {
                h->shCreature::die (kMisc, "Burned his face off");
            }
            h->removeObjectFromInventory (h->mGoggles);
        } else {
            I->p ("You try to ignite %s without result.", YOUR (h->mGoggles));
        }
        return FULLTURN;
    } else if (h->isBlind ()) {
        I->p ("But you are blind!");
        return 0;
    }
    shDirection dir = I->getDirection ();
    if (kNoDirection == dir) {
        return 0;
    } else {
        return h->opticBlast (dir);
    }
}

static int
useRegeneration (shHero *h)
{
    return h->regeneration ();
}

static int
useRestoration (shHero *h)
{   /* TODO: Factor in skill. */
    return h->restoration (RNG (1, 2));
}

static int
useHypnosis (shHero *h)
{
    shDirection dir = I->getDirection ();
    if (kNoDirection == dir) {
        return 0;
    } else if (kOrigin == dir) {
        I->p ("You need a mirror for that.");
        return 0;
    } else {
        return h->hypnosis (dir);
    }
}

static int
useXRayVision (shHero *h)
{
    return h->xRayVision (1);
}

static int
stopXRayVision (shHero *h)
{
    return h->xRayVision (0);
}

/* Returns how much psi ability points using/sustaning given */
static int /* power 'power' costs while autolysis is active. */
autolysisCost (int power)
{
    if (MutantPowers[power].mOffFunc) {
        return maxi (0, mini (MutantPowers[power].mLevel - 2,
                              MutantPowers[power].mLevel / 2));
    }
    return MutantPowers[power].mLevel / 2;
}

static int
useAutolysis (shHero *h)
{
    /* Difference of Psi points for each continuous power is given back. */
    for (int i = kNoMutantPower; i <= kMaxHeroPower; i++) {
        if (i == kAutolysis) continue;
        if (h->mMutantPowers[i] > 1) {
            h->mPsiDrain += MutantPowers[i].mLevel - autolysisCost (i);
            h->mMaxAbil.mPsi += MutantPowers[i].mLevel - autolysisCost (i);
        }
    }
    return FULLTURN;
}

static int
stopAutolysis (shHero *h)
{
    int need = 0; /* Psi points needed to sustain powers without autolysis. */
    int powercost = MutantPowers[kAutolysis].mLevel;
    int pool = h->getPsi () + h->mPsiDrain - 1;
    for (int i = kNoMutantPower; i <= kMaxHeroPower; i++) {
        if (i == kAutolysis) continue;
        if (h->mMutantPowers[i] > 1) {
            need += MutantPowers[i].mLevel - autolysisCost (i);
        }
    }
    if (need <= powercost) { /* Not full Psi cost of autolysis regained. */
        h->mMaxAbil.mPsi -= need;
        h->mPsiDrain -= need;
        return FULLTURN;
    } else if (need <= pool) { /* Additional Psi cost incurred. */
        h->mMaxAbil.mPsi -= need;
        h->mAbil.mPsi -= need - powercost;
        h->mPsiDrain -= powercost;
        return FULLTURN;
    } /* Else not enough Psi.  Drop continuous powers. */
    for (int i = kNoMutantPower; i <= kMaxHeroPower; i++) {
        if (i == kAutolysis) continue;
        if (h->mMutantPowers[i] > 1) {
            h->mPsiDrain += autolysisCost (i);
            h->mMutantPowers[i] = 1;
            MutantPowers[i].mOffFunc (h);
        }
    }
    I->p ("You will need to reactivate your continuous powers.");
    return FULLTURN;
}

static int
useTeleport (shHero *h)
{
    int x, y;
    Level->findSquare (&x, &y);
    h->transport (x, y, 100, 1);
    return HALFTURN;
}

static int
useIllumination (shHero *h)
{
    int skill = h->getSkillModifier (kMutantPower, kIllumination);
    int radius = 4 + (skill / 3);

    for (int x = h->mX - radius; x <= h->mX + radius; x++) {
        for (int y = h->mY - radius; y <= h->mY + radius; y++) {
            if (Level->isInBounds (x, y) and Level->isInLOS (x, y)
                and distance (x, y, h->mX, h->mY) < radius * 5)
            {
                Level->setLit (x, y, 1, 1, 1, 1);
            }
        }
    }
    return HALFTURN;
}

static int
useWeb (shHero *h)
{
    shDirection dir = I->getDirection ();
    if (kNoDirection == dir) {
        return 0;
    } else {
        return h->shootWeb (dir);
    }
}

/* The two below are pretty similar. When third of ilk appears */
static int /* try joining them into one. */
useMentalBlast (shHero *h)
{
    int x = -1, y = -1;
    if (0 == I->getSquare ("What do you want to blast?  (select a location)",
                           &x, &y, shortRange (h), 0, MUTPOWER_CURSOR))
    {
        return 0;
    } else {
        return h->mentalBlast (x, y);
    }
}

static int
useTheVoice (shHero *h)
{
    int x = -1, y = -1;
    if (0 == I->getSquare ("Who do you want to command?  (select a location)",
                           &x, &y, shortRange (h), 0, MUTPOWER_CURSOR))
    {
        return 0;
    } else {
        return h->theVoice (x, y);
    }
}

static int
usePsionicStorm (shHero *h)
{
    int x = -1, y = -1;
    if (0 == I->getSquare ("Where do you want to invoke the storm?"
                   " (select a location)", &x, &y, 100, 0, MUTPOWER_CURSOR))
    {
        return 0;
    } else {
        int success = h->psionicStorm (x, y);
        if (success) { /* This is costly power. */
            int drain = --Hero.mAbil.mPsi - 1;
            if (h->mMutantPowers[kAutolysis] == 2) drain /= 2;
            Hero.mAbil.mPsi -= drain;
            Hero.mPsiDrain += drain;
            Hero.mPsionicStormFatigue += 2;
        }
        return success;
    }
}

/* make sure these agree with the enum def in Global.h: */
MutantPower MutantPowers[kMaxAllPower] =
{ { 0, ' ', "empty", NULL, NULL },
  { 1, 'i', "Illumination", useIllumination, NULL },
  { 1, 'd', "Digestion", useDigestion, NULL },
  { 1, 'h', "Hypnosis", useHypnosis, NULL },
  { 1, 'r', "Regeneration", useRegeneration, NULL },
  { 2, 'o', "Optic Blast", useOpticBlast, NULL },
  { 2, 'H', "Haste", useHaste, stopHaste },
  { 2, 'T', "Telepathy", useTelepathy, stopTelepathy },
  { 2, 'R', "Tremorsense", useTremorsense, stopTremorsense },
  { 2, 'w', "Web", useWeb, NULL },
  { 3, 'm', "Mental Blast", useMentalBlast, NULL},
  { 3, 's', "Restoration", useRestoration, NULL },
  { 4, 'A', "Adrenaline Control", useAdrenalineControl, stopAdrenalineControl },
  { 4, 'X', "X-Ray Vision", useXRayVision, stopXRayVision },
  { 4, 'v', "The Voice", useTheVoice, NULL },
  { 5, 't', "Teleport", useTeleport, NULL },
  { 6, 'L', "Autolysis", useAutolysis, stopAutolysis },
  { 8, 'p', "Psionic Storm", usePsionicStorm, NULL }
};


const char *
getMutantPowerName (shMutantPower id)
{
    return MutantPowers[id].mName;
}


int
shHero::getMutantPowerChance (int power)
{
    int chance =
        (getSkillModifier (kMutantPower, (shMutantPower) power) +
         + mini (mCLevel, 10) /* At higher levels anyone could use any power. */
         + mPsiModifier + 15 - 2 * MutantPowers[power].mLevel) * 5;
    if (MutantPowers[power].mLevel * 2 > mCLevel + 1) chance -= 20;
    if (power == kPsionicStorm) chance -= 5 * mPsionicStormFatigue;
    if (Psion == mProfession) chance += mCLevel * 5;
    if (chance > 100) chance = 100;
    if (chance < 0) chance = 0;
    return chance;
}


int
shHero::stopMutantPower (shMutantPower id)
{
    int autolysis = mMutantPowers[kAutolysis] == 2;
    if (kAutolysis == id or !autolysis) {
        mMaxAbil.mPsi += MutantPowers[id].mLevel;
        mPsiDrain += MutantPowers[id].mLevel;
    } else {
        mMaxAbil.mPsi += autolysisCost (id);
        mPsiDrain += autolysisCost (id);
    }
    mMutantPowers[id] = 1;
    return MutantPowers[id].mOffFunc (this);
}

/* returns: elapsed time */
int
shHero::useMutantPower ()
{
    shMenu *menu = I->newMenu ("Use which mutant power?", 0);
    menu->attachHelp ("mutpowers.txt");
    int n = 0;
    char buf[50];

    if (is (kUnableUsePsi)) {
        I->p ("Mere thought of this seems disguisting to you.");
        return 0;
    }
    menu->addHeader ("       Power              Level Chance");
    for (int i = kNoMutantPower; i <= kMaxHeroPower; i++) {
        if (mMutantPowers[i]) {
            int l;
            int chance = getMutantPowerChance (i);
            l = snprintf (buf, 50, "%-19s  %d    ",
                          MutantPowers[i].mName, MutantPowers[i].mLevel);
            if (mMutantPowers[i] > 1) {
                /* persistant power that's already toggled */
                snprintf (&buf[l], 10, "(on)");
            } else {
                snprintf (&buf[l], 10, "%3d%%", chance);
            }
            menu->addIntItem (MutantPowers[i].mLetter, buf, i, 1);
            ++n;
        }
    }
    if (0 == n) {
        I->p ("You don't have any mutant powers.");
        return 0;
    }
    int i = 0;
    int autolysis = mMutantPowers[kAutolysis] == 2;
    menu->getIntResult (&i, NULL);
    delete menu;
    if (i) {
        /* Deactivating a power, always succeed. */
        if (mMutantPowers[i] > 1) {
            return stopMutantPower ((shMutantPower) i);
        }

        int cost = autolysis ? autolysisCost (i) : MutantPowers[i].mLevel;
        int pool = MutantPowers[i].mOffFunc ? mAbil.mPsi + mPsiDrain : mAbil.mPsi;

        if (pool <= cost) {
            I->p ("You are too weak to use that power.");
            return 200;
        }

        int chance = getMutantPowerChance (i);
        if (chance == 0) {
            I->p ("No point in trying this.");
            return 0;
        }

        if (i == kRegeneration and is (kSickened)) {
            I->p ("Your organism is busy fighting sickness.");
            return 0;
        }

        int roll = RNG (100);
        debug.log ("mutant power attempt: rolled %d %s %d.",
                   roll, roll < chance ? "<" : ">=", chance);

        mAbil.mPsi -= cost;
        if (autolysis) {
            int level = MutantPowers[i].mLevel;
            int hp_loss = maxi (level, level * mHP / 10);
            if (hp_loss >= mHP) {
                shCreature::die (kMisc, "Went out with a bang");
                return 0;
            } else {
                mHP -= hp_loss;
            }
        }
        if (roll < chance) {
            if (MutantPowers[i].isPersistant ()) {
                mMaxAbil.mPsi -= cost;
                mMutantPowers[i] = 2;
            } else {
                mPsiDrain += cost;
            }
            return MutantPowers[i].mFunc (this);
        } else {
            mPsiDrain += cost;
            I->p ("You fail to use your power successfully.");
            return HALFTURN;
        }
    }
    return 0;
}

/* Certain distractions (such as getting stunned) may cause your */
void    /* concentration to falter and powers to be stopped. */
shCreature::checkConcentration ()
{
    int failed = 0;
    int basesk = Hero.getSkillModifier (kConcentration);

    if (!isHero ()) {
        return;
    }

    for (int i = kNoMutantPower; i <= kMaxHeroPower; i++) {
        int sk = basesk + RNG (1, 20);
        if (mMutantPowers[i] > 1 and sk < 15) {
            if (!failed and isHero ()) {
                I->p ("You lose your concentration.");
            }
            ++failed;
            mPsiDrain += MutantPowers[i].mLevel;
            mMutantPowers[i] = 1;
            MutantPowers[i].mOffFunc ( (shHero *) this);
        }
    }
    if (failed)
        I->p ("You fail to maintain %d continuous mutant power%s.",
            failed, failed > 1 ? "s" : "");
}



shMutantPower
shHero::getMutantPower (shMutantPower power, int silent)
{
    if (kNoMutantPower == power) {
        int attempts = 10;
        while (attempts--) {
            power = (shMutantPower) RNG (kNoMutantPower + 1, kMaxHeroPower);
            if (!mMutantPowers[power] and MutantPowers[power].mFunc and
                mCLevel + 3 >= MutantPowers[power].mLevel and
                /* Those powers need character to have eyes: */
                (!(kBlind & mInnateIntrinsics) or
                 (kOpticBlast != power and kXRayVisionPower != power))
                )
            {
                mMutantPowers[power] = 1;
                if (!silent) {
                    I->p ("Your brain is warping!");
                    I->p ("You gain the \"%s\" mutant power!",
                          MutantPowers[power].mName);
                }
                return power;
            }
        }
    } else {
        mMutantPowers[power] = 1;
        if (!silent) {
            I->p ("Your brain is warping!");
            I->p ("You gain the \"%s\" mutant power!",
                  MutantPowers[power].mName);
        }
        return power;
    }
    return kNoMutantPower;
}



/* returns elapsed time, 0 if no power used, -1 if creature dies */

int
shMonster::useMutantPower ()
{
    int x0, y0, x, y, res;

    if (mSpellTimer > Clock) {
        return 0;
    }

    const char *who = the ();
    /* Guide: blocking a mutant power and printing a message causes four
       turns of delay before next attempt. Blocking a power silently causes
       only two turns of delay so player has a chance of seeing effects of
       The Voice. */
    bool forbidden = is (kUnableUsePsi);

    for (int attempts = myIlk ()->mNumPowers * 2; attempts; --attempts) {
        int choice = RNG (myIlk ()->mNumPowers);
        shMutantPower power = myIlk ()->mPowers[choice];

        switch (power) {
        case kHypnosis:
            if (!areAdjacent (this, &Hero) or Hero.is (kAsleep) or
                Hero.is (kParalyzed) or Hero.isBlind ())
            {
                continue;
            }
            if (forbidden) {
                if (Hero.canSee (this)) {
                    I->p ("%s gazes at you but immediately flinches.", who);
                }
                mSpellTimer = Clock + 4 * FULLTURN;
                return FULLTURN;
            }
            if (Hero.canSee (this)) {
                I->p ("%s gazes into your eyes.", who);
            }
            mSpellTimer = Clock + 10 * FULLTURN;
            return hypnosis (vectorDirection (mX, mY, Hero.mX, Hero.mY));
        case kOpticBlast:
            if (!canSee (&Hero))
                continue;
            {
                shDirection dir = linedUpDirection (this, &Hero);
                if (kNoDirection == dir)
                    continue;
                if (forbidden) {
                    if (Hero.canSee (this)) {
                        I->p ("%s stares at you intently but immediately flinches.", who);
                    }
                    mSpellTimer = Clock + 4 * FULLTURN;
                    return FULLTURN;
                }
                if (Hero.canSee (this)) {
                    I->p ("%s shoots a laser beam out of %s!",
                          who, her ("eyes"));
                } else {
                    I->p ("Someone shoots a laser beam out of %s!",
                          her ("eyes"));
                    Level->feelSq (mX, mY);
                }
                return opticBlast (dir);
            }
        case kMentalBlast:
            if (!canSee (&Hero) and (!hasTelepathy () or
                                    distance (&Hero, mX, mY) >= 60))
            {
                continue;
            }
            if (forbidden) {
                if (Hero.canSee (this)) {
                    I->p ("%s begins to concentrate but immediately stops it.", who);
                }
                mSpellTimer = Clock + 4 * FULLTURN;
                return FULLTURN;
            }
            if (Hero.canSee (this) and numHands ()) {
                I->p ("%s concentrates.", who);
            }
            mSpellTimer = Clock + 10 * FULLTURN;
            return mentalBlast (Hero.mX, Hero.mY);
        case kRegeneration:
            if (mHP >= mMaxHP or is (kSickened))
                continue;
            if (forbidden) {
                mSpellTimer = Clock + 2 * FULLTURN;
                return 0;
            }
            if (Hero.canSee (this)) {
                if (numHands ())
                    I->p ("%s concentrates.", who);
                I->p ("%s looks better.", who);
            }
            mSpellTimer = Clock + 2 * FULLTURN;
            return regeneration ();
        case kTeleport:
            if ((mHP < mMaxHP / 4 or is (kFleeing)) and canSee (&Hero)) {
                /* escape to safety */
                x = -1, y = -1;
                /* make double teleport less likely */
                mSpellTimer = Clock + RNG (10 * FULLTURN);
            } else if (hasTelepathy () and mHP == mMaxHP and !RNG (10)) {
                /* teleport in to make trouble */
                x = Hero.mX;
                y = Hero.mY;
                if (mLevel->findNearbyUnoccupiedSquare (&x, &y)) {
                    return 0;
                }
                mSpellTimer = Clock + FULLTURN;
            } else {
                continue;
            }
            if (forbidden) {
                if (Hero.canSee (this)) {
                    I->p ("%s flickers for a moment.", who);
                }
                mSpellTimer = Clock + 4 * FULLTURN;
                return FULLTURN;
            }
            res = transport (x, y, 100, 1);
            return (-1 == res) ? -1 : FULLTURN;
        case kPsionicStorm:
            if (distance (mX, mY, Hero.mX, Hero.mY) <= 5 * 4 + 2)
                continue; /* Too close, would catch self in blast range. */
            if (forbidden) {
                if (Hero.canSee (this)) {
                    I->p ("%s seems to be seething with anger.", who);
                }
                mSpellTimer = Clock + 4 * FULLTURN;
                return FULLTURN;
            }
            mSpellTimer = Clock + RNG (10 * FULLTURN);
            return this->psionicStorm (Hero.mX, Hero.mY);
        case kTheVoice:
            if (!canSee (&Hero) or !Level->existsLOS (mX, mY, Hero.mX, Hero.mY))
                continue;
            return theVoice (Hero.mX, Hero.mY);
        case kTerrify:
            if (Hero.is (kFrightened) or !canSee (&Hero) or
                distance (&Hero, mX, mY) >= 40)
            {
                continue;
            }
            if (forbidden) {
                if (Hero.canSee (this)) {
                    I->p ("%s begins to concentrate but immediately stops it.", who);
                }
                mSpellTimer = Clock + 4 * FULLTURN;
                return FULLTURN;
            }
            if (Hero.canSee (this) and numHands ()) {
                I->p ("%s concentrates.", who);
            }
            if (!Hero.hasBrainShield () and !Hero.is (kConfused) and
                !Hero.is (kStunned) and !willSave (getPsionicDC (3)))
            {
                I->p ("You suddenly feel very afraid!");
                Hero.inflict (kFrightened, FULLTURN * NDX (2, 10));
            } else {
                I->p ("You tremble for a moment.");
            }
            mSpellTimer = Clock + 10 * FULLTURN;
            return FULLTURN;
        case kDarkness:
            if (RNG (4)) /* a rarely used power */
                continue;
            if (!canSee (&Hero)) {
                continue;
            }
            if (forbidden) {
                mSpellTimer = Clock + 2 * FULLTURN;
                return FULLTURN;
            }
            if (mLevel->isLit (mX, mY, Hero.mX, Hero.mY)) {
                x0 = mX;
                y0 = mY;
                if (Hero.canSee (this))
                    I->p ("%s is surrounded by darkness!", who);
            } else if (mLevel->isLit (Hero.mX, Hero.mY, Hero.mX, Hero.mY)) {
                x0 = Hero.mX;
                y0 = Hero.mY;
                I->p ("You are surrounded by darkness!");
            } else {
                continue;
            }
            for (y = y0 - 5; y <= y0 + 5; y++) {
                for (x = x0 - 5; x <= x0 + 5; x++) {
                    if (distance (x, y, x0, y0) <= 25 and
                        mLevel->existsLOS (x, y, x0, y0))
                    {
                        mLevel->setLit (x, y, -1, -1, -1, -1);
                    }
                }
            }
            if (Hero.canSee (this)) {
                I->drawScreen ();

            }
            mSpellTimer = Clock + 2 * FULLTURN;
            return FULLTURN;
        case kCeaseAndDesist:
            if (!areAdjacent (this, &Hero) or Hero.is (kParalyzed)
                or Hero.is (kAsleep))
                continue;
            I->p ("%s reads a cease and desist letter to you!", who);
            if (Hero.hasBrainShield () or willSave (getPsionicDC (4))) {
                I->p("But you ignore it.");
            }  else {
                Hero.inflict (kParalyzed, FULLTURN * NDX (2, 6));
            }
            mSpellTimer = Clock + 2 * FULLTURN;
            return FULLTURN;
        case kSeizeEvidence:
            if (!areAdjacent (this, &Hero) or is (kFleeing))
                continue;
            if ((Hero.is (kParalyzed) or Hero.is (kAsleep)) and RNG (4))
                /* hard for lawyer to seize your stuff unless you're helpless */
                continue;

            { /* else */
                I->p ("%s rifles through your pack!", who);

                shObjectVector v;
                selectObjectsByFunction (&v, Hero.mInventory, &shObject::isCracked);
                if (v.count ()) {
                    int i = selectObjects (&v, Hero.mInventory, kObjGenericComputer);
                    I->p ("%s seizes your pirated software%s", who,
                          i > 1 ? " and your computers!" :
                          1 == i ? " and your computer!" : "!");
                    for (i = 0; i < v.count (); i++) {
                        shObject *obj = v.get (i);
                        Hero.removeObjectFromInventory (obj);
                        addObjectToInventory (obj);
                    }
                    inflict (kFleeing, RNG (50 * FULLTURN));
                }
            }
            mSpellTimer = Clock + FULLTURN;
            return LONGTURN;
        case kSueForDamages:
            if (!areAdjacent (this, &Hero) or is (kFleeing))
                continue;

            I->p ("%s sues you for damages!", who);
            x = Hero.loseMoney (NDX (mCLevel, 100));
            if (x) {
                gainMoney (x);
            } else {
                I->p ("But you're broke!");
                inflict (kFleeing, RNG (50 * FULLTURN));
            }
            mSpellTimer = Clock + FULLTURN;
            return FULLTURN;
        case kSummonWitness:
            if (!areAdjacent (this, &Hero) or is (kFleeing))
                continue;
            {
                shMonId ilk;
                shMonster *mon;

                do {
                    ilk = pickAMonsterIlk(RNG((Level->mDLevel+mCLevel+1)/2));
                } while (!ilk);
                mon = new shMonster (ilk);
                x = Hero.mX;
                y = Hero.mY;
                if (Level->findNearbyUnoccupiedSquare (&x, &y)) {
                    /* nowhere to put it */
                    delete mon;
                    res = transport (-1, -1, 100, 1);
                    return (-1 == res) ? -1 : FULLTURN;
                }
                if (Level->putCreature (mon, x, y)) {
                    /* shouldn't happen */
                    return FULLTURN;
                }
                mon->mDisposition = kHostile;
                I->p ("%s summons a hostile witness!", who);
                mSpellTimer = Clock + FULLTURN;
                return FULLTURN;
            }
        case kLaunchMissile:
            // FIXME: This is unprepared to work as pet power.
            if (areAdjacent (this, &Hero) or !canSee (&Hero))
                continue;
            {
                int nx = Hero.mX - this->mX;
                if (nx)
                    nx /= abs (nx);
                nx += this->mX;
                int ny = Hero.mY - this->mY;
                if (ny)
                    ny /= abs (ny);
                ny += this->mY;
                if (!Level->isFloor (nx, ny) or Level->isOccupied (nx, ny))
                    continue;
                shMonster *msl = new shMonster (kMonSmartMissile);
                msl->mConditions |= kNoExp;
                Level->putCreature (msl, nx, ny);
                if (isPet ()) {
                    msl->makePet ();
                }
                if (Hero.canSee (msl)) {
                    I->p ("%s launches %s!", THE (this), AN (msl));
                } else {
                    I->p ("You hear a whoosh.");
                }
                mSpellTimer = Clock + 3 * FULLTURN;
                return FULLTURN;
            }

        default:
            break;
        }
    }
    return 0;
}
