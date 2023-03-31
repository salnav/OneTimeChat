#include "MyLineEdit.h"
#include <QMouseEvent>


void MyLineEdit::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        emit clicked();
        setStyleSheet("");
        setText("");
    }
    QLineEdit::mousePressEvent(event);
}

void MyLineEdit::setFixedSize(const QSize& size) {
    QLineEdit::setFixedSize(size);
}

// const QMetaObject* MyLineEdit::staticMetaObject() const
// {
//     return QLineEdit::staticMetaObject();
// }
