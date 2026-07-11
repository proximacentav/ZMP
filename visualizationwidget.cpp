#include "visualizationwidget.h"
#include <libprojectM/projectM.hpp>
#include <cmath>
#include <QDebug>
#include <QOpenGLFunctions>
#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QStandardPaths>

// ── SpectrogramWidget ──────────────────────────────────────────────────────

SpectrogramWidget::SpectrogramWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(200);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void SpectrogramWidget::setLevels(const QVector<float> &levels, const QVector<double> &frequencies)
{
    m_levels = levels;
    m_frequencies = frequencies;
    update();
}

static QColor colorForLevel(float t)
{
    t = qBound(0.0f, t, 1.0f);
    int r, g, b;
    if (t < 0.33f) {
        float s = t / 0.33f;
        r = 0;
        g = (int)(s * 230);
        b = (int)(s * 118);
    } else if (t < 0.66f) {
        float s = (t - 0.33f) / 0.33f;
        r = (int)(s * 255);
        g = 230 - (int)(s * 30);
        b = 118 - (int)(s * 118);
    } else {
        float s = (t - 0.66f) / 0.34f;
        r = 255;
        g = 200 - (int)(s * 200);
        b = 0;
    }
    return QColor(r, g, b);
}

void SpectrogramWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);

    int w = width();
    int h = height();
    if (w <= 0 || h <= 0) return;

    p.fillRect(rect(), QColor("#111111"));

    int n = m_levels.size();
    if (n == 0) return;

    float barWidth = (float)w / n;

    for (int i = 0; i < n; ++i) {
        float level = qBound(0.0f, m_levels[i], 1.0f);
        int barHeight = qMax(1, (int)(level * h));

        float x = i * barWidth;
        float r = barWidth - 0.5f;

        QColor color = colorForLevel(level);
        p.fillRect(QRectF(x, h - barHeight, r, barHeight), color);
    }

    p.setPen(QColor("#555555"));
    QFont font = p.font();
    font.setPointSize(8);
    p.setFont(font);

    static const struct { float freq; const char *label; } marks[] = {
        {0, "0Hz"}, {5000, "5kHz"}, {10000, "10kHz"},
        {15000, "15kHz"}, {20000, "20kHz"}, {25000, "25kHz"}, {30000, "30kHz"}
    };

    double maxFreq = 30000.0;

    for (const auto &mark : marks) {
        double t = mark.freq / maxFreq;
        int xPos = qBound(0, (int)(t * w), w);

        p.setPen(QColor("#404060"));
        p.drawLine(xPos, 0, xPos, h);

        p.setPen(QColor("#8888aa"));
        QRect labelRect(xPos - 30, h - 18, 60, 14);
        p.drawText(labelRect, Qt::AlignCenter, mark.label);
    }
}

// ── ProjectMWidget ─────────────────────────────────────────────────────────

ProjectMWidget::ProjectMWidget(AudioManager *audioManager, QWidget *parent)
    : QOpenGLWidget(parent), m_audioManager(audioManager), m_projectM(nullptr)
{
    setAttribute(Qt::WA_OpaquePaintEvent);

    m_timer = new QTimer(this);
    m_timer->setInterval(30);

    connect(m_timer, &QTimer::timeout, this, [this]() {
        if (!m_projectM) return;
        float pcm[2048];
        int bytes = m_audioManager->getPCMData(pcm, 1024);
        if (bytes > 0) {
            int frames = bytes / (int)(2 * sizeof(float));
            m_projectM->pcm()->addPCMfloat_2ch(pcm, frames);
        }
        update();
    });
}

ProjectMWidget::~ProjectMWidget()
{
    m_timer->stop();
    delete m_projectM;
    m_projectM = nullptr;
}

void ProjectMWidget::initializeGL()
{
}

void ProjectMWidget::resizeGL(int w, int h)
{
    if (m_projectM) {
        m_projectM->projectM_resetGL(w, h);
    }
}

void ProjectMWidget::paintGL()
{
    QOpenGLFunctions *gl = QOpenGLContext::currentContext()->functions();
    gl->glClearColor(0.067f, 0.067f, 0.067f, 1.0f);
    gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (m_projectM) {
        m_projectM->renderFrame();
    }
}

void ProjectMWidget::ensureProjectM()
{
    delete m_projectM;
    m_projectM = nullptr;

    makeCurrent();

    QOpenGLFunctions *gl = QOpenGLContext::currentContext()->functions();
    gl->glClearColor(0.067f, 0.067f, 0.067f, 1.0f);

    std::string presetPath;

    if (!m_pendingPreset.isEmpty()) {
        QFileInfo fi(m_pendingPreset);
        presetPath = fi.absolutePath().toUtf8().constData();
    } else {
        presetPath = "/usr/share/projectM/presets/presets_projectM/";
    }

    projectM::Settings settings;
    settings.meshX = 48;
    settings.meshY = 36;
    settings.fps = 35;
    settings.textureSize = 1024;
    settings.windowWidth = width();
    settings.windowHeight = height();
    settings.presetURL = presetPath;
    settings.datadir = "/usr/share/projectM/";
    settings.titleFontURL = "/usr/share/projectM/fonts/Vera.ttf";
    settings.menuFontURL = "/usr/share/projectM/fonts/VeraMono.ttf";
    settings.smoothPresetDuration = 10;
    settings.presetDuration = 15;
    settings.beatSensitivity = 1.0;
    settings.shuffleEnabled = false;
    settings.softCutRatingsEnabled = false;

    m_projectM = new projectM(settings);
    m_projectM->populatePresetMenu();

    if (!m_pendingPreset.isEmpty()) {
        QFileInfo fi(m_pendingPreset);
        QByteArray pathBytes = m_pendingPreset.toUtf8();
        std::string urlStr(pathBytes.constData());
        unsigned int idx = m_projectM->getPresetIndex(urlStr);

        if (idx < m_projectM->getPlaylistSize()) {
            m_projectM->selectPreset(idx, true);
        } else if (m_projectM->getPlaylistSize() > 0) {
            m_projectM->selectPreset(0, true);
        }

        m_pendingPreset.clear();
    }

    if (m_projectM->getPlaylistSize() == 0) {
        delete m_projectM;
        m_projectM = nullptr;

        projectM::Settings fallbackSettings = settings;
        fallbackSettings.presetURL = "/usr/share/projectM/presets/presets_projectM/";
        m_projectM = new projectM(fallbackSettings);
        m_projectM->populatePresetMenu();
    }

    m_timer->start();
    doneCurrent();
}

void ProjectMWidget::startProjectM()
{
    ensureProjectM();
}

void ProjectMWidget::loadPresetFile(const QString &filePath)
{
    if (filePath.isEmpty() || !QFile::exists(filePath)) return;
    m_pendingPreset = filePath;

    if (isVisible()) {
        ensureProjectM();
    }
}

// ── VisualizationWidget ────────────────────────────────────────────────────

VisualizationWidget::VisualizationWidget(AudioManager *audioManager, QWidget *parent)
    : QWidget(parent), m_audioManager(audioManager)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    m_modeCombo = new QComboBox;
    m_modeCombo->addItem("Ничего");
    m_modeCombo->addItem("Спектрограмма");
    m_modeCombo->addItem("projectM");
    mainLayout->addWidget(m_modeCombo);

    m_spectrogram = new SpectrogramWidget;
    m_spectrogram->hide();
    mainLayout->addWidget(m_spectrogram, 1);

    m_projectMWidget = new ProjectMWidget(m_audioManager, this);
    m_projectMWidget->hide();
    mainLayout->addWidget(m_projectMWidget, 1);

    connect(m_modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        m_spectrogram->setVisible(index == 1);
        if (index == 2) {
            m_projectMWidget->startProjectM();
            m_projectMWidget->show();
        } else {
            m_projectMWidget->hide();
        }
    });

    m_modeCombo->setCurrentIndex(0);
}

void VisualizationWidget::updateSpectrum(const QVector<float> &levels, const QVector<double> &frequencies)
{
    if (m_modeCombo->currentIndex() == 1) {
        m_spectrogram->setLevels(levels, frequencies);
    }
}

void VisualizationWidget::loadProjectMPreset(const QString &filePath)
{
    m_projectMWidget->loadPresetFile(filePath);
}
