//
// Created by Kolby on 6/19/2019.
//

#include <guiutil.h>

#include <QLabel>
#include <QLineEdit>
#include <startoptionsrestore.h>
#include <ui_startoptionsrestore.h>

StartOptionsRestore::StartOptionsRestore(QStringList _wordList, int rows,
                                         QWidget *parent)
    : QWidget(parent), ui(new Ui::StartOptionsRestore), wordList(_wordList) {
    ui->setupUi(this);

    QString txt(tr("Restore your wallet using a recovery seed phrase below."));
    ui->restoreLabel->setText(txt);

    for (int i = 0; i < rows; i++) {
        for (int k = 0; k < 6; k++) {
            QLineEdit *label = new QLineEdit(this);
            label->setMinimumSize(80 * GUIUtil::scale(), 36 * GUIUtil::scale());
            label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            label->setContentsMargins(0, 0, 0, 0);
            label->setAlignment(Qt::AlignCenter);
            editList.push_back(label);
            connect(label, SIGNAL(textChanged(const QString &)), this,
                    SLOT(textChanged(const QString &)));
            ui->gridLayoutRevealed->addWidget(label, i, k, Qt::AlignCenter);
        }
    }
}

void StartOptionsRestore::textChanged(const QString &text) {

    QObject *senderObj = sender();
    QLineEdit* label = static_cast<QLineEdit*>(senderObj);
    label->setProperty("class", "");
    if (text != "") {
        label->setProperty("class", "highlight-border");
        if (!wordList.contains(text)) {
            label->setProperty("class", "error-border");
        }
    }
    qApp->setStyleSheet(qApp->styleSheet()); // It's a hack I know :P
}

std::vector<std::string> StartOptionsRestore::getOrderedStrings() {
    std::vector<std::string> list;
    for (QLineEdit *label : editList) {
        list.push_back(label->text().toStdString());
    }
    return list;
}

StartOptionsRestore::~StartOptionsRestore() {
    delete ui;
}
