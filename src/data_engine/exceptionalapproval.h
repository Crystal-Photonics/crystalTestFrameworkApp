#ifndef EXCEPTIONALAPPROVAL_H
#define EXCEPTIONALAPPROVAL_H
#include <QList>
#include <QString>

class ExceptionalApproval {
    public:
    int id = 1;
    QString description;
    bool hidden = false;
};

class FailedField {
    public:
    QString id;
    int instance_index;
    QString instance_caption;
    QString description;
    QString desired_value;
    QString actual_value;
};

class ExceptionalApprovalResult {
    public:
    FailedField failed_field;
    ExceptionalApproval exceptional_approval;
    QString approving_operator_name;
 //   QString failure;
};

class ExceptionalApprovalDB {
    public:
    ExceptionalApprovalDB(const QString &file_name);
    const QList<FailedField> &get_failed_fields() const;
    QList<ExceptionalApprovalResult> select_exceptional_approval(QList<FailedField> failed_fields, QWidget *parent);

    static     QString get_failure_text(const FailedField &ff);
private:
    QList<ExceptionalApproval> approvals;
    QList<FailedField> failed_fields;
};

#endif // EXCEPTIONALAPPROVAL_H
