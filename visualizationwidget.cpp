#include "visualizationwidget.h"
#include "depsmanager.h"
#include <libprojectM/projectM.hpp>
#include <cmath>
#include <QDebug>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QStandardPaths>

// ---------------------------------------------------------------------------
// projectM 3.x требует fixed-function/compatibility контекст. Но EGL
// отвергает CompatibilityProfile для версий < 3.2 (EGL_BAD_MATCH / 3009),
// поэтому пробуем кандидатов и выбираем первый реально создавшийся контекст.
// ---------------------------------------------------------------------------
static QSurfaceFormat makeGlFormat(int major, int minor, QSurfaceFormat::OpenGLContextProfile prof)
{
    QSurfaceFormat f;
    f.setDepthBufferSize(24);
    f.setStencilBufferSize(8);
    if (major > 0) {
        f.setVersion(major, minor);
        f.setProfile(prof);
    }
    return f;
}

static QSurfaceFormat pickProjectMFormat()
{
    static QSurfaceFormat cached;
    static bool probed = false;
    if (probed)
        return cached;
    probed = true;

    const QSurfaceFormat candidates[] = {
        makeGlFormat(4, 3, QSurfaceFormat::CompatibilityProfile),
        makeGlFormat(3, 3, QSurfaceFormat::CompatibilityProfile),
        makeGlFormat(0, 0, QSurfaceFormat::NoProfile),
    };

    QOffscreenSurface surface;
    surface.create();

    for (const QSurfaceFormat &f : candidates) {
        QOpenGLContext ctx;
        ctx.setFormat(f);
        if (ctx.create() && ctx.isValid()) {
            cached = ctx.format();
            qDebug() << "projectM: using GL format" << cached.majorVersion()
                     << "." << cached.minorVersion()
                     << "profile" << int(cached.profile());
            break;
        }
    }

    return cached;
}

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

    // Формат выбирается пробным созданием контекста (см. pickProjectMFormat)
    setFormat(pickProjectMFormat());

    m_timer = new QTimer(this);
    m_timer->setInterval(30);

    connect(m_timer, &QTimer::timeout, this, [this]() {
        if (!m_projectM || !isVisible())
            return;   // скрытый визуализатор не рендерится вообще
        try {
            float pcm[2048];
            int bytes = m_audioManager->getPCMData(pcm, 1024);
            if (bytes > 0) {
                const int frames = bytes / static_cast<int>(2 * sizeof(float));
                if (frames > 0 && frames * 2 <= 2048)
                    m_projectM->pcm()->addPCMfloat_2ch(pcm, frames);
            }
        } catch (...) {
            qWarning() << "projectM: PCM feed failed";
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
    if (!m_projectM || !isVisible())
        return;
    QOpenGLFunctions *gl = QOpenGLContext::currentContext()->functions();
    gl->glClearColor(0.067f, 0.067f, 0.067f, 1.0f);
    gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    try {
        m_projectM->renderFrame();
    } catch (...) {
        qWarning() << "projectM: renderFrame failed";
    }
}

void ProjectMWidget::ensureProjectM()
{
    delete m_projectM;
    m_projectM = nullptr;
    m_timer->stop();

    // Проверяем наличие пресетов заранее — без них projectM бесполезен
    const QString defaultPresets = "/usr/share/projectM/presets/presets_projectM/";
    QString presetDir = defaultPresets;
    if (!m_pendingPreset.isEmpty()) {
        QFileInfo fi(m_pendingPreset);
        presetDir = fi.absolutePath();
    }

    if (!QDir(presetDir).exists() || QDir(presetDir).entryList({"*.milk"}, QDir::Files).isEmpty()) {
        qCritical() << "projectM: presets not found in" << presetDir;
        DependencyManager::instance()->reportMissingDependencies();
        return;
    }

    try {
        makeCurrent();

        QOpenGLFunctions *gl = QOpenGLContext::currentContext()->functions();
        gl->glClearColor(0.067f, 0.067f, 0.067f, 1.0f);

        std::string presetPath = QDir(presetDir).absolutePath().toUtf8().constData();

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

        if (m_projectM->getPlaylistSize() == 0) {
            delete m_projectM;
            m_projectM = nullptr;

            projectM::Settings fallbackSettings = settings;
            fallbackSettings.presetURL = defaultPresets.toUtf8().constData();
            m_projectM = new projectM(fallbackSettings);
            m_projectM->populatePresetMenu();
        }

        if (!m_pendingPreset.isEmpty() && m_projectM) {
            QFileInfo fi(m_pendingPreset);
            QString presetFileName = fi.fileName();
            QByteArray pathBytes = presetFileName.toUtf8();
            std::string urlStr(pathBytes.constData());
            unsigned int idx = m_projectM->getPresetIndex(urlStr);

            if (idx < m_projectM->getPlaylistSize()) {
                m_projectM->selectPreset(idx, true);
            } else if (m_projectM->getPlaylistSize() > 0) {
                m_projectM->selectPreset(0, true);
                qWarning() << "ProjectM preset not found:" << presetFileName;
            }

            m_pendingPreset.clear();
        }
    } catch (const std::exception &e) {
        qCritical() << "projectM init failed:" << e.what();
        delete m_projectM;
        m_projectM = nullptr;
    } catch (...) {
        qCritical() << "projectM init failed: unknown exception";
        delete m_projectM;
        m_projectM = nullptr;
    }

    doneCurrent();

    if (!m_projectM) {
        DependencyManager::instance()->reportMissingDependencies();
        return;
    }

    m_timer->start();
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
    m_modeCombo->addItem(ztr("Ничего"));
    m_modeCombo->addItem(ztr("Спектрограмма"));
    m_modeCombo->addItem("projectM");
    ztrRegister(m_retrans, [this]{
        m_modeCombo->setItemText(0, ztr("Ничего"));
        m_modeCombo->setItemText(1, ztr("Спектрограмма"));
    });
    mainLayout->addWidget(m_modeCombo);

    m_spectrogram = new SpectrogramWidget;
    m_spectrogram->hide();
    mainLayout->addWidget(m_spectrogram, 1);

#ifndef ZMP_NO_PROJECTM
    m_projectMWidget = new ProjectMWidget(m_audioManager, this);
    m_projectMWidget->hide();
    mainLayout->addWidget(m_projectMWidget, 1);
#endif

    connect(m_modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        m_spectrogram->setVisible(index == 1);
#ifndef ZMP_NO_PROJECTM
        if (index == 2) {
            m_projectMWidget->startProjectM();
            m_projectMWidget->show();
        } else {
            m_projectMWidget->stopVisualizer();
            m_projectMWidget->hide();
        }
#else
        Q_UNUSED(index)
#endif
    });

    m_modeCombo->setCurrentIndex(0);

    connect(&Translator::instance(), &Translator::languageChanged, this, &VisualizationWidget::retranslateUi);
}

void VisualizationWidget::retranslateUi() {
    runRetrans(m_retrans);
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
