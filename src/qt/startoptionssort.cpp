//
// Created by Kolby on 6/19/2019.
//

#include <QLineEdit>
#include <QListView>
#include <QListWidget>
#include <QStringListModel>
#include <QTableWidget>
#include <QtWidgets>
#include <algorithm>
#include <chrono>
#include <random>
#include <startoptionssort.h>
#include <ui_startoptionssort.h>

CustomRectItem::CustomRectItem(QGraphicsItem *parent)
        : QGraphicsRectItem(parent) {
    setAcceptDrops(true);
    setFlags(QGraphicsItem::ItemIsSelectable);
}

void StartOptionsSort::keyPressEvent(QKeyEvent *event){
    Q_FOREACH(QGraphicsItem *item, scene->selectedItems())
    if(event->key() == Qt::Key_Backspace){

        if(CustomRectItem *rItem = qgraphicsitem_cast<CustomRectItem*> (item)){
            for (auto const& i : labelsList)
            {
                if (i->count() < 4) {
                    i->addItem(rItem->text());
                    rItem->setText(QString());
                    break;
                }
            }
        }
        scene->clearSelection();
    }
    QWidget::keyPressEvent(event);
}

void CustomRectItem::setText(const QString &text) {
    this->m_text = text;
    update();
}

void CustomRectItem::paint(QPainter *painter,
                           const QStyleOptionGraphicsItem *option,
                           QWidget *widget) {
    QGraphicsRectItem::paint(painter, option, widget);
    painter->drawText(boundingRect(), m_text, QTextOption(Qt::AlignCenter));
}

void CustomRectItem::dropEvent(QGraphicsSceneDragDropEvent *event) {
    if (event->mimeData()->hasFormat(
            "application/x-qabstractitemmodeldatalist")) {
        QByteArray itemData =
                event->mimeData()->data("application/x-qabstractitemmodeldatalist");
        QDataStream stream(&itemData, QIODevice::ReadOnly);
        int row, col;
        QMap<int, QVariant> valueMap;
        stream >> row >> col >> valueMap;
        QString origText = m_text;
        if (!valueMap.isEmpty()) setText(valueMap.value(0).toString());

        event->setDropAction(Qt::MoveAction);
        event->accept();

        if (!origText.isEmpty()) {
            if (QListWidget *lWidget =
                        qobject_cast<QListWidget *>(event->source()))
                lWidget->addItem(origText);
        }
    } else
        event->ignore();
}

StartOptionsSort::StartOptionsSort(std::vector<std::string> Words, int rows,
                                   QWidget *parent)
        : QWidget(parent), ui(new Ui::StartOptionsSort) {
    ui->setupUi(this);

    ui->dragdropLabel->setText(
            tr("Please <b>drag</b> and <b>drop</b> your seed words into the "
               "correct order to confirm your recovery phrase. "));
    ui->dragdropLabel2->setText(
            tr("To remove words click the word then hit backspace."));
    scene = new QGraphicsScene(this);
    if (rows == 4) {
        scene->setSceneRect(0, 0, 620, 229);
        view = new QGraphicsView;
        view->setScene(scene);
        view->setMinimumSize(QSize(1, 250));
        view->setContentsMargins(0, 0, 0, 2);
    } else {
        scene->setSceneRect(0, 0, 620, 154);
        view = new QGraphicsView;
        view->setScene(scene);
        view->setMinimumSize(QSize(1, 155));
    }

    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QRectF rect(0, 0, 90, 70);
    QRectF rect2(0, 0, 90, 2);

    for (int i = 0; i < rows; i++) {
        for (int k = 0; k < 6; k++) {
            int ki;
            if (k < 1) {
                ki = 10;
            } else {
                ki = 10 + 100 * k;
            }
            int ii;
            if (i < 1) {
                ii = 1;
            } else {
                ii = 1 + 50 * i;
            }

            CustomRectItem *listView = new CustomRectItem;
            scene->addItem(listView);
            listView->setRect(rect);

            CustomRectItem *listViewBorder = new CustomRectItem;
            scene->addItem(listViewBorder);
            listViewBorder->setRect(rect2);
            listViewBorder->setBrush(QColor(117, 120, 162));
            listViewBorder->setPos(ki, ii + 50);
            listViewBorder->setPen(Qt::NoPen);

            listView->setPos(ki, ii);
            QPen myPen(QColor(117, 120, 162), 2, Qt::MPenStyle);
            listView->setPen(myPen);

            graphicsList.push_back(listView);
            graphicsList.push_back(listViewBorder);
        }
    }

    ui->gridLayoutRevealed->addWidget(view, 0, 0, 1, 6, Qt::AlignCenter);

    // Randomizes the word list
    std::vector<std::string> randomWords = Words;
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    shuffle(randomWords.begin(), randomWords.end(),
            std::default_random_engine(seed));

    for (int k = 0; k < 6; k++) {

        QListWidget *itemListWidget = new QListWidget;
        QStringList itemList;
        for (int i = 0; i < rows; i++) {
            itemList.append(QString::fromStdString(randomWords[k * rows + i]));
        }

        itemListWidget->addItems(itemList);
        itemListWidget->setFixedWidth(120);
        itemListWidget->setMinimumSize(QSize(118, 180));
        itemListWidget->setMaximumSize(QSize(118, 180));
        itemListWidget->setDragEnabled(true);
        itemListWidget->setFocusPolicy(Qt::NoFocus);
        itemListWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        labelsList.push_back(itemListWidget);
        ui->gridLayoutRevealed->addWidget(itemListWidget, 2, k, Qt::AlignCenter);
    }

    setAcceptDrops(true);
    setWindowTitle(tr("Draggable Text"));
}

std::list<QString> StartOptionsSort::getOrderedStrings() {
    std::list<QString> list;
    Q_FOREACH (QGraphicsItem *item, scene->items())
    if (CustomRectItem *rItem =
                qgraphicsitem_cast<CustomRectItem *>(item)) {
        list.push_back(rItem->text());
    }
    return list;
}

StartOptionsSort::~StartOptionsSort() {
    delete ui;
}
