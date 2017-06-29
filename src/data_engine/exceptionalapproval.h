#ifndef EXCEPTIONALAPPROVAL_H
#define EXCEPTIONALAPPROVAL_H
#include <QString>
#include <QList>

class ExceptionalApproval
{
public:
    int id=1;
    QString description;
    bool hidden=false;
};

class FailedField
{
public:
    QString id;
    int instance_index;
    QString instance_caption;
    QString description;
    QString desired_value;
    QString actual_value;
};

class ExceptionalApprovalResult
{
public:
    FailedField failed_field;
    ExceptionalApproval exceptional_approval;
    QString approving_operator_name;
    QString dissent;
};


class ExceptionalApprovalDB
{
public:
    ExceptionalApprovalDB(QString filename);

    QList<ExceptionalApprovalResult> select_exceptional_approval(QList<FailedField> failed_fields);
private:
    QList<ExceptionalApproval> approvals;
};



#endif // EXCEPTIONALAPPROVAL_H
