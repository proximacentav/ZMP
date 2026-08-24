#ifndef VISUALIZATIONWIDGET_H
#define VISUALIZATIONWIDGET_H

#include <QWidget>
#include <QComboBox>
#include <QVBoxLayout>
#include <QVector>
#include <QPainter>
#include <QOpenGLWidget>
#include <QTimer>
#include "audiomanager.h"
#include "translator.h"

class SpectrogramWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SpectrogramWidget(QWidget *parent = nullptr);
    void setLevels(const QVector<float> &levels, const QVector<double> &frequencies);
protected:
    void paintEvent(QPaintEvent *event) override;
private:
    QVector<float> m_levels;
    QVector<double> m_frequencies;
};

#ifndef ZMP_NO_PROJECTM
class projectM;

class ProjectMWidget : public QOpenGLWidget
{
    Q_OBJECT
public:
    explicit ProjectMWidget(AudioManager *audioManager, QWidget *parent = nullptr);
    ~ProjectMWidget();
    bool isInitialized() const { return m_projectM != nullptr; }
    void loadPresetFile(const QString &filePath);
    void startProjectM();
    void stopVisualizer() { m_timer->stop(); }

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

private:
    void ensureProjectM();
    AudioManager *m_audioManager;
    projectM *m_projectM;
    QTimer *m_timer;
    QString m_pendingPreset;
};
#endif // !ZMP_NO_PROJECTM

class VisualizationWidget : public QWidget
{
    Q_OBJECT
public:
    explicit VisualizationWidget(AudioManager *audioManager, QWidget *parent = nullptr);
    void updateSpectrum(const QVector<float> &levels, const QVector<double> &frequencies);
    void loadProjectMPreset(const QString &filePath);

private:
    AudioManager *m_audioManager;
    QComboBox *m_modeCombo;
    SpectrogramWidget *m_spectrogram;
#ifndef ZMP_NO_PROJECTM
    ProjectMWidget *m_projectMWidget;
#endif
    RetransList m_retrans;
    void retranslateUi();
};

#endif
