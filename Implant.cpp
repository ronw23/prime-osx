#include "Global.h"
#include "Util.h"
#include "Hero.h"
#include "Object.h"
#include "Interface.h"

static int
useHealthMonitor (shObject *monitor)
{
    if (monitor->isBuggy ()) {
        I->p ("Firmware malfunction.  Unable to gauge radiation level.");
        monitor->setBugginessKnown ();
    } else {
        I->p ("Reported radiation dosage absorbed by your body is %s.",
            Hero.radLevels ());
    }
    return 0;
}

static int
useGloryDevice (shObject *device)
{
    if (I->yn ("Activate the glory device and die a martyr's death?")) {
        Hero.shCreature::die (kSuicide); /* Death routine handles everything. */
    }
    return 0;
}

void
initializeImplants ()
{
    AllIlks[kObjHealthMonitor].mUseFunc = useHealthMonitor;
    AllIlks[kObjGloryDevice].mUseFunc = useGloryDevice;
}


const char *
describeImplantSite (shObjectIlk::Site site)
{
    switch (site) {
    case shObjectIlk::kFrontalLobe: return "frontal lobe";
    case shObjectIlk::kTemporalLobe: return "temporal lobe";
    case shObjectIlk::kParietalLobe: return "parietal lobe";
    case shObjectIlk::kOccipitalLobe: return "occipital lobe";
    case shObjectIlk::kCerebellum: return "cerebellum";
    case shObjectIlk::kLeftEar: return "left ear";
    case shObjectIlk::kRightEyeball: return "right eyeball";
    case shObjectIlk::kNeck: return "neck";
    default:
        return "brain";
    }
}
