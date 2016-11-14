#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "commodulinterface.h"

namespace Ui {
class MainWindow;
}
#include "export.h"

class EXPORT MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
	void on_actionPaths_triggered();

	void on_device_detect_button_clicked();

private:
	QDialog *path_dialog = nullptr;
    Ui::MainWindow *ui;
    bool loadPlugin();
	comModulInterface *ComModulInterface;
};

#endif // MAINWINDOW_H
