#include "exceptiontalapprovaldialog.h"
#include "data_engine/exceptionalapproval.h"
#include "dummydatacreator.h"
#include "ui_exceptiontalapprovaldialog.h"

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
        ui->tree_failures->addTopLevelItem(item);
    }
    for (int i = 0; i < ui->tree_failures->columnCount(); i++) {
        ui->tree_failures->resizeColumnToContents(i);
    }
}

ExceptiontalApprovalDialog::~ExceptiontalApprovalDialog() {
    delete ui;
}

void ExceptiontalApprovalDialog::on_ExceptiontalApprovalDialog_accepted() {
    exceptiontal_approval_results.clear();
    for (int i = 0; i < ui->tree_failures->topLevelItemCount(); i++) {
        QTreeWidgetItem *item = ui->tree_failures->topLevelItem(i);

        if (item->checkState(3) == Qt::Checked) {
            ExceptionalApprovalResult r{};
            r.failed_field = failed_fields[i];
            r.approving_operator_name = ui->edt_approved_by->text();
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
