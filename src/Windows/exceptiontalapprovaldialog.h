#ifndef EXCEPTIONTALAPPROVALDIALOG_H
#define EXCEPTIONTALAPPROVALDIALOG_H

#include <QDialog>

class FailedField;
class ExceptionalApprovalResult;
class ExceptionalApproval;

namespace Ui {
    class ExceptiontalApprovalDialog;
}

class ExceptiontalApprovalDialog : public QDialog {
    Q_OBJECT

    public:
    explicit ExceptiontalApprovalDialog(const QList<ExceptionalApproval> &approvals_, const QList<FailedField> &failed_fields_, QWidget *parent = 0);
    ~ExceptiontalApprovalDialog();

    QList<ExceptionalApprovalResult> get_exceptiontal_approval_results();


    private slots:
    void on_ExceptiontalApprovalDialog_accepted();

    void on_buttonBox_accepted();

private:
    Ui::ExceptiontalApprovalDialog *ui;
    QList<ExceptionalApprovalResult> exceptiontal_approval_results;
    QList<FailedField> failed_fields;
    QList<ExceptionalApproval> approvals;
};

#endif // EXCEPTIONTALAPPROVALDIALOG_H
