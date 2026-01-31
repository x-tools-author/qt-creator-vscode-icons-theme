/***************************************************************************************************
 * Qt Creator VS Code Icons Theme
 *
 * Copyright (C) 2026-2026 x-tools-author(x-tools@outlook.com). All rights reserved.
 ***************************************************************************************************/
#include <QDir>
#include <QFile>
#include <QIcon>
#include <QIconEngine>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainter>
#include <QSettings>
#include <QStringList>
#include <QSvgRenderer>

#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/private/qiconloader_p.h>
#include <QtGui/qpa/qplatformtheme.h>

#include <extensionsystem/iplugin.h>
#include <utils/fsengine/fileiconprovider.h>
#include <utils/hostosinfo.h>

#ifdef Q_OS_WIN32
#include <dwmapi.h>

#include <coreplugin/icore.h>

#include <QAbstractNativeEventFilter>
#include <QColor>
#include <QPalette>
#include <QTimer>
#include <QWidget>
#include <QWindow>
#endif

Q_LOGGING_CATEGORY(debugLog, "qtc.themes.vs_code_icons", QtDebugMsg)

#define VS_CODE_ICON_ROOT QString(":/3rd/vscode-icons/icons")
#define VS_CODE_ICON(name) QString(":/3rd/vscode-icons/icons/%1.svg").arg(name)
#define VS_CODE_FILE_ICON(name) QString(":/3rd/vscode-icons/icons/file_type_%1.svg").arg(name)
#define VS_CODE_FOLDER_ICON(name) QString(":/3rd/vscode-icons/icons/folder_type_%1.svg").arg(name)
#define NOT_VS_CODE_ICON(name) QString(":/resources/%1.svg").arg(name)

static void updateWindowTheme(QWidget* widget)
{
#ifdef Q_OS_WIN32
    if (widget) {
        QPalette palette = qApp->palette();
        QWindow* window = widget->windowHandle();
        if (!window) {
            return;
        }

        QColor c = palette.color(QPalette::Window);
        COLORREF colorref = c.red() | (c.green() << 8) | (c.blue() << 16);
        // The value of DWMWA_CAPTION_COLOR is 35, visit the website for more information:
        // https://learn.microsoft.com/zh-cn/windows/win32/api/dwmapi/ne-dwmapi-dwmwindowattribute
        DwmSetWindowAttribute((HWND) window->winId(), 35, &colorref, sizeof(colorref));
    }
#else
    Q_UNUSED(widget)
#endif
}

static void registerIcons()
{
    auto registerIcon = [](const QString& iconFile, const QString& mimeType) {
        Utils::FileIconProvider::registerIconForMimeType(QIcon(iconFile), mimeType);
    };

    registerIcon(VS_CODE_FILE_ICON("markdown"), "text/markdown");
    registerIcon(VS_CODE_FILE_ICON("lua"), "text/x-lua");
    registerIcon(VS_CODE_FILE_ICON("json"), "application/json");
    registerIcon(VS_CODE_FILE_ICON("yaml"), "application/x-yaml");
    registerIcon(VS_CODE_FILE_ICON("xml"), "application/xml");
    registerIcon(VS_CODE_FILE_ICON("text-plain"), "text/plain");
    registerIcon(VS_CODE_FILE_ICON("text-html"), "text/html");
    registerIcon(VS_CODE_FILE_ICON("shell"), "application/x-sh");
    registerIcon(VS_CODE_FILE_ICON("python"), "application/x-python-code");
    registerIcon(VS_CODE_FILE_ICON("cpp"), "text/x-c++src");
    registerIcon(VS_CODE_FILE_ICON("cppheader"), "text/x-c++hdr");
    registerIcon(VS_CODE_FILE_ICON("c"), "text/x-csrc");
    registerIcon(VS_CODE_FILE_ICON("javascript"), "application/javascript");
    registerIcon(VS_CODE_FILE_ICON("typescript"), "application/typescript");
    registerIcon(VS_CODE_FILE_ICON("css"), "text/css");
    registerIcon(VS_CODE_FILE_ICON("svg"), "image/svg+xml");
    registerIcon(VS_CODE_FILE_ICON("qml"), "text/x-qml");
    registerIcon(VS_CODE_FILE_ICON("cmake"), "text/x-cmake");
    registerIcon(VS_CODE_FILE_ICON("cmake"), "text/x-cmake-in");
    registerIcon(VS_CODE_FILE_ICON("cmake"), "text/x-cmake-project");

    // Qt
    registerIcon(NOT_VS_CODE_ICON("qt"), "application/vnd.qt.xml.resource");
    registerIcon(NOT_VS_CODE_ICON("qt"), "application/x-designer");
    registerIcon(NOT_VS_CODE_ICON("qt"), "application/x-qt-windows-metadata");
    registerIcon(NOT_VS_CODE_ICON("qt"), "text/x-qdoc");

    // Image
    registerIcon(VS_CODE_FILE_ICON("image"), "image/png");
    registerIcon(VS_CODE_FILE_ICON("image"), "image/jpeg");
    registerIcon(VS_CODE_FILE_ICON("image"), "image/bmp");
    registerIcon(VS_CODE_FILE_ICON("image"), "image/gif");
    registerIcon(VS_CODE_FILE_ICON("image"), "image/tiff");
    registerIcon(VS_CODE_FILE_ICON("image"), "image/webp");
    registerIcon(VS_CODE_FILE_ICON("image"), "image/vnd.microsoft.icon");
    registerIcon(VS_CODE_FILE_ICON("image"), "image/icns");
}

static QIcon getDefaultFolderIcon()
{
    QIcon folderIcon(VS_CODE_ICON("default_folder"));
    QString opened = VS_CODE_ICON("default_folder_opened");
    if (QFile::exists(opened)) {
        folderIcon.addFile(opened, QSize(), QIcon::Normal, QIcon::On);
    }
    return folderIcon;
}

static QIcon getFolderIcon(const QString& folderName)
{
    static QMap<QString, QString> folderIconCache;
    if (folderIconCache.isEmpty()) {
        QFile supportedFoldersFile(":/resources/supportedFolders.json");
        if (!supportedFoldersFile.open(QIODevice::ReadOnly)) {
            qCWarning(debugLog) << "Failed to open supportedFolders.json";
            return getDefaultFolderIcon();
        }

        const QByteArray data = supportedFoldersFile.readAll();
        supportedFoldersFile.close();

        folderIconCache.clear();
        const QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        const QJsonArray rootArray = jsonDoc.array();
        for (const QJsonValue& value : rootArray) {
            const QJsonObject obj = value.toObject();
            const QString icon = obj.value("icon").toString();
            const QJsonArray folders = obj.value("folders").toArray();
            for (const QJsonValue& folder : folders) {
                const QString folderName = folder.toString();
                folderIconCache.insert(folderName, icon);
            }
        }
    }

    // Not VS Code folder icons
    if (folderName.toLower() == QString(".qtcreator")) {
        return QIcon(NOT_VS_CODE_ICON("qt"));
    }

    // VS Code folder icon
    QString iconName = folderIconCache.value(folderName.toLower());
    QString iconFile = VS_CODE_ICON_ROOT + QString("/folder_type_%1.svg").arg(iconName);
    if (QFile::exists(iconFile)) {
        QIcon folderIcon(iconFile);
        QString openedIconFile = VS_CODE_ICON_ROOT;
        openedIconFile += QString("/folder_type_%1_opened.svg").arg(iconName);
        if (QFile::exists(openedIconFile)) {
            folderIcon.addFile(openedIconFile, QSize(), QIcon::Normal, QIcon::On);
        }
        return folderIcon;
    }

    return getDefaultFolderIcon();
}

static QIcon getFileIcon(const QFileInfo& info)
{
    // Load file icon cache from json
    static QMap<QString, QString> fileIconCache;
    if (fileIconCache.isEmpty()) {
        QFile supportedExtensionsFile(":/resources/supportedExtensions.json");
        if (!supportedExtensionsFile.open(QIODevice::ReadOnly)) {
            return QIcon(VS_CODE_ICON("default_file"));
        }

        const QByteArray data = supportedExtensionsFile.readAll();
        supportedExtensionsFile.close();
        fileIconCache.clear();
        const QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        const QJsonArray rootArray = jsonDoc.array();
        for (const QJsonValue& value : rootArray) {
            const QJsonObject obj = value.toObject();
            const QString icon = obj.value("icon").toString();
            const QJsonArray extensions = obj.value("extensions").toArray();
            for (const QJsonValue& extension : extensions) {
                const QString extensionStr = extension.toString();
                fileIconCache.insert(extensionStr, icon);
            }
        }
    }

    QString cookedSuffix;
    if (fileIconCache.contains(info.fileName())) {
        cookedSuffix = info.fileName();
    } else if (info.fileName().startsWith('.') && fileIconCache.contains(info.fileName().mid(1))) {
        cookedSuffix = info.fileName().mid(1);
    } else if (fileIconCache.contains(info.completeSuffix())) {
        cookedSuffix = info.completeSuffix();
    } else if (fileIconCache.contains(info.suffix())) {
        cookedSuffix = info.suffix();
    }

    const QString iconName = fileIconCache.value(cookedSuffix.toLower());
    QString iconFile = VS_CODE_FILE_ICON(iconName);
    if (QFile::exists(iconFile)) {
        return QIcon(iconFile);
    } else {
        QString tmp = QString(":/res/%1.svg").arg(iconName);
        if (QFile::exists(tmp)) {
            return QIcon(tmp);
        }
    }

    return QIcon(VS_CODE_ICON("default_file"));
}

class SVGIconOffOnEngine : public QIconEngine
{
    QByteArray dataOff;
    QByteArray dataOn;

public:
    explicit SVGIconOffOnEngine(const QString& offIcon, const QString& onIcon)
    {
        auto readFile = [](const QString fileName) -> QByteArray {
            if (fileName.isEmpty()) {
                return {};
            }

            QFile file(fileName);
            if (file.open(QIODevice::ReadOnly)) {
                return file.readAll();
            }

            return {};
        };

        dataOff = readFile(offIcon);
        dataOn = readFile(onIcon);
    }

    void paint(QPainter* painter, const QRect& rect, QIcon::Mode mode, QIcon::State state) override
    {
        QSvgRenderer renderer(state == QIcon::Off ? dataOff : dataOn);
        renderer.render(painter, rect);
    }

    QIconEngine* clone() const override { return new SVGIconOffOnEngine(*this); }

    QPixmap pixmap(const QSize& size, QIcon::Mode mode, QIcon::State state) override
    {
        // This function is necessary to create an EMPTY pixmap. It's called always before paint()
        QImage img(size, QImage::Format_ARGB32);
        img.fill(qRgba(0, 0, 0, 0));
        QPixmap pix = QPixmap::fromImage(img, Qt::NoFormatConversion);
        {
            QPainter painter(&pix);
            QRect r(QPoint(0.0, 0.0), size);
            this->paint(&painter, r, mode, state);
        }
        return pix;
    }
};

class VSCodeIconsPlatformTheme : public QPlatformTheme
{
public:
    QPlatformTheme* m_platformTheme = nullptr;

    VSCodeIconsPlatformTheme(QPlatformTheme* platformTheme)
        : m_platformTheme(platformTheme)
    {}

    QVariant themeHint(ThemeHint hint) const override
    {
        if (hint == QPlatformTheme::PreferFileIconFromTheme) {
            qCDebug(debugLog) << "themeHint" << "QPlatformTheme::PreferFileIconFromTheme";
            return false;
        }
        return m_platformTheme->themeHint(hint);
    }

    QPlatformMenuItem* createPlatformMenuItem() const override
    {
        return m_platformTheme->createPlatformMenuItem();
    }

    QPlatformMenu* createPlatformMenu() const override
    {
        return m_platformTheme->createPlatformMenu();
    }

    QPlatformMenuBar* createPlatformMenuBar() const override
    {
        return m_platformTheme->createPlatformMenuBar();
    }

    void showPlatformMenuBar() override { return m_platformTheme->showPlatformMenuBar(); }

    bool usePlatformNativeDialog(DialogType type) const override
    {
        return m_platformTheme->usePlatformNativeDialog(type);
    }

    QPlatformDialogHelper* createPlatformDialogHelper(DialogType type) const override
    {
        return m_platformTheme->createPlatformDialogHelper(type);
    }

    QPlatformSystemTrayIcon* createPlatformSystemTrayIcon() const override
    {
        return m_platformTheme->createPlatformSystemTrayIcon();
    }

    Qt::ColorScheme colorScheme() const override { return m_platformTheme->colorScheme(); }

    const QPalette* palette(Palette type = SystemPalette) const override
    {
        return m_platformTheme->palette(type);
    }

    const QFont* font(Font type = SystemFont) const override { return m_platformTheme->font(type); }

    QPixmap standardPixmap(StandardPixmap sp, const QSizeF& size) const override
    {
        qCDebug(debugLog) << "standardPixmap" << sp << size;

        static QHash<StandardPixmap, QIcon> pixHash;
        if (pixHash.isEmpty()) {
            pixHash.insert(QPlatformTheme::DirIcon, QIcon(VS_CODE_ICON("default_folder")));
            pixHash.insert(QPlatformTheme::DirOpenIcon,
                           QIcon(VS_CODE_ICON("default_folder_opened")));
            pixHash.insert(QPlatformTheme::FileIcon, QIcon(VS_CODE_ICON("default_file")));
        };

        if (pixHash.contains(sp)) {
            return pixHash.value(sp).pixmap(size.width(), size.height());
        }

        return m_platformTheme->standardPixmap(sp, size);
    }

    QIcon fileIcon(const QFileInfo& fileInfo, QPlatformTheme::IconOptions iconOptions) const override
    {
        Q_UNUSED(iconOptions)
        qCDebug(debugLog) << "fileIcon" << fileInfo.filePath();

        if (fileInfo.isDir()) {
            return getFolderIcon(fileInfo.fileName());
        } else if (fileInfo.isFile()) {
            return getFileIcon(fileInfo);
        }

        return QIcon(VS_CODE_ICON("default_file"));
    }

    QIconEngine* createIconEngine(const QString& iconName) const override
    {
        qCDebug(debugLog) << "createIconEngine" << iconName;

        if (iconName == "folder") {
            return new SVGIconOffOnEngine(VS_CODE_FOLDER_ICON("default_folder"),
                                          VS_CODE_FOLDER_ICON("default_folder_opened"));
        }

        if (iconName == "text-x-generic") {
            return new SVGIconOffOnEngine(VS_CODE_FILE_ICON("default_file"), QString());
        }

        return m_platformTheme->createIconEngine(iconName);
    }

    QList<QKeySequence> keyBindings(QKeySequence::StandardKey key) const override
    {
        return m_platformTheme->keyBindings(key);
    }

    QString standardButtonText(int button) const override
    {
        return m_platformTheme->standardButtonText(button);
    }

    QKeySequence standardButtonShortcut(int button) const override
    {
        return m_platformTheme->standardButtonShortcut(button);
    }

    void requestColorScheme(Qt::ColorScheme scheme) override
    {
        return m_platformTheme->requestColorScheme(scheme);
    }
};

#ifdef Q_OS_WIN32
class NativeEventFilter : public QAbstractNativeEventFilter
{
public:
    bool nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) override
    {
#ifdef Q_OS_WIN
        if (eventType == "windows_generic_MSG") {
            MSG* msg = static_cast<MSG*>(message);
            if (msg->message == WM_ACTIVATE) {
                updateWindowTheme(QWidget::find((WId) msg->hwnd));
            }
        }
#endif

        return false;
    }
};
#endif

class VSCodeIconsThemePlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "VSCodeIconsTheme.json")

    VSCodeIconsPlatformTheme m_platformTheme;

public:
    explicit VSCodeIconsThemePlugin()
        : m_platformTheme(QGuiApplicationPrivate::platformTheme())
    {
        Q_INIT_RESOURCE(resources);

        // Code needed to get asked about Icons from the QGuiApplication
        QGuiApplicationPrivate::platform_theme = &m_platformTheme;
        if (Utils::HostOsInfo::isMacHost()) {
            QIcon::setThemeName("qt-creator-vscode-icons-theme");
        }
    }

    void initialize() override
    {
#ifdef Q_OS_WIN32
        QTimer* timer = new QTimer(this);
        timer->setInterval(100);
        connect(timer, &QTimer::timeout, this, [timer, this]() {
            QWidget* widget = Core::ICore::mainWindow();
            if (widget) {
                // Do something after main window found...
                timer->stop();
                timer->deleteLater();
                ::updateWindowTheme(widget);
                ::registerIcons();
            }
        });
        timer->start();
        qApp->installNativeEventFilter(new NativeEventFilter());
#endif
    }

    ShutdownFlag aboutToShutdown() override
    {
        QGuiApplicationPrivate::platform_theme = m_platformTheme.m_platformTheme;
        return SynchronousShutdown;
    }
};

#include <plugin.moc>
