#pragma once
#include <QWidget>

extern "C" {
#include <obs.h>
}

class PreviewSource : public QWidget
{
    Q_OBJECT

public:
    explicit PreviewSource(QWidget *parent = nullptr);
    ~PreviewSource() override;

    void open(obs_source_t *source);
    void setSource(obs_source_t *source);
    void closeWindow();
    bool isOpen() const { return isOpen_; }

    static void clearStoredGeometry();

signals:
    void windowOpened();
    void windowClosed();

protected:
    void showEvent(QShowEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private:
    void createDisplay();
    void destroyDisplay();
    void storeGeometryState();
    void restoreGeometryState();
    static void drawPreview(void *data, uint32_t cx, uint32_t cy);

    obs_display_t *display = nullptr;
    obs_source_t *source = nullptr;
    bool isOpen_ = false;
    QWidget *previewHost = nullptr;
};
