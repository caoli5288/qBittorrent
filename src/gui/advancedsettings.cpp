/*
 * Bittorrent Client using Qt and libtorrent.
 * Copyright (C) 2016 qBittorrent project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * In addition, as a special exception, the copyright holders give permission to
 * link this program with the OpenSSL project's "OpenSSL" library (or with
 * modified versions of it that use the same license as the "OpenSSL" library),
 * and distribute the linked executables. You must obey the GNU General Public
 * License in all respects for all of the code used other than "OpenSSL".  If you
 * modify file(s), you may extend this exception to your version of the file(s),
 * but you are not obligated to do so. If you do not wish to do so, delete this
 * exception statement from your version.
 */

#include "advancedsettings.h"

#include <limits>

#include <QHeaderView>
#include <QHostAddress>
#include <QLabel>
#include <QNetworkInterface>

#include "base/bittorrent/session.h"
#include "base/global.h"
#include "base/preferences.h"
#include "base/unicodestrings.h"
#include "app/application.h"
#include "gui/addnewtorrentdialog.h"
#include "gui/mainwindow.h"

namespace
{
    QString makeLink(const QString &url, const QString &linkLabel)
    {
         return QStringLiteral("<a href=\"%1\">%2</a>").arg(url, linkLabel);
    }
}

enum AdvSettingsCols
{
    PROPERTY,
    VALUE,
    COL_COUNT
};
enum AdvSettingsRows
{
    // qBittorrent section
    QBITTORRENT_HEADER,
    // network interface
    NETWORK_IFACE,
    //Optional network address
    NETWORK_IFACE_ADDRESS,
    NETWORK_LISTEN_IPV6,
    // behavior
    SAVE_RESUME_DATA_INTERVAL,
    CONFIRM_RECHECK_TORRENT,
    RECHECK_COMPLETED,
#if defined(Q_OS_WIN) || defined(Q_OS_MAC)
    UPDATE_CHECK,
#endif
    // UI related
    LIST_REFRESH,
    RESOLVE_HOSTS,
    RESOLVE_COUNTRIES,
    PROGRAM_NOTIFICATIONS,
    TORRENT_ADDED_NOTIFICATIONS,
    CONFIRM_REMOVE_ALL_TAGS,
    DOWNLOAD_TRACKER_FAVICON,
    SAVE_PATH_HISTORY_LENGTH,
    ENABLE_SPEED_WIDGET,
#if (defined(Q_OS_UNIX) && !defined(Q_OS_MAC))
    USE_ICON_THEME,
#endif

    // libtorrent section
    LIBTORRENT_HEADER,
    ASYNC_IO_THREADS,
    FILE_POOL_SIZE,
    CHECKING_MEM_USAGE,
    // cache
    DISK_CACHE,
    DISK_CACHE_TTL,
    OS_CACHE,
    COALESCE_RW,
    SUGGEST_MODE,
    SEND_BUF_WATERMARK,
    SEND_BUF_LOW_WATERMARK,
    SEND_BUF_WATERMARK_FACTOR,
    // networking & ports
    SOCKET_BACKLOG_SIZE,
    OUTGOING_PORT_MIN,
    OUTGOING_PORT_MAX,
    UTP_MIX_MODE,
    MULTI_CONNECTIONS_PER_IP,
    // embedded tracker
    TRACKER_STATUS,
    TRACKER_PORT,
    // seeding
    CHOKING_ALGORITHM,
    SEED_CHOKING_ALGORITHM,
    SUPER_SEEDING,
    // tracker
    ANNOUNCE_ALL_TRACKERS,
    ANNOUNCE_ALL_TIERS,
    ANNOUNCE_IP,

    ROW_COUNT
};

AdvancedSettings::AdvancedSettings(QWidget *parent)
    : QTableWidget(parent)
{
    // column
    setColumnCount(COL_COUNT);
    QStringList header = {tr("Setting"), tr("Value", "Value set for this setting")};
    setHorizontalHeaderLabels(header);
    // row
    setRowCount(ROW_COUNT);
    verticalHeader()->setVisible(false);
    // etc.
    setAlternatingRowColors(true);
    setSelectionMode(QAbstractItemView::NoSelection);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    // Signals
    connect(&m_spinBoxCache, qOverload<int>(&QSpinBox::valueChanged)
            , this, &AdvancedSettings::updateCacheSpinSuffix);
    connect(&m_comboBoxInterface, qOverload<int>(&QComboBox::currentIndexChanged)
            , this, &AdvancedSettings::updateInterfaceAddressCombo);
    connect(&m_spinBoxSaveResumeDataInterval, qOverload<int>(&QSpinBox::valueChanged)
            , this, &AdvancedSettings::updateSaveResumeDataIntervalSuffix);
    // Load settings
    loadAdvancedSettings();
    resizeColumnToContents(0);
    horizontalHeader()->setStretchLastSection(true);
}

void AdvancedSettings::saveAdvancedSettings()
{
    Preferences *const pref = Preferences::instance();
    BitTorrent::Session *const session = BitTorrent::Session::instance();

    // Async IO threads
    session->setAsyncIOThreads(m_spinBoxAsyncIOThreads.value());
    // File pool size
    session->setFilePoolSize(m_spinBoxFilePoolSize.value());
    // Checking Memory Usage
    session->setCheckingMemUsage(m_spinBoxCheckingMemUsage.value());
    // Disk write cache
    session->setDiskCacheSize(m_spinBoxCache.value());
    session->setDiskCacheTTL(m_spinBoxCacheTTL.value());
    // Enable OS cache
    session->setUseOSCache(m_checkBoxOsCache.isChecked());
    // Coalesce reads & writes
    session->setCoalesceReadWriteEnabled(m_checkBoxCoalesceRW.isChecked());
    // Suggest mode
    session->setSuggestMode(m_checkBoxSuggestMode.isChecked());
    // Send buffer watermark
    session->setSendBufferWatermark(m_spinBoxSendBufferWatermark.value());
    session->setSendBufferLowWatermark(m_spinBoxSendBufferLowWatermark.value());
    session->setSendBufferWatermarkFactor(m_spinBoxSendBufferWatermarkFactor.value());
    // Socket listen backlog size
    session->setSocketBacklogSize(m_spinBoxSocketBacklogSize.value());
    // Save resume data interval
    session->setSaveResumeDataInterval(m_spinBoxSaveResumeDataInterval.value());
    // Outgoing ports
    session->setOutgoingPortsMin(m_spinBoxOutgoingPortsMin.value());
    session->setOutgoingPortsMax(m_spinBoxOutgoingPortsMax.value());
    // uTP-TCP mixed mode
    session->setUtpMixedMode(static_cast<BitTorrent::MixedModeAlgorithm>(m_comboBoxUtpMixedMode.currentIndex()));
    // multiple connections per IP
    session->setMultiConnectionsPerIpEnabled(m_checkBoxMultiConnectionsPerIp.isChecked());
    // Recheck torrents on completion
    pref->recheckTorrentsOnCompletion(m_checkBoxRecheckCompleted.isChecked());
    // Transfer list refresh interval
    session->setRefreshInterval(m_spinBoxListRefresh.value());
    // Peer resolution
    pref->resolvePeerCountries(m_checkBoxResolveCountries.isChecked());
    pref->resolvePeerHostNames(m_checkBoxResolveHosts.isChecked());
    // Super seeding
    session->setSuperSeedingEnabled(m_checkBoxSuperSeeding.isChecked());
    // Network interface
    if (m_comboBoxInterface.currentIndex() == 0) {
        // All interfaces (default)
        session->setNetworkInterface(QString());
        session->setNetworkInterfaceName(QString());
    }
    else {
        session->setNetworkInterface(m_comboBoxInterface.itemData(m_comboBoxInterface.currentIndex()).toString());
        session->setNetworkInterfaceName(m_comboBoxInterface.currentText());
    }

    // Interface address
    if (m_comboBoxInterfaceAddress.currentIndex() == 0) {
        // All addresses (default)
        session->setNetworkInterfaceAddress({});
    }
    else {
        QHostAddress ifaceAddr(m_comboBoxInterfaceAddress.currentText().trimmed());
        ifaceAddr.isNull() ? session->setNetworkInterfaceAddress({}) : session->setNetworkInterfaceAddress(ifaceAddr.toString());
    }
    session->setIPv6Enabled(m_checkBoxListenIPv6.isChecked());
    // Announce IP
    QHostAddress addr(m_lineEditAnnounceIP.text().trimmed());
    session->setAnnounceIP(addr.isNull() ? "" : addr.toString());

    // Program notification
    MainWindow *const mainWindow = static_cast<Application*>(QCoreApplication::instance())->mainWindow();
    mainWindow->setNotificationsEnabled(m_checkBoxProgramNotifications.isChecked());
    mainWindow->setTorrentAddedNotificationsEnabled(m_checkBoxTorrentAddedNotifications.isChecked());
    // Misc GUI properties
    mainWindow->setDownloadTrackerFavicon(m_checkBoxTrackerFavicon.isChecked());
    AddNewTorrentDialog::setSavePathHistoryLength(m_spinBoxSavePathHistoryLength.value());
    pref->setSpeedWidgetEnabled(m_checkBoxSpeedWidgetEnabled.isChecked());

    // Tracker
    pref->setTrackerPort(m_spinBoxTrackerPort.value());
    session->setTrackerEnabled(m_checkBoxTrackerStatus.isChecked());
    // Choking algorithm
    session->setChokingAlgorithm(static_cast<BitTorrent::ChokingAlgorithm>(m_comboBoxChokingAlgorithm.currentIndex()));
    // Seed choking algorithm
    session->setSeedChokingAlgorithm(static_cast<BitTorrent::SeedChokingAlgorithm>(m_comboBoxSeedChokingAlgorithm.currentIndex()));

#if defined(Q_OS_WIN) || defined(Q_OS_MAC)
    pref->setUpdateCheckEnabled(m_checkBoxUpdateCheck.isChecked());
#endif
    // Icon theme
#if (defined(Q_OS_UNIX) && !defined(Q_OS_MAC))
    pref->useSystemIconTheme(m_checkBoxUseIconTheme.isChecked());
#endif
    pref->setConfirmTorrentRecheck(m_checkBoxConfirmTorrentRecheck.isChecked());

    pref->setConfirmRemoveAllTags(m_checkBoxConfirmRemoveAllTags.isChecked());

    session->setAnnounceToAllTrackers(m_checkBoxAnnounceAllTrackers.isChecked());
    session->setAnnounceToAllTiers(m_checkBoxAnnounceAllTiers.isChecked());
}

void AdvancedSettings::updateCacheSpinSuffix(int value)
{
    if (value == 0)
        m_spinBoxCache.setSuffix(tr(" (disabled)"));
    else if (value < 0)
        m_spinBoxCache.setSuffix(tr(" (auto)"));
    else
        m_spinBoxCache.setSuffix(tr(" MiB"));
}

void AdvancedSettings::updateSaveResumeDataIntervalSuffix(const int value)
{
    if (value > 0)
        m_spinBoxSaveResumeDataInterval.setSuffix(tr(" min", " minutes"));
    else
        m_spinBoxSaveResumeDataInterval.setSuffix(tr(" (disabled)"));
}

void AdvancedSettings::updateInterfaceAddressCombo()
{
    // Try to get the currently selected interface name
    const QString ifaceName = m_comboBoxInterface.itemData(m_comboBoxInterface.currentIndex()).toString(); // Empty string for the first element
    const QString currentAddress = BitTorrent::Session::instance()->networkInterfaceAddress();

    // Clear all items and reinsert them, default to all
    m_comboBoxInterfaceAddress.clear();
    m_comboBoxInterfaceAddress.addItem(tr("All addresses"));
    m_comboBoxInterfaceAddress.setCurrentIndex(0);

    auto populateCombo = [this, &currentAddress](const QString &ip, const QAbstractSocket::NetworkLayerProtocol &protocol)
    {
        Q_ASSERT((protocol == QAbstractSocket::IPv4Protocol) || (protocol == QAbstractSocket::IPv6Protocol));
        // Only take ipv4 for now?
        if ((protocol != QAbstractSocket::IPv4Protocol) && (protocol != QAbstractSocket::IPv6Protocol))
            return;
        m_comboBoxInterfaceAddress.addItem(ip);
        //Try to select the last added one
        if (ip == currentAddress)
            m_comboBoxInterfaceAddress.setCurrentIndex(m_comboBoxInterfaceAddress.count() - 1);
    };

    if (ifaceName.isEmpty()) {
        for (const QHostAddress &ip : asConst(QNetworkInterface::allAddresses()))
            populateCombo(ip.toString(), ip.protocol());
    }
    else {
        const QNetworkInterface iface = QNetworkInterface::interfaceFromName(ifaceName);
        const QList<QNetworkAddressEntry> addresses = iface.addressEntries();
        for (const QNetworkAddressEntry &entry : addresses) {
            const QHostAddress ip = entry.ip();
            populateCombo(ip.toString(), ip.protocol());
        }
    }
}

void AdvancedSettings::loadAdvancedSettings()
{
    const Preferences *const pref = Preferences::instance();
    const BitTorrent::Session *const session = BitTorrent::Session::instance();

    // add section headers
    auto *labelQbtLink = new QLabel(
        makeLink(QLatin1String("https://github.com/qbittorrent/qBittorrent/wiki/Explanation-of-Options-in-qBittorrent#Advanced")
                 , tr("Open documentation"))
        , this);
    labelQbtLink->setOpenExternalLinks(true);
    addRow(QBITTORRENT_HEADER, QString("<b>%1</b>").arg(tr("qBittorrent Section")), labelQbtLink);
    static_cast<QLabel *>(cellWidget(QBITTORRENT_HEADER, PROPERTY))->setAlignment(Qt::AlignCenter | Qt::AlignVCenter);

    auto *labelLibtorrentLink = new QLabel(
        makeLink(QLatin1String("https://www.libtorrent.org/reference.html")
                 , tr("Open documentation"))
        , this);
    labelLibtorrentLink->setOpenExternalLinks(true);
    addRow(LIBTORRENT_HEADER, QString("<b>%1</b>").arg(tr("libtorrent Section")), labelLibtorrentLink);
    static_cast<QLabel *>(cellWidget(LIBTORRENT_HEADER, PROPERTY))->setAlignment(Qt::AlignCenter | Qt::AlignVCenter);

    // Async IO threads
    m_spinBoxAsyncIOThreads.setMinimum(1);
    m_spinBoxAsyncIOThreads.setMaximum(1024);
    m_spinBoxAsyncIOThreads.setValue(session->asyncIOThreads());
    addRow(ASYNC_IO_THREADS, (tr("Asynchronous I/O threads") + ' ' + makeLink("https://www.libtorrent.org/reference-Settings.html#aio_threads", "(?)"))
            , &m_spinBoxAsyncIOThreads);

    // File pool size
    m_spinBoxFilePoolSize.setMinimum(1);
    m_spinBoxFilePoolSize.setMaximum(std::numeric_limits<int>::max());
    m_spinBoxFilePoolSize.setValue(session->filePoolSize());
    addRow(FILE_POOL_SIZE, (tr("File pool size") + ' ' + makeLink("https://www.libtorrent.org/reference-Settings.html#file_pool_size", "(?)"))
        , &m_spinBoxFilePoolSize);

    // Checking Memory Usage
    m_spinBoxCheckingMemUsage.setMinimum(1);
    // When build as 32bit binary, set the maximum value lower to prevent crashes.
#ifdef QBT_APP_64BIT
    m_spinBoxCheckingMemUsage.setMaximum(1024);
#else
    // Allocate at most 128MiB out of the remaining 512MiB (see the cache part below)
    m_spinBoxCheckingMemUsage.setMaximum(128);
#endif
    m_spinBoxCheckingMemUsage.setValue(session->checkingMemUsage());
    m_spinBoxCheckingMemUsage.setSuffix(tr(" MiB"));
    addRow(CHECKING_MEM_USAGE, (tr("Outstanding memory when checking torrents") + ' ' + makeLink("https://www.libtorrent.org/reference-Settings.html#checking_mem_usage", "(?)"))
            , &m_spinBoxCheckingMemUsage);

    // Disk write cache
    m_spinBoxCache.setMinimum(-1);
    // When build as 32bit binary, set the maximum at less than 2GB to prevent crashes.
#ifdef QBT_APP_64BIT
    m_spinBoxCache.setMaximum(33554431);  // 32768GiB
#else
    // allocate 1536MiB and leave 512MiB to the rest of program data in RAM
    m_spinBoxCache.setMaximum(1536);
#endif
    m_spinBoxCache.setValue(session->diskCacheSize());
    updateCacheSpinSuffix(m_spinBoxCache.value());
    addRow(DISK_CACHE, (tr("Disk cache") + ' ' + makeLink("https://www.libtorrent.org/reference-Settings.html#cache_size", "(?)"))
            , &m_spinBoxCache);
    // Disk cache expiry
    m_spinBoxCacheTTL.setMinimum(1);
    m_spinBoxCacheTTL.setMaximum(std::numeric_limits<int>::max());
    m_spinBoxCacheTTL.setValue(session->diskCacheTTL());
    m_spinBoxCacheTTL.setSuffix(tr(" s", " seconds"));
    addRow(DISK_CACHE_TTL, (tr("Disk cache expiry interval") + ' ' + makeLink("https://www.libtorrent.org/reference-Settings.html#cache_expiry", "(?)"))
            , &m_spinBoxCacheTTL);
    // Enable OS cache
    m_checkBoxOsCache.setChecked(session->useOSCache());
    addRow(OS_CACHE, (tr("Enable OS cache") + ' ' + makeLink("https://www.libtorrent.org/reference-Settings.html#disk_io_write_mode", "(?)"))
            , &m_checkBoxOsCache);
    // Coalesce reads & writes
    m_checkBoxCoalesceRW.setChecked(session->isCoalesceReadWriteEnabled());
    addRow(COALESCE_RW, (tr("Coalesce reads & writes") + ' ' + makeLink("https://www.libtorrent.org/reference-Settings.html#coalesce_reads", "(?)"))
            , &m_checkBoxCoalesceRW);
    // Suggest mode
    m_checkBoxSuggestMode.setChecked(session->isSuggestModeEnabled());
    addRow(SUGGEST_MODE, (tr("Send upload piece suggestions") + ' ' + makeLink("https://www.libtorrent.org/reference-Settings.html#suggest_mode", "(?)"))
            , &m_checkBoxSuggestMode);
    // Send buffer watermark
    m_spinBoxSendBufferWatermark.setMinimum(1);
    m_spinBoxSendBufferWatermark.setMaximum(std::numeric_limits<int>::max());
    m_spinBoxSendBufferWatermark.setSuffix(tr(" KiB"));
    m_spinBoxSendBufferWatermark.setValue(session->sendBufferWatermark());
    addRow(SEND_BUF_WATERMARK, (tr("Send buffer watermark") + ' ' + makeLink("https://www.libtorrent.org/reference-Settings.html#send_buffer_watermark", "(?)"))
            , &m_spinBoxSendBufferWatermark);
    m_spinBoxSendBufferLowWatermark.setMinimum(1);
    m_spinBoxSendBufferLowWatermark.setMaximum(std::numeric_limits<int>::max());
    m_spinBoxSendBufferLowWatermark.setSuffix(tr(" KiB"));
    m_spinBoxSendBufferLowWatermark.setValue(session->sendBufferLowWatermark());
    addRow(SEND_BUF_LOW_WATERMARK, (tr("Send buffer low watermark") + ' ' + makeLink("https://www.libtorrent.org/reference-Settings.html#send_buffer_low_watermark", "(?)"))
            , &m_spinBoxSendBufferLowWatermark);
    m_spinBoxSendBufferWatermarkFactor.setMinimum(1);
    m_spinBoxSendBufferWatermarkFactor.setMaximum(std::numeric_limits<int>::max());
    m_spinBoxSendBufferWatermarkFactor.setSuffix(" %");
    m_spinBoxSendBufferWatermarkFactor.setValue(session->sendBufferWatermarkFactor());
    addRow(SEND_BUF_WATERMARK_FACTOR, (tr("Send buffer watermark factor") + ' ' + makeLink("https://www.libtorrent.org/reference-Settings.html#send_buffer_watermark_factor", "(?)"))
            , &m_spinBoxSendBufferWatermarkFactor);
    // Socket listen backlog size
    m_spinBoxSocketBacklogSize.setMinimum(1);
    m_spinBoxSocketBacklogSize.setMaximum(std::numeric_limits<int>::max());
    m_spinBoxSocketBacklogSize.setValue(session->socketBacklogSize());
    addRow(SOCKET_BACKLOG_SIZE, (tr("Socket backlog size") + ' ' + makeLink("https://www.libtorrent.org/reference-Settings.html#listen_queue_size", "(?)"))
            , &m_spinBoxSocketBacklogSize);
    // Save resume data interval
    m_spinBoxSaveResumeDataInterval.setMinimum(0);
    m_spinBoxSaveResumeDataInterval.setMaximum(std::numeric_limits<int>::max());
    m_spinBoxSaveResumeDataInterval.setValue(session->saveResumeDataInterval());
    updateSaveResumeDataIntervalSuffix(m_spinBoxSaveResumeDataInterval.value());
    addRow(SAVE_RESUME_DATA_INTERVAL, tr("Save resume data interval", "How often the fastresume file is saved."), &m_spinBoxSaveResumeDataInterval);
    // Outgoing port Min
    m_spinBoxOutgoingPortsMin.setMinimum(0);
    m_spinBoxOutgoingPortsMin.setMaximum(65535);
    m_spinBoxOutgoingPortsMin.setValue(session->outgoingPortsMin());
    addRow(OUTGOING_PORT_MIN, tr("Outgoing ports (Min) [0: Disabled]"), &m_spinBoxOutgoingPortsMin);
    // Outgoing port Min
    m_spinBoxOutgoingPortsMax.setMinimum(0);
    m_spinBoxOutgoingPortsMax.setMaximum(65535);
    m_spinBoxOutgoingPortsMax.setValue(session->outgoingPortsMax());
    addRow(OUTGOING_PORT_MAX, tr("Outgoing ports (Max) [0: Disabled]"), &m_spinBoxOutgoingPortsMax);
    // uTP-TCP mixed mode
    m_comboBoxUtpMixedMode.addItems({tr("Prefer TCP"), tr("Peer proportional (throttles TCP)")});
    m_comboBoxUtpMixedMode.setCurrentIndex(static_cast<int>(session->utpMixedMode()));
    addRow(UTP_MIX_MODE, (tr("%1-TCP mixed mode algorithm", "uTP-TCP mixed mode algorithm").arg(C_UTP)
            + ' ' + makeLink("https://www.libtorrent.org/reference-Settings.html#mixed_mode_algorithm", "(?)"))
            , &m_comboBoxUtpMixedMode);
    // multiple connections per IP
    m_checkBoxMultiConnectionsPerIp.setChecked(session->multiConnectionsPerIpEnabled());
    addRow(MULTI_CONNECTIONS_PER_IP, tr("Allow multiple connections from the same IP address"), &m_checkBoxMultiConnectionsPerIp);
    // Recheck completed torrents
    m_checkBoxRecheckCompleted.setChecked(pref->recheckTorrentsOnCompletion());
    addRow(RECHECK_COMPLETED, tr("Recheck torrents on completion"), &m_checkBoxRecheckCompleted);
    // Transfer list refresh interval
    m_spinBoxListRefresh.setMinimum(30);
    m_spinBoxListRefresh.setMaximum(99999);
    m_spinBoxListRefresh.setValue(session->refreshInterval());
    m_spinBoxListRefresh.setSuffix(tr(" ms", " milliseconds"));
    addRow(LIST_REFRESH, tr("Transfer list refresh interval"), &m_spinBoxListRefresh);
    // Resolve Peer countries
    m_checkBoxResolveCountries.setChecked(pref->resolvePeerCountries());
    addRow(RESOLVE_COUNTRIES, tr("Resolve peer countries (GeoIP)"), &m_checkBoxResolveCountries);
    // Resolve peer hosts
    m_checkBoxResolveHosts.setChecked(pref->resolvePeerHostNames());
    addRow(RESOLVE_HOSTS, tr("Resolve peer host names"), &m_checkBoxResolveHosts);
    // Super seeding
    m_checkBoxSuperSeeding.setChecked(session->isSuperSeedingEnabled());
    addRow(SUPER_SEEDING, (tr("Strict super seeding") + ' ' + makeLink("https://www.libtorrent.org/reference-Settings.html#strict_super_seeding", "(?)"))
            , &m_checkBoxSuperSeeding);
    // Network interface
    m_comboBoxInterface.addItem(tr("Any interface", "i.e. Any network interface"));
    const QString currentInterface = session->networkInterface();
    bool interfaceExists = currentInterface.isEmpty();
    int i = 1;
    for (const QNetworkInterface &iface : asConst(QNetworkInterface::allInterfaces())) {
        m_comboBoxInterface.addItem(iface.humanReadableName(), iface.name());
        if (!currentInterface.isEmpty() && (iface.name() == currentInterface)) {
            m_comboBoxInterface.setCurrentIndex(i);
            interfaceExists = true;
        }
        ++i;
    }
    // Saved interface does not exist, show it anyway
    if (!interfaceExists) {
        m_comboBoxInterface.addItem(session->networkInterfaceName(), currentInterface);
        m_comboBoxInterface.setCurrentIndex(i);
    }
    addRow(NETWORK_IFACE, tr("Network Interface (requires restart)"), &m_comboBoxInterface);
    // Network interface address
    updateInterfaceAddressCombo();
    addRow(NETWORK_IFACE_ADDRESS, tr("Optional IP Address to bind to (requires restart)"), &m_comboBoxInterfaceAddress);
    // Listen on IPv6 address
    m_checkBoxListenIPv6.setChecked(session->isIPv6Enabled());
    addRow(NETWORK_LISTEN_IPV6, tr("Listen on IPv6 address (requires restart)"), &m_checkBoxListenIPv6);
    // Announce IP
    m_lineEditAnnounceIP.setText(session->announceIP());
    addRow(ANNOUNCE_IP, tr("IP Address to report to trackers (requires restart)"), &m_lineEditAnnounceIP);

    // Program notifications
    const MainWindow *const mainWindow = static_cast<Application*>(QCoreApplication::instance())->mainWindow();
    m_checkBoxProgramNotifications.setChecked(mainWindow->isNotificationsEnabled());
    addRow(PROGRAM_NOTIFICATIONS, tr("Display notifications"), &m_checkBoxProgramNotifications);
    // Torrent added notifications
    m_checkBoxTorrentAddedNotifications.setChecked(mainWindow->isTorrentAddedNotificationsEnabled());
    addRow(TORRENT_ADDED_NOTIFICATIONS, tr("Display notifications for added torrents"), &m_checkBoxTorrentAddedNotifications);
    // Download tracker's favicon
    m_checkBoxTrackerFavicon.setChecked(mainWindow->isDownloadTrackerFavicon());
    addRow(DOWNLOAD_TRACKER_FAVICON, tr("Download tracker's favicon"), &m_checkBoxTrackerFavicon);
    // Save path history length
    m_spinBoxSavePathHistoryLength.setRange(AddNewTorrentDialog::minPathHistoryLength, AddNewTorrentDialog::maxPathHistoryLength);
    m_spinBoxSavePathHistoryLength.setValue(AddNewTorrentDialog::savePathHistoryLength());
    addRow(SAVE_PATH_HISTORY_LENGTH, tr("Save path history length"), &m_spinBoxSavePathHistoryLength);
    // Enable speed graphs
    m_checkBoxSpeedWidgetEnabled.setChecked(pref->isSpeedWidgetEnabled());
    addRow(ENABLE_SPEED_WIDGET, tr("Enable speed graphs"), &m_checkBoxSpeedWidgetEnabled);
    // Tracker State
    m_checkBoxTrackerStatus.setChecked(session->isTrackerEnabled());
    addRow(TRACKER_STATUS, tr("Enable embedded tracker"), &m_checkBoxTrackerStatus);
    // Tracker port
    m_spinBoxTrackerPort.setMinimum(1);
    m_spinBoxTrackerPort.setMaximum(65535);
    m_spinBoxTrackerPort.setValue(pref->getTrackerPort());
    addRow(TRACKER_PORT, tr("Embedded tracker port"), &m_spinBoxTrackerPort);
    // Choking algorithm
    m_comboBoxChokingAlgorithm.addItems({tr("Fixed slots"), tr("Upload rate based")});
    m_comboBoxChokingAlgorithm.setCurrentIndex(static_cast<int>(session->chokingAlgorithm()));
    addRow(CHOKING_ALGORITHM, (tr("Upload slots behavior") + ' ' + makeLink("https://www.libtorrent.org/reference-Settings.html#choking_algorithm", "(?)"))
            , &m_comboBoxChokingAlgorithm);
    // Seed choking algorithm
    m_comboBoxSeedChokingAlgorithm.addItems({tr("Round-robin"), tr("Fastest upload"), tr("Anti-leech")});
    m_comboBoxSeedChokingAlgorithm.setCurrentIndex(static_cast<int>(session->seedChokingAlgorithm()));
    addRow(SEED_CHOKING_ALGORITHM, (tr("Upload choking algorithm") + ' ' + makeLink("https://www.libtorrent.org/reference-Settings.html#seed_choking_algorithm", "(?)"))
            , &m_comboBoxSeedChokingAlgorithm);

#if defined(Q_OS_WIN) || defined(Q_OS_MAC)
    m_checkBoxUpdateCheck.setChecked(pref->isUpdateCheckEnabled());
    addRow(UPDATE_CHECK, tr("Check for software updates"), &m_checkBoxUpdateCheck);
#endif
#if (defined(Q_OS_UNIX) && !defined(Q_OS_MAC))
    m_checkBoxUseIconTheme.setChecked(pref->useSystemIconTheme());
    addRow(USE_ICON_THEME, tr("Use system icon theme"), &m_checkBoxUseIconTheme);
#endif
    // Torrent recheck confirmation
    m_checkBoxConfirmTorrentRecheck.setChecked(pref->confirmTorrentRecheck());
    addRow(CONFIRM_RECHECK_TORRENT, tr("Confirm torrent recheck"), &m_checkBoxConfirmTorrentRecheck);

    // Remove all tags confirmation
    m_checkBoxConfirmRemoveAllTags.setChecked(pref->confirmRemoveAllTags());
    addRow(CONFIRM_REMOVE_ALL_TAGS, tr("Confirm removal of all tags"), &m_checkBoxConfirmRemoveAllTags);

    // Announce to all trackers in a tier
    m_checkBoxAnnounceAllTrackers.setChecked(session->announceToAllTrackers());
    addRow(ANNOUNCE_ALL_TRACKERS, tr("Always announce to all trackers in a tier"), &m_checkBoxAnnounceAllTrackers);

    // Announce to all tiers
    m_checkBoxAnnounceAllTiers.setChecked(session->announceToAllTiers());
    addRow(ANNOUNCE_ALL_TIERS, tr("Always announce to all tiers"), &m_checkBoxAnnounceAllTiers);
}

template <typename T>
void AdvancedSettings::addRow(const int row, const QString &text, T *widget)
{
    auto label = new QLabel(text);
    label->setOpenExternalLinks(true);

    setCellWidget(row, PROPERTY, label);
    setCellWidget(row, VALUE, widget);

    if (std::is_same<T, QCheckBox>::value)
        connect(widget, SIGNAL(stateChanged(int)), this, SIGNAL(settingsChanged()));
    else if (std::is_same<T, QSpinBox>::value)
        connect(widget, SIGNAL(valueChanged(int)), this, SIGNAL(settingsChanged()));
    else if (std::is_same<T, QComboBox>::value)
        connect(widget, SIGNAL(currentIndexChanged(int)), this, SIGNAL(settingsChanged()));
    else if (std::is_same<T, QLineEdit>::value)
        connect(widget, SIGNAL(textChanged(QString)), this, SIGNAL(settingsChanged()));
}
