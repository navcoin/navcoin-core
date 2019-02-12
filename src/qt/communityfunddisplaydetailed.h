#ifndef COMMUNITYFUNDDISPLAYDETAILED_H
#define COMMUNITYFUNDDISPLAYDETAILED_H

#include <QWidget>

namespace Ui {
class CommunityFundDisplayDetailed;
}

class CommunityFundDisplayDetailed : public QWidget
{
    Q_OBJECT

public:
    explicit CommunityFundDisplayDetailed(QWidget *parent = 0);
    ~CommunityFundDisplayDetailed();

private:
    Ui::CommunityFundDisplayDetailed *ui;
};

#endif // COMMUNITYFUNDDISPLAYDETAILED_H
