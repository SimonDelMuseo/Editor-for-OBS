#pragma once

#include <QWidget>

class QVBoxLayout;
class QScrollArea;
class FiltersModel;
class FiltersProperRenderer;

class FilterWindow : public QWidget {
    Q_OBJECT

public:
    explicit FilterWindow(QWidget *parent = nullptr);

    void setModel(FiltersModel *model, int filterIndex);
    void updateValues();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    FiltersModel *model = nullptr;
    int index = -1;

    QVBoxLayout *mainLayout = nullptr;
    QScrollArea *scrollArea = nullptr;
    FiltersProperRenderer *properRenderer = nullptr;
};
