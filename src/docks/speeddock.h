/*
 * Copyright (c) 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef SPEEDDOCK_H
#define SPEEDDOCK_H

#include <QDockWidget>

class QCheckBox;
class QDoubleSpinBox;
class QListWidget;

class SpeedDock : public QDockWidget
{
    Q_OBJECT

public:
    explicit SpeedDock(QWidget *parent = nullptr);

private slots:
    void onApplyPresetClicked();
    void onApplyConstantClicked();

private:
    QListWidget *m_presetList;
    QDoubleSpinBox *m_speedSpinBox;
    QCheckBox *m_pitchCheckBox;
};

#endif // SPEEDDOCK_H
