#include "Interface.h"
#include "Hero.h"
#include "Object.h"

int
shObject::execTime (void)
{   /* Optimized computers execute programs faster. */
    return isOptimized () ? FULLTURN : LONGTURN;
}

int
shObject::hackingModifier (void)
{
    return hasSystem () ? mEnhancement : 0;
}

/* Hardware must be supported by software. */
bool
shObject::supportsInterface (shCompInterface interface)
{
    switch (interface)
    {
    case kIVisualOnly: /* Taken for granted. */
        return true;
    case kIVisualVoice:
        return mEnhancement > kAbysmalSys;
    case kIUniversal:
        return mEnhancement > kPoorSys;
    default:
        return false;
    }
}

bool
shObject::hasSystem (void)
{
    if (!isComputer ()) {
        return false;
    }
    if (mEnhancement != kNoSys)
        return true;
    else {
        if (!Hero.isBlind ()) {
            I->p ("DISK BOOT FAILURE.  INSERT SYSTEM DISK AND PRESS ENTER.");
            setEnhancementKnown ();
            I->pause ();
            I->p ("Your computer lights floppy drive LEDs for a moment.");
        } else
            I->p ("You hear floppy drives seeking briefly.");
        return false;
    }
}

bool
shObject::systemRunPOST (void)
{
    if (mEnhancement >= kGoodSys) {
        if (!isBugginessKnown ()) {
            if (isBuggy ())
                I->p ("Warning! POST detected hardware malfunction.");
            else if (isOptimized ())
                I->p ("POST running... high performance settings active.");
            else
                I->p ("POST running... all tests passed.");
            setBugginessKnown ();
            if (isBuggy ())
                return I->yn ("Cancel execution?");
        }
    }
    return true;
}

/* Returns true if system check passed and false otherwise. */
bool
shObject::systemCheckDisk (shObject *disk)
{
    int sys = mEnhancement;
    if (sys >= kGoodSys and !disk->isKnown () and
        (disk->isA (kObjSpamDisk) or disk->isA (kObjHypnosisDisk)))
    {
        I->p ("Program has been recognized as fraudulent software.");
        disk->setKnown ();
        disk->announce ();
        int ans = I->yn ("Run anyway?");
        if (ans == 0) return false;
    }

    /* Does not tell debugged from optimized.  That would be too good. */
    if (sys >= kExcellentSys and
        !disk->isBugginessKnown () and disk->isBuggy ())
    {
        I->p ("Software has not passed reliability tests.");
        disk->setBugginessKnown ();
        disk->announce ();
        int ans = I->yn ("Continue?");
        if (ans == 0) return false;
    }

    /* This one does. */
    if (sys >= kSuperbSys and !disk->isBugginessKnown ())
    {
        disk->setBugginessKnown ();
        disk->announce ();
        int ans = I->yn ("Continue?");
        if (ans == 0) return false;
    }

    /* Antivirus in computer can recover infected floppies. */
    if (isFooproof () and disk->isInfected ()) {
        I->p ("Antivirus alert: trojan Zap'M-OverwriteDisk found.");
        disk->setInfectedKnown ();
        if (disk->isA (kObjComputerVirus)) {
            disk->setKnown ();
        }
        disk->announce ();
        if (I->yn ("Recover floppy disk?", disk->mCount > 1 ? "s" : "")) {
            disk = Hero.removeOneObjectFromInventory (disk);
            disk->resetInfected ();
            if (disk->isA (kObjComputerVirus)) {
                disk->mIlkId = kObjBlankDisk;
            }
            Hero.addObjectToInventory (disk, 0);
            return 0; /* Disinfection should take time. */
        } else { /* No point in disallowing that. */
            int ans = I->yn ("Run anyway? (computer will not get infected)");
            if (ans == 0) return false;
        }
    } else if (isFooproof () and isFooproofKnown ()) {
        /* You know the computer is protected by antivirus?
           So you know the floppy disk has been scanned. */
        disk->setInfectedKnown (); /* Mark clean. */
    }

    return true; /* Nothing suspicious or warnings ignored. */
}

/* In each five minutes system will reserve 30 seconds for updates. */
#define INTERVAL (300 * FULLTURN)
#define UPDATES 30
static int
updatesToDo (shObject *obj)
{
    long int time_phase = (Clock                % INTERVAL) / FULLTURN;
    long int comp_phase = (obj->mLastEnergyBill % INTERVAL) / FULLTURN;
    int updates = time_phase + UPDATES - comp_phase;
    if (updates > UPDATES or updates <= 0)
        return 0;
    return updates;
}

void
shObject::kickComputer (void)
{   /* Kick it into future somewhat. */
    mLastEnergyBill += RNG (100, 200) * FULLTURN;
    resetToggled (); /* Break recursive loop. */
}

/* Returns true OS has decided to troll you right now. */
bool
shObject::isUpdateTime ()
{ /* Good system does not lock up your computer during updates. */
    if (!isComputer ())  return false;
    if (mEnhancement <= kAverageSys and updatesToDo (this)) {
        return true;
    }
    return false;
}

void
shObject::systemUpdateMessage (void)
{   /* Hint!  Hint!  X-D */
    I->p ("Do not turn off or kick your computer.  Loading update %d of %d.",
        updatesToDo (this), UPDATES);
}
#undef INTERVAL
#undef UPDATES
