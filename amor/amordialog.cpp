//---------------------------------------------------------------------------
//
// amordialog.cpp
//
// Copyright (c) 1999 Martin R. Jones <mjones@kde.org>
//

#include <qlayout.h>
#include <qpushbutton.h>
#include <qcheckbox.h>
#include <qlabel.h>
#include <qslider.h>
#include <qdir.h>
#include <qpainter.h>
#include <kapp.h>
#include <ksimpleconfig.h>

#include "amordialog.h"
#include "amordialog.moc"
#include "version.h"

//---------------------------------------------------------------------------
//
// Constructor
//
AmorDialog::AmorDialog()
    : QDialog()
{
    readConfig();

    QVBoxLayout *layout = new QVBoxLayout(this, 15);

    QHBoxLayout *hb = new QHBoxLayout();
    layout->addLayout(hb, 1);

    // Theme list
    QVBoxLayout *themeBox = new QVBoxLayout();
    hb->addLayout(themeBox, 1);

    QLabel *label = new QLabel(i18n("Theme:"), this);
    label->setFixedHeight(label->sizeHint().height());
    themeBox->addWidget(label);

    mThemeListBox = new QListBox(this);
    connect(mThemeListBox,SIGNAL(highlighted(int)),SLOT(slotHighlighted(int)));
    mThemeListBox->setMinimumHeight(70);
    themeBox->addWidget(mThemeListBox);

    // Animation offset
    QVBoxLayout *offsetBox = new QVBoxLayout();
    hb->addLayout(offsetBox);

    label = new QLabel(i18n("Offset:"), this);
    label->setFixedSize(label->sizeHint());
    offsetBox->addWidget(label);

    QSlider *slider = new QSlider(-40, 40, 5, mOffset, QSlider::Vertical, this);
    connect(slider, SIGNAL(valueChanged(int)), SLOT(slotOffset(int)));
    slider->setFixedWidth(slider->sizeHint().width());
    offsetBox->addWidget(slider);

    // Always on top
    QCheckBox *checkBox = new QCheckBox(i18n("Always on top"), this);
    connect(checkBox, SIGNAL(toggled(bool)), SLOT(slotOnTop(bool)));
    checkBox->setFixedHeight(checkBox->sizeHint().height());
    checkBox->setChecked(mOnTop);
    layout->addWidget(checkBox);

    // OK/Cancel
    hb = new QHBoxLayout();
    layout->addLayout(hb);

    QPushButton *button = new QPushButton(i18n("&OK"), this);
    connect(button, SIGNAL(clicked()), SLOT(slotOk()));
    button->setFixedSize(button->sizeHint());
    hb->addWidget(button);

    hb->addStretch(1);

    button = new QPushButton(i18n("&Cancel"), this);
    connect(button, SIGNAL(clicked()), SLOT(slotCancel()));
    button->setFixedSize(button->sizeHint());
    hb->addWidget(button);

    layout->activate();

    readThemes();

    resize( 350, 200 );
}

//---------------------------------------------------------------------------
//
// Destructor
//
AmorDialog::~AmorDialog()
{
}

//---------------------------------------------------------------------------
//
// Read the configuration
//
void AmorDialog::readConfig()
{
    KConfig *config = kapp->getConfig();

    mOnTop = config->readBoolEntry("OnTop");
    mOffset = config->readNumEntry("Offset");
    mCurrTheme = config->readEntry("Theme", "blobrc");
}

//---------------------------------------------------------------------------
//
// Write the configuration
//
void AmorDialog::writeConfig()
{
    KConfig *config = kapp->getConfig();

    config->writeEntry("OnTop", mOnTop);
    config->writeEntry("Offset", mOffset);
    config->writeEntry("Theme", mCurrTheme);

    config->sync();
}

//---------------------------------------------------------------------------
//
// Get list of all themes
//
void AmorDialog::readThemes()
{
    // read global themes
    QString path = KApplication::localkdedir().copy();
    path += "/share/apps/amor";

    QDir dir(path, "*rc");
    if (dir.isReadable())
    {
        const QStrList *list = dir.entryList();

        QStrListIterator it( *list );
        for (; it.current(); ++it)
        {
            addTheme(path, it.current());
        }
    }

    // read local themes
    path = KApplication::kde_datadir().copy();
    path += "/amor";

    dir.setPath(path);
    if (dir.isReadable())
    {
        const QStrList *list = dir.entryList();

        QStrListIterator it( *list );
        for (; it.current(); ++it)
        {
            addTheme(path, it.current());
        }
    }
}

//---------------------------------------------------------------------------
//
// Add a single theme to the list
//
void AmorDialog::addTheme(QString path, QString file)
{
    KSimpleConfig config(path + "/" + file, true);

    config.setGroup("Config");

    QString pixmapPath = config.readEntry("PixmapPath");
    if (pixmapPath.isEmpty())
    {
        return;
    }

    if (pixmapPath[0] != '/')
    {
        // relative to config file.
        pixmapPath = path + "/" + pixmapPath;
    }

    QString description = config.readEntry("Description");
    QString pixmapName = config.readEntry("Icon");

    pixmapPath += "/";
    pixmapPath += pixmapName;

    QPixmap pixmap(pixmapPath);

    AmorListBoxItem *item = new AmorListBoxItem(description, pixmap);
    mThemeListBox->insertItem(item);
    mThemes.append(file);

    if (mCurrTheme == file)
    {
        mThemeListBox->setSelected(mThemeListBox->count()-1, true);
    }
}

//---------------------------------------------------------------------------
//
// User highlighted a theme
//
void AmorDialog::slotHighlighted(int index)
{
    mCurrTheme = mThemes.at(index);
}

//---------------------------------------------------------------------------
//
// User changed offset
//
void AmorDialog::slotOffset(int off)
{
    mOffset = off;
    emit offsetChanged(mOffset);
}

//---------------------------------------------------------------------------
//
// User toggled always on top
//
void AmorDialog::slotOnTop(bool onTop)
{
    mOnTop = onTop;
}

//---------------------------------------------------------------------------
//
// User clicked Ok
//
void AmorDialog::slotOk()
{
    writeConfig();
    emit changed();
    accept();
}

//---------------------------------------------------------------------------
//
// User clicked Cancel
//
void AmorDialog::slotCancel()
{
    // restore offset
    KConfig *config = kapp->getConfig();
    emit offsetChanged(config->readNumEntry("Offset"));
    reject();
}

//===========================================================================
//
// AmorListBoxItem implements a list box items for selection of themes
//
void AmorListBoxItem::paint( QPainter *p )
{
    p->drawPixmap( 3, 0, mPixmap );
    QFontMetrics fm = p->fontMetrics();
    int yPos;                       // vertical text position
    if ( mPixmap.height() < fm.height() )
        yPos = fm.ascent() + fm.leading()/2;
    else
        yPos = mPixmap.height()/2 - fm.height()/2 + fm.ascent();
    p->drawText( mPixmap.width() + 5, yPos, text() );
}

int AmorListBoxItem::height(const QListBox *lb ) const
{
    return QMAX( mPixmap.height(), lb->fontMetrics().lineSpacing() + 1 );
}

int AmorListBoxItem::width(const QListBox *lb ) const
{
    return mPixmap.width() + lb->fontMetrics().width( text() ) + 6;
}

