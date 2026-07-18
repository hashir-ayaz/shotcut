/*
 * Copyright (c) 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef AUDIODOCK_H
#define AUDIODOCK_H

#include <QDockWidget>

class QSpinBox;

class AudioDock : public QDockWidget
{
    Q_OBJECT

public:
    explicit AudioDock(QWidget *parent = nullptr);

private slots:
    void onApplyClicked();

private:
    QSpinBox *m_amountSpinBox;
};

#endif // AUDIODOCK_H
