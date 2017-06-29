#include "exceptionalapproval.h"
#include <QByteArray>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

ExceptionalApprovalDB::ExceptionalApprovalDB(QString filename) {
    QFile loadFile(filename);

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
        }
    }
}

QList<ExceptionalApprovalResult> ExceptionalApprovalDB::select_exceptional_approval(QList<FailedField> failed_fields)
{

    QList<ExceptionalApprovalResult> result;

    return result;

}
