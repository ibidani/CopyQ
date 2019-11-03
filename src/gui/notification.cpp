/*
    Copyright (c) 2019, Lukas Holecek <hluk@email.cz>

    This file is part of CopyQ.

    CopyQ is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    CopyQ is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with CopyQ.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "gui/notification.h"

#include "common/log.h"
#include "common/timer.h"
#include "gui/iconfactory.h"
#include "gui/icons.h"

#include <KNotification>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QStandardPaths>
#include <QWidget>

namespace {

constexpr auto componentName = "copyq";

constexpr auto defaultConfiguration = R"(
[Global]
IconName=copyq
Comment=CopyQ Clipboard Manager
Name=CopyQ

[Event/generic]
Name=Generic event
Action=Popup
)";

} // namespace

void Notification::initConfiguration()
{
    const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    QDir dir(dataDir);
    const bool pathOk = dir.mkpath("knotifications5") && dir.cd("knotifications5");
    if (!pathOk) {
        log( QString("Failed to create directory for notification configuration: %1")
             .arg(dir.absolutePath()), LogWarning );
        return;
    }

    const QString configPath = dir.absoluteFilePath("%1.notifyrc").arg(componentName);
    QFile configFile(configPath);
    if ( !configFile.open(QIODevice::WriteOnly) ) {
        log( QString("Failed to open notification config file \"%1\": %2")
             .arg(configPath, configFile.errorString()), LogWarning );
        return;
    }

    if ( configFile.write(defaultConfiguration) == -1 ) {
        log( QString("Failed to write notification config file \"%1\": %2")
             .arg(configPath, configFile.errorString()), LogWarning );
        return;
    }

    configFile.close();
}

Notification::Notification(const QColor &iconColor, QObject *parent)
    : QObject(parent)
    , m_iconColor(iconColor)
{
    initSingleShotTimer( &m_timer, 0, this, &Notification::close );
}

Notification::~Notification()
{
    if (m_notification) {
        disconnect(m_notification);
        // FIXME: For some reason this doesn't update the notification.
        m_notification->setFlags(KNotification::CloseOnTimeout);
        m_notification->update();
    }
}

void Notification::setTitle(const QString &title)
{
    m_title = title;
}

void Notification::setMessage(const QString &msg, Qt::TextFormat format)
{
    if (format == Qt::PlainText)
        m_message = msg.toHtmlEscaped();
    else
        m_message = msg;
}

void Notification::setPixmap(const QPixmap &pixmap)
{
    m_icon.clear();
    m_iconId = 0;
    m_pixmap = pixmap;
}

void Notification::setIcon(const QString &icon)
{
    m_icon = icon;
    m_iconId = 0;
    m_pixmap = QPixmap();
}

void Notification::setIcon(ushort icon)
{
    m_icon.clear();
    m_iconId = icon;
    m_pixmap = QPixmap();
}

void Notification::setInterval(int msec)
{
    m_intervalMsec = msec;
}

void Notification::setButtons(const NotificationButtons &buttons)
{
    m_buttons = buttons;
}

void Notification::show()
{
    if (m_notification) {
        update();
        m_notification->update();
        return;
    }

    m_notification = new KNotification("generic");
    m_notification->setComponentName(componentName);

    connect( m_notification.data(), static_cast<void (KNotification::*)(unsigned int)>(&KNotification::activated),
             this, &Notification::onButtonClicked );
    connect( m_notification.data(), &KNotification::closed,
             this, &Notification::close );
    connect( m_notification.data(), &KNotification::ignored,
             this, &Notification::close );
    connect( m_notification.data(), static_cast<void (KNotification::*)()>(&KNotification::activated),
             this, &Notification::close );

    update();
    m_notification->sendEvent();
}

void Notification::close()
{
    if (m_notification) {
        m_notification->close();
        m_notification = nullptr;
    }
    emit closeNotification(this);
}

void Notification::onButtonClicked(unsigned int id)
{
    if ( static_cast<unsigned int>(m_buttons.size()) < id - 1 ) {
        emit buttonClicked(m_buttons[id - 1]);
        emit closeNotification(this);
    }
}

void Notification::update()
{
    m_notification->setTitle(m_title);
    m_notification->setText(m_message);

    if ( !m_icon.isEmpty() ) {
        m_notification->setIconName(m_icon);
    } else {
        const auto height = 64;
        const auto ratio = qApp->devicePixelRatio();
        QPixmap pixmap = m_pixmap;
        if (m_iconId != 0)
            pixmap = createPixmap(m_iconId, m_iconColor, height * ratio);
        pixmap.setDevicePixelRatio(ratio);
        m_notification->setPixmap(pixmap);
    }

    QStringList actions;
    for (const auto &button : m_buttons)
        actions.append(button.name);
    m_notification->setActions(actions);

    if (m_intervalMsec < 0) {
        m_timer.stop();
        m_notification->setFlags(KNotification::Persistent);
    } else {
        // Specific timeout is not supported by KNotifications.
        m_timer.start(m_intervalMsec);
        m_notification->setFlags(KNotification::CloseOnTimeout);
    }
}
