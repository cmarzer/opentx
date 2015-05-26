#ifndef TELEMETRY_H
#define TELEMETRY_H

#include "modeledit.h"
#include "eeprominterface.h"
#include <QGroupBox>
#include <QComboBox>
#include <QDoubleSpinBox>

class AutoComboBox;

namespace Ui {
  class TelemetryAnalog;
  class TelemetryCustomScreen;
  class TelemetrySensor;
  class Telemetry;
}

class TelemetryAnalog: public ModelPanel
{
    Q_OBJECT

    friend class TelemetryPanel;

  public:
    TelemetryAnalog(QWidget *parent, FrSkyChannelData & analog, ModelData & model, GeneralSettings & generalSettings, Firmware * firmware);
    virtual ~TelemetryAnalog();

  signals:
    void modified();

  private slots:
    void on_UnitCB_currentIndexChanged(int index);
    void on_RatioSB_editingFinished();
    void on_RatioSB_valueChanged();
    void on_CalibSB_editingFinished();
    void on_alarm1LevelCB_currentIndexChanged(int index);
    void on_alarm1GreaterCB_currentIndexChanged(int index);
    void on_alarm1ValueSB_editingFinished();
    void on_alarm2LevelCB_currentIndexChanged(int index);
    void on_alarm2GreaterCB_currentIndexChanged(int index);
    void on_alarm2ValueSB_editingFinished();

  private:
    Ui::TelemetryAnalog *ui;
    FrSkyChannelData & analog;
    bool lock;

    void update();
};

class TelemetryCustomScreen: public ModelPanel
{
    Q_OBJECT

  public:
    TelemetryCustomScreen(QWidget *parent, ModelData & model, FrSkyScreenData & screen, GeneralSettings & generalSettings, Firmware * firmware);
    ~TelemetryCustomScreen();
    void update();

  private slots:
    void on_screenType_currentIndexChanged(int index);
    void customFieldChanged(int index);
    void barSourceChanged(int index);
    void barMinChanged(double value);
    void barMaxChanged(double value);

  protected:
    void populateTelemetrySourceCB(QComboBox * b, RawSource & source, bool last=false);

  private:
    void updateBar(int line);
    Ui::TelemetryCustomScreen * ui;
    FrSkyScreenData & screen;
    QComboBox * fieldsCB[4][3];
    QComboBox * barsCB[4];
    QDoubleSpinBox * minSB[4];
    QDoubleSpinBox * maxSB[4];
};

class TelemetrySensorPanel: public ModelPanel
{
    Q_OBJECT

  public:
    TelemetrySensorPanel(QWidget *parent, SensorData & sensor, ModelData & model, GeneralSettings & generalSettings, Firmware * firmware);
    ~TelemetrySensorPanel();
    void update();

  signals:
    void nameModified();

  protected slots:
    void on_name_editingFinished();
    void on_type_currentIndexChanged(int index);
    void on_formula_currentIndexChanged(int index);
    void on_unit_currentIndexChanged(int index);
    void on_prec_valueChanged();

  protected:
    void updateSourcesComboBox(AutoComboBox * cb, bool negative);

  private:
    Ui::TelemetrySensor * ui;
    SensorData & sensor;
    bool lock;
};

class TelemetryPanel : public ModelPanel
{
    Q_OBJECT

  public:
    TelemetryPanel(QWidget *parent, ModelData & model, GeneralSettings & generalSettings, Firmware * firmware);
    virtual ~TelemetryPanel();
    virtual void update();

  private slots:
    void on_telemetryProtocol_currentIndexChanged(int index);
    void onAnalogModified();
    void on_frskyProtoCB_currentIndexChanged(int index);
    void on_bladesCount_editingFinished();
    void on_rssiAlarm1CB_currentIndexChanged(int index);
    void on_rssiAlarm2CB_currentIndexChanged(int index);
    void on_rssiAlarm1SB_editingFinished();
    void on_rssiAlarm2SB_editingFinished();
    void on_varioLimitMin_DSB_editingFinished();
    void on_varioLimitMax_DSB_editingFinished();
    void on_varioLimitCenterMin_DSB_editingFinished();
    void on_varioLimitCenterMax_DSB_editingFinished();
    void on_fasOffset_DSB_editingFinished();
    void on_mahCount_SB_editingFinished();
    void on_mahCount_ChkB_toggled(bool checked);

  private:
    Ui::Telemetry *ui;
    TelemetryAnalog * analogs[4];
    TelemetryCustomScreen * telemetryCustomScreens[4];
    TelemetrySensorPanel * sensorPanels[C9X_MAX_SENSORS];

    void setup();
    void telBarUpdate();
    void populateVoltsSource();
    void populateCurrentSource();
    void populateVarioSource();
};

#endif // TELEMETRY_H
