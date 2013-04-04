#include "MonsterIlk.h"
#include <string.h>

#define STUFF(creature, ...) \
static const char *EQ##creature [] = \
{\
__VA_ARGS__ \
};\
equipment[creature].mPtr = EQ##creature;\
equipment[creature].mCnt = sizeof(EQ##creature) / sizeof(const char *)


struct shEquip {
    const char **mPtr;
    int mCnt;
} equipment[kMonNumberOf];

void
initializeEquipment()
{
    memset (equipment, 0, kMonNumberOf * sizeof (shEquip));

STUFF(kMonLittleGreyAlien,
"anal probe",
"75% reticulan jumpsuit",
"25% reticulan zap gun",
", 50% 3d3 energy cells",
"5% reticulan bio armor"
);

STUFF(kMonMutantHuman,
"1% badass trenchcoat",
"| 1% loser anorak",
"5% canister of Nano-Cola",
"5% torc"
);

STUFF(kMonSpaceGoblin,
"10% pea shooter",
"| 15% club",
"80% 4d8 buckazoids"
);

STUFF(kMonKamikazeGoblin,
"10% pea shooter",
"1d3 grenades",
"80% 5d10 buckazoids"
);

STUFF(kMonRedshirt,
"phaser pistol",
"4d20 energy cells",
"5% pair of sturdy boots",
"5% tricorder"
);

STUFF(kMonSpaceOrc,
"50% pistol",
", 3d10 bullets",
"| 10% shotgun",
", 1d12 shotgun shells",
"| pea shooter",
"60% power claw",
"1% ray gun"
);

STUFF(kMonCylonCenturion,
"laser pistol",
"4d20 energy cells"
);

STUFF(kMonStormtrooper,
"stormtrooper suit",
"stormtrooper helmet",
"pair of stormtrooper boots",
"blaster pistol",
"4d20 energy cells"
);

STUFF(kMonHighPingBastard,
"flak jacket",
"90% shotgun",
", 1d12 shotgun shells",
"| pea shooter",
"1% badass trenchcoat",
"| 3% loser anorak"
);

STUFF(kMonSpaceOrcPyromaniac,
"5% asbestos jumpsuit",
"90% light flamethrower",
", 6d4 fuel drums",
"| flamethrower",
", 10d4 fuel drums",
"50% power claw",
"1% ray gun"
);

STUFF(kMonCreepingCredits,
"4d100 buckazoids"
);

STUFF(kMonMelnorme,
"3d50 buckazoids",
"15% ray gun",
"80% laser rifle",
"| plasma pistol",
"8d10 energy cells"
);

STUFF(kMonCylonCommandCenturion,
"laser rifle",
"6d20 energy cells"
);

STUFF(kMonKlingon,
"flak jacket",
"20% bat'leth",
"| phaser pistol",
", 3d20 energy cells"
);

STUFF(kMonTroubleshooter,
"75% suit of reflec armor",
"75% buggy laser pistol",
"| laser pistol",
"5% 1d2 stun grenades",
"| 5% 1d2 frag grenades",
"| 5% 1d2 concussion grenades",
"| 5% 1d2 rad grenades",
"6d20 energy cells",
"10% buggy ray gun",
"| 5% ray gun",
"2% buggy motion tracker",
"40% canister of Bouncy Bubbly Beverage",
"4% canister of Bouncy Bubbly Beverage"
);

STUFF(kMonCheerleaderNinja,
"10% katana",
"2d3 shurikens"
);

STUFF(kMonBorg,
"phaser pistol",
"3d20 energy cells",
"20% bionic implant"
);

STUFF(kMonSpaceOrcBoss,
"flak jacket",
"30% shotgun",
", 3d12 shotgun shells",
"| 50% laser rifle",
", 4d20 energy cells",
"| plasma rifle",
", 5d20 energy cells",
"10% ray gun",
"power claw"
);

STUFF(kMonKlingonCaptain,
"flak jacket",
"70% bat'leth",
"phaser pistol",
"3d20 energy cells",
"15% ray gun"
);

STUFF(kMonMiGo,
"50% brain cylinder"
);

STUFF(kMonBeneGesserit,
"50% Gom Jabbar",
"30% silver torc",
"| 5% gold torc"
);

STUFF(kMonKlingonCommander,
"80% +2 flak jacket",
"| +4 flak jacket",
"40% +2 acidproof bat'leth",
"| not buggy acidproof bat'leth",
"80% not buggy +3 phaser pistol",
"| not buggy +6 phaser pistol",
"3d20 energy cells",
"25% not buggy ray gun"
);

STUFF(kMonUsenetTroll,
"25% asbestos jumpsuit",
"10% asbestos jacket",
"50% buggy computer",
"| 50% computer",
"60% floppy disk",
"50% floppy disk",
"40% floppy disk",
"30% floppy disk",
"5% cracked floppy disk",
"1% badass trenchcoat",
"| 1% loser anorak"
);

STUFF(kMonLowPingBastard,
"40% +2 kevlar jacket",
"| kevlar jacket",
"shotgun",
", 1d12 shotgun shells",
"90% railgun",
", 1d12 railgun slugs",
"3% badass trenchcoat",
"| 1% loser anorak"
);

STUFF(kMonSpaceElf,
"elven jumpsuit",
"25% elven combat helmet",
"15% elven combat armor",
"10% elven psychic sword",
"| +1 elven dagger",
"laser pistol",
"15% ray gun",
"4d10 energy cells"
);

STUFF(kMonCyborgNinja,
"chameleon suit",
"90% katana",
"| lightsaber",
"25% implant",
"75% 3d2 shurikens",
"10% badass trenchcoat"
);

STUFF(kMonMutantNinjaTurtle,
"50% pair of nunchucks",
"| bo",
"15% ray gun"
);

STUFF(kMonTallGreyAlien,
"anal probe",
"50% 2d10 energy cells",
", reticulan zap gun",
"reticulan jumpsuit",
"25% energy dome",
"| 5% harmony dome",
"5% shield belt"
);

STUFF(kMonBrotherhoodPaladin,
"brotherhood power armor",
"brotherhood power helmet",
"10% ordinary jumpsuit",
"1% jumpsuit",
"5d20 energy cells",
"50% plasma pistol",
", power fist",
"| plasma rifle",
"canister of healing",
"80% canister",
"50% canister of Rad-Away"
);

STUFF(kMonSorceress,
"20% silver torc"
);

STUFF(kMonSpaceElfLord,
"+1 elven dagger",
"elven jumpsuit",
"50% elven space suit",
"| 50% elven exarch armor",
"75% laser rifle",
"| plasma rifle",
"5% lightsaber",
"30% elven psychic sword",
"| katana",
"60% elven combat helmet",
"| elven exarch helmet",
"25% ray gun",
"50% golden torc",
"5d10 energy cells"
);

STUFF(kMonClerkbot,
"20d10 buckazoids",
"1d3 canisters of beer",
"25% shield belt",
", 4d50 energy cells"
);

STUFF(kMonDocbot,
"1d400 buckazoids",
"1d3 canisters of beer",
"50% 1d2 canisters of Rad-Away",
"50% 1d2 canisters of healing",
"50% canister of full healing",
"50% 1d2 canisters of restoration",
"10% canister of gain ability"
);

STUFF(kMonAMarine,
"5% M56 Smartgun",
", 6d10 bullets",
"| 5% sniper rifle",
", 3d5 bullets",
", chainsaw",
"| 20% pulse rifle",
", 6d10 bullets",
"| assault pistol",
", 2d10 bullets",
", chainsaw",
", 5d4 energy cells",
"aquamarine power armor",
"aquamarine power helmet",
"10% canister of healing",
"20% adamantine cloak"
);

STUFF(kMonMGMarine,
"50% pulse rifle",
", 8d10 bullets",
"| assault pistol",
", 2d10 bullets",
", power fist",
"mean green power armor",
"mean green power helmet",
"40% 4d10 energy cells",
"40% canister of healing",
"20% canister of restoration",
"3% adamantine cloak"
);

STUFF(kMonReticulanDissident,
"anal probe",
"reticulan zap gun",
"1d100 energy cells",
"reticulan jumpsuit",
"reticulan bio armor",
"80% bio computer",
"1d3 floppy disks",
"25% energy dome",
"| 5% harmony dome",
"50% shield belt",
", 10d4 energy cells"
);

STUFF(kMonSpaceElfQueen,
"50% optimized elven jumpsuit",
"33% energy dome",
"| elven exarch helmet",
"50% elven space suit",
"| elven exarch armor",
"20% lightsaber",
"| 50% optimized katana",
"| elven psychic sword",
"2d10 energy cells",
"80% golden torc"
);

STUFF(kMonCCMarine,
"15% sword of chaos",
"| 50% plasma rifle",
", 10d10 energy cells",
"| pulse rifle",
", 8d10 bullets",
"40% chainsaw",
"| 75% power fist",
"| power claw",
"chaos power armor",
"40% 5d10 energy cells",
"40% canister of healing",
"30% canister of restoration",
"40% canister of mutagen"
);

STUFF(kMonZealot,
"pair of psi blades",
"protoss power suit",
"20% pair of zealot leg enhancements",
"20% psionic shield generator",
"10% Khaydarin crystal shard"
);

STUFF(kMonHighTemplar,
"15% pair of psi blades",
"energy handgun",
"energy cells",
"50% Khaydarin crystal shard"
);

STUFF(kMonDalek,
"49 energy cells",
"1d101 energy cells"
);

STUFF(kMonWarbot,
"50% laser cannon",
"| plasma cannon",
"300 energy cells",
"20% frag grenades",
"80% ray gun"
);

STUFF(kMonSecuritron,
"auto-cannon",
"50 bullets",
"bullets",
"50% 1 flashbang",
"| 1 stun grenade",
"80% ray gun"
);

STUFF(kMonGuardbot,
"80% laser cannon",
"| plasma cannon",
"200 energy cells",
"50% ray gun"
);

STUFF(kMonBOFH,
"ray gun",
"pea shooter",
"+2 ordinary jumpsuit",
"+1 suit of reflec armor",
"Eye of the BOFH",
"computer",
"cracked floppy disk",
"75% floppy disk",
"75% floppy disk"
);

STUFF(kMonAgent,
"optimized +2 badass trenchcoat",
"optimized sunglasses"
);

STUFF(kMonShodan,
"Bizarro Orgasmatron"
);
}
