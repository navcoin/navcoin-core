#ifndef LISTVIEW_H
#define LISTVIEW_H

class ListView : public QListView {
   void paintEvent(QPaintEvent *e) {
      QListView::paintEvent(e);
      if (model() && model()->rowCount(rootIndex()) > 0) return;
      // The view is empty.
      QPainter p(this->viewport());
      p.drawText(rect(), Qt::AlignCenter, "No transactions");
   }
public:
   ListView(QWidget* parent = 0) : QListView(parent) {}
};

#endif // LISTVIEW_H
