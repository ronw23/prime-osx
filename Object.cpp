#include <ctype.h>
#include <math.h>
#include "Global.h"
#include "Object.h"
#include "Interface.h"
#include "ObjectType.h"
#include "Creature.h"
#include "Hero.h"

/* Definitions of all game items. */
#include "ObjectIlk.cpp"

/* Put generated FlavorData table here. */
#include "Flavor.cpp"

/* Purpose of the nothing object is to mark choice of '-' when
   wielding weapons for example.  It must be distinct from NULL
   because NULL means not making a choice at all.
   The value of 0x22 is most probably arbitrary. -- MB */
shObject *TheNothingObject = (shObject *) 0x22;

static const shColor floppyColors[] = {kRed, kGreen, kBrown, kMagenta,
    kCyan, kGray, kOrange, kLime, kYellow, kNavy,
    kPink, kAqua, kWhite};
static const int numFloppyColors = sizeof (floppyColors) / sizeof (shColor);

const char *matname[kLastMaterial] = {
    "fleshy", "paper", "cloth", "plastic", "leather", "wood", "bone",
    "iron", "brass", "tin", "bronze", "lead", "steel", "aluminum", "asbestos",
    "titanium", "kevlar", "carbon-fiber", "plasteel", "adamantium",
    "depleted uranium", "copper", "silver", "gold", "platinum", "sillicon",
    "glass", "crystal"
};

void
randomizeIlkNames ()
{
    static const int shuftable[][2] =
    {
     {kFFirstBelt, kFLastBelt},
     {kFFirstCan, kFLastCan},
     {kFFirstDisk, kFLastDisk},
     /*{kFFirstGrenade, kFLastGrenade}, <-- pointess*/
     {kFFirstGizmo, kFLastGizmo},
     {kFFirstImplant, kFLastImplant},
     {kFFirstJumpsuit, kFLastJumpsuit},
     {kFFirstKey, kFLastKey},
     {kFFirstRayGun, kFLastRayGun}
    };
    const int nshuf = sizeof (shuftable) / sizeof (int[2]);

    for (int i = 0; i < nshuf; ++i)
        shuffle (Flavors + shuftable[i][0],
                 shuftable[i][1] - shuftable[i][0] + 1,
                 sizeof (shFlavor));

    /* Generate and assign grenade names.  Prepare short names
       consisting only from third part. */

    /* Some data to cook names from.  Stylized after Xenocide. */
    static const char *adjs[] = {"heavy", "little", "shiny", "small",
        "strange", "tiny", "polished", "rugged", "smooth", "rigid"};
    const int nadjs = sizeof (adjs) / sizeof (char *);
    static const char *mats[] = {"aluminum", "carbon-fiber", "electronic",
        "lead", "metal", "plastic", "alien-alloy", "plasteel", "sillicon",
        "steel", "titanium"};
    const int nmats = sizeof (mats) / sizeof (char *);
    /* Material also dicates color. */
    static const shColor matcols[nmats] = {kCyan, kNavy, kGreen, kGray,
        kCyan, kWhite, kLime, kMagenta, kGray, kAqua, kWhite};
    /* Also weight adjustment.  Not implemented until we do the necessary
       reseach how exactly each material should modify weight so that chosen
       numbers are at least remotely believeable. */
    static const char *shapes[] = {"ball", "box", "rounded box", "cube",
        "cuboid", "rounded cuboid", "cylinder", "device", "disc", "oval disc",
        "egg", "stick", "tetrahedron", "torus"};
    const int nshapes = sizeof (shapes) / sizeof (char *);
    /* Shapes will determine tile in the future. */
    const int tier2 = nadjs;
    const int tier3 = tier2 * nmats;

    /* Generate grenade flavor names by permuting choices. */
    const int numgren = kFLastGrenade - kFFirstGrenade + 1;
    int choices[numgren];
    for (int i = 0; i < numgren; ++i) {
        bool ok;
        int n1, n2, n3;
        do { /* Make sure names do not repeat. */
            n1 = RNG (nadjs);
            n2 = RNG (nmats);
            n3 = RNG (nshapes);
            choices[i] = n1 + tier2 * n2 + tier3 * n3;
            ok = true;
            /* Check if new choice matches with any earlier. */
            for (int j = i-1; j >= 0; --j) {
                if (choices[j] == choices[i]) { /* It does!  Retry. */
                    ok = false;
                    break;
                }
            }
        } while (!ok);
        int k = i + kFFirstGrenade; /* Index into flavor table. */
        snprintf (Flavors[k].mAppearance.mName, 40, "%s %s %s",
                  adjs[n1], mats[n2], shapes[n3]);
        Flavors[k].mAppearance.mGlyph.mColor = matcols[n2];
        snprintf (Flavors[k].mVague.mName, 40, "%s", shapes[n3]);
    }
}

shTileRow
wpnRow (shSkillCode skill)
{
    switch (skill) {
    case kHandgun:       return kRowHandgun;
    case kLightGun:      return kRowLightGun;
    case kHeavyGun:      return kRowHeavyGun;
    case kMeleeWeapon:   return kRowMelee;
    case kSword:         return kRowSword;
    case kUnarmedCombat: return kRowUnarmed;
    case kGrenade:       return kRowMissile;
    default:             return kRowDefault;
    }
}

shTileRow
mslRow (shObjId parent)
{
    if (parent == kObjGenericGrenade) return kRowGrenade;
    return kRowMissile;
}

shTileRow
armRow (shObjId parent)
{
    while (parent != kObjNothing and AllIlks[parent].mParent != kObjAnyArmor) {
        parent = AllIlks[parent].mParent;
    }
    switch (parent) {
    case kObjNothing:         return kRowBodyArmor;
    case kObjGenericHelmet:   return kRowHelmet;
    case kObjGenericGoggles:  return kRowGoggles;
    case kObjGenericBelt:     return kRowBelt;
    case kObjGenericBoots:    return kRowBoots;
    case kObjGenericCloak:    return kRowCloak;
    case kObjGenericJumpsuit: return kRowJumpsuit;
    case kObjGenericArmor:    return kRowBodyArmor;
    default:                  return kRowDefault;
    }
}

shTileRow
toolRow (shObjId parent)
{
    while (parent != kObjNothing and AllIlks[parent].mParent != kObjGenericTool)
    {
        parent = AllIlks[parent].mParent;
    }
    switch (parent) {
    case kObjGenericComputer: return kRowComputer;
    case kObjGenericKeycard:  return kRowKeycard;
    case kObjGenericGizmo:    return kRowGizmo;
    case kObjGenericPlant:    return kRowPowerPlant;
    default:                  return kRowGenTool;
    }
}

shEnergyType
shObject::vulnerability ()
{
    if (isA (kEnergyCell) or (isA (kImplant) and mIlkId != kObjTorc)) {
        return kElectrical;
    } else if (isA (kCanister)) {
        if (isA (kObjLNO)) return kNoEnergy; /* Already frozen. */
        return kFreezing;
    }
    switch (myIlk ()->mMaterial) {
    case kFleshy:
    case kPaper:
    case kCloth:
    case kLeather:
    case kWood:
    case kPlastic:
    case kBone:
        return kBurning;

    case kIron:
    case kBrass:
    case kTin:
    case kBronze:
    case kLead:
    case kSteel:
    case kAluminum:
    case kCopper:
    case kSilver:
        return kCorrosive;

    case kSilicon:
    case kGlass:
    case kCrystal:
        return kElectrical;

    case kTitanium:
    case kGold:
    case kPlatinum:
    case kCarbonFiber:
    case kPlasteel:
    case kAdamantium:
    case kAsbestos:
    case kKevlar:
    case kDepletedUranium:
    case kLastMaterial:
        return kNoEnergy;
    }
    return kNoEnergy;
}

void initializeObjCounts (); /* Defined later in this file. */

void
initializeObjects ()
{
    initializeTools ();
    initializeWeapons ();
    initializeRayGuns ();
    initializeCanisters ();
    initializeFloppyDisks ();
    initializeImplants ();
    initializeLore ();
    initializeObjCounts ();
    initializeEquipment ();
    for (int i = 0; i < kObjNumIlks; ++i) {
        AllIlks[i].mId = shObjId (i);

        /* Use flavor if applicable. */
        if (AllIlks[i].mFlavor) {
            shFlavorId fid = AllIlks[i].mFlavor;
            /* Overrides name? */
            if (Flavors[fid].mAppearance.mName[0] != 0) {
                AllIlks[i].mAppearance.mName = Flavors[fid].mAppearance.mName;
                /* Where main name is empty override that. */
                if (!AllIlks[i].mReal.mName)
                    AllIlks[i].mReal.mName = Flavors[fid].mAppearance.mName;
            }
            if (Flavors[fid].mVague.mName[0] != 0)
                AllIlks[i].mVague.mName = Flavors[fid].mVague.mName;
            /* Color?  Symbol? */
            if (Flavors[fid].mAppearance.mGlyph.mColor) {
                AllIlks[i].mAppearance.mGlyph.mColor = Flavors[fid].mAppearance.mGlyph.mColor;
                /* Real color is not defined override that too. */
                if (!AllIlks[i].mReal.mGlyph.mColor)
                    AllIlks[i].mReal.mGlyph.mColor = Flavors[fid].mAppearance.mGlyph.mColor;
            }
            if (Flavors[fid].mVague.mGlyph.mColor)
                AllIlks[i].mVague.mGlyph.mColor = Flavors[fid].mVague.mGlyph.mColor;
            if (Flavors[fid].mAppearance.mGlyph.mSym != '@')
                AllIlks[i].mAppearance.mGlyph.mSym = Flavors[fid].mAppearance.mGlyph.mSym;
            if (Flavors[fid].mVague.mGlyph.mSym != '@')
                AllIlks[i].mVague.mGlyph.mSym = Flavors[fid].mVague.mGlyph.mSym;
            /* Tile? */
            if (Flavors[fid].mAppearance.mGlyph.mTileX != -1)
                AllIlks[i].mAppearance.mGlyph.mTileX = Flavors[fid].mAppearance.mGlyph.mTileX;
            if (Flavors[fid].mVague.mGlyph.mTileX != -1)
                AllIlks[i].mVague.mGlyph.mTileX = Flavors[fid].mVague.mGlyph.mTileX;
            if (Flavors[fid].mAppearance.mGlyph.mTileY != -1)
                AllIlks[i].mAppearance.mGlyph.mTileY = Flavors[fid].mAppearance.mGlyph.mTileY;
            if (Flavors[fid].mVague.mGlyph.mTileY != -1)
                AllIlks[i].mVague.mGlyph.mTileY = Flavors[fid].mVague.mGlyph.mTileY;
            /* Modifies material, price or weight? */
            if (Flavors[fid].mMaterial != kPaper)
                AllIlks[i].mMaterial = Flavors[fid].mMaterial;
            AllIlks[i].mCost += Flavors[fid].mCostMod;
            AllIlks[i].mWeight += Flavors[fid].mWeightMod;
        }

        /* If appearance description data is unspecified, it is assumed
           to be identical to real description.  Likewise, if vague
           description data is unspecified, it is assumed to be identical
           to appearance description data. */
        if (!AllIlks[i].mAppearance.mName)
            AllIlks[i].mAppearance.mName = AllIlks[i].mReal.mName;
        if (!AllIlks[i].mVague.mName)
            AllIlks[i].mVague.mName = AllIlks[i].mAppearance.mName;

        if (!AllIlks[i].mAppearance.mType)
            AllIlks[i].mAppearance.mType = AllIlks[i].mReal.mType;
        if (!AllIlks[i].mVague.mType)
            AllIlks[i].mVague.mType = AllIlks[i].mAppearance.mType;

        if (AllIlks[i].mAppearance.mGlyph.mSym == '@')
            AllIlks[i].mAppearance.mGlyph.mSym = AllIlks[i].mReal.mGlyph.mSym;
        if (AllIlks[i].mVague.mGlyph.mSym == '@')
            AllIlks[i].mVague.mGlyph.mSym = AllIlks[i].mAppearance.mGlyph.mSym;

        if (!AllIlks[i].mAppearance.mGlyph.mColor)
            AllIlks[i].mAppearance.mGlyph.mColor = AllIlks[i].mReal.mGlyph.mColor;
        if (!AllIlks[i].mVague.mGlyph.mColor)
            AllIlks[i].mVague.mGlyph.mColor = AllIlks[i].mAppearance.mGlyph.mColor;

        if (AllIlks[i].mAppearance.mGlyph.mTileY == -1)
            AllIlks[i].mAppearance.mGlyph.mTileY = AllIlks[i].mReal.mGlyph.mTileY;
        if (AllIlks[i].mVague.mGlyph.mTileY == -1)
            AllIlks[i].mVague.mGlyph.mTileY = AllIlks[i].mAppearance.mGlyph.mTileY;
        if (AllIlks[i].mAppearance.mGlyph.mTileX == -1)
            AllIlks[i].mAppearance.mGlyph.mTileX = AllIlks[i].mReal.mGlyph.mTileX;
        if (AllIlks[i].mVague.mGlyph.mTileX == -1)
            AllIlks[i].mVague.mGlyph.mTileX = AllIlks[i].mAppearance.mGlyph.mTileX;
        /* Help detect unlinked tiles.
           Anything undefined should point to `this is not a tile' tile. */
        if (AllIlks[i].mReal.mGlyph.mTileX == -1) {
            AllIlks[i].mReal.mGlyph.mTileX = 5;
            AllIlks[i].mReal.mGlyph.mTileY = kRowDefault;
        }
        if (AllIlks[i].mAppearance.mGlyph.mTileX == -1) {
            AllIlks[i].mAppearance.mGlyph.mTileX = 5;
            AllIlks[i].mAppearance.mGlyph.mTileY = kRowDefault;
        }
        if (AllIlks[i].mVague.mGlyph.mTileX == -1) {
            AllIlks[i].mVague.mGlyph.mTileX = 5;
            AllIlks[i].mVague.mGlyph.mTileY = kRowDefault;
        }
    }
    initializeParser ();
}

bool
shObjectIlk::isA (shObjId ilk)
{
    shObjId testilk = mId;
    while (kObjNothing != testilk) {
        if (testilk == ilk)  return true;
        testilk = AllIlks[testilk].mParent;
    }
    return false;
}

bool
shObjectIlk::isAbstract (void)
{
    return mProbability == ABSTRACT;
}

/* Two radioactivity functions.  They should be updated together. */
bool
shObject::isRadioactive (void)
{
    return getIlkFlag (kRadioactive) or
        (isA (kObjXRayGoggles) and isBuggy ()) or
        (isA (kObjMutagenCanister) and !isOptimized () and
            myIlk ()->mMaterial != kLead);
}

bool
shObject::isKnownRadioactive (void)
{   /* It is radioactive and you know it. */
    return (isRadioactive () and isIlkKnown () and
            (!isA (kObjXRayGoggles) or isBugginessKnown ())) or
    /* It is likely to be radioactive and you suspect it. */
            (isA (kObjMutagenCanister) and isKnown () and
             (!isBugginessKnown () or !isOptimized ()) and
             (!isAppearanceKnown () or myIlk ()->mMaterial != kLead));
}

bool
shObject::isWeldedWeapon (void)
{
    return isWielded () and isBuggy () and
        (isMeleeWeapon () or isAimedWeapon ()) and
        (!mOwner or !mOwner->isOrc ());
}

/* TODO: This is getting complicated and has potential for more.
   Object ilks could have flags telling when apply command is available. */
int
shObject::isUseable (void)
{
    return myIlk ()->mUseFunc and
        ((isA (kTool) or isA (kCanister) or isA (kObjLightSaber)) or
         (isA (kObjGenericGrenade) and !isKnown ()) or
         (isA (kArmor) and isKnown () and isWorn ()) or
         (!isA (kImplant) and isKnown ()) or
         (isA (kImplant) and isKnown () and
          (isWorn () or isA (kObjMechaDendrites))));
}

void
shObject::announce (void)
{
    I->p ("%c - %s", mLetter, inv ());
}

shGlyph
shObject::getGlyph ()
{ /* Put specific items first as to not allow categories to override them. */
    shGlyph g;
    if (isA (kObjTorc)) {
        g = myIlk ()->mAppearance.mGlyph;
        if (isAppearanceKnown ()) {
            switch (mBugginess) {
            case -1: g.mColor = kGray; break;
            case  0: g.mColor = kWhite; break;
            case +1: g.mColor = kYellow; break;
            }
            g.mTileX += mBugginess;
        }
    } else if (isA (kObjLightSaber)) {
        if (isAppearanceKnown () and isKnown ()) {
            g = myIlk ()->mReal.mGlyph;
            g.mTileX += 7 + mColor;
            if      (mColor == 0) g.mColor = kOrange;
            else if (mColor == 1) g.mColor = kLime;
            else if (mColor == 2) g.mColor = kNavy;
        } else if (isKnown ()) {
            g = myIlk ()->mReal.mGlyph;
        } else {
            g = myIlk ()->mAppearance.mGlyph;
        }
    } else if (isA (kFloppyDisk)) {
        g = myIlk ()->mVague.mGlyph;
        if (isAppearanceKnown ()) {
            g.mColor = floppyColors[mColor];
            g.mTileX = 3+mColor;
        } else {
            g.mColor = kGray;
        }
    } else if (myIlk ()->mReal.mGlyph.mTileX == kRowTag1) {
        if (isAppearanceKnown ()) {
            g = myIlk ()->mAppearance.mGlyph;
        } else {
            g = myIlk ()->mVague.mGlyph;
        }
    } else {
        g = apparent ()->mGlyph;
    }
    return g;
}


void
itemSpoilers ()
{
    FILE *spoilerfile = fopen ("items.txt", "w");

    for (int i = 0; i < 7; ++i) {
        int first, last;
        unsigned int total = 0;
        double prob;
        switch (i) {
            case 0:
                first = kObjFirstWeapon;
                last = kObjLastWeapon;
                break;
            case 1:
                first = kObjFirstArmor;
                last = kObjLastArmor;
                break;
            case 2:
                first = kObjFirstCanister;
                last = kObjLastCanister;
                break;
            case 3:
                first = kObjFirstFloppyDisk;
                last = kObjLastFloppyDisk;
                break;
            case 4:
                first = kObjFirstTool;
                last = kObjLastTool;
                break;
            case 5:
                first = kObjFirstRayGun;
                last = kObjLastRayGun;
                break;
            case 6:
                first = kObjFirstImplant;
                last = kObjLastImplant;
                break;
            default:
                first = kObjWreck;
                last = kObjWreck;
                break;
        }
        for (int k = first; k <= last; ++k) {
            total += AllIlks[k].mProbability;
        }
        shObjectType type = AllIlks[first].mReal.mType;
        fprintf (spoilerfile, "\n%s:\n", objectTypeHeader[type]);
        for (int k = first; k <= last; ++k) {
            shObjectIlk *ilk = &AllIlks[k];
            prob = ilk->mProbability;
            if (prob > 0) prob = prob / total * 100;
            fprintf (spoilerfile, "%8.4f - %s\n", prob, ilk->mReal.mName);
        }
    }

    fprintf (spoilerfile, "\n\n\n");
    int collision[40][120];
    memset (collision, 0, 40 * 120 * sizeof (int));
    for (int i = 0; i < kObjNumIlks; ++i) {
        shGlyph *gr = &AllIlks[i].mReal.mGlyph;
        shGlyph *ga = &AllIlks[i].mAppearance.mGlyph;
        shGlyph *gv = &AllIlks[i].mVague.mGlyph;
        ++collision[gr->mTileX][gr->mTileY];
        if (ga->mTileX != gr->mTileX or ga->mTileY != gr->mTileY)
            ++collision[ga->mTileX][ga->mTileY];
        if ((gv->mTileX != gr->mTileX or gv->mTileY != gr->mTileY) and
            (gv->mTileX != ga->mTileX or gv->mTileY != ga->mTileY))
            ++collision[gv->mTileX][gv->mTileY];
    }
    for (int y = 0; y < 120; ++y)
        for (int x = 0; x < 40; ++x) {
            if (collision[x][y] > 1 and x > 0) {
                fprintf (spoilerfile, "Tile (%d,%d) referenced %d times.\n",
                         x, y, collision[x][y]);
                for (int i = 0; i < kObjNumIlks; ++i) {
                    shGlyph *gr = &AllIlks[i].mReal.mGlyph;
                    shGlyph *ga = &AllIlks[i].mAppearance.mGlyph;
                    shGlyph *gv = &AllIlks[i].mVague.mGlyph;
                    if (gr->mTileX == x and gr->mTileY == y)
                        fprintf (spoilerfile, "real : %s\n", AllIlks[i].mReal.mName);
                    if (ga->mTileX == x and ga->mTileY == y)
                        fprintf (spoilerfile, "apprc: %s\n", AllIlks[i].mReal.mName);
                    if (gv->mTileX == x and gv->mTileY == y)
                        fprintf (spoilerfile, "vague: %s\n", AllIlks[i].mReal.mName);
                }
            }
        }

    fclose (spoilerfile);
    I->p ("Tables saved in items.txt.");
}


void
shObject::name (const char *newname)
{
    if (!newname) {
        char prompt[80];
        char buf[80];

        snprintf (prompt, 80, "%same %s:", mUserName ? "Ren" : "N", these ());
        if (!I->getStr (buf, 40, prompt)) {
            I->p ("Never mind.");
            return;
        }
        newname = buf;
    }
    if (mUserName) {
      free ((void *) mUserName);
    }
    if (1 == strlen (newname) and ' ' == *newname) {
        mUserName = NULL;
    } else {
        mUserName = strdup (newname);
    }
}


void
shObject::nameIlk ()
{
    if (!isAppearanceKnown ()) {
        I->p ("You would never recognize another one.");
        return;
    }

    const char *desc;
    {   /* Retrieve description without previously given user name (if any). */
        const char *tmp = myIlk ()->mUserName;
        myIlk ()->mUserName = NULL;
        desc = getDescription ();
        myIlk ()->mUserName = tmp;
    }

    char prompt[80], buf[80];

    snprintf (prompt, 80, "Call %s:", desc);
    if (!I->getStr (buf, 40, prompt)) {
        I->p ("Never mind.");
        return;
    }
    if (myIlk ()->mUserName) {
      free ((void *) myIlk ()->mUserName);
      myIlk ()->mUserName = NULL;
    }
    if (1 == strlen (buf) and ' ' == *buf) {
        return;
    } else {
        myIlk ()->mUserName = strdup (buf);
    }
}


void
shObject::maybeName (void)
{
    if (!isIlkKnown () and !myIlk ()->mUserName and isAppearanceKnown ())
        nameIlk ();
}


const char *
shObject::getVagueDescription ()
{
    return myIlk ()->mVague.mName;
}


/* Descriptive function block begins. */

const char *
shObject::getShortDescription (int cnt)
{
    if (cnt == -1)  cnt = mCount;
    int l = 0;
    char *buf = GetBuf ();
    const char *name = apparent ()->mName;

    if (isA (kObjLightSaber) and isAppearanceKnown () and isKnown ()) {
        const char *color;
        switch (mColor) {
        case 0: color = "red"; break;
        case 1: color = "green"; break;
        case 2: color = "blue"; break;
        default: color = "funny"; break;
        }
        l += snprintf (buf + l, SHBUFLEN - l, "%s ", color);
    }
    l += snprintf (buf + l, SHBUFLEN - l, "%s", name);
    if (cnt > 1) {
        makePlural (buf, SHBUFLEN);
        l = strlen (buf); /* Pluralisation alters length. */
    }
    if (myIlk ()->mUserName) {
        l += snprintf (buf + l, SHBUFLEN - l, " called %s", myIlk ()->mUserName);
    }
    return buf;
}


const char *
shObject::getDescription (int cnt)
{
    if (cnt == -1)  cnt = mCount;
    int l = 0;
    const char *fooproof = NULL;
    const char *dmg;
    char *buf = GetBuf ();

    if (isBugginessKnown () and !isBugProof ()) {
        if (mIlkId != kObjTorc) {
            l += snprintf (buf + l, SHBUFLEN - l, "%s ",
                           isBuggy () ? "buggy" :
                           isOptimized () ? "optimized" : "debugged");
        } else {
            l += snprintf (buf + l, SHBUFLEN - l, "%s ",
                           isBuggy () ? "gray" :
                           isOptimized () ? "golden" : "silver");
        }
    }

    switch (vulnerability ()) {
    case kCorrosive: fooproof = "acidproof"; dmg = "corroded"; break;
    case kBurning: fooproof = "fireproof"; dmg = "burnt"; break;
    default: dmg = "damaged";
    }
    if (isA (kObjGenericComputer))  fooproof = "protected";

    switch (mDamage) {
    case 1:
        l += snprintf (buf + l, SHBUFLEN - l, "%s ", dmg); break;
    case 2:
        l += snprintf (buf + l, SHBUFLEN - l, "very %s ", dmg); break;
    case 3:
        l += snprintf (buf + l, SHBUFLEN - l, "extremely %s ", dmg); break;
    default: break;
    }

    if (isFooproof () and isFooproofKnown ()) {
        l += snprintf (buf + l, SHBUFLEN - l, "%s ", fooproof);
    }

    if (isInfectable () and isInfectedKnown () and
        /* Protected and clean together are redundant. */
        (!isFooproof () or !isFooproofKnown ()))
    {
        l += snprintf (buf + l, SHBUFLEN - l,
                       isInfected () ? "infected " : "clean ");
    }

    if (isCracked () and isCrackedKnown ()) {
        l += snprintf (buf + l, SHBUFLEN - l, "cracked ");
    }

    if (isEnhanceable () and isEnhancementKnown ()) {
        l += snprintf (buf + l, SHBUFLEN - l, "%s%d ",
                       mEnhancement < 0 ? "" : "+", mEnhancement);
    }

    if (kObjWreck == mIlkId) {
        l += snprintf (buf + l, SHBUFLEN - l, "disabled %s", MonIlks[mWreckIlk].mName);
        if (cnt > 1) {
            makePlural (buf, SHBUFLEN);
            l = strlen (buf); /* Pluralisation alters length. */
        }
    } else {
        const char *shortd = getShortDescription ();
        l += snprintf (buf +l, SHBUFLEN - l, "%s", shortd);
    }
    if (mUserName) {
        l += snprintf (buf + l, SHBUFLEN - l, " named %s", mUserName);
    }

    if (isChargeable () and isChargeKnown () and !isA (kObjEmptyRayGun)) {
        l += snprintf (buf + l, SHBUFLEN - 1, " (%d charg%s)", mCharges,
                       mCharges == 1 ? "e" : "es");
    }
    buf[SHBUFLEN-1] = 0;
    return buf;
}


const char *
shObject::stackDesc (int cnt, const char *pronoun, const char *desc)
{
    if (cnt == -1)  cnt = mCount;
    char *buf = GetBuf ();
    if (cnt > 1)
        snprintf (buf, SHBUFLEN, "%s%d %s", pronoun, cnt, desc);
    else
        snprintf (buf, SHBUFLEN, "%s%s", pronoun, desc);
    return buf;
}

const char *
shObject::those (int cnt)
{
    const char *pronoun = "those ";
    if (cnt == 1 or (cnt == -1 and mCount == 1))
        pronoun = "that ";
    return stackDesc (cnt, pronoun, getDescription (cnt));
}

const char *
shObject::these (int cnt)
{
    const char *pronoun = "these ";
    if (cnt == 1 or (cnt == -1 and mCount == 1))
        pronoun = "this ";
    return stackDesc (cnt, pronoun, getDescription (cnt));
}

const char *
shObject::anStackDesc (int cnt, const char *desc)
{
    if (cnt == -1)  cnt = mCount;
    const char *pronoun =
        cnt > 1 ? "" :
        isUniqueKnown () ? "the " :
        isvowel (desc[0]) ? "an " : "a ";
    return stackDesc (cnt, pronoun, desc);
}

const char *shObject::an (int cnt) {
    return anStackDesc (cnt, getDescription (cnt));
}

const char *shObject::anQuick (int cnt) {
    return anStackDesc (cnt, getShortDescription (cnt));
}

const char *shObject::anVague (int cnt) {
    return anStackDesc (cnt, getVagueDescription ());
}

const char *shObject::the (int cnt) {
    return stackDesc (cnt, "the ", getDescription (cnt));
}

const char *shObject::theQuick (int cnt) {
    return stackDesc (cnt, "the ", getShortDescription (cnt));
}

const char *shObject::your (int cnt) {
    return stackDesc (cnt, "your ", getShortDescription (cnt));
}

const char *
shObject::her (shCreature *owner, int cnt)
{
    const char *pronoun = "its ";
    if (kFemale == owner->mGender)
        pronoun = "her ";
    else if (kMale == owner->mGender)
        pronoun = "his ";
    return stackDesc (cnt, pronoun, getDescription (cnt));
}

const char *
shObject::inv (void)
{
    char *buf = GetBuf ();
    int l;

    if (mCount > 1) {
        l = snprintf (buf, SHBUFLEN, "%d %s", mCount, getDescription ());
    } else {
        const char *tmp = getDescription ();
        l = snprintf (buf, SHBUFLEN, "%s %s",
                      isUniqueKnown () ? "the" :
                      isvowel (tmp[0]) ? "an" : "a", tmp);
    }
    if (isWorn ()) {
        if (isA (kImplant)) {
            l += snprintf (buf + l, SHBUFLEN - l, " (implanted in %s)",
                           describeImplantSite (mImplantSite));
        } else {
            l += snprintf (buf + l, SHBUFLEN - l, " (worn");
            if (isA (kObjBioMask) and isIlkKnown ()) {
                l += snprintf (buf + l, SHBUFLEN - l, ", %s",
                               isToggled () ? "EM field mode" : "thermal mode");
            }
            l += snprintf (buf + l, SHBUFLEN - l, ")");
        }
    } else if (isWielded ()) {
        l += snprintf (buf + l, SHBUFLEN - l, " (wielded");
        if (isSelectiveFireWeapon () and isIlkKnown ()) {
            l += snprintf (buf + l, SHBUFLEN - l, ", %s",
                           isToggled () ? "burst mode" : "single fire");
        }
        l += snprintf (buf + l, SHBUFLEN - l, ")");
    }
    if (isPrepared ()) {
        l += snprintf (buf + l, SHBUFLEN - l, " (prepared)");
    }
    if (isDesignated ()) {
        l += snprintf (buf + l, SHBUFLEN - l, " (designated)");
    }
    if (isActive ()) {
        l += snprintf (buf + l, SHBUFLEN - l, " (activated");
        if (isA (kObjGeigerCounter)) {
            if (Hero.getStoryFlag ("geiger counter message")) {
                l += snprintf (buf + l, SHBUFLEN - 1, ", clicking");
            }
        }
        l += snprintf (buf + l, SHBUFLEN - l, ")");
    }
    if (isUnpaid ()) {
        l += snprintf (buf + l, SHBUFLEN - l, " (unpaid, $%d)",
                       mCount * myIlk ()->mCost);
    } else if (Hero.isInShop () and Level->getShopKeeper (Hero.mX, Hero.mY) and
               !isA (kObjMoney))
    {
        l += snprintf (buf + l, SHBUFLEN - l, " (worth $%d)",
                       mCount * myIlk ()->mCost);
    }
/*  if (BOFH) {
        l += snprintf (buf + l, SHBUFLEN - l, " (%d g)", getMass ());
    }*/
    buf[SHBUFLEN-1] = 0;
    return buf;
}

/* Descriptive function block ends. */

int
shObject::isA (shObjId type)
{
    return myIlk ()->isA (type);
}


int
shObject::canMerge (shObject *obj)
{
    if (!obj) {
        obj = this;
    }
    if (obj->myIlk () != myIlk () or kObjWreck == mIlkId) {
        return 0;
    }
    if ((myIlk ()->mFlags & kMergeable) and
        (obj->mFlags == mFlags) and
        (obj->mBugginess == mBugginess) and
        (obj->mDamage == mDamage) and
        (!obj->mUserName and !mUserName))
    {
        return 1;
    }
    return 0;
}


static shObjectVector DeletedObjects;

void
shObject::merge (shObject *obj)
{
    if (obj == this) return;
    mCount += obj->mCount;
    /* HACK: don't delete the object right away; we might need to use
       it to print a message or something.  (For example, the case of
       a shopkeeper giving a quote about an object picked up in a
       shop.) */
    DeletedObjects.add (obj);
    //delete obj;
}


shObject *
shObject::split (int count)
{
    shObject *result;

    assert (count < mCount);

    mCount -= count;
    result = new shObject ();
    result->mIlkId = mIlkId;
    result->mCount = count;
    result->mHP = mHP;
    result->mBugginess = mBugginess;
    result->mFlags = mFlags;
    if (isA (kObjWreck)) {
        result->mWreckIlk = mWreckIlk;
    } else {
        result->mColor = mColor;
    }
    return result;
}


/* Returns number of objects destroyed from stack. */
int
shObject::sufferDamage (shAttack *attack, int x, int y,
    int multiplier, int specialonly)
{
    if (isIndestructible ())  return 0;

    for (int j = 0; j < 3; ++j) {
        shEnergyType energy = (shEnergyType) attack->mDamage[j].mEnergy;

        if (kNoEnergy == energy)
            break;

        /* If some type of energy deals damage consider it enough. */
        int result = sufferDamage (energy, attack, NULL, x, y);
        if (result)  return result;
    }

    return 0;
}

/* Returns number of objects destroyed from stack. */
int
shObject::sufferDamage (shEnergyType energy, shAttack *attack,
    shCreature *attacker, int x, int y)
{
    if (isIndestructible ())  return 0;
    /* Cannot get damaged by what you protect against. */
    if (myIlk ()->mResistances[energy] > 0)  return 0;
    bool annihilated = false;
    bool destroy = false;
    bool cansee = Hero.canSee (x, y);
    bool nearby = distance (&Hero, x, y) <= 30;
    const char *theobj;

    if (Hero.owns (this)) {
        cansee = 1;
        theobj = your ();
    } else if (mOwner) {
        theobj = her (mOwner);
    } else {
        theobj = theQuick ();
    }

    /* First consider only attack type. */
    if (energy == kNoEnergy) switch (attack->mType) {
    case shAttack::kHealingRay:
        if (isA (kObjBioArmor)) {
            if (!Hero.owns (this) and
                (Hero.isBlind () or x != Hero.mX or y != Hero.mY))
            {
                cansee = false;
            }
            int worked = 0;
            if (mEnhancement < 2) {
                ++mEnhancement;
                worked = 1;
            }
            debug.log ("healed %s %d", theobj, mEnhancement);
            if (cansee) {
                if (worked)
                    I->p ("%s feels tougher!", theobj);
                else
                    I->p ("%s seems to be enjoying something.", theobj);
            }
        }
        break;
    case shAttack::kPlague:
        if (isA (kObjBioArmor) and !isA (kObjMBioArmor1)) {
            int domsg = 1;
            int damage = 3;
            if (!Hero.owns (this)) {
                cansee = 0;
            }
            if (isOptimized () and RNG (4)) {
                if (cansee) {
                    if (mEnhancement > -3) {
                        I->p ("%s manages to resist somewhat.", theobj);
                    } else {
                        I->p ("%s tries to resist but fails.", theobj);
                    }
                }
                damage = 1;
                domsg = 0;
            }
            mEnhancement -= damage;
            debug.log ("plagued %s %d", theobj, mEnhancement);
            if (domsg) {
                if (mEnhancement <= -4) {
                    I->p ("%s gets horribly sick and dies!", theobj);
                } else {
                    I->p ("%s gets much weaker!", theobj);
                }
            }
            if (mEnhancement <= -4) return 1;
        }
        break;
    case shAttack::kRestorationRay:
        if (isA (kObjBioArmor)) {
            if (!Hero.owns (this) and
                (Hero.isBlind () or x != Hero.mX or y != Hero.mY))
            {
                cansee = 0;
            }
            int worked = 0;
            if (isBuggy ()) {
                setDebugged ();
                ++worked;
            }
            if (mEnhancement < 0) {
                mEnhancement = 0;
                ++worked;
            }
            if (mDamage) {
                mDamage = 0;
                ++worked;
            }
            debug.log ("restored %s %d", theobj, mEnhancement);
            if (cansee) {
                switch (worked) {
                case 0: I->p ("%s seems to be enjoying something.", theobj);
                    break;
                case 1: I->p ("%s feels better!", theobj); break;
                case 2: I->p ("%s feels much better!", theobj); break;
                case 3: I->p ("%s feels great!", theobj); break;
                }
            }
        }
        break;
    default:
        break;
    }
    if (energy == kNoEnergy)  return 0;

    /* Note: If energy effect causes something to destroy and this
       is accompanied by a special message, use return to skip default
       destruction messages. */
    switch (energy) {
    case kCorrosive:
        if (!Hero.owns (this))
            cansee = false; /* reduce messages */
        if (isA (kEnergyCell)) {
            if (cansee) {
                I->p ("%s dissolve%s!", theobj, mCount == 1 ? "s" : "");
            }
            return mCount;
        } else if (kCorrosive == vulnerability ()) {
            if (isFooproof ()) {
                if (cansee)  I->p ("%s is not affected.", theobj);
                return 0;
            }
            if (isOptimized () and RNG (4)) {
                if (cansee) {
                    setBugginessKnown ();
                    I->p ("Somehow, %s is not affected.", theobj);
                }
                return 0;
            }
            if (mDamage < 3) {
                ++mDamage;
            } else {
                return 0; /* Avoid message spam. */
            }
            debug.log ("corroded %s %d", theobj, mDamage);
            if (cansee) {
                switch (mDamage) {
                case 1:
                    I->p ("%s corrodes!", theobj); break;
                case 2:
                    I->p ("%s corrodes some more!", theobj); break;
                default:
                    I->p ("%s is thoroughly corroded!", theobj); break;
                }
            }
        } else if (cansee)  I->p ("%s is not affected.", theobj);
        break;
    case kElectrical:
        if (kElectrical != vulnerability ())  break;
        if (mOwner and isWorn ()) {
            if (Hero.owns (this)) {
                I->p ("%s is ejected from your %s in a shower of sparks!",
                      theobj, describeImplantSite (mImplantSite));
            } else if (cansee) {
                I->p ("%s is ejected from head of %s in a shower of sparks!",
                      theobj, THE (mOwner));
            }
            mOwner->doff (this);
            shCreature *owner = mOwner;
            mOwner->removeObjectFromInventory (this);
            Level->putObject (this, owner->mX, owner->mY);
            return 0;
        }
        break;
    case kBurning:
    case kPlasma:
        if (isA (kMoney)) {
            I->p ("%s melt%s!", theobj, mCount == 1 ? "s" : "");
            return mCount;
        } else if (isA (kFloppyDisk)) {
            int numdestroyed = RNG (4) ? 1 : 2;
            if (cansee) {
                if (1 == mCount) {
                    I->p ("%s melts!", theobj);
                } else if (1 == numdestroyed) {
                    I->p ("One of %s melts!", theobj);
                } else if (numdestroyed == mCount) {
                    I->p ("%s melt!", theobj);
                } else {
                    I->p ("Some of %s melt!", theobj);
                }
            } else if (nearby) { /* FIXME: No msg needed when underwater. */
                I->p ("You smell burning plastic.");
            }
            return numdestroyed;
        } else if (kBurning == vulnerability ()) {
            if (!Hero.owns (this))
                cansee = false; /* reduce messages */
            if (isFooproof ()) {
                if (cansee)  I->p ("%s is not affected.", theobj);
                return 0;
            }
            if (isOptimized () and RNG (4)) {
                if (cansee) {
                    setBugginessKnown ();
                    I->p ("Somehow, %s is not affected.", theobj);
                }
                return 0;
            }
            if (mDamage < 4)
                ++mDamage;
            debug.log ("burned %s %d", theobj, mDamage);
            if (cansee) {
                switch (mDamage) {
                case 1:
                    I->p ("%s burns!", theobj); break;
                case 2:
                    I->p ("%s burns some more!", theobj); break;
                case 3:
                    I->p ("%s is thoroughly burned!", theobj); break;
                default:
                    I->p ("%s burns completely!", theobj);
                }
            }
            if (mDamage == 4)  return 1;
        }
        break;
    case kFreezing:
        if (isA (kCanister) and kFreezing == vulnerability ()) {
            int numdestroyed = RNG (4) ? 1 : 2;
            if (cansee) {
                if (1 == mCount ) {
                    I->p ("%s freezes and shatters!", theobj);
                } else if (1 == numdestroyed) {
                    I->p ("One of %s freezes and shatters!", theobj);
                } else if (numdestroyed == mCount) {
                    I->p ("%s freeze and shatter!", theobj);
                } else {
                    I->p ("Some of %s freeze and shatter!", theobj);
                }
            } else if (nearby) { /* FIXME: No msg when in vacuum. */
                I->p ("You hear something shatter.");
            }
            return numdestroyed;
        }
        break;
    case kMagnetic:
    {
        int oldenh = mEnhancement;
        /* This one is powerful and more destructive. */
        if (attack->mType == shAttack::kGaussRay) {
            /* FIXME: clerkbot should get mad if any of this is hero's fault. */
            if (isA (kObjM56Smartgun) or isA (kObjM57Smartgun)) {
                mIlkId = kObjPulseRifle;  /* Erase auto-aim system. */
                if (mOwner == &Hero)  Hero.computeSkills ();
            }
            if (isA (kObjGenericComputer)) {
                mEnhancement = -5; /* Lost OS. */
                if (isDesignated ())  Hero.computeSkills ();
            }
            if (isA (kFloppyDisk)) { /* Yes, the whole stack. */
                mIlkId = kObjBlankDisk;
                resetCracked ();
                resetInfected ();
                setInfectedKnown ();
                setCrackedKnown ();
            }
            mBugginess = 0;
            mEnhancement = 0;
        }
        /* Introduce some variety in damage.  It would be boring if
           a hit either damaged no equipment or all for -1 enhancement.
           Similarly neutralizing bugginess for every item or none is
           undesirable. */
        mEnhancement = mEnhancement * RNG (2, 4) / 5;
        if (oldenh != mEnhancement and mOwner and isWorn ()) {
            if (mOwner == &Hero)
                I->p ("You feel performance of %s %screase.", your (),
                      oldenh > mEnhancement ? "de" : "in");
            mOwner->computeIntrinsics ();
        }
        if (!RNG (3))  setDebugged ();
        break;
    }
    case kBugging:
        if (isBugProof ())  break;
        if (isOptimized ()) {
            setDebugged ();
        } else {
            setBuggy ();
        }
        break;
    case kToxic:
        if (isA (kObjBioArmor) and !isA (kObjMBioArmor0)) {
            if (!Hero.owns (this) and
                (Hero.isBlind () or x != Hero.mX or y != Hero.mY))
            {
                cansee = false;
            }
            if (isOptimized () and RNG (4)) {
                if (cansee)  I->p ("Somehow, %s is not affected.", theobj);
                return 0;
            }
            --mEnhancement;
            debug.log ("poisoned %s %d", theobj, mEnhancement);
            if (cansee) {
                if (mEnhancement == -4) {
                    I->p ("%s quivers and dies!", theobj);
                } else {
                    I->p ("%s weakens!", theobj);
                }
            }
            if (mEnhancement == -4) return 1;
        } else if (isA (kObjGreenCPA)) {
            if (!Hero.owns (this))
                cansee = false;
            int worked = 0;
            if (mEnhancement < 3) {
                ++mEnhancement;
                worked = 1;
            }
            debug.log ("poisoned %s %d", theobj, mEnhancement);
            if (cansee and worked) {
                if (!Hero.isBlind ()) {
                    I->p ("%s glistens.", theobj);
                } else {
                    I->p ("%s throbs.", theobj);
                }
            }
        }
        break;
    case kDisintegrating:
        annihilated = true;
        break;
    case kConcussive:
        //if (myIlk ()->mMaterial == kLeather)  break;
        if (isComputer ()) {
            /* Computers generally do not like to be kicked around... */
            if (!RNG (10)) {
                if (cansee)  I->p ("Sparks fly from %s!", theobj);
                setBuggy ();
            }
            if (isA (kObjBioComputer) and !RNG (3) and hasSystem ()) {
                if (cansee)  I->p ("%s suffers brain damage.", theobj);
                --mEnhancement;
            }
            /* ... but this may also be beneficial. */
            kickComputer ();
        }
        destroy = myIlk ()->mMaterial == kGlass or isA (kFloppyDisk);
        break;
    default:
        break;
    }

    if (destroy) {
        if (cansee) {
            if (annihilated) {
                I->p ("%s is annihilated!", theobj);
            } else if (myIlk ()->mMaterial == kGlass) {
                I->p ("%s shatters!", theobj);
            } else {
                I->p ("%s is destroyed!", theobj);
            }
        } else if (nearby and myIlk ()->mMaterial == kGlass) {
            I->p ("You hear something shatter.");
        }
        return 1;
    }
    return 0;
}


int
shObject::getPsiModifier ()
{
    if (isA (kObjHarmonyDome) and !isBuggy ()) {
        /* Negate all psi aura disruption caused by cranial implants. */
        int psimod = 0;
        for (int i = 0; i <= shObjectIlk::kCerebellum; ++i) {
            if (mOwner->mImplants[i] and
                mOwner->mImplants[i]->myIlk ()->mPsiModifier < 0)
            {
                psimod -= mOwner->mImplants[i]->myIlk ()->mPsiModifier;
            }
        }
        return psimod;
    } else if (isA (kObjPsiAmp) or isA (kObjSBioArmor4)) {
        return mEnhancement + myIlk ()->mPsiModifier;
    } else {
        return myIlk ()->mPsiModifier;
    }
}


int
shObject::getArmorBonus ()
{
    if (kArmor != myIlk ()->mReal.mType) {
        return 0;
    } else {
        return maxi (0, myIlk ()->mArmorBonus - mDamage + mEnhancement);
    }
}


int
shObject::getThreatRange (shCreature *target)
{
    if (isA (kWeapon)) {
        return 20;
    } else { /* impossible to critical hit with improvised weapon */
        return 999;
    }
}

int
shObject::getCriticalMultiplier ()
{
    if (isA (kWeapon)) {
        return 2;
    } else { /* impossible to critical hit with improvised weapon */
        return 1;
    }
}


static struct shProbTotals
{
    int mArmSum;
    int mWpnSum;
    int mRaySum;
    int mCanSum;
    int mImpSum;
    int mDiskSum;
    int mToolSum;
} totals;


static shObject *
pickFromRange (shObjId from, shObjId to, int total)
{
    if (from > to) {
        return NULL;
    }

    int r = RNG (total);
    int n = 0;
    for (int i = from; i <= to; ++i) {
        int p = AllIlks[i].mProbability;
        if (p == ABSTRACT) continue;
        n += p;
        if (n >= r) {
            return new shObject ((shObjId) i);
        }
    }
    debug.log ("Failed pick an ilk! [%d,%d] (%d)", from, to, total);
    return NULL;
}


shObject *
createWeapon ()
{
    return pickFromRange (kObjFirstWeapon, kObjLastWeapon, totals.mWpnSum);
}

shObject *
createArmor ()
{
    return pickFromRange (kObjFirstArmor, kObjLastArmor, totals.mArmSum);
}

shObject *
createTool ()
{
    return pickFromRange (kObjFirstTool, kObjLastTool, totals.mToolSum);
}

shObject *
createCanister ()
{
    return pickFromRange (kObjFirstCanister, kObjLastCanister, totals.mCanSum);
}

shObject *
createFloppyDisk ()
{
    return pickFromRange (kObjFirstFloppyDisk, kObjLastFloppyDisk, totals.mDiskSum);
}

shObject *
createImplant ()
{
    return pickFromRange (kObjFirstImplant, kObjLastImplant, totals.mImpSum);
}

shObject *
createRayGun ()
{
    return pickFromRange (kObjFirstRayGun, kObjLastRayGun, totals.mRaySum);
}

/* generate random object according to level specification:
   -1 level indicates shop
*/
shObject *
generateObject (int level)
{   /* first what kind of object? */
    while (1) {
        int type = RNG (100);
        if (type < 20) {
            if (-1 != level) {
                /* 10d50 is largest amount possible. */
                shObject *cash = new shObject (kObjMoney);
                cash->mCount =
                    (NDX (mini (level, 10), 20 + 3 * mini (level, 10)));
                return cash;
            }
        } else if (type < 35) {
            return createWeapon ();
        } else if (type < 45) {
            return createArmor ();
        } else if (type < 48) {
            return new shObject (kObjEnergyCell);
        } else if (type < 60) {
            return createTool ();
        } else if (type < 75) {
            return createCanister ();
        } else if (type < 93) {
            return createFloppyDisk ();
        } else if (type < 98) {
            return createImplant ();
        } else if (type < 100) {
            return createRayGun ();
        } else if (-1 == level) {
            continue;
        } else {
            return new shObject (kObjMoney);
        }
    }
    return NULL; /* Not reached. */
}


static int
sumProb (shObjId from, shObjId to)
{
    int sum = 0;
    for (int i = from; i <= to; ++i)
        if (AllIlks[i].mProbability != ABSTRACT)
            sum += AllIlks[i].mProbability;
    return sum;
}


void
initializeObjCounts ()
{
    totals.mArmSum = sumProb (kObjFirstArmor, kObjLastArmor);
    totals.mWpnSum = sumProb (kObjFirstWeapon, kObjLastWeapon);
    totals.mRaySum = sumProb (kObjFirstRayGun, kObjLastRayGun);
    totals.mCanSum = sumProb (kObjFirstCanister, kObjLastCanister);
    totals.mImpSum = sumProb (kObjFirstImplant, kObjLastImplant);
    totals.mDiskSum = sumProb (kObjFirstFloppyDisk, kObjLastFloppyDisk);
    totals.mToolSum = sumProb (kObjFirstTool, kObjLastTool);
}


static int InventorySortOrder [kMaxObjectType] =
{
    13, 0, 5, 8, 6, 11, 3, 1, 2, 12, 7, -1
};

/* used for sorting
   returns: -1, 0 or 1 depending on whether obj1 belongs before, with, or
            after obj2 in an inventory list.  sorts on type
*/
int
compareObjects (shObject **obj1, shObject **obj2)
{
    int t1 = InventorySortOrder[(*obj1)->apparent ()->mType];
    int t2 = InventorySortOrder[(*obj2)->apparent ()->mType];
    if (t1 < t2) return -1;
    if (t1 > t2) return 1;
    return 0;
}


void
purgeDeletedObjects ()
{
    while (DeletedObjects.count ()) {
        delete DeletedObjects.removeByIndex(0);
    }
}

/* Returns object that should be shown at the top of the stack. */
shObject *   /* This means: find most interesting thing to player. */
findBestObject (shObjectVector *objs, bool markSeen)
{   /* Finds object with the lowest objecttype. */
    shObject *bestobj = NULL;
    int besttype = kMaxObjectType;

    for (int i = 0; i < objs->count (); ++i) {
        shObject *obj = objs->get (i);
        if (markSeen)  obj->setAppearanceKnown ();
        if (obj->apparent ()->mType < besttype) {
            besttype = obj->apparent ()->mType;
            bestobj = obj;
        }
    }
    return bestobj;
}

extern int getChargesForRayGun (shObjId raygun);

shObject::shObject (shObjId id)
{
    mIlkId = id;
    mFlags = 0;
    mLetter = 0;
    mUserName = NULL;
    mOwner = NULL;
    mLastEnergyBill = MAXTIME;

    /* Set count. */
    if (myIlk ()->mFlags & kMergeable) {
        /* Ammunition. Bullets, shells, slugs, fuel drums and like. */
        if (isA (kProjectile)) {
            int range = (int) (18.0 / sqrt ((double) myIlk ()->mWeight));
            mCount = RNG (range, range * 10);
        } /* Shurikens, daggers and grenades. */
        else if (myIlk ()->mFlags & kMissile) mCount = RNG (1, 4);
        else if (id == kObjEnergyCell) mCount = RNG (8, 80);
        /* Canisters can merge but are usually generated single. */
        else mCount = 1;
    } else { /* Guns, armor pieces, tools etc. */
        mCount = 1;
    }

    /* Set bugginess. */
    if (kBugProof & myIlk ()->mFlags) {
        mBugginess = 0;
        setBugginessKnown ();
    } else if (kUsuallyBuggy & myIlk ()->mFlags and RNG (9)) {
        mBugginess = -1;
    } else {
        int tmp;
        if (isA (kImplant) or isA (kFloppyDisk)) tmp = RNG (6);
        else tmp = RNG (8);
        mBugginess = (1 == tmp) ? 1 : (0 == tmp) ? -1 : 0;
    } /* Special cases for some items. */
    if (kObjSpamDisk == mIlkId and RNG (4)) {
        mBugginess = -1;
    } else if (kObjBugDetectionDisk == mIlkId and RNG (2)) {
        mBugginess = 1;
    }

    /* Set crackedness. */
    if (isA (kFloppyDisk)) {
        if (!RNG (100) and
            mIlkId != kObjHackingDisk and mIlkId != kObjMatterCompiler)
        {
            setCracked ();
        }
    } else if (isA (kObjGenericKeycard)) {
        if (!RNG (100) and !isA (kObjMasterKeycard)) setCracked ();
    }

    /* Set fooproof knowledge.  Currently only relevant for computers. */
    if (!isA (kObjGenericComputer))
        setFooproofKnown ();

    /* Set enhancement.  Requires bugginess determined beforehand. */
    if (!isEnhanceable ()) {
        mEnhancement = 0;
    } else if (isA (kArmor)) {
        int max = myIlk ()->mMaxEnhancement;
        if (isOptimized ()) {
            mEnhancement = (RNG (max) * RNG (max) + max - 1) / max;
        } else if (isBuggy ()) {
            mEnhancement = RNG (max) * RNG (max) / -max;
        } else {
            mEnhancement = RNG (6) ? RNG (max) * RNG (max) / max
                                   : RNG (max) * RNG (max) / -max;
        }
    } else if (isA (kWeapon)) {
        int tmp = RNG (20); /* -1 or +1 weapons are rare. */
        mEnhancement = (1 == tmp) ? 1 : (0 == tmp) ? -1 : 0;
    } else if (isA (kImplant)) {
        if (isOptimized ()) {
            mEnhancement = RNG (1, 5);
        } else if (isBuggy ()) {
            mEnhancement = -RNG (1, 5);
        } else {
            mEnhancement = RNG (6) ? RNG (0, 2) + RNG (0, 3) : -RNG (0, 5);
        }
    } else if (isA (kObjGenericComputer)) {
        if (!RNG (10)) {
            mEnhancement = -5; /* No OS! */
        } else if (!RNG (8)) {
            mEnhancement = RNG(-4, +4); /* Hardware independent OS. */
        } else switch (mBugginess) { /* Some OS. */
            case -1: mEnhancement = RNG(-4, -1); break;
            case  0: mEnhancement = RNG(-1, +1); break;
            case +1: mEnhancement = RNG(+1, +4); break;
        }
    } else { /* Other enhanceable items start out without it. */
        mEnhancement = 0;
    }

    /* Set charges. */
    if (mIlkId == kObjEmptyRayGun) {
        mCharges = 0;
    } else if (isA (kRayGun)) {
        mCharges = getChargesForRayGun (mIlkId);
    } else if (isA (kObjGenericPlant)) {
        mCharges = RNG (1, myIlk ()->mMaxCharges);
        setChargeKnown ();
    } else if (isA (kObjGenericEnergyTank)) {
        mCharges = RNG (0, myIlk ()->mMaxCharges / 5);
        setChargeKnown ();
    } else if (mIlkId == kObjKhaydarin) {
        mCharges = NDX (2, 2);
    } else if (mIlkId == kObjPlasmaCaster) {
        mCharges = !RNG (10) ? 1 : 0;
    } else {
        mCharges = RNG (0, myIlk ()->mMaxCharges);
    }

    /* Set damage or infectedness. */
    mDamage = 0; /* Currently no items start damaged. */
    if (mIlkId == kObjBlankDisk) {
        if (isIlkKnown ())  {
            setInfectedKnown ();
            setCrackedKnown ();
        }
    } else if (!isInfectable ()) {
        setInfectedKnown ();
    } else if (mIlkId == kObjComputerVirus or
               (mIlkId != kObjAntivirusDisk and
                mIlkId != kObjResidentAntivirusDisk and
                !RNG (20)))
    {
        setInfected ();
    }

    /* Set toggledness. */
    if (isSelectiveFireWeapon ()) {
        setToggled (); /* Guns start out in burst mode. */
    } else if (isA (kObjBioMask)) {
        if (RNG (2)) setToggled (); /* Random mode. */
    }

    /* Remaining properties. */
    mHP = myIlk ()->mHP;
    if (!isEnhanceable ())
        setEnhancementKnown ();
    if (!isChargeable ())
        setChargeKnown ();
    if (!isA (kFloppyDisk) and !isA (kObjGenericKeycard))
        setCrackedKnown ();

    if (!strcmp (myIlk ()->mVague.mName, myIlk ()->mAppearance.mName) and
        !isA (kObjTorc) and !isA (kObjLightSaber) and !isA (kObjFlashlight))
    {
        setAppearanceKnown ();
    }

    /* Some items come in random colors. */
    if (isA (kObjLightSaber)) {
        mColor = RNG (3); /* Red, blue and green. */
    } else if (isA (kFloppyDisk)) {
        mColor = RNG (numFloppyColors);
    } else {
        mColor = 0;
    }
}
