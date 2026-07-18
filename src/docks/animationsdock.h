/*
 * Copyright (c) 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef ANIMATIONSDOCK_H
#define ANIMATIONSDOCK_H

#include <QDockWidget>

class QComboBox;
class QListWidget;
class QSpinBox;

class AnimationsDock : public QDockWidget
{
    Q_OBJECT

public:
    explicit AnimationsDock(QWidget *parent = nullptr);

private slots:
    void onApplyClicked();

private:
    QListWidget *m_presetList;
    QComboBox *m_modeCombo;
    QSpinBox *m_durationSpinBox;
};

#endif // ANIMATIONSDOCK_H
