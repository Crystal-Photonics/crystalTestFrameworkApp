#include "exceptiontalapprovaldialog.h"
#include "data_engine/exceptionalapproval.h"
#include "dummydatacreator.h"
#include "ui_exceptiontalapprovaldialog.h"
#include <QMessageBox>

ExceptiontalApprovalDialog::ExceptiontalApprovalDialog(const QList<ExceptionalApproval> &approvals_, const QList<FailedField> &failed_fields_, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ExceptiontalApprovalDialog)
    , failed_fields{failed_fields_}
    , approvals{approvals_} {
    ui->setupUi(this);
    const Qt::ItemFlags item_flags_editable = Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable;
    QStringList approvals_str{};
    for (auto apr : approvals) {
        approvals_str.append(apr.description);
    }
    ui->tree_failures->clear();
    ui->tree_failures->setItemDelegateForColumn(0, new NoEditDelegate(this));
    ui->tree_failures->setItemDelegateForColumn(1, new NoEditDelegate(this));
    ui->tree_failures->setItemDelegateForColumn(2, new NoEditDelegate(this));
    ui->tree_failures->setItemDelegateForColumn(3, new NoEditDelegate(this));
    ui->tree_failures->setItemDelegateForColumn(4, new ComboBoxDelegate(approvals_str, this));

    for (auto ff : failed_fields) {
        QStringList sl;
        sl.append(ff.id);
        QString s = ff.instance_caption;
        if (s.count()) {
            s = "(" + ff.instance_caption + ")";
        }
        sl.append(ff.description + s);
        sl.append(ExceptionalApprovalDB::get_failure_text(ff));
        sl.append("");
        sl.append("");
        QTreeWidgetItem *item = new QTreeWidgetItem(sl);
        item->setFlags(item_flags_editable);
        item->setCheckState(3, Qt::Unchecked);
        if (ff.data_entry) {
            auto ea_result = ff.data_entry->get_exceptional_approval();
            if (ea_result.approved) {
                item->setCheckState(3, Qt::Checked);
            }
            item->setText(4, ea_result.exceptional_approval.description);
            ui->edt_approved_by->setText(ea_result.approving_operator_name);
        }
        ui->tree_failures->addTopLevelItem(item);
    }
    for (int i = 0; i < ui->tree_failures->columnCount(); i++) {
        ui->tree_failures->resizeColumnToContents(i);
    }
}

ExceptiontalApprovalDialog::~ExceptiontalApprovalDialog() {
    delete ui;
}

QList<ExceptionalApprovalResult> ExceptiontalApprovalDialog::get_exceptiontal_approval_results() {
    return exceptiontal_approval_results;
}

void ExceptiontalApprovalDialog::on_ExceptiontalApprovalDialog_accepted() {
    exceptiontal_approval_results.clear();
    for (int i = 0; i < ui->tree_failures->topLevelItemCount(); i++) {
        QTreeWidgetItem *item = ui->tree_failures->topLevelItem(i);

        if (item->checkState(3) == Qt::Checked) {
            ExceptionalApprovalResult r{};
            r.failed_field = failed_fields[i];
            r.approving_operator_name = ui->edt_approved_by->text();
            r.approved = true;
            bool found = false;
            for (auto a : approvals) {
                if (a.description == item->text(4)) {
                    found = true;
                    r.exceptional_approval = a;
                    break;
                }
            }
            if (found) {
                exceptiontal_approval_results.append(r);
            }
        }
    }
}

void ExceptiontalApprovalDialog::on_buttonBox_accepted() {
    QStringList errors;
    QStringList warnings;
    if (ui->edt_approved_by->text().count() == 0) {
        errors.append("No responsible person is specified for an exceptional approval in the approved by field.");
    }
    for (int i = 0; i < ui->tree_failures->topLevelItemCount(); i++) {
        QTreeWidgetItem *item = ui->tree_failures->topLevelItem(i);
        ExceptionalApprovalResult r{};
        r.failed_field = failed_fields[i];
        if (item->checkState(3) == Qt::Checked) {
            bool found = false;
            for (auto a : approvals) {
                if (a.description == item->text(4)) {
                    found = true;
                    r.exceptional_approval = a;
                    break;
                }
            }
            if (found == false) {
                errors.append(QString{QObject::tr("- There is no exceptional approval is assigned to the field \"%1\"")}.arg(r.failed_field.description + "(" +
                                                                                                                             r.failed_field.id + ")"));
            }
        } else {
            warnings.append(QString{QObject::tr("- The field \"%1\" is not approved")}.arg(r.failed_field.description + "(" + r.failed_field.id + ")"));
        }
    }
    if ((errors.count() == 0) && (warnings.count() == 0)) {
        accept();
        return;
    }
    if (errors.count()) {
        QMessageBox::critical(this, tr("Exceptional approval"),
                              tr("There are still open fields you need to fill for assigning exceptional approvals:\n\n") + errors.join("\n"));
    } else if (warnings.count()) {
        int ret = QMessageBox::warning(this, tr("Exceptional approval"), tr("There are still open fields which are not exceptionally approved:\n\n") +
                                                                             warnings.join("\n") + tr("\n proceed anyway?"),
                                       QMessageBox::Yes | QMessageBox::No);
        if (ret == QMessageBox::Yes) {
            accept();
        }
    }
}

void ExceptiontalApprovalDialog::on_ExceptiontalApprovalDialog_rejected()
{
    exceptiontal_approval_results.clear();
}
