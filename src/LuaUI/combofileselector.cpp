#include "combofileselector.h"
#include <QDir>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QProcess>
#include <QSplitter>
#include <QStandardPaths>
#include <QStringList>
#if 1
//from http://stackoverflow.com/questions/3490336/how-to-reveal-in-finder-or-show-in-explorer-with-qt
void showInGraphicalShell(QWidget *parent, const QString &pathIn) {
// Mac, Windows support folder or file.
#if defined(Q_OS_WIN)
    const QString explorer = QStandardPaths::findExecutable(QLatin1String("explorer.exe"));

    // const QString explorer = QProcessEnvironment::systemEnvironment().searchInPath(QLatin1String("explorer.exe"));
    if (explorer.isEmpty()) {
        QMessageBox::warning(parent, QObject::tr("Launching Windows Explorer failed"),
                             QObject::tr("Could not find explorer.exe in path to launch Windows Explorer."));
        return;
    }
    QString param;
    if (!QFileInfo(pathIn).isDir())
        param = QLatin1String("/select,");
    param += QDir::toNativeSeparators(pathIn);
    QString command = explorer + " " + param;
    QProcess::startDetached(command);
#elif defined(Q_OS_MAC)
    Q_UNUSED(parent)
    QStringList scriptArgs;
    scriptArgs << QLatin1String("-e") << QString::fromLatin1("tell application \"Finder\" to reveal POSIX file \"%1\"").arg(pathIn);
    QProcess::execute(QLatin1String("/usr/bin/osascript"), scriptArgs);
    scriptArgs.clear();
    scriptArgs << QLatin1String("-e") << QLatin1String("tell application \"Finder\" to activate");
    QProcess::execute("/usr/bin/osascript", scriptArgs);
#else
    // we cannot select a file here, because no file browser really supports it...
    const QFileInfo fileInfo(pathIn);
    const QString folder = fileInfo.absoluteFilePath();
    const QString app = Utils::UnixUtils::fileBrowser(Core::ICore::instance()->settings());
    QProcess browserProc;
    const QString browserArgs = Utils::UnixUtils::substituteFileBrowserParameters(app, folder);
    if (debug)
        qDebug() << browserArgs;
    bool success = browserProc.startDetached(browserArgs);
    const QString error = QString::fromLocal8Bit(browserProc.readAllStandardError());
    success = success && error.isEmpty();
    if (!success)
        showGraphicalShellError(parent, app, error);
#endif
}
#endif

ComboBoxFileSelector::ComboBoxFileSelector(QSplitter *parent, const std::string &directory, const sol::table &filter) {
    this->parent = parent;
    base_widget = new QWidget(parent);
    combobox = new QComboBox(base_widget);
    button = new QPushButton(base_widget);
    QHBoxLayout *layout = new QHBoxLayout();

    layout->addWidget(combobox);
    layout->addWidget(button);
    base_widget->setLayout(layout);
    parent->addWidget(base_widget);

    button->setText("explore..");
    base_widget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    combobox->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    button->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Minimum);

    current_directory = QString::fromStdString(directory);
    button_clicked_connection = QObject::connect(button, &QPushButton::pressed, [this] { showInGraphicalShell(this->parent, this->current_directory); });

    for (auto &filt : filter) {
        filters.append(QString::fromStdString(filt.second.as<std::string>()));
    }

    scan_directory();
    fill_combobox();
}

ComboBoxFileSelector::~ComboBoxFileSelector() {
    base_widget->setEnabled(false);
    QObject::disconnect(button_clicked_connection);
}

std::string ComboBoxFileSelector::get_selected_file() {
    return file_entries[combobox->currentIndex()].filenpath.toStdString();
}

void ComboBoxFileSelector::set_order_by(const std::string &field, const bool ascending) {
    QString order_by = QString::fromStdString(field).toLower().trimmed();
    if (order_by == "name") {
        qSort(file_entries.begin(), file_entries.end(), [ascending](FileEntry &p1, FileEntry &p2) {
            if (ascending) {
                return p1.filename.toLower() < p2.filename.toLower();
            } else {
                return p1.filename.toLower() > p2.filename.toLower();
            }
        });
    } else if (order_by == "date") {
        qSort(file_entries.begin(), file_entries.end(), [ascending](FileEntry &p1, FileEntry &p2) {
            if (ascending) {
                return p1.date < p2.date;
            } else {
                return p1.date > p2.date;
            }
        });
    } else {
        //TODO error
    }
    fill_combobox();
}

void ComboBoxFileSelector::scan_directory() {
    file_entries.clear();
    QString dirname = current_directory;

    QDir dir{dirname};
    dir.setFilter(QDir::Files | QDir::Hidden | QDir::NoSymLinks);
    dir.setSorting(QDir::Time); //QDir::Time // QDir::Name // QDir::Size  | QDir::Reversed
    dir.setNameFilters(filters);
    QFileInfoList list = dir.entryInfoList();

    for (auto &item : list) {
        FileEntry fe;
        fe.filename = item.fileName();
        fe.filenpath = item.filePath();
        fe.date = item.created();
        file_entries.append(fe);
    }
}

void ComboBoxFileSelector::fill_combobox() {
    combobox->clear();
    for (auto &fe : file_entries) {
        combobox->addItem(fe.filename);
    }
}
