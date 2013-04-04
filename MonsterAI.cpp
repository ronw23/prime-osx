#include "Global.h"
#include "Util.h"
#include "Monster.h"
#include "Hero.h"


void
shMonster::newEnemy (shCreature *c)
{
    if (c->isHero ()) {
        if (isPet ()) {

        } else {
            makeAngry ();
        }
    } else if (c->isPet ()) {
        mEnemyX = c->mX;
        mEnemyY = c->mY;
    }
    if (isA (kMonAlienEgg)) {
        ++mAlienEgg.mHatchChance;
    }
}


void
shMonster::makeAngry ()
{
    if (!isHostile ()) {
        /* What if not hero damaged the Monolith?  If hit with stun grenade
           blast it will still get angry for no good reason. */
        if (mHP == mMaxHP and isA (kMonMonolith)) {
            return;
        }
        if (Hero.canSee (this)) {
            I->p ("%s gets angry!", the ());
        }
        mDisposition = kHostile;
        mTactic = kNewEnemy;
    }
}


void
shMonster::newDest (int x, int y)
{
    mDestX = x;
    mDestY = y;
}


/* try to move the creature one square in the given direction
   RETURNS: elapsed time, or
            -2 if the creature dies
*/

int
shCreature::doMove (shDirection dir)
{
    int x = mX;
    int y = mY;
    int speed;

    speed = isDiagonal (dir) ? DIAGTURN : FULLTURN;

    mDir = dir;

    if (0 == speed) {
        return 200;
    }

    if (!mLevel->moveForward (dir, &x, &y)) {
        I->p ("Probable bug: a monster tried to move off the map!");
        /* do nothing */
        return 100;
    }

    if (mHidden) {
        int res = revealSelf ();
        if (res)  return res;
    }

    if (mLevel->isObstacle (x, y)) {
        if (Hero.canSee (this)) {
            if (Hero.canSee (x, y)) {
                I->p ("%s bumps into %s.", the (),
                      mLevel->getSquare (x, y) -> the ());
            } else {
                I->p ("%s bumps into something.", the ());
            }
        }
        /* you can't tell when an unseen monster bumps into an obstacle even if
           you can see the obstacle, so don't print a message */
        return speed / 2;
    } else if (mLevel->isOccupied (x, y)) {
        if (&Hero == mLevel->getCreature (x,y)) {
            I->p ("%s bumps into you!", the ());
        } else if (Hero.canSee (this)) {
            if (Hero.canSee (x, y)) {
                I->p ("%s bumps into %s.", the (),
                      mLevel->getCreature (x, y) -> the ());
            } else {
                I->p ("%s bump into something.", the ());
            }
        } else if (Hero.canSee (x, y)) {
            I->p ("Something bumps into %s.", the ());
        }
        return speed / 2;
    } else {
        if (mLevel->moveCreature (this, x, y)) {
            /* we moved and died. */
            return -2;
        }
    }
    return speed;
}

/* Uses A* algorithm.  Implemented in Path.cpp. */
extern int calcShortestPath (int, int, int, int, shMonId, shDirection [], int);

int
shMonster::setupPath ()
{
    if (calcShortestPath (mX, mY, mDestX, mDestY, mIlkId, mPlannedMoves, 100)) {
        mPlannedMoveIndex = 0;
        return 1;
    } else {
        mPlannedMoveIndex = -1;
        return 0;
    }
}


#if 0
int
shMonster::patchPath ()
{
    int len;
    int i, x, y;
    shDirection patch[15];

    for (i = 0, x = mX, y = mY;
         i < 5 and x != mDestX and y != mDestY;
         i++)
    {
        mLevel->moveForward (mPlannedMoves[mPlannedMoveIndex+i], &x, &y);
    }

    len = shortestPath (mX, mY, x, y, &patch, 15);
    if (len) {
        memmove (&mPlannedMoves[mPlannedMoveIndex+len],
                 &mPlannedMoves[mPlannedMoveIndex+i],
                 (100 - i) * sizeof (shDirection));
        memcpy (&mPlannedMoves[mPlannedMoveIndex], &patch[0],
                len * sizeof (shDirection));
        return 1;
    }
    return 0;
}
#endif


/* Attempts to move one square towards destination, with simple obstacle
   avoidance.  This is a quick and dirty routine for use in close combat
   situations when the destination is within sight.

   returns ms elapsed, or -1 if nothing happened, or -2 if monster died
*/

int
shMonster::doQuickMoveTo (shDirection dir /* = kNoDirection */)
{

    int i;
    int dx = 0;
    int dy = 0;
    shDirection dirlist[9];

    if (kNoDirection == dir) {
        if (mX == mDestX and mY == mDestY) {
            /* we're already there */
            mTactic = kReady;
            return -1;
        }
        dir = vectorDirection (mX, mY, mDestX, mDestY);
        debug.log ("quickmoveto %d, %d", mDestX, mDestY);
    }
//    debug.log ("quickmoveto %s");


    dirlist[0] = dir;  /* first try to move directly towards destination */
    dirlist[1] = mDir; /* then try just repeating the last move we made */
    dirlist[2] = (shDirection) ((dir + 1) % 8); /* then try moving in nearby */
    dirlist[3] = (shDirection) ((dir + 7) % 8); /* directions */
    dirlist[4] = (shDirection) ((dir + 6) % 8);
    dirlist[5] = (shDirection) ((dir + 2) % 8);
    dirlist[6] = (shDirection) ((dir + 3) % 8);
    dirlist[7] = (shDirection) ((dir + 5) % 8);
    dirlist[8] = (shDirection) ((dir + 4) % 8);

    for (i = 0; i < 9; i++) {
        dir = dirlist[i];
        dx = 0;
        dy = 0;
        if (kNoDirection == dir) {
            continue;
        }
        mLevel->moveForward (dir, &dx, &dy);
        if (mLevel->isObstacle (mX + dx, mY + dy)) {
            int elapsed = clearObstacle (mX + dx, mY + dy);
            if (elapsed > 0) {
                return elapsed;
            }
            continue;
        }
        if (mLevel->isOccupied (mX + dx, mY + dy)) {
            if ((4 == i and RNG (4)) or
                (6 == i and RNG (4)))
            { /* most of the time, it's better to wait for the other creature
                 to get out of the way */
                return 100;
            }
            continue;
        }
        return doMove (dir);
    }

    /* we seem to be stuck here, give up */
    mTactic = kReady;
    return HALFTURN;
}


/* attempts to clear an obstacle in square x, y
   returns time elapsed on success, -1 on failure
*/

int
shMonster::clearObstacle (int x, int y)
{
    shFeature *f;

    debug.log ("  clear obstacle");

    f = mLevel->getFeature (x, y);
    if (f and shFeature::kDoorClosed == f->mType) {
        if (!openDoor (x, y)) {
            return -1;
        }
        if (Hero.canSee (x, y)) {
            Hero.interrupt ();
        }
        return FULLTURN;
    }
    return -1;
}


/* move one square towards the destination using computed pathfinding data
   returns ms elapsed, or -1 if nothing happened, or -2 if monster died
*/
int
shMonster::doMoveTo ()
{
    shDirection dir;
    int dx = 0;
    int dy = 0;

    debug.log ("  move to destination %d, %d", mDestX, mDestY);

    if (mX == mDestX and mY == mDestY) {
        /* we're already there */
        mTactic = kReady;
        return -1;
    }

    if (mPlannedMoveIndex > 15 and !setupPath()) {
        mTactic = kReady;
        return -1;
    }

    dir = mPlannedMoves[mPlannedMoveIndex];
    mLevel->moveForward (dir, &dx, &dy);

    if (mLevel->isObstacle (mX + dx, mY + dy)) {
        int elapsed = clearObstacle (mX + dx, mY + dy);
        if (-1 == elapsed) {
            if (!setupPath ()) {
                mTactic = kReady;
            }
        }
        return elapsed;
    }
    if (mLevel->isOccupied (mX + dx, mY + dy)) {
        if (RNG (4)) {
            /* best bet: just wait for the creature to move out of the way */
            return 500;
        } else  {
            mTactic = kReady;
            return doQuickMoveTo (dir);
        }
        return -1;
    }

    ++mPlannedMoveIndex;
    return doMove (dir);
}

/*******
  Nethack-style quick and simple AI

  consider each adjacent square

 *******/

int
shMonster::findSquares (int flags, shCoord *coord, int *info)
{
    int n = 0;
    int gridbug = isA (kMonGridBug);

    for (int x = mX - 1; x <= mX + 1; ++x) {
        for (int y = mY - 1; y <= mY + 1; ++y) {
            *info = 0;
            if (!mLevel->isInBounds (x, y))
                continue;

            if (gridbug and abs (x-mX) + abs (y-mY) > 1){
                continue;
            }

            shCreature *c = mLevel->getCreature (x, y);
            shFeature *f = mLevel->getFeature (x, y);
            if (f) {
                if (f->isDoor () and
                    !(f->isOpenDoor () or f->isAutomaticDoor ()))
                {
                    if (flags & kDoor) {
                        *info |= kDoor;
                    } else {
                        continue;
                    }
                } else if (f->isObstacle ()) {
                    continue;
                }

                if (f->isTrap () and
                    (isPet () ? !f->mTrapUnknown : !f->mTrapMonUnknown))
                {
                    if (isFlying () and (shFeature::kPit == f->mType or
                                        shFeature::kAcidPit == f->mType or
                                        shFeature::kTrapDoor == f->mType or
                                        shFeature::kHole == f->mType))
                    {
                        /* this is an acceptable square, keep going */
                    } else if (canSwim () and
                               shFeature::kSewagePit == f->mType)
                    {

                    } else if (f->isBerserkDoor () and mHP > 8) {
                        /* risk it */
                    } else if (flags & kTrap) {
                        *info |= kTrap;
                    } else {
                        continue;
                    }
                }

                if (canHideUnder (f) and flags & kHidingSpot) {
                    *info |= kHidingSpot;
                }
            }

            if (!mLevel->appearsToBeFloor (x, y)) {
                if (flags & kWall) {
                    *info |= kWall;
                } else {
                    continue;
                }
            } else if (kSewage == mLevel->getSquare (x, y) -> mTerr) {
                if (canHideUnderWater () and flags & kHidingSpot) {
                    *info |= kHidingSpot;
                }
            } else if (canBurrow () and !mLevel->isWatery (x, y)) {
            /*  Rule 1: Cannot burrow next to another creature (which may be
                also burrowed or trying to hide).  This rule ensures good spread
                of lurking monsters.  Also chance to catch player unaware rises.
                Finally, concentrations are simply boring.

                Rule 2: Do not burrow next to walls.  In cooridors this makes
                you easy target and in rooms decreases chance of hero stumbling
                close to you. */
                for (int cx = x - 1; cx <= x + 1; ++cx)
                    for (int cy = y - 1; cy <= y + 1; ++cy) {
                        if (!mLevel->isInBounds (cx, cy))
                            continue;
                        if (!mLevel->appearsToBeFloor (cx, cy))
                            goto notok;
                        if (mLevel->isOccupied (cx, cy) and
                            mLevel->getCreature (cx, cy) != this)
                            goto notok;
                    }
                *info |= kHidingSpot;
                notok: ;
            }

            if (c == this) {

            } else if (c and c->isHero ()) {
                if (flags & kHero) {
                    *info |= kHero;
                } else {
                    continue;
                }
            } else if (c and !c->isHero ()) {
                if (flags & kMonster) {
                    *info |= kMonster;
                } else {
                    continue;
                }
            }

            if ((x == Hero.mX or y == Hero.mY or
                 x - y == Hero.mX - Hero.mY or
                 x + y == Hero.mX + Hero.mY)
                and mLevel->isInLOS (x, y))
            {
                if (flags & kLinedUp) {
                    *info |= kLinedUp;
                } else {
                    continue;
                }
            }

            if (flags & kFreeItem or flags & kHidingSpot) {
                shObjectVector *v = mLevel->getObjects (x, y);
                shObject *obj;
                int i;

                if (v) {
                    for (i = 0; i < v->count (); i++) {
                        obj = v->get (i);
                        if (canHideUnder (obj)) {
                            *info |= kHidingSpot;
                        }
                        /* This is needed because you could bring droid wreck
                           to armor/weapon store then repair it to make it
                           pick up goodies and lure it out. */
                        if (obj->isUnpaid ()) {
                            continue;
                        }
                        if (flags & kFreeMoney and obj->isA (kMoney)) {
                            *info |= kFreeMoney;
                        } else if (flags & kFreeWeapon and
                                   (obj->isA (kWeapon) or obj->isA (kRayGun)))
                        {
                            *info |= kFreeWeapon;
                        } else if (flags &kFreeArmor and obj->isA (kArmor)) {
                            *info |= kFreeArmor;
                        } else if (flags &kFreeEnergy and
                                   obj->isA (kEnergyCell))
                        {
                            *info |= kFreeEnergy;
                        } else if (flags &kFreeWreck and obj->isA (kObjWreck)) {
                            *info |= kFreeWreck;
                        }
                    }
                }
            }

            coord->mX = x;
            coord->mY = y;
            ++coord;
            ++info;
            ++n;
        }
    }
    return n;
}


shAttackId
shMonster::pickRangedAttack ()
{   /* First attack to pass one in prob chance is chosen. */
    shMonsterIlk::shAttackData *table = myIlk ()->mRangedAttacks;
    for (int i = 0; i < MAXATTACKS; ++i) {
        if (!table[i].mAttId)  break;
        if (!RNG (table[i].mProb))  return table[i].mAttId;
    }
    return kAttDummy; /* Do not attack at range. */
}


void
shMonster::doRangedAttack (shAttack *atk, shDirection dir)
{
    const char *buf = the ();

    Hero.interrupt ();

    if (mHidden and revealSelf ())  return;

	switch (atk->mEffect) {
    case shAttack::kBeam:
        switch (atk->mType) {
        case shAttack::kBreatheFire:
            I->p ("%s breathes flames!", buf); break;
        case shAttack::kBreatheBugs:
            I->p ("%s breathes bugs!", buf); break;
        case shAttack::kBreatheViruses:
            I->p ("%s breathes viruses!", buf); break;
        case shAttack::kBreatheTime:
            I->p ("%s breathes time!", buf); break;
        case shAttack::kBreatheTraffic:
            I->p ("%s breathes megabytes!", buf); break;
        case shAttack::kLaserBeam:
            I->p ("%s shoots a laser!", buf); break;
        case shAttack::kSpit:
            I->p ("%s spits green sludge!", buf); break;
        case shAttack::kFreezeRay:
        case shAttack::kGammaRay:
        case shAttack::kGaussRay:
        case shAttack::kHeatRay:
        case shAttack::kPoisonRay:
        case shAttack::kStasisRay:
        case shAttack::kTransporterRay:
            I->p ("%s opens its jaw emitting a ray!", buf); break;
        case shAttack::kPlague:
            I->p ("%s vomits!", buf); break;
        default: /* Silent. */ break;
        }
        Level->areaEffect (atk, NULL, mX, mY, dir, this, 0);
        break;
    case shAttack::kSingle:
        switch (atk->mType) {
        case shAttack::kShot:
            I->p ("%s fires!", buf); break;
        case shAttack::kWeb:
            I->p ("%s weaves and throws a web!", buf); break;
        default: /* Silent. */ break;
        }
        this->shootWeapon (NULL, dir, atk);
        break;
    case shAttack::kBurst:
        if (is (kUnableUsePsi)) {
            if (Hero.canSee (this)) I->p ("%s closes %s.", the (), her ("eyes"));
            break;
        }
        if (atk->mType == shAttack::kExplode)
            I->p ("%s unleashes a psychic wave!", buf);
        //TODO: Should now rely merely on attack definition.
        atk->mRadius = 5;
        die (kSuicide);
        Level->areaEffect (atk, NULL, mX, mY, dir, this, 0);
        break;
    default:
        debug.log ("UNIMPLEMENTED RANGED ATTACK");
    }

}


shAttackId
shMonster::pickMeleeAttack ()
{   /* Melee attack is chosen by weight. */
    shMonsterIlk::shAttackData *table = myIlk ()->mAttacks;
    int score = RNG (myIlk ()->mAttackSum);
    for (int i = 0; i < MAXATTACKS; ++i) {
        if (!table[i].mAttId)  break;  /* No melee attacks?  Can happen. */
        score -= table[i].mProb;
        if (score <= 0)  return table[i].mAttId;
    }
    return kAttDummy; /* Monster unable to attack without weapon. */
}


/* attempt a melee attack against the target
   returns:   1 if target is eliminated (dies, teleports, etc)
              0 if target was attacked
             -1 if attack was a miss
             -2 if attacker dies
             -3 if no attack was made
   modifies: elapsed is set to elapsed AP
*/
int
shMonster::doAttack (shCreature *target, int *elapsed)
{
    const char *t_monster = the ();
    const char *t_weapon;

    if (myIlk ()->mAttacks[0].mAttId == kAttDummy) {
        *elapsed = HALFTURN;
        return -3;
    }
    if (kAlien == mType and
        target->isHero () and
        Hero.getStoryFlag ("impregnation") and
        mHP == mMaxHP)
    { /* Aliens won't attack creatures that have already
         been impregnated (until they've taken damage) */
        *elapsed = HALFTURN;
        return -3;
    }

    if (mHidden) {
        int res = revealSelf ();
        if (res) {
            *elapsed = res;
            return -3;
        }
    }

    shAttackId attid = kAttDummy;

    if (NULL == mWeapon) {
        attid = pickMeleeAttack ();
    } else if (mWeapon->isMeleeWeapon ()) {
        attid = mWeapon->myIlk ()->mMeleeAttack;
    } else {
        if (target->isHero () and Hero.canSee (this)) {
            t_weapon = mWeapon->her (this);
            I->p ("%s swings %s at you!", t_monster, t_weapon);
        }
        attid = kAttImprovisedMelee;
    }
    shAttack *atk = &Attacks[attid];

    if (attid and is (kUnableAttack)) {
        *elapsed = SLOWTURN;
        if (Hero.canSee (this)) {
            if (mWeapon) {
                I->p ("%s waves %s helplessly.", the (), THE (mWeapon));
            } else {
                I->p ("%s demonstrates %s.", the (), her (atk->noun ()));
            }
        }
        return -3;
    }

    if (!attid) {
        *elapsed = HALFTURN;
        return -3;
    } else {
        *elapsed = atk->mAttackTime;
        if (shAttack::kExplode == atk->mType) {
            if (target->isHero () or Hero.canSee (this)) {
                I->p ("%s explodes!", t_monster);
            }
            die (kSuicide, NULL, NULL, atk);
            return -2;
        }
        return meleeAttack (mWeapon, atk, target->mX, target->mY);
    }
}


/* try to return attack against a pet who attacked from mEnemyX,Y */
int
shMonster::doRetaliate ()
{
    shCreature *c = mLevel->getCreature (mEnemyX, mEnemyY);
    int elapsed;
    int result;

    debug.log ("retaliating against %d %d", mEnemyX, mEnemyY);
    if (c and c->isPet ()) {
        result = doAttack (c, &elapsed);
        if (-2 == result) {
            return -2;
        } else if (-3 != result) {
            mEnemyX = -1;
        }
        return elapsed;
    } else {
        mEnemyX = -1;
    }
    return 0;
}


int
shMonster::doWander ()
{
    int i, n;
    shCoord coord[9];
    int info[9];
    int best = -1;
    int score, bestscore = -9999;
    int flags = 0;
    int res = -1;
    int dist;
    int health = mHP * 3 / mMaxHP;
    int gridbug = isA (kMonGridBug);
    int scrubbot = isA (kMonScrubbot);

    int val_linedup;
    int val_adjacent;
    int val_near;
    int val_medium;
    int val_far;
    int val_owntrack;   // where this monster has been
    int val_htrack;     // where the hero has been
    int val_same;
    int val_money;
    int val_weapon;
    int val_armor;
    int val_energy;
    int val_wreck = scrubbot ? 20 : 0;
    int val_crowd = isMultiplier () ? -2 : 0;
    int val_hidingspot; // obj or feature big enough to hide under */

    int elapsed;
    int hasrangedweapon = 0;

    if (mDestX < 0) {
        mLevel->findUnoccupiedSquare (&mDestX, &mDestY);
    }

    if (!isHostile ()) {
        flags = kLinedUp | kDoor;
        val_adjacent = 0;
        val_linedup = 0;
        val_near = 0;
        val_medium = 0;
        val_far = 0;
        val_owntrack = -2;
        val_htrack = -3;
        val_same = -5;
        val_money = 0;
        val_weapon = 0;
        val_armor = 0;
        val_energy = 0;
        val_hidingspot = 0;
    } else if (myIlk ()->mNumPowers and RNG (3) and
               (res = useMutantPower ()))
    {
         return res;
    } else if (canSee (&Hero) or canSmell (&Hero) or canSenseLife (&Hero)) {
        mDestX = Hero.mX;
        mDestY = Hero.mY;

        if (getInt () > 7 and !RNG(3)) {
            // tell other monsters where the hero is
            mLevel->alertMonsters (mX, mY, 50, Hero.mX, Hero.mY);
            debug.log ("  alerting monsters near %d %d", mX, mY);
        }


        res = readyWeapon ();
        if (-1 != res) {
            return res;
        }

        val_money = 0;
        val_weapon = (numHands () and getInt () > 7) ? 35 : 0;
        val_armor = 0;
        val_energy = (numHands ()) ? 30 : 0;
        val_hidingspot = canSee (&Hero) ? 0 : 40;

        if (myIlk ()->mRangedAttacks[0].mAttId and
            canSee (&Hero) and distance (this, &Hero) < 60)
        {
            shAttackId attid = pickRangedAttack ();
            if (attid) {
                shDirection dir = linedUpDirection (this, &Hero);
                if (dir != kNoDirection) {
                    shAttack *atk = &Attacks[attid];
                    doRangedAttack (atk, dir);
                    return atk->mAttackTime;
                }
            }
            hasrangedweapon = 1;
        }

        if (mWeapon and !mWeapon->isMeleeWeapon ()) {
            if (!hasAmmo (mWeapon)) {
                return readyWeapon ();
            }

            int maxrange;
            if (mWeapon->isThrownWeapon ())
                maxrange = 60; //FIXME: calculate range
            else if (mWeapon->isA (kRayGun))
                maxrange = 45;
            else
                maxrange = Attacks[mWeapon->myIlk ()->mGunAttack].mRange * 5 + 5;

            shDirection dir = linedUpDirection (this, &Hero);
            if (kNoDirection != dir and
                RNG (10) and
                canSee (&Hero) and
                distance (this, &Hero) < maxrange)
            {
                if (mHidden) {
                    int res = revealSelf ();
                    if (res)  return res;
                }
                if (mWeapon->isThrownWeapon ()) {
                    if (is (kUnableAttack)) {
                        if (Hero.canSee (this)) {
                            if (mWeapon->mCount > 1) {
                                I->p ("%s juggles %s.", the (), THE (mWeapon));
                            } else {
                                I->p ("%s tosses %s from hand to hand.",
                                    the (), THE (mWeapon));
                            }
                        }
                        return FULLTURN;
                    }
                    shObject *obj = removeOneObjectFromInventory (mWeapon);
                    elapsed = throwObject (obj, NULL, dir);
                } else {
                    if (is (kUnableAttack)) {
                        if (Hero.canSee (this)) {
                            I->p ("%s pets %s.", the (), mWeapon->her (this));
                        }
                        return FULLTURN;
                    }
                    elapsed = shootWeapon (mWeapon, dir);
                }
                if (-2 == elapsed) {
                    die (kSuicide);
                }
                return elapsed;
            }

            if (mHidden) {
                return HALFTURN;
            }

            flags = kLinedUp | kDoor;
            val_linedup = 15;    /* try to line up a shot */
            val_adjacent = -10;

            if (is (kFleeing) or health < 1) { /* hurt, retreat */
                val_near = 2;
                val_medium = 1;
                val_far = 0;
                val_linedup = -5;
                val_htrack = -3;
            } else { /* close in for a shot */
                val_near = 0;
                val_medium = 1;
                val_far = -1;
                val_htrack = 3;
            }

            val_owntrack = -3;
            val_same = -2;
        } else {
            if (areAdjacent (this, &Hero) and !is (kFleeing)) {
                if (gridbug and abs (Hero.mX-mX) + abs (Hero.mY-mY) > 1) {
                    /* can't attack diagonally */
                } else if (-2 == doAttack (&Hero, &elapsed)) {
                    return -2;
                } else {
                    return elapsed;
                }
            }

            if (mHidden) {
                return HALFTURN;
            }

            if (isMultiplier () and !RNG (mHP > 1 ? 6 : 12) and
                mLevel->countAdjacentCreatures (mX, mY) < 5 and
                mLevel->mCrList.count () < 150)
            {
                int x = mX;
                int y = mY;
                if (0 == mLevel->findAdjacentUnoccupiedSquare (&x, &y) and
                    mLevel->countAdjacentCreatures (x, y) < 5)
                {
                    shMonster *m = new shMonster (mIlkId);
                    if (0 == mLevel->putCreature (m, x, y)) {
                        if (mHP > 1) {
                            --mHP;
                            --mMaxHP;
                        }
                        //m->mHP = m->mMaxHP = mHP;
                    } else {
                        /* delete m; */
                    }
                    return LONGTURN;
                }
            }

            flags = kLinedUp | kDoor;
            if (hasrangedweapon) {
                val_linedup = 2;
            } else if (mCLevel < 2) {
                val_linedup = 0;
            } else if (getInt () < 7) {
                val_linedup = 1;
            } else if (getInt () < 10) {
                val_linedup = -3;
            } else {
                val_linedup = -10;
            }

            if (is (kFleeing)) {
                val_near = 1;
                val_medium = 1;
                val_far = 1;
                val_linedup = -5;
                val_htrack = -3;
            } else {
                val_adjacent = 30;
                val_near = -1;
                val_medium = -1;
                val_far = -1;
                val_htrack = 3;
            }
            val_owntrack = -10;
            val_same = -10;
        }
    } else { /* don't know where hero is */
        if (mHidden) {
            return HALFTURN;
        }

        if (canMimicObjects ()) {
            mimicSomething ();
            return FULLTURN;
        }

        if (mX == mDestX and mY == mDestY) {
            /* already here, what now? */
            if (!mLevel->moveForward (mDir, &mDestX, &mDestY)) {
                mDestX = mX + RNG (13) - 6;
                mDestY = mY + RNG (9) - 4;
                mLevel->findNearbyUnoccupiedSquare (&mDestX, &mDestY);
            }
        }

        flags = kDoor | kLinedUp;
        val_linedup = 0;
        val_adjacent = 0;
        val_near = -3;
        val_medium = -2;
        val_far = -1;
        val_owntrack = -5;
        val_htrack = 10;
        val_same = -20;
        val_money = 0;
        val_weapon = numHands () ? 5 : 0;
        val_energy = (numHands ()) ? 20 : 0;
        val_armor = 0;
        val_hidingspot = 40;
        if (!RNG (40)) { /* wander somewhere new */
            mDestX = mX + RNG (13) - 6;
            mDestY = mY + RNG (9) - 4;
            mLevel->findNearbyUnoccupiedSquare (&mDestX, &mDestY);
        }
    }

    if (mHidden) { /* short circuit the rest of this for now */
        return HALFTURN;
    }

    if (val_money) flags |= kFreeMoney;
    if (val_weapon) flags |= kFreeWeapon;
    if (val_armor) flags |= kFreeArmor;
    if (val_energy) flags |= kFreeEnergy;
    if (val_wreck) flags |= kFreeWreck;
    if (val_hidingspot and canHideUnderObjects ()) flags |= kHidingSpot;

    char buffers[3][3][8] = { { "       ", "       ", "       " },
                              { "       ", "   X   ", "       " },
                              { "       ", "       ", "       " } };

    n = findSquares (flags, coord, info);
    for (i = 0; i < n; i++) {
        char ownt = 0, herot = 0;
        score = 100;
        if (info[i] & kLinedUp) score += val_linedup;
        if (info[i] & kFreeMoney) score += val_money;
        if (info[i] & kFreeWeapon) score += val_weapon;
        if (info[i] & kFreeArmor) score += val_armor;
        if (info[i] & kFreeEnergy) score += val_energy;
        if (info[i] & kFreeWreck) score += val_wreck;
        if (info[i] & kHidingSpot) score += val_hidingspot;
        if (!(info[i] & kFreeItem)) {
            int ti;
            for (ti = 0 ; ti < TRACKLEN; ti++) {
                if (mTrack[ti].mX == coord[i].mX and
                    mTrack[ti].mY == coord[i].mY) {
                    score += val_owntrack;
                    ownt++;
                }
            }
            for (ti = 0 ; ti < TRACKLEN; ti++) {
                if (Hero.mTrack[ti].mX == coord[i].mX and
                    Hero.mTrack[ti].mY == coord[i].mY) {
                    score += val_htrack;
                    herot++;
                }
            }
            if (coord[i].mX == mX and coord[i].mY == mY) {
                if (info[i] & kHidingSpot) {
                    /* this prevents endless pacing between adjacent
                       hiding spots */
                    score += 5;
                } else {
                    score += val_same;
                }
            }
        }
        if (areAdjacent (coord[i].mX, coord[i].mY, mDestX, mDestY))
            score += val_adjacent;
        dist = rlDistance (coord[i].mX, coord[i].mY, mDestX, mDestY);
        if (dist < 25) {
            score += val_near * dist;
        } else if (dist < 50) {
            score += val_near * 25 + val_medium * (dist - 25);
        } else {
            score += val_near * 25 + val_medium * 25 + val_far * (dist - 50);
        }

        if (val_crowd) {
            score += val_crowd *
                mLevel->countAdjacentCreatures (coord[i].mX, coord[i].mY);
            if (coord[i].mX == mX and coord[i].mY == mY)
                score += val_crowd; /* count as adjacent to self */
        }
/*
        debug.log (" %5d [%2d %2d] %s%s%s", score, coord[i].mX, coord[i].mY,
                  info[i] & kLinedUp ? "X" : "",
                  info[i] & kFreeMoney ? "$" : "",
                  info[i] & kFreeWeapon ? ")" : "");
*/

        {
            sprintf (buffers[coord[i].mY - mY + 1][(coord[i].mX - mX + 1)],
                     " % 4d%c", score,
                     info[i] & kLinedUp ? 'l' :
                     info[i] & kFreeMoney ? '$' :
                     info[i] & kFreeWeapon ? ')' :
                     info[i] & kDoor ? '+' :
                     ownt ? '_' :
                     herot ? 't' : ' ');
        }


        if (score > bestscore or
            (score == bestscore and RNG (2)))
        {
            bestscore = score;
            best = i;
        }
    }

    debug.log ("  dest: %d %d   Near/Medium/Far %d %d %d", mDestX, mDestY,
              val_near, val_medium, val_far);
    for (i = 0; i < 3; i++)
        debug.log ("  %7s%7s%7s", buffers[i][0], buffers[i][1], buffers[i][2]);


    if (-1 == best) {
        /* nothing to do but stay where we are for now */
        return HALFTURN;
    }
    if (coord[best].mX == mX and coord[best].mY == mY) {
        /* apparently, we like where we are, try to do some things here: */
        shFeature *f = mLevel->getFeature (mX, mY);
        shObjectVector *v = mLevel->getObjects (mX, mY);
        shObject *obj;

        if (!mHidden and mLevel->isWatery (mX, mY) and canHideUnderWater () and !canSee (&Hero)) {
            mHidden = RNG (1, 20) + 10;
            return FULLTURN;
        }

        if (!mHidden and f and canHideUnder (f) and !canSee (&Hero)) {
            mHidden = RNG (1, 20) + 10;
            return FULLTURN;
        }

        if (!mHidden and v and canHideUnderObjects () and !canSee (&Hero)) {
            for (i = 0; i < v->count (); i++) {
                obj = v->get (i);
                if (canHideUnder (obj)) {
                    mHidden = RNG (1, 20) + 10;
                    return FULLTURN;
                }
            }
        }

        if (!mHidden and !mLevel->isWatery (mX, mY) and canBurrow () and !canSee (&Hero)) {
            mHidden = RNG (1, 15) + 15;
            if (Hero.canSee (this)) {
                I->p ("%s burrows.", the ());
                mHidden = -mHidden;
            }
            return LONGTURN;
        }

        /* try to pick up some junk off the floor */
        if (!mHidden and v) {
            for (i = 0; i < v->count (); i++) {
                obj = v->get (i);

                if (!mHidden and canHideUnder (obj)) {
                    mHidden = RNG (1, 20) + 10;
                    return FULLTURN;
                }

                if (numHands () and
                   ((info[best] & kFreeMoney and obj->isA (kMoney)) or
                    (info[best] & kFreeWeapon and obj->isA (kWeapon)) or
                    (info[best] & kFreeWeapon and obj->isA (kRayGun)) or
                    (info[best] & kFreeArmor and obj->isA (kArmor)) or
                    (info[best] & kFreeEnergy and obj->isA (kEnergyCell))))
                {
                    if (Hero.canSee (this)) {
                        I->p ("%s picks up %s.", the (), AN (obj));
                    }
                    addObjectToInventory (obj);
                    v->remove (obj);
                    if (0 == v->count ()) {
                        delete v;
                        mLevel->setObjects (mX, mY, NULL);
                    }
                    return FULLTURN;
                }
                if (info[best] & kFreeWreck and obj->isA (kObjWreck)) {
                    v->remove (obj);
                    if (0 == v->count ()) {
                        delete v;
                        mLevel->setObjects (mX, mY, NULL);
                    }
                    return utilizeWreck (obj);
                }
            }
        }
        return QUICKTURN;
    } else {
        return doQuickMoveTo (vectorDirection (mX, mY,
                                               coord[best].mX, coord[best].mY));
    }
}


int
shMonster::doHatch ()
{
    debug.log ("hatch strategy");
    int chance = 100000;

    if (mAlienEgg.mHatchChance) {
        /* we've been disturbed by something */
        --mAlienEgg.mHatchChance;
        chance = 2;
    } else if (distance (this, &Hero) <= 15 and
               !Hero.getStoryFlag ("impregnation"))
    {
        chance = canSee (&Hero) ? 6 : 22;
    }
    if (!RNG (chance)) {
        shMonster *facehugger = new shMonster (kMonFacehugger);
        int x = mX;
        int y = mY;
        assert (facehugger);
        if (Hero.canSee (this)) {
            I->p ("The alien egg hatches!");
        }
        mLevel->removeCreature (this);
        mState = kDead;
        mLevel->putCreature (facehugger, x, y);
        return -2;
    }
    return FULLTURN;
}


/* sitstill strategy: do nothing until Hero is adjacent, then attack
   returns ms elapsed, -2 if the monster dies
*/
int
shMonster::doSitStill ()
{
    int elapsed;

    if (!isHostile ()) {
        return FULLTURN;
    }

    if (myIlk ()->mRangedAttacks[0].mAttId and canSee (&Hero)) {
        shAttackId attid = pickRangedAttack ();
        if (attid) {
            shAttack *atk = &Attacks[attid];
            shDirection dir = linedUpDirection (this, &Hero);
            if (kNoDirection != dir or
                distance (this, &Hero) * 5 + 5 < atk->mRange)
            {
                doRangedAttack (atk, dir);
                return atk->mAttackTime;
            }
        }
    }

    if (myIlk ()->mNumPowers) {
        int res = useMutantPower ();
        if (-1 == res) return -2;
        if (res) return res;
    }

/* at first, I had sessile creatures attack any monster that was adjacent,
   but that just resulted in a lot of annoying "you hear combat" messages
   and fewer monsters for the hero to fight
*/
    if (Hero.isAdjacent (mX, mY)) {
        if (-2 == doAttack (&Hero, &elapsed)) {
            return -2;
        } else {
            return elapsed;
        }
    } else {
        return RNG (HALFTURN, LONGTURN);
    }
}


/* returns -2 if died

*/

int
shMonster::mimicSomething ()
{
    if (canMimicMoney ()) {
        mHidden = RNG (1, 20) + 10;
        mMimic = kObject;
        mMimickedObject = &AllIlks[kObjMoney];
    } else if (canMimicObjects ()) {
        shObjectIlk *ilk = NULL;
        int tries = 10;
        while (!ilk and tries--) {
            shObject *obj = generateObject (50);
            if (obj->myIlk ()->mSize == getSize ()) {
                ilk = obj->myIlk ();
            }
            delete obj;
        }

        if (!ilk) {
            return 0;
        }

        mHidden = RNG (1, 20) + 10;
        mMimic = kObject;
        mMimickedObject = ilk;
    }
    return 0;
}


/* hide strategy: do nothing until Hero is adjacent, then surprise attack
   returns: elapsed time
            -2 if monster dies
 */
int
shMonster::doHide ()
{
    int elapsed;
    if (Hero.isAdjacent (mX, mY)) {
        mHidden = 0;
        if (-2 == doAttack (&Hero, &elapsed)) {
            return -2;
        } else {
            return elapsed;
        }
    }
    if (mHidden) {
        /* stay hidden */
        return RNG (500, 1500);
    } else if (Hero.canSee (this)) {
        /* Hero sees us, no point in hiding now... */
        return doWander ();
    } else {
        mHidden = RNG (1, 20);
        if (isA (kMonCreepingCredits)) {
            mHidden += 10; /* too lazy to add skill modifier */
            mMimic = kObject;
            mMimickedObject = &AllIlks[kObjMoney];
        }
        return RNG (2000);
    }
}


/* lurk strategy: do nothing until Hero stumbles on us
   returns ms elapsed, -2 if the monster dies
*/
int
shMonster::doLurk ()
{
    debug.log ("  lurk strategy");
    int elapsed;

    if (canSee (&Hero) or (canSmell (&Hero) and !RNG(22)) or
        mTactic == kNewEnemy)
    {
        mStrategy = kWander;
        mTactic = kReady;
        if (isA (kMonDalek)) {
            I->p ("%s screams in irritating high-pitched voice: \"EXTERMINATE!\"", an ());
        }
        elapsed = readyWeapon ();
        if (-1 == elapsed) {
            return doWander ();
        } else {
            return elapsed;
        }
    }
    return RNG (300, 1000); /* keep on lurking */
}



#define GD_UNCHALLENGED 0
#define GD_CHALLENGED   1
#define GD_JANITOR      2

int
shMonster::doGuard ()
{
    if (isHostile ()) {
        mStrategy = kWander;
        mTactic = kNewEnemy;
        return doWander ();
    }

    if (canSee (&Hero)) {
        const char *the_guard = the ();
        readyWeapon ();
        if (mGuard.mToll > 0) {
            if (Hero.looksLikeJanitor ()) {
                switch (mGuard.mChallengeIssued) {
                case 2:
                    goto done;
                case 1:
                    mGuard.mChallengeIssued = 2;
                    if (Hero.tryToTranslate (this)) {
                        I->p ("\"Oh, it's you.  Get in there and clean up"
                              " in aisle %d!\"", RNG (1, 8));
                    } else {
                        I->p ("%s emits some gruff beeps.", the_guard);
                    }
                    goto done;
                case 0:
                default:
                    mGuard.mChallengeIssued = 2;
                    if (Hero.tryToTranslate (this)) {
                        I->p ("\"You may pass, janitor.\"");
                    } else {
                        I->p ("%s emits some gruff beeps.", the_guard);
                    }
                    goto done;
                }
            }

            if (Hero.mX >= mGuard.mSX and Hero.mX <= mGuard.mEX and
                Hero.mY >= mGuard.mSY and Hero.mY <= mGuard.mEY)
            {
                switch (mGuard.mChallengeIssued) {
                case 0: /* The Hero somehow slipped past (e.g. transportation)
                           without the guard issuing a challenge. */
                    mGuard.mToll *= 2;
                    if (Hero.tryToTranslate (this)) {
                        I->p ("\"Halt!  You must pay a fine of %d buckazoids "
                              "for trespassing!\"", mGuard.mToll);
                    } else {
                        I->p ("%s beeps an ultimatum concerning %d "
                              "buckazoids.", the_guard, mGuard.mToll);
                    }
                    break;
                case 2: /* The Hero was once dressed as a janitor but now
                           appears in street clothes. */
                    mGuard.mToll /= 2;
                    if (Hero.tryToTranslate (this)) {
                        I->p ("\"Halt!  You must pay a fine of %d buckazoids "
                              " for removing your uniform while on duty!\"",
                              mGuard.mToll);
                    } else {
                        I->p ("%s beeps an ultimatum concerning %d "
                              "buckazoids.", the_guard, mGuard.mToll);
                    }
                    break;
                case 1: default: /* The Hero ignored the challenge,
                                    and must be vaporized. */
                    goto getangry;
                }

                if (I->yn ("Will you pay?")) {
                    if (Hero.countMoney () < mGuard.mToll) {
                        I->p ("You don't have enough money.");
                    } else {
                        Hero.loseMoney (mGuard.mToll);
                        gainMoney (mGuard.mToll);
                        mGuard.mToll = 0;
                        if (Hero.tryToTranslate (this)) {
                            I->p ("\"You may pass.\"");
                        } else {
                            I->p ("%s beeps calmly.", the_guard);
                        }
                        return FULLTURN;
                    }
                }
            getangry:
                makeAngry ();
                mStrategy = kWander;
                return doWander ();
            }

            switch (mGuard.mChallengeIssued) {
            case 1:
                goto done;
            case 2:
                if (Hero.tryToTranslate (this)) {
                    I->p ("\"If you're not on duty, you must pay %d "
                          "buckazoids to pass this way.\"", mGuard.mToll);
                } else {
                    I->p ("%s emits some intimidating beeps about %d "
                          "buckazoids.", the_guard, mGuard.mToll);
                }
                break;
            case 0:
            default:
                if (Hero.tryToTranslate (this)) {
                    I->p ("\"Halt!  You must pay a toll of %d buckazoids "
                          "to pass this way.\"", mGuard.mToll);
                } else {
                    I->p ("%s emits some intimidating beeps about %d "
                          "buckazoids.", the_guard, mGuard.mToll);
                }
            }
            mGuard.mChallengeIssued = 1;

            if (I->yn ("Will you pay?")) {
                if (Hero.countMoney () < mGuard.mToll) {
                    I->p ("You don't have enough money.");
                } else {
                    Hero.loseMoney (mGuard.mToll);
                    gainMoney (mGuard.mToll);
                    mGuard.mToll = 0;
                    if (Hero.tryToTranslate (this)) {
                        I->p ("\"You may pass.\"");
                    } else {
                        I->p ("%s beeps calmly.", the_guard);
                    }
                }
            }
        }
    } else {
        if (mGuard.mToll > 0 and 1 == mGuard.mChallengeIssued)
            mGuard.mChallengeIssued = 0;
    }
done:
    return FULLTURN;
}


void
shMonster::takeTurn ()
{
    int elapsed;
    int couldsee = Hero.canSee (this);

    /* decide what to do on the monster's turn */

    if (mLevel != Hero.mLevel) {
        /* we'll be reawakened when the Hero returns to our level. */
        mState = kWaiting;
        return;
    }
    debug.log ("* %p %s @ %d, %d has %d AP:", this, getDescription (),
              mX, mY, mAP);

    if (hasAutoRegeneration ()) {
        if (MAXTIME == mLastRegen)
            mLastRegen = Clock;
        while (Clock - mLastRegen > 1000) {
            if (mHP < mMaxHP) ++mHP;
            mLastRegen += 1000;
        }
    }

    if (checkTimeOuts ()) {
        return;
    }

    if (is (kAsleep) or is (kParalyzed)) {
        elapsed = FULLTURN;
    } else if (isTrapped () and !isSessile ()) {
        elapsed = tryToFreeSelf ();
    } else if (is (kStunned) or (is (kConfused) and RNG (2))) {
        if (shMonster::kSitStill == mStrategy or
            shMonster::kHatch == mStrategy)
        {
            /* do nothing */
            elapsed = FULLTURN;
        } else {
            /* move at random */
            shDirection dir = (shDirection) RNG (8);
            elapsed = doMove (dir);
        }
    } else if (-1 != mEnemyX) {
        elapsed = doRetaliate ();
    } else {
        switch (mStrategy) {
        case shMonster::kPet:
            elapsed = doPet (); break;
        case shMonster::kHide:
            elapsed = doHide (); break;
        case shMonster::kLurk:
            elapsed = doLurk (); break;
        case shMonster::kSitStill:
            elapsed = doSitStill (); break;
        case shMonster::kHatch:
            elapsed = doHatch (); break;
        case shMonster::kShopKeep:
            elapsed = doShopKeep (); break;
        case shMonster::kAngryShopKeep:
            elapsed = doAngryShopKeep (); break;
        case shMonster::kGuard:
            elapsed = doGuard (); break;
        case shMonster::kDoctor:
            elapsed = doDoctor (); break;
        case shMonster::kMelnorme:
            elapsed = doMelnorme (); break;
        case shMonster::kWander:
        default:
            elapsed = doWander (); break;
        }
    }
    //debug.log ("%d elapsed", elapsed);
    if (-2 == elapsed) {

    } else {
        if (!couldsee and !isPet () and Hero.canSee (this)) {
            Hero.interrupt ();
        }
        if (is (kStunned)) {
            elapsed = HALFTURN;
        }
        mAP -= elapsed;
    }
}


int
shMonster::likesMoney ()
{
    return isA (kMonCreepingCredits);
}


int
shMonster::likesWeapons ()
{
    return (myIlk ()->mNumHands > 0) ? 1 : 0;
}


int
shMonster::needsWeapon ()
{
    if (mWeapon and !hasAmmo (mWeapon)) {
        return 1;
    }
    return (myIlk ()->mNumHands > 0 and NULL == mWeapon) ? 1 : 0;
}

/* This is becoming too long and unwieldy.  A better algorithm to compare
   weapons could come handy. */
static const shObjId weaponpreferences[] = {
    kObjDisintegrationRayGun,
    kObjFlashbang,
    kObjStunGrenade,
    kObjRailgun,
    kObjFreezeRayGun,
    kObjHeatRayGun,
    kObjPoisonRayGun,
    kObjPlasmaCannon,
    kObjAutoCannon,
    kObjLaserCannon,
    kObjIncendiaryGrenade,
    kObjGammaRayGun,
    kObjPulseRifle,
    kObjPlasmaRifle,
    kObjSniperRifle,
    kObjConcussionGrenade,
    kObjFragGrenade,
    kObjShotgun,
    kObjLaserRifle,
    kObjAssaultPistol,
    kObjPlasmaPistol,
    kObjFlamer,
    kObjLightFlamer,
    kObjBlaster,
    kObjPhaser,
    kObjPistol,
    kObjLaserPistol,
    kObjPeaShooter,
    kObjLightSaber,
    kObjPowerFist,
    kObjElvenSword,
    kObjKatana,
    kObjPowerClaw,
    kObjChainsaw,
    kObjZapBaton,
    kObjNunchucks,
    kObjCattleProd,
    kObjGomJabbar,
    kObjBo,
    kObjZapGun,
    kObjMop,
    kObjAnalProbe,
    kObjClub,
    kObjNothing
};

/* selects and ready a weapon
   returns: ms elapsed, -1 if nothing done
*/
int
shMonster::readyWeapon ()
{
    int i;
    shObject *best = NULL;
    int bestrank = 1000;
    int j;

/*
    if (!needsWeapon ()) {
        return -1;
    }
*/
    if (myIlk ()->mNumHands < 1) {
        return -1;
    }

    for (i = 0; i < mInventory->count (); i++) {
        shObject *obj = mInventory->get (i);
        if (!(obj->isA (kWeapon) or obj->isA (kRayGun)) or
            !hasAmmo (obj))
        {
            continue;
        }

        if (obj->isA (kObjStasisRayGun) and
            !Hero.is (kParalyzed) and !Hero.is (kAsleep))
        {
            best = obj;
            break;
        }
        for (j = 0; weaponpreferences[j] != kObjNothing and j < bestrank; j++) {
            if (obj->isA (weaponpreferences[j])) {
                bestrank = j;
                best = obj;
            }
        }
        if (!best and
            !obj->isA (kRayGun))  /* don't accidentaly use helpful ray guns! */
        {
            /* somehow we fell through here because a weapon isn't in the
               the preferences list
             */
            best = obj;
        }
    }

    if (best) {
        if (best == mWeapon) {
            return -1;
        }
        wield (best);
        return FULLTURN;
    } else {
        wield (NULL);
        return -1;
    }
}


void
shMapLevel::makeNoise (int x, int y, int radius)
{
    shCreature *c;
    int i;
    int d;

    for (i = 0; i < mCrList.count (); i++) {
        c = mCrList.get (i);
        d = distance (x, y, c->mX, c->mY);
        if (d < radius or
            (d < 2 * radius and RNG(2)))
        {
            /* do something to wakeup the mosnter*/
        }
    }
}


void
shMapLevel::alertMonsters (int x, int y, int radius,
                           int destx, int desty)
{
    shCreature *c;
    int i,d;

    for (i = 0; i < mCrList.count (); i++) {
        c = mCrList.get (i);
        d = distance (x, y, c->mX, c->mY);
        if (d < radius or
            (d < 2 * radius and RNG(2)))
        {
            if (!c->isHero () and c->isHostile () and !c->isPet () and
                !c->is (kFleeing) and !c->is (kAsleep))
            {
                shMonster *m = (shMonster *) c;
                if (m->getInt () < 5)
                    continue;

                if (!m->canSee (&Hero)) {
                    if (shMonster::kLurk == m->mStrategy)
                        m->mStrategy = shMonster::kWander;
                    m->mDestX = destx + RNG (9) - 4;
                    m->mDestY = desty + RNG (7) - 3;
                }
            }
        }
    }
}
