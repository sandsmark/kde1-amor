//---------------------------------------------------------------------------
//
// amoranim.h
//
// Copyright (c) 1999 Martin R. Jones <mjones@kde.org>
//

#include <stdlib.h>
#include <kconfig.h>
#include "amoranim.h"
#include "amorpm.h"

//---------------------------------------------------------------------------
//
// Constructor
//
AmorAnim::AmorAnim(KConfigBase &config)
    : mMaximumSize(0, 0)
{
    mCurrent = 0;
    mTotalMovement = 0;
    readConfig(config);
}

//---------------------------------------------------------------------------
//
// Destructor
//
AmorAnim::~AmorAnim()
{
}

//---------------------------------------------------------------------------
//
// Get the Pixmap for the current frame.
//
const QPixmap *AmorAnim::frame()
{
    const QPixmap *pixmap = 0;

    if (validFrame())
    {
        pixmap = AmorPixmapManager::manager()->pixmap(mSequence.at(mCurrent));
    }

    return pixmap;
}

//---------------------------------------------------------------------------
//
// Read a single animation's parameters.  The config class should already
// have its group set to the animation that is to be read.
//
void AmorAnim::readConfig(KConfigBase &config)
{
    // Read the list of frames to display and load them into the pixmap
    // manager.
    int frames = config.readListEntry("Sequence",mSequence);
    for (int i = 0; i < frames; i++)
    {
        const QPixmap *pixmap =
                        AmorPixmapManager::manager()->load(mSequence.at(i));
        if (pixmap)
        {
            mMaximumSize = mMaximumSize.expandedTo(pixmap->size());
        }
    }

    // Read the delays between frames.
    QStrList list;
    int entries = config.readListEntry("Delay",list);
    mDelay.resize(frames);
    for (int i = 0; i < entries && i < frames; i++)
    {
        mDelay[i] = atoi(list.at(i));
    }

    // Read the distance to move between frames and calculate the total
    // distance that this aniamtion moves from its starting position.
    entries = config.readListEntry("Movement",list);
    mMovement.resize(frames);
    for (int i = 0; i < entries; i++)
    {
        mMovement[i] = atoi(list.at(i));
        mTotalMovement += mMovement[i];
    }

    // Read the hotspot for each frame.
    entries = config.readListEntry("HotspotX",list);
    mHotspot.resize(frames);
    for (int i = 0; i < entries && i < frames; i++)
    {
        mHotspot[i].setX(atoi(list.at(i)));
    }

    entries = config.readListEntry("HotspotY",list);
    for (int i = 0; i < entries && i < frames; i++)
    {
        mHotspot[i].setY(atoi(list.at(i)));
    }

    // Add the overlap of the last frame to the total movement.
    const QPoint &lastHotspot = mHotspot[mHotspot.size()-1];
    if (mTotalMovement >= 0)
    {
        const QPixmap *lastFrame =
                    AmorPixmapManager::manager()->pixmap(mSequence.getLast());
        if (lastFrame)
        {
            mTotalMovement += (lastFrame->width() - lastHotspot.x());
        }
    }
    else
    {
        mTotalMovement -= lastHotspot.x();
    }
}

