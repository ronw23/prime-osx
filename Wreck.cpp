#include "Global.h"
#include "Object.h"
#include "ObjectType.h"
#include "Monster.h"

shObject *
createWreck (shCreature *m)
{
    if (m->isRobot ()) {
        shObject *wreck = new shObject ();

        wreck->mIlkId = kObjWreck;
        wreck->mCount = 1;
        wreck->mHP = wreck->myIlk ()->mHP;
        wreck->mWreckIlk = m->mIlkId;
        return wreck;
    }
    return NULL;
}
