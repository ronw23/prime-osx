#ifndef MUTANT_H
#define MUTANT_H

struct MutantPower {
    int mLevel;
    char mLetter;
    const char *mName;
    int (*mFunc) (shHero *);
    int (*mOffFunc) (shHero *);

    int isPersistant () { return NULL != mOffFunc ? 1 : 0; }
};

extern MutantPower MutantPowers[kMaxAllPower];
#endif
