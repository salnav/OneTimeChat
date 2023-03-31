#ifndef MYLINEEDIT_H
#define MYLINEEDIT_H

#include <QLineEdit>
#include <QObject>
#include <QWidget>
#include <QMouseEvent>

class MyLineEdit : public QLineEdit
{
    Q_OBJECT

public:
    MyLineEdit(QWidget *parent = nullptr) : QLineEdit(parent) {}
    QString text() const { return QLineEdit::text(); } // new member function
    void mousePressEvent(QMouseEvent *event) override;
    void setFixedSize(const QSize& size);
signals:
    void clicked();

};

#endif // MYLINEEDIT_H
