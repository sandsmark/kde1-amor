//---------------------------------------------------------------------------
//
// amorconfig.cpp
//
// Copyright (c) 1999 Martin R. Jones <mjones@kde.org>
//

#include <kapp.h>
#include "amorconfig.h"

//---------------------------------------------------------------------------
//
// Constructor
//
AmorConfig::AmorConfig()
{
    mOnTop = false;
    mOffset = 0;
    mTheme = "blobrc";
    mTips = false;
}

//---------------------------------------------------------------------------
//
// Read the configuration
//
void AmorConfig::read()
{
    KConfig *config = kapp->getConfig();

    mOnTop = config->readBoolEntry("OnTop");
    mOffset = config->readNumEntry("Offset");
    mTheme = config->readEntry("Theme", "blobrc");
    mTips  = config->readBoolEntry("Tips");
}

//---------------------------------------------------------------------------
//
// Write the configuration
//
void AmorConfig::write()
{
    KConfig *config = kapp->getConfig();

    config->writeEntry("OnTop", mOnTop);
    config->writeEntry("Offset", mOffset);
    config->writeEntry("Theme", mTheme);
    config->writeEntry("Tips", mTips);

    config->sync();
}


