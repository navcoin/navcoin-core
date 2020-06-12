//
// Created by Kolby on 6/19/2019.
//
#pragma once

#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsSceneDragDropEvent>
#include <QGraphicsView>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QListView>
#include <QListWidget>
#include <QMimeData>
#include <QPainter>
#include <QWidget>

class QDragEnterEvent;
class QDropEvent;

namespace Ui {
    class StartOptionsSort;
}

class StartOptionsSort : public QWidget {
    Q_OBJECT

public:
    explicit StartOptionsSort(std::vector<std::string> Words, int rows,
                              QWidget *parent = nullptr);
    ~StartOptionsSort();
    std::list<QString> getOrderedStrings();

    void keyPressEvent(QKeyEvent  *);

private:
    Ui::StartOptionsSort *ui;
    std::list<QListWidget *> labelsList;
    std::list<QGraphicsRectItem *> graphicsList;
    QString m_text;
    QGraphicsView *view;
    QGraphicsScene *scene;
};

class CustomRectItem : public QGraphicsRectItem {
public:
    CustomRectItem(QGraphicsItem *parent = 0);
    void dropEvent(QGraphicsSceneDragDropEvent *event);
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget);
    void setText(const QString &text);
    QString text() const { return m_text; }

private:
    QString m_text;
};
