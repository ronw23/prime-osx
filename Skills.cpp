/*******************************

Skill Management Code

each character has access to a variety of skills

********************************/

#include "Global.h"
#include "Util.h"
#include "Object.h"
#include "Hero.h"
#include "Interface.h"

const char *
shSkill::getName ()
{
    switch (mCode) {
    /* Combat */
    case kGrenade:       return "Thrown Weapons";
    case kHandgun:       return "Handguns";
    case kLightGun:      return "Light Guns";
    case kHeavyGun:      return "Heavy Guns";
    case kUnarmedCombat: return "Unarmed Combat";
    case kMeleeWeapon:   return "Basic Melee Weapons";
    case kSword:         return "Swords";
    /* Adventuring */
    case kConcentration: return "Concentration";
    case kOpenLock:      return "Pick Locks";
    case kRepair:        return "Repair";
    case kSearch:        return "Search";
    case kHacking:       return "Programming";
    case kSpot:          return "Spot";

    case kMutantPower:
        return getMutantPowerName (mPower);

    default:
        return "Unknown!";
    }
}


#if 0
static int
compareSkills (shSkill **a, shSkill **b)
{
    shSkill *s1 = * (shSkill **) a;
    shSkill *s2 = * (shSkill **) b;

    if (s1->mCode < s2->mCode) {
        return -1;
    } else if (s1->mCode > s2->mCode) {
        return 1;
    } else if (s1->mPower < s2->mPower) {
        return -1;
    } else {
        return 1;
    }
}
#endif


void
shSkill::getDesc (char *buf, int len)
{
    const char *abilname = NULL;

    switch (kSkillAbilityMask & mCode) {
    case kStrSkill: abilname = "Str"; break;
    case kConSkill: abilname = "Con"; break;
    case kAgiSkill: abilname = "Agi"; break;
    case kDexSkill: abilname = "Dex"; break;
    case kIntSkill: abilname = "Int"; break;
    case kPsiSkill: abilname = "Psi"; break;
    }

    int abil = Hero.mAbil.getByIndex (SKILL_KEY_ABILITY (mCode));
    int mod = mBonus + ABILITY_MODIFIER (abil);
    int max = mAccess*(Hero.mCLevel+1)/4;
    snprintf (buf, len, "%-20s  %s   %2d  %+3d  %2d    ",
              getName (), abilname, mRanks, mod, max);
}


static const char *
prepareHeader (const char *hdr, bool choice)
{
    char *buf = GetBuf ();
    snprintf (buf, SHBUFLEN, "%s  %-16s  Abil. Rank Mod Max",
              choice ? "Pick    " : "", hdr);
    return buf;
}

void
shHero::editSkills ()
{
    char prompt[50];
    int i;
    int flags = shMenu::kSelectIsPlusOne | shMenu::kCountAllowed | shMenu::kShowCount;
    int navail = 0;
    char buf[70];
    shSkill *skill;
    shSkillCode lastcode = kWeaponSkill;

//    mSkills.sort (&compareSkills);

    do {
        if (mSkillPoints > 1) {
            snprintf (prompt, 50, "You may make %d skill advancements:",
                      mSkillPoints);
            flags |= shMenu::kMultiPick;
        } else if (1 == mSkillPoints) {
            snprintf (prompt, 50, "You may advance a skill:");
            flags |= shMenu::kMultiPick;
        } else {
            snprintf (prompt, 50, "Skills");
            flags |= shMenu::kNoPick;
        }

        shMenu *menu = I->newMenu (prompt, flags);
        menu->attachHelp ("skills.txt");
        char letter = 'a';

        menu->addHeader (prepareHeader ("Combat", mSkillPoints));
        for (i = 0; i < mSkills.count (); i++) {
            skill = mSkills.get (i);
            skill->getDesc (buf, 70);

            if (kMutantPower == skill->mCode and !mMutantPowers[skill->mPower])
                continue; /* can't advance skills til you learn the power */

            if (kWeaponSkill & lastcode and !(kWeaponSkill & skill->mCode)) {
                menu->addHeader (prepareHeader ("Adventuring", mSkillPoints));
            } else if (kMutantPower != lastcode and
                       kMutantPower == skill->mCode)
            {
                menu->addHeader (prepareHeader ("Mutant Powers", mSkillPoints));
            }

            lastcode = skill->mCode;

            int max = skill->mAccess*(mCLevel+1)/4;
            if (mSkillPoints and
                max > skill->mRanks)
            {
                menu->addPtrItem (letter++, buf, skill,
                    mini (mSkillPoints, max - skill->mRanks));
                navail++;
                if (letter > 'z') letter = 'A';
            } else {
                menu->addPtrItem (0, buf, skill);
            }
        }
        if (mSkillPoints and !navail) {
            delete menu;
            break;
        }
        int advanced = 0;
        do {
            int count;
            i = menu->getPtrResult ((const void **) &skill, &count);
            if (skill) {
                if (count > mSkillPoints) count = mSkillPoints;
                mSkillPoints -= count;
                skill->mRanks += count;
                advanced++;
            }
        } while (i and mSkillPoints);
        delete menu;
        if (!advanced)
            break;
        computeSkills ();
        computeIntrinsics (); /* For improving Haste for example. */
        I->drawSideWin ();
    } while (mSkillPoints);
    /* Do computation again to be sure. */
    // TODO: figure out if this paranoia is really needed.
    computeSkills ();
    computeIntrinsics ();
}
