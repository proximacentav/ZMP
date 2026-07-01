#ifndef ICONBUTTON_H
#define ICONBUTTON_H

#include <QWidget>
#include <QLabel>

class IconButton : public QLabel {
    Q_OBJECT
public:
    explicit IconButton(QWidget *parent = nullptr) : QLabel(parent) {}
protected:
    void mousePressEvent(QMouseEvent *event) override {
        emit clicked();
    }
signals:
    void clicked();
};

#endif // ICONBUTTON_H
