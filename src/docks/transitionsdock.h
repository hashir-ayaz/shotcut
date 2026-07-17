/*
 * Copyright (c) 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef TRANSITIONSDOCK_H
#define TRANSITIONSDOCK_H

#include <QDockWidget>

class QListWidget;
class QSpinBox;

class TransitionsDock : public QDockWidget
{
    Q_OBJECT

public:
    explicit TransitionsDock(QWidget *parent = nullptr);

private slots:
    void onApplyClicked();

private:
    QListWidget *m_presetList;
    QSpinBox *m_durationSpinBox;
};

#endif // TRANSITIONSDOCK_H
