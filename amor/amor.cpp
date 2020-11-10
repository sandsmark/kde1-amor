//---------------------------------------------------------------------------
//
// amor.cpp
//
// Copyright (c) 1999 Martin R. Jones <mjones@kde.org>
//

#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <ksimpleconfig.h>
#include <qmessagebox.h>
#include "amor.h"
#include "amor.moc"
#include "amorpm.h"
#include "version.h"

#define SLEEP_TIMEOUT   60  // Animation sleeps after SLEEP_TIMEOUT seconds
                            // of mouse inactivity.

//---------------------------------------------------------------------------
//
// Constructor
//
Amor::Amor(KWMModuleApplication &app)
    : QObject(), mApp(app), mMaximumSize(0, 0)
{
    if (readConfig())
    {
        connect(&app, SIGNAL(windowActivate(Window)),
                SLOT(slotWindowActivate(Window)));
        connect(&app, SIGNAL(windowRemove(Window)),
                SLOT(slotWindowRemove(Window)));
        connect(&app, SIGNAL(windowRaise(Window)),
                SLOT(slotStacking(Window)));
        connect(&app, SIGNAL(windowLower(Window)),
                SLOT(slotStacking(Window)));
        connect(&app, SIGNAL(windowChange(Window)),
                SLOT(slotWindowChange(Window)));

        mAnimations.setAutoDelete(true);
        mFocusAnim.setAutoDelete(true);
        mBlurAnim.setAutoDelete(true);
        mDestroyAnim.setAutoDelete(true);
        mSleepAnim.setAutoDelete(true);
        mWakeAnim.setAutoDelete(true);

        mTargetWin   = 0;
        mNextTarget  = 0;
        mAmorDialog  = 0;
        mMenu        = 0;
        mCurrAnim    = mBaseAnim;
        mPosition    = mCurrAnim->hotspot().x();
        mState       = Normal;
        mResizeId    = 0;
        mCursId      = 0;

        mAmor = new AmorWidget();
        connect(mAmor, SIGNAL(mouseClicked(const QPoint &)),
                        SLOT(slotMouseClicked(const QPoint &)));
        mAmor->resize(mMaximumSize);

        mTimer = new QTimer(this);
        connect(mTimer, SIGNAL(timeout()), SLOT(slotTimeout()));

        time(&mActiveTime);
        mCursPos = QCursor::pos();
        mCursId = startTimer(200);
    }
    else
    {
        mApp.quit();
    }
}

//---------------------------------------------------------------------------
//
// Destructor
//
Amor::~Amor()
{
    delete mAmor;
}

//---------------------------------------------------------------------------
//
// Clear existing theme and reload configuration
//
void Amor::reset()
{
    mTimer->stop();

    mMaximumSize.setWidth(0);
    mMaximumSize.setHeight(0);

    mAnimations.clear();
    mFocusAnim.clear();
    mBlurAnim.clear();
    mDestroyAnim.clear();
    mSleepAnim.clear();
    mWakeAnim.clear();
    AmorPixmapManager::manager()->reset();

    readConfig();

    mTargetWin  = 0;
    mNextTarget = 0;
    mCurrAnim   = mBaseAnim;
    mPosition   = mCurrAnim->hotspot().x();
    mState      = Normal;

    mAmor->resize(mMaximumSize);
}

//---------------------------------------------------------------------------
//
// Read the selected theme.
//
bool Amor::readConfig()
{
    KConfig *config = mApp.getConfig();

    mOnTop  = config->readBoolEntry("OnTop");
    mOffset = config->readNumEntry("Offset");
    mTheme  = config->readEntry("Theme", "blobrc");
    if (!readThemeConfig(mTheme))
    {
        QMessageBox::critical(0, "Amor",
                             i18n("Error reading theme: ") + mTheme);
        return false;
    }

    return true;
}

//---------------------------------------------------------------------------
//
// Read all animations for a particluar theme
//
bool Amor::readThemeConfig(const char *file)
{
    QString path = KApplication::localkdedir().copy();
    path += "/share/apps/amor/";
    path += file;

    if (access(path, R_OK))
    {
        path = KApplication::kde_datadir().copy();
        path += "/amor/";
        path += file;
    }

    KSimpleConfig config(path, true);
    config.setGroup("Config");

    // Get the directory where the pixmaps are stored and tell the
    // pixmap manager.
    QString pixmapPath = config.readEntry("PixmapPath");
    if (pixmapPath.isEmpty())
    {
        return false;
    }

    if (pixmapPath[0] == '/')
    {
        // absolute path to pixmaps
        path = pixmapPath;
    }
    else
    {
        // relative to config file.
        path.truncate(path.findRev('/')+1);
        path += pixmapPath;
    }

    AmorPixmapManager::manager()->setPixmapDir(path);

    // There must always be a base animation which does not move and is
    // the "glue" between all animations.
    config.setGroup("Base");
    mBaseAnim = new AmorAnim(config);
    mAnimations.append(mBaseAnim);   // base can be used as normal anim too.

    // Read all the animation groups
    readGroupConfig(config, mAnimations, "Sequences");
    readGroupConfig(config, mFocusAnim, "Focus");
    readGroupConfig(config, mBlurAnim, "Blur");
    readGroupConfig(config, mDestroyAnim, "Destroy");
    readGroupConfig(config, mSleepAnim, "Sleep");
    readGroupConfig(config, mWakeAnim, "Wake");

    return true;
}

//---------------------------------------------------------------------------
//
// Read an animation group.
//
void Amor::readGroupConfig(KConfigBase &config, QList<AmorAnim> &animList,
                                const char *seq)
{
    // Read the list of available animations.
    config.setGroup("Config");
    QStrList list;
    int entries = config.readListEntry(seq, list);

    // Read each individual animation
    for (int i = 0; i < entries; i++)
    {
        config.setGroup(list.at(i));
        AmorAnim *anim = new AmorAnim(config);
        animList.append(anim);
        mMaximumSize = mMaximumSize.expandedTo(anim->maximumSize());
    }

    // If no animations were available for this group, just add the base anim
    if (entries == 0)
    {
        config.setGroup("Base");
        AmorAnim *anim = new AmorAnim(config);
        animList.append(anim);
        mMaximumSize = mMaximumSize.expandedTo(anim->maximumSize());
    }
}

//---------------------------------------------------------------------------
//
// Randomly select a new animation.
//
void Amor::selectAnimation(State state)
{
    switch (state)
    {
        case Blur:
            mCurrAnim = mBlurAnim.at(random()%mBlurAnim.count());
            mState = Focus;
            break;

        case Focus:
            mCurrAnim = mFocusAnim.at(random()%mFocusAnim.count());
            mCurrAnim->reset();
            mTargetWin = mNextTarget;
            if (mTargetWin != None)
            {
                mTargetRect = KWM::geometry(mTargetWin, true);
                if (mCurrAnim->frame())
                {
                    mPosition = (random() %
                        (mTargetRect.width() - mCurrAnim->frame()->width())) +
                         mCurrAnim->hotspot().x();
                }
                else
                {
                    mPosition = mTargetRect.width()/2;
                }
            }
            else
            {
                // We don't want to do anything until a window comes into
                // focus.
                mTimer->stop();
            }
            mAmor->hide();
            restack();
            mState = Normal;
            break;

        case Destroy:
            mCurrAnim = mDestroyAnim.at(random()%mDestroyAnim.count());
            mState = Focus;
            break;

        case Sleeping:
            mCurrAnim = mSleepAnim.at(random()%mSleepAnim.count());
            break;

        case Waking:
            mCurrAnim = mWakeAnim.at(random()%mWakeAnim.count());
            mState = Normal;
            break;

        default:
            if (mCurrAnim == mBaseAnim)
            {
                mCurrAnim = mAnimations.at(random()%mAnimations.count());
            }
            else
            {
                mCurrAnim = mBaseAnim;
            }
            break;
    }

    if (mCurrAnim->totalMovement() + mPosition > mTargetRect.width() ||
        mCurrAnim->totalMovement() + mPosition < 0)
    {
        // The selected animation would end outside of this window's width
        // We could randomly select a different one, but I prefer to just
        // use the default animation.
        mCurrAnim = mBaseAnim;
    }
    mCurrAnim->reset();
}

//---------------------------------------------------------------------------
//
// Set the animation's stacking order to be just above the target window's
// window decoration, or on top.
//
void Amor::restack()
{
    if (mTargetWin == None)
    {
        return;
    }

    if (mOnTop)
    {
        // simply raise the widget to the top
        mAmor->raise();
        return;
    }

    Window dw, parent, *wins;
    unsigned int nwins = 0;

    // We must use the target window's parent as our sibling.
    // Is there a faster way to get parent window than XQueryTree?
    if (XQueryTree(qt_xdisplay(), mTargetWin, &dw, &parent, &wins, &nwins))
    {
        if (nwins)
        {
            XFree(wins);
        }
    }

    // Set animation's stacking order to be above the window manager's
    // decoration of target window.
    XWindowChanges values;
    values.sibling = parent != None ? parent : mTargetWin;
    values.stack_mode = Above;
    XConfigureWindow(qt_xdisplay(), mAmor->winId(), CWSibling | CWStackMode,
                     &values);

}

//---------------------------------------------------------------------------
//
// Handle various timer events.
//
void Amor::timerEvent(QTimerEvent *te)
{
    if (te->timerId() == mResizeId)
    {
        killTimer(mResizeId);
        restack();
    }
    else if (te->timerId() == mCursId)
    {
        QPoint currPos = QCursor::pos();
        QPoint diff = currPos - mCursPos;
        time_t now = time(0);

        if (abs(diff.x()) > 1 || abs(diff.y()) > 1)
        {
            if (mState == Sleeping)
            {
                // Set waking immediatedly
                selectAnimation(Waking);
            }
            mActiveTime = now;
            mCursPos = currPos;
        }
        else if (mState != Sleeping && now - mActiveTime > SLEEP_TIMEOUT)
        {
            // The next animation will become sleeping
            mState = Sleeping;
        }
    }
}

//---------------------------------------------------------------------------
//
// The user clicked on our animation.
//
void Amor::slotMouseClicked(const QPoint &pos)
{
    bool restartTimer = mTimer->isActive();

    // Stop the animation while the menu is open.
    if (restartTimer)
    {
        mTimer->stop();
    }

    if (!mMenu)
    {
        mMenu = new QPopupMenu();
        mMenu->insertItem(i18n("&Options..."), this, SLOT(slotConfigure()));
        mMenu->insertItem(i18n("&About..."), this, SLOT(slotAbout()));
        mMenu->insertSeparator();
        mMenu->insertItem(i18n("&Quit"), kapp, SLOT(quit()));
    }

    mMenu->exec(pos);

    if (restartTimer)
    {
        mTimer->start(1000, true);
    }
}

//---------------------------------------------------------------------------
//
// Display the next frame or a new animation
//
void Amor::slotTimeout()
{
    mPosition += mCurrAnim->movement();
    mAmor->setPixmap(mCurrAnim->frame());
    mAmor->move(mPosition + mTargetRect.x() - mCurrAnim->hotspot().x(),
                 mTargetRect.y() - mCurrAnim->hotspot().y() + mOffset);
    if (!mAmor->isVisible())
    {
        mAmor->show();
        restack();
    }

    mTimer->start(mCurrAnim->delay(), true);

    if (!mCurrAnim->next())
    {
        selectAnimation(mState);
    }
}

//---------------------------------------------------------------------------
//
// Display configuration dialog
//
void Amor::slotConfigure()
{
    if (!mAmorDialog)
    {
        mAmorDialog = new AmorDialog();
        connect(mAmorDialog, SIGNAL(changed()), SLOT(slotConfigChanged()));
        connect(mAmorDialog, SIGNAL(offsetChanged(int)),
                SLOT(slotOffsetChanged(int)));
    }

    mAmorDialog->show();
}

//---------------------------------------------------------------------------
//
// Configuration changed.
//
void Amor::slotConfigChanged()
{
    reset();
}

//---------------------------------------------------------------------------
//
// Offset changed
//
void Amor::slotOffsetChanged(int off)
{
    mOffset = off;

    if (mCurrAnim->frame())
    {
        mAmor->move(mPosition + mTargetRect.x() - mCurrAnim->hotspot().x(),
                 mTargetRect.y() - mCurrAnim->hotspot().y() + mOffset);
    }
}

//---------------------------------------------------------------------------
//
// Display About box
//
void Amor::slotAbout()
{
    QString about = i18n("Amor  Version ") + QString(AMOR_VERSION) + "\n\n" +
                    i18n("Amusing Misuse Of Resources\n\n") +
                    i18n("Copyright (c) 1999 Martin R. Jones <mjones@kde.org>");
    QMessageBox mb;
    mb.setText(about);
    mb.setCaption(i18n("About Amor"));
    mb.setIcon(QMessageBox::Information);
    mb.show();
}

//---------------------------------------------------------------------------
//
// Focus changed to a different window
//
void Amor::slotWindowActivate(Window win)
{
    mTimer->stop();
    mNextTarget = win;

    // This is an active event that affects the target window
    time(&mActiveTime);

    // A window gaining focus implies that the current window has lost
    // focus.  Initiate a blur event if there is a current active window.
    if (mTargetWin)
    {
        // We are losing focus from the current window
        selectAnimation(Blur);
        mTimer->start(0, true);
    }
    else if (mNextTarget)
    {
        // We are setting focus to a new window
        selectAnimation(Focus);
        mTimer->start(0, true);
    }
    else
    {
        // No action - We can get this when we switch between two empty
        // desktops
        mAmor->hide();
    }
}

//---------------------------------------------------------------------------
//
// Window removed
//
void Amor::slotWindowRemove(Window win)
{
    if (win == mTargetWin)
    {
        // This is an active event that affects the target window
        time(&mActiveTime);

        selectAnimation(Destroy);
        mTimer->stop();
        mTimer->start(0, true);
    }
}

//---------------------------------------------------------------------------
//
// Window raised/lowered
//
void Amor::slotStacking(Window win)
{
    if (win == mTargetWin)
    {
        // This is an active event that affects the target window
        time(&mActiveTime);

        // We seem to get this signal before the window has been restacked,
        // so we just schedule a restack.
        mResizeId = startTimer(20);
    }
}

//---------------------------------------------------------------------------
//
// Properties of a window changed
//
void Amor::slotWindowChange(Window win)
{
    if (win != mTargetWin)
    {
        return;
    }

    // This is an active event that affects the target window
    time(&mActiveTime);

    if (KWM::isIconified(mTargetWin))
    {
        // The target window has been iconified
        selectAnimation(Destroy);
        mTimer->stop();
        mTimer->start(0, true);
    }
    else
    {
        // The size or position of the window has changed.
        mTargetRect = KWM::geometry(mTargetWin, true);

        // make sure the animation is still on the window.
        if (mCurrAnim->frame())
        {
            if (mPosition > mTargetRect.width() -
                    (mCurrAnim->frame()->width() - mCurrAnim->hotspot().x()))
            {
                mPosition = mTargetRect.width() -
                    (mCurrAnim->frame()->width() - mCurrAnim->hotspot().x());
            }
            mAmor->move(mPosition + mTargetRect.x() - mCurrAnim->hotspot().x(),
                     mTargetRect.y() - mCurrAnim->hotspot().y() + mOffset);
        }
    }
}

//===========================================================================

AmorSessionWidget::AmorSessionWidget()
{
    // the only function of this widget is to catch & forward the
    // saveYourself() signal from the session manager
    connect(kapp, SIGNAL(saveYourself()), SLOT(wm_saveyourself()));
}
 
void AmorSessionWidget::wm_saveyourself()
{
    // no action required currently.
}

