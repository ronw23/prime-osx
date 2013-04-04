#include "Global.h"
#include "Util.h"
#include "Hero.h"
#include "Interface.h"
#include "Services.h"
#include <ctype.h>

/******************************************************

    Code for docbot and Melnorme.

******************************************************/

shServiceDescData MedicalProcedureData[kMedMaxService] =
{
    { "SNU-???", 0 },
    { "THERAC-25", 0 },
    { "JARVIK 7", 0 },
    { "PFAZER", 0 },
    { "PNLS DTH", 0 },      //  5
    { "NACSTAC", 0 },
    { "X-14-39-ZS", 0 },
    { "89 50 4E 47 0D 0A 1A 0A", 0 }, /* PNG file header. */
    { "BBQ--BBQ", 0 }
};

shServiceDescData MelnormeServiceData[6 /* kMelnMaxService */] =
{
    { "Puun-Taffy", 0 },
    { "Juffo-Wup", 0 },
    { "Ta Puun", 0 },
    { "Huffi-Muffi-Guffi", 0 },
    { "Wezzy-Wezzah", 0 },  //  5
    { "Tuffa-Yuf", 0 }
};
/* Could not decide on one to leave off so hardcoded array size. */

void
randomizeServiceNames ()
{
    shuffle (MedicalProcedureData, kMedMaxService, sizeof (shServiceDescData));
    shuffle (MelnormeServiceData, 6, sizeof (shServiceDescData));
}

/* called when the hero enters a shop room */

void
shHero::enterHospital ()
{
    shMonster *doctor = mLevel->getDoctor (mX, mY);

    if (doctor) {
        if (doctor->isHostile ()) {
            return;
        }
        if (tryToTranslate (doctor)) {
            I->p ("\"Welcome to Ol' Sawbot's Precision Surgery Center!\"");
        } else {
            I->p ("%s beeps and bloops pleasantly.", doctor->the ());
        }
        I->p ("Press 'p' to see a menu of available services.");
    } else {
        I->p ("Hmm...  This store appears to be deserted.");
    }
}


void
shHero::leaveHospital ()
{
    shMonster *doc = mLevel->getDoctor (mX, mY);
    shObjectVector v;

    if (!doc) {
        return;
    }
    if (doc->isHostile ()) {
        return;
    } else if (tryToTranslate (doc)) {
        I->p ("\"Come again soon!\"");
    } else {
        I->p ("%s twitters cheerfully.", doc->the ());
    }
}



static void
amputationDiscount (shCreature *doctor, shCreature *hero, bool *flag, int price)
{
    if (!*flag) {
        *flag = true;
    } else {
        if (Hero.tryToTranslate (doctor)) {
            I->p ("\"As advertised every second amputation is half off.\"");
        } else {
            I->p ("%s gives you back half what you paid.", THE (doctor));
        }
        hero->gainMoney (price / 2);
        doctor->loseMoney (price / 2);
        *flag = false;
    }
}

void
shHero::payDoctor (shMonster *doctor)
{
    static const char *MedicalProcedureNames[] =
    { "Wound Treatment",
      "Restoration Treatment",
      "Intestinal Examination",
      "Systems Diagnostics",
      "Radiation Purge",
      "Caesarean Section",
      "Canister Amputation",
      "Teef Extraction",
      "Tail Amputation"
    };

    static bool hadamputation = false;
    char buf[200];
    shMenu *menu = I->newMenu ("Medical Services Menu", 0);
    int serv;
    const int TREATMENT_COST = 200;

    if (tryToTranslate (doctor)) {
        /* make all services known */
        for (int i = 0; i < kMedMaxService; ++i) {
            MedicalProcedureData[i].mNameKnown = 1;
        }
    }

    char letter = 'a';
    for (int i = 0; i < kMedMaxService; ++i) {
        serv = doctor->mDoctor.mPermute[i];

        /* try not to offer unneeded services */
        switch (serv) {
        case kMedHealing:
            if (Hero.mHP != Hero.mMaxHP or
                Hero.is (kViolated) or
                Hero.is (kConfused) or
                Hero.is (kStunned))
            {
                break;
            }
            continue;
        case kMedRestoration: {
            int add = 0;
            FOR_ALL_ABILITIES (j) {
                int abil = Hero.mAbil.getByIndex (j);
                if (kPsi == j) {
                    abil += Hero.mPsiDrain;
                }
                if (abil < Hero.mMaxAbil.getByIndex (j)) {
                    add = 1;
                    break;
                }
            }
            if (add) break;
            continue;
        }
        case kMedRectalExam:
        case kMedDiagnostics:
        case kMedRadPurification:
            /* these services are always available: */
            break;
        case kMedCaesareanSection:
            if (Hero.getStoryFlag ("impregnation"))
                break;
            continue;
        case kMedCanisterAmputation:
            if (Hero.getStoryFlag ("superglued tongue"))
                break;
            continue;
        case kMedTeefExtraction:
            if (!Hero.getStoryFlag ("lost teef") and Hero.isOrc ())
                break;
            continue;
        case kMedTailAmputation:
            if (Hero.mIlkId == kMonXelNaga and !Hero.getStoryFlag ("lost tail"))
                break;
            continue;
        default:
            continue;
        }

        snprintf (buf, sizeof(buf), "%s ($%d)",
                  MedicalProcedureData[serv].mNameKnown
                      ? MedicalProcedureNames[serv]
                      : MedicalProcedureData[serv].mDesc,
                 TREATMENT_COST);
        menu->addIntItem (letter++, buf, serv);
    }

    int res;
    if (!menu->getIntResult (&res))
        return; /* nothing picked */
    delete menu;

    MedicalProcedures choice = MedicalProcedures (res);
    int price = TREATMENT_COST;

    if (countMoney () < price) {
        I->p ("You don't have enough money for that.");
        return;
    }

    if (Hero.is (kFrightened) and choice == kMedTeefExtraction and
        MedicalProcedureData[choice].mNameKnown)
    {
        I->p ("No way!  You are too afraid of the dentist at the moment!");
        return;
    }

    loseMoney (price);
    doctor->gainMoney (price);

    const char *who = doctor->the ();
    shObjectIlk *ilk;

    switch (choice) {
    case kMedHealing:
        ilk = &AllIlks[kObjHealingRayGun];
        I->p ("%s injects you with a %s serum!", who,
            !Hero.isBlind () ? ilk->getRayGunColor () : "kind of");
        if (Hero.healing (NDX (10, 5), 0) and !Hero.isBlind ()) {
            ilk->mFlags |= kIdentified;
            MedicalProcedureData[choice].mNameKnown = 1;
        }
        break;
    case kMedRestoration:
        ilk = &AllIlks[kObjRestorationRayGun];
        I->p ("%s injects you with a %s serum!", who,
            !Hero.isBlind () ? ilk->getRayGunColor () : "kind of");
        if (Hero.restoration (NDX (3, 2))) {
            MedicalProcedureData[choice].mNameKnown = 1;
            if (!Hero.isBlind ())  ilk->mFlags |= kIdentified;
        }
        break;
    case kMedRectalExam:
    {
        int probed = 0;
        /* Was loser option pre PRIME 1.7 but why not make it useful
           in a border case? */
        MedicalProcedureData[choice].mNameKnown = 1;
        I->p ("%s probes you!", who);
        /* Any resistance is treated like obstacle. */
        if (!Hero.mResistances[kViolating]) {
            Hero.sufferDamage (kAttProbe);
            probed = 1;
        } else { /* If hero wears loser anorak he is immune. */
            I->p ("You seem unaffected.");
            Hero.mCloak->setKnown ();
            if (Hero.tryToTranslate (doctor)) {
                I->p ("\"I cannot perform the service while you wear %s.\"",
                    THE (Hero.mCloak));
                Hero.mCloak->setKnown ();
                /* Get rid of buggy loser anorak for 200 BZ. */
                if (I->yn ("\"May I remove it?\"")) {
                    I->p ("%s removes %s.", who, YOUR (Hero.mCloak));
                    Hero.doff (Hero.mCloak);
                    I->p ("%s probes you again!", who);
                    Hero.sufferDamage (kAttProbe);
                    probed = 1;
                } else {
                    I->p ("Money-back guarantee requires me to refuse payment.");
                    gainMoney (price);
                    doctor->loseMoney (price);
                }
            } else {
                I->p ("%s chirps something and gives back your money.", who);
                gainMoney (price);
                doctor->loseMoney (price);
            }
            if (probed and Hero.getStoryFlag ("impregnation")) {
                I->p ("\"You have a parasite.\"");
            }   /* Thank you, Captain Obvious! */
        }
        break;
    }
    case kMedDiagnostics:
        MedicalProcedureData[choice].mNameKnown = 1;
        I->p ("%s probes you!", who);
        Hero.doDiagnostics (0);
        break;
    case kMedRadPurification:
        MedicalProcedureData[choice].mNameKnown = 1;
        I->p ("%s injects you with a %s serum!", who,
            !Hero.isBlind () ? "bubbly" : "kind of");
        Hero.mRad = maxi (0, Hero.mRad - RNG (50, 200));
        if (!Hero.mRad)
            I->p ("You feel purified.");
        else
            I->p ("You feel less contaminated.");
        break;
    case kMedCaesareanSection:
    {
        if (!Hero.getStoryFlag ("impregnation"))
            break;

        MedicalProcedureData[choice].mNameKnown = 1;
        Hero.setStoryFlag ("impregnation", 0);

        int x = Hero.mX;
        int y = Hero.mY;
        int queen = !RNG (0, 17);
        int colicky = !RNG (0, 5);
        shMonster *baby = new shMonster (queen ? kMonAlienPrincess : kMonChestburster);
        if (!colicky) {
            baby->mDisposition = shMonster::kIndifferent;
        }
        Level->findNearbyUnoccupiedSquare (&x, &y);
        if (!baby) {
            I->p ("Unfortunately, your baby was stillborn.");
        } else {
            I->p ("It's a %s!", queen ? "girl" : "boy");
            if (Level->putCreature (baby, x, y)) {
                /* FIXME: something went wrong */
            } else {
                I->drawScreen ();
            }
        }

        I->p ("You lose a lot of blood during the operation.");
        if (Hero.sufferDamage (kAttCaesareanSection)) {
            Hero.shCreature::die (kKilled, "complications in childbirth");
        }
        break;
    }
    case kMedCanisterAmputation:
    {
        if (!Hero.getStoryFlag ("superglued tongue"))
            break;

        MedicalProcedureData[choice].mNameKnown = 1;
        I->p ("%s cuts away the glued canister.", who);
        Hero.resetStoryFlag ("superglued tongue");
        amputationDiscount (doctor, this, &hadamputation, price);
    }
    case kMedTeefExtraction:
    {
        if (Hero.getStoryFlag ("lost teef"))
            break;

        int sum = NDX (11, 100);
        Hero.setStoryFlag ("lost teef", sum);
        MedicalProcedureData[choice].mNameKnown = 1; /* Kind of pointless. */
        if (Hero.is (kFrightened)) {
            char *buf = GetBuf ();
            strcpy (buf, who);
            buf[0] = toupper (buf[0]);
            I->p ("Oh no!!  %s gets at your teef!", buf);
            I->p ("You panic and try to flee but %s has immobilized you already.", who);
        } else
            I->p ("%s extracts your teef.", who);
        I->p ("Your magnificent teef are worth $%d buckazoids.", sum);
        gainMoney (sum);
        break;
    }
    case kMedTailAmputation:
    {
        if (Hero.getStoryFlag ("lost tail"))
            break;

        Hero.setStoryFlag ("lost tail", 1);
        MedicalProcedureData[choice].mNameKnown = 1;
        I->p ("%s amputates your tail!", who);

        Hero.amputateTail ();

        amputationDiscount (doctor, this, &hadamputation, price);
    }
    case kMedMaxService:
        break;
    }
    I->drawSideWin ();
}

/* doctor strategy:
     wait around for hero to request services;
     occasionally advertise

   returns ms elapsed, -2 if the monster dies
*/
int
shMonster::doDoctor ()
{
    int elapsed;
    int res = -1;
    int retry = 3;

    while (-1 == res) {
        if (!retry--) {
            return 200;
        }

        switch (mTactic) {

        case kNewEnemy:
            mStrategy = kWander;
            mTactic = kNewEnemy;
            return doWander ();
        case kMoveTo:
            res = doMoveTo ();
            continue;
        case kReady:
            if (Level->getRoomID (mX, mY) != mDoctor.mRoomID) {
                /* somehow, we're not in our hospital! */
                mDestX = mDoctor.mHomeX;
                mDestY = mDoctor.mHomeY;

                if (setupPath ()) {
                    mTactic = kMoveTo;
                    continue;
                } else {
                    return 2000;
                }
            }

            if (canSee (&Hero) and
                mDoctor.mRoomID == Level->getRoomID (Hero.mX, Hero.mY))
            {
                if (0 == RNG (12)) {
                    /* move to an empty square near the home square */
                    mDestX = mDoctor.mHomeX;
                    mDestY = mDoctor.mHomeY;
                    if (!mLevel ->
                        findAdjacentUnoccupiedSquare (&mDestX, &mDestY))
                    {
                        elapsed = doQuickMoveTo ();
                        if (-1 == elapsed) elapsed = 800;
                        return elapsed;
                    }
                    return RNG (300, 1000); /* nowhere to go, let's wait... */
                } else if (0 == RNG (50)) {
                    const char *quips[] = {
                        "\"Dammit, %s! I'm a docbot, not %s!\"",
                        "\"Your second amputation is half off!\"",
                        "\"Galaxy-class health care services!\"",
                        "\"I've been through much experience, you're dealing with an expert!\""
                    };
                    const unsigned int NUMQUIPS = sizeof (quips) / sizeof (char *);
                    const char *nouns[] = {
                        "a magician",
                        "a psychiatrist",
                        "a bartender",
                        "a bricklayer",
                        "a mechanic",
                        "a priest",
                        "a lawyer",
                        "a prostitute",
                        "a toaster oven",
                        "a warbot"
                    };
                    const unsigned int NUMNOUNS = sizeof (nouns) / sizeof (char *);
                    if (Hero.tryToTranslate (this)) {
                        I->p (quips[RNG (NUMQUIPS)],
                              Hero.mName, nouns[RNG (NUMNOUNS)]);
                        Hero.interrupt ();
                    }
                    return RNG (500, 1000);
                }
                return RNG (500, 1000);
            } else {
                return RNG (800, 1600);
            }
        case kFight:
        case kShoot:
            mTactic = kReady;
            debug.log ("Unexpected doctor tactic!");
        }
    }

    return RNG (300, 1000); /* keep on lurking */
}


void
pacifyMelnorme (shMonster *trader)
{
    trader->mStrategy = shMonster::kMelnorme;
    trader->mTactic = shMonster::kReady;
    trader->mDisposition = shMonster::kIndifferent;
    if (Level->getRoomID (Hero.mX, Hero.mY) ==
        Level->getRoomID (trader->mX, trader->mY))
    {
        trader->mGlyph.mColor = kMagenta;
        trader->mGlyph.mTileX = 3;
    } else {
        trader->mGlyph.mColor = kGreen;
        trader->mGlyph.mTileX = 2;
    }
    I->drawScreen ();
}

const char *MelnormeServiceNames[] =
{ "Reveal object bugginess",
  "Reveal object charges",
  "Reveal object enhancement",
  "Describe object appearance",
  "Buy knowledge"
};

void
shHero::payMelnorme (shMonster *trader)
{
    if (trader->isHostile ()) {
        int result;
        shMenu *amends = I->newMenu ("Make amends:", 0);
        amends->addIntItem ('a', "Apologize", 1, 1);
        amends->addIntItem ('p', "Pay reparations", 2, 1);
        amends->getIntResult (&result);
        delete amends;
        switch (result) {
            case -1:
                return;
            case 1:
                if (trader->mHP == trader->mMaxHP) {
                    if (tryToTranslate (trader)) {
                        I->p ("\"We believe you.\"");
                    } else {
                        I->p ("%s mumbles soothingly.", THE (trader));
                    }
                    pacifyMelnorme (trader);
                } else {
                    if (tryToTranslate (trader)) {
                        I->p ("\"It is not enough.\"");
                    } else {
                        I->p ("%s mumbles stalwartly.", THE (trader));
                    }
                }
                return;
            case 2:
            {
                int demand = (trader->mMaxHP - trader->mHP) * 10;
                demand = maxi (50, demand);
                if (demand <= countMoney ()) {
                    if (I->yn ("Pay %s %d buckazoids?", THE (trader), demand)) {
                        I->p ("%s accepts the money.", THE (trader));
                        pacifyMelnorme (trader);
                        loseMoney (demand);
                        trader->gainMoney (demand);
                    }
                } else {
                    I->p ("%s demands %d buckazoids but you don't"
                          " have that much.", THE (trader), demand);
                }
            }
        }
        return;
    }
    static int (shObject::*tests[])(void) =
    {
        &shObject::isBugginessKnown,
        &shObject::isChargeKnown,
        &shObject::isEnhancementKnown,
        &shObject::isAppearanceKnown
    };
    static void (shObject::*actions[])(void) =
    {
        &shObject::setBugginessKnown,
        &shObject::setChargeKnown,
        &shObject::setEnhancementKnown,
        &shObject::setAppearanceKnown
    };
    int (shObject::*test) (void);
    void (shObject::*action) (void);
    reorganizeInventory ();
    shMenu *menu = I->newMenu ("Melnorme Services Menu", 0);
    char buf[200];
    shObjectVector v;
    int price = 1000, services = 0;
    int price1 = 40 - Hero.mAbil.mPsi; /* Bugginess. */
    int price2 = 225 - 5 * Hero.mAbil.mPsi; /* Knowledge. */
    int price3 = price1 / 2; /* Remaining. */
    if (!trader->is (kGenerous)) {
        /* Ward against drinking a lot of nano cola. */
        price1 = maxi (10, price1);
        price2 = maxi (100, price2);
        price3 = maxi (10, price3);
    } else {
        price1 = 10;
        price2 = 100;
        price3 = 10;
    }

    if (tryToTranslate (trader)) {
        /* Make all services known. */
        for (int i = 0; i < kMelnMaxService; ++i) {
            MelnormeServiceData[i].mNameKnown = 1;
        }
    }

    char letter = 'a';
    for (int i = 0; i < kMelnMaxService; ++i) {
        int serv = trader->mMelnorme.mPermute[i];

        /* Offer only eligible services. */
        if (serv == kMelnRandomIdentify) {
            if (trader->mMelnorme.mKnowledgeExhausted or
                !hasTranslation ())
                continue;
            price = price2;
            ++services;
        } else if (serv >= kMelnRevealBugginess and
                   serv <= kMelnRevealAppearance)
        {
            test = tests[serv - kMelnRevealBugginess];
            v.reset ();
            unselectObjectsByFunction (&v, Hero.mInventory, test);
            if (!v.count ()) continue;
            if (serv == kMelnRevealBugginess) {
                price = price1;
            } else {
                price = price3;
            }
            ++services;
        }
        snprintf (buf, sizeof (buf), "%s ($%d)",
                  MelnormeServiceData[serv].mNameKnown
                      ? MelnormeServiceNames[serv]
                      : MelnormeServiceData[serv].mDesc,
                  price);
        menu->addIntItem (letter++, buf, serv);
    }

    if (!services) {
        if (tryToTranslate (trader)) {
            I->p ("Unfortunately I cannot help you at the moment.");
        } else {
            I->p ("%s mumbles something in sad voice.", THE (trader));
        }
        return;
    }

    int choice;
    if (!menu->getIntResult (&choice))
        return;
    delete menu;

    price = choice == kMelnRandomIdentify ? price2 :
            choice == kMelnRevealBugginess ? price1 : price3;
    if (countMoney () < price) {
        I->p ("You don't have enough money for that.");
        return;
    }
    loseMoney (price);
    trader->gainMoney (price);

    shObject *obj = NULL;
    if (choice == kMelnRandomIdentify) {
        int n = kObjNumIlks;
        int tries = 10;
        int success = 0;
        while (tries--) {
            int i = RNG (n);
            shObjectIlk *ilk = &AllIlks[i];
            if (!(ilk->mFlags & kIdentified) and ilk->mProbability != ABSTRACT) {
                char *buf = GetBuf ();
                strncpy (buf, ilk->mReal.mName, SHBUFLEN);
                makePlural (buf, SHBUFLEN);
                ilk->mFlags |= kIdentified;
                I->p ("%s tells you how to recognize %s.", THE (trader), buf);
                success = 1;
                break;
            }
        }
        if (!success) {
            I->p ("%s offers you several facts but you know all of them.", THE (trader));
            I->p ("\"It seems you know all I do.\"");
            trader->mMelnorme.mKnowledgeExhausted = 1;
            I->p ("%s returns your money.");
            trader->loseMoney (price);
            gainMoney (price);
        }
        MelnormeServiceData[choice].mNameKnown = 1;
    } else if (choice >= kMelnRevealBugginess and
               choice <= kMelnRevealAppearance)
    {
        v.reset ();
        test = tests[choice - kMelnRevealBugginess];
        unselectObjectsByFunction (&v, Hero.mInventory, test);
        obj = quickPickItem (&v, "perform service on", 0);
        if (obj) {
            if (hasTranslation () or RNG (4)) { /* 75% - lets be generous. */
                action = actions[choice - kMelnRevealBugginess];
                (obj->*action) ();
                if (!hasTranslation ()) {
                    I->p ("You manage to understand %s.", THE (trader));
                }
                MelnormeServiceData[choice].mNameKnown = 1;
                I->p ("%c - %s", obj->mLetter, obj->inv ());
            } else {
                I->p ("This time you fail to understand %s.", THE (trader));
            }
        }
    }
    I->drawSideWin ();
}

/* Melnorme strategy:
      - Wander around continuously until hero comes near
      - When hero is in room stop and turn purple
      - When hero left the room start to wander and turn green

   Returns ms elapsed, -2 if the monster dies.
*/
int
shMonster::doMelnorme ()
{
    int elapsed;
    int result = -1;
    int retries = 3;
    int myRoom = Level->getRoomID (mX, mY);

    while (-1 == result) {
        if (!retries--) {
            return 200;
        }

        /* Melnorme have no designated shop. They are willing to trade
           if you enter their room. This also allows them to relocate. */
        if (Level->getRoomID (Hero.mX, Hero.mY) == myRoom and
            Level->getRoomID (mX, mY) != 0)
        {
            if (mGlyph.mColor == kGreen and !isHostile ()) {
                /* Greet hero who just entered the room. */
                if (Hero.tryToTranslate (this)) {
                    char *buf = GetBuf ();
                    int rnd;
                    switch (RNG (3)) {
                    case 0: /* hello, heroname/title/racename */
                        rnd = RNG (4);
                        strncpy (buf, rnd == 0 ? Hero.mName :
                                      rnd == 1 ? Hero.mProfession->mName :
                                      rnd == 2 ? Hero.getTitle () :
                                                 Hero.myIlk ()->mName, SHBUFLEN);
                        if (rnd == 0) buf[0] = toupper (buf[0]);
                        break;
                    case 1: /* hello, heroname the Janitor */
                        snprintf (buf, SHBUFLEN, "%s the %s", Hero.mName,
                                  !RNG (3) ? Hero.mProfession->mName :
                                  !RNG (2) ? Hero.getTitle () :
                                             Hero.myIlk ()->mName);
                        buf[0] = toupper (buf[0]);
                        break;
                    case 2: /* hello, earthling Detective */
                        snprintf (buf, SHBUFLEN, "%s %s", Hero.myIlk ()->mName,
                                  !RNG (2) ? Hero.mProfession->mName :
                                             Hero.getTitle ());
                        break;
                    }

                    switch (RNG (4)) {
                    case 0:
                        I->p ("I bid you a formal welcome, %s.", buf); break;
                    case 1:
                        I->p ("Hello, %s.", buf); break;
                    case 2:
                        I->p ("How nice to see you, %s.", buf); break;
                    case 3: /* Use just the capitalized name because
                               greetings could be too long otherwise. */
                        strncpy (buf, Hero.mName, SHBUFLEN);
                        buf[0] = toupper (buf[0]);
                        I->p ("I had itchy pods recently, %s"
                              ", and here you are!", buf);
                        break;
                    }
                } else {
                    I->p ("%s mumbles something in coarse voice.", THE (this));
                }
            }
            mGlyph.mColor = kMagenta;
            mGlyph.mTileX = 3;

            /* Hero was greeted properly. Is this first encounter? */
            if (!Hero.getStoryFlag ("Met Melnorme")) {
                Hero.setStoryFlag ("Met Melnorme", 1);
                I->p ("Press '%s' to request services.",
                    I->getKeyForCommand (shInterface::kPay));
            }
        } else {
            mGlyph.mColor = kGreen;
            mGlyph.mTileX = 2;
        }

        switch (mTactic) {

        case kNewEnemy:
            mStrategy = kWander;
            mTactic = kNewEnemy;
            mGlyph.mColor = kNavy;
            mGlyph.mTileX = 4;
            return doWander ();
        case kMoveTo:
            result = doMoveTo ();
            continue;
        case kReady:
            /* Usually stay where you are but occassionally
               move to new square inside this room. */
            if (myRoom and !RNG (6)) {
                mDestX = RNG (mMelnorme.mSX, mMelnorme.mEX);
                mDestY = RNG (mMelnorme.mSY, mMelnorme.mEY);
                if (!mLevel->isOccupied (mDestX, mDestY)) {
                    elapsed = doQuickMoveTo ();
                    if (-1 == elapsed) elapsed = 800;
                    return elapsed;
                }
                return RNG (600, 1000); /* Path blocked. Wait. */
                /* !myRoom == Warp out of corridors eagerly. */
            } else if ((!myRoom or !RNG (40)) and
                       mGlyph.mColor == kGreen)
            {
                int newx = mX, newy = mY, tries = 3;
                while (tries--) {
                    mLevel->findUnoccupiedSquare (&newx, &newy);
                    if (!mLevel->isOccupied (newx, newy) and
                        !mLevel->isInShop (newx, newy) and
                        /* Do not accept corridors. */
                        (myRoom = Level->getRoomID (newx, newy)))
                    {
                        transport (newx, newy, 100, 1);
                        /* Update location data. */
                        Level->getRoomDimensions (myRoom,
                            &mMelnorme.mSX, &mMelnorme.mSY,
                            &mMelnorme.mEX, &mMelnorme.mEY);
                        /* Exclude walls from room coordinates. */
                        ++mMelnorme.mSX; ++mMelnorme.mSY;
                        --mMelnorme.mEX; --mMelnorme.mEY;
                        return FULLTURN;
                    }
                }
                return FULLTURN;
            } else {
                return FULLTURN;
            }
        default:
            mTactic = kReady;
            debug.log ("Unexpected Melnorme tactic!");
        }
    }

    return FULLTURN;
}
