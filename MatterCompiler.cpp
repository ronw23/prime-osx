/* Matter compiler. This one is huge for just a single floppy disk program. */

#include "Global.h"
#include "Util.h"
#include "Object.h"
#include "Hero.h"

extern int hackingRoll (shObject *computer);

#define NOFEATURE shFeature::kMaxFeatureType
static const struct shIlkPickData
{
    int score;
    const char *dispName;
    const char *ilkName;
    shFeature::Type type;
} pickData[] =
{
    {0, "Relinquish control", NULL, NOFEATURE},
    {1,  "random item", "ask RNG", NOFEATURE},
    /* Armor. */
    {0, "Armor", NULL, NOFEATURE},
    {16, "armor piece", "armor piece", NOFEATURE},
    {23, "jumpsuit", "jumpsuit", NOFEATURE},
    {24, "body armor", "body armor", NOFEATURE},
    {25, "headwear", "helmet", NOFEATURE},
    {26, "eyewear", "pair of goggles", NOFEATURE},
    {26, "cloak", "cloak", NOFEATURE},
    {27, "belt", "belt", NOFEATURE},
    {27, "boots", "pair of boots", NOFEATURE},
    {30, "armor of chaos", "chaos power armor", NOFEATURE},
    /* Weapons. */
    {0, "Weapons", NULL, NOFEATURE},
    {16, "weapon", "weapon", NOFEATURE},
    {20, "ammunition", "ammunition", NOFEATURE},
    {23, "melee weapon", "melee weapon", NOFEATURE},
    {23, "gun", "gun", NOFEATURE},
    {24, "grenade", "grenade", NOFEATURE},
    {25, "energy gun", "energy gun", NOFEATURE},
    {26, "conventional gun", "conventional gun", NOFEATURE},
    {26, "energy handgun", "energy handgun", NOFEATURE},
    {26, "energy light gun", "energy light gun", NOFEATURE},
    {26, "energy heavy gun", "energy heavy gun", NOFEATURE},
    {26, "conventional handgun", "handgun", NOFEATURE},
    {26, "conventional light gun", "light gun", NOFEATURE},
    {26, "conventional heavy gun", "heavy gun", NOFEATURE},
    /* Tools. */
    {0, "Tools", NULL, NOFEATURE},
    {17, "tool", "tool", NOFEATURE},
    {21, "keycard", "keycard", NOFEATURE},
    {28, "computer", "computer", NOFEATURE},
    {30, "power plant", "power plant", NOFEATURE},
    /* Miscellaneous. */
    {0,  "Miscellaneous", NULL, NOFEATURE},
    {25, "canister", "canister", NOFEATURE},
    {27, "ray gun", "ray gun", NOFEATURE},
    {29, "bionic implant", "implant", NOFEATURE},
    /* Terrain features. */
    {0,  "Features", NULL, NOFEATURE},
    {20, "door", NULL, shFeature::kDoorOpen},
    {23, "radiation trap", NULL, shFeature::kRadTrap},
    {27, "sludge vat", NULL, shFeature::kVat}
};
static const int pickCount = sizeof (pickData) / sizeof (struct shIlkPickData);

/* Returns obtained weight. */
static int
decompileFeature ()
{
    shFeature *f = Level->getFeature (Hero.mX, Hero.mY);
    switch (f->mType) {
        case shFeature::kDoorOpen:
            I->p ("The door beside you is decompiled!");
            /* This encourages dungeon vandalism, I know. */
            Level->removeFeature (f);
            I->pause ();
            /* Nooo! I needed that door! */
            if (Level->isInShop (Hero.mX, Hero.mY)) {
                Hero.damagedShop (Hero.mX, Hero.mY);
            }
            return 10000;
        case shFeature::kVat:
            I->p ("The sludge vat next to you is decompiled!");
            Level->removeFeature (f);
            I->pause ();
            return 20000;
        case shFeature::kAcidPit:
            I->p ("Suddenly the acid pool beneath you is gone.");
            f->mType = shFeature::kPit;
            I->pause ();
            return 500;
        case shFeature::kRadTrap:
            I->p ("The rad trap you are standing on is decompiled!");
            if (Hero.mProfession == Janitor) {
                I->p ("Nice cleaning job.");
                Hero.earnXP (Level->mDLevel);
            }
            Level->removeFeature (f);
            I->pause ();
            return 2500;
        case shFeature::kSewagePit:
            /* Sewage decompilation handled by core routine. */
            f->mType = shFeature::kPit;
            if (Hero.mTrapped.mDrowning) {
                I->p ("You can now breathe freely!");
                Hero.mTrapped.mDrowning = 0;
            }
            return 750; /* More sewage fits in deep spot. */
        //case shFeature::kComputerTerminal:
        default: /* Feature cannot be decompiled. */
            break;
    }
    return 0;
}

/* Returns 1 if hero stands between two walls and 0 otherwise. */
static int
isDoorSpot ()
{
    return
    /* Horizontal. */
    ((Level->isObstacle (Hero.mX-1, Hero.mY) and
      Level->isObstacle (Hero.mX+1, Hero.mY)) or
    /* Vertical. */
     (Level->isObstacle (Hero.mX, Hero.mY-1) and
      Level->isObstacle (Hero.mX, Hero.mY+1)));
}

/* Returns 1 if hero is on a level where doors could be added. */
static int
isDoorLevel ()
{
    return Level->mMapType == shMapLevel::kBunkerRooms or Level->isTownLevel ();
}

/* Returns ilk name and kNoFeature in kind or NULL and feature type in kind. */
static const char *
pickCompilerTarget (int score, shFeature::Type *kind)
{
    shMenu *pickIlk = I->newMenu ("choose object type", 0);
    shFeature *f = Level->getFeature (Hero.mX, Hero.mY);
    char letter = 'a';
    const char *headerstr = NULL;
    int header_placed = 1;
    int options = 0;
    for (int i = 0; i < pickCount; ++i) {
        int skill_or_luck =
            (score >= pickData[i].score or // Through skill.
             !RNG (pickData[i].score));    // Through luck.
        if (pickData[i].score == 0) {
        /* Do not place header unless an object needs it. */
            headerstr = pickData[i].dispName;
            header_placed = 0;
        /* Give first (random) pick and others 80% of the time
           if other conditions are fulfilled. */
        } else if ((RNG (5) or !options) and skill_or_luck) {
            /* Offer features only if place is available. */
            if ((pickData[i].type != NOFEATURE and f == NULL and
                /* Offered feature is not a door or... */
                (pickData[i].type != shFeature::kDoorOpen or
                /* ...it matches both door requirements. */
                (isDoorLevel () and isDoorSpot ())))
                /* Objects always fine. */
                or (pickData[i].type == NOFEATURE))
            {
                if (!header_placed) {
                    pickIlk->addHeader (headerstr);
                    header_placed = 1;
                }
                pickIlk->addPtrItem (letter++, pickData[i].dispName, &pickData[i]);
                if (letter > 'z') letter = 'A';
                ++options;
            }
        }
    } /* Time to choose. */
    shIlkPickData *ilkdata = NULL;
    if (options > 1) { /* Have options besides completely random. */
        pickIlk->getPtrResult ((const void **) &ilkdata, NULL);
    }
    delete pickIlk;
    /* Hero gets or wants random item? */
    if (options == 1 or !ilkdata or ilkdata->ilkName == pickData[1].ilkName) {
        *kind = NOFEATURE;
        return NULL;
    } /* Return chosen feature or object. */
    *kind = ilkdata->type;
    return ilkdata->ilkName;
}

/* Returns 1 if succesful and 0 otherwise. */
static int
compileObject (const char *ilkstr, int score, int *weight)
{
    shMenu *pickone = I->newMenu ("select one", 0);
    shObjectVector v;
    char letter = 'a';
    int choices = (score - 5) / 5;
    if (choices <= 0) choices = 1;
    /* Prepare choices and one backup item. */
    for (int i = 0; i < choices + 1; ++i) {
        shObject *obj = NULL;
        if (!ilkstr) { /* Choose fully randomly. */
            do { /* Money is boring.  Floppies are unbalancing.  Do not want. */
                if (obj) delete obj;
                obj = generateObject (Hero.mCLevel);
            } while (obj->isA (kObjMoney) or obj->isA (kFloppyDisk));
        } else { /* Choose from selected category. */
            obj = createObject (ilkstr, 0);
        } /* Should not happen but if it does lets not ruin a good game. */
        if (!obj) {
            debug.log ("Failed to generate \"%s\".", ilkstr);
            I->p ("Suddenly something goes very wrong!");
            return 0;
        } /* Items may be described in varying degrees of accuracy. */
        if ((score >= 15 and RNG (3)) or !RNG (10))
            obj->setAppearanceKnown ();
        if ((score >= 20 and RNG (5)) or !RNG (10))
            obj->setBugginessKnown ();
        if ((score >= 20 and RNG (5)) or !RNG (10))
            obj->setInfectedKnown ();
        if ((score >= 20 and RNG (5)) or !RNG (10))
            obj->setEnhancementKnown ();
        v.add (obj);
        if (i != choices) { /* Hide backup item. */
            if (!RNG (15)) { /* Sometimes give no description at all. */
                pickone->addPtrItem (letter++, "something", obj);
            } else {
                pickone->addPtrItem (letter++, obj->inv (), obj);
            }
        }
    } /* Make your pick. */
    shObject *chosen = NULL;
    int manual_pick = 1;
    if (choices > 1) {
        pickone->getPtrResult ((const void **) &chosen, NULL);
        /* Hero is not satisfied with options? Give backup item. */
        if (!chosen) {
            chosen = v.get (choices);
            manual_pick = 0;
        }
    } else {
        chosen = v.get (0);
    } /* Discard rest. */
    delete pickone;
    for (int i = 0; i < choices + 1; ++i) {
        shObject *obj = v.get (i);
        if (obj != chosen) delete obj;
    } /* Check weight. */
    if (chosen->myIlk ()->mWeight > *weight) {
        if (choices > 1) {
            I->p ("There is not enough particles left to assemble %s.",
                manual_pick ? chosen->inv () : "last item");
        }
        delete chosen;
        return 0;
    } else {
        if (!Hero.isBlind ()) chosen->setAppearanceKnown ();
        Level->putObject (chosen, Hero.mX, Hero.mY);
        *weight -= chosen->myIlk ()->mWeight;
    }
    return 1;
}


static int
compileFeature (shFeature::Type type, int score, int *weight)
{
    const struct shFeatureIlkData
    {
        shFeature * (*spawner) ();
        const char *sthstr;
        int weight;
    } featureData[] =
    {
        {&shFeature::newDoor, "a door", 20000},
        {&shFeature::newVat, "a vat", 100000}
    };
    shMenu *pickone = I->newMenu ("select one", 0);
    shVector<shFeature *> v;
    int row = -1;
    char letter = 'a';
    int choices = (score - 5) / 5;
    if (choices <= 0) choices = 1;
    /* Prepare choices based on type. */
    switch (type) {
        case shFeature::kRadTrap: /* Nothing to choose. */
            if (*weight < 5000) {
                I->p ("You lack particles to assemble it.");
                return 0;
            }
            *weight -= 5000;
            Level->addTrap (Hero.mX, Hero.mY, shFeature::kRadTrap);
            return 1;
        case shFeature::kDoorOpen: row = 0; break;
        case shFeature::kVat:      row = 1; break;
        default:
            I->p ("Unimplemented feature compilation.");
            return 0;
    }
    if (*weight < featureData[row].weight) {
        int lack = featureData[row].weight - *weight;
        I->p ("You fail to assemble %s.", featureData[row].sthstr);
        I->p ("You lack %s particles.",
            lack < 100 ? "just a few" :
            lack < 1000 ? "a small amount of" :
            lack < 2500 ? "a moderate amount of" :
            lack < 5000 ? "a large amount of" : "a huge amount of");
        return 0;
    }
    *weight -= featureData[row].weight;
    for (int i = 0; i < choices + 1; ++i) {
        shFeature *f = (*featureData[row].spawner) ();
        f->mX = Hero.mX;
        f->mY = Hero.mY;
        v.add (f);
        if (i != choices) { /* Hide last option. */
            if ((score >= 15 and RNG (3)) or !RNG (10)) {
                pickone->addPtrItem (letter++, featureData[row].sthstr, f);
            } else {
                pickone->addPtrItem (letter++, f->getDescription (), f);
            }
        }
    } /* Choose among generated features. */
    shFeature *chosen = NULL;
    if (choices > 1) {
        pickone->getPtrResult ((const void **) &chosen, NULL);
        /* Hero is not satisfied with options?  Give backup feature. */
        if (!chosen) {
            chosen = v.get (choices);
        }
    } else {
        chosen = v.get (0);
    } /* Discard rest. */
    delete pickone;
    for (int i = 0; i < choices + 1; ++i) {
        shFeature *obj = v.get (i);
        if (obj != chosen) delete obj;
    }
    Level->addFeature (chosen);
    return 1;
}

/* Acquirement for PRIME. */
shExecResult
doMatterCompiler (shObject *computer, shObject *disk)
{
    if (disk->isBuggy ()) {
        I->p ("Your computer transforms into dust.");
        if (Hero.mProfession == SoftwareEngineer) {
            I->p ("You've heard buggy disks of matter compiler behave like this.");
            disk->setKnown ();
        } else {
            disk->maybeName ();
        }
        return kDestroyComputer;
    }
    int weight = 0;
    int true_orgasmatron = 0;
    if (!disk->isKnown ()) {
        I->p ("You have found a floppy disk of matter compiler!");
        disk->setKnown ();
    }
    /* Proceed to decompile stuff. */
    if (Hero.is (kConfused)) { /* Additionally targets your equipment. */
        shObjectVector v;
        selectObjectsByFunction (&v, Hero.mInventory, &shObject::isWorn);
        int count = 0;
        for (int i = 0; i < v.count (); ++i) {
            shObject *obj = v.get (i);
            if (obj->isA (kImplant) and
                obj->mIlkId != kObjTorc and obj->mIlkId != kObjMechaDendrites)
                continue; /* Implants residing in brain are safe. */
            ++count;
            weight += obj->myIlk ()->mWeight;
            Hero.useUpOneObjectFromInventory (obj);
        }
        if (count) {
            I->p ("Your armor is decompiled.");
        }
        if (Hero.mWeapon) {
            if (Hero.mWeapon == computer) {
                I->p ("Your computer transforms into dust.");
                return kDestroyComputer;
            }
            if (Hero.mWeapon->isA (kObjTheOrgasmatron)) {
                true_orgasmatron = 1;
            }
            I->p ("%s %s decompiled.", YOUR (Hero.mWeapon),
                Hero.mWeapon->mCount > 1 ? "are" : "is");
            weight += Hero.mWeapon->myIlk ()->mWeight * Hero.mWeapon->mCount;
            Hero.useUpSomeObjectsFromInventory (Hero.mWeapon, Hero.mWeapon->mCount);
        }
    } /* Decompile web hero might be entangled in. */
    if (Hero.mTrapped.mWebbed) {
        I->p ("The web holding you is decompiled!");
        weight += 50 * Hero.mTrapped.mWebbed;
        Hero.mTrapped.mWebbed = 0;
    } /* Decompile duct tape hero might be held by. */
    if (Hero.mTrapped.mWebbed) {
        I->p ("The duct tape holding you is decompiled!");
        weight += 5 * Hero.mTrapped.mTaped;
        Hero.mTrapped.mTaped = 0;
    } /* Decompile sewage. */
    if (Level->mSquares[Hero.mX][Hero.mY].mTerr == kSewage) {
        I->p ("The sewage around you is decompiled!");
        Level->mSquares[Hero.mX][Hero.mY].mTerr = kSewerFloor;
        /* Hero might be drowning, but this is handled by decompileFeature () */
        weight += 250;
    } /* Decompile objects too. */
    shObjectVector *v = Level->getObjects (Hero.mX, Hero.mY);
    if (v) { /* Nuke whole stack of items. */
        if (v->count () > 1) {
            I->p ("Items on ground are decompiled.");
        } else {
            I->p ("Something on ground is decompiled.");
        }
        for (int i = 0; i < v->count (); ++i) {
            shObject *obj = v->get (i);
            weight += obj->myIlk ()->mWeight;
            if (obj->isA (kObjTheOrgasmatron)) {
                true_orgasmatron = 1;
            }
            if (obj->isUnpaid ()) {
                Hero.usedUpItem (obj, obj->mCount, "decompile");
            }
            delete obj;
        }
        delete v;
        Level->setObjects (Hero.mX, Hero.mY, NULL);
    } /* Perhaps decompile some terrain feature? */
    if (Level->getFeature (Hero.mX, Hero.mY)) {
        weight += decompileFeature ();
        if (Hero.mState == kDead)  return kNoExpiry;
    } /* Unavoidable weight loss. Some particles escape. */
    if        (disk->isInfected ()) {
        weight = weight * 75 / 100;
    } else if (disk->isDebugged ()) {
        weight = weight * 90 / 100;
    } else if (disk->isOptimized ()) {
        weight = weight * 95 / 100;
    } /* (De)compilation should not be attempted in irradiated level/room. */
    if (Level->isRadioactive (Hero.mX, Hero.mY)) {
        weight /= 2;
        I->p ("Particles behave in unstable way.  Half of them speed away.");
        I->pause ();
    }
    int objs_created = 0;
    int feature_created = 0;
    int last_ok = 1;
    if (weight) { /* Create stuff! */
        int numitems = RNG (1, 3 + disk->mBugginess);
        while (numitems-- and weight and last_ok) {
            const char *ilkstr = NULL;
            shFeature::Type kind = NOFEATURE;
            if (true_orgasmatron) { /* Give it back. */
                shObject *orgasmatron = createObject ("Bizarro Orgasmatron", 0);
                Level->putObject (orgasmatron, Hero.mX, Hero.mY);
                true_orgasmatron = 0;
                continue;
            }
            if (hackingRoll (computer) >= 20) {
                ilkstr = pickCompilerTarget (hackingRoll (computer), &kind);
            } else {
                if (weight >= 50000 and !RNG (20)) {
                    switch (RNG (2)) {
                        case 0: kind = shFeature::kRadTrap; break;
                        case 1: kind = shFeature::kVat; break;
                    }
                }
            }
            if (kind == NOFEATURE) { /* Generate an object. */
                last_ok = compileObject (ilkstr, hackingRoll (computer), &weight);
                if (!last_ok) break; /* Out of particles or an error. */
                objs_created += last_ok;
            } else {
                last_ok = compileFeature (kind, hackingRoll (computer), &weight);
                feature_created = last_ok;
            }
        }
    } /* Report results. */
    if (feature_created) {
        shFeature *f = Level->getFeature (Hero.mX, Hero.mY);
        f->mTrapUnknown = 0;
        I->p ("%s forms.", f->getShortDescription ());
    }
    if (objs_created > 1) {
        I->p ("Some items form at your feet.");
    } else if (objs_created == 1) {
        I->p ("An item forms at your feet.");
    }
    if (!objs_created and !feature_created) {
        I->p ("Matter compilation failed entirely.");
    } /* If a lot of weight is left do not eat it all. */
    shObjectIlk *junk = &AllIlks[kObjJunk];
    shFeature *f = Level->getFeature (Hero.mX, Hero.mY);
    if (f) { /* What cannot be transformed into junk may fill pit/hole. */
        int unglued = weight % junk->mWeight;
        switch (f->mType) {
            case shFeature::kHole:
                if (unglued >= 20000) {
                    weight -= 20000;
                    I->p ("The hole below you is sealed.");
                    Level->removeFeature (f);
                } else if (unglued >= 10000) {
                    I->p ("The hole below you is partially filled.");
                    f->mType = shFeature::kPit;
                }
                break;
            case shFeature::kPit:
                if (unglued >= 10000) {
                    weight -= 10000;
                    Level->removeFeature (f);
                    if (Hero.mZ == -1) {
                        I->p ("You are lifted as the pit you were in is sealed with matter.");
                        Hero.mZ = 0;
                        Hero.mTrapped.mInPit = 0;
                        Hero.mTrapped.mDrowning = 0;
                    } else {
                        I->p ("The pit below you is filled.");
                    }
                }
                break;
            default:
                break;
        }
    }
    if (weight > junk->mWeight) {
        I->p ("Excess matter is haphazardly glued into piles of junk.");
        for (int i = 0; i < weight / junk->mWeight; ++i) {
            shObject *obj = createObject ("heap of space junk", 0);
            Level->putObject (obj, Hero.mX, Hero.mY);
        }
    } else if (weight > 0) {
        I->p ("Leftover particles disperse.");
    }
    return kNormal;
}
#undef NOFEATURE
