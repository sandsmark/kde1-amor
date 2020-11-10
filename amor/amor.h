//---------------------------------------------------------------------------
//
// amor.h
//
// Copyright (c) 1999 Martin R. Jones <mjones@kde.org>
//

#ifndef AMOR_H 
#define AMOR_H 

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif 

#include <qtimer.h>
#include <qwidget.h>
#include <kwmmapp.h>

#include "amoranim.h"
#include "amorwidget.h"
#include "amordialog.h"

//---------------------------------------------------------------------------
//
// Amor handles window manager input and animation selection and updates.
//
class Amor : public QObject
{
    Q_OBJECT
public:
    Amor(KWMModuleApplication &app);
    virtual ~Amor();

    void reset();

protected slots:
    void slotMouseClicked(const QPoint &pos);
    void slotTimeout();
    void slotConfigure();
    void slotConfigChanged();
    void slotOffsetChanged(int);
    void slotAbout();
    void slotWindowActivate(Window);
    void slotWindowRemove(Window);
    void slotStacking(Window);
    void slotWindowChange(Window);

protected:
    enum State { Focus, Blur, Normal, Sleeping, Waking, Destroy };

    bool readConfig();
    bool readThemeConfig(const char *file);
    void readGroupConfig(KConfigBase &config, QList<AmorAnim> &animList,
                            const char *seq);
    void selectAnimation(State state=Normal);
    void restack();
    void active();

    virtual void timerEvent(QTimerEvent *);

private:
    KWMModuleApplication  &mApp;
    Window          mTargetWin;   // The window that the animations sits on
    QRect           mTargetRect;  // The goemetry of the target window
    Window          mNextTarget;  // The window that will become the target
    AmorWidget      *mAmor;       // The widget displaying the animation
    QList<AmorAnim> mAnimations;  // List of all animations available
    QList<AmorAnim> mFocusAnim;   // List of into focus animations 
    QList<AmorAnim> mBlurAnim;    // List of loosing focus animations 
    QList<AmorAnim> mDestroyAnim; // List of window unmapped animations 
    QList<AmorAnim> mSleepAnim;   // List of sleeping animations 
    QList<AmorAnim> mWakeAnim;    // List of waking up animations 
    AmorAnim        *mBaseAnim;   // The base animation
    AmorAnim        *mCurrAnim;   // The currently running animation
    QSize           mMaximumSize; // The largest pixmap used
    int             mPosition;    // The position of the animation
    int             mOffset;      // The vertical offset of the animation
    State           mState;       // The current state of the animation
    QTimer          *mTimer;      // Frame timer
    QString         mTheme;       // Animation theme
    AmorDialog      *mAmorDialog; // Setup dialog
    QPopupMenu      *mMenu;       // Our menu
    int             mResizeId;    // Resize timer Id
    bool            mOnTop;       // Should the animation always be on top?
    time_t          mActiveTime;  // The time an active event occurred
    QPoint          mCursPos;     // The last recorded position of the pointer
    int             mCursId;      // Pointer position timer id
};

//---------------------------------------------------------------------------

class AmorSessionWidget : public QWidget
{
    Q_OBJECT
public:
    AmorSessionWidget();
    ~AmorSessionWidget() {};
public slots:
    void wm_saveyourself();
};

#endif // AMOR_H 

