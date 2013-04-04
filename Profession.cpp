#include "Global.h"
#include "Util.h"
#include "Creature.h"
#include "Profession.h"
#include "Object.h"
#include "Creature.h"
#include "Mutant.h"
#include "Hero.h"


shVector <shProfession *> Professions;
shProfession *Janitor;
shProfession *SoftwareEngineer;
shProfession *SpaceMarine;
shProfession *Quarterback;
shProfession *Psion;
shProfession *Astronaut;
shProfession *Ninja;
shProfession *Detective;
shProfession *Abductor;
shProfession *Dissident;
shProfession *Mastermind;
shProfession *Yautja;
shProfession *XelNaga;

/* LoreHelp.cpp: */
extern const char *processLoreLine (const char *src, const char *arg = NULL);

shProfession::shProfession (const char *name,
                            int hitdiesize,
                            int numskills,
                            int reflex,
                            int will,
                            shHeroInitFunction *f,
                            const char *t1,
                            const char *t2,
                            const char *t3,
                            const char *t4,
                            const char *t5,
                            const char *t6,
                            const char *t7,
                            const char *t8,
                            const char *t9,
                            const char *t10)
{
    mId = Professions.add (this);
    mName = name;
    mHitDieSize = hitdiesize;
    mNumPracticedSkills = numskills;
    mReflexSaveBonus = reflex;
    mWillSaveBonus = will;
    mInitFunction = f;
    mTitles[0] = t1;
    mTitles[1] = t2;
    mTitles[2] = t3;
    mTitles[3] = t4;
    mTitles[4] = t5;
    mTitles[5] = t6;
    mTitles[6] = t7;
    mTitles[7] = t8;
    mTitles[8] = t9;
    mTitles[9] = t10;
}

static const shObjId commonlore[] = {
    /* Everyone knows stormtroopers wear stormtrooper stuff. Almost. */
    kObjStormtrooperHelmet,
    kObjStormtrooperSuit,
    kObjStormtrooperBoots,
    /* Same with aquamarine and mean green marines. */
    kObjAquamarinePH,
    kObjAquamarinePA,
    kObjMeanGreenPH,
    kObjMeanGreenPA,
    kObjDeepBluePA,
    /* Elves too. */
    kObjElvenJumpsuit,
    kObjElvenSpaceSuit,
    /* Standard issue weapons. */
    kObjBlaster,
    kObjPhaser,
    kObjLaserPistol,
    kObjLaserRifle
};
static const int numcommonfacts = sizeof (commonlore) / sizeof (shObjId);

static const shObjId humanlore[] = {
    /* Humans and orcs recognize this stuff. */
    kObjKevlarHelmet,
    kObjKevlarJacket,
    kObjAsbestosJacket,
    kObjFlakJacket,
    kObjSpaceHelmet,
    kObjSpaceSuit,
    kObjSpaceBoots,
    kObjLaserCannon,
    kObjMiningLaser
};
static const int numhumanfacts = sizeof (humanlore) / sizeof (shObjId);

static const shObjId orclore[] = {
    /* Orcs know these well. */
    kObjPowerClaw,
    kObjPeaShooter
};
static const int numorcfacts = sizeof (orclore) / sizeof (shObjId);

static const shObjId reticulanlore[] = {
    /* Reticulans recognize this stuff. Humans usually do not. */
    kObjReticulanJumpsuit,
    kObjBioArmor,
    kObjEBioArmor1,
    kObjEBioArmor2,
    kObjEBioArmor3,
    kObjZapGun,
    kObjZapBaton
};
static const int numreticulanfacts = sizeof (reticulanlore) / sizeof (shObjId);

static const shObjId yautjalore[] = {
    /* Predator stuff. */
    kObjPlateArmor,
    kObjBioMask,
    kObjCloakingBelt,
    kObjSmartDisc,
    kObjWristBlade,
    kObjCombiStick,
    kObjRazorWhip,
    kObjNetLauncher,
    kObjSpeargun,
    kObjPlasmaCaster,
    kObjMedicomp,
    kObjSatCom
};
static const int numyautjafacts = sizeof (yautjalore) / sizeof (shObjId);

static void
addKnowledge (const shObjId list[], int count)
{
    for (int i = 0; i < count; ++i)
        AllIlks[list[i]].mFlags |= kIdentified;
}

static void
addStartingEquipment (shHero *hero, const char *spec[], int count)
{
    int ignore_and = 1;
    int ignore_or = 1;
    shObject *obj;
    for (int i = 0; i < count; ++i) {
        const char *desc = spec[i];
        if (spec[i][0] == '|') {
            if (!ignore_or) {
                ++desc;
                ignore_and = 0;
                ignore_or = 1;
            } else {
                ignore_and = 1;
                continue;
            }
        } else if (spec[i][0] == ',') {
            if (!ignore_and) {
                ++desc;
            } else {
                continue;
            }
        }
        obj = createObject (desc, 0);
        ignore_and = obj == NULL;
        ignore_or = obj != NULL;
        if (obj) {
            obj->identify ();
            if (hero->isOrc () and !obj->isBugProof ()) {
                /* Orcs don't care as much.  They get random bugginess. */
                obj->resetBugginessKnown ();
                obj->mBugginess = RNG (-1, +1);
                if (obj->isA (kObjMasterKeycard) and obj->isBuggy ()) {
                    obj->mBugginess = RNG (2); /* Would suck if it was buggy. */
                }
            }
            hero->addObjectToInventory (obj, 1);
            if (obj->canBeWorn ()) {
                hero->don (obj, 1);
            } else if (obj->isA (kWeapon)) {
                hero->wield (obj, 1);
            }
        }
    }
    hero->reorganizeInventory ();
}

struct skillData
{
    shSkillCode skill;
    int access;
    int ranks;
};

static void
addSkills (shHero *hero, const skillData spec[], int count)
{
    for (int i = 0; i < count; ++i) {
        hero->addSkill (spec[i].skill, spec[i].access);
        if (spec[i].ranks)
            hero->gainRank (spec[i].skill, spec[i].ranks);
    }
}

struct mutationData
{
    shMutantPower power;
    int access;
};

static void
addPowers (shHero *hero, const mutationData spec[], int count,
                 int powers, int ranks)
{
    for (int i = 0; i < count; ++i) {
        hero->addSkill (kMutantPower, spec[i].access, spec[i].power);
    }
    for (int i = 0; i < powers; ++i) {
        int idx;
        do {
            idx = RNG (count);
        } while (hero->mMutantPowers[spec[idx].power] >= 1 or
                 MutantPowers[spec[idx].power].mLevel >= 5);
        hero->getMutantPower (spec[idx].power, 1);
        hero->gainRank (kMutantPower, ranks, spec[idx].power);
    }
}

void
showIntro (shHero *hero, const char *introLines[], int count)
{
    char *buf = GetBuf ();
    if (hero->isOrc ()) {
        snprintf (buf, SHBUFLEN, "Hai %s!", hero->mName);
    } else {
        snprintf (buf, SHBUFLEN, "Greetings, %s the %s.",
                  hero->mName, hero->mProfession->mName);
    }
    shMenu *intro = I->newMenu (buf, shMenu::kNoPick);
    for (int i = 0; i < count; ++i) {
        const char *line = processLoreLine (introLines[i]);
        intro->addPtrItem (' ', line, NULL);
    }
    intro->finish ();
    delete intro;
}

static void
initSoftwareEngineer (shHero *hero)
{
    hero->mIlkId = kMonEarthling;
    hero->mGlyph = hero->myIlk ()->mGlyph;
    hero->mGlyph.mTileX = 4;
    hero->mGlyph.mTileY = 0;

    hero->mProfession = SoftwareEngineer;
    hero->rollAbilityScores (8, 8, 8, 12, 13, 6);

    hero->mMaxHP = SoftwareEngineer->mHitDieSize +
        ABILITY_MODIFIER (hero->getCon ());

    hero->mInnateIntrinsics |= kBugSensing;

    const skillData skills[] =
    {
        {kGrenade,       2, 0},   {kHandgun,       3, 0},
        {kLightGun,      1, 0},   {kHeavyGun,      1, 0},
        {kUnarmedCombat, 0, 0},   {kMeleeWeapon,   1, 0},
        {kSword,         1, 0},

        {kConcentration, 2, 0},   {kOpenLock,      3, 0},
        {kRepair,        3, 0},   {kSearch,        3, 0},
        {kHacking,       4, 2},   {kSpot,          2, 0}
    };
    addSkills (hero, skills, sizeof (skills) / sizeof (skillData));

    addKnowledge (commonlore, numcommonfacts);
    addKnowledge (humanlore, numhumanfacts);

    const char *stuff[] =
    {
        "debugged +0 pea shooter",
        "debugged +0 ordinary jumpsuit",
        "debugged not protected clean +0 mini computer",
        "clean not cracked floppy disk",
        "clean not cracked floppy disk",
        "clean not cracked floppy disk",
        "66% clean not cracked floppy disk",
        "33% clean not cracked floppy disk",
        "1d2 debugged canisters of coffee",
        "1d2 debugged rolls of duct tape",
        "debugged restraining bolt",
        "200 energy cells",
        "2d100 buckazoids"
    };
    addStartingEquipment (hero, stuff, sizeof (stuff) / sizeof (char *));
}


static void
initSpaceMarine (shHero *hero)
{
    hero->mIlkId = kMonEarthling;
    hero->mGlyph = hero->myIlk ()->mGlyph;
    hero->mGlyph.mTileX = 5;
    hero->mGlyph.mTileY = 0;

    hero->mProfession = SpaceMarine;
    hero->rollAbilityScores (13, 12, 9, 11, 7, 8);
    if (hero->mAbil.mStr < 10) {
        hero->mAbil.mStr = 10;
        hero->mMaxAbil.mStr = 10;
    }

    hero->mMaxHP = SpaceMarine->mHitDieSize +
        ABILITY_MODIFIER (hero->getCon ());

    const skillData skills[] =
    {
        {kGrenade,       3, 2},   {kHandgun,       4, 2},
        {kLightGun,      4, 2},   {kHeavyGun,      4, 2},
        {kUnarmedCombat, 3, 0},   {kMeleeWeapon,   3, 0},
        {kSword,         1, 0},

        {kConcentration, 0, 0},   {kOpenLock,      2, 0},
        {kRepair,        2, 0},   {kSearch,        2, 0},
        {kHacking,       0, 0},   {kSpot,          2, 0}
    };
    addSkills (hero, skills, sizeof (skills) / sizeof (skillData));

    /* Professional knowledge: */
    const shObjId proflore[] = {
        kObjPowerFist,
        kObjPlasmaPistol,
        kObjPlasmaRifle,
        kObjPlasmaCannon
    };
    addKnowledge (commonlore, numcommonfacts);
    addKnowledge (humanlore, numhumanfacts);
    addKnowledge (proflore, 4);

    const char *stuff[] =
    {
        "100 energy cells",
        "99 bullets",
        "3d2 debugged +0 flares",
        "debugged +1 flak jacket",
        "debugged +0 ordinary pistol",
        "debugged +0 pulse rifle",
        "debugged motion tracker",
        "2d10 buckazoids"
    };
    addStartingEquipment (hero, stuff, sizeof (stuff) / sizeof (char *));

    const char *welcome[] =
    {
    "Two weeks. Two weeks trapped inside that damned cargo pod with only",
    "the requisition clerkbot to keep company. And the terrors outside. You",
    "told him. YOU TOLD HIM. Don't buy those damn \"krab patties\", Johnny.",
    "You don't know what that \"krab\" is, you never know with these backwater",
    "agriworlds! He laughed on you. You told them. I wouldn't eat those damn",
    "\"krab patties\" guys, Johnny bought'em off a weird looking yellow guy",
    "in a fishbowl! They scorned you, and their laughter echoed through the",
    "mess hall. Go eat your frozen earth shitburgers, they said. You're being",
    "hypochondriac, these krab patties are awesome - but don't worry, I'll",
    "take yours. And they laughed.",
    "",
    "WELL WHO'S LAUGHING NOW WITH HIS GUTS SPRAYED ALL OVER THE DINNER TABLE,",
    "JOHNNY?! WHO'S LAUGHING WITH HIS LUNGS OOZING OUT OF HIS EXPLODED CHEST?!!",
    "",
    "Not me! I'm getting out of this graveyard ship, and calling back to command,",
    "however long it takes. I'm going to be the most frickin' condecorated marine",
    "this side of the Perdus rift. And your sorry asses will rot in that",
    "MOTHERFUCKING DEATH TRAP FOREVER!!!",
    "",
    "So long, bastards.",
    "",
    "~*heroname*"
    };
    showIntro (hero, welcome, sizeof (welcome) / sizeof (char *));
}


static void
initQuarterback (shHero *hero)
{
    hero->mIlkId = kMonEarthling;
    hero->mGlyph = hero->myIlk ()->mGlyph;
    hero->mGlyph.mTileX = 10;
    hero->mGlyph.mTileY = 0;

    hero->mProfession = Quarterback;
    hero->rollAbilityScores (12, 10, 13, 12, 7, 10);

    hero->mMaxHP = Quarterback->mHitDieSize +
        ABILITY_MODIFIER (hero->getCon ());

    const skillData skills[] =
    {
        {kGrenade,       4, 2},   {kHandgun,       2, 0},
        {kLightGun,      2, 0},   {kHeavyGun,      2, 0},
        {kUnarmedCombat, 4, 0},   {kMeleeWeapon,   3, 0},
        {kSword,         2, 0},

        {kConcentration, 2, 0},   {kOpenLock,      0, 0},
        {kRepair,        0, 0},   {kSearch,        2, 0},
        {kHacking,       0, 0},   {kSpot,          4, 2}
    };
    addSkills (hero, skills, sizeof (skills) / sizeof (skillData));

    addKnowledge (commonlore, numcommonfacts);
    addKnowledge (humanlore, numhumanfacts);

    const char *stuff[] =
    {
        "debugged +0 set of football pads",
        "debugged +0 football helmet",
        "1 debugged +3 football",
        "200 energy cells",
        "6 debugged canisters of beer",
        "3d100 buckazoids"
    };
    addStartingEquipment (hero, stuff, sizeof (stuff) / sizeof (char *));
}


static void
initPsion (shHero *hero)
{
    hero->mIlkId = kMonEarthling;
    hero->mGlyph = hero->myIlk ()->mGlyph;
    hero->mGlyph.mTileX = 7;
    hero->mGlyph.mTileY = 0;

    hero->mProfession = Psion;
    hero->rollAbilityScores (7, 10, 8, 7, 11, 15);
    if (hero->mAbil.mPsi < 14) {
        hero->mAbil.mPsi = 14;
        hero->mMaxAbil.mPsi = 14;
    }

    hero->mMaxHP = Psion->mHitDieSize +
        ABILITY_MODIFIER (hero->getCon ());

    const skillData skills[] =
    {
        {kGrenade,       1, 0},   {kHandgun,       2, 1},
        {kLightGun,      1, 0},   {kHeavyGun,      1, 0},
        {kUnarmedCombat, 1, 0},   {kMeleeWeapon,   1, 0},
        {kSword,         4, 0},

        {kConcentration, 4, 0},   {kOpenLock,      1, 0},
        {kRepair,        1, 0},   {kSearch,        3, 0},
        {kHacking,       0, 0},   {kSpot,          2, 0}
    };
    addSkills (hero, skills, sizeof (skills) / sizeof (skillData));

    const mutationData powers[] =
    {
        {kIllumination,      4},   {kDigestion,         4},
        {kHypnosis,          4},   {kRegeneration,      4},
        {kOpticBlast,        4},   {kHaste,             4},
        {kTelepathyPower,    4},   {kTremorsensePower,  4},
        {kShootWebs,         4},   {kMentalBlast,       4},
        {kRestoration,       4},   {kAdrenalineControl, 4},
        {kXRayVisionPower,   4}, /*{kTelekinesis,       4},*/
      /*{kInvisibility,      4},   {kCharm,             4},*/
        {kTeleport,          4},   {kPsionicStorm,      3}
    };
    addPowers (hero, powers, sizeof (powers) / sizeof (mutationData), 2, 2);

    addKnowledge (commonlore, numcommonfacts);
    addKnowledge (humanlore, numhumanfacts);

    const char *stuff[] =
    {
        "debugged +1 laser pistol",
        "debugged +1 ordinary jumpsuit",
        "debugged pair of sunglasses",
        "200 energy cells",
        "2 debugged canisters of Nano-Cola",
        "1 debugged canister of healing",
        "1 debugged canister of RadAway",
        "2d10 buckazoids"
    };
    addStartingEquipment (hero, stuff, sizeof (stuff) / sizeof (char *));
}


static void
initJanitor (shHero *hero)
{
    hero->mIlkId = kMonSpaceOrc;
    hero->mGlyph = hero->myIlk ()->mGlyph;
    hero->mGlyph.mTileX = 6;
    hero->mGlyph.mTileY = 0;

    hero->mProfession = Janitor;
    hero->rollAbilityScores (10, 12, 10, 12, 8, 8);

    hero->mMaxHP = Janitor->mHitDieSize +
        ABILITY_MODIFIER (hero->getCon ());

    const skillData skills[] =
    {
        {kGrenade,       1, 0},   {kHandgun,       1, 0},
        {kLightGun,      2, 0},   {kHeavyGun,      1, 0},
        {kUnarmedCombat, 3, 0},   {kMeleeWeapon,   4, 0},
        {kSword,         2, 0},

        {kConcentration, 2, 0},   {kOpenLock,      0, 0},
        {kRepair,        4, 2},   {kSearch,        4, 2},
        {kHacking,       0, 0},   {kSpot,          4, 2}
    };
    addSkills (hero, skills, sizeof (skills) / sizeof (skillData));

    addKnowledge (commonlore, numcommonfacts);
    addKnowledge (humanlore, numhumanfacts);
    addKnowledge (orclore, numorcfacts);

    const char *stuff[] =
    {
        "debugged +1 mop",
        "debugged +0 janitor uniform",
        "debugged +1 pair of sturdy boots",
        "debugged canister of super glue",
        "optimized +0 monkey wrench",
        "debugged master keycard",
        "debugged restraining bolt",
        "75 energy cells",
        "1d100 buckazoids"
    };
    addStartingEquipment (hero, stuff, sizeof (stuff) / sizeof (char *));

    const char *welcome[] =
    {
    "Oi kusin Noshkop!",
    "",
    "Ya recall dat blabbing bout putting ol' Skragashuv back in biznes?",
    "",
    "Well me found one of dem Ur Magnetrons ya need for the warp",
    "gubbinz to rev up.",
    "",
    "I gab it all cleverly thott.",
    "",
    "Dere's dis Space Base, wiv all sorta loot, an a Ur Mangegubbin",
    "sittin' all alone in da bottom. An dey lookin' fer a jenitar!",
    "",
    "Dis 'janiter' is s'posed to go into dis 'ulk an get rid of any",
    "panzees, beekees, roaches sittin' dere an clean up all da mess",
    "dem making in the 'ulk.",
    "",
    "Sum one to \"clean da mess up\"! Got it!?",
    "",
    "Im sure gonna lick da place clean. I Can go anywhere I like cos",
    "I'm some sorta junitor, anything dats not bolted to da floor is",
    "mine, an nobody's messin wiv meh cos I'm a ork."
    "",
    "So you start savin' dem teef cus Im a bringin dat magnefing.",
    "",
    "Oh if you'z not up fer it, dun worry, Ill just clobber you up da",
    "head so bad you'll be seein stars by day but no bad feelins kusin.",
    "",
    "~*heroname*"
    };
    showIntro (hero, welcome, sizeof (welcome) / sizeof (char *));
}


static void
initAstronaut (shHero *hero) //Psiweapon keeps screwing around
{
    hero->mIlkId = kMonEarthling;
    hero->mGlyph = hero->myIlk ()->mGlyph;
    hero->mGlyph.mTileX = 8;
    hero->mGlyph.mTileY = 0;

    hero->mProfession = Astronaut;
    hero->rollAbilityScores (8, 10, 10, 12, 12, 10);

    hero->mMaxHP = Astronaut->mHitDieSize +
        ABILITY_MODIFIER (hero->getCon ());

    const skillData skills[] =
    {
        {kGrenade,       1, 0},   {kHandgun,       3, 0},
        {kLightGun,      2, 0},   {kHeavyGun,      1, 0},
        {kUnarmedCombat, 3, 1},   {kMeleeWeapon,   1, 0},
        {kSword,         0, 0},

        {kConcentration, 2, 0},   {kOpenLock,      1, 0},
        {kRepair,        3, 1},   {kSearch,        3, 2},
        {kHacking,       2, 0},   {kSpot,          4, 0}
    };
    addSkills (hero, skills, sizeof (skills) / sizeof (skillData));

    addKnowledge (commonlore, numcommonfacts);
    addKnowledge (humanlore, numhumanfacts);

    //HAHAHA NO WEAPON! SUCKER! (well some of a challenge!)
    const char *stuff[] =
    {
        "debugged +0 radiation suit",
        "debugged +0 space helmet",
        "debugged +0 pair of space boots",
        "debugged -3 space suit",
        "1 optimized canister of antimatter",
        "1 optimized canister of healing",
        "1 optimized canister of water",
        "(200 charges) small energy tank",
        "optimized tricorder"
    };
    addStartingEquipment (hero, stuff, sizeof (stuff) / sizeof (char *));

    const char *welcome[] =
    {
        "You had been tasked with the first test flight of the Quantum IV",
        "hyperspace engine.  Just as you flew clear of any gravity wells, you",
        "deccelerated and began recording the experiment logs.  Main power",
        "diverted to the prototype engine, all systems green.  All systems?  No!",
        "The flux capacitor displays in red!  Without a second thought, you fasten",
        "your space helmet on, and with an adept kick you deftly propel yourself",
        "to an emergency pod, all the while screaming OH SHIT OH SHIT OH SHIT",
        "at the top of your lungs.",
        "",
        "From a couple miles away, you see the spaceship liquefy and fold into",
        "itself as it's sucked away from within by some sort of warp-hole.",
        "You broadcast what you think are your last words, full with excrement",
        "slang and considerations about the dubious parentage and upbringing",
        "of the shipyard CEO, among other colorful expletives.",
        "",
        "After passing out from a panic attack, and through the warp rift, you find",
        "yourself orbiting a derelict asteroid turned space station.  With nothing",
        "to lose except your life, you dock and set out to explore its bowels."
    };
    showIntro (hero, welcome, sizeof (welcome) / sizeof (char *));
}


static void
initMastermind (shHero *hero)
{
    hero->mIlkId = kMonTallGreyAlien;
    hero->mGlyph = hero->myIlk ()->mGlyph;
    hero->mGlyph.mTileX = 3;
    hero->mGlyph.mTileY = 0;

	hero->mProfession = Mastermind;
    /* The -2 penalty is for activated telepathy on the start. */
    hero->rollAbilityScores (7, 7, 12, 10, 14, 16 - 2);
    hero->mMaxHP = Mastermind->mHitDieSize +
        ABILITY_MODIFIER (hero->getCon ());

    const skillData skills[] =
    {
        {kGrenade,       2, 0},   {kHandgun,       2, 1},
        {kLightGun,      2, 0},   {kHeavyGun,      1, 0},
        {kUnarmedCombat, 3, 1},   {kMeleeWeapon,   1, 0},
        {kSword,         2, 0},

        {kConcentration, 4, 0},   {kOpenLock,      3, 0},
        {kRepair,        1, 0},   {kSearch,        2, 0},
        {kHacking,       0, 0},   {kSpot,          3, 0}
    };
    addSkills (hero, skills, sizeof (skills) / sizeof (skillData));

    hero->mMutantPowers[kTelepathyPower] = 2; /* Posessed and activated. */
    hero->telepathy (1);
    const mutationData powers[] =
    {
        {kIllumination,      4},   {kDigestion,         4},
        {kHypnosis,          4},   {kRegeneration,      4},
      /*{kOpticBlast,        0},*/ {kHaste,             4},
        {kTelepathyPower,    4},   {kTremorsensePower,  4},
      /*{kShootWebs,         0},*/ {kMentalBlast,       4},
        {kRestoration,       4},   {kAdrenalineControl, 4},
        {kXRayVisionPower,   4}, /*{kTelekinesis,       4},*/
      /*{kInvisibility,      4},   {kCharm,             4},*/
        {kTeleport,          4},   {kPsionicStorm,      3}
    };
    addPowers (hero, powers, sizeof (powers) / sizeof (mutationData), 2, 1);
    hero->gainRank (kMutantPower, 2, kTelepathyPower);

    addKnowledge (commonlore, numcommonfacts);
    addKnowledge (reticulanlore, numreticulanfacts);

    const char *stuff[] =
    {
        "not buggy +0 anal probe",
        "debugged +1 reticulan jumpsuit",
        "debugged +0 energy dome",
        "optimized +1 reticulan zap gun",
        "150 energy cells",
        "2 optimized canisters of nano cola",
        "optimized canister of full healing"
    };
    addStartingEquipment (hero, stuff, sizeof (stuff) / sizeof (char *));
}

static void
initDissident (shHero *hero)
{
    hero->mIlkId = kMonLittleGreyAlien;
    hero->mGlyph = hero->myIlk ()->mGlyph;
    hero->mGlyph.mTileX = 1;
    hero->mGlyph.mTileY = 0;

	hero->mProfession = Dissident;
	hero->rollAbilityScores (8, 6, 10, 14, 16, 8);
	hero->mMaxHP = Dissident->mHitDieSize +
		ABILITY_MODIFIER (hero->getCon ());

    const skillData skills[] =
    {
        {kGrenade,       2, 0},   {kHandgun,       1, 0},
        {kLightGun,      3, 1},   {kHeavyGun,      3, 0},
        {kUnarmedCombat, 0, 0},   {kMeleeWeapon,   2, 1},
        {kSword,         0, 0},

        {kConcentration, 1, 0},   {kOpenLock,      2, 0},
        {kRepair,        2, 0},   {kSearch,        2, 0},
        {kHacking,       3, 1},   {kSpot,          3, 0}
    };
    addSkills (hero, skills, sizeof (skills) / sizeof (skillData));

    addKnowledge (commonlore, numcommonfacts);
    addKnowledge (reticulanlore, numreticulanfacts);

    const char *stuff[] =
    {
	    "optimized +0 reticulan bio armor",
        "debugged stabilizer belt",
        "debugged not protected clean +0 bio computer",
        "clean not cracked floppy disk",
        "clean not cracked floppy disk",
        "3 debugged grenades",
        "debugged +0 laser rifle",
        "200 energy cells",
        "debugged +1 reticulan zap baton"
    };
    addStartingEquipment (hero, stuff, sizeof (stuff) / sizeof (char *));
}

static void
initAbductor (shHero *hero)
{
    hero->mIlkId = kMonLittleGreyAlien;
    hero->mGlyph = hero->myIlk ()->mGlyph;
    hero->mGlyph.mTileX = 2;
    hero->mGlyph.mTileY = 0;

    hero->mProfession = Abductor;
    hero->rollAbilityScores (6, 6, 10, 12, 14, 14 - 2);
    hero->mMaxHP = Abductor->mHitDieSize +
        ABILITY_MODIFIER (hero->getCon ());

    const skillData skills[] =
    {
        {kGrenade,       3, 0},   {kHandgun,       2, 1},
        {kLightGun,      2, 0},   {kHeavyGun,      2, 0},
        {kUnarmedCombat, 0, 0},   {kMeleeWeapon,   3, 1},
        {kSword,         0, 0},

        {kConcentration, 4, 0},   {kOpenLock,      3, 0},
        {kRepair,        2, 0},   {kSearch,        3, 0},
        {kHacking,       0, 0},   {kSpot,          4, 2}
    };
    addSkills (hero, skills, sizeof (skills) / sizeof (skillData));

    /* Start with telepathy activated already.  Since this avoids -2 to
       max psionics Abductors get penalized for this in ability roll. */
    hero->mMutantPowers[kTelepathyPower] = 2;
    hero->telepathy (1);

    addKnowledge (commonlore, numcommonfacts);
    addKnowledge (reticulanlore, numreticulanfacts);

    const char *stuff[] =
    {
        "optimized +1 reticulan jumpsuit",
        "debugged +0 space helmet",
        "debugged shield belt",
        "300 energy cells",
        "optimized +0 anal probe",
        "not buggy ray gun",
        "debugged +0 reticulan zap gun",
        "2 debugged canisters of healing"
    };
    addStartingEquipment (hero, stuff, sizeof (stuff) / sizeof (char *));
}

static void
initNinja (shHero *hero)
{
    hero->mIlkId = kMonEarthling;
    hero->mGlyph = hero->myIlk ()->mGlyph;
    hero->mGlyph.mTileX = 0;
    hero->mGlyph.mTileY = 0;

    hero->mProfession = Ninja;
    hero->rollAbilityScores (13, 10, 10, 13, 8, 8);

    hero->mMaxHP = Ninja->mHitDieSize +
        ABILITY_MODIFIER (hero->getCon ());

    const skillData skills[] =
    {
        {kGrenade,       3, 1},   {kHandgun,       3, 0},
        {kLightGun,      2, 0},   {kHeavyGun,      2, 0},
        {kUnarmedCombat, 2, 0},   {kMeleeWeapon,   3, 0},
        {kSword,         4, 1},

        {kConcentration, 2, 0},   {kOpenLock,      4, 2},
        {kRepair,        0, 0},   {kSearch,        2, 0},
        {kHacking,       2, 0},   {kSpot,          4, 0}
    };
    addSkills (hero, skills, sizeof (skills) / sizeof (skillData));

    /* Professional knowledge: */
    const shObjId proflore[] = {
        kObjNunchucks,
        kObjShuriken,
        kObjBo
    };
    addKnowledge (commonlore, numcommonfacts);
    addKnowledge (humanlore, numhumanfacts);
    addKnowledge (proflore, 3);

    /* Up to three additional implant types known at the start. */
    for (int i = RNG (1, 3); i > 0; --i) {
        shObjId imp = (shObjId) RNG (kObjFirstImplant, kObjLastImplant);
        AllIlks[imp].mFlags |= kIdentified;
    }

    const char *stuff[] =
    {
        "3 debugged +0 frag grenades",
        "50% 1 debugged +0 frag grenade",
        "optimized +0 katana",
        "optimized +1 chameleon suit",
        "debugged lock pick",
        "25% debugged pair of scouter goggles",
        "| 33% debugged pair of night vision goggles",
        "| 50% debugged pair of x-ray goggles",
        "| debugged pair of targeter goggles",
        "debugged +2 beneficial implant"
    };
    addStartingEquipment (hero, stuff, sizeof (stuff) / sizeof (char *));

    const char *welcome[] =
    { /* Dead Cold style purposeful arrival. */
        "You have come here bearing the body",
        "of your mentor.  It was his request",
        "that interrment be performed here.",
        "",
        "Oddly, the station gave no response",
        "to your docking request.  You pull",
        "into an open shuttle bay and prepare",
        "to disembark."
    };
    showIntro (hero, welcome, sizeof (welcome) / sizeof (char *));
}
/*
static void
initDetective (shHero *hero)
{
    hero->mIlkId = kMonEarthling;
    hero->mGlyph = hero->myIlk ()->mGlyph;
    hero->mGlyph.mTileX = 9;
    hero->mGlyph.mTileY = 0;

    hero->mProfession = Detective;
    hero->rollAbilityScores (10, 12, 10, 13, 11, 8);

    hero->mMaxHP = Detective->mHitDieSize +
        ABILITY_MODIFIER (hero->getCon ());

    const skillData skills[] =
    {
        {kGrenade,       1, 0},   {kHandgun,       4, 2},
        {kLightGun,      2, 0},   {kHeavyGun,      0, 0},
        {kUnarmedCombat, 3, 0},   {kMeleeWeapon,   1, 0},
        {kSword,         0, 0},

        {kConcentration, 2, 0},   {kOpenLock,      3, 0},
        {kRepair,        0, 0},   {kSearch,        3, 2},
        {kHacking,       0, 0},   {kSpot,          2, 0}
    };
    addSkills (hero, skills, sizeof (skills) / sizeof (skillData));

    addKnowledge (commonlore, numcommonfacts);
    addKnowledge (humanlore, numhumanfacts);

    const char *stuff[] =
    {
        "debugged +0 Khan heavy pistol",
        "debugged +0 badass trenchcoat",
        "bullets",
        "bullets",
        "1d2 debugged canisters of coffee",
        "debugged detective license",
        "debugged flashlight",
        "10 energy cells"
    };
    addStartingEquipment (hero, stuff, sizeof (stuff) / sizeof (char *));
}
*/
static void
initYautja (shHero *hero)
{
    hero->mIlkId = kMonYautja;
    hero->mGlyph = hero->myIlk ()->mGlyph;
    hero->mGlyph.mTileX = 11;
    hero->mGlyph.mTileY = 0;

    hero->mProfession = Yautja;
    hero->rollAbilityScores (12, 15, 8, 10, 10, 5);
    if (hero->mAbil.mCon < 12) {
        hero->mAbil.mCon = 12;
        hero->mMaxAbil.mCon = 12;
    }

    hero->mMaxHP = Yautja->mHitDieSize +
        ABILITY_MODIFIER (hero->getCon ());

    hero->mInnateIntrinsics |= kJumpy;

    const skillData skills[] =
    {
        {kGrenade,       4, 0},   {kHandgun,       3, 0},
        {kLightGun,      3, 0},   {kHeavyGun,      2, 0},
        {kUnarmedCombat, 4, 0},   {kMeleeWeapon,   4, 0},
        {kSword,         0, 0},

        {kConcentration, 1, 0},   {kOpenLock,      0, 0},
        {kRepair,        2, 0},   {kSearch,        1, 0},
        {kHacking,       1, 0},   {kSpot,          3, 1}
    };
    addSkills (hero, skills, sizeof (skills) / sizeof (skillData));

    addKnowledge (yautjalore, numyautjafacts);

    const char *stuff[] =
    {
        "debugged +0 plate armor",
        "debugged +0 bio-mask",
        "debugged cloaking belt",
        "100 energy cells",
        "buggy glory device",
        "debugged not protected clean +0 sat-com computer",
        "debugged (40 charges) medicomp"
    };
    addStartingEquipment (hero, stuff, sizeof (stuff) / sizeof (char *));

    const char *ranged[] =
    {
        "debugged +0 net launcher",
        ", 8d4 web caskets",
        "debugged +0 plasma caster",
        ", 200 energy cells",
        "debugged +0 speargun rifle",
        ", 20d2 spearhead darts",
        "debugged +0 Smart-Disc",
        ", debugged clean not cracked floppy disk of Smart-Disc recall"
    };
    int rngwpn = RNG (4);
    addStartingEquipment (hero, &ranged[rngwpn * 2], 2);
    hero->gainRank (rngwpn <= 1 ? kHandgun :
                    rngwpn == 2 ? kLightGun : kGrenade, 2);

    const char *melee[] =
    {
        "optimized +0 wrist blade",
        "debugged +0 razor whip",
        "debugged +0 combi-stick"
    };
    int melwpn = RNG (3);
    addStartingEquipment (hero, &melee[melwpn], 1);
    hero->gainRank (melwpn == 0 ? kUnarmedCombat : kMeleeWeapon, 2);
}


static void
initXelNaga (shHero *hero)
{
    hero->mIlkId = kMonXelNaga;
    hero->mGlyph = hero->myIlk ()->mGlyph;
    hero->mGlyph.mTileX = 12;
    hero->mGlyph.mTileY = 0;

    hero->mProfession = XelNaga;
    hero->rollAbilityScores (9, 9, 9, 9, 9, 9);

    hero->mMaxHP = XelNaga->mHitDieSize +
        ABILITY_MODIFIER (hero->getCon ());

    const skillData skills[] =
    {
        {kGrenade,       3, 0},   {kHandgun,       3, 0},
        {kLightGun,      3, 0},   {kHeavyGun,      3, 0},
        {kUnarmedCombat, 3, 0},   {kMeleeWeapon,   3, 0},
        {kSword,         3, 0},

        {kConcentration, 3, 0},   {kOpenLock,      3, 0},
        {kRepair,        3, 0},   {kSearch,        3, 0},
        {kHacking,       3, 0}/*,   {kSpot,          0, 0} */
    };
    addSkills (hero, skills, sizeof (skills) / sizeof (skillData));

    const mutationData powers[] =
    {
        {kIllumination,      3},   {kDigestion,         3},
        {kHypnosis,          3},   {kRegeneration,      3},
      /*{kOpticBlast,        0},*/ {kHaste,             3},
        {kTelepathyPower,    3},   {kTremorsensePower,  3},
        {kShootWebs,         3},   {kMentalBlast,       3},
        {kRestoration,       3},   {kAdrenalineControl, 3},
      /*{kXRayVisionPower,   3},   {kTelekinesis,       3},*/
      /*{kInvisibility,      3},   {kCharm,             3},*/
        {kTeleport,          3},   {kPsionicStorm,      2}
    };
    addPowers (hero, powers, sizeof (powers) / sizeof (mutationData), 0, 0);
    /* No starting mutations. */
    /* No knowledge at all. */
    /* No equipment either. */
}

/* Check generated inventory for items that should not be there. */
void
shProfession::postInit (shHero *hero)
{   /* Idea: When more such items appear there could be a table with columns:
             unwanted item, replacement1, replacement2. */
    for (int i = 0; i < hero->mInventory->count (); ++i) {
        shObject *obj = hero->mInventory->get (i);
        switch (obj->mIlkId) {
        case kObjRadGrenade:
            if (hero->mProfession != Astronaut)
                obj->mIlkId = RNG (2) ? kObjConcussionGrenade : kObjFragGrenade;
            break;
        case kObjGammaRayGun:
            if (hero->mProfession != Astronaut)
                obj->mIlkId = RNG (2) ? kObjHeatRayGun : kObjFreezeRayGun;
            break;
        case kObjRecallDisk:
            if (hero->mProfession != Yautja)
                obj->mIlkId = kObjIdentifyDisk;
            break;
        default:
            break;
        }
    }
}


void
initializeProfessions (void)
{
    Psion = new shProfession ("psion", 8, 3, 2, 2, initPsion,
        "Odd Ball",
        "Weirdo",
        "Mind Reader",
        "Spoon Bender",
        "Freakazoid",
        "Mutant",
        "Empath",
        "Psion",
        "Psyker",
        "Farseer");

    SoftwareEngineer = new shProfession ("software engineer", 8, 3, 1, 2,
        initSoftwareEngineer,
        "Summer Intern",
        "Q/A Tester",
        "Web Designer",
        "Help Desk Jockey",
        "Jr. Programmer",
        "Sysadmin",
        "Programmer",
        "Lead Programmer",
        "VP Engineering",
        "High Programmer");

/*  Was too similar to software engineer to warrant inclusion.
    Keeping the titles for now.
    Cracker = new shProfession ("cracker", 8, 4, 1, 2,
        initCracker,
        "Savescummer",
        "File Sharer",
        "W@r3z d00d",
        "Script Kiddie",
        "h@x0r",
        "1337 h@x0r",
        "Decker",
        "Sneaker",
        "Phreaker",
        "One");
*/
    Janitor = new shProfession ("janitor", 9, 3, 2, 2, initJanitor,
        "Toilet Scrubber",
        "Mop Boy",
        "Janitor",
        "Housekeeper",
        "Custodian",
        "Maintenance Man",
        "Sanitation Freak",
        "Superintendent",
        "Property Manager",
        "Landlord");

    SpaceMarine = new shProfession ("space marine", 10, 2, 2, 1,
        initSpaceMarine,
        "Private",
        "Corporal",
        "Sergeant",
        "Cadet",
        "Lieutentant",
        "Captain",
        "Major",
        "Lt. Colonel",
        "Colonel",
        "General");

    Quarterback = new shProfession ("quarterback", 10, 2, 2, 1,
        initQuarterback,
        "Towel Boy",
        "Rookie",
        "Bench Warmer",
        "Starter",
        "Jock",
        "Star Player",
        "Team Captain",
        "MVP",
        "Pro Bowler",
        "Hall Of Famer");

    Astronaut = new shProfession ("astronaut", 8, 3, 3, 2,
        initAstronaut,
        "Rocket Tester",
        "Vostok Martyr",
        "Moon Walker",
        "Deimos Stevedore",
        "Leonov Engineer",
        "Marauder Pilot",
        "Von Braun Staff",
        "LongShot Navigator", /* two chars too long */
        "Tie Fighter Ace",
        "Nostromo Survivor"); /* one char too long */
    /* Titles are cool so let them stay like this. -- MB */

    Ninja = new shProfession ("ninja", 9, 3, 4, 1, initNinja,
        "Eavesdropper",
        "Stalker",
        "Looming Shadow",
        "Infiltrator",
        "Sabotager",
        "Terrorist",
        "Corporate Spy",
        "Black Hand",
        "Cyber Assassin",
        "Unseen Master");
/*
    Detective = new shProfession ("detective", 9, 2, 2, 1,
        initDetective,
        "Inquirer",
        "Whistleblower",
        "Street Hound",
        "Plainclothes",
        "Interrogator",
        "Inspector",
        "Private Eye",
        "Blade Runner",
        "Deduction Master",
        "Sherlock Heir");
*/
    Abductor = new shProfession ("abductor", 6, 3, 1, 4,
        initAbductor,
        "Crop Artist",
        "Cattle Molester",
        "Lightshow Pilot",
        "Close Encounter",
        "Branding Aide",
        "Chip Implanter",
        "Cross Breeder",
        "Foetus Retriever",
        "Gene Harvester",
        "Vivisector");

    Dissident = new shProfession ("dissident", 6, 3, 2, 3,
        initDissident,
        "Mars Castaway",
        "Moon Base Staff",
        "Orbital Watch",
        "Land Agent",
        "Covert Eye",
        "Arms Dealer",
        "Belt Capo",
        "Planet Conqueror",
        "System Kingpin",
        "Sector Crimelord");

    Mastermind = new shProfession ("mastermind", 6, 3, 2, 3,
        initMastermind,
        "Far Herald",
        "Space Brother",
        "Cult Idol",
        "First Contact",
        "Federation Envoy",
        "Galactic Senator",
        "Coadunator",
        "Uplifter",
        "Terraformer",
        "Astronaut God");

    Yautja = new shProfession ("hunter", 10, 2, 2, 1,
        initYautja,
        "Unblooded",
        "Blooded",
        "Hunter",
        "Honored One",
        "Master Hunter",
        "Vanguard",
        "Elite",
        "Clan Leader",
        "Elder",
        "Adjudicator");

    XelNaga = new shProfession ("wanderer", 10, 4, 0, 0, initXelNaga,
        "Outworlder",
        "Void Explorer",
        "Genesis Witness",
        "Chaos Shaper",
        "Nebulae Catcher",
        "Star Assembler",
        "Planet Shifter",
        "Galaxy Designer",
        "DNA Programmer",
        "Species Creator");
}


shProfession *
chooseProfession ()
{
    const void *result = NULL;
    const void *randomNoOutsider = (void *) 0x1;
    const void *random = (void *) 0x2;

    shMenu *menu = I->newMenu ("Choose your profession", shMenu::kNoHelp);
    menu->attachHelp ("profession.txt");

    menu->addHeader ("Humans:");
    menu->addPtrItem ('a', "Astronaut", Astronaut);
    /* Detective is not interesting enough at the moment. */
    /* menu->addPtrItem ('d', "Detective", Detective); */
    menu->addPtrItem ('m', "Space Marine", SpaceMarine);
    menu->addPtrItem ('n', "Ninja", Ninja);
    menu->addPtrItem ('p', "Psion", Psion);
    menu->addPtrItem ('q', "Quarterback", Quarterback);
    menu->addPtrItem ('s', "Software Engineer", SoftwareEngineer);
    menu->addHeader ("Space Orcs:");
    menu->addPtrItem ('j', "Janitor", Janitor);
    menu->addHeader ("Reticulans:");
    menu->addPtrItem ('A', "Abductor", Abductor);
    menu->addPtrItem ('D', "Dissident", Dissident);
    menu->addPtrItem ('M', "Mastermind", Mastermind);
    menu->addHeader ("Yautja:");
    menu->addPtrItem ('y', "Hunter", Yautja);
    menu->addHeader ("Outsiders:");
    menu->addPtrItem ('X', "Xel'Naga", XelNaga);
    menu->addHeader ("Random choice:");
    menu->addPtrItem ('r', "Random non-outsider", randomNoOutsider);
    menu->addPtrItem ('R', "Random", random);

    do {
        menu->getPtrResult (&result);
        menu->dropResults ();
        if (randomNoOutsider == result) {
            result = Professions.get (RNG (Professions.count () - 1));
        } else if (random == result) {
            result = Professions.get (RNG (Professions.count ()));
        }
    } while (!result);
    delete menu;
    return (shProfession *) result;
}
