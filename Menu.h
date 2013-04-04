#ifndef MENU_H
#define MENU_H

struct shMenuChoice
{
    char mLetter;
    int mCount;           /* count available. -1 indicates this is a header */
    int mSelected;        /* count currently selected */
    char mText[256];
    union {
        const void *mPtr; /* value of the object returned later if this */
        int mInt;         /* choice is selected. */
    } mValue;

    shMenuChoice (char letter, const char *text, const void *value, int count,
                  int selected = 0)
    {
        mLetter = letter;
        strncpy (mText, text, 255); mText[255] = 0;
        mValue.mPtr = value;
        mSelected = selected;
        mCount = count;
    }
    shMenuChoice (char letter, const char *text, int value, int count,
                  int selected = 0)
    {
        mLetter = letter;
        strncpy (mText, text, 255); mText[255] = 0;
        mValue.mInt = value;
        mSelected = selected;
        mCount = count;
    }
};

#endif
