#include "equalizerwidget.h"
#include "translator.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSlider>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QScrollArea>
#include <QGridLayout>
#include <QDebug>
#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QFileDialog>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QMessageBox>

EqualizerWidget::EqualizerWidget(AudioManager *audioManager, QWidget *parent)
    : QWidget(parent), m_audioManager(audioManager)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    m_powerModeLabel = new QLabel("POWERMODE");
    m_powerModeLabel->setStyleSheet("color: red; font-size: 18px; font-weight: bold;");
    m_powerModeLabel->setAlignment(Qt::AlignCenter);
    m_powerModeLabel->setVisible(false);
    mainLayout->addWidget(m_powerModeLabel);

    QHBoxLayout *resetLayout = new QHBoxLayout;
    m_resetButton = new QPushButton;
    ztrSetText(m_retrans, m_resetButton, "Сбросить EQ");
    resetLayout->addWidget(m_resetButton);
    m_saveButton = new QPushButton;
    ztrSetText(m_retrans, m_saveButton, "Сохранить");
    resetLayout->addWidget(m_saveButton);
    m_saveAsButton = new QPushButton;
    ztrSetText(m_retrans, m_saveAsButton, "Сохранить как");
    resetLayout->addWidget(m_saveAsButton);
    resetLayout->addStretch();
    mainLayout->addLayout(resetLayout);

    QHBoxLayout *preampLayout = new QHBoxLayout;
    preampLayout->addWidget(new QLabel("Preamp (dB):"));
    m_preampSlider = new QSlider(Qt::Horizontal);
    m_preampSlider->setRange(-1000, 1000);
    m_preampSlider->setValue(0);
    m_preampSlider->setTickInterval(100);
    m_preampSlider->setTickPosition(QSlider::TicksBelow);
    preampLayout->addWidget(m_preampSlider);
    m_preampSpinBox = new QSpinBox;
    m_preampSpinBox->setRange(-1000, 1000);
    m_preampSpinBox->setValue(0);
    m_preampSpinBox->setSuffix(" dB");
    preampLayout->addWidget(m_preampSpinBox);
    mainLayout->addLayout(preampLayout);

    QHBoxLayout *speedLayout = new QHBoxLayout;
    speedLayout->addWidget(new QLabel("Speed (x):"));
    m_speedSlider = new QSlider(Qt::Horizontal);
    m_speedSlider->setRange(1, 5000);
    m_speedSlider->setValue(100);
    m_speedSlider->setTickInterval(100);
    m_speedSlider->setTickPosition(QSlider::TicksBelow);
    speedLayout->addWidget(m_speedSlider);
    m_speedSpinBox = new QDoubleSpinBox;
    m_speedSpinBox->setRange(0.01, 50.0);
    m_speedSpinBox->setValue(1.0);
    m_speedSpinBox->setSingleStep(0.01);
    m_speedSpinBox->setDecimals(2);
    m_speedSpinBox->setSuffix("x");
    speedLayout->addWidget(m_speedSpinBox);
    mainLayout->addLayout(speedLayout);

    QHBoxLayout *pitchLayout = new QHBoxLayout;
    pitchLayout->addWidget(new QLabel("Pitch (semitones):"));
    m_pitchSlider = new QSlider(Qt::Horizontal);
    m_pitchSlider->setRange(-150, 150);
    m_pitchSlider->setValue(0);
    m_pitchSlider->setTickInterval(30);
    m_pitchSlider->setTickPosition(QSlider::TicksBelow);
    pitchLayout->addWidget(m_pitchSlider);
    m_pitchSpinBox = new QDoubleSpinBox;
    m_pitchSpinBox->setRange(-15.0, 15.0);
    m_pitchSpinBox->setValue(0.0);
    m_pitchSpinBox->setSingleStep(0.1);
    m_pitchSpinBox->setDecimals(1);
    m_pitchSpinBox->setSuffix(" st");
    pitchLayout->addWidget(m_pitchSpinBox);
    mainLayout->addLayout(pitchLayout);

    m_scrollArea = new QScrollArea;
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    mainLayout->addWidget(m_scrollArea);

    m_scrollContent = new QWidget;
    m_layout = new QGridLayout(m_scrollContent);
    m_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    m_scrollArea->setWidget(m_scrollContent);
    createBands();

    m_modeCombo = new QComboBox;
    m_modeCombo->addItem("Custom");
    m_modeCombo->addItem("Default");
    m_modeCombo->addItem("Bass");
    m_modeCombo->addItem("Treble");
    m_modeCombo->addItem("Pop");
    m_modeCombo->addItem("Dance");
    mainLayout->addWidget(m_modeCombo);

    m_echoCheckBox = new QCheckBox;
    ztrSetText(m_retrans, m_echoCheckBox, "Эхо");
    mainLayout->addWidget(m_echoCheckBox);

    connect(m_resetButton, &QPushButton::clicked, this, &EqualizerWidget::onResetClicked);
    connect(m_saveButton, &QPushButton::clicked, this, &EqualizerWidget::onSavePresetClicked);
    connect(m_saveAsButton, &QPushButton::clicked, this, &EqualizerWidget::onSavePresetAsClicked);
    connect(m_modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &EqualizerWidget::onModeChanged);
    connect(m_preampSlider, &QSlider::valueChanged, this, &EqualizerWidget::onPreampSliderMoved);
    connect(m_preampSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &EqualizerWidget::onPreampSpinBoxChanged);
    connect(m_speedSlider, &QSlider::valueChanged, this, &EqualizerWidget::onSpeedSliderMoved);
    connect(m_speedSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &EqualizerWidget::onSpeedSpinBoxChanged);
    connect(m_pitchSlider, &QSlider::valueChanged, this, &EqualizerWidget::onPitchSliderMoved);
    connect(m_pitchSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &EqualizerWidget::onPitchSpinBoxChanged);
    connect(m_echoCheckBox, &QCheckBox::toggled, this, &EqualizerWidget::onEchoToggled);

    connect(&Translator::instance(), &Translator::languageChanged, this, &EqualizerWidget::retranslateUi);

    // Пользовательские пресеты: сканируем папку при старте
    scanUserPresets();
}

void EqualizerWidget::retranslateUi() {
    runRetrans(m_retrans);
}

void EqualizerWidget::createBands() {
    QVector<double> freqs = {5,7,20,40,75,150,300,350,500,800,1200,1400,2500,4000,5600,6000,10000,11200,13000,16000,19000,20000,22000,23000,25000};
    m_bands.resize(NUM_BANDS);
    for (int i=0; i<NUM_BANDS; ++i) {
        double f = freqs[i];
        QWidget *cell = new QWidget;
        QVBoxLayout *vbox = new QVBoxLayout(cell);
        vbox->setAlignment(Qt::AlignCenter);
        QString label = (f<1000) ? QString::number(f)+" Hz" : QString::number(f/1000,'f',1)+" kHz";
        QLabel *fl = new QLabel(label);
        fl->setAlignment(Qt::AlignCenter);
        vbox->addWidget(fl);
        QSlider *sl = new QSlider(Qt::Vertical);
        sl->setRange(-500, 500);
        sl->setValue(0);
        sl->setTickInterval(100);
        sl->setTickPosition(QSlider::TicksBothSides);
        sl->setFixedHeight(200);
        vbox->addWidget(sl, 0, Qt::AlignHCenter);
        QSpinBox *sb = new QSpinBox;
        sb->setRange(-500, 500);
        sb->setValue(0);
        sb->setSuffix(" dB");
        vbox->addWidget(sb);
        m_layout->addWidget(cell, 0, i);
        m_bands[i] = {f, sl, sb};
        connect(sl, &QSlider::valueChanged, this, [this,i](int v){ onSliderMoved(i,v); });
        connect(sb, QOverload<int>::of(&QSpinBox::valueChanged), this, [this,i](int v){ onSpinBoxChanged(i,v); });
    }
}

void EqualizerWidget::setBandValue(int index, int value) {
    if (index < 0 || index >= m_bands.size()) return;
    m_bands[index].slider->blockSignals(true);
    m_bands[index].spinBox->blockSignals(true);
    m_bands[index].slider->setValue(value);
    m_bands[index].spinBox->setValue(value);
    m_bands[index].slider->blockSignals(false);
    m_bands[index].spinBox->blockSignals(false);
    if (m_audioManager) m_audioManager->setEqualizerGain(index, (float)value);
}

void EqualizerWidget::setPreampValue(int value) {
    m_preampSlider->blockSignals(true);
    m_preampSpinBox->blockSignals(true);
    m_preampSlider->setValue(value);
    m_preampSpinBox->setValue(value);
    m_preampSlider->blockSignals(false);
    m_preampSpinBox->blockSignals(false);
    if (m_audioManager) m_audioManager->setPreampGain((float)value);
}

void EqualizerWidget::setSpeedValue(double value) {
    m_speedSlider->blockSignals(true);
    m_speedSpinBox->blockSignals(true);
    m_speedSlider->setValue((int)(value * 100));
    m_speedSpinBox->setValue(value);
    m_speedSlider->blockSignals(false);
    m_speedSpinBox->blockSignals(false);
    if (m_audioManager) m_audioManager->setPlaybackSpeed(value);
}

void EqualizerWidget::setPitchValue(double value) {
    m_pitchSlider->blockSignals(true);
    m_pitchSpinBox->blockSignals(true);
    m_pitchSlider->setValue((int)(value * 10));
    m_pitchSpinBox->setValue(value);
    m_pitchSlider->blockSignals(false);
    m_pitchSpinBox->blockSignals(false);
    if (m_audioManager) m_audioManager->setPitchShift(value);
}

void EqualizerWidget::applyPreset(const QString &name) {
    m_isApplyingPreset = true;
    if (name == "Default") {
        for (int i = 0; i < NUM_BANDS; ++i)
            setBandValue(i, 0);
        setPreampValue(0);
        setSpeedValue(1.0);
        setPitchValue(0);
    } else if (name == "Bass") {
        QVector<int> gains = {15,15,12,10,8,6,0,-2,-4,-6,-5,-4,-2,0,2,2,4,4,3,2,1,0,-2,-3,-5};
        for (int i = 0; i < NUM_BANDS; ++i)
            setBandValue(i, gains[i]);
        setPreampValue(0);
        setSpeedValue(1.0);
        setPitchValue(0);
    } else if (name == "Treble") {
        QVector<int> gains = {-10,-10,-8,-6,-4,-2,0,0,0,0,0,2,4,8,12,12,10,10,8,6,4,3,2,1,0};
        for (int i = 0; i < NUM_BANDS; ++i)
            setBandValue(i, gains[i]);
        setPreampValue(0);
        setSpeedValue(1.0);
        setPitchValue(0);
    } else if (name == "Pop") {
        QVector<int> gains = {-2,-2,0,1,2,2,1,1,1,1,2,2,3,3,2,2,1,1,0,0,-1,-1,-2,-2,-3};
        for (int i = 0; i < NUM_BANDS; ++i)
            setBandValue(i, gains[i]);
        setPreampValue(0);
        setSpeedValue(1.0);
        setPitchValue(0);
    } else if (name == "Dance") {
        QVector<int> gains = {10,10,8,8,6,4,0,-2,-4,-6,-6,-4,-2,0,4,6,8,8,6,4,2,1,0,-1,-2};
        for (int i = 0; i < NUM_BANDS; ++i)
            setBandValue(i, gains[i]);
        setPreampValue(0);
        setSpeedValue(1.0);
        setPitchValue(0);
    }
    m_isApplyingPreset = false;
}

void EqualizerWidget::onResetClicked() {
    m_modeCombo->setCurrentIndex(1);
}

void EqualizerWidget::onModeChanged(int index) {
    // Пользовательский пресет: путь к файлу лежит в данных элемента
    const QString path = m_modeCombo->itemData(index).toString();
    if (!path.isEmpty()) {
        applyPresetFile(path);
        return;
    }
    QString name = m_modeCombo->currentText();
    if (name != "Custom") {
        applyPreset(name);
    }
}

void EqualizerWidget::onSliderMoved(int idx, int val) {
    m_bands[idx].spinBox->blockSignals(true);
    m_bands[idx].spinBox->setValue(val);
    m_bands[idx].spinBox->blockSignals(false);
    emit bandGainChanged(m_bands[idx].freq, val);
    if (m_audioManager) m_audioManager->setEqualizerGain(idx, (float)val);
    if (!m_isApplyingPreset) m_modeCombo->setCurrentIndex(0);
}
void EqualizerWidget::onSpinBoxChanged(int idx, int val) {
    m_bands[idx].slider->blockSignals(true);
    m_bands[idx].slider->setValue(val);
    m_bands[idx].slider->blockSignals(false);
    emit bandGainChanged(m_bands[idx].freq, val);
    if (m_audioManager) m_audioManager->setEqualizerGain(idx, (float)val);
    if (!m_isApplyingPreset) m_modeCombo->setCurrentIndex(0);
}
void EqualizerWidget::onPreampSliderMoved(int v) {
    m_preampSpinBox->blockSignals(true);
    m_preampSpinBox->setValue(v);
    m_preampSpinBox->blockSignals(false);
    emit preampGainChanged(v);
    if (m_audioManager) m_audioManager->setPreampGain((float)v);
    if (!m_isApplyingPreset) m_modeCombo->setCurrentIndex(0);
}
void EqualizerWidget::onPreampSpinBoxChanged(int v) {
    m_preampSlider->blockSignals(true);
    m_preampSlider->setValue(v);
    m_preampSlider->blockSignals(false);
    emit preampGainChanged(v);
    if (m_audioManager) m_audioManager->setPreampGain((float)v);
    if (!m_isApplyingPreset) m_modeCombo->setCurrentIndex(0);
}
void EqualizerWidget::onSpeedSliderMoved(int v) {
    double speed = v / 100.0;
    m_speedSpinBox->blockSignals(true);
    m_speedSpinBox->setValue(speed);
    m_speedSpinBox->blockSignals(false);
    emit speedChanged(speed);
    if (m_audioManager) m_audioManager->setPlaybackSpeed(speed);
    if (!m_isApplyingPreset) m_modeCombo->setCurrentIndex(0);
}
void EqualizerWidget::onSpeedSpinBoxChanged(double speed) {
    int sliderVal = (int)(speed * 100);
    m_speedSlider->blockSignals(true);
    m_speedSlider->setValue(sliderVal);
    m_speedSlider->blockSignals(false);
    emit speedChanged(speed);
    if (m_audioManager) m_audioManager->setPlaybackSpeed(speed);
    if (!m_isApplyingPreset) m_modeCombo->setCurrentIndex(0);
}
void EqualizerWidget::onPitchSliderMoved(int v) {
    double pitch = v / 10.0;
    m_pitchSpinBox->blockSignals(true);
    m_pitchSpinBox->setValue(pitch);
    m_pitchSpinBox->blockSignals(false);
    emit pitchChanged(pitch);
    if (m_audioManager) m_audioManager->setPitchShift(pitch);
    if (!m_isApplyingPreset) m_modeCombo->setCurrentIndex(0);
}
void EqualizerWidget::onPitchSpinBoxChanged(double pitch) {
    int sliderVal = (int)(pitch * 10);
    m_pitchSlider->blockSignals(true);
    m_pitchSlider->setValue(sliderVal);
    m_pitchSlider->blockSignals(false);
    emit pitchChanged(pitch);
    if (m_audioManager) m_audioManager->setPitchShift(pitch);
    if (!m_isApplyingPreset) m_modeCombo->setCurrentIndex(0);
}

void EqualizerWidget::onEchoToggled(bool enabled) {
    if (m_audioManager) m_audioManager->setEchoEnabled(enabled);
}

void EqualizerWidget::setPowerMode(bool enabled) {
    m_powerMode = enabled;
    m_powerModeLabel->setVisible(enabled);
    updateRanges();
}

void EqualizerWidget::updateRanges() {
    int bandRange = m_powerMode ? 1000000 : 500;
    int preampRange = m_powerMode ? 1000000 : 1000;
    double speedMin = m_powerMode ? 0.000001 : 0.01;
    double speedMax = m_powerMode ? 10000000.0 : 50.0;
    int pitchRange = m_powerMode ? 1000000 : 150;

    for (int i = 0; i < m_bands.size(); ++i) {
        int val = m_bands[i].slider->value();
        m_bands[i].slider->setRange(-bandRange, bandRange);
        m_bands[i].slider->setTickInterval(m_powerMode ? 100000000 : 100);
        m_bands[i].spinBox->setRange(-bandRange, bandRange);
        m_bands[i].slider->setValue(qBound(-bandRange, val, bandRange));
        m_bands[i].spinBox->setValue(qBound(-bandRange, val, bandRange));
    }

    int preampVal = m_preampSlider->value();
    m_preampSlider->setRange(-preampRange, preampRange);
    m_preampSlider->setTickInterval(m_powerMode ? 100000000 : 100);
    m_preampSpinBox->setRange(-preampRange, preampRange);
    m_preampSlider->setValue(qBound(-preampRange, preampVal, preampRange));
    m_preampSpinBox->setValue(qBound(-preampRange, preampVal, preampRange));

    double speedVal = m_speedSpinBox->value();
    int speedSliderVal = m_speedSlider->value();
    m_speedSlider->setRange((int)(speedMin * 100), (int)(speedMax * 100));
    m_speedSlider->setTickInterval(m_powerMode ? 10000000 : 100);
    m_speedSpinBox->setRange(speedMin, speedMax);
    m_speedSlider->setValue(qBound((int)(speedMin * 100), speedSliderVal, (int)(speedMax * 100)));
    m_speedSpinBox->setValue(qBound(speedMin, speedVal, speedMax));

    int pitchVal = m_pitchSlider->value();
    m_pitchSlider->setRange(-pitchRange, pitchRange);
    m_pitchSlider->setTickInterval(m_powerMode ? 100000000 : 30);
    m_pitchSpinBox->setRange(-(double)pitchRange / 10.0, (double)pitchRange / 10.0);
    m_pitchSlider->setValue(qBound(-pitchRange, pitchVal, pitchRange));
    m_pitchSpinBox->setValue(qBound(-(double)pitchRange / 10.0, (double)pitchVal / 10.0, (double)pitchRange / 10.0));
}

QMap<double,int> EqualizerWidget::getBandGains() const {
    QMap<double,int> g;
    for (int i = 0; i < NUM_BANDS; ++i) {
        g[m_bands[i].freq] = m_bands[i].slider->value();
    }
    return g;
}
int EqualizerWidget::getPreampGain() const { return m_preampSlider->value(); }

// ---------------------------------------------------------------------------
//  Пользовательские пресеты эквалайзера (~/zmp_playlists/eq_presets/*.json)
// ---------------------------------------------------------------------------

QString EqualizerWidget::eqPresetsDir()
{
    const QString dir = QDir::homePath() + "/zmp_playlists/eq_presets";
    QDir().mkpath(dir);
    return dir;
}

QJsonObject EqualizerWidget::currentPresetJson(const QString &presetName) const
{
    QJsonObject root;
    root["name"] = presetName;
    QJsonArray bands;
    for (const Band &b : m_bands)
        bands.append(b.slider->value());
    root["bands"] = bands;
    root["preamp"] = m_preampSlider->value();
    root["speed"] = m_speedSpinBox->value();
    root["pitch"] = m_pitchSpinBox->value();
    root["echo"] = m_echoCheckBox->isChecked();
    return root;
}

bool EqualizerWidget::writePresetFile(const QString &path, const QString &presetName)
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::warning(this, ztr("Ошибка"), ztr("Не удалось сохранить пресет"));
        return false;
    }
    f.write(QJsonDocument(currentPresetJson(presetName)).toJson(QJsonDocument::Indented));
    f.close();
    return true;
}

void EqualizerWidget::scanUserPresets()
{
    // Убираем ранее добавленные пользовательские пресеты (после стандартных)
    static const int standardCount = 6;   // Custom, Default, Bass, Treble, Pop, Dance
    while (m_modeCombo->count() > standardCount)
        m_modeCombo->removeItem(m_modeCombo->count() - 1);

    QDir dir(eqPresetsDir());
    const QFileInfoList files =
        dir.entryInfoList({"*.json"}, QDir::Files, QDir::Name | QDir::IgnoreCase);
    for (const QFileInfo &fi : files) {
        QString title = fi.completeBaseName();
        QFile f(fi.absoluteFilePath());
        if (f.open(QIODevice::ReadOnly)) {
            const QString name = QJsonDocument::fromJson(f.readAll())
                                     .object().value("name").toString();
            f.close();
            if (!name.isEmpty()) title = name;   // имя пресета из json
        }
        m_modeCombo->addItem(title, fi.absoluteFilePath());
    }
}

void EqualizerWidget::applyPresetFile(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return;
    const QJsonObject root = QJsonDocument::fromJson(f.readAll()).object();
    f.close();

    m_isApplyingPreset = true;

    const QJsonArray bands = root.value("bands").toArray();
    for (int i = 0; i < NUM_BANDS && i < bands.size(); ++i)
        setBandValue(i, bands.at(i).toInt());
    setPreampValue(root.value("preamp").toInt(0));
    setSpeedValue(root.value("speed").toDouble(1.0));
    setPitchValue(root.value("pitch").toDouble(0.0));

    m_echoCheckBox->blockSignals(true);
    m_echoCheckBox->setChecked(root.value("echo").toBool(false));
    m_echoCheckBox->blockSignals(false);
    if (m_audioManager) m_audioManager->setEchoEnabled(root.value("echo").toBool(false));

    m_isApplyingPreset = false;
}

void EqualizerWidget::onSavePresetClicked()
{
    QDialog dlg(this);
    dlg.setWindowTitle(ztr("Сохранить пресет"));
    dlg.setModal(true);
    dlg.resize(420, 160);
    QVBoxLayout *l = new QVBoxLayout(&dlg);
    l->addWidget(new QLabel(ztr("Название пресета:")));
    QLineEdit *nameEdit = new QLineEdit;
    l->addWidget(nameEdit);
    l->addWidget(new QLabel(ztr("Имя файла:")));
    QLineEdit *fileEdit = new QLineEdit;
    l->addWidget(fileEdit);
    QHBoxLayout *btns = new QHBoxLayout;
    QPushButton *saveBtn = new QPushButton(ztr("Сохранить"));
    QPushButton *cancelBtn = new QPushButton(ztr("Отмена"));
    btns->addWidget(saveBtn);
    btns->addWidget(cancelBtn);
    l->addLayout(btns);
    connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);
    connect(saveBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
    // Автоматически подставляем имя файла из названия пресета
    connect(nameEdit, &QLineEdit::textChanged, fileEdit, [nameEdit, fileEdit]() {
        QString fn = nameEdit->text();
        fn.replace(' ', '_');
        fn.replace('/', '_');
        if (!fn.isEmpty() && !fn.endsWith(".json", Qt::CaseInsensitive))
            fn += ".json";
        fileEdit->setText(fn);
    });

    if (dlg.exec() != QDialog::Accepted)
        return;

    const QString presetName = nameEdit->text().trimmed();
    if (presetName.isEmpty()) {
        QMessageBox::warning(this, ztr("Ошибка"), ztr("Введите название пресета"));
        return;
    }
    QString fileName = fileEdit->text().trimmed();
    fileName.replace(' ', '_');
    fileName.replace('/', '_');
    if (fileName.isEmpty()) {
        QMessageBox::warning(this, ztr("Ошибка"), ztr("Введите имя файла"));
        return;
    }
    if (!fileName.endsWith(".json", Qt::CaseInsensitive))
        fileName += ".json";

    const QString path = eqPresetsDir() + "/" + fileName;
    if (!writePresetFile(path, presetName))
        return;

    // Пересканируем и сразу выбираем сохранённый пресет
    scanUserPresets();
    const int idx = m_modeCombo->findData(path);
    if (idx >= 0)
        m_modeCombo->setCurrentIndex(idx);
}

void EqualizerWidget::onSavePresetAsClicked()
{
    QString suggested = eqPresetsDir() + "/preset.json";
    const QString path = QFileDialog::getSaveFileName(
        this, ztr("Сохранить пресет"), suggested,
        ztr("JSON (*.json)"));
    if (path.isEmpty())
        return;

    QString fileName = QFileInfo(path).fileName();
    fileName.replace(' ', '_');
    QString fullPath = QFileInfo(path).absolutePath() + "/" + fileName;
    if (!fullPath.endsWith(".json", Qt::CaseInsensitive))
        fullPath += ".json";

    // Название пресета по умолчанию — имя файла без расширения
    const QString presetName = QFileInfo(fullPath).completeBaseName();
    if (!writePresetFile(fullPath, presetName))
        return;

    scanUserPresets();
    const int idx = m_modeCombo->findData(fullPath);
    if (idx >= 0)
        m_modeCombo->setCurrentIndex(idx);
}
