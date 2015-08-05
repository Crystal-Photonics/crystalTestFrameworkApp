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
    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

private:
    Ui::MainWindow *ui;
    bool loadPlugin();
    bool pythonTest();
	comModulInterface *ComModulInterface;
};

#endif // MAINWINDOW_H
