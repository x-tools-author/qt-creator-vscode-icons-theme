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

#include <optional>

#ifdef Q_OS_WIN32
#include <dwmapi.h>

#include <coreplugin/icore.h>

#include <QAbstractNativeEventFilter>
#include <QColor>
#include <QPalette>
#include <QSysInfo>
#include <QTimer>
#include <QWidget>
#include <QWindow>
#endif

Q_LOGGING_CATEGORY(debugLog, "qtc.nomo.icontheme", QtDebugMsg)

#define VS_CODE_ICON_ROOT QString(":/3rd/vscode-icons/icons")
#define VS_CODE_ICON(name) QString(":/3rd/vscode-icons/icons/%1.svg").arg(name)
#define VS_CODE_FILE_ICON(name) QString(":/3rd/vscode-icons/icons/file_type_%1.svg").arg(name)
#define VS_CODE_FOLDER_ICON(name) QString(":/3rd/vscode-icons/icons/folder_type_%1.svg").arg(name)
#define NOT_VS_CODE_ICON(name) QString(":/resources/%1.svg").arg(name)

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// SVGIconOnOffEngine
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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
        // This function is necessary to create an EMPTY pixmap. It's called always
        // before paint()
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

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// NomoPlatformTheme
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class NomoPlatformTheme : public QPlatformTheme
{
public:
    QPlatformTheme* thePlatformTheme = nullptr;
    QMap<QString, QString> fileIconCache;   // <extension, icon>
    QMap<QString, QString> folderIconCache; // <folder name, icon>

    NomoPlatformTheme(QPlatformTheme* platformTheme)
        : thePlatformTheme(platformTheme)
    {
        QFile supportedExtensionsFile(":/resources/supportedExtensions.json");
        if (supportedExtensionsFile.open(QIODevice::ReadOnly)) {
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

        QFile supportedFoldersFile(":/resources/supportedFolders.json");
        if (supportedFoldersFile.open(QIODevice::ReadOnly)) {
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
    }

    virtual QVariant themeHint(ThemeHint hint) const
    {
        if (hint == QPlatformTheme::PreferFileIconFromTheme) {
            qCDebug(debugLog) << "themeHint" << "QPlatformTheme::PreferFileIconFromTheme";
            return false;
        }
        return thePlatformTheme->themeHint(hint);
    }

    virtual QPlatformMenuItem* createPlatformMenuItem() const
    {
        return thePlatformTheme->createPlatformMenuItem();
    }
    virtual QPlatformMenu* createPlatformMenu() const
    {
        return thePlatformTheme->createPlatformMenu();
    }
    virtual QPlatformMenuBar* createPlatformMenuBar() const
    {
        return thePlatformTheme->createPlatformMenuBar();
    }
    virtual void showPlatformMenuBar() { return thePlatformTheme->showPlatformMenuBar(); }

    virtual bool usePlatformNativeDialog(DialogType type) const
    {
        return thePlatformTheme->usePlatformNativeDialog(type);
    }
    virtual QPlatformDialogHelper* createPlatformDialogHelper(DialogType type) const
    {
        return thePlatformTheme->createPlatformDialogHelper(type);
    }

    virtual QPlatformSystemTrayIcon* createPlatformSystemTrayIcon() const
    {
        return thePlatformTheme->createPlatformSystemTrayIcon();
    }
    virtual Qt::ColorScheme colorScheme() const { return thePlatformTheme->colorScheme(); }

    virtual const QPalette* palette(Palette type = SystemPalette) const
    {
        return thePlatformTheme->palette(type);
    }

    virtual const QFont* font(Font type = SystemFont) const { return thePlatformTheme->font(type); }

    virtual QPixmap standardPixmap(StandardPixmap sp, const QSizeF& size) const
    {
        qCDebug(debugLog) << "standardPixmap" << sp << size;

        static QHash<StandardPixmap, QIcon>
            standardPixMapHash{{QPlatformTheme::DirIcon, QIcon(VS_CODE_ICON("default_folder"))},
                               {QPlatformTheme::DirOpenIcon,
                                QIcon(VS_CODE_ICON("default_folder_opened"))},
                               {QPlatformTheme::FileIcon, QIcon(VS_CODE_ICON("default_file"))}};

        if (standardPixMapHash.contains(sp)) {
            return standardPixMapHash.value(sp).pixmap(size.width(), size.height());
        }

        return thePlatformTheme->standardPixmap(sp, size);
    }

    virtual QIcon fileIcon(const QFileInfo& fileInfo,
                           QPlatformTheme::IconOptions iconOptions = {}) const
    {
        qCDebug(debugLog) << "fileIcon" << fileInfo.filePath();

        if (fileInfo.isDir()) {
            if (fileInfo.fileName() == QString(".qtcreator")) {
                return QIcon(NOT_VS_CODE_ICON("qt"));
            }

            QString folderName = fileInfo.fileName().toLower();
            QString iconName = folderIconCache.value(folderName.toLower());
            QString iconFile = VS_CODE_ICON_ROOT + QString("/folder_type_%1.svg").arg(iconName);
            if (QFile::exists(iconFile)) {
                QIcon folderIcon(iconFile);
                QString openedIconFile = VS_CODE_ICON_ROOT
                                         + QString("/folder_type_%1_opened.svg").arg(iconName);
                if (QFile::exists(openedIconFile)) {
                    folderIcon.addFile(openedIconFile, QSize(), QIcon::Normal, QIcon::On);
                }
                return folderIcon;
            } else {
                QIcon folderIcon(VS_CODE_ICON("default_folder"));
                QString opened = VS_CODE_ICON("default_folder_opened");
                if (QFile::exists(opened)) {
                    folderIcon.addFile(opened, QSize(), QIcon::Normal, QIcon::On);
                }
                return folderIcon;
            }
        }

        if (fileInfo.isFile()) {
            // Full file name match first, such as "Makefile", "Dockerfile", "CMakeLists.txt"
            if (fileIconCache.contains(fileInfo.fileName())) {
                const QString iconName = fileIconCache.value(fileInfo.fileName());
                auto iconFile = QString(":/3rd/vscode-icons/icons/file_type_%1.svg").arg(iconName);
                if (QFile::exists(iconFile)) {
                    return QIcon(iconFile);
                }
            }

            // Then try complete suffix match, such as "xxx.knip.json" -> "knip.json"
            QString suffix = fileInfo.completeSuffix();
            if (!fileIconCache.contains(suffix.toLower())) {
                // Try only the last suffix, such as "xxx.cpp" -> "cpp"
                suffix = fileInfo.suffix();
            }

            const QString iconName = fileIconCache.value(suffix.toLower());
            auto iconFile = QString(":/3rd/vscode-icons/icons/file_type_%1.svg").arg(iconName);
            if (QFile::exists(iconFile)) {
                return QIcon(iconFile);
            } else {
                QString tmp = QString(":/res/%1.svg").arg(iconName);
                if (QFile::exists(tmp)) {
                    return QIcon(tmp);
                }
            }
        }
        return QIcon(":/3rd/vscode-icons/icons/default_file.svg");
    }

    virtual QIconEngine* createIconEngine(const QString& iconName) const
    {
        qCDebug(debugLog) << "createIconEngine" << iconName;

        // The "folder-hack.svg" was needed so that QIconLoader would miss "folder.svg" as
        // part of the theme and then try to get the QPlatformTheme IconEngine to load it.
        // All of this was required due to the QThemeIconEngine doesn't associate QIcon::On states.
        if (iconName == "folder") {
            return new SVGIconOffOnEngine(VS_CODE_FOLDER_ICON("default_folder"),
                                          VS_CODE_FOLDER_ICON("default_folder_opened"));
        }
        if (iconName == "text-x-generic") {
            return new SVGIconOffOnEngine(VS_CODE_FILE_ICON("default_file"), QString());
        }

        return thePlatformTheme->createIconEngine(iconName);
    }

    virtual QList<QKeySequence> keyBindings(QKeySequence::StandardKey key) const
    {
        return thePlatformTheme->keyBindings(key);
    }

    virtual QString standardButtonText(int button) const
    {
        return thePlatformTheme->standardButtonText(button);
    }
    virtual QKeySequence standardButtonShortcut(int button) const
    {
        return thePlatformTheme->standardButtonShortcut(button);
    }
    virtual void requestColorScheme(Qt::ColorScheme scheme)
    {
        return thePlatformTheme->requestColorScheme(scheme);
    }
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// IconThemePlugin
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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
                HWND hwnd = msg->hwnd;
                QWidget* w = QWidget::find((WId) hwnd);
                if (w) {
                    updateWindowTheme(w);
                }
            }
        }
#endif

        return false;
    }
};
#endif

class IconThemePlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "VSCodeIconsTheme.json")

    NomoPlatformTheme nomoPlatformTheme;

public:
    explicit IconThemePlugin()
        : nomoPlatformTheme(QGuiApplicationPrivate::platformTheme())
    {
        Q_INIT_RESOURCE(resources);

        // Code needed to get asked about Icons from the QGuiApplication
        QGuiApplicationPrivate::platform_theme = &nomoPlatformTheme;

        if (Utils::HostOsInfo::isMacHost())
            QIcon::setThemeName("qt-creator-vscode-icons-theme");
    }

    void initialize() override
    {
        // Nothing to do yet...
    }

    void extensionsInitialized() override
    {
#ifdef Q_OS_WIN32
        QTimer* timer = new QTimer(this);
        timer->setInterval(100);
        connect(timer, &QTimer::timeout, this, [timer, this]() {
            QWidget* widget = Core::ICore::mainWindow();
            if (widget) {
                ::updateWindowTheme(widget);
                ::registerIcons();
                timer->stop();
                timer->deleteLater();
            }
        });
        timer->start();
        qApp->installNativeEventFilter(new NativeEventFilter());
#endif
    }

    ShutdownFlag aboutToShutdown() override
    {
        QGuiApplicationPrivate::platform_theme = nomoPlatformTheme.thePlatformTheme;
        return SynchronousShutdown;
    }
};

#include <plugin.moc>
