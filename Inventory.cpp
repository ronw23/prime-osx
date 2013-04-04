/* Hero inventory functions */

#include "Global.h"
#include "Util.h"
#include "Hero.h"
#include "Creature.h"
#include "Object.h"
#include "Interface.h"
#include <ctype.h>

void
shHero::reorganizeInventory ()
{
    int i;
    int j;
    shObject *obj;
    shObject *obj2;
#ifdef MERGEMESSAGE
    char buf[80];
#endif

    for (i = 0; i < mInventory->count (); i++) {
        obj = mInventory->get (i);
        if (hasBugSensing ()) {
            obj->setBugginessKnown ();
        }
        if (!isBlind ()) {
            obj->setAppearanceKnown ();
        }
        for (j = 0; j < i; j++) {
            obj2 = mInventory->get (j);
            if (obj2->canMerge (obj)) {
                mInventory->remove (obj);
                obj2->merge (obj);
#ifdef MERGEMESSAGE
                I->p ("Merging: %c - %s", obj2->mLetter, obj2->inv ());
#endif
                --i;
            }
        }
    }
    mInventory->sort (&compareObjects);
}


int
shHero::addObjectToInventory (shObject *obj, int quiet /* = 0 */)
{
    int spot;
    char let, spots[52];

    if (hasBugSensing ())  obj->setBugginessKnown ();

    const char *what = AN (obj);

    for (int i = 0; i < 52; spots[i++] = 0)
        ;
    /* Try merging. */
    for (int i = 0; i < mInventory->count (); ++i) {
        shObject *iobj;

        iobj = mInventory->get (i);
        if (iobj->canMerge (obj)) {
            mWeight += obj->getMass ();
            iobj->merge (obj);
            if (0 == quiet) {
                I->p ("%c - %s", iobj->mLetter, what);
            }
            computeIntrinsics (quiet);
            return 1;
        }
        let = iobj->mLetter;
        spot =
            (let >= 'a' and let <= 'z') ? let - 'a' :
            (let >= 'A' and let <= 'Z') ? let - 'A' + 26 :
            -1;

        assert (-1 != spot);
        assert (0 == spots[spot]);
        spots[spot] = 1;
    }
    /* Try to place item in exactly the spot it was before. */
    let = obj->mLetter;
    spot =
        (let >= 'a' and let <= 'z') ? let - 'a' :
        (let >= 'A' and let <= 'Z') ? let - 'A' + 26 :
        -1;
    int i;
    if (-1 != spot and 0 == spots[spot]) {
        i = spot; /* Place item under its old letter. */
    } else { /* Look for free spot. */
        for (i = 0; i < 52 and 0 != spots[i]; ++i)
            ;
    }
    if (i < 52) {
        mInventory->add (obj);
        mWeight += obj->getMass ();
        obj->mLetter = i < 26 ? i + 'a' : i + 'A' - 26;
        obj->mLocation = shObject::kInventory;
        obj->mOwner = this;
        if (!quiet)  I->p ("%c - %s", obj->mLetter, what);
        mInventory->sort (&compareObjects);
        computeIntrinsics (quiet);
        return 1;
    }

    if (!quiet)  I->p ("You don't have any room in your pack for %s.", what);
    return 0;
}


int
selectObjects (shObjectVector *dest, shObjectVector *src, shObjId ilk)
{
    int n = 0;
    for (int i = 0; i < src->count (); ++i) {
        if (src->get (i) -> isA (ilk)) {
            dest->add (src->get (i));
            n++;
        }
    }
    return n;
}

int
selectObjects (shObjectVector *dest, shObjectVector *src,
               shObjectType type)
{
    int n = 0;
    for (int i = 0; i < src->count (); ++i) {
        if ((kMaxObjectType == type) or (src->get (i) -> isA (type))) {
            dest->add (src->get (i));
            n++;
        }
    }
    return n;
}


int
selectObjectsByFlag (shObjectVector *dest, shObjectVector *src, int flag)
{
    int n = 0;
    for (int i = 0; i < src->count (); ++i) {
        if (flag & src->get (i) -> myIlk () -> mFlags) {
            dest->add (src->get (i));
            n++;
        }
    }
    return n;
}


int
selectObjectsByFunction (shObjectVector *dest, shObjectVector *src,
                         int (shObject::*idfunc) (), int neg)
{
    int n = 0;
    for (int i = 0; i < src->count (); ++i) {
        if ((src->get (i)->*idfunc) () ? !neg : neg)  {
            dest->add (src->get (i));
            n++;
        }
    }
    return n;
}

int
unselectObjectsByFunction (shObjectVector *dest, shObjectVector *src,
                           int (shObject::*idfunc) ())
{
    return selectObjectsByFunction (dest, src, idfunc, 1);
}

int
selectObjectsByCallback (shObjectVector *dest, shObjectVector *src,
                         int (*objfunc) (shObject *), int neg)
{
    int n = 0;
    for (int i = 0; i < src->count (); ++i) {
        if ((*objfunc) (src->get (i)) ? !neg : neg)  {
            dest->add (src->get (i));
            n++;
        }
    }
    return n;
}

int
unselectObjectsByCallback (shObjectVector *dest, shObjectVector *src,
                           int (*objfunc) (shObject *))
{
    return selectObjectsByCallback (dest, src, objfunc, 1);
}



int
compare_letters (const void *a, const void *b)
{
    if (*(char*) a < *(char*)b)
        return -1;
    else if (*(char*)a > *(char*)b)
        return 1;
    else
        return 0;
}

// EFFECTS: prompts the user to select an item
//          if type is kMaxObjectType, no type restriction

shObject *
shHero::quickPickItem (shObjectVector *v, const char *action, int flags,
                       int *count /* = NULL */ )
{
    if (0 == v->count () and !(flags & shMenu::kFiltered)) {
        I->p ("You don't have anything to %s.", action);
        return NULL;
    }
    char *actionbuf = GetBuf ();
    strncpy (actionbuf, action, SHBUFLEN);
    actionbuf[0] = toupper (actionbuf[0]);
    if (v->count ()) {
        shMenu *menu = I->newMenu (actionbuf, flags);
        shObject *obj;
        for (int i = 0; i < v->count (); ++i) {
            obj = v->get (i);
            menu->addPtrItem (obj->mLetter, obj->inv (), obj, obj->mCount);
        }
        int res = menu->getPtrResult ((const void **) &obj, count);
        delete menu;
        if (res == DELETE_FILTER_SIGNAL) {
            /* Fall below to choice from whole inventory. */
        } else if (!obj) {
            I->pageLog ();
            I->nevermind ();
            return NULL;
        } else {
            return obj;
        }
    }
    if (flags & shMenu::kFiltered) {
        flags &= ~shMenu::kFiltered;
        shMenu *menu = I->newMenu (actionbuf, flags);
        shObject *obj;
        for (int i = 0; i < mInventory->count (); ++i) {
            obj = mInventory->get (i);
            menu->addPtrItem (obj->mLetter, obj->inv (), obj, obj->mCount);
        }
        menu->getPtrResult ((const void **) &obj, count);
        delete menu;
        if (!obj) {
            I->pageLog ();
            I->nevermind ();
        }
        return obj;
    }
    return NULL;
}
