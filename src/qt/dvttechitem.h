#ifndef NAVTECHITEM_H
#define NAVTECHITEM_H

#include <QWidget>

namespace Ui {
class dvttechitem;
}

class dvttechitem : public QWidget
{
    Q_OBJECT

public:
    explicit dvttechitem(QWidget *parent = 0);
    ~dvttechitem();
    void setHost(QString hostStr);
    void setName(QString nameStr);

private:
    Ui::dvttechitem *ui;
    QString host;
    QString name;
};

#endif // NAVTECHITEM_H
