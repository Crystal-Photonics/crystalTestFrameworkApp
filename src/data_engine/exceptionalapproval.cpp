#include "exceptionalapproval.h"
#include "Windows/exceptiontalapprovaldialog.h"
#include "Windows/mainwindow.h"
#include "util.h"
#include <QByteArray>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
///\cond HIDDEN_SYMBOLS
ExceptionalApprovalDB::ExceptionalApprovalDB(const QString &file_name) {
    QFile loadFile(file_name);
    if (file_name == "") {
        return;
    }
    if (!QFile::exists(file_name)) {
        return;
    }
    if (loadFile.open(QIODevice::ReadOnly)) {
        QByteArray saveData = loadFile.readAll();
        QJsonDocument loadDoc(QJsonDocument::fromJson(saveData));
        QJsonObject obj = loadDoc.object();

        QJsonArray section_order = obj["exceptional_approvals"].toArray();
        for (auto jitem : section_order) {
            QJsonObject obj = jitem.toObject();
            ExceptionalApproval ea{};
            ea.id = obj["id"].toInt();
            ea.description = obj["description"].toString();
            ea.hidden = obj["hidden"].toBool();
            approvals.append(ea);
        }
    }
}

const QList<FailedField> &ExceptionalApprovalDB::get_failed_fields() const {
    return failed_fields;
}

QString ExceptionalApprovalDB::get_failure_text(const FailedField &ff) {
    return ff.desired_value + " ≠ " + ff.actual_value;
}

QList<ExceptionalApprovalResult> ExceptionalApprovalDB::select_exceptional_approval(QList<FailedField> failed_fields, QWidget *parent) {
    QList<ExceptionalApprovalResult> result;

    this->failed_fields = failed_fields;
    if (parent) {
        return Utility::promised_thread_call(MainWindow::mw, [failed_fields, this, parent] {
            if (failed_fields.count()) {
                ExceptiontalApprovalDialog diag{approvals, failed_fields, parent};
                if (diag.exec()) {
                    return diag.get_exceptiontal_approval_results();
                }
            }
            QList<ExceptionalApprovalResult> empty_result;
            return empty_result;
        });

    } else {
        auto a = approvals[0];
        for (auto ff : failed_fields) {
            ExceptionalApprovalResult r{};
            r.failed_field = ff;
            r.exceptional_approval = a;
            r.approving_operator_name = "Operator";
            result.append(r);
        }
    }
    return result;
}

QJsonObject ExceptionalApprovalResult::get_json_dump() const {
    QJsonObject result;
    result["approved"] = approved;
    result["approved_by"] = approving_operator_name;
    result["id"] = exceptional_approval.id;
    result["description"] = exceptional_approval.description;
    return result;
}
/// \endcond
