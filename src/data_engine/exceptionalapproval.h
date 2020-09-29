#ifndef EXCEPTIONALAPPROVAL_H
#define EXCEPTIONALAPPROVAL_H
#include <QJsonObject>
#include <QList>
#include <QString>
#include <QWidget>

///\cond HIDDEN_SYMBOLS
class DataEngineDataEntry;

class ExceptionalApproval {
    public:
    int id = 1;
    QString description;
    bool hidden = false;
};

class FailedField {
    public:
    QString id;
    //int instance_index=0;
    QString instance_caption;
    QString description;
    QString desired_value;
    QString actual_value;
    DataEngineDataEntry *data_entry = nullptr;
};

class ExceptionalApprovalResult {
    public:
    FailedField failed_field;
    ExceptionalApproval exceptional_approval;
    QString approving_operator_name;
    bool approved = false;
    QJsonObject get_json_dump() const;
};

class ExceptionalApprovalDB {
    public:
    ExceptionalApprovalDB(const QString &file_name);
    const QList<FailedField> &get_failed_fields() const;
    QList<ExceptionalApprovalResult> select_exceptional_approval(QList<FailedField> failed_fields, QWidget *parent);

    static QString get_failure_text(const FailedField &ff);

    private:
    QList<ExceptionalApproval> approvals;
    QList<FailedField> failed_fields;
};
///\endcond
#endif // EXCEPTIONALAPPROVAL_H
