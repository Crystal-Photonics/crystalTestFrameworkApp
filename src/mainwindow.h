#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "CommunicationDevices/communicationdevice.h"
#include "export.h"

#include <QMainWindow>
#include <QString>
#include <memory>
#include <set>
#include <vector>

namespace Ui {
	class MainWindow;
}

class EXPORT MainWindow : public QMainWindow {
    Q_OBJECT

	public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

	private slots:
	void on_actionPaths_triggered();
	void on_device_detect_button_clicked();
	void on_update_devices_list_button_clicked();

	private:
	QDialog *path_dialog = nullptr;
    Ui::MainWindow *ui;
	struct Comp {
		bool operator()(const std::unique_ptr<CommunicationDevice> &lhs, const std::unique_ptr<CommunicationDevice> &rhs) const {
			return lhs->getTarget() < rhs->getTarget();
		}
		bool operator()(const std::unique_ptr<CommunicationDevice> &lhs, const QString &rhs) const {
			return lhs->getTarget() < rhs;
		}
		bool operator()(const QString &lhs, const std::unique_ptr<CommunicationDevice> &rhs) const {
			return lhs < rhs->getTarget();
		}
		typedef void is_transparent;
	};
	std::set<std::unique_ptr<CommunicationDevice>, Comp> devices;
};

#endif // MAINWINDOW_H
