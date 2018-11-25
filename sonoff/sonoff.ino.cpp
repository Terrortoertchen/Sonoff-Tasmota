# 1 "c:\\users\\terrortoertchen\\appdata\\local\\temp\\tmp5bn35o"
#include <Arduino.h>
# 1 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/sonoff.ino"
# 29 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/sonoff.ino"
#include <core_version.h>
#include "sonoff_version.h"
#include "sonoff.h"
#include "my_user_config.h"
#ifdef USE_CONFIG_OVERRIDE
  #include "user_config_override.h"
#endif
#include "sonoff_post.h"
#include "i18n.h"
#include "sonoff_template.h"

#ifdef ARDUINO_ESP8266_RELEASE_2_4_0
#include "lwip/init.h"
#if LWIP_VERSION_MAJOR != 1
  #error Please use stable lwIP v1.4
#endif
#endif


#include <Ticker.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <StreamString.h>
#include <ArduinoJson.h>
#ifdef USE_WEBSERVER
  #include <ESP8266WebServer.h>
  #include <DNSServer.h>
#endif
#ifdef USE_ARDUINO_OTA
  #include <ArduinoOTA.h>
  #ifndef USE_DISCOVERY
  #define USE_DISCOVERY
  #endif
#endif
#ifdef USE_DISCOVERY
  #include <ESP8266mDNS.h>
#endif
#ifdef USE_I2C
  #include <Wire.h>
#endif
#ifdef USE_SPI
  #include <SPI.h>
#endif


#include "settings.h"

enum TasmotaCommands {
  CMND_BACKLOG, CMND_DELAY, CMND_POWER, CMND_FANSPEED, CMND_STATUS, CMND_STATE, CMND_POWERONSTATE, CMND_PULSETIME,
  CMND_BLINKTIME, CMND_BLINKCOUNT, CMND_SENSOR, CMND_SAVEDATA, CMND_SETOPTION, CMND_TEMPERATURE_RESOLUTION, CMND_HUMIDITY_RESOLUTION,
  CMND_PRESSURE_RESOLUTION, CMND_POWER_RESOLUTION, CMND_VOLTAGE_RESOLUTION, CMND_FREQUENCY_RESOLUTION, CMND_CURRENT_RESOLUTION, CMND_ENERGY_RESOLUTION, CMND_WEIGHT_RESOLUTION,
  CMND_MODULE, CMND_MODULES, CMND_GPIO, CMND_GPIOS, CMND_PWM, CMND_PWMFREQUENCY, CMND_PWMRANGE, CMND_COUNTER, CMND_COUNTERTYPE,
  CMND_COUNTERDEBOUNCE, CMND_BUTTONDEBOUNCE, CMND_SWITCHDEBOUNCE, CMND_SLEEP, CMND_UPGRADE, CMND_UPLOAD, CMND_OTAURL, CMND_SERIALLOG, CMND_SYSLOG,
  CMND_LOGHOST, CMND_LOGPORT, CMND_IPADDRESS, CMND_NTPSERVER, CMND_AP, CMND_SSID, CMND_PASSWORD, CMND_HOSTNAME,
  CMND_WIFICONFIG, CMND_FRIENDLYNAME, CMND_SWITCHMODE,
  CMND_TELEPERIOD, CMND_RESTART, CMND_RESET, CMND_TIMEZONE, CMND_TIMESTD, CMND_TIMEDST, CMND_ALTITUDE, CMND_LEDPOWER, CMND_LEDSTATE,
  CMND_I2CSCAN, CMND_SERIALSEND, CMND_BAUDRATE, CMND_SERIALDELIMITER, CMND_DRIVER };
const char kTasmotaCommands[] PROGMEM =
  D_CMND_BACKLOG "|" D_CMND_DELAY "|" D_CMND_POWER "|" D_CMND_FANSPEED "|" D_CMND_STATUS "|" D_CMND_STATE "|" D_CMND_POWERONSTATE "|" D_CMND_PULSETIME "|"
  D_CMND_BLINKTIME "|" D_CMND_BLINKCOUNT "|" D_CMND_SENSOR "|" D_CMND_SAVEDATA "|" D_CMND_SETOPTION "|" D_CMND_TEMPERATURE_RESOLUTION "|" D_CMND_HUMIDITY_RESOLUTION "|"
  D_CMND_PRESSURE_RESOLUTION "|" D_CMND_POWER_RESOLUTION "|" D_CMND_VOLTAGE_RESOLUTION "|" D_CMND_FREQUENCY_RESOLUTION "|" D_CMND_CURRENT_RESOLUTION "|" D_CMND_ENERGY_RESOLUTION "|" D_CMND_WEIGHT_RESOLUTION "|"
  D_CMND_MODULE "|" D_CMND_MODULES "|" D_CMND_GPIO "|" D_CMND_GPIOS "|" D_CMND_PWM "|" D_CMND_PWMFREQUENCY "|" D_CMND_PWMRANGE "|" D_CMND_COUNTER "|" D_CMND_COUNTERTYPE "|"
  D_CMND_COUNTERDEBOUNCE "|" D_CMND_BUTTONDEBOUNCE "|" D_CMND_SWITCHDEBOUNCE "|" D_CMND_SLEEP "|" D_CMND_UPGRADE "|" D_CMND_UPLOAD "|" D_CMND_OTAURL "|" D_CMND_SERIALLOG "|" D_CMND_SYSLOG "|"
  D_CMND_LOGHOST "|" D_CMND_LOGPORT "|" D_CMND_IPADDRESS "|" D_CMND_NTPSERVER "|" D_CMND_AP "|" D_CMND_SSID "|" D_CMND_PASSWORD "|" D_CMND_HOSTNAME "|"
  D_CMND_WIFICONFIG "|" D_CMND_FRIENDLYNAME "|" D_CMND_SWITCHMODE "|"
  D_CMND_TELEPERIOD "|" D_CMND_RESTART "|" D_CMND_RESET "|" D_CMND_TIMEZONE "|" D_CMND_TIMESTD "|" D_CMND_TIMEDST "|" D_CMND_ALTITUDE "|" D_CMND_LEDPOWER "|" D_CMND_LEDSTATE "|"
  D_CMND_I2CSCAN "|" D_CMND_SERIALSEND "|" D_CMND_BAUDRATE "|" D_CMND_SERIALDELIMITER "|" D_CMND_DRIVER;

const uint8_t kIFan02Speed[4][3] = {{6,6,6}, {7,6,6}, {7,7,6}, {7,6,7}};


SerialConfig serial_config = SERIAL_8N1;

#ifdef USE_MQTT_TLS
  WiFiClientSecure EspClient;
#else
  WiFiClient EspClient;
#endif

WiFiUDP PortUdp;

unsigned long feature_drv1;
unsigned long feature_drv2;
unsigned long feature_sns1;
unsigned long feature_sns2;
unsigned long serial_polling_window = 0;
unsigned long state_second = 0;
unsigned long state_50msecond = 0;
unsigned long state_100msecond = 0;
unsigned long state_250msecond = 0;
unsigned long pulse_timer[MAX_PULSETIMERS] = { 0 };
unsigned long blink_timer = 0;
unsigned long backlog_delay = 0;
unsigned long button_debounce = 0;
unsigned long switch_debounce = 0;
power_t power = 0;
power_t blink_power;
power_t blink_mask = 0;
power_t blink_powersave;
power_t latching_power = 0;
power_t rel_inverted = 0;
int baudrate = APP_BAUDRATE;
int serial_in_byte_counter = 0;
int ota_state_flag = 0;
int ota_result = 0;
int restart_flag = 0;
int wifi_state_flag = WIFI_RESTART;
int tele_period = 1;
int blinks = 201;
uint32_t uptime = 0;
uint32_t global_update = 0;
float global_temperature = 0;
float global_humidity = 0;
char *ota_url;
uint16_t dual_button_code = 0;
uint16_t mqtt_cmnd_publish = 0;
uint16_t blink_counter = 0;
uint16_t seriallog_timer = 0;
uint16_t syslog_timer = 0;
uint16_t holdbutton[MAX_KEYS] = { 0 };
uint16_t switch_no_pullup = 0;
int16_t save_data_counter;
RulesBitfield rules_flag;
uint8_t serial_local = 0;
uint8_t fallback_topic_flag = 0;
uint8_t state_250mS = 0;
uint8_t latching_relay_pulse = 0;
uint8_t backlog_index = 0;
uint8_t backlog_pointer = 0;
uint8_t backlog_mutex = 0;
uint8_t interlock_mutex = 0;
uint8_t sleep;
uint8_t stop_flash_rotate = 0;
uint8_t blinkstate = 0;
uint8_t blinkspeed = 1;
uint8_t lastbutton[MAX_KEYS] = { NOT_PRESSED, NOT_PRESSED, NOT_PRESSED, NOT_PRESSED };
uint8_t multiwindow[MAX_KEYS] = { 0 };
uint8_t multipress[MAX_KEYS] = { 0 };
uint8_t lastwallswitch[MAX_SWITCHES];
uint8_t holdwallswitch[MAX_SWITCHES] = { 0 };
uint8_t virtualswitch[MAX_SWITCHES];
uint8_t pin[GPIO_MAX];
uint8_t led_inverted = 0;
uint8_t pwm_inverted = 0;
uint8_t counter_no_pullup = 0;
uint8_t dht_flg = 0;
uint8_t energy_flg = 0;
uint8_t i2c_flg = 0;
uint8_t spi_flg = 0;
uint8_t light_type = 0;
uint8_t ntp_force_sync = 0;
byte serial_in_byte;
byte dual_hex_code = 0;
byte ota_retry_counter = OTA_ATTEMPTS;
byte web_log_index = 1;
byte reset_web_log_flag = 0;
byte devices_present = 0;
byte seriallog_level;
byte syslog_level;
byte mdns_delayed_start = 0;
boolean latest_uptime_flag = true;
boolean pwm_present = false;
boolean mdns_begun = false;
mytmplt my_module;
StateBitfield global_state;
char my_version[33];
char my_hostname[33];
char mqtt_client[33];
char mqtt_topic[33];
char serial_in_buffer[INPUT_BUFFER_SIZE];
char mqtt_data[MESSZ];
char log_data[LOGSZ];
char web_log[WEB_LOG_SIZE] = {'\0'};
String backlog[MAX_BACKLOG];
char* Format(char* output, const char* input, int size);
char* GetOtaUrl(char *otaurl, size_t otaurl_size);
void GetTopic_P(char *stopic, byte prefix, char *topic, const char* subtopic);
char* GetStateText(byte state);
void SetLatchingRelay(power_t lpower, uint8_t state);
void SetDevicePower(power_t rpower, int source);
void SetLedPower(uint8_t state);
uint8_t GetFanspeed();
void SetFanspeed(uint8_t fanspeed);
void SetPulseTimer(uint8_t index, uint16_t time);
uint16_t GetPulseTimer(uint8_t index);
void MqttDataHandler(char* topic, byte* data, unsigned int data_len);
boolean SendKey(byte key, byte device, byte state);
void ExecuteCommandPower(byte device, byte state, int source);
void StopAllPowerBlink();
void ExecuteCommand(char *cmnd, int source);
void PublishStatus(uint8_t payload);
void MqttShowPWMState();
void MqttShowState();
boolean MqttShowSensor();
void PerformEverySecond();
void ButtonHandler();
void SwitchHandler(byte mode);
void Every100mSeconds();
void Every250mSeconds();
void ArduinoOTAInit();
void SerialInput();
void GpioSwitchPinMode(uint8_t index);
void GpioInit();
void setup();
void loop();
uint32_t GetRtcSettingsCrc();
void RtcSettingsSave();
void RtcSettingsLoad();
boolean RtcSettingsValid();
uint32_t GetRtcRebootCrc();
void RtcRebootSave();
void RtcRebootLoad();
boolean RtcRebootValid();
void SetFlashModeDout();
void SettingsBufferFree();
bool SettingsBufferAlloc();
uint16_t GetSettingsCrc();
void SettingsSaveAll();
uint32_t GetSettingsAddress();
void SettingsSave(byte rotate);
void SettingsLoad();
void SettingsErase(uint8_t type);
bool SettingsEraseConfig(void);
void SettingsSdkErase();
void SettingsDefault();
void SettingsDefaultSet1();
void SettingsDefaultSet2();
void SettingsDefaultSet_5_8_1();
void SettingsDefaultSet_5_10_1();
void SettingsResetStd();
void SettingsResetDst();
void SettingsDefaultSet_5_13_1c();
void SettingsDelta();
void OsWatchTicker();
void OsWatchInit();
void OsWatchLoop();
String GetResetReason();
boolean OsWatchBlockedLoop();
void* memchr(const void* ptr, int value, size_t num);
size_t strcspn(const char *str1, const char *str2);
size_t strchrspn(const char *str1, int character);
char* subStr(char* dest, char* str, const char *delim, int index);
double CharToDouble(char *str);
int TextToInt(char *str);
char* dtostrfd(double number, unsigned char prec, char *s);
char* Unescape(char* buffer, uint16_t* size);
char* RemoveSpace(char* p);
char* UpperCase(char* dest, const char* source);
char* UpperCase_P(char* dest, const char* source);
char* LTrim(char* p);
char* RTrim(char* p);
char* Trim(char* p);
char* NoAlNumToUnderscore(char* dest, const char* source);
void SetShortcut(char* str, uint8_t action);
uint8_t Shortcut(const char* str);
boolean ParseIp(uint32_t* addr, const char* str);
void MakeValidMqtt(byte option, char* str);
bool NewerVersion(char* version_str);
char* GetPowerDevice(char* dest, uint8_t idx, size_t size, uint8_t option);
char* GetPowerDevice(char* dest, uint8_t idx, size_t size);
float ConvertTemp(float c);
char TempUnit();
void SetGlobalValues(float temperature, float humidity);
void ResetGlobalValues();
double FastPrecisePow(double a, double b);
uint32_t SqrtInt(uint32_t num);
uint32_t RoundSqrtInt(uint32_t num);
char* GetTextIndexed(char* destination, size_t destination_size, uint16_t index, const char* haystack);
int GetCommandCode(char* destination, size_t destination_size, const char* needle, const char* haystack);
int GetStateNumber(char *state_text);
boolean GetUsedInModule(byte val, uint8_t *arr);
void SetSerialBaudrate(int baudrate);
void ClaimSerial();
void SerialSendRaw(char *codes);
uint32_t GetHash(const char *buffer, size_t size);
void ShowSource(int source);
uint8_t ValidGPIO(uint8_t pin, uint8_t gpio);
long TimeDifference(unsigned long prev, unsigned long next);
long TimePassedSince(unsigned long timestamp);
bool TimeReached(unsigned long timer);
void SetNextTimeInterval(unsigned long& timer, const unsigned long step);
void GetFeatures();
int WifiGetRssiAsQuality(int rssi);
boolean WifiConfigCounter();
void WifiWpsStatusCallback(wps_cb_status status);
boolean WifiWpsConfigDone(void);
boolean WifiWpsConfigBegin(void);
void WifiConfig(uint8_t type);
void WiFiSetSleepMode();
void WifiBegin(uint8_t flag);
void WifiState(uint8_t state);
void WifiCheckIp();
void WifiCheck(uint8_t param);
int WifiState();
void WifiConnect();
void EspRestart();
bool I2cValidRead(uint8_t addr, uint8_t reg, uint8_t size);
bool I2cValidRead8(uint8_t *data, uint8_t addr, uint8_t reg);
bool I2cValidRead16(uint16_t *data, uint8_t addr, uint8_t reg);
bool I2cValidReadS16(int16_t *data, uint8_t addr, uint8_t reg);
bool I2cValidRead16LE(uint16_t *data, uint8_t addr, uint8_t reg);
bool I2cValidReadS16_LE(int16_t *data, uint8_t addr, uint8_t reg);
bool I2cValidRead24(int32_t *data, uint8_t addr, uint8_t reg);
uint8_t I2cRead8(uint8_t addr, uint8_t reg);
uint16_t I2cRead16(uint8_t addr, uint8_t reg);
int16_t I2cReadS16(uint8_t addr, uint8_t reg);
uint16_t I2cRead16LE(uint8_t addr, uint8_t reg);
int16_t I2cReadS16_LE(uint8_t addr, uint8_t reg);
int32_t I2cRead24(uint8_t addr, uint8_t reg);
bool I2cWrite(uint8_t addr, uint8_t reg, uint32_t val, uint8_t size);
bool I2cWrite8(uint8_t addr, uint8_t reg, uint16_t val);
bool I2cWrite16(uint8_t addr, uint8_t reg, uint16_t val);
int8_t I2cReadBuffer(uint8_t addr, uint8_t reg, uint8_t *reg_data, uint16_t len);
int8_t I2cWriteBuffer(uint8_t addr, uint8_t reg, uint8_t *reg_data, uint16_t len);
void I2cScan(char *devs, unsigned int devs_len);
boolean I2cDevice(byte addr);
String GetBuildDateAndTime();
String GetDateAndTime(byte time_type);
String GetUptime();
uint32_t GetMinutesUptime();
uint32_t GetMinutesPastMidnight();
void BreakTime(uint32_t time_input, TIME_T &tm);
uint32_t MakeTime(TIME_T &tm);
uint32_t RuleToTime(TimeRule r, int yr);
String GetTime(int type);
uint32_t LocalTime();
uint32_t Midnight();
boolean MidnightNow();
void RtcSecond();
void RtcInit();
uint16_t AdcRead();
void AdcEvery250ms();
void AdcShow(boolean json);
boolean Xsns02(byte function);
void SetSeriallog(byte loglevel);
void GetLog(byte idx, char** entry_pp, size_t* len_p);
void Syslog();
void AddLog(byte loglevel);
void AddLog_P(byte loglevel, const char *formatP);
void AddLog_P(byte loglevel, const char *formatP, const char *formatP2);
void AddLogSerial(byte loglevel, uint8_t *buffer, int count);
void AddLogSerial(byte loglevel);
void AddLogMissed(char *sensor, uint8_t misses);
static void WebGetArg(const char* arg, char* out, size_t max);
void ShowWebSource(int source);
void ExecuteWebCommand(char* svalue, int source);
void StartWebserver(int type, IPAddress ipweb);
void StopWebserver();
void WifiManagerBegin();
void PollDnsWebserver();
void SetHeader();
bool WebAuthenticate(void);
void ShowPage(String &page, bool auth);
void ShowPage(String &page);
void WebRestart(uint8_t type);
void HandleWifiLogin();
void HandleRoot();
void HandleAjaxStatusRefresh();
boolean HttpUser();
void HandleConfiguration();
void HandleModuleConfiguration();
void ModuleSaveSettings();
String htmlEscape(String s);
void HandleWifiConfiguration();
void WifiSaveSettings();
void HandleLoggingConfiguration();
void LoggingSaveSettings();
void HandleOtherConfiguration();
void OtherSaveSettings();
void HandleBackupConfiguration();
void HandleResetConfiguration();
void HandleRestoreConfiguration();
void HandleInformation();
void HandleUpgradeFirmware();
void HandleUpgradeFirmwareStart();
void HandleUploadDone();
void HandleUploadLoop();
void HandlePreflightRequest();
void HandleHttpCommand();
void HandleConsole();
void HandleAjaxConsoleRefresh();
void HandleNotFound();
boolean CaptivePortal();
boolean ValidIpAddress(String str);
String UrlEncode(const String& text);
int WebSend(char *buffer);
bool WebCommand();
boolean Xdrv01(byte function);
bool MqttIsConnected();
void MqttDisconnect();
void MqttSubscribeLib(char *topic);
bool MqttPublishLib(const char* topic, boolean retained);
void MqttLoop();
bool MqttIsConnected();
void MqttDisconnect();
void MqttDisconnectedCb();
void MqttSubscribeLib(char *topic);
bool MqttPublishLib(const char* topic, boolean retained);
void MqttLoop();
bool MqttIsConnected();
void MqttDisconnect();
void MqttMyDataCb(String &topic, String &data);
void MqttSubscribeLib(char *topic);
bool MqttPublishLib(const char* topic, boolean retained);
void MqttLoop();
boolean MqttDiscoverServer();
int MqttLibraryType();
void MqttRetryCounter(uint8_t value);
void MqttSubscribe(char *topic);
void MqttPublishDirect(const char* topic, boolean retained);
void MqttPublish(const char* topic, boolean retained);
void MqttPublish(const char* topic);
void MqttPublishPrefixTopic_P(uint8_t prefix, const char* subtopic, boolean retained);
void MqttPublishPrefixTopic_P(uint8_t prefix, const char* subtopic);
void MqttPublishPowerState(byte device);
void MqttPublishPowerBlinkState(byte device);
void MqttDisconnected(int state);
void MqttConnected();
boolean MqttCheckTls();
void MqttReconnect();
void MqttCheck();
bool MqttCommand();
void HandleMqttConfiguration();
void MqttSaveSettings();
boolean Xdrv02(byte function);
void EnergyUpdateToday();
void Energy200ms();
void EnergySaveState();
boolean EnergyMargin(byte type, uint16_t margin, uint16_t value, byte &flag, byte &save_flag);
void EnergySetPowerSteadyCounter();
void EnergyMarginCheck();
void EnergyMqttShow();
boolean EnergyCommand();
void EnergyDrvInit();
void EnergySnsInit();
void EnergyShow(boolean json);
boolean Xdrv03(byte function);
boolean Xsns03(byte function);
void AriluxRfInterrupt();
void AriluxRfHandler();
void AriluxRfInit();
void AriluxRfDisable();
void LightDiPulse(uint8_t times);
void LightDckiPulse(uint8_t times);
void LightMy92x1Write(uint8_t data);
void LightMy92x1Init();
void LightMy92x1Duty(uint8_t duty_r, uint8_t duty_g, uint8_t duty_b, uint8_t duty_w, uint8_t duty_c);
void LightInit();
void LightSetColorTemp(uint16_t ct);
uint16_t LightGetColorTemp();
void LightSetDimmer(uint8_t myDimmer);
void LightSetColor();
void LightSetSignal(uint16_t lo, uint16_t hi, uint16_t value);
char* LightGetColor(uint8_t type, char* scolor);
void LightPowerOn();
void LightState(uint8_t append);
void LightPreparePower();
void LightFade();
void LightWheel(uint8_t wheel_pos);
void LightCycleColor(int8_t direction);
void LightRandomColor();
void LightSetPower();
void LightAnimate();
void LightRgbToHsb();
void LightHsbToRgb();
void LightGetHsb(float *hue, float *sat, float *bri, bool gotct);
void LightSetHsb(float hue, float sat, float bri, uint16_t ct, bool gotct);
boolean LightColorEntry(char *buffer, uint8_t buffer_length);
boolean LightCommand();
boolean Xdrv04(byte function);
void IrSendInit(void);
void IrReceiveInit(void);
void IrReceiveCheck();
boolean IrHvacToshiba(const char *HVAC_Mode, const char *HVAC_FanMode, boolean HVAC_Power, int HVAC_Temp);
boolean IrHvacMitsubishi(const char *HVAC_Mode, const char *HVAC_FanMode, boolean HVAC_Power, int HVAC_Temp);
boolean IrSendCommand();
boolean Xdrv05(byte function);
ssize_t rf_find_hex_record_start(uint8_t *buf, size_t size);
ssize_t rf_find_hex_record_end(uint8_t *buf, size_t size);
ssize_t rf_glue_remnant_with_new_data_and_write(const uint8_t *remnant_data, uint8_t *new_data, size_t new_data_len);
ssize_t rf_decode_and_write(uint8_t *record, size_t size);
ssize_t rf_search_and_write(uint8_t *buf, size_t size);
uint8_t rf_erase_flash();
uint8_t SnfBrUpdateInit();
void SonoffBridgeReceivedRaw();
void SonoffBridgeLearnFailed();
void SonoffBridgeReceived();
boolean SonoffBridgeSerialInput();
void SonoffBridgeSendCommand(byte code);
void SonoffBridgeSendAck();
void SonoffBridgeSendCode(uint32_t code);
void SonoffBridgeSend(uint8_t idx, uint8_t key);
void SonoffBridgeLearn(uint8_t key);
boolean SonoffBridgeCommand();
void SonoffBridgeInit();
boolean Xdrv06(byte function);
int DomoticzBatteryQuality();
int DomoticzRssiQuality();
void MqttPublishDomoticzPowerState(byte device);
void DomoticzUpdatePowerState(byte device);
void DomoticzMqttUpdate();
void DomoticzMqttSubscribe();
boolean DomoticzMqttData();
boolean DomoticzCommand();
boolean DomoticzSendKey(byte key, byte device, byte state, byte svalflg);
uint8_t DomoticzHumidityState(char *hum);
void DomoticzSensor(byte idx, char *data);
void DomoticzSensor(byte idx, uint32_t value);
void DomoticzTempHumSensor(char *temp, char *hum);
void DomoticzTempHumPressureSensor(char *temp, char *hum, char *baro);
void DomoticzSensorPowerEnergy(int power, char *energy);
void HandleDomoticzConfiguration();
void DomoticzSaveSettings();
boolean Xdrv07(byte function);
void SerialBridgeInput();
void SerialBridgeInit(void);
boolean SerialBridgeCommand();
boolean Xdrv08(byte function);
double JulianischesDatum();
double InPi(double x);
double eps(double T);
double BerechneZeitgleichung(double *DK,double T);
void DuskTillDawn(uint8_t *hour_up,uint8_t *minute_up, uint8_t *hour_down, uint8_t *minute_down);
void ApplyTimerOffsets(Timer *duskdawn);
String GetSun(byte dawn);
uint16_t GetSunMinutes(byte dawn);
void TimerSetRandomWindow(byte index);
void TimerSetRandomWindows();
void TimerEverySecond();
void PrepShowTimer(uint8_t index);
boolean TimerCommand();
void HandleTimerConfiguration();
void TimerSaveSettings();
boolean Xdrv09(byte function);
bool RulesRuleMatch(byte rule_set, String &event, String &rule);
bool RuleSetProcess(byte rule_set, String &event_saved);
bool RulesProcessEvent(char *json_event);
bool RulesProcess();
void RulesInit();
void RulesEvery50ms();
void RulesEvery100ms();
void RulesEverySecond();
void RulesSetPower();
void RulesTeleperiod();
boolean RulesCommand();
double map_double(double x, double in_min, double in_max, double out_min, double out_max);
boolean Xdrv10(byte function);
void KNX_ADD_GA( byte GAop, byte GA_FNUM, byte GA_AREA, byte GA_FDEF );
void KNX_DEL_GA( byte GAnum );
void KNX_ADD_CB( byte CBop, byte CB_FNUM, byte CB_AREA, byte CB_FDEF );
void KNX_DEL_CB( byte CBnum );
bool KNX_CONFIG_NOT_MATCH();
void KNXStart();
void KNX_INIT();
void KNX_CB_Action(message_t const &msg, void *arg);
void KnxUpdatePowerState(byte device, power_t state);
void KnxSendButtonPower(byte key, byte device, byte state);
void KnxSensor(byte sensor_type, float value);
void HandleKNXConfiguration();
void KNX_Save_Settings();
boolean KnxCommand();
boolean Xdrv11(byte function);
void HAssDiscoverRelay();
void HAssDiscoverButton();
void HAssDiscovery(uint8_t mode);
boolean Xdrv12(byte function);
void DisplayInit(uint8_t mode);
void DisplayClear();
void DisplayDrawHLine(uint16_t x, uint16_t y, int16_t len, uint16_t color);
void DisplayDrawVLine(uint16_t x, uint16_t y, int16_t len, uint16_t color);
void DisplayDrawLine(uint16_t x, uint16_t y, uint16_t x2, uint16_t y2, uint16_t color);
void DisplayDrawCircle(uint16_t x, uint16_t y, uint16_t rad, uint16_t color);
void DisplayDrawFilledCircle(uint16_t x, uint16_t y, uint16_t rad, uint16_t color);
void DisplayDrawRectangle(uint16_t x, uint16_t y, uint16_t x2, uint16_t y2, uint16_t color);
void DisplayDrawFilledRectangle(uint16_t x, uint16_t y, uint16_t x2, uint16_t y2, uint16_t color);
void DisplayDrawFrame();
void DisplaySetSize(uint8_t size);
void DisplaySetFont(uint8_t font);
void DisplaySetRotation(uint8_t rotation);
void DisplayDrawStringAt(uint16_t x, uint16_t y, char *str, uint16_t color, uint8_t flag);
void DisplayOnOff(uint8_t on);
uint8_t atoiv(char *cp, int16_t *res);
uint8_t atoiV(char *cp, uint16_t *res);
void DisplayText();
void DisplayClearScreenBuffer();
void DisplayFreeScreenBuffer();
void DisplayAllocScreenBuffer();
void DisplayReAllocScreenBuffer();
void DisplayFillScreen(uint8_t line);
void DisplayClearLogBuffer();
void DisplayFreeLogBuffer();
void DisplayAllocLogBuffer();
void DisplayReAllocLogBuffer();
void DisplayLogBufferAdd(char* txt);
char* DisplayLogBuffer(char temp_code);
void DisplayLogBufferInit();
void DisplayJsonValue(const char *topic, const char* device, const char* mkey, const char* value);
void DisplayAnalyzeJson(char *topic, char *json);
void DisplayMqttSubscribe();
boolean DisplayMqttData();
void DisplayLocalSensor();
void DisplayInitDriver();
void DisplaySetPower();
boolean DisplayCommand();
boolean Xdrv13(byte function);
uint16_t MP3_Checksum(uint8_t *array);
void MP3PlayerInit(void);
void MP3_CMD(uint8_t mp3cmd,uint16_t val);
boolean MP3PlayerCmd(void);
boolean Xdrv14(byte function);
void PCA9685_Detect(void);
void PCA9685_Reset(void);
void PCA9685_SetPWMfreq(double freq);
void PCA9685_SetPWM_Reg(uint8_t pin, uint16_t on, uint16_t off);
void PCA9685_SetPWM(uint8_t pin, uint16_t pwm, bool inverted);
bool PCA9685_Command(void);
void PCA9685_OutputTelemetry(bool telemetry);
boolean Xdrv15(byte function);
boolean TuyaSetPower();
void LightSerialDuty(uint8_t duty);
void TuyaPacketProcess();
void TuyaSerialInput();
boolean TuyaModuleSelected();
void TuyaResetWifiLed();
void TuyaInit();
void TuyaResetWifi();
boolean TuyaButtonPressed();
boolean Xdrv16(byte function);
void RfReceiveCheck();
void RfInit();
boolean RfSendCommand();
boolean Xdrv17(byte function);
boolean XdrvCommand(uint8_t grpflg, char *type, uint16_t index, char *dataBuf, uint16_t data_len, int16_t payload, uint16_t payload16);
boolean XdrvMqttData(char *topicBuf, uint16_t stopicBuf, char *dataBuf, uint16_t sdataBuf);
boolean XdrvRulesProcess();
void ShowFreeMem(const char *where);
boolean XdrvCall(byte Function);
void LcdInitMode();
void LcdInit(uint8_t mode);
void LcdInitDriver();
void LcdDrawStringAt();
void LcdDisplayOnOff(uint8_t on);
void LcdCenter(byte row, char* txt);
boolean LcdPrintLog();
void LcdTime();
void LcdRefresh();
boolean Xdsp01(byte function);
void Ssd1306InitMode();
void Ssd1306Init(uint8_t mode);
void Ssd1306InitDriver();
void Ssd1306Clear();
void Ssd1306DrawStringAt(uint16_t x, uint16_t y, char *str, uint16_t color, uint8_t flag);
void Ssd1306DisplayOnOff(uint8_t on);
void Ssd1306OnOff();
void Ssd1306PrintLog();
void Ssd1306Time();
void Ssd1306Refresh();
boolean Xdsp02(byte function);
void MatrixWrite();
void MatrixClear();
void MatrixFixed(char* txt);
void MatrixCenter(char* txt);
void MatrixScrollLeft(char* txt, int loop);
void MatrixScrollUp(char* txt, int loop);
void MatrixInitMode();
void MatrixInit(uint8_t mode);
void MatrixInitDriver();
void MatrixOnOff();
void MatrixDrawStringAt(uint16_t x, uint16_t y, char *str, uint16_t color, uint8_t flag);
void MatrixPrintLog(uint8_t direction);
void MatrixRefresh();
boolean Xdsp03(byte function);
void Ili9341InitMode();
void Ili9341Init(uint8_t mode);
void Ili9341InitDriver();
void Ili9341Clear();
void Ili9341DrawStringAt(uint16_t x, uint16_t y, char *str, uint16_t color, uint8_t flag);
void Ili9341DisplayOnOff(uint8_t on);
void Ili9341OnOff();
void Ili9341PrintLog();
void Ili9341Refresh();
boolean Xdsp04(byte function);
uint8_t XdspPresent();
boolean XdspCall(byte Function);
void HlwCfInterrupt();
void HlwCf1Interrupt();
void HlwEvery200ms();
void HlwEverySecond();
void HlwSnsInit();
void HlwDrvInit();
boolean HlwCommand();
int Xnrg01(byte function);
void CseReceived();
bool CseSerialInput();
void CseEverySecond();
void CseDrvInit();
boolean CseCommand();
int Xnrg02(byte function);
uint8_t PzemCrc(uint8_t *data);
void PzemSend(uint8_t cmd);
bool PzemReceiveReady();
bool PzemRecieve(uint8_t resp, float *data);
void PzemEvery200ms();
void PzemSnsInit();
void PzemDrvInit();
int Xnrg03(byte function);
uint8_t McpChecksum(uint8_t *data);
unsigned long McpExtractInt(char *data, uint8_t offset, uint8_t size);
void McpSetInt(unsigned long value, uint8_t *data, uint8_t offset, size_t size);
void McpSend(uint8_t *data);
void McpGetAddress(void);
void McpAddressReceive(void);
void McpGetCalibration(void);
void McpParseCalibration(void);
bool McpCalibrationCalc(struct mcp_cal_registers_type *cal_registers, uint8_t range_shift);
void McpSetCalibration(struct mcp_cal_registers_type *cal_registers);
void McpSetSystemConfiguration(uint16 interval);
void McpGetFrequency(void);
void McpParseFrequency(void);
void McpSetFrequency(uint16_t line_frequency_ref, uint16_t gain_line_frequency);
void McpGetData(void);
void McpParseData(void);
bool McpSerialInput(void);
void McpEverySecond(void);
void McpSnsInit(void);
void McpDrvInit(void);
boolean McpCommand(void);
int Xnrg04(byte function);
void PzemAcEverySecond();
void PzemAcSnsInit();
void PzemAcDrvInit();
int Xnrg05(byte function);
void PzemDcEverySecond();
void PzemDcSnsInit();
void PzemDcDrvInit();
int Xnrg06(byte function);
int XnrgCall(byte Function);
String WemoSerialnumber();
String WemoUuid();
void WemoRespondToMSearch(int echo_type);
String HueBridgeId();
String HueSerialnumber();
String HueUuid();
void HueRespondToMSearch();
boolean UdpDisconnect();
boolean UdpConnect();
void PollUdp();
void HandleUpnpEvent();
void HandleUpnpService();
void HandleUpnpMetaService();
void HandleUpnpSetupWemo();
String GetHueDeviceId(uint8_t id);
String GetHueUserId();
void HandleUpnpSetupHue();
void HueNotImplemented(String *path);
void HueConfigResponse(String *response);
void HueConfig(String *path);
void HueLightStatus1(byte device, String *response);
void HueLightStatus2(byte device, String *response);
void HueGlobalConfig(String *path);
void HueAuthentication(String *path);
void HueLights(String *path);
void HueGroups(String *path);
void HandleHueApi(String *path);
void HueWemoAddHandlers();
void Ws2812StripShow();
int mod(int a, int b);
void Ws2812UpdatePixelColor(int position, struct WsColor hand_color, float offset);
void Ws2812UpdateHand(int position, uint8_t index);
void Ws2812Clock();
void Ws2812GradientColor(uint8_t schemenr, struct WsColor* mColor, uint16_t range, uint16_t gradRange, uint16_t i);
void Ws2812Gradient(uint8_t schemenr);
void Ws2812Bars(uint8_t schemenr);
void Ws2812Init();
void Ws2812Clear();
void Ws2812SetColor(uint16_t led, uint8_t red, uint8_t green, uint8_t blue, uint8_t white);
void Ws2812ForceSuspend ();
void Ws2812ForceUpdate ();
char* Ws2812GetColor(uint16_t led, char* scolor);
void Ws2812ShowScheme(uint8_t scheme);
void CounterUpdate(byte index);
void CounterUpdate1();
void CounterUpdate2();
void CounterUpdate3();
void CounterUpdate4();
void CounterSaveState();
void CounterInit();
void CounterShow(boolean json);
boolean Xsns01(byte function);
void SonoffScSend(const char *data);
void SonoffScInit();
void SonoffScSerialInput(char *rcvstat);
void SonoffScShow(boolean json);
boolean Xsns04(byte function);
uint8_t OneWireReset();
void OneWireWriteBit(uint8_t v);
uint8_t OneWireReadBit();
void OneWireWrite(uint8_t v);
uint8_t OneWireRead();
boolean OneWireCrc8(uint8_t *addr);
void Ds18b20Convert();
boolean Ds18b20Read();
void Ds18b20EverySecond();
void Ds18b20Show(boolean json);
boolean Xsns05(byte function);
uint8_t OneWireReset();
void OneWireWriteBit(uint8_t v);
uint8_t OneWireReadBit();
void OneWireWrite(uint8_t v);
uint8_t OneWireRead();
void OneWireSelect(const uint8_t rom[8]);
void OneWireResetSearch();
uint8_t OneWireSearch(uint8_t *newAddr);
boolean OneWireCrc8(uint8_t *addr);
void Ds18x20Init();
void Ds18x20Convert();
bool Ds18x20Read(uint8_t sensor);
void Ds18x20Name(uint8_t sensor);
void Ds18x20EverySecond();
void Ds18x20Show(boolean json);
boolean Xsns05(byte function);
void Ds18x20Init();
void Ds18x20Search();
uint8_t Ds18x20Sensors();
String Ds18x20Addresses(uint8_t sensor);
void Ds18x20Convert();
boolean Ds18x20Read(uint8_t sensor, float &t);
void Ds18x20Type(uint8_t sensor);
void Ds18x20Show(boolean json);
boolean Xsns05(byte function);
void DhtReadPrep();
int32_t DhtExpectPulse(byte sensor, bool level);
boolean DhtRead(byte sensor);
void DhtReadTempHum(byte sensor);
boolean DhtSetup(byte pin, byte type);
void DhtInit();
void DhtEverySecond();
void DhtShow(boolean json);
boolean Xsns06(byte function);
boolean ShtReset();
boolean ShtSendCommand(const byte cmd);
boolean ShtAwaitResult();
int ShtReadData();
boolean ShtRead();
void ShtDetect();
void ShtEverySecond();
void ShtShow(boolean json);
boolean Xsns07(byte function);
uint8_t HtuCheckCrc8(uint16_t data);
uint8_t HtuReadDeviceId(void);
void HtuSetResolution(uint8_t resolution);
void HtuReset(void);
void HtuHeater(uint8_t heater);
void HtuInit();
boolean HtuRead();
void HtuDetect();
void HtuEverySecond();
void HtuShow(boolean json);
boolean Xsns08(byte function);
boolean Bmp180Calibration(uint8_t bmp_idx);
void Bmp180Read(uint8_t bmp_idx);
boolean Bmx280Calibrate(uint8_t bmp_idx);
void Bme280Read(uint8_t bmp_idx);
static void BmeDelayMs(uint32_t ms);
boolean Bme680Init(uint8_t bmp_idx);
void Bme680Read(uint8_t bmp_idx);
void BmpDetect();
void BmpRead();
void BmpEverySecond();
void BmpShow(boolean json);
boolean Xsns09(byte function);
bool Bh1750Read();
void Bh1750Detect();
void Bh1750EverySecond();
void Bh1750Show(boolean json);
boolean Xsns10(byte function);
void Veml6070Detect(void);
void Veml6070UvTableInit(void);
void Veml6070EverySecond(void);
void Veml6070ModeCmd(boolean mode_cmd);
uint16_t Veml6070ReadUv(void);
double Veml6070UvRiskLevel(uint16_t uv_level);
double Veml6070UvPower(double uvrisk);
void Veml6070Show(boolean json);
boolean Xsns11(byte function);
void Ads1115StartComparator(uint8_t channel, uint16_t mode);
int16_t Ads1115GetConversion(uint8_t channel);
void Ads1115Detect();
void Ads1115Show(boolean json);
boolean Xsns12(byte function);
int16_t Ads1115GetConversion(byte channel);
void Ads1115Detect();
void Ads1115Show(boolean json);
boolean Xsns12(byte function);
bool Ina219SetCalibration(uint8_t mode);
float Ina219GetShuntVoltage_mV();
float Ina219GetBusVoltage_V();
float Ina219GetCurrent_mA();
bool Ina219Read();
bool Ina219CommandSensor();
void Ina219Detect();
void Ina219EverySecond();
void Ina219Show(boolean json);
boolean Xsns13(byte function);
bool Sht3xRead(float &t, float &h, uint8_t sht3x_address);
void Sht3xDetect();
void Sht3xShow(boolean json);
boolean Xsns14(byte function);
byte MhzCalculateChecksum(byte *array);
size_t MhzSendCmd(byte command_id);
bool MhzCheckAndApplyFilter(uint16_t ppm, uint8_t s);
void MhzEverySecond();
bool MhzCommandSensor();
void MhzInit();
void MhzShow(boolean json);
boolean Xsns15(byte function);
bool Tsl2561Read();
void Tsl2561Detect();
void Tsl2561EverySecond();
void Tsl2561Show(boolean json);
boolean Xsns16(byte function);
void Senseair250ms();
void SenseairInit();
void SenseairShow(boolean json);
boolean Xsns17(byte function);
boolean PmsReadData();
void PmsSecond();
void PmsInit();
void PmsShow(boolean json);
boolean Xsns18(byte function);
void MGSInit();
boolean MGSPrepare();
char* measure_gas(int gas_type, char* buffer);
void MGSShow(boolean json);
boolean Xsns19(byte function);
void NovaSdsSetWorkPeriod();
bool NovaSdsReadData();
void NovaSdsSecond();
void NovaSdsInit();
void NovaSdsShow(boolean json);
boolean Xsns20(byte function);
void Sgp30Update();
void Sgp30Show(boolean json);
boolean Xsns21(byte function);
void Sr04Init();
boolean Sr04Read(uint16_t *distance);
uint16_t Sr04Ping(uint16_t max_cm_distance);
uint16_t Sr04GetSamples(uint8_t it, uint16_t max_cm_distance);
void Sr04Show(boolean json);
boolean Xsns22(byte function);
bool SDM120_ModbusReceiveReady();
void SDM120_ModbusSend(uint8_t function_code, uint16_t start_address, uint16_t register_count);
uint8_t SDM120_ModbusReceive(float *value);
uint16_t SDM120_calculateCRC(uint8_t *frame, uint8_t num);
void SDM120250ms();
void SDM120Init();
void SDM120Show(boolean json);
boolean Xsns23(byte function);
uint8_t Si1145ReadByte(uint8_t reg);
uint16_t Si1145ReadHalfWord(uint8_t reg);
bool Si1145WriteByte(uint8_t reg, uint16_t val);
uint8_t Si1145WriteParamData(uint8_t p, uint8_t v);
bool Si1145Present();
void Si1145Reset();
void Si1145DeInit();
boolean Si1145Begin();
uint16_t Si1145ReadUV();
uint16_t Si1145ReadVisible();
uint16_t Si1145ReadIR();
void Si1145Update();
void Si1145Show(boolean json);
boolean Xsns24(byte function);
bool SDM630_ModbusReceiveReady();
void SDM630_ModbusSend(uint8_t function_code, uint16_t start_address, uint16_t register_count);
uint8_t SDM630_ModbusReceive(float *value);
uint16_t SDM630_calculateCRC(uint8_t *frame, uint8_t num);
void SDM630250ms();
void SDM630Init();
void SDM630Show(boolean json);
boolean Xsns25(byte function);
void LM75ADDetect(void);
float LM75ADGetTemp(void);
void LM75ADShow(boolean json);
boolean Xsns26(byte function);
int8_t wireReadDataBlock( uint8_t reg,
                                        uint8_t *val,
                                        uint16_t len);
void calculateColorTemperature();
float powf(const float x, const float y);
bool APDS9960_init();
uint8_t getMode();
void setMode(uint8_t mode, uint8_t enable);
void enableLightSensor();
void disableLightSensor();
void enableProximitySensor();
void disableProximitySensor();
void enableGestureSensor();
void disableGestureSensor();
bool isGestureAvailable();
int16_t readGesture();
void enablePower();
void disablePower();
void readAllColorAndProximityData();
void resetGestureParameters();
bool processGestureData();
bool decodeGesture();
void handleGesture();
void APDS9960_adjustATime(void);
void APDS9960_loop();
bool APDS9960_detect(void);
void APDS9960_show(boolean json);
bool APDS9960CommandSensor();
boolean Xsns27(byte function);
void Tm16XXSend(byte data);
void Tm16XXSendCommand(byte cmd);
void TM16XXSendData(byte address, byte data);
byte Tm16XXReceive();
void Tm16XXClearDisplay();
void Tm1638SetLED(byte color, byte pos);
void Tm1638SetLEDs(word leds);
byte Tm1638GetButtons();
void TmInit();
void TmLoop();
boolean Xsns28(byte function);
void MCP230xx_CheckForIntCounter(void);
const char* IntModeTxt(uint8_t intmo);
uint8_t MCP230xx_readGPIO(uint8_t port);
void MCP230xx_ApplySettings(void);
void MCP230xx_Detect(void);
void MCP230xx_CheckForInterrupt(void);
void MCP230xx_Show(boolean json);
void MCP230xx_SetOutPin(uint8_t pin,uint8_t pinstate);
void MCP230xx_Reset(uint8_t pinmode);
bool MCP230xx_Command(void);
void MCP230xx_UpdateWebData(void);
void MCP230xx_OutputTelemetry(void);
void MCP230xx_Interrupt_Counter_Report(void);
boolean Xsns29(byte function);
void Mpr121Init(struct mpr121 *pS);
void Mpr121Show(struct mpr121 *pS, byte function);
boolean Xsns30(byte function);
void CCS811Update();
void CCS811Show(boolean json);
boolean Xsns31(byte function);
void MPU_6050PerformReading();
void MPU_6050Detect();
void MPU_6050Show(boolean json);
boolean Xsns32(byte function);
boolean DS3231Detect();
uint8_t bcd2dec(uint8_t n);
uint8_t dec2bcd(uint8_t n);
uint32_t ReadFromDS3231();
void SetDS3231Time (uint32_t epoch_time);
boolean Xsns33(byte function);
bool HxIsReady(uint16_t timeout);
long HxRead();
void HxReset();
void HxCalibrationStateTextJson(uint8_t msg_id);
bool HxCommand();
long HxWeight();
void HxInit();
void HxEvery100mSecond();
void HxShow(boolean json);
void HandleHxAction();
void HxSaveSettings();
void HxLogUpdates();
boolean Xsns34(byte function);
void Tx20StartRead();
void Tx20Read();
void Tx20Init();
void Tx20Show(boolean json);
boolean Xsns35(byte function);
uint8_t XsnsPresent();
boolean XsnsNextCall(byte Function);
boolean XsnsCall(byte Function);
#line 207 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/sonoff.ino"
char* Format(char* output, const char* input, int size)
{
  char *token;
  uint8_t digits = 0;

  if (strstr(input, "%")) {
    strlcpy(output, input, size);
    token = strtok(output, "%");
    if (strstr(input, "%") == input) {
      output[0] = '\0';
    } else {
      token = strtok(NULL, "");
    }
    if (token != NULL) {
      digits = atoi(token);
      if (digits) {
        if (strchr(token, 'd')) {
          snprintf_P(output, size, PSTR("%s%c0%dd"), output, '%', digits);
          snprintf_P(output, size, output, ESP.getChipId() & 0x1fff);
        } else {
          snprintf_P(output, size, PSTR("%s%c0%dX"), output, '%', digits);
          snprintf_P(output, size, output, ESP.getChipId());
        }
      } else {
        if (strchr(token, 'd')) {
          snprintf_P(output, size, PSTR("%s%d"), output, ESP.getChipId());
          digits = 8;
        }
      }
    }
  }
  if (!digits) strlcpy(output, input, size);
  return output;
}

char* GetOtaUrl(char *otaurl, size_t otaurl_size)
{
  if (strstr(Settings.ota_url, "%04d") != NULL) {
    snprintf(otaurl, otaurl_size, Settings.ota_url, ESP.getChipId() & 0x1fff);
  }
  else if (strstr(Settings.ota_url, "%d") != NULL) {
    snprintf_P(otaurl, otaurl_size, Settings.ota_url, ESP.getChipId());
  }
  else {
    snprintf(otaurl, otaurl_size, Settings.ota_url);
  }
  return otaurl;
}

void GetTopic_P(char *stopic, byte prefix, char *topic, const char* subtopic)
{




  char romram[CMDSZ];
  String fulltopic;

  snprintf_P(romram, sizeof(romram), subtopic);
  if (fallback_topic_flag) {
    fulltopic = FPSTR(kPrefixes[prefix]);
    fulltopic += F("/");
    fulltopic += mqtt_client;
  } else {
    fulltopic = Settings.mqtt_fulltopic;
    if ((0 == prefix) && (-1 == fulltopic.indexOf(F(MQTT_TOKEN_PREFIX)))) {
      fulltopic += F("/" MQTT_TOKEN_PREFIX);
    }
    for (byte i = 0; i < 3; i++) {
      if ('\0' == Settings.mqtt_prefix[i][0]) {
        snprintf_P(Settings.mqtt_prefix[i], sizeof(Settings.mqtt_prefix[i]), kPrefixes[i]);
      }
    }
    fulltopic.replace(F(MQTT_TOKEN_PREFIX), Settings.mqtt_prefix[prefix]);
    fulltopic.replace(F(MQTT_TOKEN_TOPIC), topic);
    fulltopic.replace(F(MQTT_TOKEN_HOSTNAME), my_hostname);
    String token_id = WiFi.macAddress();
    token_id.replace(":", "");
    fulltopic.replace(F(MQTT_TOKEN_ID), token_id);
  }
  fulltopic.replace(F("#"), "");
  fulltopic.replace(F("//"), "/");
  if (!fulltopic.endsWith("/")) fulltopic += "/";
  snprintf_P(stopic, TOPSZ, PSTR("%s%s"), fulltopic.c_str(), romram);
}

char* GetStateText(byte state)
{
  if (state > 3) state = 1;
  return Settings.state_text[state];
}



void SetLatchingRelay(power_t lpower, uint8_t state)
{





  if (state && !latching_relay_pulse) {
    latching_power = lpower;
    latching_relay_pulse = 2;
  }

  for (byte i = 0; i < devices_present; i++) {
    uint8_t port = (i << 1) + ((latching_power >> i) &1);
    if (pin[GPIO_REL1 +port] < 99) {
      digitalWrite(pin[GPIO_REL1 +port], bitRead(rel_inverted, port) ? !state : state);
    }
  }
}

void SetDevicePower(power_t rpower, int source)
{
  uint8_t state;

  ShowSource(source);

  if (POWER_ALL_ALWAYS_ON == Settings.poweronstate) {
    power = (1 << devices_present) -1;
    rpower = power;
  }
  if (Settings.flag.interlock) {
    power_t mask = 1;
    uint8_t count = 0;
    for (byte i = 0; i < devices_present; i++) {
      if (rpower & mask) count++;
      mask <<= 1;
    }
    if (count > 1) {
      power = 0;
      rpower = 0;
    }
  }

  XdrvMailbox.index = rpower;
  XdrvCall(FUNC_SET_POWER);

  XdrvMailbox.index = rpower;
  XdrvMailbox.payload = source;
  if (XdrvCall(FUNC_SET_DEVICE_POWER)) {

  }
  else if ((SONOFF_DUAL == Settings.module) || (CH4 == Settings.module)) {
    Serial.write(0xA0);
    Serial.write(0x04);
    Serial.write(rpower &0xFF);
    Serial.write(0xA1);
    Serial.write('\n');
    Serial.flush();
  }
  else if (EXS_RELAY == Settings.module) {
    SetLatchingRelay(rpower, 1);
  }
  else {
    for (byte i = 0; i < devices_present; i++) {
      state = rpower &1;
      if ((i < MAX_RELAYS) && (pin[GPIO_REL1 +i] < 99)) {
        digitalWrite(pin[GPIO_REL1 +i], bitRead(rel_inverted, i) ? !state : state);
      }
      rpower >>= 1;
    }
  }
}

void SetLedPower(uint8_t state)
{
  if (state) state = 1;
  digitalWrite(pin[GPIO_LED1], (bitRead(led_inverted, 0)) ? !state : state);
}

uint8_t GetFanspeed()
{
  uint8_t fanspeed = 0;
# 391 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/sonoff.ino"
    fanspeed = (uint8_t)(power &0xF) >> 1;
    if (fanspeed) { fanspeed = (fanspeed >> 1) +1; }

  return fanspeed;
}

void SetFanspeed(uint8_t fanspeed)
{
 for (byte i = 0; i < 3; i++) {
    uint8_t state = kIFan02Speed[fanspeed][i];

    ExecuteCommandPower(i +2, state, SRC_IGNORE);
  }
}

void SetPulseTimer(uint8_t index, uint16_t time)
{
  pulse_timer[index] = (time > 111) ? millis() + (1000 * (time - 100)) : (time > 0) ? millis() + (100 * time) : 0L;
}

uint16_t GetPulseTimer(uint8_t index)
{
  uint16_t result = 0;

  long time = TimePassedSince(pulse_timer[index]);
  if (time < 0) {
    time *= -1;
    result = (time > 11100) ? (time / 1000) + 100 : (time > 0) ? time / 100 : 0;
  }
  return result;
}



void MqttDataHandler(char* topic, byte* data, unsigned int data_len)
{
  char *str;

  if (!strcmp(Settings.mqtt_prefix[0],Settings.mqtt_prefix[1])) {
    str = strstr(topic,Settings.mqtt_prefix[0]);
    if ((str == topic) && mqtt_cmnd_publish) {
      if (mqtt_cmnd_publish > 3) {
        mqtt_cmnd_publish -= 3;
      } else {
        mqtt_cmnd_publish = 0;
      }
      return;
    }
  }

  char topicBuf[TOPSZ];
  char dataBuf[data_len+1];
  char command [CMDSZ];
  char stemp1[TOPSZ];
  char *p;
  char *type = NULL;
  byte jsflg = 0;
  byte lines = 1;
  uint8_t grpflg = 0;

  uint16_t i = 0;
  uint16_t index;
  uint32_t address;

  ShowFreeMem(PSTR("MqttDataHandler"));

  strlcpy(topicBuf, topic, sizeof(topicBuf));
  for (i = 0; i < data_len; i++) {
    if (!isspace(data[i])) break;
  }
  data_len -= i;
  memcpy(dataBuf, data +i, sizeof(dataBuf));
  dataBuf[sizeof(dataBuf)-1] = 0;

  if (topicBuf[0] != '/') { ShowSource(SRC_MQTT); }

  snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_RESULT D_RECEIVED_TOPIC " %s, " D_DATA_SIZE " %d, " D_DATA " %s"),
    topicBuf, data_len, dataBuf);
  AddLog(LOG_LEVEL_DEBUG_MORE);


  if (XdrvMqttData(topicBuf, sizeof(topicBuf), dataBuf, sizeof(dataBuf))) return;

  grpflg = (strstr(topicBuf, Settings.mqtt_grptopic) != NULL);
  fallback_topic_flag = (strstr(topicBuf, mqtt_client) != NULL);
  type = strrchr(topicBuf, '/');

  index = 1;
  if (type != NULL) {
    type++;
    for (i = 0; i < strlen(type); i++) {
      type[i] = toupper(type[i]);
    }
    while (isdigit(type[i-1])) {
      i--;
    }
    if (i < strlen(type)) {
      index = atoi(type +i);

    }
    type[i] = '\0';
  }

  snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_RESULT D_GROUP " %d, " D_INDEX " %d, " D_COMMAND " %s, " D_DATA " %s"),
    grpflg, index, type, dataBuf);
  AddLog(LOG_LEVEL_DEBUG);

  if (type != NULL) {
    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"" D_JSON_COMMAND "\":\"" D_JSON_ERROR "\"}"));
    if (Settings.ledstate &0x02) blinks++;

    if (!strcmp(dataBuf,"?")) data_len = 0;
    int16_t payload = -99;
    uint16_t payload16 = 0;
    long payload32 = strtol(dataBuf, &p, 10);
    if (p != dataBuf) {
      payload = (int16_t) payload32;
      payload16 = (uint16_t) payload32;
    } else {
      payload32 = 0;
    }
    backlog_delay = millis() + (100 * MIN_BACKLOG_DELAY);

    int temp_payload = GetStateNumber(dataBuf);
    if (temp_payload > -1) { payload = temp_payload; }




    int command_code = GetCommandCode(command, sizeof(command), type, kTasmotaCommands);
    if (-1 == command_code) {
      if (!XdrvCommand(grpflg, type, index, dataBuf, data_len, payload, payload16)) {
        type = NULL;
      }
    }
    else if (CMND_BACKLOG == command_code) {
      if (data_len) {
        uint8_t bl_pointer = (!backlog_pointer) ? MAX_BACKLOG -1 : backlog_pointer;
        bl_pointer--;
        char *blcommand = strtok(dataBuf, ";");
        while ((blcommand != NULL) && (backlog_index != bl_pointer)) {
          while(true) {
            blcommand = LTrim(blcommand);
            if (!strncasecmp_P(blcommand, PSTR(D_CMND_BACKLOG), strlen(D_CMND_BACKLOG))) {
              blcommand += strlen(D_CMND_BACKLOG);
            } else {
              break;
            }
          }
          if (*blcommand != '\0') {
            backlog[backlog_index] = String(blcommand);
            backlog_index++;
            if (backlog_index >= MAX_BACKLOG) backlog_index = 0;
          }
          blcommand = strtok(NULL, ";");
        }

        mqtt_data[0] = '\0';
      } else {
        uint8_t blflag = (backlog_pointer == backlog_index);
        backlog_pointer = backlog_index;
        snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_SVALUE, command, blflag ? D_JSON_EMPTY : D_JSON_ABORTED);
      }
    }
    else if (CMND_DELAY == command_code) {
      if ((payload >= MIN_BACKLOG_DELAY) && (payload <= 3600)) {
        backlog_delay = millis() + (100 * payload);
      }
      uint16_t bl_delay = 0;
      long bl_delta = TimePassedSince(backlog_delay);
      if (bl_delta < 0) { bl_delay = (bl_delta *-1) / 100; }
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_NVALUE, command, bl_delay);
    }
    else if ((CMND_POWER == command_code) && (index > 0) && (index <= devices_present)) {
      if ((payload < 0) || (payload > 4)) payload = 9;

      ExecuteCommandPower(index, payload, SRC_IGNORE);
      fallback_topic_flag = 0;
      return;
    }
    else if ((CMND_FANSPEED == command_code) && (SONOFF_IFAN02 == Settings.module)) {
      if (data_len > 0) {
        if ('-' == dataBuf[0]) {
          payload = (int16_t)GetFanspeed() -1;
          if (payload < 0) { payload = 3; }
        }
        else if ('+' == dataBuf[0]) {
          payload = GetFanspeed() +1;
          if (payload > 3) { payload = 0; }
        }
      }
      if ((payload >= 0) && (payload <= 3) && (payload != GetFanspeed())) {
        SetFanspeed(payload);
      }
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_NVALUE, command, GetFanspeed());
    }
    else if (CMND_STATUS == command_code) {
      if ((payload < 0) || (payload > MAX_STATUS)) payload = 99;
      PublishStatus(payload);
      fallback_topic_flag = 0;
      return;
    }
    else if (CMND_STATE == command_code) {
      mqtt_data[0] = '\0';
      MqttShowState();
    }
    else if (CMND_SLEEP == command_code) {
      if ((payload >= 0) && (payload < 251)) {
        Settings.sleep = payload;
        sleep = payload;
        WiFiSetSleepMode();
      }
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_NVALUE_UNIT_NVALUE_UNIT, command, sleep, (Settings.flag.value_units) ? " " D_UNIT_MILLISECOND : "", Settings.sleep, (Settings.flag.value_units) ? " " D_UNIT_MILLISECOND : "");
    }
    else if ((CMND_UPGRADE == command_code) || (CMND_UPLOAD == command_code)) {




      if (((1 == data_len) && (1 == payload)) || ((data_len >= 3) && NewerVersion(dataBuf))) {
        ota_state_flag = 3;

        snprintf_P(mqtt_data, sizeof(mqtt_data), "{\"%s\":\"" D_JSON_VERSION " %s " D_JSON_FROM " %s\"}", command, my_version, GetOtaUrl(stemp1, sizeof(stemp1)));
      } else {
        snprintf_P(mqtt_data, sizeof(mqtt_data), "{\"%s\":\"" D_JSON_ONE_OR_GT "\"}", command, my_version);
      }
    }
    else if (CMND_OTAURL == command_code) {
      if ((data_len > 0) && (data_len < sizeof(Settings.ota_url))) {
        strlcpy(Settings.ota_url, (SC_DEFAULT == Shortcut(dataBuf)) ? OTA_URL : dataBuf, sizeof(Settings.ota_url));
      }
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_SVALUE, command, Settings.ota_url);
    }
    else if (CMND_SERIALLOG == command_code) {
      if ((payload >= LOG_LEVEL_NONE) && (payload <= LOG_LEVEL_ALL)) {
        Settings.flag.mqtt_serial = 0;
        SetSeriallog(payload);
      }
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_NVALUE_ACTIVE_NVALUE, command, Settings.seriallog_level, seriallog_level);
    }
    else if (CMND_RESTART == command_code) {
      switch (payload) {
      case 1:
        restart_flag = 2;
        snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_SVALUE, command, D_JSON_RESTARTING);
        break;
      case 99:
        AddLog_P(LOG_LEVEL_INFO, PSTR(D_LOG_APPLICATION D_RESTARTING));
        EspRestart();
        break;
      default:
        snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_SVALUE, command, D_JSON_ONE_TO_RESTART);
      }
    }
    else if ((CMND_POWERONSTATE == command_code) && (Settings.module != MOTOR)) {







      if ((payload >= POWER_ALL_OFF) && (payload <= POWER_ALL_OFF_PULSETIME_ON)) {
        Settings.poweronstate = payload;
        if (POWER_ALL_ALWAYS_ON == Settings.poweronstate) {
          for (byte i = 1; i <= devices_present; i++) {
            ExecuteCommandPower(i, POWER_ON, SRC_IGNORE);
          }
        }
      }
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_NVALUE, command, Settings.poweronstate);
    }
    else if ((CMND_PULSETIME == command_code) && (index > 0) && (index <= MAX_PULSETIMERS)) {
      if (data_len > 0) {
        Settings.pulse_timer[index -1] = payload16;
        SetPulseTimer(index -1, payload16);
      }
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_INDEX_NVALUE_ACTIVE_NVALUE, command, index, Settings.pulse_timer[index -1], GetPulseTimer(index -1));
    }
    else if (CMND_BLINKTIME == command_code) {
      if ((payload > 1) && (payload <= 3600)) {
        Settings.blinktime = payload;
        if (blink_timer > 0) { blink_timer = millis() + (100 * payload); }
      }
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_NVALUE, command, Settings.blinktime);
    }
    else if (CMND_BLINKCOUNT == command_code) {
      if (data_len > 0) {
        Settings.blinkcount = payload16;
        if (blink_counter) blink_counter = Settings.blinkcount *2;
      }
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_NVALUE, command, Settings.blinkcount);
    }
    else if (CMND_SAVEDATA == command_code) {
      if ((payload >= 0) && (payload <= 3600)) {
        Settings.save_data = payload;
        save_data_counter = Settings.save_data;
      }
      SettingsSaveAll();
      if (Settings.save_data > 1) {
        snprintf_P(stemp1, sizeof(stemp1), PSTR(D_JSON_EVERY " %d " D_UNIT_SECOND), Settings.save_data);
      }
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_SVALUE, command, (Settings.save_data > 1) ? stemp1 : GetStateText(Settings.save_data));
    }
    else if ((CMND_SENSOR == command_code) || (CMND_DRIVER == command_code)) {
      XdrvMailbox.index = index;
      XdrvMailbox.data_len = data_len;
      XdrvMailbox.payload16 = payload16;
      XdrvMailbox.payload = payload;
      XdrvMailbox.grpflg = grpflg;
      XdrvMailbox.topic = command;
      XdrvMailbox.data = dataBuf;
      if (CMND_SENSOR == command_code) {
        XsnsCall(FUNC_COMMAND);
      } else {
        XdrvCall(FUNC_COMMAND);
      }
    }
    else if ((CMND_SETOPTION == command_code) && (index < 82)) {
      byte ptype;
      byte pindex;
      if (index <= 31) {
        ptype = 0;
        pindex = index;
      }
      else if (index <= 49) {
        ptype = 2;
        pindex = index -32;
      }
      else {
        ptype = 1;
        pindex = index -50;
      }
      if (payload >= 0) {
        if (0 == ptype) {
          if (payload <= 1) {
            switch (pindex) {
              case 5:
              case 6:
              case 7:
              case 9:
              case 22:
              case 23:
              case 25:
              case 27:
                ptype = 99;
                break;
              case 3:
              case 15:
                restart_flag = 2;
              default:
                bitWrite(Settings.flag.data, pindex, payload);
            }
            if (12 == pindex) {
              stop_flash_rotate = payload;
              SettingsSave(2);
            }
#ifdef USE_HOME_ASSISTANT
            if ((19 == pindex) || (30 == pindex)) {
              HAssDiscovery(1);
            }
#endif
          }
        }
        else if (1 == ptype) {
          if (payload <= 1) {
            bitWrite(Settings.flag3.data, pindex, payload);
          }
        }
        else {
          uint8_t param_low = 0;
          uint8_t param_high = 255;
          switch (pindex) {
            case P_HOLD_TIME:
            case P_MAX_POWER_RETRY:
              param_low = 1;
              param_high = 250;
              break;
          }
          if ((payload >= param_low) && (payload <= param_high)) {
            Settings.param[pindex] = payload;
          }
        }
      }
      if (ptype < 99) {
        if (2 == ptype) snprintf_P(stemp1, sizeof(stemp1), PSTR("%d"), Settings.param[pindex]);
        snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_INDEX_SVALUE, command, index, (2 == ptype) ? stemp1 : (1 == ptype) ? GetStateText(bitRead(Settings.flag3.data, pindex)) : GetStateText(bitRead(Settings.flag.data, pindex)));
      }
    }
    else if (CMND_TEMPERATURE_RESOLUTION == command_code) {
      if ((payload >= 0) && (payload <= 3)) {
        Settings.flag2.temperature_resolution = payload;
      }
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_NVALUE, command, Settings.flag2.temperature_resolution);
    }
    else if (CMND_HUMIDITY_RESOLUTION == command_code) {
      if ((payload >= 0) && (payload <= 3)) {
        Settings.flag2.humidity_resolution = payload;
      }
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_NVALUE, command, Settings.flag2.humidity_resolution);
    }
    else if (CMND_PRESSURE_RESOLUTION == command_code) {
      if ((payload >= 0) && (payload <= 3)) {
        Settings.flag2.pressure_resolution = payload;
      }
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_NVALUE, command, Settings.flag2.pressure_resolution);
    }
    else if (CMND_POWER_RESOLUTION == command_code) {
      if ((payload >= 0) && (payload <= 3)) {
        Settings.flag2.wattage_resolution = payload;
      }
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_NVALUE, command, Settings.flag2.wattage_resolution);
    }
    else if (CMND_VOLTAGE_RESOLUTION == command_code) {
      if ((payload >= 0) && (payload <= 3)) {
        Settings.flag2.voltage_resolution = payload;
      }
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_NVALUE, command, Settings.flag2.voltage_resolution);
    }
    else if (CMND_FREQUENCY_RESOLUTION == command_code) {
      if ((payload >= 0) && (payload <= 3)) {
        Settings.flag2.frequency_resolution = payload;
      }
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_NVALUE, command, Settings.flag2.frequency_resolution);
    }
    else if (CMND_CURRENT_RESOLUTION == command_code) {
      if ((payload >= 0) && (payload <= 3)) {
        Settings.flag2.current_resolution = payload;
      }
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_NVALUE, command, Settings.flag2.current_resolution);
    }
    else if (CMND_ENERGY_RESOLUTION == command_code) {
      if ((payload >= 0) && (payload <= 5)) {
        Settings.flag2.energy_resolution = payload;
      }
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_NVALUE, command, Settings.flag2.energy_resolution);
    }
    else if (CMND_WEIGHT_RESOLUTION == command_code) {
      if ((payload >= 0) && (payload <= 3)) {
        Settings.flag2.weight_resolution = payload;
      }
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_NVALUE, command, Settings.flag2.weight_resolution);
    }
    else if (CMND_MODULE == command_code) {
      if ((payload > 0) && (payload <= MAXMODULE)) {
        payload--;
        Settings.last_module = Settings.module;
        Settings.module = payload;
        if (Settings.last_module != payload) {
          for (byte i = 0; i < MAX_GPIO_PIN; i++) {
            Settings.my_gp.io[i] = 0;
          }
        }
        restart_flag = 2;
      }
      snprintf_P(stemp1, sizeof(stemp1), kModules[Settings.module].name);
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_NVALUE_SVALUE, command, Settings.module +1, stemp1);
    }
    else if (CMND_MODULES == command_code) {
      for (byte i = 0; i < MAXMODULE; i++) {
        if (!jsflg) {
          snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"" D_CMND_MODULES "%d\":["), lines);
        } else {
          snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s,"), mqtt_data);
        }
        jsflg = 1;
        snprintf_P(stemp1, sizeof(stemp1), kModules[i].name);
        snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s\"%d (%s)\""), mqtt_data, i +1, stemp1);
        if ((strlen(mqtt_data) > (LOGSZ - TOPSZ)) || (i == MAXMODULE -1)) {
          snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s]}"), mqtt_data);
          MqttPublishPrefixTopic_P(RESULT_OR_STAT, type);
          jsflg = 0;
          lines++;
        }
      }
      mqtt_data[0] = '\0';
    }
    else if ((CMND_GPIO == command_code) && (index < MAX_GPIO_PIN)) {
      mytmplt cmodule;
      memcpy_P(&cmodule, &kModules[Settings.module], sizeof(cmodule));
      if ((GPIO_USER == ValidGPIO(index, cmodule.gp.io[index])) && (payload >= 0) && (payload < GPIO_SENSOR_END)) {
        for (byte i = 0; i < MAX_GPIO_PIN; i++) {
          if ((GPIO_USER == ValidGPIO(i, cmodule.gp.io[i])) && (Settings.my_gp.io[i] == payload)) {
            Settings.my_gp.io[i] = 0;
          }
        }
        Settings.my_gp.io[index] = payload;
        restart_flag = 2;
      }
      snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{"));
      for (byte i = 0; i < MAX_GPIO_PIN; i++) {
        if (GPIO_USER == ValidGPIO(i, cmodule.gp.io[i])) {
          if (jsflg) snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s,"), mqtt_data);
          jsflg = 1;
          snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s\"" D_CMND_GPIO "%d\":\"%d (%s)\""),
            mqtt_data, i, Settings.my_gp.io[i], GetTextIndexed(stemp1, sizeof(stemp1), Settings.my_gp.io[i], kSensorNames));
        }
      }
      if (jsflg) {
        snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s}"), mqtt_data);
      } else {
        snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_SVALUE, command, D_JSON_NOT_SUPPORTED);
      }
    }
    else if (CMND_GPIOS == command_code) {
      mytmplt cmodule;
      memcpy_P(&cmodule, &kModules[Settings.module], sizeof(cmodule));
      for (byte i = 0; i < GPIO_SENSOR_END; i++) {
        if (!GetUsedInModule(i, cmodule.gp.io)) {
          if (!jsflg) {
            snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"" D_CMND_GPIOS "%d\":["), lines);
          } else {
            snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s,"), mqtt_data);
          }
          jsflg = 1;
          snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s\"%d (%s)\""), mqtt_data, i, GetTextIndexed(stemp1, sizeof(stemp1), i, kSensorNames));
          if ((strlen(mqtt_data) > (LOGSZ - TOPSZ)) || (i == GPIO_SENSOR_END -1)) {
            snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s]}"), mqtt_data);
            MqttPublishPrefixTopic_P(RESULT_OR_STAT, type);
            jsflg = 0;
            lines++;
          }
        }
      }
      mqtt_data[0] = '\0';
    }
    else if ((CMND_PWM == command_code) && pwm_present && (index > 0) && (index <= MAX_PWMS)) {
      if ((payload >= 0) && (payload <= Settings.pwm_range) && (pin[GPIO_PWM1 + index -1] < 99)) {
        Settings.pwm_value[index -1] = payload;
        analogWrite(pin[GPIO_PWM1 + index -1], bitRead(pwm_inverted, index -1) ? Settings.pwm_range - payload : payload);
      }
      snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{"));
      MqttShowPWMState();
      snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s}"), mqtt_data);
    }
    else if (CMND_PWMFREQUENCY == command_code) {
      if ((1 == payload) || ((payload >= PWM_MIN) && (payload <= PWM_MAX))) {
        Settings.pwm_frequency = (1 == payload) ? PWM_FREQ : payload;
        analogWriteFreq(Settings.pwm_frequency);
      }
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_NVALUE, command, Settings.pwm_frequency);
    }
    else if (CMND_PWMRANGE == command_code) {
      if ((1 == payload) || ((payload > 254) && (payload < 1024))) {
        Settings.pwm_range = (1 == payload) ? PWM_RANGE : payload;
        for (byte i = 0; i < MAX_PWMS; i++) {
          if (Settings.pwm_value[i] > Settings.pwm_range) {
            Settings.pwm_value[i] = Settings.pwm_range;
          }
        }
        analogWriteRange(Settings.pwm_range);
      }
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_NVALUE, command, Settings.pwm_range);
    }
    else if ((CMND_COUNTER == command_code) && (index > 0) && (index <= MAX_COUNTERS)) {
      if ((data_len > 0) && (pin[GPIO_CNTR1 + index -1] < 99)) {
        if ((dataBuf[0] == '-') || (dataBuf[0] == '+')) {
          RtcSettings.pulse_counter[index -1] += payload32;
          Settings.pulse_counter[index -1] += payload32;
        } else {
          RtcSettings.pulse_counter[index -1] = payload32;
          Settings.pulse_counter[index -1] = payload32;
        }
      }
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_INDEX_LVALUE, command, index, RtcSettings.pulse_counter[index -1]);
    }
    else if ((CMND_COUNTERTYPE == command_code) && (index > 0) && (index <= MAX_COUNTERS)) {
      if ((payload >= 0) && (payload <= 1) && (pin[GPIO_CNTR1 + index -1] < 99)) {
        bitWrite(Settings.pulse_counter_type, index -1, payload &1);
        RtcSettings.pulse_counter[index -1] = 0;
        Settings.pulse_counter[index -1] = 0;
      }
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_INDEX_NVALUE, command, index, bitRead(Settings.pulse_counter_type, index -1));
    }
    else if (CMND_COUNTERDEBOUNCE == command_code) {
      if ((data_len > 0) && (payload16 < 32001)) {
        Settings.pulse_counter_debounce = payload16;
      }
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_NVALUE, command, Settings.pulse_counter_debounce);
    }
    else if (CMND_BUTTONDEBOUNCE == command_code) {
      if ((payload > 39) && (payload < 1001)) {
        Settings.button_debounce = payload;
      }
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_NVALUE, command, Settings.button_debounce);
    }
    else if (CMND_SWITCHDEBOUNCE == command_code) {
      if ((payload > 39) && (payload < 1001)) {
        Settings.switch_debounce = payload;
      }
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_NVALUE, command, Settings.switch_debounce);
    }
    else if (CMND_BAUDRATE == command_code) {
      if (payload32 > 0) {
        payload32 /= 1200;
        baudrate = (1 == payload) ? APP_BAUDRATE : payload32 * 1200;
        SetSerialBaudrate(baudrate);
      }
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_NVALUE, command, Settings.baudrate * 1200);
    }
    else if ((CMND_SERIALSEND == command_code) && (index > 0) && (index <= 5)) {
      SetSeriallog(LOG_LEVEL_NONE);
      Settings.flag.mqtt_serial = 1;
      Settings.flag.mqtt_serial_raw = (index > 3) ? 1 : 0;
      if (data_len > 0) {
        if (1 == index) {
          Serial.printf("%s\n", dataBuf);
        }
        else if (2 == index || 4 == index) {
          for (int i = 0; i < data_len; i++) {
            Serial.write(dataBuf[i]);
          }
        }
        else if (3 == index) {
          uint16_t dat_len = data_len;
          Serial.printf("%s", Unescape(dataBuf, &dat_len));
        }
        else if (5 == index) {
          SerialSendRaw(RemoveSpace(dataBuf));
        }
        snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_SVALUE, command, D_JSON_DONE);
      }
    }
    else if (CMND_SERIALDELIMITER == command_code) {
      if ((data_len > 0) && (payload < 256)) {
        if (payload > 0) {
          Settings.serial_delimiter = payload;
        } else {
          uint16_t dat_len = data_len;
          Unescape(dataBuf, &dat_len);
          Settings.serial_delimiter = dataBuf[0];
        }
      }
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_NVALUE, command, Settings.serial_delimiter);
    }
    else if (CMND_SYSLOG == command_code) {
      if ((payload >= LOG_LEVEL_NONE) && (payload <= LOG_LEVEL_ALL)) {
        Settings.syslog_level = payload;
        syslog_level = payload;
        syslog_timer = 0;
      }
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_NVALUE_ACTIVE_NVALUE, command, Settings.syslog_level, syslog_level);
    }
    else if (CMND_LOGHOST == command_code) {
      if ((data_len > 0) && (data_len < sizeof(Settings.syslog_host))) {
        strlcpy(Settings.syslog_host, (SC_DEFAULT == Shortcut(dataBuf)) ? SYS_LOG_HOST : dataBuf, sizeof(Settings.syslog_host));
      }
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_SVALUE, command, Settings.syslog_host);
    }
    else if (CMND_LOGPORT == command_code) {
      if (payload16 > 0) {
        Settings.syslog_port = (1 == payload16) ? SYS_LOG_PORT : payload16;
      }
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_NVALUE, command, Settings.syslog_port);
    }
    else if ((CMND_IPADDRESS == command_code) && (index > 0) && (index <= 4)) {
      if (ParseIp(&address, dataBuf)) {
        Settings.ip_address[index -1] = address;

      }
      snprintf_P(stemp1, sizeof(stemp1), PSTR(" (%s)"), WiFi.localIP().toString().c_str());
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_INDEX_SVALUE_SVALUE, command, index, IPAddress(Settings.ip_address[index -1]).toString().c_str(), (1 == index) ? stemp1:"");
    }
    else if ((CMND_NTPSERVER == command_code) && (index > 0) && (index <= 3)) {
      if ((data_len > 0) && (data_len < sizeof(Settings.ntp_server[0]))) {
        strlcpy(Settings.ntp_server[index -1], (SC_CLEAR == Shortcut(dataBuf)) ? "" : (SC_DEFAULT == Shortcut(dataBuf)) ? (1==index)?NTP_SERVER1:(2==index)?NTP_SERVER2:NTP_SERVER3 : dataBuf, sizeof(Settings.ntp_server[0]));
        for (i = 0; i < strlen(Settings.ntp_server[index -1]); i++) {
          if (Settings.ntp_server[index -1][i] == ',') Settings.ntp_server[index -1][i] = '.';
        }

        ntp_force_sync = 1;
      }
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_INDEX_SVALUE, command, index, Settings.ntp_server[index -1]);
    }
    else if (CMND_AP == command_code) {
      if ((payload >= 0) && (payload <= 2)) {
        switch (payload) {
        case 0:
          Settings.sta_active ^= 1;
          break;
        case 1:
        case 2:
          Settings.sta_active = payload -1;
        }
        restart_flag = 2;
      }
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_NVALUE_SVALUE, command, Settings.sta_active +1, Settings.sta_ssid[Settings.sta_active]);
    }
    else if ((CMND_SSID == command_code) && (index > 0) && (index <= 2)) {
      if ((data_len > 0) && (data_len < sizeof(Settings.sta_ssid[0]))) {
        strlcpy(Settings.sta_ssid[index -1], (SC_CLEAR == Shortcut(dataBuf)) ? "" : (SC_DEFAULT == Shortcut(dataBuf)) ? (1 == index) ? STA_SSID1 : STA_SSID2 : dataBuf, sizeof(Settings.sta_ssid[0]));
        Settings.sta_active = index -1;
        restart_flag = 2;
      }
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_INDEX_SVALUE, command, index, Settings.sta_ssid[index -1]);
    }
    else if ((CMND_PASSWORD == command_code) && (index > 0) && (index <= 2)) {
      if ((data_len > 0) && (data_len < sizeof(Settings.sta_pwd[0]))) {
        strlcpy(Settings.sta_pwd[index -1], (SC_CLEAR == Shortcut(dataBuf)) ? "" : (SC_DEFAULT == Shortcut(dataBuf)) ? (1 == index) ? STA_PASS1 : STA_PASS2 : dataBuf, sizeof(Settings.sta_pwd[0]));
        Settings.sta_active = index -1;
        restart_flag = 2;
        snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_INDEX_SVALUE, command, index, Settings.sta_pwd[index -1]);
      } else {
        snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_INDEX_ASTERIX, command, index);
      }
    }
    else if ((CMND_HOSTNAME == command_code) && !grpflg) {
      if ((data_len > 0) && (data_len < sizeof(Settings.hostname))) {
        strlcpy(Settings.hostname, (SC_DEFAULT == Shortcut(dataBuf)) ? WIFI_HOSTNAME : dataBuf, sizeof(Settings.hostname));
        if (strstr(Settings.hostname,"%")) {
          strlcpy(Settings.hostname, WIFI_HOSTNAME, sizeof(Settings.hostname));
        }
        restart_flag = 2;
      }
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_SVALUE, command, Settings.hostname);
    }
    else if (CMND_WIFICONFIG == command_code) {
      if ((payload >= WIFI_RESTART) && (payload < MAX_WIFI_OPTION)) {
        Settings.sta_config = payload;
        wifi_state_flag = Settings.sta_config;
        snprintf_P(stemp1, sizeof(stemp1), kWifiConfig[Settings.sta_config]);
        snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"" D_CMND_WIFICONFIG "\":\"%s " D_JSON_SELECTED "\"}"), stemp1);
        if (WifiState() != WIFI_RESTART) {

          restart_flag = 2;
        }
      } else {
        snprintf_P(stemp1, sizeof(stemp1), kWifiConfig[Settings.sta_config]);
        snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_NVALUE_SVALUE, command, Settings.sta_config, stemp1);
      }
    }
    else if ((CMND_FRIENDLYNAME == command_code) && (index > 0) && (index <= MAX_FRIENDLYNAMES)) {
      if ((data_len > 0) && (data_len < sizeof(Settings.friendlyname[0]))) {
        if (1 == index) {
          snprintf_P(stemp1, sizeof(stemp1), PSTR(FRIENDLY_NAME));
        } else {
          snprintf_P(stemp1, sizeof(stemp1), PSTR(FRIENDLY_NAME "%d"), index);
        }
        strlcpy(Settings.friendlyname[index -1], (SC_DEFAULT == Shortcut(dataBuf)) ? stemp1 : dataBuf, sizeof(Settings.friendlyname[index -1]));
      }
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_INDEX_SVALUE, command, index, Settings.friendlyname[index -1]);
    }
    else if ((CMND_SWITCHMODE == command_code) && (index > 0) && (index <= MAX_SWITCHES)) {
      if ((payload >= 0) && (payload < MAX_SWITCH_OPTION)) {
        Settings.switchmode[index -1] = payload;
        GpioSwitchPinMode(index -1);
      }
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_INDEX_NVALUE, command, index, Settings.switchmode[index-1]);
    }
    else if (CMND_TELEPERIOD == command_code) {
      if ((payload >= 0) && (payload < 3601)) {
        Settings.tele_period = (1 == payload) ? TELE_PERIOD : payload;
        if ((Settings.tele_period > 0) && (Settings.tele_period < 10)) Settings.tele_period = 10;
        tele_period = Settings.tele_period;
      }
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_NVALUE_UNIT, command, Settings.tele_period, (Settings.flag.value_units) ? " " D_UNIT_SECOND : "");
    }
    else if (CMND_RESET == command_code) {
      switch (payload) {
      case 1:
        restart_flag = 211;
        snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_SVALUE, command , D_JSON_RESET_AND_RESTARTING);
        break;
      case 2:
      case 3:
      case 4:
      case 5:
        restart_flag = 210 + payload;
        snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"" D_CMND_RESET "\":\"" D_JSON_ERASE ", " D_JSON_RESET_AND_RESTARTING "\"}"));
        break;
      default:
        snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_SVALUE, command, D_JSON_ONE_TO_RESET);
      }
    }
    else if (CMND_TIMEZONE == command_code) {
      if ((data_len > 0) && (((payload >= -13) && (payload <= 14)) || (99 == payload))) {
        Settings.timezone = payload;
      }
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_NVALUE, command, Settings.timezone);
    }
    else if ((CMND_TIMESTD == command_code) || (CMND_TIMEDST == command_code)) {

      uint8_t ts = 0;
      if (CMND_TIMEDST == command_code) { ts = 1; }
      if (data_len > 0) {
        if (strstr(dataBuf, ",")) {
          uint8_t tpos = 0;
          int value = 0;
          p = dataBuf;
          char *q = p;
          while (p && (tpos < 7)) {
            if (p > q) {
              if (1 == tpos) { Settings.tflag[ts].hemis = value &1; }
              if (2 == tpos) { Settings.tflag[ts].week = (value < 0) ? 0 : (value > 4) ? 4 : value; }
              if (3 == tpos) { Settings.tflag[ts].month = (value < 1) ? 1 : (value > 12) ? 12 : value; }
              if (4 == tpos) { Settings.tflag[ts].dow = (value < 1) ? 1 : (value > 7) ? 7 : value; }
              if (5 == tpos) { Settings.tflag[ts].hour = (value < 0) ? 0 : (value > 23) ? 23 : value; }
              if (6 == tpos) { Settings.toffset[ts] = (value < -900) ? -900 : (value > 900) ? 900 : value; }
            }
            p = LTrim(p);
            if (tpos && (*p == ',')) { p++; }
            p = LTrim(p);
            q = p;
            value = strtol(p, &p, 10);
            tpos++;
          }
          ntp_force_sync = 1;
        } else {
          if (0 == payload) {
            if (0 == ts) {
              SettingsResetStd();
            } else {
              SettingsResetDst();
            }
          }
          ntp_force_sync = 1;
        }
      }
      snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"%s\":{\"Hemisphere\":%d,\"Week\":%d,\"Month\":%d,\"Day\":%d,\"Hour\":%d,\"Offset\":%d}}"),
        command, Settings.tflag[ts].hemis, Settings.tflag[ts].week, Settings.tflag[ts].month, Settings.tflag[ts].dow, Settings.tflag[ts].hour, Settings.toffset[ts]);
    }
    else if (CMND_ALTITUDE == command_code) {
      if ((data_len > 0) && ((payload >= -30000) && (payload <= 30000))) {
        Settings.altitude = payload;
      }
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_NVALUE, command, Settings.altitude);
    }
    else if (CMND_LEDPOWER == command_code) {
      if ((payload >= 0) && (payload <= 2)) {
        Settings.ledstate &= 8;
        switch (payload) {
        case 0:
        case 1:
          Settings.ledstate = payload << 3;
          break;
        case 2:
          Settings.ledstate ^= 8;
          break;
        }
        blinks = 0;
        SetLedPower(Settings.ledstate &8);
      }
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_SVALUE, command, GetStateText(bitRead(Settings.ledstate, 3)));
    }
    else if (CMND_LEDSTATE == command_code) {
      if ((payload >= 0) && (payload < MAX_LED_OPTION)) {
        Settings.ledstate = payload;
        if (!Settings.ledstate) SetLedPower(0);
      }
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_NVALUE, command, Settings.ledstate);
    }
#ifdef USE_I2C
    else if ((CMND_I2CSCAN == command_code) && i2c_flg) {
      I2cScan(mqtt_data, sizeof(mqtt_data));
    }
#endif
    else type = NULL;
  }
  if (type == NULL) {
    blinks = 201;
    snprintf_P(topicBuf, sizeof(topicBuf), PSTR(D_JSON_COMMAND));
    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"" D_JSON_COMMAND "\":\"" D_JSON_UNKNOWN "\"}"));
    type = (char*)topicBuf;
  }
  if (mqtt_data[0] != '\0') MqttPublishPrefixTopic_P(RESULT_OR_STAT, type);
  fallback_topic_flag = 0;
}



boolean SendKey(byte key, byte device, byte state)
{
# 1271 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/sonoff.ino"
  char stopic[TOPSZ];
  char scommand[CMDSZ];
  char key_topic[sizeof(Settings.button_topic)];
  boolean result = false;

  char *tmp = (key) ? Settings.switch_topic : Settings.button_topic;
  Format(key_topic, tmp, sizeof(key_topic));
  if (Settings.flag.mqtt_enabled && MqttIsConnected() && (strlen(key_topic) != 0) && strcmp(key_topic, "0")) {
    if (!key && (device > devices_present)) device = 1;
    GetTopic_P(stopic, CMND, key_topic, GetPowerDevice(scommand, device, sizeof(scommand), key));
    if (9 == state) {
      mqtt_data[0] = '\0';
    } else {
      if ((!strcmp(mqtt_topic, key_topic) || !strcmp(Settings.mqtt_grptopic, key_topic)) && (2 == state)) {
        state = ~(power >> (device -1)) &1;
      }
      snprintf_P(mqtt_data, sizeof(mqtt_data), GetStateText(state));
    }
#ifdef USE_DOMOTICZ
    if (!(DomoticzSendKey(key, device, state, strlen(mqtt_data)))) {
      MqttPublishDirect(stopic, (key) ? Settings.flag.mqtt_switch_retain : Settings.flag.mqtt_button_retain);
    }
#else
    MqttPublishDirect(stopic, (key) ? Settings.flag.mqtt_switch_retain : Settings.flag.mqtt_button_retain);
#endif
    result = true;
  } else {
    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"%s%d\":{\"State\":%d}}"), (key) ? "Switch" : "Button", device, state);
    result = XdrvRulesProcess();
  }
#ifdef USE_KNX
  KnxSendButtonPower(key, device, state);
#endif
  return result;
}

void ExecuteCommandPower(byte device, byte state, int source)
{
# 1321 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/sonoff.ino"
  if (SONOFF_IFAN02 == Settings.module) {
    blink_mask &= 1;
    Settings.flag.interlock = 0;
    Settings.pulse_timer[1] = 0;
    Settings.pulse_timer[2] = 0;
    Settings.pulse_timer[3] = 0;
  }

  uint8_t publish_power = 1;
  if ((POWER_OFF_NO_STATE == state) || (POWER_ON_NO_STATE == state)) {
    state &= 1;
    publish_power = 0;
  }
  if ((device < 1) || (device > devices_present)) device = 1;
  if (device <= MAX_PULSETIMERS) { SetPulseTimer(device -1, 0); }
  power_t mask = 1 << (device -1);
  if (state <= POWER_TOGGLE) {
    if ((blink_mask & mask)) {
      blink_mask &= (POWER_MASK ^ mask);
      MqttPublishPowerBlinkState(device);
    }
    if (Settings.flag.interlock && !interlock_mutex) {
      interlock_mutex = 1;
      for (byte i = 0; i < devices_present; i++) {
        power_t imask = 1 << i;
        if ((power & imask) && (mask != imask)) ExecuteCommandPower(i +1, POWER_OFF, SRC_IGNORE);
      }
      interlock_mutex = 0;
    }
    switch (state) {
    case POWER_OFF: {
      power &= (POWER_MASK ^ mask);
      break; }
    case POWER_ON:
      power |= mask;
      break;
    case POWER_TOGGLE:
      power ^= mask;
    }
    SetDevicePower(power, source);
#ifdef USE_DOMOTICZ
    DomoticzUpdatePowerState(device);
#endif
#ifdef USE_KNX
    KnxUpdatePowerState(device, power);
#endif
    if (device <= MAX_PULSETIMERS) {
      SetPulseTimer(device -1, (((POWER_ALL_OFF_PULSETIME_ON == Settings.poweronstate) ? ~power : power) & mask) ? Settings.pulse_timer[device -1] : 0);
    }
  }
  else if (POWER_BLINK == state) {
    if (!(blink_mask & mask)) {
      blink_powersave = (blink_powersave & (POWER_MASK ^ mask)) | (power & mask);
      blink_power = (power >> (device -1))&1;
    }
    blink_timer = millis() + 100;
    blink_counter = ((!Settings.blinkcount) ? 64000 : (Settings.blinkcount *2)) +1;
    blink_mask |= mask;
    MqttPublishPowerBlinkState(device);
    return;
  }
  else if (POWER_BLINK_STOP == state) {
    byte flag = (blink_mask & mask);
    blink_mask &= (POWER_MASK ^ mask);
    MqttPublishPowerBlinkState(device);
    if (flag) ExecuteCommandPower(device, (blink_powersave >> (device -1))&1, SRC_IGNORE);
    return;
  }
  if (publish_power) MqttPublishPowerState(device);
}

void StopAllPowerBlink()
{
  power_t mask;

  for (byte i = 1; i <= devices_present; i++) {
    mask = 1 << (i -1);
    if (blink_mask & mask) {
      blink_mask &= (POWER_MASK ^ mask);
      MqttPublishPowerBlinkState(i);
      ExecuteCommandPower(i, (blink_powersave >> (i -1))&1, SRC_IGNORE);
    }
  }
}

void ExecuteCommand(char *cmnd, int source)
{
  char stopic[CMDSZ];
  char svalue[INPUT_BUFFER_SIZE];
  char *start;
  char *token;

  ShowFreeMem(PSTR("ExecuteCommand"));
  ShowSource(source);

  token = strtok(cmnd, " ");
  if (token != NULL) {
    start = strrchr(token, '/');
    if (start) token = start +1;
  }
  snprintf_P(stopic, sizeof(stopic), PSTR("/%s"), (token == NULL) ? "" : token);
  token = strtok(NULL, "");

  strlcpy(svalue, (token == NULL) ? "" : token, sizeof(svalue));
  MqttDataHandler(stopic, (byte*)svalue, strlen(svalue));
}

void PublishStatus(uint8_t payload)
{
  uint8_t option = STAT;
  char stemp[MAX_FRIENDLYNAMES * (sizeof(Settings.friendlyname[0]) +MAX_FRIENDLYNAMES)];
  char stemp2[MAX_SWITCHES * 3];


  if (!strcmp(Settings.mqtt_prefix[0],Settings.mqtt_prefix[1]) && (!payload)) option++;

  if ((!Settings.flag.mqtt_enabled) && (6 == payload)) payload = 99;
  if (!energy_flg && (9 == payload)) payload = 99;

  if ((0 == payload) || (99 == payload)) {
    uint8_t maxfn = (devices_present > MAX_FRIENDLYNAMES) ? MAX_FRIENDLYNAMES : (!devices_present) ? 1 : devices_present;
    if (SONOFF_IFAN02 == Settings.module) { maxfn = 1; }
    stemp[0] = '\0';
    for (byte i = 0; i < maxfn; i++) {
      snprintf_P(stemp, sizeof(stemp), PSTR("%s%s\"%s\"" ), stemp, (i > 0 ? "," : ""), Settings.friendlyname[i]);
    }
    stemp2[0] = '\0';
    for (byte i = 0; i < MAX_SWITCHES; i++) {
      snprintf_P(stemp2, sizeof(stemp2), PSTR("%s%s%d" ), stemp2, (i > 0 ? "," : ""), Settings.switchmode[i]);
    }
    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"" D_CMND_STATUS "\":{\"" D_CMND_MODULE "\":%d,\"" D_CMND_FRIENDLYNAME "\":[%s],\"" D_CMND_TOPIC "\":\"%s\",\"" D_CMND_BUTTONTOPIC "\":\"%s\",\"" D_CMND_POWER "\":%d,\"" D_CMND_POWERONSTATE "\":%d,\"" D_CMND_LEDSTATE "\":%d,\"" D_CMND_SAVEDATA "\":%d,\"" D_JSON_SAVESTATE "\":%d,\"" D_CMND_SWITCHTOPIC "\":\"%s\",\"" D_CMND_SWITCHMODE "\":[%s],\"" D_CMND_BUTTONRETAIN "\":%d,\"" D_CMND_SWITCHRETAIN "\":%d,\"" D_CMND_SENSORRETAIN "\":%d,\"" D_CMND_POWERRETAIN "\":%d}}"),
      Settings.module +1, stemp, mqtt_topic, Settings.button_topic, power, Settings.poweronstate, Settings.ledstate, Settings.save_data, Settings.flag.save_state, Settings.switch_topic, stemp2, Settings.flag.mqtt_button_retain, Settings.flag.mqtt_switch_retain, Settings.flag.mqtt_sensor_retain, Settings.flag.mqtt_power_retain);
    MqttPublishPrefixTopic_P(option, PSTR(D_CMND_STATUS));
  }

  if ((0 == payload) || (1 == payload)) {
    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"" D_CMND_STATUS D_STATUS1_PARAMETER "\":{\"" D_JSON_BAUDRATE "\":%d,\"" D_CMND_GROUPTOPIC "\":\"%s\",\"" D_CMND_OTAURL "\":\"%s\",\"" D_JSON_RESTARTREASON "\":\"%s\",\"" D_JSON_UPTIME "\":\"%s\",\"" D_JSON_STARTUPUTC "\":\"%s\",\"" D_CMND_SLEEP "\":%d,\"" D_JSON_BOOTCOUNT "\":%d,\"" D_JSON_SAVECOUNT "\":%d,\"" D_JSON_SAVEADDRESS "\":\"%X\"}}"),
      baudrate, Settings.mqtt_grptopic, Settings.ota_url, GetResetReason().c_str(), GetUptime().c_str(), GetDateAndTime(DT_RESTART).c_str(), Settings.sleep, Settings.bootcount, Settings.save_flag, GetSettingsAddress());
    MqttPublishPrefixTopic_P(option, PSTR(D_CMND_STATUS "1"));
  }

  if ((0 == payload) || (2 == payload)) {
    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"" D_CMND_STATUS D_STATUS2_FIRMWARE "\":{\"" D_JSON_VERSION "\":\"%s\",\"" D_JSON_BUILDDATETIME "\":\"%s\",\"" D_JSON_BOOTVERSION "\":%d,\"" D_JSON_COREVERSION "\":\"" ARDUINO_ESP8266_RELEASE "\",\"" D_JSON_SDKVERSION "\":\"%s\"}}"),
      my_version, GetBuildDateAndTime().c_str(), ESP.getBootVersion(), ESP.getSdkVersion());
    MqttPublishPrefixTopic_P(option, PSTR(D_CMND_STATUS "2"));
  }

  if ((0 == payload) || (3 == payload)) {
    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"" D_CMND_STATUS D_STATUS3_LOGGING "\":{\"" D_CMND_SERIALLOG "\":%d,\"" D_CMND_WEBLOG "\":%d,\"" D_CMND_SYSLOG "\":%d,\"" D_CMND_LOGHOST "\":\"%s\",\"" D_CMND_LOGPORT "\":%d,\"" D_CMND_SSID "\":[\"%s\",\"%s\"],\"" D_CMND_TELEPERIOD "\":%d,\"" D_CMND_SETOPTION "\":[\"%08X\",\"%08X\",\"%08X\"]}}"),
      Settings.seriallog_level, Settings.weblog_level, Settings.syslog_level, Settings.syslog_host, Settings.syslog_port, Settings.sta_ssid[0], Settings.sta_ssid[1], Settings.tele_period, Settings.flag.data, Settings.flag2.data, Settings.flag3.data);
    MqttPublishPrefixTopic_P(option, PSTR(D_CMND_STATUS "3"));
  }

  if ((0 == payload) || (4 == payload)) {
    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"" D_CMND_STATUS D_STATUS4_MEMORY "\":{\"" D_JSON_PROGRAMSIZE "\":%d,\"" D_JSON_FREEMEMORY "\":%d,\"" D_JSON_HEAPSIZE "\":%d,\"" D_JSON_PROGRAMFLASHSIZE "\":%d,\"" D_JSON_FLASHSIZE "\":%d,\"" D_JSON_FLASHMODE "\":%d,\"" D_JSON_FEATURES "\":[\"%08X\",\"%08X\",\"%08X\",\"%08X\",\"%08X\"]}}"),
      ESP.getSketchSize()/1024, ESP.getFreeSketchSpace()/1024, ESP.getFreeHeap()/1024, ESP.getFlashChipSize()/1024, ESP.getFlashChipRealSize()/1024, ESP.getFlashChipMode(), LANGUAGE_LCID, feature_drv1, feature_drv2, feature_sns1, feature_sns2);
    MqttPublishPrefixTopic_P(option, PSTR(D_CMND_STATUS "4"));
  }

  if ((0 == payload) || (5 == payload)) {
    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"" D_CMND_STATUS D_STATUS5_NETWORK "\":{\"" D_CMND_HOSTNAME "\":\"%s\",\"" D_CMND_IPADDRESS "\":\"%s\",\"" D_JSON_GATEWAY "\":\"%s\",\"" D_JSON_SUBNETMASK "\":\"%s\",\"" D_JSON_DNSSERVER "\":\"%s\",\"" D_JSON_MAC "\":\"%s\",\"" D_CMND_WEBSERVER "\":%d,\"" D_CMND_WIFICONFIG "\":%d}}"),
      my_hostname, WiFi.localIP().toString().c_str(), IPAddress(Settings.ip_address[1]).toString().c_str(), IPAddress(Settings.ip_address[2]).toString().c_str(), IPAddress(Settings.ip_address[3]).toString().c_str(),
      WiFi.macAddress().c_str(), Settings.webserver, Settings.sta_config);
    MqttPublishPrefixTopic_P(option, PSTR(D_CMND_STATUS "5"));
  }

  if (((0 == payload) || (6 == payload)) && Settings.flag.mqtt_enabled) {
    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"" D_CMND_STATUS D_STATUS6_MQTT "\":{\"" D_CMND_MQTTHOST "\":\"%s\",\"" D_CMND_MQTTPORT "\":%d,\"" D_CMND_MQTTCLIENT D_JSON_MASK "\":\"%s\",\"" D_CMND_MQTTCLIENT "\":\"%s\",\"" D_CMND_MQTTUSER "\":\"%s\",\"MqttType\":%d,\"MAX_PACKET_SIZE\":%d,\"KEEPALIVE\":%d}}"),
      Settings.mqtt_host, Settings.mqtt_port, Settings.mqtt_client, mqtt_client, Settings.mqtt_user, MqttLibraryType(), MQTT_MAX_PACKET_SIZE, MQTT_KEEPALIVE);
    MqttPublishPrefixTopic_P(option, PSTR(D_CMND_STATUS "6"));
  }

  if ((0 == payload) || (7 == payload)) {
#if defined(USE_TIMERS) && defined(USE_SUNRISE)
    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"" D_CMND_STATUS D_STATUS7_TIME "\":{\"" D_JSON_UTC_TIME "\":\"%s\",\"" D_JSON_LOCAL_TIME "\":\"%s\",\"" D_JSON_STARTDST "\":\"%s\",\"" D_JSON_ENDDST "\":\"%s\",\"" D_CMND_TIMEZONE "\":%d,\"" D_JSON_SUNRISE "\":\"%s\",\"" D_JSON_SUNSET "\":\"%s\"}}"),
      GetTime(0).c_str(), GetTime(1).c_str(), GetTime(2).c_str(), GetTime(3).c_str(), Settings.timezone, GetSun(0).c_str(), GetSun(1).c_str());
#else
    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"" D_CMND_STATUS D_STATUS7_TIME "\":{\"" D_JSON_UTC_TIME "\":\"%s\",\"" D_JSON_LOCAL_TIME "\":\"%s\",\"" D_JSON_STARTDST "\":\"%s\",\"" D_JSON_ENDDST "\":\"%s\",\"" D_CMND_TIMEZONE "\":%d}}"),
      GetTime(0).c_str(), GetTime(1).c_str(), GetTime(2).c_str(), GetTime(3).c_str(), Settings.timezone);
#endif
    MqttPublishPrefixTopic_P(option, PSTR(D_CMND_STATUS "7"));
  }

  if (energy_flg) {
    if ((0 == payload) || (9 == payload)) {
      snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"" D_CMND_STATUS D_STATUS9_MARGIN "\":{\"" D_CMND_POWERDELTA "\":%d,\"" D_CMND_POWERLOW "\":%d,\"" D_CMND_POWERHIGH "\":%d,\"" D_CMND_VOLTAGELOW "\":%d,\"" D_CMND_VOLTAGEHIGH "\":%d,\"" D_CMND_CURRENTLOW "\":%d,\"" D_CMND_CURRENTHIGH "\":%d}}"),
        Settings.energy_power_delta, Settings.energy_min_power, Settings.energy_max_power, Settings.energy_min_voltage, Settings.energy_max_voltage, Settings.energy_min_current, Settings.energy_max_current);
      MqttPublishPrefixTopic_P(option, PSTR(D_CMND_STATUS "9"));
    }
  }

  if ((0 == payload) || (8 == payload) || (10 == payload)) {
    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"" D_CMND_STATUS D_STATUS10_SENSOR "\":"));
    MqttShowSensor();
    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s}"), mqtt_data);
    if (8 == payload) {
      MqttPublishPrefixTopic_P(option, PSTR(D_CMND_STATUS "8"));
    } else {
      MqttPublishPrefixTopic_P(option, PSTR(D_CMND_STATUS "10"));
    }
  }

  if ((0 == payload) || (11 == payload)) {
    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"" D_CMND_STATUS D_STATUS11_STATUS "\":"));
    MqttShowState();
    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s}"), mqtt_data);
    MqttPublishPrefixTopic_P(option, PSTR(D_CMND_STATUS "11"));
  }

}

void MqttShowPWMState()
{
  snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s\"" D_CMND_PWM "\":{"), mqtt_data);
  bool first = true;
  for (byte i = 0; i < MAX_PWMS; i++) {
    if (pin[GPIO_PWM1 + i] < 99) {
      snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s%s\"" D_CMND_PWM "%d\":%d"), mqtt_data, first ? "" : ",", i+1, Settings.pwm_value[i]);
      first = false;
    }
  }
  snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s}"), mqtt_data);
}

void MqttShowState()
{
  char stemp1[33];

  snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s{\"" D_JSON_TIME "\":\"%s\",\"" D_JSON_UPTIME "\":\"%s\""), mqtt_data, GetDateAndTime(DT_LOCAL).c_str(), GetUptime().c_str());
#ifdef USE_ADC_VCC
  dtostrfd((double)ESP.getVcc()/1000, 3, stemp1);
  snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s,\"" D_JSON_VCC "\":%s"), mqtt_data, stemp1);
#endif

  for (byte i = 0; i < devices_present; i++) {
    if (i == light_device -1) {
      LightState(1);
    } else {
      snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s,\"%s\":\"%s\""), mqtt_data, GetPowerDevice(stemp1, i +1, sizeof(stemp1), Settings.flag.device_index_enable), GetStateText(bitRead(power, i)));
      if (SONOFF_IFAN02 == Settings.module) {
        snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s,\"" D_CMND_FANSPEED "\":%d"), mqtt_data, GetFanspeed());
        break;
      }
    }
  }

  if (pwm_present) {
    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s,"), mqtt_data);
    MqttShowPWMState();
  }

  snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s,\"" D_JSON_WIFI "\":{\"" D_JSON_AP "\":%d,\"" D_JSON_SSID "\":\"%s\",\"" D_JSON_BSSID "\":\"%s\",\"" D_JSON_CHANNEL "\":%d,\"" D_JSON_RSSI "\":%d}}"),
    mqtt_data, Settings.sta_active +1, Settings.sta_ssid[Settings.sta_active], WiFi.BSSIDstr().c_str(), WiFi.channel(), WifiGetRssiAsQuality(WiFi.RSSI()));
}

boolean MqttShowSensor()
{
  snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s{\"" D_JSON_TIME "\":\"%s\""), mqtt_data, GetDateAndTime(DT_LOCAL).c_str());
  int json_data_start = strlen(mqtt_data);
  for (byte i = 0; i < MAX_SWITCHES; i++) {
#ifdef USE_TM1638
    if ((pin[GPIO_SWT1 +i] < 99) || ((pin[GPIO_TM16CLK] < 99) && (pin[GPIO_TM16DIO] < 99) && (pin[GPIO_TM16STB] < 99))) {
#else
    if (pin[GPIO_SWT1 +i] < 99) {
#endif
      boolean swm = ((FOLLOW_INV == Settings.switchmode[i]) || (PUSHBUTTON_INV == Settings.switchmode[i]) || (PUSHBUTTONHOLD_INV == Settings.switchmode[i]));
      snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s,\"" D_JSON_SWITCH "%d\":\"%s\""), mqtt_data, i +1, GetStateText(swm ^ lastwallswitch[i]));
    }
  }
  XsnsCall(FUNC_JSON_APPEND);
  boolean json_data_available = (strlen(mqtt_data) - json_data_start);
  if (strstr_P(mqtt_data, PSTR(D_JSON_TEMPERATURE))) {
    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s,\"" D_JSON_TEMPERATURE_UNIT "\":\"%c\""), mqtt_data, TempUnit());
  }
  snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s}"), mqtt_data);

  if (json_data_available) XdrvCall(FUNC_SHOW_SENSOR);
  return json_data_available;
}



void PerformEverySecond()
{
  uptime++;

  if (BOOT_LOOP_TIME == uptime) {
    RtcReboot.fast_reboot_count = 0;
    RtcRebootSave();
  }

  if ((4 == uptime) && (SONOFF_IFAN02 == Settings.module)) {
    SetDevicePower(1, SRC_RETRY);
    SetDevicePower(power, SRC_RETRY);
  }

  if (seriallog_timer) {
    seriallog_timer--;
    if (!seriallog_timer) {
      if (seriallog_level) {
        AddLog_P(LOG_LEVEL_INFO, PSTR(D_LOG_APPLICATION D_SERIAL_LOGGING_DISABLED));
      }
      seriallog_level = 0;
    }
  }

  if (syslog_timer) {
    syslog_timer--;
    if (!syslog_timer) {
      syslog_level = Settings.syslog_level;
      if (Settings.syslog_level) {
        AddLog_P(LOG_LEVEL_INFO, PSTR(D_LOG_APPLICATION D_SYSLOG_LOGGING_REENABLED));
      }
    }
  }

  ResetGlobalValues();

  if (Settings.tele_period) {
    tele_period++;
    if (tele_period == Settings.tele_period -1) {
      XsnsCall(FUNC_PREP_BEFORE_TELEPERIOD);
    }
    if (tele_period >= Settings.tele_period) {
      tele_period = 0;

      mqtt_data[0] = '\0';
      MqttShowState();
      MqttPublishPrefixTopic_P(TELE, PSTR(D_RSLT_STATE), MQTT_TELE_RETAIN);

      mqtt_data[0] = '\0';
      if (MqttShowSensor()) {
        MqttPublishPrefixTopic_P(TELE, PSTR(D_RSLT_SENSOR), Settings.flag.mqtt_sensor_retain);
#ifdef USE_RULES
        RulesTeleperiod();
#endif
      }
    }
  }

  XdrvCall(FUNC_EVERY_SECOND);
  XsnsCall(FUNC_EVERY_SECOND);

  if ((2 == RtcTime.minute) && latest_uptime_flag) {
    latest_uptime_flag = false;
    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"" D_JSON_TIME "\":\"%s\",\"" D_JSON_UPTIME "\":\"%s\"}"), GetDateAndTime(DT_LOCAL).c_str(), GetUptime().c_str());
    MqttPublishPrefixTopic_P(TELE, PSTR(D_RSLT_UPTIME));
  }
  if ((3 == RtcTime.minute) && !latest_uptime_flag) latest_uptime_flag = true;
}





void ButtonHandler()
{
  uint8_t button = NOT_PRESSED;
  uint8_t button_present = 0;
  uint8_t hold_time_extent = IMMINENT_RESET_FACTOR;
  uint16_t loops_per_second = 1000 / Settings.button_debounce;
  char scmnd[20];

  uint8_t maxdev = (devices_present > MAX_KEYS) ? MAX_KEYS : devices_present;
  for (byte button_index = 0; button_index < maxdev; button_index++) {
    button = NOT_PRESSED;
    button_present = 0;

    if (!button_index && ((SONOFF_DUAL == Settings.module) || (CH4 == Settings.module))) {
      button_present = 1;
      if (dual_button_code) {
        snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_APPLICATION D_BUTTON " " D_CODE " %04X"), dual_button_code);
        AddLog(LOG_LEVEL_DEBUG);
        button = PRESSED;
        if (0xF500 == dual_button_code) {
          holdbutton[button_index] = (loops_per_second * Settings.param[P_HOLD_TIME] / 10) -1;
          hold_time_extent = 1;
        }
        dual_button_code = 0;
      }
    } else {
      if (pin[GPIO_KEY1 +button_index] < 99) {
        if (!((uptime < 4) && (0 == pin[GPIO_KEY1 +button_index]))) {
          button_present = 1;
          button = digitalRead(pin[GPIO_KEY1 +button_index]);
        }
      }
    }

    if (button_present) {
      XdrvMailbox.index = button_index;
      XdrvMailbox.payload = button;
      if (XdrvCall(FUNC_BUTTON_PRESSED)) {

      }
      else if (SONOFF_4CHPRO == Settings.module) {
        if (holdbutton[button_index]) { holdbutton[button_index]--; }

        boolean button_pressed = false;
        if ((PRESSED == button) && (NOT_PRESSED == lastbutton[button_index])) {
          snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_APPLICATION D_BUTTON "%d " D_LEVEL_10), button_index +1);
          AddLog(LOG_LEVEL_DEBUG);
          holdbutton[button_index] = loops_per_second;
          button_pressed = true;
        }
        if ((NOT_PRESSED == button) && (PRESSED == lastbutton[button_index])) {
          snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_APPLICATION D_BUTTON "%d " D_LEVEL_01), button_index +1);
          AddLog(LOG_LEVEL_DEBUG);
          if (!holdbutton[button_index]) { button_pressed = true; }
        }
        if (button_pressed) {
          if (!SendKey(0, button_index +1, POWER_TOGGLE)) {
            ExecuteCommandPower(button_index +1, POWER_TOGGLE, SRC_BUTTON);
          }
        }
      }
      else {
        if ((PRESSED == button) && (NOT_PRESSED == lastbutton[button_index])) {
          if (Settings.flag.button_single) {
            snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_APPLICATION D_BUTTON "%d " D_IMMEDIATE), button_index +1);
            AddLog(LOG_LEVEL_DEBUG);
            if (!SendKey(0, button_index +1, POWER_TOGGLE)) {
              ExecuteCommandPower(button_index +1, POWER_TOGGLE, SRC_BUTTON);
            }
          } else {
            multipress[button_index] = (multiwindow[button_index]) ? multipress[button_index] +1 : 1;
            snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_APPLICATION D_BUTTON "%d " D_MULTI_PRESS " %d"), button_index +1, multipress[button_index]);
            AddLog(LOG_LEVEL_DEBUG);
            multiwindow[button_index] = loops_per_second / 2;
          }
          blinks = 201;
        }

        if (NOT_PRESSED == button) {
          holdbutton[button_index] = 0;
        } else {
          holdbutton[button_index]++;
          if (Settings.flag.button_single) {
            if (holdbutton[button_index] == loops_per_second * hold_time_extent * Settings.param[P_HOLD_TIME] / 10) {

              snprintf_P(scmnd, sizeof(scmnd), PSTR(D_CMND_SETOPTION "13 0"));
              ExecuteCommand(scmnd, SRC_BUTTON);
            }
          } else {
            if (Settings.flag.button_restrict) {
              if (holdbutton[button_index] == loops_per_second * Settings.param[P_HOLD_TIME] / 10) {
                multipress[button_index] = 0;
                SendKey(0, button_index +1, 3);
              }
            } else {
              if (holdbutton[button_index] == loops_per_second * hold_time_extent * Settings.param[P_HOLD_TIME] / 10) {
                multipress[button_index] = 0;
                snprintf_P(scmnd, sizeof(scmnd), PSTR(D_CMND_RESET " 1"));
                ExecuteCommand(scmnd, SRC_BUTTON);
              }
            }
          }
        }

        if (!Settings.flag.button_single) {
          if (multiwindow[button_index]) {
            multiwindow[button_index]--;
          } else {
            if (!restart_flag && !holdbutton[button_index] && (multipress[button_index] > 0) && (multipress[button_index] < MAX_BUTTON_COMMANDS +3)) {
              boolean single_press = false;
              if (multipress[button_index] < 3) {
                if ((SONOFF_DUAL_R2 == Settings.module) || (SONOFF_DUAL == Settings.module) || (CH4 == Settings.module)) {
                  single_press = true;
                } else {
                  single_press = (Settings.flag.button_swap +1 == multipress[button_index]);
                  multipress[button_index] = 1;
                }
              }
              if (single_press && SendKey(0, button_index + multipress[button_index], POWER_TOGGLE)) {

              } else {
                if (multipress[button_index] < 3) {
                  if (WifiState()) {
                    restart_flag = 1;
                  } else {
                    ExecuteCommandPower(button_index + multipress[button_index], POWER_TOGGLE, SRC_BUTTON);
                  }
                } else {
                  if (!Settings.flag.button_restrict) {
                    snprintf_P(scmnd, sizeof(scmnd), kCommands[multipress[button_index] -3]);
                    ExecuteCommand(scmnd, SRC_BUTTON);
                  }
                }
              }
              multipress[button_index] = 0;
            }
          }
        }
      }
    }
    lastbutton[button_index] = button;
  }
}





void SwitchHandler(byte mode)
{
  uint8_t button = NOT_PRESSED;
  uint8_t switchflag;
  uint16_t loops_per_second = 1000 / Settings.switch_debounce;

  for (byte i = 0; i < MAX_SWITCHES; i++) {
    if ((pin[GPIO_SWT1 +i] < 99) || (mode)) {

      if (holdwallswitch[i]) {
        holdwallswitch[i]--;
        if (0 == holdwallswitch[i]) {
          SendKey(1, i +1, 3);
        }
      }

      if (mode) {
        button = virtualswitch[i];
      } else {
        if (!((uptime < 4) && (0 == pin[GPIO_SWT1 +i]))) {
          button = digitalRead(pin[GPIO_SWT1 +i]);
        }
      }

      if (button != lastwallswitch[i]) {
        switchflag = 3;
        switch (Settings.switchmode[i]) {
        case TOGGLE:
          switchflag = 2;
          break;
        case FOLLOW:
          switchflag = button &1;
          break;
        case FOLLOW_INV:
          switchflag = ~button &1;
          break;
        case PUSHBUTTON:
          if ((PRESSED == button) && (NOT_PRESSED == lastwallswitch[i])) {
            switchflag = 2;
          }
          break;
        case PUSHBUTTON_INV:
          if ((NOT_PRESSED == button) && (PRESSED == lastwallswitch[i])) {
            switchflag = 2;
          }
          break;
        case PUSHBUTTON_TOGGLE:
          if (button != lastwallswitch[i]) {
            switchflag = 2;
          }
          break;
        case PUSHBUTTONHOLD:
          if ((PRESSED == button) && (NOT_PRESSED == lastwallswitch[i])) {
            holdwallswitch[i] = loops_per_second * Settings.param[P_HOLD_TIME] / 10;
          }
          if ((NOT_PRESSED == button) && (PRESSED == lastwallswitch[i]) && (holdwallswitch[i])) {
            holdwallswitch[i] = 0;
            switchflag = 2;
          }
          break;
        case PUSHBUTTONHOLD_INV:
          if ((NOT_PRESSED == button) && (PRESSED == lastwallswitch[i])) {
            holdwallswitch[i] = loops_per_second * Settings.param[P_HOLD_TIME] / 10;
          }
          if ((PRESSED == button) && (NOT_PRESSED == lastwallswitch[i]) && (holdwallswitch[i])) {
            holdwallswitch[i] = 0;
            switchflag = 2;
          }
          break;
        }

        if (switchflag < 3) {
          if (!SendKey(1, i +1, switchflag)) {
            ExecuteCommandPower(i +1, switchflag, SRC_SWITCH);
          }
        }

        lastwallswitch[i] = button;
      }
    }
  }
}
# 1914 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/sonoff.ino"
void Every100mSeconds()
{

  power_t power_now;

  if (latching_relay_pulse) {
    latching_relay_pulse--;
    if (!latching_relay_pulse) SetLatchingRelay(0, 0);
  }

  for (byte i = 0; i < MAX_PULSETIMERS; i++) {
    if (pulse_timer[i] != 0L) {
      if (TimeReached(pulse_timer[i])) {
        pulse_timer[i] = 0L;
        ExecuteCommandPower(i +1, (POWER_ALL_OFF_PULSETIME_ON == Settings.poweronstate) ? POWER_ON : POWER_OFF, SRC_PULSETIMER);
      }
    }
  }

  if (blink_mask) {
    if (TimeReached(blink_timer)) {
      SetNextTimeInterval(blink_timer, 100 * Settings.blinktime);
      blink_counter--;
      if (!blink_counter) {
        StopAllPowerBlink();
      } else {
        blink_power ^= 1;
        power_now = (power & (POWER_MASK ^ blink_mask)) | ((blink_power) ? blink_mask : 0);
        SetDevicePower(power_now, SRC_IGNORE);
      }
    }
  }


  if (TimeReached(backlog_delay)) {
    if ((backlog_pointer != backlog_index) && !backlog_mutex) {
      backlog_mutex = 1;
      ExecuteCommand((char*)backlog[backlog_pointer].c_str(), SRC_BACKLOG);
      backlog_mutex = 0;
      backlog_pointer++;
      if (backlog_pointer >= MAX_BACKLOG) { backlog_pointer = 0; }
    }
  }
}





void Every250mSeconds()
{


  uint8_t blinkinterval = 1;

  state_250mS++;
  state_250mS &= 0x3;

  if (mqtt_cmnd_publish) mqtt_cmnd_publish--;

  if (!Settings.flag.global_state) {
    if (global_state.data) {
      if (global_state.mqtt_down) { blinkinterval = 7; }
      if (global_state.wifi_down) { blinkinterval = 3; }
      blinks = 201;
    }
  }
  if (blinks || restart_flag || ota_state_flag) {
    if (restart_flag || ota_state_flag) {
      blinkstate = 1;
    } else {
      blinkspeed--;
      if (!blinkspeed) {
        blinkspeed = blinkinterval;
        blinkstate ^= 1;
      }
    }
    if ((!(Settings.ledstate &0x08)) && ((Settings.ledstate &0x06) || (blinks > 200) || (blinkstate))) {

      SetLedPower(blinkstate);
    }
    if (!blinkstate) {
      blinks--;
      if (200 == blinks) blinks = 0;
    }
  }
  else if (Settings.ledstate &1) {
    boolean tstate = power;
    if ((SONOFF_TOUCH == Settings.module) || (SONOFF_T11 == Settings.module) || (SONOFF_T12 == Settings.module) || (SONOFF_T13 == Settings.module)) {
      tstate = (!power) ? 1 : 0;
    }
    SetLedPower(tstate);
  }





  switch (state_250mS) {
  case 0:
    PerformEverySecond();

    if (ota_state_flag && (backlog_pointer == backlog_index)) {
      ota_state_flag--;
      if (2 == ota_state_flag) {
        ota_url = Settings.ota_url;
        RtcSettings.ota_loader = 0;
        ota_retry_counter = OTA_ATTEMPTS;
        ESPhttpUpdate.rebootOnUpdate(false);
        SettingsSave(1);
      }
      if (ota_state_flag <= 0) {
#ifdef USE_WEBSERVER
        if (Settings.webserver) StopWebserver();
#endif
#ifdef USE_ARILUX_RF
        AriluxRfDisable();
#endif
        ota_state_flag = 92;
        ota_result = 0;
        ota_retry_counter--;
        if (ota_retry_counter) {
          strlcpy(mqtt_data, GetOtaUrl(log_data, sizeof(log_data)), sizeof(mqtt_data));
#ifndef BE_MINIMAL
          if (RtcSettings.ota_loader) {
            char *bch = strrchr(mqtt_data, '/');
            char *pch = strrchr((bch != NULL) ? bch : mqtt_data, '-');
            char *ech = strrchr((bch != NULL) ? bch : mqtt_data, '.');
            if (!pch) pch = ech;
            if (pch) {
              mqtt_data[pch - mqtt_data] = '\0';
              char *ech = strrchr(Settings.ota_url, '.');
              snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s-" D_JSON_MINIMAL "%s"), mqtt_data, ech);
            }
          }
#endif
          snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_UPLOAD "%s"), mqtt_data);
          AddLog(LOG_LEVEL_DEBUG);
#if defined(ARDUINO_ESP8266_RELEASE_2_3_0) || defined(ARDUINO_ESP8266_RELEASE_2_4_0) || defined(ARDUINO_ESP8266_RELEASE_2_4_1) || defined(ARDUINO_ESP8266_RELEASE_2_4_2)
          ota_result = (HTTP_UPDATE_FAILED != ESPhttpUpdate.update(mqtt_data));
#else

          WiFiClient OTAclient;
          ota_result = (HTTP_UPDATE_FAILED != ESPhttpUpdate.update(OTAclient, mqtt_data));
#endif
          if (!ota_result) {
#ifndef BE_MINIMAL
            int ota_error = ESPhttpUpdate.getLastError();


            if ((HTTP_UE_TOO_LESS_SPACE == ota_error) || (HTTP_UE_BIN_FOR_WRONG_FLASH == ota_error)) {
              RtcSettings.ota_loader = 1;
            }
#endif
            ota_state_flag = 2;
          }
        }
      }
      if (90 == ota_state_flag) {
        ota_state_flag = 0;
        if (ota_result) {
          SetFlashModeDout();
          snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR(D_JSON_SUCCESSFUL ". " D_JSON_RESTARTING));
        } else {
          snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR(D_JSON_FAILED " %s"), ESPhttpUpdate.getLastErrorString().c_str());
        }
        restart_flag = 2;
        MqttPublishPrefixTopic_P(STAT, PSTR(D_CMND_UPGRADE));
      }
    }
    break;
  case 1:
    if (MidnightNow()) { CounterSaveState(); }
    if (save_data_counter && (backlog_pointer == backlog_index)) {
      save_data_counter--;
      if (save_data_counter <= 0) {
        if (Settings.flag.save_state) {
          power_t mask = POWER_MASK;
          for (byte i = 0; i < MAX_PULSETIMERS; i++) {
            if ((Settings.pulse_timer[i] > 0) && (Settings.pulse_timer[i] < 30)) {
              mask &= ~(1 << i);
            }
          }
          if (!((Settings.power &mask) == (power &mask))) {
            Settings.power = power;
          }
        } else {
          Settings.power = 0;
        }
        SettingsSave(0);
        save_data_counter = Settings.save_data;
      }
    }
    if (restart_flag && (backlog_pointer == backlog_index)) {
      if ((214 == restart_flag) || (215 == restart_flag)) {
        char storage[sizeof(Settings.sta_ssid) + sizeof(Settings.sta_pwd)];
        memcpy(storage, Settings.sta_ssid, sizeof(storage));
        if (215 == restart_flag) {
          SettingsErase(0);
        }
        SettingsDefault();
        memcpy(Settings.sta_ssid, storage, sizeof(storage));
        restart_flag = 2;
      }
      else if (213 == restart_flag) {
        SettingsSdkErase();
        restart_flag = 2;
      }
      else if (212 == restart_flag) {
        SettingsErase(0);
        restart_flag = 211;
      }
      if (211 == restart_flag) {
        SettingsDefault();
        restart_flag = 2;
      }
      SettingsSaveAll();
      restart_flag--;
      if (restart_flag <= 0) {
        AddLog_P(LOG_LEVEL_INFO, PSTR(D_LOG_APPLICATION D_RESTARTING));
        EspRestart();
      }
    }
    break;
  case 2:
    WifiCheck(wifi_state_flag);
    wifi_state_flag = WIFI_RESTART;
    break;
  case 3:
    if (WL_CONNECTED == WiFi.status()) { MqttCheck(); }
    break;
  }
}

#ifdef USE_ARDUINO_OTA







bool arduino_ota_triggered = false;
uint16_t arduino_ota_progress_dot_count = 0;

void ArduinoOTAInit()
{
  ArduinoOTA.setPort(8266);
  ArduinoOTA.setHostname(Settings.hostname);
  if (Settings.web_password[0] !=0) ArduinoOTA.setPassword(Settings.web_password);

  ArduinoOTA.onStart([]()
  {
    SettingsSave(1);
#ifdef USE_WEBSERVER
    if (Settings.webserver) StopWebserver();
#endif
#ifdef USE_ARILUX_RF
    AriluxRfDisable();
#endif
    if (Settings.flag.mqtt_enabled) MqttDisconnect();
    snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_UPLOAD "Arduino OTA " D_UPLOAD_STARTED));
    AddLog(LOG_LEVEL_INFO);
    arduino_ota_triggered = true;
    arduino_ota_progress_dot_count = 0;
    delay(100);
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
  {
    if ((LOG_LEVEL_DEBUG <= seriallog_level)) {
      arduino_ota_progress_dot_count++;
      Serial.printf(".");
      if (!(arduino_ota_progress_dot_count % 80)) Serial.println();
    }
  });

  ArduinoOTA.onError([](ota_error_t error)
  {




    char error_str[100];

    if ((LOG_LEVEL_DEBUG <= seriallog_level) && arduino_ota_progress_dot_count) Serial.println();
    switch (error) {
      case OTA_BEGIN_ERROR: strncpy_P(error_str, PSTR(D_UPLOAD_ERR_2), sizeof(error_str)); break;
      case OTA_RECEIVE_ERROR: strncpy_P(error_str, PSTR(D_UPLOAD_ERR_5), sizeof(error_str)); break;
      case OTA_END_ERROR: strncpy_P(error_str, PSTR(D_UPLOAD_ERR_7), sizeof(error_str)); break;
      default:
        snprintf_P(error_str, sizeof(error_str), PSTR(D_UPLOAD_ERROR_CODE " %d"), error);
    }
    snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_UPLOAD "Arduino OTA  %s. " D_RESTARTING), error_str);
    AddLog(LOG_LEVEL_INFO);
    EspRestart();
  });

  ArduinoOTA.onEnd([]()
  {
    if ((LOG_LEVEL_DEBUG <= seriallog_level)) Serial.println();
    snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_UPLOAD "Arduino OTA " D_SUCCESSFUL ". " D_RESTARTING));
    AddLog(LOG_LEVEL_INFO);
    EspRestart();
 });

  ArduinoOTA.begin();
  snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_UPLOAD "Arduino OTA " D_ENABLED " " D_PORT " 8266"));
  AddLog(LOG_LEVEL_INFO);
}
#endif



void SerialInput()
{
  while (Serial.available()) {
    yield();
    serial_in_byte = Serial.read();




    if ((SONOFF_DUAL == Settings.module) || (CH4 == Settings.module)) {
      if (dual_hex_code) {
        dual_hex_code--;
        if (dual_hex_code) {
          dual_button_code = (dual_button_code << 8) | serial_in_byte;
          serial_in_byte = 0;
        } else {
          if (serial_in_byte != 0xA1) {
            dual_button_code = 0;
          }
        }
      }
      if (0xA0 == serial_in_byte) {
        serial_in_byte = 0;
        dual_button_code = 0;
        dual_hex_code = 3;
      }
    }



    if (XdrvCall(FUNC_SERIAL)) {
      serial_in_byte_counter = 0;
      Serial.flush();
      return;
    }



    if (serial_in_byte > 127 && !Settings.flag.mqtt_serial_raw) {
      serial_in_byte_counter = 0;
      Serial.flush();
      return;
    }
    if (!Settings.flag.mqtt_serial) {
      if (isprint(serial_in_byte)) {
        if (serial_in_byte_counter < INPUT_BUFFER_SIZE -1) {
          serial_in_buffer[serial_in_byte_counter++] = serial_in_byte;
        } else {
          serial_in_byte_counter = 0;
        }
      }
    } else {
      if (serial_in_byte || Settings.flag.mqtt_serial_raw) {
        if ((serial_in_byte_counter < INPUT_BUFFER_SIZE -1) &&
            ((serial_in_byte != Settings.serial_delimiter) || Settings.flag.mqtt_serial_raw)) {
          serial_in_buffer[serial_in_byte_counter++] = serial_in_byte;
          serial_polling_window = millis();
        } else {
          serial_polling_window = 0;
          break;
        }
      }
    }




    if (SONOFF_SC == Settings.module) {
      if (serial_in_byte == '\x1B') {
        serial_in_buffer[serial_in_byte_counter] = 0;
        SonoffScSerialInput(serial_in_buffer);
        serial_in_byte_counter = 0;
        Serial.flush();
        return;
      }
    }



    else if (!Settings.flag.mqtt_serial && (serial_in_byte == '\n')) {
      serial_in_buffer[serial_in_byte_counter] = 0;
      seriallog_level = (Settings.seriallog_level < LOG_LEVEL_INFO) ? (byte)LOG_LEVEL_INFO : Settings.seriallog_level;
      snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_COMMAND "%s"), serial_in_buffer);
      AddLog(LOG_LEVEL_INFO);
      ExecuteCommand(serial_in_buffer, SRC_SERIAL);
      serial_in_byte_counter = 0;
      serial_polling_window = 0;
      Serial.flush();
      return;
    }
  }

  if (Settings.flag.mqtt_serial && serial_in_byte_counter && (millis() > (serial_polling_window + SERIAL_POLLING))) {
    serial_in_buffer[serial_in_byte_counter] = 0;
    if (!Settings.flag.mqtt_serial_raw) {
      snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"" D_JSON_SERIALRECEIVED "\":\"%s\"}"), serial_in_buffer);
    } else {
      snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"" D_JSON_SERIALRECEIVED "\":\""));
      for (int i = 0; i < serial_in_byte_counter; i++) {
        snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s%02x"), mqtt_data, serial_in_buffer[i]);
      }
      snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s\"}"), mqtt_data);
    }
    MqttPublishPrefixTopic_P(RESULT_OR_TELE, PSTR(D_JSON_SERIALRECEIVED));

    serial_in_byte_counter = 0;
  }
}


void GpioSwitchPinMode(uint8_t index)
{
  if (pin[GPIO_SWT1 +index] < 99) {


    uint8_t no_pullup = bitRead(switch_no_pullup, index);
    if (no_pullup) {
      if (SHELLY2 == Settings.module) {
        no_pullup = (Settings.switchmode[index] < PUSHBUTTON);
      }
    }
    pinMode(pin[GPIO_SWT1 +index], (16 == pin[GPIO_SWT1 +index]) ? INPUT_PULLDOWN_16 : (no_pullup) ? INPUT : INPUT_PULLUP);
  }
}

void GpioInit()
{
  uint8_t mpin;
  uint8_t key_no_pullup = 0;
  mytmplt def_module;

  if (!Settings.module || (Settings.module >= MAXMODULE)) {
    Settings.module = MODULE;
    Settings.last_module = MODULE;
  }

  memcpy_P(&def_module, &kModules[Settings.module], sizeof(def_module));
  strlcpy(my_module.name, def_module.name, sizeof(my_module.name));
  for (byte i = 0; i < MAX_GPIO_PIN; i++) {
    if (Settings.my_gp.io[i] > GPIO_NONE) {
      my_module.gp.io[i] = Settings.my_gp.io[i];
    }
    if ((def_module.gp.io[i] > GPIO_NONE) && (def_module.gp.io[i] < GPIO_USER)) {
      my_module.gp.io[i] = def_module.gp.io[i];
    }
  }

  for (byte i = 0; i < GPIO_MAX; i++) {
    pin[i] = 99;
  }
  for (byte i = 0; i < MAX_GPIO_PIN; i++) {
    mpin = ValidGPIO(i, my_module.gp.io[i]);




    if (mpin) {
      if ((mpin >= GPIO_SWT1_NP) && (mpin < (GPIO_SWT1_NP + MAX_SWITCHES))) {
        bitSet(switch_no_pullup, mpin - GPIO_SWT1_NP);
        mpin -= (GPIO_SWT1_NP - GPIO_SWT1);
      }
      else if ((mpin >= GPIO_KEY1_NP) && (mpin < (GPIO_KEY1_NP + MAX_KEYS))) {
        bitSet(key_no_pullup, mpin - GPIO_KEY1_NP);
        mpin -= (GPIO_KEY1_NP - GPIO_KEY1);
      }
      else if ((mpin >= GPIO_REL1_INV) && (mpin < (GPIO_REL1_INV + MAX_RELAYS))) {
        bitSet(rel_inverted, mpin - GPIO_REL1_INV);
        mpin -= (GPIO_REL1_INV - GPIO_REL1);
      }
      else if ((mpin >= GPIO_LED1_INV) && (mpin < (GPIO_LED1_INV + MAX_LEDS))) {
        bitSet(led_inverted, mpin - GPIO_LED1_INV);
        mpin -= (GPIO_LED1_INV - GPIO_LED1);
      }
      else if ((mpin >= GPIO_PWM1_INV) && (mpin < (GPIO_PWM1_INV + MAX_PWMS))) {
        bitSet(pwm_inverted, mpin - GPIO_PWM1_INV);
        mpin -= (GPIO_PWM1_INV - GPIO_PWM1);
      }
      else if ((mpin >= GPIO_CNTR1_NP) && (mpin < (GPIO_CNTR1_NP + MAX_COUNTERS))) {
        bitSet(counter_no_pullup, mpin - GPIO_CNTR1_NP);
        mpin -= (GPIO_CNTR1_NP - GPIO_CNTR1);
      }
#ifdef USE_DHT
      else if ((mpin >= GPIO_DHT11) && (mpin <= GPIO_SI7021)) {
        if (DhtSetup(i, mpin)) {
          dht_flg = 1;
          mpin = GPIO_DHT11;
        } else {
          mpin = 0;
        }
      }
#endif
    }
    if (mpin) pin[mpin] = i;
  }

  if ((2 == pin[GPIO_TXD]) || (H801 == Settings.module)) { Serial.set_tx(2); }

  analogWriteRange(Settings.pwm_range);
  analogWriteFreq(Settings.pwm_frequency);

#ifdef USE_SPI
  spi_flg = ((((pin[GPIO_SPI_CS] < 99) && (pin[GPIO_SPI_CS] > 14)) || (pin[GPIO_SPI_CS] < 12)) || (((pin[GPIO_SPI_DC] < 99) && (pin[GPIO_SPI_DC] > 14)) || (pin[GPIO_SPI_DC] < 12)));
  if (spi_flg) {
    for (byte i = 0; i < GPIO_MAX; i++) {
      if ((pin[i] >= 12) && (pin[i] <=14)) pin[i] = 99;
    }
    my_module.gp.io[12] = GPIO_SPI_MISO;
    pin[GPIO_SPI_MISO] = 12;
    my_module.gp.io[13] = GPIO_SPI_MOSI;
    pin[GPIO_SPI_MOSI] = 13;
    my_module.gp.io[14] = GPIO_SPI_CLK;
    pin[GPIO_SPI_CLK] = 14;
  }
#endif

#ifdef USE_I2C
  i2c_flg = ((pin[GPIO_I2C_SCL] < 99) && (pin[GPIO_I2C_SDA] < 99));
  if (i2c_flg) Wire.begin(pin[GPIO_I2C_SDA], pin[GPIO_I2C_SCL]);
#endif

  devices_present = 1;

  light_type = LT_BASIC;
  if (Settings.flag.pwm_control) {
    for (byte i = 0; i < MAX_PWMS; i++) {
      if (pin[GPIO_PWM1 +i] < 99) light_type++;
    }
  }

  if (SONOFF_BRIDGE == Settings.module) {
    Settings.flag.mqtt_serial = 0;
    baudrate = 19200;
  }

  if (XdrvCall(FUNC_MODULE_INIT)) {

  }
  else if (SONOFF_DUAL == Settings.module) {
    Settings.flag.mqtt_serial = 0;
    devices_present = 2;
    baudrate = 19200;
  }
  else if (CH4 == Settings.module) {
    Settings.flag.mqtt_serial = 0;
    devices_present = 4;
    baudrate = 19200;
  }
  else if (SONOFF_SC == Settings.module) {
    Settings.flag.mqtt_serial = 0;
    devices_present = 0;
    baudrate = 19200;
  }
  else if (SONOFF_BN == Settings.module) {
    light_type = LT_PWM1;
  }
  else if (SONOFF_LED == Settings.module) {
    light_type = LT_PWM2;
  }
  else if (AILIGHT == Settings.module) {
    light_type = LT_RGBW;
  }
  else if (SONOFF_B1 == Settings.module) {
    light_type = LT_RGBWC;
  }
  else {
    if (!light_type) devices_present = 0;
    for (byte i = 0; i < MAX_RELAYS; i++) {
      if (pin[GPIO_REL1 +i] < 99) {
        pinMode(pin[GPIO_REL1 +i], OUTPUT);
        devices_present++;
        if (EXS_RELAY == Settings.module) {
          digitalWrite(pin[GPIO_REL1 +i], bitRead(rel_inverted, i) ? 1 : 0);
          if (i &1) { devices_present--; }
        }
      }
    }
  }

  for (byte i = 0; i < MAX_KEYS; i++) {
    if (pin[GPIO_KEY1 +i] < 99) {
      pinMode(pin[GPIO_KEY1 +i], (16 == pin[GPIO_KEY1 +i]) ? INPUT_PULLDOWN_16 : bitRead(key_no_pullup, i) ? INPUT : INPUT_PULLUP);
    }
  }
  for (byte i = 0; i < MAX_LEDS; i++) {
    if (pin[GPIO_LED1 +i] < 99) {
      pinMode(pin[GPIO_LED1 +i], OUTPUT);
      digitalWrite(pin[GPIO_LED1 +i], bitRead(led_inverted, i));
    }
  }
  for (byte i = 0; i < MAX_SWITCHES; i++) {
    lastwallswitch[i] = 1;
    if (pin[GPIO_SWT1 +i] < 99) {
      GpioSwitchPinMode(i);
      lastwallswitch[i] = digitalRead(pin[GPIO_SWT1 +i]);
    }
    virtualswitch[i] = lastwallswitch[i];
  }

#ifdef USE_WS2812
  if (!light_type && (pin[GPIO_WS2812] < 99)) {
    devices_present++;
    light_type = LT_WS2812;
  }
#endif
  if (!light_type) {
    for (byte i = 0; i < MAX_PWMS; i++) {
      if (pin[GPIO_PWM1 +i] < 99) {
        pwm_present = true;
        pinMode(pin[GPIO_PWM1 +i], OUTPUT);
        analogWrite(pin[GPIO_PWM1 +i], bitRead(pwm_inverted, i) ? Settings.pwm_range - Settings.pwm_value[i] : Settings.pwm_value[i]);
      }
    }
  }

  SetLedPower(Settings.ledstate &8);

  XdrvCall(FUNC_PRE_INIT);
}

extern "C" {
extern struct rst_info resetInfo;
}

void setup()
{
  byte idx;

  RtcRebootLoad();
  if (!RtcRebootValid()) { RtcReboot.fast_reboot_count = 0; }
  RtcReboot.fast_reboot_count++;
  RtcRebootSave();

  Serial.begin(baudrate);
  delay(10);
  Serial.println();
  seriallog_level = LOG_LEVEL_INFO;

  snprintf_P(my_version, sizeof(my_version), PSTR("%d.%d.%d"), VERSION >> 24 & 0xff, VERSION >> 16 & 0xff, VERSION >> 8 & 0xff);
  if (VERSION & 0xff) {
    snprintf_P(my_version, sizeof(my_version), PSTR("%s.%d"), my_version, VERSION & 0xff);
  }
#ifdef BE_MINIMAL
  snprintf_P(my_version, sizeof(my_version), PSTR("%s-" D_JSON_MINIMAL), my_version);
#endif

  SettingsLoad();
  SettingsDelta();

  OsWatchInit();

  GetFeatures();

  if (1 == RtcReboot.fast_reboot_count) {
    XdrvCall(FUNC_SETTINGS_OVERRIDE);
  }

  baudrate = Settings.baudrate * 1200;
  mdns_delayed_start = Settings.param[P_MDNS_DELAYED_START];
  seriallog_level = Settings.seriallog_level;
  seriallog_timer = SERIALLOG_TIMER;
#ifndef USE_EMULATION
  Settings.flag2.emulation = 0;
#endif
  syslog_level = Settings.syslog_level;
  stop_flash_rotate = Settings.flag.stop_flash_rotate;
  save_data_counter = Settings.save_data;
  sleep = Settings.sleep;


  if (RtcReboot.fast_reboot_count > 1) {
    Settings.flag3.user_esp8285_enable = 0;
    if (RtcReboot.fast_reboot_count > 2) {
      for (byte i = 0; i < MAX_RULE_SETS; i++) {
        if (bitRead(Settings.rule_stop, i)) {
          bitWrite(Settings.rule_enabled, i, 0);
        }
      }
    }
    if (RtcReboot.fast_reboot_count > 3) {
      Settings.rule_enabled = 0;
    }
    if (RtcReboot.fast_reboot_count > 4) {
      Settings.module = SONOFF_BASIC;
      Settings.last_module = SONOFF_BASIC;
      for (byte i = 0; i < MAX_GPIO_PIN; i++) {
        Settings.my_gp.io[i] = GPIO_NONE;
      }
    }
    snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_APPLICATION D_LOG_SOME_SETTINGS_RESET " (%d)"), RtcReboot.fast_reboot_count);
    AddLog(LOG_LEVEL_DEBUG);
  }

  Settings.bootcount++;
  snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_APPLICATION D_BOOT_COUNT " %d"), Settings.bootcount);
  AddLog(LOG_LEVEL_DEBUG);

  Format(mqtt_client, Settings.mqtt_client, sizeof(mqtt_client));
  Format(mqtt_topic, Settings.mqtt_topic, sizeof(mqtt_topic));
  if (strstr(Settings.hostname, "%")) {
    strlcpy(Settings.hostname, WIFI_HOSTNAME, sizeof(Settings.hostname));
    snprintf_P(my_hostname, sizeof(my_hostname)-1, Settings.hostname, mqtt_topic, ESP.getChipId() & 0x1FFF);
  } else {
    snprintf_P(my_hostname, sizeof(my_hostname)-1, Settings.hostname);
  }

  GpioInit();

  SetSerialBaudrate(baudrate);

  WifiConnect();

  if (MOTOR == Settings.module) Settings.poweronstate = POWER_ALL_ON;
  if (POWER_ALL_ALWAYS_ON == Settings.poweronstate) {
    SetDevicePower(1, SRC_RESTART);
  } else {
    if ((resetInfo.reason == REASON_DEFAULT_RST) || (resetInfo.reason == REASON_EXT_SYS_RST)) {
      switch (Settings.poweronstate) {
      case POWER_ALL_OFF:
      case POWER_ALL_OFF_PULSETIME_ON:
        power = 0;
        SetDevicePower(power, SRC_RESTART);
        break;
      case POWER_ALL_ON:
        power = (1 << devices_present) -1;
        SetDevicePower(power, SRC_RESTART);
        break;
      case POWER_ALL_SAVED_TOGGLE:
        power = (Settings.power & ((1 << devices_present) -1)) ^ POWER_MASK;
        if (Settings.flag.save_state) {
          SetDevicePower(power, SRC_RESTART);
        }
        break;
      case POWER_ALL_SAVED:
        power = Settings.power & ((1 << devices_present) -1);
        if (Settings.flag.save_state) {
          SetDevicePower(power, SRC_RESTART);
        }
        break;
      }
    } else {
      power = Settings.power & ((1 << devices_present) -1);
      if (Settings.flag.save_state) {
        SetDevicePower(power, SRC_RESTART);
      }
    }
  }


  for (byte i = 0; i < devices_present; i++) {
    if ((i < MAX_RELAYS) && (pin[GPIO_REL1 +i] < 99)) {
      bitWrite(power, i, digitalRead(pin[GPIO_REL1 +i]) ^ bitRead(rel_inverted, i));
    }
    if ((i < MAX_PULSETIMERS) && (bitRead(power, i) || (POWER_ALL_OFF_PULSETIME_ON == Settings.poweronstate))) {
      SetPulseTimer(i, Settings.pulse_timer[i]);
    }
  }
  blink_powersave = power;

  snprintf_P(log_data, sizeof(log_data), PSTR(D_PROJECT " %s %s (" D_CMND_TOPIC " %s, " D_FALLBACK " %s, " D_CMND_GROUPTOPIC " %s) " D_VERSION " %s-" ARDUINO_ESP8266_RELEASE),
    PROJECT, Settings.friendlyname[0], mqtt_topic, mqtt_client, Settings.mqtt_grptopic, my_version);
  AddLog(LOG_LEVEL_INFO);
#ifdef BE_MINIMAL
  snprintf_P(log_data, sizeof(log_data), PSTR(D_WARNING_MINIMAL_VERSION));
  AddLog(LOG_LEVEL_INFO);
#endif

  RtcInit();

#ifdef USE_ARDUINO_OTA
  ArduinoOTAInit();
#endif

  XdrvCall(FUNC_INIT);
  XsnsCall(FUNC_INIT);
}

void loop()
{
  XdrvCall(FUNC_LOOP);

  OsWatchLoop();

  if (TimeReached(button_debounce)) {
    SetNextTimeInterval(button_debounce, Settings.button_debounce);
    ButtonHandler();
  }
  if (TimeReached(switch_debounce)) {
    SetNextTimeInterval(switch_debounce, Settings.switch_debounce);
    SwitchHandler(0);
  }
  if (TimeReached(state_50msecond)) {
    SetNextTimeInterval(state_50msecond, 50);
    XdrvCall(FUNC_EVERY_50_MSECOND);
    XsnsCall(FUNC_EVERY_50_MSECOND);
  }
  if (TimeReached(state_100msecond)) {
    SetNextTimeInterval(state_100msecond, 100);
    Every100mSeconds();
    XdrvCall(FUNC_EVERY_100_MSECOND);
    XsnsCall(FUNC_EVERY_100_MSECOND);
  }
  if (TimeReached(state_250msecond)) {
    SetNextTimeInterval(state_250msecond, 250);
    Every250mSeconds();
    XdrvCall(FUNC_EVERY_250_MSECOND);
    XsnsCall(FUNC_EVERY_250_MSECOND);
  }

  if (!serial_local) SerialInput();

#ifdef USE_ARDUINO_OTA
  ArduinoOTA.handle();

  while (arduino_ota_triggered) ArduinoOTA.handle();
#endif


  delay(sleep);
}
# 1 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/_changelog.ino"
# 1 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/settings.ino"
# 20 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/settings.ino"
#ifndef DOMOTICZ_UPDATE_TIMER
#define DOMOTICZ_UPDATE_TIMER 0
#endif

#ifndef EMULATION
#define EMULATION EMUL_NONE
#endif

#ifndef MTX_ADDRESS1
#define MTX_ADDRESS1 0
#endif
#ifndef MTX_ADDRESS2
#define MTX_ADDRESS2 0
#endif
#ifndef MTX_ADDRESS3
#define MTX_ADDRESS3 0
#endif
#ifndef MTX_ADDRESS4
#define MTX_ADDRESS4 0
#endif
#ifndef MTX_ADDRESS5
#define MTX_ADDRESS5 0
#endif
#ifndef MTX_ADDRESS6
#define MTX_ADDRESS6 0
#endif
#ifndef MTX_ADDRESS7
#define MTX_ADDRESS7 0
#endif
#ifndef MTX_ADDRESS8
#define MTX_ADDRESS8 0
#endif

#ifndef HOME_ASSISTANT_DISCOVERY_ENABLE
#define HOME_ASSISTANT_DISCOVERY_ENABLE 0
#endif

#ifndef LATITUDE
#define LATITUDE 48.858360
#endif
#ifndef LONGITUDE
#define LONGITUDE 2.294442
#endif





#define RTC_MEM_VALID 0xA55A

uint32_t rtc_settings_crc = 0;

uint32_t GetRtcSettingsCrc()
{
  uint32_t crc = 0;
  uint8_t *bytes = (uint8_t*)&RtcSettings;

  for (uint16_t i = 0; i < sizeof(RTCMEM); i++) {
    crc += bytes[i]*(i+1);
  }
  return crc;
}

void RtcSettingsSave()
{
  if (GetRtcSettingsCrc() != rtc_settings_crc) {
    RtcSettings.valid = RTC_MEM_VALID;
    ESP.rtcUserMemoryWrite(100, (uint32_t*)&RtcSettings, sizeof(RTCMEM));
    rtc_settings_crc = GetRtcSettingsCrc();
  }
}

void RtcSettingsLoad()
{
  ESP.rtcUserMemoryRead(100, (uint32_t*)&RtcSettings, sizeof(RTCMEM));
  if (RtcSettings.valid != RTC_MEM_VALID) {
    memset(&RtcSettings, 0, sizeof(RTCMEM));
    RtcSettings.valid = RTC_MEM_VALID;
    RtcSettings.energy_kWhtoday = Settings.energy_kWhtoday;
    RtcSettings.energy_kWhtotal = Settings.energy_kWhtotal;
    for (byte i = 0; i < MAX_COUNTERS; i++) {
      RtcSettings.pulse_counter[i] = Settings.pulse_counter[i];
    }
    RtcSettings.power = Settings.power;
    RtcSettingsSave();
  }
  rtc_settings_crc = GetRtcSettingsCrc();
}

boolean RtcSettingsValid()
{
  return (RTC_MEM_VALID == RtcSettings.valid);
}



uint32_t rtc_reboot_crc = 0;

uint32_t GetRtcRebootCrc()
{
  uint32_t crc = 0;
  uint8_t *bytes = (uint8_t*)&RtcReboot;

  for (uint16_t i = 0; i < sizeof(RTCRBT); i++) {
    crc += bytes[i]*(i+1);
  }
  return crc;
}

void RtcRebootSave()
{
  if (GetRtcRebootCrc() != rtc_reboot_crc) {
    RtcReboot.valid = RTC_MEM_VALID;
    ESP.rtcUserMemoryWrite(100 - sizeof(RTCRBT), (uint32_t*)&RtcReboot, sizeof(RTCRBT));
    rtc_reboot_crc = GetRtcRebootCrc();
  }
}

void RtcRebootLoad()
{
  ESP.rtcUserMemoryRead(100 - sizeof(RTCRBT), (uint32_t*)&RtcReboot, sizeof(RTCRBT));
  if (RtcReboot.valid != RTC_MEM_VALID) {
    memset(&RtcReboot, 0, sizeof(RTCRBT));
    RtcReboot.valid = RTC_MEM_VALID;

    RtcRebootSave();
  }
  rtc_reboot_crc = GetRtcRebootCrc();
}

boolean RtcRebootValid()
{
  return (RTC_MEM_VALID == RtcReboot.valid);
}





extern "C" {
#include "spi_flash.h"
}
#include "eboot_command.h"

extern "C" uint32_t _SPIFFS_end;


#define SPIFFS_END ((uint32_t)&_SPIFFS_end - 0x40200000) / SPI_FLASH_SEC_SIZE


#define SETTINGS_LOCATION SPIFFS_END

#define CFG_ROTATES 8

uint16_t settings_crc = 0;
uint32_t settings_location = SETTINGS_LOCATION;
uint8_t *settings_buffer = NULL;





void SetFlashModeDout()
{
  uint8_t *_buffer;
  uint32_t address;

  eboot_command ebcmd;
  eboot_command_read(&ebcmd);
  address = ebcmd.args[0];
  _buffer = new uint8_t[FLASH_SECTOR_SIZE];

  if (ESP.flashRead(address, (uint32_t*)_buffer, FLASH_SECTOR_SIZE)) {
    if (_buffer[2] != 3) {
      _buffer[2] = 3;
      if (ESP.flashEraseSector(address / FLASH_SECTOR_SIZE)) ESP.flashWrite(address, (uint32_t*)_buffer, FLASH_SECTOR_SIZE);
    }
  }
  delete[] _buffer;
}

void SettingsBufferFree()
{
  if (settings_buffer != NULL) {
    free(settings_buffer);
    settings_buffer = NULL;
  }
}

bool SettingsBufferAlloc()
{
  SettingsBufferFree();
  if (!(settings_buffer = (uint8_t *)malloc(sizeof(Settings)))) {
    AddLog_P(LOG_LEVEL_DEBUG, PSTR(D_LOG_APPLICATION D_UPLOAD_ERR_2));
    return false;
  }
  return true;
}

uint16_t GetSettingsCrc()
{
  uint16_t crc = 0;
  uint8_t *bytes = (uint8_t*)&Settings;

  for (uint16_t i = 0; i < sizeof(SYSCFG); i++) {
    if ((i < 14) || (i > 15)) { crc += bytes[i]*(i+1); }
  }
  return crc;
}

void SettingsSaveAll()
{
  if (Settings.flag.save_state) {
    Settings.power = power;
  } else {
    Settings.power = 0;
  }
  XsnsCall(FUNC_SAVE_BEFORE_RESTART);
  SettingsSave(0);
}





uint32_t GetSettingsAddress()
{
  return settings_location * SPI_FLASH_SEC_SIZE;
}

void SettingsSave(byte rotate)
{
# 260 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/settings.ino"
#ifndef BE_MINIMAL
  if ((GetSettingsCrc() != settings_crc) || rotate) {
    if (1 == rotate) {
      stop_flash_rotate = 1;
    }
    if (2 == rotate) {
      settings_location = SETTINGS_LOCATION +1;
    }
    if (stop_flash_rotate) {
      settings_location = SETTINGS_LOCATION;
    } else {
      settings_location--;
      if (settings_location <= (SETTINGS_LOCATION - CFG_ROTATES)) {
        settings_location = SETTINGS_LOCATION;
      }
    }
    Settings.save_flag++;
    Settings.cfg_size = sizeof(SYSCFG);
    Settings.cfg_crc = GetSettingsCrc();
    ESP.flashEraseSector(settings_location);
    ESP.flashWrite(settings_location * SPI_FLASH_SEC_SIZE, (uint32*)&Settings, sizeof(SYSCFG));
    if (!stop_flash_rotate && rotate) {
      for (byte i = 1; i < CFG_ROTATES; i++) {
        ESP.flashEraseSector(settings_location -i);
        delay(1);
      }
    }
    snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_CONFIG D_SAVED_TO_FLASH_AT " %X, " D_COUNT " %d, " D_BYTES " %d"),
       settings_location, Settings.save_flag, sizeof(SYSCFG));
    AddLog(LOG_LEVEL_DEBUG);

    settings_crc = Settings.cfg_crc;
  }
#endif
  RtcSettingsSave();
}

void SettingsLoad()
{


  struct SYSCFGH {
    uint16_t cfg_holder;
    uint16_t cfg_size;
    unsigned long save_flag;
  } _SettingsH;

  bool bad_crc = false;
  settings_location = SETTINGS_LOCATION +1;
  for (byte i = 0; i < CFG_ROTATES; i++) {
    settings_location--;
    ESP.flashRead(settings_location * SPI_FLASH_SEC_SIZE, (uint32*)&Settings, sizeof(SYSCFG));
    ESP.flashRead((settings_location -1) * SPI_FLASH_SEC_SIZE, (uint32*)&_SettingsH, sizeof(SYSCFGH));
    if (Settings.version > 0x06000000) { bad_crc = (Settings.cfg_crc != GetSettingsCrc()); }
    if (Settings.flag.stop_flash_rotate || bad_crc || (Settings.cfg_holder != _SettingsH.cfg_holder) || (Settings.save_flag > _SettingsH.save_flag)) {
      break;
    }
    delay(1);
  }
  snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_CONFIG D_LOADED_FROM_FLASH_AT " %X, " D_COUNT " %d"), settings_location, Settings.save_flag);
  AddLog(LOG_LEVEL_DEBUG);

#ifndef BE_MINIMAL
  if (bad_crc || (Settings.cfg_holder != (uint16_t)CFG_HOLDER)) { SettingsDefault(); }
  settings_crc = GetSettingsCrc();
#endif

  RtcSettingsLoad();
}

void SettingsErase(uint8_t type)
{





  bool result;

  uint32_t _sectorStart = (ESP.getSketchSize() / SPI_FLASH_SEC_SIZE) + 1;
  uint32_t _sectorEnd = ESP.getFlashChipRealSize() / SPI_FLASH_SEC_SIZE;
  if (1 == type) {
    _sectorStart = SETTINGS_LOCATION +2;
    _sectorEnd = SETTINGS_LOCATION +5;
  }

  boolean _serialoutput = (LOG_LEVEL_DEBUG_MORE <= seriallog_level);

  snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_APPLICATION D_ERASE " %d " D_UNIT_SECTORS), _sectorEnd - _sectorStart);
  AddLog(LOG_LEVEL_DEBUG);

  for (uint32_t _sector = _sectorStart; _sector < _sectorEnd; _sector++) {
    result = ESP.flashEraseSector(_sector);
    if (_serialoutput) {
      Serial.print(F(D_LOG_APPLICATION D_ERASED_SECTOR " "));
      Serial.print(_sector);
      if (result) {
        Serial.println(F(" " D_OK));
      } else {
        Serial.println(F(" " D_ERROR));
      }
      delay(10);
    }
    OsWatchLoop();
  }
}


bool SettingsEraseConfig(void) {
  const size_t cfgSize = 0x4000;
  size_t cfgAddr = ESP.getFlashChipSize() - cfgSize;

  for (size_t offset = 0; offset < cfgSize; offset += SPI_FLASH_SEC_SIZE) {
    if (!ESP.flashEraseSector((cfgAddr + offset) / SPI_FLASH_SEC_SIZE)) {
      return false;
    }
  }
  return true;
}

void SettingsSdkErase()
{
  WiFi.disconnect(true);
  SettingsErase(1);
  SettingsEraseConfig();
  delay(1000);
}



void SettingsDefault()
{
  AddLog_P(LOG_LEVEL_NONE, PSTR(D_LOG_CONFIG D_USE_DEFAULTS));
  SettingsDefaultSet1();
  SettingsDefaultSet2();
  SettingsSave(2);
}

void SettingsDefaultSet1()
{
  memset(&Settings, 0x00, sizeof(SYSCFG));

  Settings.cfg_holder = (uint16_t)CFG_HOLDER;
  Settings.cfg_size = sizeof(SYSCFG);

  Settings.version = VERSION;


}

void SettingsDefaultSet2()
{
  memset((char*)&Settings +16, 0x00, sizeof(SYSCFG) -16);



  Settings.save_data = SAVE_DATA;
  Settings.sleep = APP_SLEEP;



  Settings.module = MODULE;

  strlcpy(Settings.friendlyname[0], FRIENDLY_NAME, sizeof(Settings.friendlyname[0]));
  strlcpy(Settings.friendlyname[1], FRIENDLY_NAME"2", sizeof(Settings.friendlyname[1]));
  strlcpy(Settings.friendlyname[2], FRIENDLY_NAME"3", sizeof(Settings.friendlyname[2]));
  strlcpy(Settings.friendlyname[3], FRIENDLY_NAME"4", sizeof(Settings.friendlyname[3]));
  strlcpy(Settings.ota_url, OTA_URL, sizeof(Settings.ota_url));


  Settings.flag.save_state = SAVE_STATE;
  Settings.power = APP_POWER;
  Settings.poweronstate = APP_POWERON_STATE;
  Settings.blinktime = APP_BLINKTIME;
  Settings.blinkcount = APP_BLINKCOUNT;
  Settings.ledstate = APP_LEDSTATE;
  Settings.pulse_timer[0] = APP_PULSETIME;



  Settings.baudrate = APP_BAUDRATE / 1200;
  Settings.sbaudrate = SOFT_BAUDRATE / 1200;
  Settings.serial_delimiter = 0xff;
  Settings.seriallog_level = SERIAL_LOG_LEVEL;


  ParseIp(&Settings.ip_address[0], WIFI_IP_ADDRESS);
  ParseIp(&Settings.ip_address[1], WIFI_GATEWAY);
  ParseIp(&Settings.ip_address[2], WIFI_SUBNETMASK);
  ParseIp(&Settings.ip_address[3], WIFI_DNS);
  Settings.sta_config = WIFI_CONFIG_TOOL;

  strlcpy(Settings.sta_ssid[0], STA_SSID1, sizeof(Settings.sta_ssid[0]));
  strlcpy(Settings.sta_pwd[0], STA_PASS1, sizeof(Settings.sta_pwd[0]));
  strlcpy(Settings.sta_ssid[1], STA_SSID2, sizeof(Settings.sta_ssid[1]));
  strlcpy(Settings.sta_pwd[1], STA_PASS2, sizeof(Settings.sta_pwd[1]));
  strlcpy(Settings.hostname, WIFI_HOSTNAME, sizeof(Settings.hostname));


  strlcpy(Settings.syslog_host, SYS_LOG_HOST, sizeof(Settings.syslog_host));
  Settings.syslog_port = SYS_LOG_PORT;
  Settings.syslog_level = SYS_LOG_LEVEL;


  Settings.flag2.emulation = EMULATION;
  Settings.webserver = WEB_SERVER;
  Settings.weblog_level = WEB_LOG_LEVEL;
  strlcpy(Settings.web_password, WEB_PASSWORD, sizeof(Settings.web_password));





  Settings.param[P_HOLD_TIME] = KEY_HOLD_TIME;


  for (byte i = 0; i < MAX_SWITCHES; i++) { Settings.switchmode[i] = SWITCH_MODE; }


  Settings.flag.mqtt_enabled = MQTT_USE;

  Settings.flag.mqtt_power_retain = MQTT_POWER_RETAIN;
  Settings.flag.mqtt_button_retain = MQTT_BUTTON_RETAIN;
  Settings.flag.mqtt_switch_retain = MQTT_SWITCH_RETAIN;




  strlcpy(Settings.mqtt_host, MQTT_HOST, sizeof(Settings.mqtt_host));
  Settings.mqtt_port = MQTT_PORT;
  strlcpy(Settings.mqtt_client, MQTT_CLIENT_ID, sizeof(Settings.mqtt_client));
  strlcpy(Settings.mqtt_user, MQTT_USER, sizeof(Settings.mqtt_user));
  strlcpy(Settings.mqtt_pwd, MQTT_PASS, sizeof(Settings.mqtt_pwd));
  strlcpy(Settings.mqtt_topic, MQTT_TOPIC, sizeof(Settings.mqtt_topic));
  strlcpy(Settings.button_topic, MQTT_BUTTON_TOPIC, sizeof(Settings.button_topic));
  strlcpy(Settings.switch_topic, MQTT_SWITCH_TOPIC, sizeof(Settings.switch_topic));
  strlcpy(Settings.mqtt_grptopic, MQTT_GRPTOPIC, sizeof(Settings.mqtt_grptopic));
  strlcpy(Settings.mqtt_fulltopic, MQTT_FULLTOPIC, sizeof(Settings.mqtt_fulltopic));
  Settings.mqtt_retry = MQTT_RETRY_SECS;
  strlcpy(Settings.mqtt_prefix[0], SUB_PREFIX, sizeof(Settings.mqtt_prefix[0]));
  strlcpy(Settings.mqtt_prefix[1], PUB_PREFIX, sizeof(Settings.mqtt_prefix[1]));
  strlcpy(Settings.mqtt_prefix[2], PUB_PREFIX2, sizeof(Settings.mqtt_prefix[2]));
  strlcpy(Settings.state_text[0], MQTT_STATUS_OFF, sizeof(Settings.state_text[0]));
  strlcpy(Settings.state_text[1], MQTT_STATUS_ON, sizeof(Settings.state_text[1]));
  strlcpy(Settings.state_text[2], MQTT_CMND_TOGGLE, sizeof(Settings.state_text[2]));
  strlcpy(Settings.state_text[3], MQTT_CMND_HOLD, sizeof(Settings.state_text[3]));
  char fingerprint[60];
  strlcpy(fingerprint, MQTT_FINGERPRINT1, sizeof(fingerprint));
  char *p = fingerprint;
  for (byte i = 0; i < 20; i++) {
    Settings.mqtt_fingerprint[0][i] = strtol(p, &p, 16);
  }
  strlcpy(fingerprint, MQTT_FINGERPRINT2, sizeof(fingerprint));
  p = fingerprint;
  for (byte i = 0; i < 20; i++) {
    Settings.mqtt_fingerprint[1][i] = strtol(p, &p, 16);
  }
  Settings.tele_period = TELE_PERIOD;


  Settings.flag2.current_resolution = 3;


  Settings.flag2.energy_resolution = ENERGY_RESOLUTION;
  Settings.param[P_MAX_POWER_RETRY] = MAX_POWER_RETRY;
  Settings.energy_power_delta = DEFAULT_POWER_DELTA;
  Settings.energy_power_calibration = HLW_PREF_PULSE;
  Settings.energy_voltage_calibration = HLW_UREF_PULSE;
  Settings.energy_current_calibration = HLW_IREF_PULSE;
# 539 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/settings.ino"
  Settings.energy_max_power_limit_hold = MAX_POWER_HOLD;
  Settings.energy_max_power_limit_window = MAX_POWER_WINDOW;

  Settings.energy_max_power_safe_limit_hold = SAFE_POWER_HOLD;
  Settings.energy_max_power_safe_limit_window = SAFE_POWER_WINDOW;



  RtcSettings.energy_kWhtotal = 0;



  memcpy_P(Settings.rf_code[0], kDefaultRfCode, 9);


  Settings.domoticz_update_timer = DOMOTICZ_UPDATE_TIMER;
# 565 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/settings.ino"
  Settings.flag.temperature_conversion = TEMP_CONVERSION;
  Settings.flag2.pressure_resolution = PRESSURE_RESOLUTION;
  Settings.flag2.humidity_resolution = HUMIDITY_RESOLUTION;
  Settings.flag2.temperature_resolution = TEMP_RESOLUTION;
# 577 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/settings.ino"
  Settings.flag.hass_discovery = HOME_ASSISTANT_DISCOVERY_ENABLE;






  Settings.flag.pwm_control = 1;




  Settings.pwm_frequency = PWM_FREQ;
  Settings.pwm_range = PWM_RANGE;
  for (byte i = 0; i < MAX_PWMS; i++) {
    Settings.light_color[i] = 255;

  }

  Settings.light_dimmer = 10;

  Settings.light_speed = 1;

  Settings.light_width = 1;

  Settings.light_pixels = WS2812_LEDS;

  SettingsDefaultSet_5_8_1();


  SettingsDefaultSet_5_10_1();


  Settings.timezone = APP_TIMEZONE;
  strlcpy(Settings.ntp_server[0], NTP_SERVER1, sizeof(Settings.ntp_server[0]));
  strlcpy(Settings.ntp_server[1], NTP_SERVER2, sizeof(Settings.ntp_server[1]));
  strlcpy(Settings.ntp_server[2], NTP_SERVER3, sizeof(Settings.ntp_server[2]));
  for (byte j = 0; j < 3; j++) {
    for (byte i = 0; i < strlen(Settings.ntp_server[j]); i++) {
      if (Settings.ntp_server[j][i] == ',') {
        Settings.ntp_server[j][i] = '.';
      }
    }
  }
  Settings.latitude = (int)((double)LATITUDE * 1000000);
  Settings.longitude = (int)((double)LONGITUDE * 1000000);
  SettingsDefaultSet_5_13_1c();

  Settings.button_debounce = KEY_DEBOUNCE_TIME;
  Settings.switch_debounce = SWITCH_DEBOUNCE_TIME;

  for (byte j = 0; j < 5; j++) {
    Settings.rgbwwTable[j] = 255;
  }
}



void SettingsDefaultSet_5_8_1()
{

  Settings.ws_width[WS_SECOND] = 1;
  Settings.ws_color[WS_SECOND][WS_RED] = 255;
  Settings.ws_color[WS_SECOND][WS_GREEN] = 0;
  Settings.ws_color[WS_SECOND][WS_BLUE] = 255;
  Settings.ws_width[WS_MINUTE] = 3;
  Settings.ws_color[WS_MINUTE][WS_RED] = 0;
  Settings.ws_color[WS_MINUTE][WS_GREEN] = 255;
  Settings.ws_color[WS_MINUTE][WS_BLUE] = 0;
  Settings.ws_width[WS_HOUR] = 5;
  Settings.ws_color[WS_HOUR][WS_RED] = 255;
  Settings.ws_color[WS_HOUR][WS_GREEN] = 0;
  Settings.ws_color[WS_HOUR][WS_BLUE] = 0;
}

void SettingsDefaultSet_5_10_1()
{
  Settings.display_model = 0;
  Settings.display_mode = 1;
  Settings.display_refresh = 2;
  Settings.display_rows = 2;
  Settings.display_cols[0] = 16;
  Settings.display_cols[1] = 8;
  Settings.display_dimmer = 1;
  Settings.display_size = 1;
  Settings.display_font = 1;
  Settings.display_rotate = 0;
  Settings.display_address[0] = MTX_ADDRESS1;
  Settings.display_address[1] = MTX_ADDRESS2;
  Settings.display_address[2] = MTX_ADDRESS3;
  Settings.display_address[3] = MTX_ADDRESS4;
  Settings.display_address[4] = MTX_ADDRESS5;
  Settings.display_address[5] = MTX_ADDRESS6;
  Settings.display_address[6] = MTX_ADDRESS7;
  Settings.display_address[7] = MTX_ADDRESS8;
}

void SettingsResetStd()
{
  Settings.tflag[0].hemis = TIME_STD_HEMISPHERE;
  Settings.tflag[0].week = TIME_STD_WEEK;
  Settings.tflag[0].dow = TIME_STD_DAY;
  Settings.tflag[0].month = TIME_STD_MONTH;
  Settings.tflag[0].hour = TIME_STD_HOUR;
  Settings.toffset[0] = TIME_STD_OFFSET;
}

void SettingsResetDst()
{
  Settings.tflag[1].hemis = TIME_DST_HEMISPHERE;
  Settings.tflag[1].week = TIME_DST_WEEK;
  Settings.tflag[1].dow = TIME_DST_DAY;
  Settings.tflag[1].month = TIME_DST_MONTH;
  Settings.tflag[1].hour = TIME_DST_HOUR;
  Settings.toffset[1] = TIME_DST_OFFSET;
}

void SettingsDefaultSet_5_13_1c()
{
  SettingsResetStd();
  SettingsResetDst();
}



void SettingsDelta()
{
  if (Settings.version != VERSION) {

    if (Settings.version < 0x05050000) {
      for (byte i = 0; i < 17; i++) { Settings.rf_code[i][0] = 0; }
      memcpy_P(Settings.rf_code[0], kDefaultRfCode, 9);
    }
    if (Settings.version < 0x05080000) {
      Settings.light_pixels = WS2812_LEDS;
      Settings.light_width = 1;
      Settings.light_color[0] = 255;
      Settings.light_color[1] = 0;
      Settings.light_color[2] = 0;
      Settings.light_dimmer = 10;
      Settings.light_correction = 0;
      Settings.light_fade = 0;
      Settings.light_speed = 1;
      Settings.light_scheme = 0;
      Settings.light_width = 1;
      Settings.light_wakeup = 0;
    }
    if (Settings.version < 0x0508000A) {
      Settings.power = 0;
      Settings.altitude = 0;
    }
    if (Settings.version < 0x0508000B) {
      for (byte i = 0; i < MAX_GPIO_PIN; i++) {
        if ((Settings.my_gp.io[i] >= 25) && (Settings.my_gp.io[i] <= 32)) {
          Settings.my_gp.io[i] += 23;
        }
      }
      for (byte i = 0; i < MAX_PWMS; i++) {
        Settings.pwm_value[i] = Settings.pulse_timer[4 +i];
        Settings.pulse_timer[4 +i] = 0;
      }
    }
    if (Settings.version < 0x0508000D) {
      Settings.pwm_frequency = PWM_FREQ;
      Settings.pwm_range = PWM_RANGE;
    }
    if (Settings.version < 0x0508000E) {
      SettingsDefaultSet_5_8_1();
    }
    if (Settings.version < 0x05090102) {
      Settings.flag2.data = Settings.flag.data;
      Settings.flag2.data &= 0xFFE80000;
      Settings.flag2.voltage_resolution = Settings.flag.not_power_linked;
      Settings.flag2.current_resolution = 3;
      Settings.ina219_mode = 0;
    }
    if (Settings.version < 0x050A0009) {
      SettingsDefaultSet_5_10_1();
    }
    if (Settings.version < 0x050B0107) {
      Settings.flag.not_power_linked = 0;
    }
    if (Settings.version < 0x050C0005) {
      Settings.light_rotation = 0;
      Settings.energy_power_delta = DEFAULT_POWER_DELTA;
      char fingerprint[60];
      memcpy(fingerprint, Settings.mqtt_fingerprint, sizeof(fingerprint));
      char *p = fingerprint;
      for (byte i = 0; i < 20; i++) {
        Settings.mqtt_fingerprint[0][i] = strtol(p, &p, 16);
        Settings.mqtt_fingerprint[1][i] = Settings.mqtt_fingerprint[0][i];
      }
    }
    if (Settings.version < 0x050C0007) {
      Settings.baudrate = APP_BAUDRATE / 1200;
    }
    if (Settings.version < 0x050C0008) {
      Settings.sbaudrate = SOFT_BAUDRATE / 1200;
      Settings.serial_delimiter = 0xff;
    }
    if (Settings.version < 0x050C000A) {
      Settings.latitude = (int)((double)LATITUDE * 1000000);
      Settings.longitude = (int)((double)LONGITUDE * 1000000);
    }
    if (Settings.version < 0x050C000B) {
      Settings.rules[0][0] = '\0';
    }
    if (Settings.version < 0x050C000D) {
      memmove(Settings.rules, Settings.rules -256, sizeof(Settings.rules));
      memset(&Settings.timer, 0x00, sizeof(Timer) * MAX_TIMERS);
      Settings.knx_GA_registered = 0;
      Settings.knx_CB_registered = 0;
      memset(&Settings.knx_physsical_addr, 0x00, 0x800 - 0x6b8);
    }
    if (Settings.version < 0x050C000F) {
      Settings.energy_kWhtoday /= 1000;
      Settings.energy_kWhyesterday /= 1000;
      RtcSettings.energy_kWhtoday /= 1000;
    }
    if (Settings.version < 0x050D0103) {
      SettingsDefaultSet_5_13_1c();
    }
    if (Settings.version < 0x050E0002) {
      for (byte i = 1; i < MAX_RULE_SETS; i++) { Settings.rules[i][0] = '\0'; }
      Settings.rule_enabled = Settings.flag.mqtt_serial_raw;
      Settings.rule_once = Settings.flag.rules_once;
    }
    if (Settings.version < 0x06000000) {
      Settings.cfg_size = sizeof(SYSCFG);
      Settings.cfg_crc = GetSettingsCrc();
    }
    if (Settings.version < 0x06000002) {
      for (byte i = 0; i < MAX_SWITCHES; i++) {
        if (i < 4) {
          Settings.switchmode[i] = Settings.ex_switchmode[i];
        } else {
          Settings.switchmode[i] = SWITCH_MODE;
        }
      }
      for (byte i = 0; i < MAX_GPIO_PIN; i++) {
        if (Settings.my_gp.io[i] >= GPIO_SWT5) {
          Settings.my_gp.io[i] += 4;
        }
      }
    }
    if (Settings.version < 0x06000003) {
      Settings.flag.mqtt_serial_raw = 0;
      Settings.flag.rules_once = 0;
      Settings.flag3.data = 0;
    }
    if (Settings.version < 0x06010103) {
      Settings.flag3.timers_enable = 1;
    }
    if (Settings.version < 0x0601010C) {
      Settings.button_debounce = KEY_DEBOUNCE_TIME;
      Settings.switch_debounce = SWITCH_DEBOUNCE_TIME;
    }
    if (Settings.version < 0x0602010A) {
      for (byte j = 0; j < 5; j++) {
        Settings.rgbwwTable[j] = 255;
      }
    }

    Settings.version = VERSION;
    SettingsSave(1);
  }
}
# 1 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/support.ino"
# 20 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/support.ino"
IPAddress syslog_host_addr;
uint32_t syslog_host_hash = 0;





Ticker tickerOSWatch;

#define OSWATCH_RESET_TIME 120

static unsigned long oswatch_last_loop_time;
byte oswatch_blocked_loop = 0;

#ifndef USE_WS2812_DMA

#endif

#ifdef USE_KNX
bool knx_started = false;
#endif

void OsWatchTicker()
{
  unsigned long t = millis();
  unsigned long last_run = abs(t - oswatch_last_loop_time);

#ifdef DEBUG_THEO
  snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_APPLICATION D_OSWATCH " FreeRam %d, rssi %d, last_run %d"), ESP.getFreeHeap(), WifiGetRssiAsQuality(WiFi.RSSI()), last_run);
  AddLog(LOG_LEVEL_DEBUG);
#endif
  if (last_run >= (OSWATCH_RESET_TIME * 1000)) {

    RtcSettings.oswatch_blocked_loop = 1;
    RtcSettingsSave();

    ESP.reset();
  }
}

void OsWatchInit()
{
  oswatch_blocked_loop = RtcSettings.oswatch_blocked_loop;
  RtcSettings.oswatch_blocked_loop = 0;
  oswatch_last_loop_time = millis();
  tickerOSWatch.attach_ms(((OSWATCH_RESET_TIME / 3) * 1000), OsWatchTicker);
}

void OsWatchLoop()
{
  oswatch_last_loop_time = millis();

}

String GetResetReason()
{
  char buff[32];
  if (oswatch_blocked_loop) {
    strncpy_P(buff, PSTR(D_JSON_BLOCKED_LOOP), sizeof(buff));
    return String(buff);
  } else {
    return ESP.getResetReason();
  }
}

boolean OsWatchBlockedLoop()
{
  return oswatch_blocked_loop;
}




#ifdef ARDUINO_ESP8266_RELEASE_2_3_0



void* memchr(const void* ptr, int value, size_t num)
{
  unsigned char *p = (unsigned char*)ptr;
  while (num--) {
    if (*p != (unsigned char)value) {
      p++;
    } else {
      return p;
    }
  }
  return 0;
}



size_t strcspn(const char *str1, const char *str2)
{
  size_t ret = 0;
  while (*str1) {
    if (strchr(str2, *str1)) {
      return ret;
    } else {
      str1++;
      ret++;
    }
  }
  return ret;
}
#endif


size_t strchrspn(const char *str1, int character)
{
  size_t ret = 0;
  char *start = (char*)str1;
  char *end = strchr(str1, character);
  if (end) ret = end - start;
  return ret;
}


char* subStr(char* dest, char* str, const char *delim, int index)
{
  char *act;
  char *sub;
  char *ptr;
  int i;


  strncpy(dest, str, strlen(str)+1);
  for (i = 1, act = dest; i <= index; i++, act = NULL) {
    sub = strtok_r(act, delim, &ptr);
    if (sub == NULL) break;
  }
  sub = Trim(sub);
  return sub;
}

double CharToDouble(char *str)
{

  char strbuf[24];

  strlcpy(strbuf, str, sizeof(strbuf));
  char *pt;
  double left = atoi(strbuf);
  double right = 0;
  short len = 0;
  pt = strtok (strbuf, ".");
  if (pt) {
    pt = strtok (NULL, ".");
    if (pt) {
      right = atoi(pt);
      len = strlen(pt);
      double fac = 1;
      while (len) {
        fac /= 10.0;
        len--;
      }


      right *= fac;
    }
  }
  double result = left + right;
  if (left < 0) { result = left - right; }
  return result;
}

int TextToInt(char *str)
{
  char *p;
  uint8_t radix = 10;
  if ('#' == str[0]) {
    radix = 16;
    str++;
  }
  return strtol(str, &p, radix);
}

char* dtostrfd(double number, unsigned char prec, char *s)
{
  if ((isnan(number)) || (isinf(number))) {
    strcpy(s, "null");
    return s;
  } else {
    return dtostrf(number, 1, prec, s);
  }
}

char* Unescape(char* buffer, uint16_t* size)
{
  uint8_t* read = (uint8_t*)buffer;
  uint8_t* write = (uint8_t*)buffer;
  uint16_t start_size = *size;
  uint16_t end_size = *size;
  uint8_t che = 0;

  while (start_size > 0) {
    uint8_t ch = *read++;
    start_size--;
    if (ch != '\\') {
      *write++ = ch;
    } else {
      if (start_size > 0) {
        uint8_t chi = *read++;
        start_size--;
        end_size--;
        switch (chi) {
          case '\\': che = '\\'; break;
          case 'a': che = '\a'; break;
          case 'b': che = '\b'; break;
          case 'e': che = '\e'; break;
          case 'f': che = '\f'; break;
          case 'n': che = '\n'; break;
          case 'r': che = '\r'; break;
          case 's': che = ' '; break;
          case 't': che = '\t'; break;
          case 'v': che = '\v'; break;

          default : {
            che = chi;
            *write++ = ch;
            end_size++;
          }
        }
        *write++ = che;
      }
    }
  }
  *size = end_size;
  return buffer;
}

char* RemoveSpace(char* p)
{
  char* write = p;
  char* read = p;
  char ch = '.';

  while (ch != '\0') {
    ch = *read++;
    if (!isspace(ch)) {
      *write++ = ch;
    }
  }
  *write = '\0';
  return p;
}

char* UpperCase(char* dest, const char* source)
{
  char* write = dest;
  const char* read = source;
  char ch = '.';

  while (ch != '\0') {
    ch = *read++;
    *write++ = toupper(ch);
  }
  return dest;
}

char* UpperCase_P(char* dest, const char* source)
{
  char* write = dest;
  const char* read = source;
  char ch = '.';

  while (ch != '\0') {
    ch = pgm_read_byte(read++);
    *write++ = toupper(ch);
  }
  return dest;
}

char* LTrim(char* p)
{
  while ((*p != '\0') && (isblank(*p))) {
    p++;
  }
  return p;
}

char* RTrim(char* p)
{
  char* q = p + strlen(p) -1;
  while ((q >= p) && (isblank(*q))) {
    q--;
  }
  q++;
  *q = '\0';
  return p;
}

char* Trim(char* p)
{
  if (*p == '\0') { return p; }
  while (isspace(*p)) { p++; }
  if (*p == '\0') { return p; }
  char* q = p + strlen(p) -1;
  while (isspace(*q) && q >= p) { q--; }
  q++;
  *q = '\0';
  return p;
}

char* NoAlNumToUnderscore(char* dest, const char* source)
{
  char* write = dest;
  const char* read = source;
  char ch = '.';

  while (ch != '\0') {
    ch = *read++;
    *write++ = (isalnum(ch) || ('\0' == ch)) ? ch : '_';
  }
  return dest;
}

void SetShortcut(char* str, uint8_t action)
{
  if ('\0' != str[0]) {
    str[0] = '0' + action;
    str[1] = '\0';
  }
}

uint8_t Shortcut(const char* str)
{
  uint8_t result = 10;

  if ('\0' == str[1]) {
    if (('"' == str[0]) || ('0' == str[0])) {
      result = SC_CLEAR;
    } else {
      result = atoi(str);
      if (0 == result) {
        result = 10;
      }
    }
  }
  return result;
}

boolean ParseIp(uint32_t* addr, const char* str)
{
  uint8_t *part = (uint8_t*)addr;
  byte i;

  *addr = 0;
  for (i = 0; i < 4; i++) {
    part[i] = strtoul(str, NULL, 10);
    str = strchr(str, '.');
    if (str == NULL || *str == '\0') {
      break;
    }
    str++;
  }
  return (3 == i);
}

void MakeValidMqtt(byte option, char* str)
{


  uint16_t i = 0;
  while (str[i] > 0) {

    if ((str[i] == '+') || (str[i] == '#') || (str[i] == ' ')) {
      if (option) {
        uint16_t j = i;
        while (str[j] > 0) {
          str[j] = str[j +1];
          j++;
        }
        i--;
      } else {
        str[i] = '_';
      }
    }
    i++;
  }
}


bool NewerVersion(char* version_str)
{
  uint32_t version = 0;
  uint8_t i = 0;
  char *str_ptr;
  char* version_dup = strdup(version_str);

  if (!version_dup) {
    return false;
  }

  for (char *str = strtok_r(version_dup, ".", &str_ptr); str && i < sizeof(VERSION); str = strtok_r(NULL, ".", &str_ptr), i++) {
    int field = atoi(str);

    if ((field < 0) || (field > 255)) {
      free(version_dup);
      return false;
    }

    version = (version << 8) + field;

    if ((2 == i) && isalpha(str[strlen(str)-1])) {
      field = str[strlen(str)-1] & 0x1f;
      version = (version << 8) + field;
      i++;
    }
  }
  free(version_dup);


  if ((i < 2) || (i > sizeof(VERSION))) {
    return false;
  }


  while (i < sizeof(VERSION)) {
    version <<= 8;
    i++;
  }

  return (version > VERSION);
}

char* GetPowerDevice(char* dest, uint8_t idx, size_t size, uint8_t option)
{
  char sidx[8];

  strncpy_P(dest, S_RSLT_POWER, size);
  if ((devices_present + option) > 1) {
    snprintf_P(sidx, sizeof(sidx), PSTR("%d"), idx);
    strncat(dest, sidx, size);
  }
  return dest;
}

char* GetPowerDevice(char* dest, uint8_t idx, size_t size)
{
  return GetPowerDevice(dest, idx, size, 0);
}

float ConvertTemp(float c)
{
  float result = c;

  if (!isnan(c) && Settings.flag.temperature_conversion) {
    result = c * 1.8 + 32;
  }
  return result;
}

char TempUnit()
{
  return (Settings.flag.temperature_conversion) ? 'F' : 'C';
}

void SetGlobalValues(float temperature, float humidity)
{
  global_update = uptime;
  global_temperature = temperature;
  global_humidity = humidity;
}

void ResetGlobalValues()
{
  if ((uptime - global_update) > GLOBAL_VALUES_VALID) {
    global_update = 0;
    global_temperature = 0;
    global_humidity = 0;
  }
}

double FastPrecisePow(double a, double b)
{


  int e = (int)b;
  union {
    double d;
    int x[2];
  } u = { a };
  u.x[1] = (int)((b - e) * (u.x[1] - 1072632447) + 1072632447);
  u.x[0] = 0;


  double r = 1.0;
  while (e) {
    if (e & 1) {
      r *= a;
    }
    a *= a;
    e >>= 1;
  }
  return r * u.d;
}

uint32_t SqrtInt(uint32_t num)
{
  if (num <= 1) {
    return num;
  }

  uint32_t x = num / 2;
  uint32_t y;
  do {
    y = (x + num / x) / 2;
    if (y >= x) {
      return x;
    }
    x = y;
  } while (true);
}

uint32_t RoundSqrtInt(uint32_t num)
{
  uint32_t s = SqrtInt(4 * num);
  if (s & 1) {
    s++;
  }
  return s / 2;
}

char* GetTextIndexed(char* destination, size_t destination_size, uint16_t index, const char* haystack)
{


  char* write = destination;
  const char* read = haystack;

  index++;
  while (index--) {
    size_t size = destination_size -1;
    write = destination;
    char ch = '.';
    while ((ch != '\0') && (ch != '|')) {
      ch = pgm_read_byte(read++);
      if (size && (ch != '|')) {
        *write++ = ch;
        size--;
      }
    }
    if (0 == ch) {
      if (index) {
        write = destination;
      }
      break;
    }
  }
  *write = '\0';
  return destination;
}

int GetCommandCode(char* destination, size_t destination_size, const char* needle, const char* haystack)
{


  int result = -1;
  const char* read = haystack;
  char* write = destination;

  while (true) {
    result++;
    size_t size = destination_size -1;
    write = destination;
    char ch = '.';
    while ((ch != '\0') && (ch != '|')) {
      ch = pgm_read_byte(read++);
      if (size && (ch != '|')) {
        *write++ = ch;
        size--;
      }
    }
    *write = '\0';
    if (!strcasecmp(needle, destination)) {
      break;
    }
    if (0 == ch) {
      result = -1;
      break;
    }
  }
  return result;
}

int GetStateNumber(char *state_text)
{
  char command[CMDSZ];
  int state_number = -1;

  if (GetCommandCode(command, sizeof(command), state_text, kOptionOff) >= 0) {
    state_number = 0;
  }
  else if (GetCommandCode(command, sizeof(command), state_text, kOptionOn) >= 0) {
    state_number = 1;
  }
  else if (GetCommandCode(command, sizeof(command), state_text, kOptionToggle) >= 0) {
    state_number = 2;
  }
  else if (GetCommandCode(command, sizeof(command), state_text, kOptionBlink) >= 0) {
    state_number = 3;
  }
  else if (GetCommandCode(command, sizeof(command), state_text, kOptionBlinkOff) >= 0) {
    state_number = 4;
  }
  return state_number;
}

boolean GetUsedInModule(byte val, uint8_t *arr)
{
  int offset = 0;

  if (!val) { return false; }
#ifndef USE_I2C
  if (GPIO_I2C_SCL == val) { return true; }
  if (GPIO_I2C_SDA == val) { return true; }
#endif
#ifndef USE_WS2812
  if (GPIO_WS2812 == val) { return true; }
#endif
#ifndef USE_IR_REMOTE
  if (GPIO_IRSEND == val) { return true; }
#ifndef USE_IR_RECEIVE
  if (GPIO_IRRECV == val) { return true; }
#endif
#endif
#ifndef USE_MHZ19
  if (GPIO_MHZ_TXD == val) { return true; }
  if (GPIO_MHZ_RXD == val) { return true; }
#endif

  int pzem = 3;
#ifndef USE_PZEM004T
  pzem--;
  if (GPIO_PZEM004_RX == val) { return true; }
#endif
#ifndef USE_PZEM_AC
  pzem--;
  if (GPIO_PZEM016_RX == val) { return true; }
#endif
#ifndef USE_PZEM_DC
  pzem--;
  if (GPIO_PZEM017_RX == val) { return true; }
#endif
  if (!pzem && (GPIO_PZEM0XX_TX == val)) { return true; }

#ifndef USE_SENSEAIR
  if (GPIO_SAIR_TX == val) { return true; }
  if (GPIO_SAIR_RX == val) { return true; }
#endif
#ifndef USE_SPI
  if (GPIO_SPI_CS == val) { return true; }
  if (GPIO_SPI_DC == val) { return true; }
#endif
#ifndef USE_DISPLAY
  if (GPIO_BACKLIGHT == val) { return true; }
#endif
#ifndef USE_PMS5003
  if (GPIO_PMS5003 == val) { return true; }
#endif
#ifndef USE_NOVA_SDS
  if (GPIO_SDS0X1_TX == val) { return true; }
  if (GPIO_SDS0X1_RX == val) { return true; }
#endif
#ifndef USE_SERIAL_BRIDGE
  if (GPIO_SBR_TX == val) { return true; }
  if (GPIO_SBR_RX == val) { return true; }
#endif
#ifndef USE_SR04
  if (GPIO_SR04_TRIG == val) { return true; }
  if (GPIO_SR04_ECHO == val) { return true; }
#endif
#ifndef USE_SDM120
  if (GPIO_SDM120_TX == val) { return true; }
  if (GPIO_SDM120_RX == val) { return true; }
#endif
#ifndef USE_SDM630
  if (GPIO_SDM630_TX == val) { return true; }
  if (GPIO_SDM630_RX == val) { return true; }
#endif
#ifndef USE_TM1638
  if (GPIO_TM16CLK == val) { return true; }
  if (GPIO_TM16DIO == val) { return true; }
  if (GPIO_TM16STB == val) { return true; }
#endif
#ifndef USE_HX711
  if (GPIO_HX711_SCK == val) { return true; }
  if (GPIO_HX711_DAT == val) { return true; }
#endif
#ifndef USE_TX20_WIND_SENSOR
  if (GPIO_TX20_TXD_BLACK == val) { return true; }
#endif
#ifndef USE_RC_SWITCH
  if (GPIO_RFSEND == val) { return true; }
  if (GPIO_RFRECV == val) { return true; }
#endif

  if ((val >= GPIO_REL1) && (val < GPIO_REL1 + MAX_RELAYS)) {
    offset = (GPIO_REL1_INV - GPIO_REL1);
  }
  if ((val >= GPIO_REL1_INV) && (val < GPIO_REL1_INV + MAX_RELAYS)) {
    offset = -(GPIO_REL1_INV - GPIO_REL1);
  }

  if ((val >= GPIO_LED1) && (val < GPIO_LED1 + MAX_LEDS)) {
    offset = (GPIO_LED1_INV - GPIO_LED1);
  }
  if ((val >= GPIO_LED1_INV) && (val < GPIO_LED1_INV + MAX_LEDS)) {
    offset = -(GPIO_LED1_INV - GPIO_LED1);
  }

  if ((val >= GPIO_PWM1) && (val < GPIO_PWM1 + MAX_PWMS)) {
    offset = (GPIO_PWM1_INV - GPIO_PWM1);
  }
  if ((val >= GPIO_PWM1_INV) && (val < GPIO_PWM1_INV + MAX_PWMS)) {
    offset = -(GPIO_PWM1_INV - GPIO_PWM1);
  }
  for (byte i = 0; i < MAX_GPIO_PIN; i++) {
    if (arr[i] == val) { return true; }
    if (arr[i] == val + offset) { return true; }
  }
  return false;
}

void SetSerialBaudrate(int baudrate)
{
  Settings.baudrate = baudrate / 1200;
  if (Serial.baudRate() != baudrate) {
    if (seriallog_level) {
      snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_APPLICATION D_SET_BAUDRATE_TO " %d"), baudrate);
      AddLog(LOG_LEVEL_INFO);
    }
    delay(100);
    Serial.flush();
    Serial.begin(baudrate, serial_config);
    delay(10);
    Serial.println();
  }
}

void ClaimSerial()
{
  serial_local = 1;
  AddLog_P(LOG_LEVEL_INFO, PSTR("SNS: Hardware Serial"));
  SetSeriallog(LOG_LEVEL_NONE);
  baudrate = Serial.baudRate();
  Settings.baudrate = baudrate / 1200;
}

void SerialSendRaw(char *codes)
{
  char *p;
  char stemp[3];
  uint8_t code;

  int size = strlen(codes);

  while (size > 0) {
    snprintf(stemp, sizeof(stemp), codes);
    code = strtol(stemp, &p, 16);
    Serial.write(code);
    size -= 2;
    codes += 2;
  }
}

uint32_t GetHash(const char *buffer, size_t size)
{
  uint32_t hash = 0;
  for (uint16_t i = 0; i <= size; i++) {
    hash += (uint8_t)*buffer++ * (i +1);
  }
  return hash;
}

void ShowSource(int source)
{
  if ((source > 0) && (source < SRC_MAX)) {
    char stemp1[20];
    snprintf_P(log_data, sizeof(log_data), PSTR("SRC: %s"), GetTextIndexed(stemp1, sizeof(stemp1), source, kCommandSource));
    AddLog(LOG_LEVEL_DEBUG);
  }
}

uint8_t ValidGPIO(uint8_t pin, uint8_t gpio)
{
  uint8_t result = gpio;
  if ((WEMOS == Settings.module) && (!Settings.flag3.user_esp8285_enable)) {
    if ((pin == 9) || (pin == 10)) { result = GPIO_NONE; }
  }
  return result;
}





long TimeDifference(unsigned long prev, unsigned long next)
{



  long signed_diff = 0;

  const unsigned long half_max_unsigned_long = 2147483647u;
  if (next >= prev) {
    const unsigned long diff = next - prev;
    if (diff <= half_max_unsigned_long) {
      signed_diff = static_cast<long>(diff);
    } else {

      signed_diff = static_cast<long>((0xffffffffUL - next) + prev + 1u);
      signed_diff = -1 * signed_diff;
    }
  } else {

    const unsigned long diff = prev - next;
    if (diff <= half_max_unsigned_long) {
      signed_diff = static_cast<long>(diff);
      signed_diff = -1 * signed_diff;
    } else {

      signed_diff = static_cast<long>((0xffffffffUL - prev) + next + 1u);
    }
  }
  return signed_diff;
}

long TimePassedSince(unsigned long timestamp)
{


  return TimeDifference(timestamp, millis());
}

bool TimeReached(unsigned long timer)
{

  const long passed = TimePassedSince(timer);
  return (passed >= 0);
}

void SetNextTimeInterval(unsigned long& timer, const unsigned long step)
{
  timer += step;
  const long passed = TimePassedSince(timer);
  if (passed < 0) { return; }
  if (static_cast<unsigned long>(passed) > step) {

    timer = millis() + step;
    return;
  }

  timer = millis() + (step - passed);
}





void GetFeatures()
{
  feature_drv1 = 0x00000000;




#ifdef USE_I2C
  feature_drv1 |= 0x00000004;
#endif
#ifdef USE_SPI
  feature_drv1 |= 0x00000008;
#endif
#ifdef USE_DISCOVERY
  feature_drv1 |= 0x00000010;
#endif
#ifdef USE_ARDUINO_OTA
  feature_drv1 |= 0x00000020;
#endif
#ifdef USE_MQTT_TLS
  feature_drv1 |= 0x00000040;
#endif
#ifdef USE_WEBSERVER
  feature_drv1 |= 0x00000080;
#endif
#ifdef WEBSERVER_ADVERTISE
  feature_drv1 |= 0x00000100;
#endif
#ifdef USE_EMULATION
  feature_drv1 |= 0x00000200;
#endif
#if (MQTT_LIBRARY_TYPE == MQTT_PUBSUBCLIENT)
  feature_drv1 |= 0x00000400;
#endif
#if (MQTT_LIBRARY_TYPE == MQTT_TASMOTAMQTT)
  feature_drv1 |= 0x00000800;
#endif
#if (MQTT_LIBRARY_TYPE == MQTT_ESPMQTTARDUINO)
  feature_drv1 |= 0x00001000;
#endif
#ifdef MQTT_HOST_DISCOVERY
  feature_drv1 |= 0x00002000;
#endif
#ifdef USE_ARILUX_RF
  feature_drv1 |= 0x00004000;
#endif
#ifdef USE_WS2812
  feature_drv1 |= 0x00008000;
#endif
#ifdef USE_WS2812_DMA
  feature_drv1 |= 0x00010000;
#endif
#ifdef USE_IR_REMOTE
  feature_drv1 |= 0x00020000;
#endif
#ifdef USE_IR_HVAC
  feature_drv1 |= 0x00040000;
#endif
#ifdef USE_IR_RECEIVE
  feature_drv1 |= 0x00080000;
#endif
#ifdef USE_DOMOTICZ
  feature_drv1 |= 0x00100000;
#endif
#ifdef USE_DISPLAY
  feature_drv1 |= 0x00200000;
#endif
#ifdef USE_HOME_ASSISTANT
  feature_drv1 |= 0x00400000;
#endif
#ifdef USE_SERIAL_BRIDGE
  feature_drv1 |= 0x00800000;
#endif
#ifdef USE_TIMERS
  feature_drv1 |= 0x01000000;
#endif
#ifdef USE_SUNRISE
  feature_drv1 |= 0x02000000;
#endif
#ifdef USE_TIMERS_WEB
  feature_drv1 |= 0x04000000;
#endif
#ifdef USE_RULES
  feature_drv1 |= 0x08000000;
#endif
#ifdef USE_KNX
  feature_drv1 |= 0x10000000;
#endif
#ifdef USE_WPS
  feature_drv1 |= 0x20000000;
#endif
#ifdef USE_SMARTCONFIG
  feature_drv1 |= 0x40000000;
#endif
#if (MQTT_LIBRARY_TYPE == MQTT_ARDUINOMQTT)
  feature_drv1 |= 0x80000000;
#endif



  feature_drv2 = 0x00000000;

#ifdef USE_CONFIG_OVERRIDE
  feature_drv2 |= 0x00000001;
#endif
#ifdef BE_MINIMAL
  feature_drv2 |= 0x00000002;
#endif
#ifdef USE_SENSORS
  feature_drv2 |= 0x00000004;
#endif
#ifdef USE_CLASSIC
  feature_drv2 |= 0x00000008;
#endif
#ifdef USE_KNX_NO_EMULATION
  feature_drv2 |= 0x00000010;
#endif
#ifdef USE_DISPLAY_MODES1TO5
  feature_drv2 |= 0x00000020;
#endif
#ifdef USE_DISPLAY_GRAPH
  feature_drv2 |= 0x00000040;
#endif
#ifdef USE_DISPLAY_LCD
  feature_drv2 |= 0x00000080;
#endif
#ifdef USE_DISPLAY_SSD1306
  feature_drv2 |= 0x00000100;
#endif
#ifdef USE_DISPLAY_MATRIX
  feature_drv2 |= 0x00000200;
#endif
#ifdef USE_DISPLAY_ILI9341
  feature_drv2 |= 0x00000400;
#endif
#ifdef USE_DISPLAY_EPAPER
  feature_drv2 |= 0x00000800;
#endif
#ifdef USE_DISPLAY_SH1106
  feature_drv2 |= 0x00001000;
#endif
#ifdef USE_MP3_PLAYER
  feature_drv2 |= 0x00002000;
#endif
#ifdef USE_PCA9685
  feature_drv2 |= 0x00004000;
#endif
#ifdef USE_TUYA_DIMMER
  feature_drv2 |= 0x00008000;
#endif
#ifdef USE_RC_SWITCH
  feature_drv2 |= 0x00010000;
#endif



#ifdef NO_EXTRA_4K_HEAP
  feature_drv2 |= 0x00800000;
#endif
#ifdef VTABLES_IN_IRAM
  feature_drv2 |= 0x01000000;
#endif
#ifdef VTABLES_IN_DRAM
  feature_drv2 |= 0x02000000;
#endif
#ifdef VTABLES_IN_FLASH
  feature_drv2 |= 0x04000000;
#endif
#ifdef PIO_FRAMEWORK_ARDUINO_LWIP_HIGHER_BANDWIDTH
  feature_drv2 |= 0x08000000;
#endif
#ifdef PIO_FRAMEWORK_ARDUINO_LWIP2_LOW_MEMORY
  feature_drv2 |= 0x10000000;
#endif
#ifdef PIO_FRAMEWORK_ARDUINO_LWIP2_HIGHER_BANDWIDTH
  feature_drv2 |= 0x20000000;
#endif
#ifdef DEBUG_THEO
  feature_drv2 |= 0x40000000;
#endif
#ifdef USE_DEBUG_DRIVER
  feature_drv2 |= 0x80000000;
#endif



  feature_sns1 = 0x00000000;



#ifdef USE_ADC_VCC
  feature_sns1 |= 0x00000002;
#endif
#ifdef USE_ENERGY_SENSOR
  feature_sns1 |= 0x00000004;
#endif
#ifdef USE_PZEM004T
  feature_sns1 |= 0x00000008;
#endif
#ifdef USE_DS18B20
  feature_sns1 |= 0x00000010;
#endif
#ifdef USE_DS18x20_LEGACY
  feature_sns1 |= 0x00000020;
#endif
#ifdef USE_DS18x20
  feature_sns1 |= 0x00000040;
#endif
#ifdef USE_DHT
  feature_sns1 |= 0x00000080;
#endif
#ifdef USE_SHT
  feature_sns1 |= 0x00000100;
#endif
#ifdef USE_HTU
  feature_sns1 |= 0x00000200;
#endif
#ifdef USE_BMP
  feature_sns1 |= 0x00000400;
#endif
#ifdef USE_BME680
  feature_sns1 |= 0x00000800;
#endif
#ifdef USE_BH1750
  feature_sns1 |= 0x00001000;
#endif
#ifdef USE_VEML6070
  feature_sns1 |= 0x00002000;
#endif
#ifdef USE_ADS1115_I2CDEV
  feature_sns1 |= 0x00004000;
#endif
#ifdef USE_ADS1115
  feature_sns1 |= 0x00008000;
#endif
#ifdef USE_INA219
  feature_sns1 |= 0x00010000;
#endif
#ifdef USE_SHT3X
  feature_sns1 |= 0x00020000;
#endif
#ifdef USE_MHZ19
  feature_sns1 |= 0x00040000;
#endif
#ifdef USE_TSL2561
  feature_sns1 |= 0x00080000;
#endif
#ifdef USE_SENSEAIR
  feature_sns1 |= 0x00100000;
#endif
#ifdef USE_PMS5003
  feature_sns1 |= 0x00200000;
#endif
#ifdef USE_MGS
  feature_sns1 |= 0x00400000;
#endif
#ifdef USE_NOVA_SDS
  feature_sns1 |= 0x00800000;
#endif
#ifdef USE_SGP30
  feature_sns1 |= 0x01000000;
#endif
#ifdef USE_SR04
  feature_sns1 |= 0x02000000;
#endif
#ifdef USE_SDM120
  feature_sns1 |= 0x04000000;
#endif
#ifdef USE_SI1145
  feature_sns1 |= 0x08000000;
#endif
#ifdef USE_SDM630
  feature_sns1 |= 0x10000000;
#endif
#ifdef USE_LM75AD
  feature_sns1 |= 0x20000000;
#endif
#ifdef USE_APDS9960
  feature_sns1 |= 0x40000000;
#endif
#ifdef USE_TM1638
  feature_sns1 |= 0x80000000;
#endif



  feature_sns2 = 0x00000000;

#ifdef USE_MCP230xx
  feature_sns2 |= 0x00000001;
#endif
#ifdef USE_MPR121
  feature_sns2 |= 0x00000002;
#endif
#ifdef USE_CCS811
  feature_sns2 |= 0x00000004;
#endif
#ifdef USE_MPU6050
  feature_sns2 |= 0x00000008;
#endif
#ifdef USE_MCP230xx_OUTPUT
  feature_sns2 |= 0x00000010;
#endif
#ifdef USE_MCP230xx_DISPLAYOUTPUT
  feature_sns2 |= 0x00000020;
#endif
#ifdef USE_HLW8012
  feature_sns2 |= 0x00000040;
#endif
#ifdef USE_CSE7766
  feature_sns2 |= 0x00000080;
#endif
#ifdef USE_MCP39F501
  feature_sns2 |= 0x00000100;
#endif
#ifdef USE_PZEM_AC
  feature_sns2 |= 0x00000200;
#endif
#ifdef USE_DS3231
  feature_sns2 |= 0x00000400;
#endif
#ifdef USE_HX711
  feature_sns2 |= 0x00000800;
#endif
#ifdef USE_PZEM_DC
  feature_sns2 |= 0x00001000;
#endif
#ifdef USE_TX20_WIND_SENSOR
  feature_sns2 |= 0x00002000;
#endif



}





#define WIFI_CONFIG_SEC 180
#define WIFI_CHECK_SEC 20
#define WIFI_RETRY_OFFSET_SEC 20

uint8_t wifi_counter;
uint8_t wifi_retry_init;
uint8_t wifi_retry;
uint8_t wifi_status;
uint8_t wps_result;
uint8_t wifi_config_type = 0;
uint8_t wifi_config_counter = 0;

int WifiGetRssiAsQuality(int rssi)
{
  int quality = 0;

  if (rssi <= -100) {
    quality = 0;
  } else if (rssi >= -50) {
    quality = 100;
  } else {
    quality = 2 * (rssi + 100);
  }
  return quality;
}

boolean WifiConfigCounter()
{
  if (wifi_config_counter) {
    wifi_config_counter = WIFI_CONFIG_SEC;
  }
  return (wifi_config_counter);
}

extern "C" {
#include "user_interface.h"
}

void WifiWpsStatusCallback(wps_cb_status status);

void WifiWpsStatusCallback(wps_cb_status status)
{
# 1271 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/support.ino"
  wps_result = status;
  if (WPS_CB_ST_SUCCESS == wps_result) {
    wifi_wps_disable();
  } else {
    snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_WIFI D_WPS_FAILED_WITH_STATUS " %d"), wps_result);
    AddLog(LOG_LEVEL_DEBUG);
    wifi_config_counter = 2;
  }
}

boolean WifiWpsConfigDone(void)
{
  return (!wps_result);
}

boolean WifiWpsConfigBegin(void)
{
  wps_result = 99;
  if (!wifi_wps_disable()) { return false; }
  if (!wifi_wps_enable(WPS_TYPE_PBC)) { return false; }
  if (!wifi_set_wps_cb((wps_st_cb_t) &WifiWpsStatusCallback)) { return false; }
  if (!wifi_wps_start()) { return false; }
  return true;
}

void WifiConfig(uint8_t type)
{
  if (!wifi_config_type) {
    if ((WIFI_RETRY == type) || (WIFI_WAIT == type)) { return; }
#if defined(USE_WEBSERVER) && defined(USE_EMULATION)
    UdpDisconnect();
#endif
    WiFi.disconnect();
    wifi_config_type = type;

#ifndef USE_WPS
    if (WIFI_WPSCONFIG == wifi_config_type) { wifi_config_type = WIFI_MANAGER; }
#endif
#ifndef USE_WEBSERVER
    if (WIFI_MANAGER == wifi_config_type) { wifi_config_type = WIFI_SMARTCONFIG; }
#endif
#ifndef USE_SMARTCONFIG
    if (WIFI_SMARTCONFIG == wifi_config_type) { wifi_config_type = WIFI_SERIAL; }
#endif

    wifi_config_counter = WIFI_CONFIG_SEC;
    wifi_counter = wifi_config_counter +5;
    blinks = 1999;
    if (WIFI_RESTART == wifi_config_type) {
      restart_flag = 2;
    }
    else if (WIFI_SERIAL == wifi_config_type) {
      AddLog_P(LOG_LEVEL_INFO, S_LOG_WIFI, PSTR(D_WCFG_6_SERIAL " " D_ACTIVE_FOR_3_MINUTES));
    }
#ifdef USE_SMARTCONFIG
    else if (WIFI_SMARTCONFIG == wifi_config_type) {
      AddLog_P(LOG_LEVEL_INFO, S_LOG_WIFI, PSTR(D_WCFG_1_SMARTCONFIG " " D_ACTIVE_FOR_3_MINUTES));
      WiFi.beginSmartConfig();
    }
#endif
#ifdef USE_WPS
    else if (WIFI_WPSCONFIG == wifi_config_type) {
      if (WifiWpsConfigBegin()) {
        AddLog_P(LOG_LEVEL_INFO, S_LOG_WIFI, PSTR(D_WCFG_3_WPSCONFIG " " D_ACTIVE_FOR_3_MINUTES));
      } else {
        AddLog_P(LOG_LEVEL_INFO, S_LOG_WIFI, PSTR(D_WCFG_3_WPSCONFIG " " D_FAILED_TO_START));
        wifi_config_counter = 3;
      }
    }
#endif
#ifdef USE_WEBSERVER
    else if (WIFI_MANAGER == wifi_config_type) {
      AddLog_P(LOG_LEVEL_INFO, S_LOG_WIFI, PSTR(D_WCFG_2_WIFIMANAGER " " D_ACTIVE_FOR_3_MINUTES));
      WifiManagerBegin();
    }
#endif
  }
}

void WiFiSetSleepMode()
{
# 1365 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/support.ino"
#if defined(ARDUINO_ESP8266_RELEASE_2_4_1) || defined(ARDUINO_ESP8266_RELEASE_2_4_2)
#else
  if (sleep) {
    WiFi.setSleepMode(WIFI_LIGHT_SLEEP);
  } else {
    WiFi.setSleepMode(WIFI_MODEM_SLEEP);
  }
#endif
}

void WifiBegin(uint8_t flag)
{
  const char kWifiPhyMode[] = " BGN";

#if defined(USE_WEBSERVER) && defined(USE_EMULATION)
  UdpDisconnect();
#endif

#ifdef ARDUINO_ESP8266_RELEASE_2_3_0
  AddLog_P(LOG_LEVEL_DEBUG, S_LOG_WIFI, PSTR(D_PATCH_ISSUE_2186));
  WiFi.mode(WIFI_OFF);
#endif

  WiFi.persistent(false);
  WiFi.disconnect(true);
  delay(200);
  WiFi.mode(WIFI_STA);
  WiFiSetSleepMode();

  if (!WiFi.getAutoConnect()) { WiFi.setAutoConnect(true); }

  switch (flag) {
  case 0:
  case 1:
    Settings.sta_active = flag;
    break;
  case 2:
    Settings.sta_active ^= 1;
  }
  if ('\0' == Settings.sta_ssid[Settings.sta_active][0]) { Settings.sta_active ^= 1; }
  if (Settings.ip_address[0]) {
    WiFi.config(Settings.ip_address[0], Settings.ip_address[1], Settings.ip_address[2], Settings.ip_address[3]);
  }
  WiFi.hostname(my_hostname);
  WiFi.begin(Settings.sta_ssid[Settings.sta_active], Settings.sta_pwd[Settings.sta_active]);
  snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_WIFI D_CONNECTING_TO_AP "%d %s " D_IN_MODE " 11%c " D_AS " %s..."),
    Settings.sta_active +1, Settings.sta_ssid[Settings.sta_active], kWifiPhyMode[WiFi.getPhyMode() & 0x3], my_hostname);
  AddLog(LOG_LEVEL_INFO);
}

void WifiState(uint8_t state)
{
  if (state == global_state.wifi_down) {
    if (state) {
      rules_flag.wifi_connected = 1;
    } else {
      rules_flag.wifi_disconnected = 1;
    }
  }
  global_state.wifi_down = state ^1;
}

void WifiCheckIp()
{
  if ((WL_CONNECTED == WiFi.status()) && (static_cast<uint32_t>(WiFi.localIP()) != 0)) {
    WifiState(1);
    wifi_counter = WIFI_CHECK_SEC;
    wifi_retry = wifi_retry_init;
    AddLog_P((wifi_status != WL_CONNECTED) ? LOG_LEVEL_INFO : LOG_LEVEL_DEBUG_MORE, S_LOG_WIFI, PSTR(D_CONNECTED));
    if (wifi_status != WL_CONNECTED) {

      Settings.ip_address[1] = (uint32_t)WiFi.gatewayIP();
      Settings.ip_address[2] = (uint32_t)WiFi.subnetMask();
      Settings.ip_address[3] = (uint32_t)WiFi.dnsIP();
    }
    wifi_status = WL_CONNECTED;
  } else {
    WifiState(0);
    uint8_t wifi_config_tool = Settings.sta_config;
    wifi_status = WiFi.status();
    switch (wifi_status) {
      case WL_CONNECTED:
        AddLog_P(LOG_LEVEL_INFO, S_LOG_WIFI, PSTR(D_CONNECT_FAILED_NO_IP_ADDRESS));
        wifi_status = 0;
        wifi_retry = wifi_retry_init;
        break;
      case WL_NO_SSID_AVAIL:
        AddLog_P(LOG_LEVEL_INFO, S_LOG_WIFI, PSTR(D_CONNECT_FAILED_AP_NOT_REACHED));
        if (WIFI_WAIT == Settings.sta_config) {
          wifi_retry = wifi_retry_init;
        } else {
          if (wifi_retry > (wifi_retry_init / 2)) {
            wifi_retry = wifi_retry_init / 2;
          }
          else if (wifi_retry) {
            wifi_retry = 0;
          }
        }
        break;
      case WL_CONNECT_FAILED:
        AddLog_P(LOG_LEVEL_INFO, S_LOG_WIFI, PSTR(D_CONNECT_FAILED_WRONG_PASSWORD));
        if (wifi_retry > (wifi_retry_init / 2)) {
          wifi_retry = wifi_retry_init / 2;
        }
        else if (wifi_retry) {
          wifi_retry = 0;
        }
        break;
      default:
        if (!wifi_retry || ((wifi_retry_init / 2) == wifi_retry)) {
          AddLog_P(LOG_LEVEL_INFO, S_LOG_WIFI, PSTR(D_CONNECT_FAILED_AP_TIMEOUT));
        } else {
          if (('\0' == Settings.sta_ssid[0][0]) && ('\0' == Settings.sta_ssid[1][0])) {
            wifi_config_tool = WIFI_CONFIG_NO_SSID;
            wifi_retry = 0;
          } else {
            AddLog_P(LOG_LEVEL_DEBUG, S_LOG_WIFI, PSTR(D_ATTEMPTING_CONNECTION));
          }
        }
    }
    if (wifi_retry) {
      if (wifi_retry_init == wifi_retry) {
        WifiBegin(3);
      }
      if ((Settings.sta_config != WIFI_WAIT) && ((wifi_retry_init / 2) == wifi_retry)) {
        WifiBegin(2);
      }
      wifi_counter = 1;
      wifi_retry--;
    } else {
      WifiConfig(wifi_config_tool);
      wifi_counter = 1;
      wifi_retry = wifi_retry_init;
    }
  }
}

void WifiCheck(uint8_t param)
{
  wifi_counter--;
  switch (param) {
  case WIFI_SERIAL:
  case WIFI_SMARTCONFIG:
  case WIFI_MANAGER:
  case WIFI_WPSCONFIG:
    WifiConfig(param);
    break;
  default:
    if (wifi_config_counter) {
      wifi_config_counter--;
      wifi_counter = wifi_config_counter +5;
      if (wifi_config_counter) {
#ifdef USE_SMARTCONFIG
        if ((WIFI_SMARTCONFIG == wifi_config_type) && WiFi.smartConfigDone()) {
          wifi_config_counter = 0;
        }
#endif
#ifdef USE_WPS
        if ((WIFI_WPSCONFIG == wifi_config_type) && WifiWpsConfigDone()) {
          wifi_config_counter = 0;
        }
#endif
        if (!wifi_config_counter) {
          if (strlen(WiFi.SSID().c_str())) {
            strlcpy(Settings.sta_ssid[0], WiFi.SSID().c_str(), sizeof(Settings.sta_ssid[0]));
          }
          if (strlen(WiFi.psk().c_str())) {
            strlcpy(Settings.sta_pwd[0], WiFi.psk().c_str(), sizeof(Settings.sta_pwd[0]));
          }
          Settings.sta_active = 0;
          snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_WIFI D_WCFG_1_SMARTCONFIG D_CMND_SSID "1 %s"), Settings.sta_ssid[0]);
          AddLog(LOG_LEVEL_INFO);
        }
      }
      if (!wifi_config_counter) {
#ifdef USE_SMARTCONFIG
        if (WIFI_SMARTCONFIG == wifi_config_type) { WiFi.stopSmartConfig(); }
#endif

        restart_flag = 2;
      }
    } else {
      if (wifi_counter <= 0) {
        AddLog_P(LOG_LEVEL_DEBUG_MORE, S_LOG_WIFI, PSTR(D_CHECKING_CONNECTION));
        wifi_counter = WIFI_CHECK_SEC;
        WifiCheckIp();
      }
      if ((WL_CONNECTED == WiFi.status()) && (static_cast<uint32_t>(WiFi.localIP()) != 0) && !wifi_config_type) {
        WifiState(1);
#ifdef BE_MINIMAL
        if (1 == RtcSettings.ota_loader) {
          RtcSettings.ota_loader = 0;
          ota_state_flag = 3;
        }
#endif

#ifdef USE_DISCOVERY
        if (!mdns_begun) {
          if (mdns_delayed_start) {
            AddLog_P(LOG_LEVEL_INFO, PSTR(D_LOG_MDNS D_ATTEMPTING_CONNECTION));
            mdns_delayed_start--;
          } else {
            mdns_delayed_start = Settings.param[P_MDNS_DELAYED_START];
            mdns_begun = MDNS.begin(my_hostname);
            snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_MDNS "%s"), (mdns_begun) ? D_INITIALIZED : D_FAILED);
            AddLog(LOG_LEVEL_INFO);
          }
        }
#endif

#ifdef USE_WEBSERVER
        if (Settings.webserver) {
          StartWebserver(Settings.webserver, WiFi.localIP());
#ifdef USE_DISCOVERY
#ifdef WEBSERVER_ADVERTISE
          if (mdns_begun) {
            MDNS.addService("http", "tcp", WEB_PORT);
          }
#endif
#endif
        } else {
          StopWebserver();
        }
#ifdef USE_EMULATION
        if (Settings.flag2.emulation) { UdpConnect(); }
#endif
#endif

#ifdef USE_KNX
        if (!knx_started && Settings.flag.knx_enabled) {
          KNXStart();
          knx_started = true;
        }
#endif

      } else {
        WifiState(0);
#if defined(USE_WEBSERVER) && defined(USE_EMULATION)
        UdpDisconnect();
#endif
        mdns_begun = false;
#ifdef USE_KNX
        knx_started = false;
#endif
      }
    }
  }
}

int WifiState()
{
  int state;

  if ((WL_CONNECTED == WiFi.status()) && (static_cast<uint32_t>(WiFi.localIP()) != 0)) {
    state = WIFI_RESTART;
  }
  if (wifi_config_type) { state = wifi_config_type; }
  return state;
}

void WifiConnect()
{
  WiFi.persistent(false);
  wifi_status = 0;
  wifi_retry_init = WIFI_RETRY_OFFSET_SEC + ((ESP.getChipId() & 0xF) * 2);
  wifi_retry = wifi_retry_init;
  wifi_counter = 1;
}
# 1655 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/support.ino"
void EspRestart()
{
  ESP.restart();
}





#ifdef USE_I2C
#define I2C_RETRY_COUNTER 3

uint32_t i2c_buffer = 0;

bool I2cValidRead(uint8_t addr, uint8_t reg, uint8_t size)
{
  byte x = I2C_RETRY_COUNTER;

  i2c_buffer = 0;
  do {
    Wire.beginTransmission(addr);
    Wire.write(reg);
    if (0 == Wire.endTransmission(false)) {
      Wire.requestFrom((int)addr, (int)size);
      if (Wire.available() == size) {
        for (byte i = 0; i < size; i++) {
          i2c_buffer = i2c_buffer << 8 | Wire.read();
        }
      }
    }
    x--;
  } while (Wire.endTransmission(true) != 0 && x != 0);
  return (x);
}

bool I2cValidRead8(uint8_t *data, uint8_t addr, uint8_t reg)
{
  bool status = I2cValidRead(addr, reg, 1);
  *data = (uint8_t)i2c_buffer;
  return status;
}

bool I2cValidRead16(uint16_t *data, uint8_t addr, uint8_t reg)
{
  bool status = I2cValidRead(addr, reg, 2);
  *data = (uint16_t)i2c_buffer;
  return status;
}

bool I2cValidReadS16(int16_t *data, uint8_t addr, uint8_t reg)
{
  bool status = I2cValidRead(addr, reg, 2);
  *data = (int16_t)i2c_buffer;
  return status;
}

bool I2cValidRead16LE(uint16_t *data, uint8_t addr, uint8_t reg)
{
  uint16_t ldata;
  bool status = I2cValidRead16(&ldata, addr, reg);
  *data = (ldata >> 8) | (ldata << 8);
  return status;
}

bool I2cValidReadS16_LE(int16_t *data, uint8_t addr, uint8_t reg)
{
  uint16_t ldata;
  bool status = I2cValidRead16LE(&ldata, addr, reg);
  *data = (int16_t)ldata;
  return status;
}

bool I2cValidRead24(int32_t *data, uint8_t addr, uint8_t reg)
{
  bool status = I2cValidRead(addr, reg, 3);
  *data = i2c_buffer;
  return status;
}

uint8_t I2cRead8(uint8_t addr, uint8_t reg)
{
  I2cValidRead(addr, reg, 1);
  return (uint8_t)i2c_buffer;
}

uint16_t I2cRead16(uint8_t addr, uint8_t reg)
{
  I2cValidRead(addr, reg, 2);
  return (uint16_t)i2c_buffer;
}

int16_t I2cReadS16(uint8_t addr, uint8_t reg)
{
  I2cValidRead(addr, reg, 2);
  return (int16_t)i2c_buffer;
}

uint16_t I2cRead16LE(uint8_t addr, uint8_t reg)
{
  I2cValidRead(addr, reg, 2);
  uint16_t temp = (uint16_t)i2c_buffer;
  return (temp >> 8) | (temp << 8);
}

int16_t I2cReadS16_LE(uint8_t addr, uint8_t reg)
{
  return (int16_t)I2cRead16LE(addr, reg);
}

int32_t I2cRead24(uint8_t addr, uint8_t reg)
{
  I2cValidRead(addr, reg, 3);
  return i2c_buffer;
}

bool I2cWrite(uint8_t addr, uint8_t reg, uint32_t val, uint8_t size)
{
  byte x = I2C_RETRY_COUNTER;

  do {
    Wire.beginTransmission((uint8_t)addr);
    Wire.write(reg);
    uint8_t bytes = size;
    while (bytes--) {
      Wire.write((val >> (8 * bytes)) & 0xFF);
    }
    x--;
  } while (Wire.endTransmission(true) != 0 && x != 0);
  return (x);
}

bool I2cWrite8(uint8_t addr, uint8_t reg, uint16_t val)
{
   return I2cWrite(addr, reg, val, 1);
}

bool I2cWrite16(uint8_t addr, uint8_t reg, uint16_t val)
{
   return I2cWrite(addr, reg, val, 2);
}

int8_t I2cReadBuffer(uint8_t addr, uint8_t reg, uint8_t *reg_data, uint16_t len)
{
  Wire.beginTransmission((uint8_t)addr);
  Wire.write((uint8_t)reg);
  Wire.endTransmission();
  if (len != Wire.requestFrom((uint8_t)addr, (byte)len)) {
    return 1;
  }
  while (len--) {
    *reg_data = (uint8_t)Wire.read();
    reg_data++;
  }
  return 0;
}

int8_t I2cWriteBuffer(uint8_t addr, uint8_t reg, uint8_t *reg_data, uint16_t len)
{
  Wire.beginTransmission((uint8_t)addr);
  Wire.write((uint8_t)reg);
  while (len--) {
    Wire.write(*reg_data);
    reg_data++;
  }
  Wire.endTransmission();
  return 0;
}

void I2cScan(char *devs, unsigned int devs_len)
{







  byte error = 0;
  byte address = 0;
  byte any = 0;

  snprintf_P(devs, devs_len, PSTR("{\"" D_CMND_I2CSCAN "\":\"" D_JSON_I2CSCAN_DEVICES_FOUND_AT));
  for (address = 1; address <= 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (0 == error) {
      any = 1;
      snprintf_P(devs, devs_len, PSTR("%s 0x%02x"), devs, address);
    }
    else if (error != 2) {
      any = 2;
      snprintf_P(devs, devs_len, PSTR("{\"" D_CMND_I2CSCAN "\":\"Error %d at 0x%02x"), error, address);
      break;
    }
  }
  if (any) {
    strncat(devs, "\"}", devs_len);
  }
  else {
    snprintf_P(devs, devs_len, PSTR("{\"" D_CMND_I2CSCAN "\":\"" D_JSON_I2CSCAN_NO_DEVICES_FOUND "\"}"));
  }
}

boolean I2cDevice(byte addr)
{
  for (byte address = 1; address <= 127; address++) {
    Wire.beginTransmission(address);
    if (!Wire.endTransmission() && (address == addr)) {
      return true;
    }
  }
  return false;
}
#endif
# 1877 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/support.ino"
extern "C" {
#include "sntp.h"
}

#define SECS_PER_MIN ((uint32_t)(60UL))
#define SECS_PER_HOUR ((uint32_t)(3600UL))
#define SECS_PER_DAY ((uint32_t)(SECS_PER_HOUR * 24UL))
#define LEAP_YEAR(Y) (((1970+Y)>0) && !((1970+Y)%4) && (((1970+Y)%100) || !((1970+Y)%400)))

Ticker TickerRtc;

static const uint8_t kDaysInMonth[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
static const char kMonthNamesEnglish[] = "JanFebMarAprMayJunJulAugSepOctNovDec";

uint32_t utc_time = 0;
uint32_t local_time = 0;
uint32_t daylight_saving_time = 0;
uint32_t standard_time = 0;
uint32_t ntp_time = 0;
uint32_t midnight = 1451602800;
uint32_t restart_time = 0;
int16_t time_timezone = 0;
uint8_t midnight_now = 0;
uint8_t ntp_sync_minute = 0;

String GetBuildDateAndTime()
{

  char bdt[21];
  char *p;
  char mdate[] = __DATE__;
  char *smonth = mdate;
  int day = 0;
  int year = 0;


  byte i = 0;
  for (char *str = strtok_r(mdate, " ", &p); str && i < 3; str = strtok_r(NULL, " ", &p)) {
    switch (i++) {
    case 0:
      smonth = str;
      break;
    case 1:
      day = atoi(str);
      break;
    case 2:
      year = atoi(str);
    }
  }
  int month = (strstr(kMonthNamesEnglish, smonth) -kMonthNamesEnglish) /3 +1;
  snprintf_P(bdt, sizeof(bdt), PSTR("%d" D_YEAR_MONTH_SEPARATOR "%02d" D_MONTH_DAY_SEPARATOR "%02d" D_DATE_TIME_SEPARATOR "%s"), year, month, day, __TIME__);
  return String(bdt);
}
# 1942 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/support.ino"
String GetDateAndTime(byte time_type)
{

  char dt[27];
  TIME_T tmpTime;

  switch (time_type) {
    case DT_ENERGY:
      BreakTime(Settings.energy_kWhtotal_time, tmpTime);
      tmpTime.year += 1970;
      break;
    case DT_UTC:
      BreakTime(utc_time, tmpTime);
      tmpTime.year += 1970;
      break;
    case DT_RESTART:
      if (restart_time == 0) {
        return "";
      }
      BreakTime(restart_time, tmpTime);
      tmpTime.year += 1970;
      break;
    default:
      tmpTime = RtcTime;
  }

  snprintf_P(dt, sizeof(dt), PSTR("%04d-%02d-%02dT%02d:%02d:%02d"),
    tmpTime.year, tmpTime.month, tmpTime.day_of_month, tmpTime.hour, tmpTime.minute, tmpTime.second);

  if (Settings.flag3.time_append_timezone && (DT_LOCAL == time_type)) {

    snprintf_P(dt, sizeof(dt), PSTR("%s%+03d:%02d"), dt, time_timezone / 10, abs((time_timezone % 10) * 6));
  }

  return String(dt);
}

String GetUptime()
{
  char dt[16];

  TIME_T ut;

  if (restart_time) {
    BreakTime(utc_time - restart_time, ut);
  } else {
    BreakTime(uptime, ut);
  }






  snprintf_P(dt, sizeof(dt), PSTR("%dT%02d:%02d:%02d"),
    ut.days, ut.hour, ut.minute, ut.second);
  return String(dt);
}

uint32_t GetMinutesUptime()
{
  TIME_T ut;

  if (restart_time) {
    BreakTime(utc_time - restart_time, ut);
  } else {
    BreakTime(uptime, ut);
  }

  return (ut.days *1440) + (ut.hour *60) + ut.minute;
}

uint32_t GetMinutesPastMidnight()
{
  uint32_t minutes = 0;

  if (RtcTime.valid) {
    minutes = (RtcTime.hour *60) + RtcTime.minute;
  }
  return minutes;
}

void BreakTime(uint32_t time_input, TIME_T &tm)
{




  uint8_t year;
  uint8_t month;
  uint8_t month_length;
  uint32_t time;
  unsigned long days;

  time = time_input;
  tm.second = time % 60;
  time /= 60;
  tm.minute = time % 60;
  time /= 60;
  tm.hour = time % 24;
  time /= 24;
  tm.days = time;
  tm.day_of_week = ((time + 4) % 7) + 1;

  year = 0;
  days = 0;
  while((unsigned)(days += (LEAP_YEAR(year) ? 366 : 365)) <= time) {
    year++;
  }
  tm.year = year;

  days -= LEAP_YEAR(year) ? 366 : 365;
  time -= days;
  tm.day_of_year = time;

  days = 0;
  month = 0;
  month_length = 0;
  for (month = 0; month < 12; month++) {
    if (1 == month) {
      if (LEAP_YEAR(year)) {
        month_length = 29;
      } else {
        month_length = 28;
      }
    } else {
      month_length = kDaysInMonth[month];
    }

    if (time >= month_length) {
      time -= month_length;
    } else {
      break;
    }
  }
  strlcpy(tm.name_of_month, kMonthNames + (month *3), 4);
  tm.month = month + 1;
  tm.day_of_month = time + 1;
  tm.valid = (time_input > 1451602800);
}

uint32_t MakeTime(TIME_T &tm)
{



  int i;
  uint32_t seconds;


  seconds = tm.year * (SECS_PER_DAY * 365);
  for (i = 0; i < tm.year; i++) {
    if (LEAP_YEAR(i)) {
      seconds += SECS_PER_DAY;
    }
  }


  for (i = 1; i < tm.month; i++) {
    if ((2 == i) && LEAP_YEAR(tm.year)) {
      seconds += SECS_PER_DAY * 29;
    } else {
      seconds += SECS_PER_DAY * kDaysInMonth[i-1];
    }
  }
  seconds+= (tm.day_of_month - 1) * SECS_PER_DAY;
  seconds+= tm.hour * SECS_PER_HOUR;
  seconds+= tm.minute * SECS_PER_MIN;
  seconds+= tm.second;
  return seconds;
}

uint32_t RuleToTime(TimeRule r, int yr)
{
  TIME_T tm;
  uint32_t t;
  uint8_t m;
  uint8_t w;

  m = r.month;
  w = r.week;
  if (0 == w) {
    if (++m > 12) {
      m = 1;
      yr++;
    }
    w = 1;
  }

  tm.hour = r.hour;
  tm.minute = 0;
  tm.second = 0;
  tm.day_of_month = 1;
  tm.month = m;
  tm.year = yr - 1970;
  t = MakeTime(tm);
  BreakTime(t, tm);
  t += (7 * (w - 1) + (r.dow - tm.day_of_week + 7) % 7) * SECS_PER_DAY;
  if (0 == r.week) {
    t -= 7 * SECS_PER_DAY;
  }
  return t;
}

String GetTime(int type)
{
  char stime[25];

  uint32_t time = utc_time;
  if (1 == type) time = local_time;
  if (2 == type) time = daylight_saving_time;
  if (3 == type) time = standard_time;
  snprintf_P(stime, sizeof(stime), sntp_get_real_time(time));
  return String(stime);
}

uint32_t LocalTime()
{
  return local_time;
}

uint32_t Midnight()
{
  return midnight;
}

boolean MidnightNow()
{
  boolean mnflg = midnight_now;
  if (mnflg) midnight_now = 0;
  return mnflg;
}

void RtcSecond()
{
  int32_t stdoffset;
  int32_t dstoffset;
  TIME_T tmpTime;

  if ((ntp_sync_minute > 59) && (RtcTime.minute > 2)) ntp_sync_minute = 1;
  uint8_t offset = (uptime < 30) ? RtcTime.second : (((ESP.getChipId() & 0xF) * 3) + 3) ;
  if ((WL_CONNECTED == WiFi.status()) && (offset == RtcTime.second) && ((RtcTime.year < 2016) || (ntp_sync_minute == RtcTime.minute) || ntp_force_sync)) {
    ntp_time = sntp_get_current_timestamp();
    if (ntp_time > 1451602800) {
      ntp_force_sync = 0;
      utc_time = ntp_time;
      ntp_sync_minute = 60;
      if (restart_time == 0) {
        restart_time = utc_time - uptime;
      }
      BreakTime(utc_time, tmpTime);
      RtcTime.year = tmpTime.year + 1970;
      daylight_saving_time = RuleToTime(Settings.tflag[1], RtcTime.year);
      standard_time = RuleToTime(Settings.tflag[0], RtcTime.year);
      snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_APPLICATION "(" D_UTC_TIME ") %s, (" D_DST_TIME ") %s, (" D_STD_TIME ") %s"),
        GetTime(0).c_str(), GetTime(2).c_str(), GetTime(3).c_str());
      AddLog(LOG_LEVEL_DEBUG);
      if (local_time < 1451602800) {
        rules_flag.time_init = 1;
      } else {
        rules_flag.time_set = 1;
      }
    } else {
      ntp_sync_minute++;
    }
  }
  utc_time++;
  local_time = utc_time;
  if (local_time > 1451602800) {
    int32_t time_offset = Settings.timezone * SECS_PER_HOUR;
    if (99 == Settings.timezone) {
      dstoffset = Settings.toffset[1] * SECS_PER_MIN;
      stdoffset = Settings.toffset[0] * SECS_PER_MIN;
      if (Settings.tflag[1].hemis) {

        if ((utc_time >= (standard_time - dstoffset)) && (utc_time < (daylight_saving_time - stdoffset))) {
          time_offset = stdoffset;
        } else {
          time_offset = dstoffset;
        }
      } else {

        if ((utc_time >= (daylight_saving_time - stdoffset)) && (utc_time < (standard_time - dstoffset))) {
          time_offset = dstoffset;
        } else {
          time_offset = stdoffset;
        }
      }
    }
    local_time += time_offset;
    time_timezone = time_offset / 360;
    if (!Settings.energy_kWhtotal_time) { Settings.energy_kWhtotal_time = local_time; }
  }
  BreakTime(local_time, RtcTime);
  if (!RtcTime.hour && !RtcTime.minute && !RtcTime.second && RtcTime.valid) {
    midnight = local_time;
    midnight_now = 1;
  }
  RtcTime.year += 1970;
}

void RtcInit()
{
  sntp_setservername(0, Settings.ntp_server[0]);
  sntp_setservername(1, Settings.ntp_server[1]);
  sntp_setservername(2, Settings.ntp_server[2]);
  sntp_stop();
  sntp_set_timezone(0);
  sntp_init();
  utc_time = 0;
  BreakTime(utc_time, RtcTime);
  TickerRtc.attach(1, RtcSecond);
}

#ifndef USE_ADC_VCC




uint16_t adc_last_value = 0;

uint16_t AdcRead()
{
  uint16_t analog = 0;
  for (byte i = 0; i < 32; i++) {
    analog += analogRead(A0);
    delay(1);
  }
  analog >>= 5;
  return analog;
}

#ifdef USE_RULES
void AdcEvery250ms()
{
  uint16_t new_value = AdcRead();
  if ((new_value < adc_last_value -10) || (new_value > adc_last_value +10)) {
    adc_last_value = new_value;
    uint16_t value = adc_last_value / 10;
    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"ANALOG\":{\"A0div10\":%d}}"), (value > 99) ? 100 : value);
    XdrvRulesProcess();
  }
}
#endif

void AdcShow(boolean json)
{
  uint16_t analog = AdcRead();

  if (json) {
    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s,\"ANALOG\":{\"A0\":%d}"), mqtt_data, analog);
#ifdef USE_WEBSERVER
  } else {
    snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_SNS_ANALOG, mqtt_data, "", 0, analog);
#endif
  }
}





#define XSNS_02 

boolean Xsns02(byte function)
{
  boolean result = false;

  if (pin[GPIO_ADC0] < 99) {
    switch (function) {
#ifdef USE_RULES
      case FUNC_EVERY_250_MSECOND:
        AdcEvery250ms();
        break;
#endif
      case FUNC_JSON_APPEND:
        AdcShow(1);
        break;
#ifdef USE_WEBSERVER
      case FUNC_WEB_APPEND:
        AdcShow(0);
        break;
#endif
    }
  }
  return result;
}
#endif
# 2340 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/support.ino"
void SetSeriallog(byte loglevel)
{
  Settings.seriallog_level = loglevel;
  seriallog_level = loglevel;
  seriallog_timer = 0;
}

#ifdef USE_WEBSERVER
void GetLog(byte idx, char** entry_pp, size_t* len_p)
{
  char* entry_p = NULL;
  size_t len = 0;

  if (idx) {
    char* it = web_log;
    do {
      byte cur_idx = *it;
      it++;
      size_t tmp = strchrspn(it, '\1');
      tmp++;
      if (cur_idx == idx) {
        len = tmp;
        entry_p = it;
        break;
      }
      it += tmp;
    } while (it < web_log + WEB_LOG_SIZE && *it != '\0');
  }
  *entry_pp = entry_p;
  *len_p = len;
}
#endif

void Syslog()
{

  char syslog_preamble[64];

  if (syslog_host_hash != GetHash(Settings.syslog_host, strlen(Settings.syslog_host))) {
    syslog_host_hash = GetHash(Settings.syslog_host, strlen(Settings.syslog_host));
    WiFi.hostByName(Settings.syslog_host, syslog_host_addr);
  }
  if (PortUdp.beginPacket(syslog_host_addr, Settings.syslog_port)) {
    snprintf_P(syslog_preamble, sizeof(syslog_preamble), PSTR("%s ESP-"), my_hostname);
    memmove(log_data + strlen(syslog_preamble), log_data, sizeof(log_data) - strlen(syslog_preamble));
    log_data[sizeof(log_data) -1] = '\0';
    memcpy(log_data, syslog_preamble, strlen(syslog_preamble));
    PortUdp.write(log_data);
    PortUdp.endPacket();
  } else {
    syslog_level = 0;
    syslog_timer = SYSLOG_TIMER;
    snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_APPLICATION D_SYSLOG_HOST_NOT_FOUND ". " D_RETRY_IN " %d " D_UNIT_SECOND), SYSLOG_TIMER);
    AddLog(LOG_LEVEL_INFO);
  }
}

void AddLog(byte loglevel)
{
  char mxtime[10];

  snprintf_P(mxtime, sizeof(mxtime), PSTR("%02d" D_HOUR_MINUTE_SEPARATOR "%02d" D_MINUTE_SECOND_SEPARATOR "%02d "), RtcTime.hour, RtcTime.minute, RtcTime.second);

  if (loglevel <= seriallog_level) {
    Serial.printf("%s%s\n", mxtime, log_data);
  }
#ifdef USE_WEBSERVER
  if (Settings.webserver && (loglevel <= Settings.weblog_level)) {


    if (!web_log_index) web_log_index++;
    while (web_log_index == web_log[0] ||
           strlen(web_log) + strlen(log_data) + 13 > WEB_LOG_SIZE)
    {
      char* it = web_log;
      it++;
      it += strchrspn(it, '\1');
      it++;
      memmove(web_log, it, WEB_LOG_SIZE -(it-web_log));
    }
    snprintf_P(web_log, sizeof(web_log), PSTR("%s%c%s%s\1"), web_log, web_log_index++, mxtime, log_data);
    if (!web_log_index) web_log_index++;
  }
#endif
  if ((WL_CONNECTED == WiFi.status()) && (loglevel <= syslog_level)) {
    Syslog();
  }
}

void AddLog_P(byte loglevel, const char *formatP)
{
  snprintf_P(log_data, sizeof(log_data), formatP);
  AddLog(loglevel);
}

void AddLog_P(byte loglevel, const char *formatP, const char *formatP2)
{
  char message[100];

  snprintf_P(log_data, sizeof(log_data), formatP);
  snprintf_P(message, sizeof(message), formatP2);
  strncat(log_data, message, sizeof(log_data));
  AddLog(loglevel);
}

void AddLogSerial(byte loglevel, uint8_t *buffer, int count)
{
  snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_SERIAL D_RECEIVED));
  for (int i = 0; i < count; i++) {
    snprintf_P(log_data, sizeof(log_data), PSTR("%s %02X"), log_data, *(buffer++));
  }
  AddLog(loglevel);
}

void AddLogSerial(byte loglevel)
{
  AddLogSerial(loglevel, (uint8_t*)serial_in_buffer, serial_in_byte_counter);
}

void AddLogMissed(char *sensor, uint8_t misses)
{
  snprintf_P(log_data, sizeof(log_data), PSTR("SNS: %s missed %d"), sensor, SENSOR_MAX_MISS - misses);
  AddLog(LOG_LEVEL_DEBUG);
}
# 1 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xdrv_01_webserver.ino"
# 20 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xdrv_01_webserver.ino"
#ifdef USE_WEBSERVER







#define HTTP_REFRESH_TIME 2345

#ifdef USE_RF_FLASH
uint8_t *efm8bb1_update = NULL;
#endif

enum UploadTypes { UPL_TASMOTA, UPL_SETTINGS, UPL_EFM8BB1 };

const char HTTP_HEAD[] PROGMEM =
  "<!DOCTYPE html><html lang=\"" D_HTML_LANGUAGE "\" class=\"\">"
  "<head>"
  "<meta charset='utf-8'>"
  "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1,user-scalable=no\"/>"
  "<title>{h} - {v}</title>"

  "<script>"
  "var cn,x,lt,to,tp,pc='';"
  "cn=180;"
  "x=null;"
  "function eb(s){"
    "return document.getElementById(s);"
  "}"
  "function u(){"
    "if(cn>=0){"
      "eb('t').innerHTML='" D_RESTART_IN " '+cn+' " D_SECONDS "';"
      "cn--;"
      "setTimeout(u,1000);"
    "}"
  "}"
  "function c(l){"
    "eb('s1').value=l.innerText||l.textContent;"
    "eb('p1').focus();"
  "}"
  "function la(p){"
    "var a='';"
    "if(la.arguments.length==1){"
      "a=p;"
      "clearTimeout(lt);"
    "}"
    "if(x!=null){x.abort();}"
    "x=new XMLHttpRequest();"
    "x.onreadystatechange=function(){"
      "if(x.readyState==4&&x.status==200){"
        "var s=x.responseText.replace(/{t}/g,\"<table style='width:100%'>\").replace(/{s}/g,\"<tr><th>\").replace(/{m}/g,\"</th><td>\").replace(/{e}/g,\"</td></tr>\").replace(/{c}/g,\"%'><div style='text-align:center;font-weight:\");"
        "eb('l1').innerHTML=s;"
      "}"
    "};"
    "x.open('GET','ay'+a,true);"
    "x.send();"
    "lt=setTimeout(la,{a});"
  "}"
  "function lb(p){"
    "la('?d='+p);"
  "}"
  "function lc(p){"
    "la('?t='+p);"
  "}";

const char HTTP_HEAD_RELOAD[] PROGMEM =
  "setTimeout(function(){location.href='.';},4000);";

const char HTTP_HEAD_STYLE[] PROGMEM =
  "</script>"

  "<style>"
  "div,fieldset,input,select{padding:5px;font-size:1em;}"
  "input{width:100%;box-sizing:border-box;-webkit-box-sizing:border-box;-moz-box-sizing:border-box;}"
  "select{width:100%;}"
  "textarea{resize:none;width:98%;height:318px;padding:5px;overflow:auto;}"
  "body{text-align:center;font-family:verdana;}"
  "td{padding:0px;}"
  "button{border:0;border-radius:0.3rem;background-color:#1fa3ec;color:#fff;line-height:2.4rem;font-size:1.2rem;width:100%;-webkit-transition-duration:0.4s;transition-duration:0.4s;cursor:pointer;}"
  "button:hover{background-color:#0e70a4;}"
  ".bred{background-color:#d43535;}"
  ".bred:hover{background-color:#931f1f;}"
  ".bgrn{background-color:#47c266;}"
  ".bgrn:hover{background-color:#5aaf6f;}"
  "a{text-decoration:none;}"
  ".p{float:left;text-align:left;}"
  ".q{float:right;text-align:right;}"
  "</style>"

  "</head>"
  "<body>"
  "<div style='text-align:left;display:inline-block;min-width:340px;'>"
#ifdef BE_MINIMAL
  "<div style='text-align:center;color:red;'><h3>" D_MINIMAL_FIRMWARE_PLEASE_UPGRADE "</h3></div>"
#endif
  "<div style='text-align:center;'><noscript>" D_NOSCRIPT "<br/></noscript>"
#ifdef LANGUAGE_MODULE_NAME
  "<h3>" D_MODULE " {ha</h3>"
#else
  "<h3>{ha " D_MODULE "</h3>"
#endif
  "<h2>{h}</h2>{j}</div>";
const char HTTP_SCRIPT_CONSOL[] PROGMEM =
  "var sn=0;"
  "var id=0;"
  "function l(p){"
    "var c,o,t;"
    "clearTimeout(lt);"
    "o='';"
    "t=eb('t1');"
    "if(p==1){"
      "c=eb('c1');"
      "o='&c1='+encodeURIComponent(c.value);"
      "c.value='';"
      "t.scrollTop=sn;"
    "}"
    "if(t.scrollTop>=sn){"
      "if(x!=null){x.abort();}"
      "x=new XMLHttpRequest();"
      "x.onreadystatechange=function(){"
        "if(x.readyState==4&&x.status==200){"
          "var z,d;"
          "d=x.responseXML;"
          "id=d.getElementsByTagName('i')[0].childNodes[0].nodeValue;"
          "if(d.getElementsByTagName('j')[0].childNodes[0].nodeValue==0){t.value='';}"
          "z=d.getElementsByTagName('l')[0].childNodes;"
          "if(z.length>0){t.value+=decodeURIComponent(z[0].nodeValue);}"
          "t.scrollTop=99999;"
          "sn=t.scrollTop;"
        "}"
      "};"
      "x.open('GET','ax?c2='+id+o,true);"
      "x.send();"
    "}"
    "lt=setTimeout(l,{a});"
    "return false;"
  "}"
  "</script>";
const char HTTP_SCRIPT_MODULE1[] PROGMEM =
  "var os;"
  "function sk(s,g){"
    "var o=os.replace(\"value='\"+s+\"'\",\"selected value='\"+s+\"'\");"
    "eb('g'+g).innerHTML=o;"
  "}"
  "function sl(){"
    "var o0=\"";
const char HTTP_SCRIPT_MODULE2[] PROGMEM =
    "}1'%d'>%02d %s}2";
const char HTTP_SCRIPT_MODULE3[] PROGMEM =
    "\";"
    "os=o0.replace(/}1/g,\"<option value=\").replace(/}2/g,\"</option>\");";
const char HTTP_SCRIPT_INFO_BEGIN[] PROGMEM =
  "function i(){"
    "var s,o=\"";
const char HTTP_SCRIPT_INFO_END[] PROGMEM =
    "\";"
    "s=o.replace(/}1/g,\"</td></tr><tr><th>\").replace(/}2/g,\"</th><td>\");"
    "eb('i').innerHTML=s;"
  "}"
  "</script>";
const char HTTP_MSG_SLIDER1[] PROGMEM =
  "<div><span class='p'>" D_COLDLIGHT "</span><span class='q'>" D_WARMLIGHT "</span></div>"
  "<div><input type='range' min='153' max='500' value='%d' onchange='lc(value)'></div>";
const char HTTP_MSG_SLIDER2[] PROGMEM =
  "<div><span class='p'>" D_DARKLIGHT "</span><span class='q'>" D_BRIGHTLIGHT "</span></div>"
  "<div><input type='range' min='1' max='100' value='%d' onchange='lb(value)'></div>";
const char HTTP_MSG_RSTRT[] PROGMEM =
  "<br/><div style='text-align:center;'>" D_DEVICE_WILL_RESTART "</div><br/>";
const char HTTP_BTN_MENU1[] PROGMEM =
#ifndef BE_MINIMAL
  "<br/><form action='cn' method='get'><button>" D_CONFIGURATION "</button></form>"
  "<br/><form action='in' method='get'><button>" D_INFORMATION "</button></form>"
#endif
  "<br/><form action='up' method='get'><button>" D_FIRMWARE_UPGRADE "</button></form>"
  "<br/><form action='cs' method='get'><button>" D_CONSOLE "</button></form>";
const char HTTP_BTN_RSTRT[] PROGMEM =
  "<br/><form action='.' method='get' onsubmit='return confirm(\"" D_CONFIRM_RESTART "\");'><button name='rstrt' class='button bred'>" D_RESTART "</button></form>";
const char HTTP_BTN_MENU_MODULE[] PROGMEM =
  "<br/><form action='md' method='get'><button>" D_CONFIGURE_MODULE "</button></form>"
  "<br/><form action='wi' method='get'><button>" D_CONFIGURE_WIFI "</button></form>";
const char HTTP_BTN_MENU4[] PROGMEM =
  "<br/><form action='lg' method='get'><button>" D_CONFIGURE_LOGGING "</button></form>"
  "<br/><form action='co' method='get'><button>" D_CONFIGURE_OTHER "</button></form>"
  "<br/>"
  "<br/><form action='rt' method='get' onsubmit='return confirm(\"" D_CONFIRM_RESET_CONFIGURATION "\");'><button class='button bred'>" D_RESET_CONFIGURATION "</button></form>"
  "<br/><form action='dl' method='get'><button>" D_BACKUP_CONFIGURATION "</button></form>"
  "<br/><form action='rs' method='get'><button>" D_RESTORE_CONFIGURATION "</button></form>";
const char HTTP_BTN_MAIN[] PROGMEM =
  "<br/><br/><form action='.' method='get'><button>" D_MAIN_MENU "</button></form>";
const char HTTP_FORM_LOGIN[] PROGMEM =
  "<form method='post' action='/'>"
  "<br/><b>" D_USER "</b><br/><input name='USER1' placeholder='" D_USER "'><br/>"
  "<br/><b>" D_PASSWORD "</b><br/><input name='PASS1' type='password' placeholder='" D_PASSWORD "'><br/>"
  "<br/>"
  "<br/><button>" D_OK "</button></form>";
const char HTTP_BTN_CONF[] PROGMEM =
  "<br/><br/><form action='cn' method='get'><button>" D_CONFIGURATION "</button></form>";
const char HTTP_FORM_MODULE[] PROGMEM =
  "<fieldset><legend><b>&nbsp;" D_MODULE_PARAMETERS "&nbsp;</b></legend><form method='get' action='md'>"
  "<br/><b>" D_MODULE_TYPE "</b> ({mt)<br/><select id='g99' name='g99'></select><br/>";
const char HTTP_LNK_ITEM[] PROGMEM =
  "<div><a href='#p' onclick='c(this)'>{v}</a>&nbsp;({w})&nbsp<span class='q'>{i} {r}%</span></div>";
const char HTTP_LNK_SCAN[] PROGMEM =
  "<div><a href='/wi?scan='>" D_SCAN_FOR_WIFI_NETWORKS "</a></div><br/>";
const char HTTP_FORM_WIFI[] PROGMEM =
  "<fieldset><legend><b>&nbsp;" D_WIFI_PARAMETERS "&nbsp;</b></legend><form method='get' action='wi'>"
  "<br/><b>" D_AP1_SSID "</b> (" STA_SSID1 ")<br/><input id='s1' name='s1' placeholder='" STA_SSID1 "' value='{s1'><br/>"
  "<br/><b>" D_AP1_PASSWORD "</b><br/><input id='p1' name='p1' type='password' placeholder='" D_AP1_PASSWORD "' value='" D_ASTERIX "'><br/>"
  "<br/><b>" D_AP2_SSID "</b> (" STA_SSID2 ")<br/><input id='s2' name='s2' placeholder='" STA_SSID2 "' value='{s2'><br/>"
  "<br/><b>" D_AP2_PASSWORD "</b><br/><input id='p2' name='p2' type='password' placeholder='" D_AP2_PASSWORD "' value='" D_ASTERIX "'><br/>"
  "<br/><b>" D_HOSTNAME "</b> (" WIFI_HOSTNAME ")<br/><input id='h' name='h' placeholder='" WIFI_HOSTNAME" ' value='{h1'><br/>";
const char HTTP_FORM_LOG1[] PROGMEM =
  "<fieldset><legend><b>&nbsp;" D_LOGGING_PARAMETERS "&nbsp;</b></legend><form method='get' action='lg'>";
const char HTTP_FORM_LOG2[] PROGMEM =
  "<br/><b>{b0</b> ({b1)<br/><select id='{b2' name='{b2'>"
  "<option{a0value='0'>0 " D_NONE "</option>"
  "<option{a1value='1'>1 " D_ERROR "</option>"
  "<option{a2value='2'>2 " D_INFO "</option>"
  "<option{a3value='3'>3 " D_DEBUG "</option>"
  "<option{a4value='4'>4 " D_MORE_DEBUG "</option>"
  "</select><br/>";
const char HTTP_FORM_LOG3[] PROGMEM =
  "<br/><b>" D_SYSLOG_HOST "</b> (" SYS_LOG_HOST ")<br/><input id='lh' name='lh' placeholder='" SYS_LOG_HOST "' value='{l2'><br/>"
  "<br/><b>" D_SYSLOG_PORT "</b> (" STR(SYS_LOG_PORT) ")<br/><input id='lp' name='lp' placeholder='" STR(SYS_LOG_PORT) "' value='{l3'><br/>"
  "<br/><b>" D_TELEMETRY_PERIOD "</b> (" STR(TELE_PERIOD) ")<br/><input id='lt' name='lt' placeholder='" STR(TELE_PERIOD) "' value='{l4'><br/>";
const char HTTP_FORM_OTHER[] PROGMEM =
  "<fieldset><legend><b>&nbsp;" D_OTHER_PARAMETERS "&nbsp;</b></legend><form method='get' action='co'>"

  "<br/><b>" D_WEB_ADMIN_PASSWORD "</b><br/><input id='p1' name='p1' type='password' placeholder='" D_WEB_ADMIN_PASSWORD "' value='" D_ASTERIX "'><br/>"
  "<br/><input style='width:10%;' id='b1' name='b1' type='checkbox'{r1><b>" D_MQTT_ENABLE "</b><br/>";
  const char HTTP_FORM_OTHER2[] PROGMEM =
  "<br/><b>" D_FRIENDLY_NAME " {1</b> ({2)<br/><input id='a{1' name='a{1' placeholder='{2' value='{3'><br/>";
#ifdef USE_EMULATION
const char HTTP_FORM_OTHER3a[] PROGMEM =
  "<br/><fieldset><legend><b>&nbsp;" D_EMULATION "&nbsp;</b></legend>";
const char HTTP_FORM_OTHER3b[] PROGMEM =
  "<br/><input style='width:10%;' id='r{1' name='b2' type='radio' value='{1'{2><b>{3</b>{4";
#endif
const char HTTP_FORM_END[] PROGMEM =
  "<br/><button name='save' type='submit' class='button bgrn'>" D_SAVE "</button></form></fieldset>";
const char HTTP_FORM_RST[] PROGMEM =
  "<div id='f1' name='f1' style='display:block;'>"
  "<fieldset><legend><b>&nbsp;" D_RESTORE_CONFIGURATION "&nbsp;</b></legend>";
const char HTTP_FORM_UPG[] PROGMEM =
  "<div id='f1' name='f1' style='display:block;'>"
  "<fieldset><legend><b>&nbsp;" D_UPGRADE_BY_WEBSERVER "&nbsp;</b></legend>"
  "<form method='get' action='u1'>"
  "<br/>" D_OTA_URL "<br/><input id='o' name='o' placeholder='OTA_URL' value='{o1'><br/>"
  "<br/><button type='submit'>" D_START_UPGRADE "</button></form>"
  "</fieldset><br/><br/>"
  "<fieldset><legend><b>&nbsp;" D_UPGRADE_BY_FILE_UPLOAD "&nbsp;</b></legend>";
const char HTTP_FORM_RST_UPG[] PROGMEM =
  "<form method='post' action='u2' enctype='multipart/form-data'>"
  "<br/><input type='file' name='u2'><br/>"
  "<br/><button type='submit' onclick='eb(\"f1\").style.display=\"none\";eb(\"f2\").style.display=\"block\";this.form.submit();'>" D_START " {r1</button></form>"
  "</fieldset>"
  "</div>"
  "<div id='f2' name='f2' style='display:none;text-align:center;'><b>" D_UPLOAD_STARTED " ...</b></div>";
const char HTTP_FORM_CMND[] PROGMEM =
  "<br/><textarea readonly id='t1' name='t1' cols='340' wrap='off'></textarea><br/><br/>"
  "<form method='get' onsubmit='return l(1);'>"
  "<input id='c1' name='c1' placeholder='" D_ENTER_COMMAND "' autofocus><br/>"

  "</form>";
const char HTTP_TABLE100[] PROGMEM =
  "<table style='width:100%'>";
const char HTTP_COUNTER[] PROGMEM =
  "<br/><div id='t' name='t' style='text-align:center;'></div>";
const char HTTP_END[] PROGMEM =
  "<br/>"
  "<div style='text-align:right;font-size:11px;'><hr/><a href='" D_WEBLINK "' target='_blank' style='color:#aaa;'>" D_PROGRAMNAME " {mv " D_BY " " D_AUTHOR "</a></div>"
  "</div>"
  "</body>"
  "</html>";

const char HTTP_DEVICE_CONTROL[] PROGMEM = "<td style='width:%d%%'><button onclick='la(\"?o=%d\");'>%s%s</button></td>";
const char HTTP_DEVICE_STATE[] PROGMEM = "%s<td style='width:%d{c}%s;font-size:%dpx'>%s</div></td>";

const char HDR_CTYPE_PLAIN[] PROGMEM = "text/plain";
const char HDR_CTYPE_HTML[] PROGMEM = "text/html";
const char HDR_CTYPE_XML[] PROGMEM = "text/xml";
const char HDR_CTYPE_JSON[] PROGMEM = "application/json";
const char HDR_CTYPE_STREAM[] PROGMEM = "application/octet-stream";

#define DNS_PORT 53
enum HttpOptions {HTTP_OFF, HTTP_USER, HTTP_ADMIN, HTTP_MANAGER};

DNSServer *DnsServer;
ESP8266WebServer *WebServer;

boolean remove_duplicate_access_points = true;
int minimum_signal_quality = -1;
uint8_t webserver_state = HTTP_OFF;
uint8_t upload_error = 0;
uint8_t upload_file_type;
uint8_t upload_progress_dot_count;
uint8_t config_block_count = 0;
uint8_t config_xor_on = 0;
uint8_t config_xor_on_set = CONFIG_FILE_XOR;


static void WebGetArg(const char* arg, char* out, size_t max)
{
  String s = WebServer->arg(arg);
  strlcpy(out, s.c_str(), max);

}

void ShowWebSource(int source)
{
  if ((source > 0) && (source < SRC_MAX)) {
    char stemp1[20];
    snprintf_P(log_data, sizeof(log_data), PSTR("SRC: %s from %s"), GetTextIndexed(stemp1, sizeof(stemp1), source, kCommandSource), WebServer->client().remoteIP().toString().c_str());
    AddLog(LOG_LEVEL_DEBUG);
  }
}

void ExecuteWebCommand(char* svalue, int source)
{
  ShowWebSource(source);
  ExecuteCommand(svalue, SRC_IGNORE);
}

void StartWebserver(int type, IPAddress ipweb)
{
  if (!Settings.web_refresh) { Settings.web_refresh = HTTP_REFRESH_TIME; }
  if (!webserver_state) {
    if (!WebServer) {
      WebServer = new ESP8266WebServer((HTTP_MANAGER==type) ? 80 : WEB_PORT);
      WebServer->on("/", HandleRoot);
      WebServer->on("/up", HandleUpgradeFirmware);
      WebServer->on("/u1", HandleUpgradeFirmwareStart);
      WebServer->on("/u2", HTTP_POST, HandleUploadDone, HandleUploadLoop);
      WebServer->on("/u2", HTTP_OPTIONS, HandlePreflightRequest);
      WebServer->on("/cs", HandleConsole);
      WebServer->on("/ax", HandleAjaxConsoleRefresh);
      WebServer->on("/ay", HandleAjaxStatusRefresh);
      WebServer->on("/cm", HandleHttpCommand);
      WebServer->onNotFound(HandleNotFound);
#ifndef BE_MINIMAL
      WebServer->on("/cn", HandleConfiguration);
      WebServer->on("/md", HandleModuleConfiguration);
      WebServer->on("/wi", HandleWifiConfiguration);
      WebServer->on("/lg", HandleLoggingConfiguration);
      WebServer->on("/co", HandleOtherConfiguration);
      WebServer->on("/dl", HandleBackupConfiguration);
      WebServer->on("/rs", HandleRestoreConfiguration);
      WebServer->on("/rt", HandleResetConfiguration);
      WebServer->on("/in", HandleInformation);
#ifdef USE_EMULATION
      HueWemoAddHandlers();
#endif
      XdrvCall(FUNC_WEB_ADD_HANDLER);
      XsnsCall(FUNC_WEB_ADD_HANDLER);
#endif
    }
    reset_web_log_flag = 0;
    WebServer->begin();
  }
  if (webserver_state != type) {
    snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_HTTP D_WEBSERVER_ACTIVE_ON " %s%s " D_WITH_IP_ADDRESS " %s"),
      my_hostname, (mdns_begun) ? ".local" : "", ipweb.toString().c_str());
    AddLog(LOG_LEVEL_INFO);
  }
  if (type) { webserver_state = type; }
}

void StopWebserver()
{
  if (webserver_state) {
    WebServer->close();
    webserver_state = HTTP_OFF;
    AddLog_P(LOG_LEVEL_INFO, PSTR(D_LOG_HTTP D_WEBSERVER_STOPPED));
  }
}

void WifiManagerBegin()
{

  if ((WL_CONNECTED == WiFi.status()) && (static_cast<uint32_t>(WiFi.localIP()) != 0)) {
    WiFi.mode(WIFI_AP_STA);
    AddLog_P(LOG_LEVEL_DEBUG, PSTR(D_LOG_WIFI D_WIFIMANAGER_SET_ACCESSPOINT_AND_STATION));
  } else {
    WiFi.mode(WIFI_AP);
    AddLog_P(LOG_LEVEL_DEBUG, PSTR(D_LOG_WIFI D_WIFIMANAGER_SET_ACCESSPOINT));
  }

  StopWebserver();

  DnsServer = new DNSServer();
  WiFi.softAP(my_hostname);
  delay(500);

  DnsServer->setErrorReplyCode(DNSReplyCode::NoError);
  DnsServer->start(DNS_PORT, "*", WiFi.softAPIP());

  StartWebserver(HTTP_MANAGER, WiFi.softAPIP());
}

void PollDnsWebserver()
{
  if (DnsServer) { DnsServer->processNextRequest(); }
  if (WebServer) { WebServer->handleClient(); }
}



void SetHeader()
{
  WebServer->sendHeader(F("Cache-Control"), F("no-cache, no-store, must-revalidate"));
  WebServer->sendHeader(F("Pragma"), F("no-cache"));
  WebServer->sendHeader(F("Expires"), F("-1"));
#ifndef ARDUINO_ESP8266_RELEASE_2_3_0
  WebServer->sendHeader(F("Access-Control-Allow-Origin"), F("*"));
#endif
}

bool WebAuthenticate(void)
{
  if (Settings.web_password[0] != 0) {
    return WebServer->authenticate(WEB_USERNAME, Settings.web_password);
  } else {
    return true;
  }
}

void ShowPage(String &page, bool auth)
{
  if (auth && (Settings.web_password[0] != 0) && !WebServer->authenticate(WEB_USERNAME, Settings.web_password)) {
    return WebServer->requestAuthentication();
  }

  page.replace(F("{a}"), String(Settings.web_refresh));
  page.replace(F("{ha"), my_module.name);
  page.replace(F("{h}"), Settings.friendlyname[0]);

  String info = "";
  if (Settings.flag3.gui_hostname_ip) {
    uint8_t more_ips = 0;
    info += F("<h3>"); info += my_hostname;
    if (mdns_begun) { info += F(".local"); }
    info += F(" (");
    if (static_cast<uint32_t>(WiFi.localIP()) != 0) {
      info += WiFi.localIP().toString();
      more_ips++;
    }
    if (static_cast<uint32_t>(WiFi.softAPIP()) != 0) {
      if (more_ips) { info += F(", "); }
      info += WiFi.softAPIP().toString();
    }
    info += F(")</h3>");
  }
  page.replace(F("{j}"), info);

  if (HTTP_MANAGER == webserver_state) {
    if (WifiConfigCounter()) {
      page.replace(F("<body>"), F("<body onload='u()'>"));
      page += FPSTR(HTTP_COUNTER);
    }
  }
  page += FPSTR(HTTP_END);
  page.replace(F("{mv"), my_version);
  SetHeader();

  ShowFreeMem(PSTR("ShowPage"));

  WebServer->send(200, FPSTR(HDR_CTYPE_HTML), page);
}

void ShowPage(String &page)
{
  ShowPage(page, true);
}



void WebRestart(uint8_t type)
{



  AddLog_P(LOG_LEVEL_DEBUG, S_LOG_HTTP, S_RESTART);

  String page = FPSTR(HTTP_HEAD);
  page += FPSTR(HTTP_HEAD_RELOAD);
  page += FPSTR(HTTP_HEAD_STYLE);

  if (type) {
    page.replace(F("{v}"), FPSTR(S_SAVE_CONFIGURATION));
    page += F("<div style='text-align:center;'><b>" D_CONFIGURATION_SAVED "</b><br/>");
    if (2 == type) {
      page += F("<br/>" D_TRYING_TO_CONNECT "<br/>");
    }
    page += F("</div>");
  }
  else {
    page.replace(F("{v}"), FPSTR(S_RESTART));
  }

  page += FPSTR(HTTP_MSG_RSTRT);
  if (HTTP_MANAGER == webserver_state) {
    webserver_state = HTTP_ADMIN;
  } else {
    page += FPSTR(HTTP_BTN_MAIN);
  }
  ShowPage(page);

  ShowWebSource(SRC_WEBGUI);
  restart_flag = 2;
}



void HandleWifiLogin()
{
  String page = FPSTR(HTTP_HEAD);
  page.replace(F("{v}"), FPSTR( D_CONFIGURE_WIFI ));
  page += FPSTR(HTTP_HEAD_STYLE);
  page += FPSTR(HTTP_FORM_LOGIN);
  ShowPage(page, false);
}

void HandleRoot()
{
  AddLog_P(LOG_LEVEL_DEBUG, S_LOG_HTTP, S_MAIN_MENU);

  if (CaptivePortal()) { return; }

  if ( WebServer->hasArg("rstrt") ) {
    WebRestart(0);
    return;
  }

  if (HTTP_MANAGER == webserver_state) {
#ifndef BE_MINIMAL
    if ((Settings.web_password[0] != 0) && !(WebServer->hasArg("USER1")) && !(WebServer->hasArg("PASS1"))) {
      HandleWifiLogin();
    } else {







      if (!(Settings.web_password[0] != 0) || ((WebServer->arg("USER1") == WEB_USERNAME ) && (WebServer->arg("PASS1") == Settings.web_password ))) {
        HandleWifiConfiguration();
      } else {

        HandleWifiLogin();
      }
    }
#endif
  } else {
    char stemp[10];
    String page = FPSTR(HTTP_HEAD);
    page.replace(F("{v}"), FPSTR(S_MAIN_MENU));
    page += FPSTR(HTTP_HEAD_STYLE);
    page.replace(F("<body>"), F("<body onload='la()'>"));

    page += F("<div id='l1' name='l1'></div>");
    if (devices_present) {
      if (light_type) {
        if ((LST_COLDWARM == (light_type &7)) || (LST_RGBWC == (light_type &7))) {
          snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_MSG_SLIDER1, LightGetColorTemp());
          page += mqtt_data;
        }
        snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_MSG_SLIDER2, Settings.light_dimmer);
        page += mqtt_data;
      }
      page += FPSTR(HTTP_TABLE100);
      page += F("<tr>");
      if (SONOFF_IFAN02 == Settings.module) {
        snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_DEVICE_CONTROL, 36, 1, D_BUTTON_TOGGLE, "");
        page += mqtt_data;
        for (byte i = 0; i < 4; i++) {
          snprintf_P(stemp, sizeof(stemp), PSTR("%d"), i);
          snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_DEVICE_CONTROL, 16, i +2, stemp, "");
          page += mqtt_data;
        }
      } else {
        for (byte idx = 1; idx <= devices_present; idx++) {
          snprintf_P(stemp, sizeof(stemp), PSTR(" %d"), idx);
          snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_DEVICE_CONTROL,
            100 / devices_present, idx, (devices_present < 5) ? D_BUTTON_TOGGLE : "", (devices_present > 1) ? stemp : "");
          page += mqtt_data;
        }
      }
      page += F("</tr></table>");
    }
    if (SONOFF_BRIDGE == Settings.module) {
      page += FPSTR(HTTP_TABLE100);
      page += F("<tr>");
      byte idx = 0;
      for (byte i = 0; i < 4; i++) {
        if (idx > 0) { page += F("</tr><tr>"); }
        for (byte j = 0; j < 4; j++) {
          idx++;
          snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("<td style='width:25%'><button onclick='la(\"?k=%d\");'>%d</button></td>"), idx, idx);
          page += mqtt_data;
        }
      }
      page += F("</tr></table>");
    }

#ifndef BE_MINIMAL
    mqtt_data[0] = '\0';
    XdrvCall(FUNC_WEB_ADD_MAIN_BUTTON);
    XsnsCall(FUNC_WEB_ADD_MAIN_BUTTON);
    page += String(mqtt_data);
#endif

    if (HTTP_ADMIN == webserver_state) {
      page += FPSTR(HTTP_BTN_MENU1);
      page += FPSTR(HTTP_BTN_RSTRT);
    }
    ShowPage(page);
  }
}

void HandleAjaxStatusRefresh()
{
  if (!WebAuthenticate()) { return WebServer->requestAuthentication(); }

  char svalue[80];
  char tmp[100];

  WebGetArg("o", tmp, sizeof(tmp));
  if (strlen(tmp)) {
    ShowWebSource(SRC_WEBGUI);
    uint8_t device = atoi(tmp);
    if (SONOFF_IFAN02 == Settings.module) {
      if (device < 2) {
        ExecuteCommandPower(1, POWER_TOGGLE, SRC_IGNORE);
      } else {
        snprintf_P(svalue, sizeof(svalue), PSTR(D_CMND_FANSPEED " %d"), device -2);
        ExecuteCommand(svalue, SRC_WEBGUI);
      }
    } else {
      ExecuteCommandPower(device, POWER_TOGGLE, SRC_IGNORE);
    }
  }
  WebGetArg("d", tmp, sizeof(tmp));
  if (strlen(tmp)) {
    snprintf_P(svalue, sizeof(svalue), PSTR(D_CMND_DIMMER " %s"), tmp);
    ExecuteWebCommand(svalue, SRC_WEBGUI);
  }
  WebGetArg("t", tmp, sizeof(tmp));
  if (strlen(tmp)) {
    snprintf_P(svalue, sizeof(svalue), PSTR(D_CMND_COLORTEMPERATURE " %s"), tmp);
    ExecuteWebCommand(svalue, SRC_WEBGUI);
  }
  WebGetArg("k", tmp, sizeof(tmp));
  if (strlen(tmp)) {
    snprintf_P(svalue, sizeof(svalue), PSTR(D_CMND_RFKEY "%s"), tmp);
    ExecuteWebCommand(svalue, SRC_WEBGUI);
  }

  snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{t}"));
  XsnsCall(FUNC_WEB_APPEND);
  if (D_DECIMAL_SEPARATOR[0] != '.') {
    for (uint16_t i = 0; i < strlen(mqtt_data); i++) {
      if ('.' == mqtt_data[i]) {
        mqtt_data[i] = D_DECIMAL_SEPARATOR[0];
      }
    }
  }
  snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s</table>"), mqtt_data);
  if (devices_present) {
    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s{t}<tr>"), mqtt_data);
    uint8_t fsize = (devices_present < 5) ? 70 - (devices_present * 8) : 32;
    if (SONOFF_IFAN02 == Settings.module) {
      snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_DEVICE_STATE,
        mqtt_data, 36, (bitRead(power, 0)) ? "bold" : "normal", 54, GetStateText(bitRead(power, 0)));
      uint8_t fanspeed = GetFanspeed();
      snprintf_P(svalue, sizeof(svalue), PSTR("%d"), fanspeed);
      snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_DEVICE_STATE,
        mqtt_data, 64, (fanspeed) ? "bold" : "normal", 54, (fanspeed) ? svalue : GetStateText(0));
    } else {
      for (byte idx = 1; idx <= devices_present; idx++) {
        snprintf_P(svalue, sizeof(svalue), PSTR("%d"), bitRead(power, idx -1));
        snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_DEVICE_STATE,
          mqtt_data, 100 / devices_present, (bitRead(power, idx -1)) ? "bold" : "normal", fsize, (devices_present < 5) ? GetStateText(bitRead(power, idx -1)) : svalue);
      }
    }
    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s</tr></table>"), mqtt_data);
  }
  WebServer->send(200, FPSTR(HDR_CTYPE_HTML), mqtt_data);
}

boolean HttpUser()
{
  boolean status = (HTTP_USER == webserver_state);
  if (status) { HandleRoot(); }
  return status;
}



#ifndef BE_MINIMAL

void HandleConfiguration()
{
  if (HttpUser()) { return; }
  if (!WebAuthenticate()) { return WebServer->requestAuthentication(); }
  AddLog_P(LOG_LEVEL_DEBUG, S_LOG_HTTP, S_CONFIGURATION);

  String page = FPSTR(HTTP_HEAD);
  page.replace(F("{v}"), FPSTR(S_CONFIGURATION));
  page += FPSTR(HTTP_HEAD_STYLE);
  page += FPSTR(HTTP_BTN_MENU_MODULE);

  mqtt_data[0] = '\0';
  XdrvCall(FUNC_WEB_ADD_BUTTON);
  XsnsCall(FUNC_WEB_ADD_BUTTON);
  page += String(mqtt_data);

  page += FPSTR(HTTP_BTN_MENU4);
  page += FPSTR(HTTP_BTN_MAIN);
  ShowPage(page);
}



void HandleModuleConfiguration()
{
  if (HttpUser()) { return; }
  if (!WebAuthenticate()) { return WebServer->requestAuthentication(); }

  AddLog_P(LOG_LEVEL_DEBUG, S_LOG_HTTP, S_CONFIGURE_MODULE);

  if (WebServer->hasArg("save")) {
    ModuleSaveSettings();
    WebRestart(1);
    return;
  }

  char stemp[20];
  uint8_t midx;

  String page = FPSTR(HTTP_HEAD);
  page.replace(F("{v}"), FPSTR(S_CONFIGURE_MODULE));
  page += FPSTR(HTTP_SCRIPT_MODULE1);
  for (byte i = 0; i < MAXMODULE; i++) {
    midx = pgm_read_byte(kModuleNiceList + i);
    snprintf_P(stemp, sizeof(stemp), kModules[midx].name);
    snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_SCRIPT_MODULE2, midx, midx +1, stemp);
    page += mqtt_data;
  }
  page += FPSTR(HTTP_SCRIPT_MODULE3);
  snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("sk(%d,99);o0=\""), Settings.module);
  page += mqtt_data;

  mytmplt cmodule;
  memcpy_P(&cmodule, &kModules[Settings.module], sizeof(cmodule));
  for (byte j = 0; j < GPIO_SENSOR_END; j++) {
    midx = pgm_read_byte(kGpioNiceList + j);
    if (!GetUsedInModule(midx, cmodule.gp.io)) {
      snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_SCRIPT_MODULE2, midx, midx, GetTextIndexed(stemp, sizeof(stemp), midx, kSensorNames));
      page += mqtt_data;
    }
  }
  page += FPSTR(HTTP_SCRIPT_MODULE3);

  for (byte i = 0; i < MAX_GPIO_PIN; i++) {
    if (GPIO_USER == ValidGPIO(i, cmodule.gp.io[i])) {
      snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("sk(%d,%d);"), my_module.gp.io[i], i);
      page += mqtt_data;
    }
  }
  page += F("}");

  page += FPSTR(HTTP_HEAD_STYLE);
  page.replace(F("<body>"), F("<body onload='sl()'>"));
  page += FPSTR(HTTP_FORM_MODULE);
  snprintf_P(stemp, sizeof(stemp), kModules[MODULE].name);
  page.replace(F("{mt"), stemp);
  page += F("<br/><table>");
  for (byte i = 0; i < MAX_GPIO_PIN; i++) {
    if (GPIO_USER == ValidGPIO(i, cmodule.gp.io[i])) {
      snprintf_P(stemp, 3, PINS_WEMOS +i*2);
      snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("<tr><td style='width:190px'>%s <b>" D_GPIO "%d</b> %s</td><td style='width:160px'><select id='g%d' name='g%d'></select></td></tr>"),
        (WEMOS==Settings.module)?stemp:"", i, (0==i)? D_SENSOR_BUTTON "1":(1==i)? D_SERIAL_OUT :(3==i)? D_SERIAL_IN :(9==i)? "<font color='red'>ESP8285</font>" :(10==i)? "<font color='red'>ESP8285</font>" :(12==i)? D_SENSOR_RELAY "1":(13==i)? D_SENSOR_LED "1i":(14==i)? D_SENSOR :"", i, i);
      page += mqtt_data;
    }
  }
  page += F("</table>");
  page += FPSTR(HTTP_FORM_END);
  page += FPSTR(HTTP_BTN_CONF);
  ShowPage(page);
}

void ModuleSaveSettings()
{
  char tmp[100];
  char stemp[TOPSZ];

  WebGetArg("g99", tmp, sizeof(tmp));
  byte new_module = (!strlen(tmp)) ? MODULE : atoi(tmp);
  Settings.last_module = Settings.module;
  Settings.module = new_module;
  mytmplt cmodule;
  memcpy_P(&cmodule, &kModules[Settings.module], sizeof(cmodule));
  String gpios = "";
  for (byte i = 0; i < MAX_GPIO_PIN; i++) {
    if (Settings.last_module != new_module) {
      Settings.my_gp.io[i] = 0;
    } else {
      if (GPIO_USER == ValidGPIO(i, cmodule.gp.io[i])) {
        snprintf_P(stemp, sizeof(stemp), PSTR("g%d"), i);
        WebGetArg(stemp, tmp, sizeof(tmp));
        Settings.my_gp.io[i] = (!strlen(tmp)) ? 0 : atoi(tmp);
        gpios += F(", " D_GPIO ); gpios += String(i); gpios += F(" "); gpios += String(Settings.my_gp.io[i]);
      }
    }
  }
  snprintf_P(stemp, sizeof(stemp), kModules[Settings.module].name);
  snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_MODULE "%s " D_CMND_MODULE "%s"), stemp, gpios.c_str());
  AddLog(LOG_LEVEL_INFO);
}



String htmlEscape(String s)
{
    s.replace("&", "&amp;");
    s.replace("<", "&lt;");
    s.replace(">", "&gt;");
    s.replace("\"", "&quot;");
    s.replace("'", "&#x27;");
    s.replace("/", "&#x2F;");
    return s;
}

void HandleWifiConfiguration()
{
  if (HttpUser()) { return; }
  if (!WebAuthenticate()) { return WebServer->requestAuthentication(); }

  AddLog_P(LOG_LEVEL_DEBUG, S_LOG_HTTP, S_CONFIGURE_WIFI);

  if (WebServer->hasArg("save")) {
    WifiSaveSettings();
    WebRestart(2);
    return;
  }

  String page = FPSTR(HTTP_HEAD);
  page.replace(F("{v}"), FPSTR(S_CONFIGURE_WIFI));
  page += FPSTR(HTTP_HEAD_STYLE);

  if (WebServer->hasArg("scan")) {
#ifdef USE_EMULATION
    UdpDisconnect();
#endif
    int n = WiFi.scanNetworks();
    AddLog_P(LOG_LEVEL_DEBUG, PSTR(D_LOG_WIFI D_SCAN_DONE));

    if (0 == n) {
      AddLog_P(LOG_LEVEL_DEBUG, S_LOG_WIFI, S_NO_NETWORKS_FOUND);
      page += FPSTR(S_NO_NETWORKS_FOUND);
      page += F(". " D_REFRESH_TO_SCAN_AGAIN ".");
    } else {

      int indices[n];
      for (int i = 0; i < n; i++) {
        indices[i] = i;
      }


      for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
          if (WiFi.RSSI(indices[j]) > WiFi.RSSI(indices[i])) {
            std::swap(indices[i], indices[j]);
          }
        }
      }


      if (remove_duplicate_access_points) {
        String cssid;
        for (int i = 0; i < n; i++) {
          if (-1 == indices[i]) { continue; }
          cssid = WiFi.SSID(indices[i]);
          for (int j = i + 1; j < n; j++) {
            if (cssid == WiFi.SSID(indices[j])) {
              snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_WIFI D_DUPLICATE_ACCESSPOINT " %s"), WiFi.SSID(indices[j]).c_str());
              AddLog(LOG_LEVEL_DEBUG);
              indices[j] = -1;
            }
          }
        }
      }


      for (int i = 0; i < n; i++) {
        if (-1 == indices[i]) { continue; }
        snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_WIFI D_SSID " %s, " D_BSSID " %s, " D_CHANNEL " %d, " D_RSSI " %d"), WiFi.SSID(indices[i]).c_str(), WiFi.BSSIDstr(indices[i]).c_str(), WiFi.channel(indices[i]), WiFi.RSSI(indices[i]));
        AddLog(LOG_LEVEL_DEBUG);
        int quality = WifiGetRssiAsQuality(WiFi.RSSI(indices[i]));

        if (minimum_signal_quality == -1 || minimum_signal_quality < quality) {
          String item = FPSTR(HTTP_LNK_ITEM);
          String rssiQ;
          rssiQ += quality;
          item.replace(F("{v}"), htmlEscape(WiFi.SSID(indices[i])));
          item.replace(F("{w}"), String(WiFi.channel(indices[i])));
          item.replace(F("{r}"), rssiQ);
          uint8_t auth = WiFi.encryptionType(indices[i]);
          item.replace(F("{i}"), (ENC_TYPE_WEP == auth) ? F(D_WEP) : (ENC_TYPE_TKIP == auth) ? F(D_WPA_PSK) : (ENC_TYPE_CCMP == auth) ? F(D_WPA2_PSK) : (ENC_TYPE_AUTO == auth) ? F(D_AUTO) : F(""));
          page += item;
          delay(0);
        } else {
          AddLog_P(LOG_LEVEL_DEBUG, PSTR(D_LOG_WIFI D_SKIPPING_LOW_QUALITY));
        }

      }
      page += "<br/>";
    }
  } else {
    page += FPSTR(HTTP_LNK_SCAN);
  }

  page += FPSTR(HTTP_FORM_WIFI);
  page.replace(F("{h1"), Settings.hostname);
  page.replace(F("{s1"), Settings.sta_ssid[0]);
  page.replace(F("{s2"), Settings.sta_ssid[1]);
  page += FPSTR(HTTP_FORM_END);
  if (HTTP_MANAGER == webserver_state) {
    page += FPSTR(HTTP_BTN_RSTRT);
  } else {
    page += FPSTR(HTTP_BTN_CONF);
  }

  ShowPage(page, !(HTTP_MANAGER == webserver_state));
}

void WifiSaveSettings()
{
  char tmp[100];

  WebGetArg("h", tmp, sizeof(tmp));
  strlcpy(Settings.hostname, (!strlen(tmp)) ? WIFI_HOSTNAME : tmp, sizeof(Settings.hostname));
  if (strstr(Settings.hostname,"%")) {
    strlcpy(Settings.hostname, WIFI_HOSTNAME, sizeof(Settings.hostname));
  }
  WebGetArg("s1", tmp, sizeof(tmp));
  strlcpy(Settings.sta_ssid[0], (!strlen(tmp)) ? STA_SSID1 : tmp, sizeof(Settings.sta_ssid[0]));
  WebGetArg("s2", tmp, sizeof(tmp));
  strlcpy(Settings.sta_ssid[1], (!strlen(tmp)) ? STA_SSID2 : tmp, sizeof(Settings.sta_ssid[1]));
  WebGetArg("p1", tmp, sizeof(tmp));
  strlcpy(Settings.sta_pwd[0], (!strlen(tmp)) ? "" : (strchr(tmp,'*')) ? Settings.sta_pwd[0] : tmp, sizeof(Settings.sta_pwd[0]));
  WebGetArg("p2", tmp, sizeof(tmp));
  strlcpy(Settings.sta_pwd[1], (!strlen(tmp)) ? "" : (strchr(tmp,'*')) ? Settings.sta_pwd[1] : tmp, sizeof(Settings.sta_pwd[1]));
  snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_WIFI D_CMND_HOSTNAME " %s, " D_CMND_SSID "1 %s, " D_CMND_SSID "2 %s"),
    Settings.hostname, Settings.sta_ssid[0], Settings.sta_ssid[1]);
  AddLog(LOG_LEVEL_INFO);
}



void HandleLoggingConfiguration()
{
  if (HttpUser()) { return; }
  if (!WebAuthenticate()) { return WebServer->requestAuthentication(); }
  AddLog_P(LOG_LEVEL_DEBUG, S_LOG_HTTP, S_CONFIGURE_LOGGING);

  if (WebServer->hasArg("save")) {
    LoggingSaveSettings();
    HandleConfiguration();
    return;
  }

  String page = FPSTR(HTTP_HEAD);
  page.replace(F("{v}"), FPSTR(S_CONFIGURE_LOGGING));
  page += FPSTR(HTTP_HEAD_STYLE);

  page += FPSTR(HTTP_FORM_LOG1);
  for (byte idx = 0; idx < 3; idx++) {
    page += FPSTR(HTTP_FORM_LOG2);
    switch (idx) {
    case 0:
      page.replace(F("{b0"), F(D_SERIAL_LOG_LEVEL));
      page.replace(F("{b1"), STR(SERIAL_LOG_LEVEL));
      page.replace(F("{b2"), F("ls"));
      for (byte i = LOG_LEVEL_NONE; i < LOG_LEVEL_ALL; i++) {
        page.replace("{a" + String(i), (i == Settings.seriallog_level) ? F(" selected ") : F(" "));
      }
      break;
    case 1:
      page.replace(F("{b0"), F(D_WEB_LOG_LEVEL));
      page.replace(F("{b1"), STR(WEB_LOG_LEVEL));
      page.replace(F("{b2"), F("lw"));
      for (byte i = LOG_LEVEL_NONE; i < LOG_LEVEL_ALL; i++) {
        page.replace("{a" + String(i), (i == Settings.weblog_level) ? F(" selected ") : F(" "));
      }
      break;
    case 2:
      page.replace(F("{b0"), F(D_SYS_LOG_LEVEL));
      page.replace(F("{b1"), STR(SYS_LOG_LEVEL));
      page.replace(F("{b2"), F("ll"));
      for (byte i = LOG_LEVEL_NONE; i < LOG_LEVEL_ALL; i++) {
        page.replace("{a" + String(i), (i == Settings.syslog_level) ? F(" selected ") : F(" "));
      }
      break;
    }
  }
  page += FPSTR(HTTP_FORM_LOG3);
  page.replace(F("{l2"), Settings.syslog_host);
  page.replace(F("{l3"), String(Settings.syslog_port));
  page.replace(F("{l4"), String(Settings.tele_period));
  page += FPSTR(HTTP_FORM_END);
  page += FPSTR(HTTP_BTN_CONF);
  ShowPage(page);
}

void LoggingSaveSettings()
{
  char tmp[100];

  WebGetArg("ls", tmp, sizeof(tmp));
  Settings.seriallog_level = (!strlen(tmp)) ? SERIAL_LOG_LEVEL : atoi(tmp);
  WebGetArg("lw", tmp, sizeof(tmp));
  Settings.weblog_level = (!strlen(tmp)) ? WEB_LOG_LEVEL : atoi(tmp);
  WebGetArg("ll", tmp, sizeof(tmp));
  Settings.syslog_level = (!strlen(tmp)) ? SYS_LOG_LEVEL : atoi(tmp);
  syslog_level = Settings.syslog_level;
  syslog_timer = 0;
  WebGetArg("lh", tmp, sizeof(tmp));
  strlcpy(Settings.syslog_host, (!strlen(tmp)) ? SYS_LOG_HOST : tmp, sizeof(Settings.syslog_host));
  WebGetArg("lp", tmp, sizeof(tmp));
  Settings.syslog_port = (!strlen(tmp)) ? SYS_LOG_PORT : atoi(tmp);
  WebGetArg("lt", tmp, sizeof(tmp));
  Settings.tele_period = (!strlen(tmp)) ? TELE_PERIOD : atoi(tmp);
  if ((Settings.tele_period > 0) && (Settings.tele_period < 10)) {
    Settings.tele_period = 10;
  }
  snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_LOG D_CMND_SERIALLOG " %d, " D_CMND_WEBLOG " %d, " D_CMND_SYSLOG " %d, " D_CMND_LOGHOST " %s, " D_CMND_LOGPORT " %d, " D_CMND_TELEPERIOD " %d"),
    Settings.seriallog_level, Settings.weblog_level, Settings.syslog_level, Settings.syslog_host, Settings.syslog_port, Settings.tele_period);
  AddLog(LOG_LEVEL_INFO);
}



void HandleOtherConfiguration()
{
  if (HttpUser()) { return; }
  if (!WebAuthenticate()) { return WebServer->requestAuthentication(); }
  AddLog_P(LOG_LEVEL_DEBUG, S_LOG_HTTP, S_CONFIGURE_OTHER);

  if (WebServer->hasArg("save")) {
    OtherSaveSettings();
    WebRestart(1);
    return;
  }

  char stemp[40];

  String page = FPSTR(HTTP_HEAD);
  page.replace(F("{v}"), FPSTR(S_CONFIGURE_OTHER));
  page += FPSTR(HTTP_HEAD_STYLE);
  page += FPSTR(HTTP_FORM_OTHER);
  page.replace(F("{r1"), (Settings.flag.mqtt_enabled) ? F(" checked") : F(""));
  uint8_t maxfn = (devices_present > MAX_FRIENDLYNAMES) ? MAX_FRIENDLYNAMES : (!devices_present) ? 1 : devices_present;
  if (SONOFF_IFAN02 == Settings.module) { maxfn = 1; }
  for (byte i = 0; i < maxfn; i++) {
    page += FPSTR(HTTP_FORM_OTHER2);
    page.replace(F("{1"), String(i +1));
    snprintf_P(stemp, sizeof(stemp), PSTR(FRIENDLY_NAME"%d"), i +1);
    page.replace(F("{2"), (i) ? stemp : FRIENDLY_NAME);
    page.replace(F("{3"), Settings.friendlyname[i]);
  }
#ifdef USE_EMULATION
  page += FPSTR(HTTP_FORM_OTHER3a);
  for (byte i = 0; i < EMUL_MAX; i++) {
    page += FPSTR(HTTP_FORM_OTHER3b);
    page.replace(F("{1"), String(i));
    page.replace(F("{2"), (i == Settings.flag2.emulation) ? F(" checked") : F(""));
    page.replace(F("{3"), (i == EMUL_NONE) ? F(D_NONE) : (i == EMUL_WEMO) ? F(D_BELKIN_WEMO) : F(D_HUE_BRIDGE));
    page.replace(F("{4"), (i == EMUL_NONE) ? F("") : (i == EMUL_WEMO) ? F(" " D_SINGLE_DEVICE) : F(" " D_MULTI_DEVICE));
  }
  page += F("<br/>");
  page += F("<br/></fieldset>");
#endif
  page += FPSTR(HTTP_FORM_END);
  page += FPSTR(HTTP_BTN_CONF);
  ShowPage(page);
}

void OtherSaveSettings()
{
  char tmp[100];
  char stemp[TOPSZ];
  char stemp2[TOPSZ];

  WebGetArg("p1", tmp, sizeof(tmp));
  strlcpy(Settings.web_password, (!strlen(tmp)) ? "" : (strchr(tmp,'*')) ? Settings.web_password : tmp, sizeof(Settings.web_password));
  Settings.flag.mqtt_enabled = WebServer->hasArg("b1");
#ifdef USE_EMULATION
  WebGetArg("b2", tmp, sizeof(tmp));
  Settings.flag2.emulation = (!strlen(tmp)) ? 0 : atoi(tmp);
#endif
  snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_OTHER D_MQTT_ENABLE " %s, " D_CMND_EMULATION " %d, " D_CMND_FRIENDLYNAME), GetStateText(Settings.flag.mqtt_enabled), Settings.flag2.emulation);
  for (byte i = 0; i < MAX_FRIENDLYNAMES; i++) {
    snprintf_P(stemp, sizeof(stemp), PSTR("a%d"), i +1);
    WebGetArg(stemp, tmp, sizeof(tmp));
    snprintf_P(stemp2, sizeof(stemp2), PSTR(FRIENDLY_NAME"%d"), i +1);
    strlcpy(Settings.friendlyname[i], (!strlen(tmp)) ? (i) ? stemp2 : FRIENDLY_NAME : tmp, sizeof(Settings.friendlyname[i]));
    snprintf_P(log_data, sizeof(log_data), PSTR("%s%s %s"), log_data, (i) ? "," : "", Settings.friendlyname[i]);
  }
  AddLog(LOG_LEVEL_INFO);
}



void HandleBackupConfiguration()
{
  if (HttpUser()) { return; }
  if (!WebAuthenticate()) { return WebServer->requestAuthentication(); }
  AddLog_P(LOG_LEVEL_DEBUG, PSTR(D_LOG_HTTP D_BACKUP_CONFIGURATION));

  if (!SettingsBufferAlloc()) { return; }

  WiFiClient myClient = WebServer->client();
  WebServer->setContentLength(sizeof(Settings));

  char attachment[100];
  char friendlyname[sizeof(Settings.friendlyname[0])];
  snprintf_P(attachment, sizeof(attachment), PSTR("attachment; filename=Config_%s_%s.dmp"), NoAlNumToUnderscore(friendlyname, Settings.friendlyname[0]), my_version);
  WebServer->sendHeader(F("Content-Disposition"), attachment);

  WebServer->send(200, FPSTR(HDR_CTYPE_STREAM), "");

  uint16_t cfg_crc = Settings.cfg_crc;
  Settings.cfg_crc = GetSettingsCrc();

  memcpy(settings_buffer, &Settings, sizeof(Settings));
  if (config_xor_on_set) {
    for (uint16_t i = 2; i < sizeof(Settings); i++) {
      settings_buffer[i] ^= (config_xor_on_set +i);
    }
  }

#ifdef ARDUINO_ESP8266_RELEASE_2_3_0
  size_t written = myClient.write((const char*)settings_buffer, sizeof(Settings));
  if (written < sizeof(Settings)) {
    myClient.write((const char*)settings_buffer +written, sizeof(Settings) -written);
  }
#else
  myClient.write((const char*)settings_buffer, sizeof(Settings));
#endif

  SettingsBufferFree();

  Settings.cfg_crc = cfg_crc;
}



void HandleResetConfiguration()
{
  if (HttpUser()) { return; }
  if (!WebAuthenticate()) { return WebServer->requestAuthentication(); }

  char svalue[33];

  AddLog_P(LOG_LEVEL_DEBUG, S_LOG_HTTP, S_RESET_CONFIGURATION);

  String page = FPSTR(HTTP_HEAD);
  page.replace(F("{v}"), FPSTR(S_RESET_CONFIGURATION));
  page += FPSTR(HTTP_HEAD_STYLE);
  page += F("<div style='text-align:center;'>" D_CONFIGURATION_RESET "</div>");
  page += FPSTR(HTTP_MSG_RSTRT);
  page += FPSTR(HTTP_BTN_MAIN);
  ShowPage(page);

  snprintf_P(svalue, sizeof(svalue), PSTR(D_CMND_RESET " 1"));
  ExecuteWebCommand(svalue, SRC_WEBGUI);
}

void HandleRestoreConfiguration()
{
  if (HttpUser()) { return; }
  if (!WebAuthenticate()) { return WebServer->requestAuthentication(); }
  AddLog_P(LOG_LEVEL_DEBUG, S_LOG_HTTP, S_RESTORE_CONFIGURATION);

  String page = FPSTR(HTTP_HEAD);
  page.replace(F("{v}"), FPSTR(S_RESTORE_CONFIGURATION));
  page += FPSTR(HTTP_HEAD_STYLE);
  page += FPSTR(HTTP_FORM_RST);
  page += FPSTR(HTTP_FORM_RST_UPG);
  page.replace(F("{r1"), F(D_RESTORE));
  page += FPSTR(HTTP_BTN_CONF);
  ShowPage(page);

  upload_error = 0;
  upload_file_type = UPL_SETTINGS;
}



void HandleInformation()
{
  if (HttpUser()) { return; }
  if (!WebAuthenticate()) { return WebServer->requestAuthentication(); }
  AddLog_P(LOG_LEVEL_DEBUG, S_LOG_HTTP, S_INFORMATION);

  char stopic[TOPSZ];

  int freeMem = ESP.getFreeHeap();

  String page = FPSTR(HTTP_HEAD);
  page.replace(F("{v}"), FPSTR(S_INFORMATION));
  page += FPSTR(HTTP_HEAD_STYLE);


  page += F("<style>td{padding:0px 5px;}</style>");
  page += F("<div id='i' name='i'></div>");




  String func = FPSTR(HTTP_SCRIPT_INFO_BEGIN);
  func += F("<table style='width:100%'><tr><th>");
  func += F(D_PROGRAM_VERSION "}2"); func += my_version;
  func += F("}1" D_BUILD_DATE_AND_TIME "}2"); func += GetBuildDateAndTime();
  func += F("}1" D_CORE_AND_SDK_VERSION "}2" ARDUINO_ESP8266_RELEASE "/"); func += String(ESP.getSdkVersion());
  func += F("}1" D_UPTIME "}2"); func += GetUptime();
  snprintf_P(stopic, sizeof(stopic), PSTR(" at %X"), GetSettingsAddress());
  func += F("}1" D_FLASH_WRITE_COUNT "}2"); func += String(Settings.save_flag); func += stopic;
  func += F("}1" D_BOOT_COUNT "}2"); func += String(Settings.bootcount);
  func += F("}1" D_RESTART_REASON "}2"); func += GetResetReason();
  uint8_t maxfn = (devices_present > MAX_FRIENDLYNAMES) ? MAX_FRIENDLYNAMES : devices_present;
  if (SONOFF_IFAN02 == Settings.module) { maxfn = 1; }
  for (byte i = 0; i < maxfn; i++) {
    func += F("}1" D_FRIENDLY_NAME " "); func += i +1; func += F("}2"); func += Settings.friendlyname[i];
  }

  func += F("}1}2&nbsp;");
  func += F("}1" D_AP); func += String(Settings.sta_active +1);
    func += F(" " D_SSID " (" D_RSSI ")}2"); func += Settings.sta_ssid[Settings.sta_active]; func += F(" ("); func += WifiGetRssiAsQuality(WiFi.RSSI()); func += F("%)");
  func += F("}1" D_HOSTNAME "}2"); func += my_hostname;
  if (mdns_begun) { func += F(".local"); }
  if (static_cast<uint32_t>(WiFi.localIP()) != 0) {
    func += F("}1" D_IP_ADDRESS "}2"); func += WiFi.localIP().toString();
    func += F("}1" D_GATEWAY "}2"); func += IPAddress(Settings.ip_address[1]).toString();
    func += F("}1" D_SUBNET_MASK "}2"); func += IPAddress(Settings.ip_address[2]).toString();
    func += F("}1" D_DNS_SERVER "}2"); func += IPAddress(Settings.ip_address[3]).toString();
    func += F("}1" D_MAC_ADDRESS "}2"); func += WiFi.macAddress();
  }
  if (static_cast<uint32_t>(WiFi.softAPIP()) != 0) {
    func += F("}1" D_AP " " D_IP_ADDRESS "}2"); func += WiFi.softAPIP().toString();
    func += F("}1" D_AP " " D_GATEWAY "}2"); func += WiFi.softAPIP().toString();
    func += F("}1" D_AP " " D_MAC_ADDRESS "}2"); func += WiFi.softAPmacAddress();
  }

  func += F("}1}2&nbsp;");
  if (Settings.flag.mqtt_enabled) {
    func += F("}1" D_MQTT_HOST "}2"); func += Settings.mqtt_host;
    func += F("}1" D_MQTT_PORT "}2"); func += String(Settings.mqtt_port);
    func += F("}1" D_MQTT_CLIENT " &<br/>&nbsp;" D_FALLBACK_TOPIC "}2"); func += mqtt_client;
    func += F("}1" D_MQTT_USER "}2"); func += Settings.mqtt_user;
    func += F("}1" D_MQTT_TOPIC "}2"); func += Settings.mqtt_topic;
    func += F("}1" D_MQTT_GROUP_TOPIC "}2"); func += Settings.mqtt_grptopic;
    GetTopic_P(stopic, CMND, mqtt_topic, "");
    func += F("}1" D_MQTT_FULL_TOPIC "}2"); func += stopic;

  } else {
    func += F("}1" D_MQTT "}2" D_DISABLED);
  }

  func += F("}1}2&nbsp;");
  func += F("}1" D_EMULATION "}2");
#ifdef USE_EMULATION
  if (EMUL_WEMO == Settings.flag2.emulation) {
    func += F(D_BELKIN_WEMO);
  }
  else if (EMUL_HUE == Settings.flag2.emulation) {
    func += F(D_HUE_BRIDGE);
  }
  else {
    func += F(D_NONE);
  }
#else
  func += F(D_DISABLED);
#endif

  func += F("}1" D_MDNS_DISCOVERY "}2");
#ifdef USE_DISCOVERY
  func += F(D_ENABLED);
  func += F("}1" D_MDNS_ADVERTISE "}2");
#ifdef WEBSERVER_ADVERTISE
  func += F(D_WEB_SERVER);
#else
  func += F(D_DISABLED);
#endif
#else
  func += F(D_DISABLED);
#endif

  func += F("}1}2&nbsp;");
  func += F("}1" D_ESP_CHIP_ID "}2"); func += String(ESP.getChipId());
  func += F("}1" D_FLASH_CHIP_ID "}2"); func += String(ESP.getFlashChipId());
  func += F("}1" D_FLASH_CHIP_SIZE "}2"); func += String(ESP.getFlashChipRealSize() / 1024); func += F("kB");
  func += F("}1" D_PROGRAM_FLASH_SIZE "}2"); func += String(ESP.getFlashChipSize() / 1024); func += F("kB");
  func += F("}1" D_PROGRAM_SIZE "}2"); func += String(ESP.getSketchSize() / 1024); func += F("kB");
  func += F("}1" D_FREE_PROGRAM_SPACE "}2"); func += String(ESP.getFreeSketchSpace() / 1024); func += F("kB");
  func += F("}1" D_FREE_MEMORY "}2"); func += String(freeMem / 1024); func += F("kB");
  func += F("</td></tr></table>");
  func += FPSTR(HTTP_SCRIPT_INFO_END);
  page.replace(F("</script>"), func);
  page.replace(F("<body>"), F("<body onload='i()'>"));


  page += FPSTR(HTTP_BTN_MAIN);
  ShowPage(page);
}
#endif



void HandleUpgradeFirmware()
{
  if (HttpUser()) { return; }
  if (!WebAuthenticate()) { return WebServer->requestAuthentication(); }
  AddLog_P(LOG_LEVEL_DEBUG, S_LOG_HTTP, S_FIRMWARE_UPGRADE);

  String page = FPSTR(HTTP_HEAD);
  page.replace(F("{v}"), FPSTR(S_FIRMWARE_UPGRADE));
  page += FPSTR(HTTP_HEAD_STYLE);
  page += FPSTR(HTTP_FORM_UPG);
  page.replace(F("{o1"), Settings.ota_url);
  page += FPSTR(HTTP_FORM_RST_UPG);
  page.replace(F("{r1"), F(D_UPGRADE));
  page += FPSTR(HTTP_BTN_MAIN);
  ShowPage(page);

  upload_error = 0;
  upload_file_type = UPL_TASMOTA;
}

void HandleUpgradeFirmwareStart()
{
  if (HttpUser()) { return; }
  if (!WebAuthenticate()) { return WebServer->requestAuthentication(); }
  char svalue[100];

  AddLog_P(LOG_LEVEL_DEBUG, PSTR(D_LOG_HTTP D_UPGRADE_STARTED));
  WifiConfigCounter();

  char tmp[100];
  WebGetArg("o", tmp, sizeof(tmp));
  if (strlen(tmp)) {
    snprintf_P(svalue, sizeof(svalue), PSTR(D_CMND_OTAURL " %s"), tmp);
    ExecuteWebCommand(svalue, SRC_WEBGUI);
  }

  String page = FPSTR(HTTP_HEAD);
  page.replace(F("{v}"), FPSTR(S_INFORMATION));
  page += FPSTR(HTTP_HEAD_STYLE);
  page += F("<div style='text-align:center;'><b>" D_UPGRADE_STARTED " ...</b></div>");
  page += FPSTR(HTTP_MSG_RSTRT);
  page += FPSTR(HTTP_BTN_MAIN);
  ShowPage(page);

  snprintf_P(svalue, sizeof(svalue), PSTR(D_CMND_UPGRADE " 1"));
  ExecuteWebCommand(svalue, SRC_WEBGUI);
}

void HandleUploadDone()
{
  if (HttpUser()) { return; }
  if (!WebAuthenticate()) { return WebServer->requestAuthentication(); }
  AddLog_P(LOG_LEVEL_DEBUG, PSTR(D_LOG_HTTP D_UPLOAD_DONE));

  char error[100];

  WifiConfigCounter();
  restart_flag = 0;
  MqttRetryCounter(0);

  String page = FPSTR(HTTP_HEAD);
  page.replace(F("{v}"), FPSTR(S_INFORMATION));
  page += FPSTR(HTTP_HEAD_STYLE);
  page += F("<div style='text-align:center;'><b>" D_UPLOAD " <font color='");
  if (upload_error) {
    page += F("red'>" D_FAILED "</font></b><br/><br/>");
    switch (upload_error) {
      case 1: strncpy_P(error, PSTR(D_UPLOAD_ERR_1), sizeof(error)); break;
      case 2: strncpy_P(error, PSTR(D_UPLOAD_ERR_2), sizeof(error)); break;
      case 3: strncpy_P(error, PSTR(D_UPLOAD_ERR_3), sizeof(error)); break;
      case 4: strncpy_P(error, PSTR(D_UPLOAD_ERR_4), sizeof(error)); break;
      case 5: strncpy_P(error, PSTR(D_UPLOAD_ERR_5), sizeof(error)); break;
      case 6: strncpy_P(error, PSTR(D_UPLOAD_ERR_6), sizeof(error)); break;
      case 7: strncpy_P(error, PSTR(D_UPLOAD_ERR_7), sizeof(error)); break;
      case 8: strncpy_P(error, PSTR(D_UPLOAD_ERR_8), sizeof(error)); break;
      case 9: strncpy_P(error, PSTR(D_UPLOAD_ERR_9), sizeof(error)); break;
#ifdef USE_RF_FLASH
      case 10: strncpy_P(error, PSTR(D_UPLOAD_ERR_10), sizeof(error)); break;
      case 11: strncpy_P(error, PSTR(D_UPLOAD_ERR_11), sizeof(error)); break;
      case 12: strncpy_P(error, PSTR(D_UPLOAD_ERR_12), sizeof(error)); break;
      case 13: strncpy_P(error, PSTR(D_UPLOAD_ERR_13), sizeof(error)); break;
#endif
      default:
        snprintf_P(error, sizeof(error), PSTR(D_UPLOAD_ERROR_CODE " %d"), upload_error);
    }
    page += error;
    snprintf_P(log_data, sizeof(log_data), PSTR(D_UPLOAD ": %s"), error);
    AddLog(LOG_LEVEL_DEBUG);
    stop_flash_rotate = Settings.flag.stop_flash_rotate;
  } else {
    page += F("green'>" D_SUCCESSFUL "</font></b><br/>");
    page += FPSTR(HTTP_MSG_RSTRT);
    ShowWebSource(SRC_WEBGUI);
    restart_flag = 2;
  }
  SettingsBufferFree();
  page += F("</div><br/>");
  page += FPSTR(HTTP_BTN_MAIN);
  ShowPage(page);
}

void HandleUploadLoop()
{

  boolean _serialoutput = (LOG_LEVEL_DEBUG <= seriallog_level);

  if (HTTP_USER == webserver_state) { return; }
  if (upload_error) {
    if (UPL_TASMOTA == upload_file_type) { Update.end(); }
    return;
  }

  HTTPUpload& upload = WebServer->upload();

  if (UPLOAD_FILE_START == upload.status) {
    restart_flag = 60;
    if (0 == upload.filename.c_str()[0]) {
      upload_error = 1;
      return;
    }
    SettingsSave(1);
    snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_UPLOAD D_FILE " %s ..."), upload.filename.c_str());
    AddLog(LOG_LEVEL_INFO);
    if (UPL_SETTINGS == upload_file_type) {
      if (!SettingsBufferAlloc()) {
        upload_error = 2;
        return;
      }
    } else {
      MqttRetryCounter(60);
#ifdef USE_EMULATION
      UdpDisconnect();
#endif
#ifdef USE_ARILUX_RF
      AriluxRfDisable();
#endif
      if (Settings.flag.mqtt_enabled) MqttDisconnect();
      uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
      if (!Update.begin(maxSketchSpace)) {






        upload_error = 2;
        return;
      }
    }
    upload_progress_dot_count = 0;
  } else if (!upload_error && (UPLOAD_FILE_WRITE == upload.status)) {
    if (0 == upload.totalSize) {
      if (UPL_SETTINGS == upload_file_type) {
        config_block_count = 0;
      }
      else {
#ifdef USE_RF_FLASH
        if ((SONOFF_BRIDGE == Settings.module) && (upload.buf[0] == ':')) {
          Update.end();
          upload_file_type = UPL_EFM8BB1;

          upload_error = SnfBrUpdateInit();
          if (upload_error != 0) { return; }
        } else
#endif
        {
          if (upload.buf[0] != 0xE9) {
            upload_error = 3;
            return;
          }
          uint32_t bin_flash_size = ESP.magicFlashChipSize((upload.buf[3] & 0xf0) >> 4);
          if(bin_flash_size > ESP.getFlashChipRealSize()) {
            upload_error = 4;
            return;
          }
          upload.buf[2] = 3;
        }
      }
    }
    if (UPL_SETTINGS == upload_file_type) {
      if (!upload_error) {
        if (upload.currentSize > (sizeof(Settings) - (config_block_count * HTTP_UPLOAD_BUFLEN))) {
          upload_error = 9;
          return;
        }
        memcpy(settings_buffer + (config_block_count * HTTP_UPLOAD_BUFLEN), upload.buf, upload.currentSize);
        config_block_count++;
      }
    }
#ifdef USE_RF_FLASH
    else if (UPL_EFM8BB1 == upload_file_type) {
      if (efm8bb1_update != NULL) {
        ssize_t result = rf_glue_remnant_with_new_data_and_write(efm8bb1_update, upload.buf, upload.currentSize);
        free(efm8bb1_update);
        efm8bb1_update = NULL;
        if (result != 0) {
          upload_error = abs(result);
          return;
        }
      }
      ssize_t result = rf_search_and_write(upload.buf, upload.currentSize);
      if (result < 0) {
        upload_error = abs(result);
        return;
      } else if (result > 0) {
        if (result > upload.currentSize) {

          upload_error = 9;
          return;
        }

        size_t remnant_sz = upload.currentSize - result;
        efm8bb1_update = (uint8_t *) malloc(remnant_sz + 1);
        if (efm8bb1_update == NULL) {
          upload_error = 2;
          return;
        }
        memcpy(efm8bb1_update, upload.buf + result, remnant_sz);

        efm8bb1_update[remnant_sz] = '\0';
      }
    }
#endif
    else {
      if (!upload_error && (Update.write(upload.buf, upload.currentSize) != upload.currentSize)) {
        upload_error = 5;
        return;
      }
      if (_serialoutput) {
        Serial.printf(".");
        upload_progress_dot_count++;
        if (!(upload_progress_dot_count % 80)) { Serial.println(); }
      }
    }
  } else if(!upload_error && (UPLOAD_FILE_END == upload.status)) {
    if (_serialoutput && (upload_progress_dot_count % 80)) {
      Serial.println();
    }
    if (UPL_SETTINGS == upload_file_type) {
      if (config_xor_on_set) {
        for (uint16_t i = 2; i < sizeof(Settings); i++) {
          settings_buffer[i] ^= (config_xor_on_set +i);
        }
      }
      bool valid_settings = false;
      unsigned long buffer_version = settings_buffer[11] << 24 | settings_buffer[10] << 16 | settings_buffer[9] << 8 | settings_buffer[8];
      if (buffer_version > 0x06000000) {
        uint16_t buffer_size = settings_buffer[3] << 8 | settings_buffer[2];
        uint16_t buffer_crc = settings_buffer[15] << 8 | settings_buffer[14];
        uint16_t crc = 0;
        for (uint16_t i = 0; i < buffer_size; i++) {
          if ((i < 14) || (i > 15)) { crc += settings_buffer[i]*(i+1); }
        }
        valid_settings = (buffer_crc == crc);
      } else {
        valid_settings = (settings_buffer[0] == CONFIG_FILE_SIGN);
      }
      if (valid_settings) {
        SettingsDefaultSet2();
        memcpy((char*)&Settings +16, settings_buffer +16, sizeof(Settings) -16);
        Settings.version = buffer_version;
        SettingsBufferFree();
      } else {
        upload_error = 8;
        return;
      }
    }
#ifdef USE_RF_FLASH
    else if (UPL_EFM8BB1 == upload_file_type) {

      upload_file_type = UPL_TASMOTA;
    }
#endif
    else {
      if (!Update.end(true)) {
        if (_serialoutput) { Update.printError(Serial); }
        upload_error = 6;
        return;
      }
    }
    if (!upload_error) {
      snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_UPLOAD D_SUCCESSFUL " %u bytes. " D_RESTARTING), upload.totalSize);
      AddLog(LOG_LEVEL_INFO);
    }
  } else if (UPLOAD_FILE_ABORTED == upload.status) {
    restart_flag = 0;
    MqttRetryCounter(0);
    upload_error = 7;
    if (UPL_TASMOTA == upload_file_type) { Update.end(); }
  }
  delay(0);
}



void HandlePreflightRequest()
{
  WebServer->sendHeader(F("Access-Control-Allow-Origin"), F("*"));
  WebServer->sendHeader(F("Access-Control-Allow-Methods"), F("GET, POST"));
  WebServer->sendHeader(F("Access-Control-Allow-Headers"), F("authorization"));
  WebServer->send(200, FPSTR(HDR_CTYPE_HTML), "");
}



void HandleHttpCommand()
{
  if (HttpUser()) { return; }

  char svalue[INPUT_BUFFER_SIZE];

  AddLog_P(LOG_LEVEL_DEBUG, PSTR(D_LOG_HTTP D_COMMAND));

  uint8_t valid = 1;
  if (Settings.web_password[0] != 0) {
    char tmp1[100];
    WebGetArg("user", tmp1, sizeof(tmp1));
    char tmp2[100];
    WebGetArg("password", tmp2, sizeof(tmp2));
    if (!(!strcmp(tmp1, WEB_USERNAME) && !strcmp(tmp2, Settings.web_password))) { valid = 0; }
  }

  String message = F("{\"" D_RSLT_WARNING "\":\"");
  if (valid) {
    byte curridx = web_log_index;
    WebGetArg("cmnd", svalue, sizeof(svalue));
    if (strlen(svalue)) {
      ExecuteWebCommand(svalue, SRC_WEBCOMMAND);

      if (web_log_index != curridx) {
        byte counter = curridx;
        message = F("{");
        do {
          char* tmp;
          size_t len;
          GetLog(counter, &tmp, &len);
          if (len) {

            char* JSON = (char*)memchr(tmp, '{', len);
            if (JSON) {
              if (message.length() > 1) { message += F(","); }
              size_t JSONlen = len - (JSON - tmp);
              strlcpy(mqtt_data, JSON +1, JSONlen -2);
              message += mqtt_data;
            }
          }
          counter++;
          if (!counter) counter++;
        } while (counter != web_log_index);
        message += F("}");
      } else {
        message += F(D_ENABLE_WEBLOG_FOR_RESPONSE "\"}");
      }
    } else {
      message += F(D_ENTER_COMMAND " cmnd=\"}");
    }
  } else {
    message += F(D_NEED_USER_AND_PASSWORD "\"}");
  }
  SetHeader();
  WebServer->send(200, FPSTR(HDR_CTYPE_JSON), message);
}



void HandleConsole()
{
  if (HttpUser()) { return; }
  if (!WebAuthenticate()) { return WebServer->requestAuthentication(); }
  AddLog_P(LOG_LEVEL_DEBUG, S_LOG_HTTP, S_CONSOLE);

  String page = FPSTR(HTTP_HEAD);
  page.replace(F("{v}"), FPSTR(S_CONSOLE));
  page += FPSTR(HTTP_HEAD_STYLE);
  page.replace(F("</script>"), FPSTR(HTTP_SCRIPT_CONSOL));
  page.replace(F("<body>"), F("<body onload='l()'>"));
  page += FPSTR(HTTP_FORM_CMND);
  page += FPSTR(HTTP_BTN_MAIN);
  ShowPage(page);
}

void HandleAjaxConsoleRefresh()
{
  if (HttpUser()) { return; }
  if (!WebAuthenticate()) { return WebServer->requestAuthentication(); }
  char svalue[INPUT_BUFFER_SIZE];
  byte cflg = 1;
  byte counter = 0;

  WebGetArg("c1", svalue, sizeof(svalue));
  if (strlen(svalue)) {
    snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_COMMAND "%s"), svalue);
    AddLog(LOG_LEVEL_INFO);
    ExecuteWebCommand(svalue, SRC_WEBCONSOLE);
  }

  WebGetArg("c2", svalue, sizeof(svalue));
  if (strlen(svalue)) { counter = atoi(svalue); }

  byte last_reset_web_log_flag = reset_web_log_flag;
  String message = F("}9");
  if (!reset_web_log_flag) {
    counter = 0;
    reset_web_log_flag = 1;
  }
  if (counter != web_log_index) {
    if (!counter) {
      counter = web_log_index;
      cflg = 0;
    }
    do {
      char* tmp;
      size_t len;
      GetLog(counter, &tmp, &len);
      if (len) {
        if (cflg) {
          message += F("\n");
        } else {
          cflg = 1;
        }
        strlcpy(mqtt_data, tmp, len);
        message += mqtt_data;
      }
      counter++;
      if (!counter) { counter++; }
    } while (counter != web_log_index);

    message.replace(F("%"), F("%25"));
    message.replace(F("&"), F("%26"));
    message.replace(F("<"), F("%3C"));
    message.replace(F(">"), F("%3E"));
  }
  snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("<r><i>%d</i><j>%d</j><l>"), web_log_index, last_reset_web_log_flag);
  message.replace(F("}9"), mqtt_data);
  message += F("</l></r>");
  WebServer->send(200, FPSTR(HDR_CTYPE_XML), message);
}



void HandleNotFound()
{



  if (CaptivePortal()) { return; }

#ifdef USE_EMULATION
  String path = WebServer->uri();
  if ((EMUL_HUE == Settings.flag2.emulation) && (path.startsWith("/api"))) {
    HandleHueApi(&path);
  } else
#endif
  {
    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR(D_FILE_NOT_FOUND "\n\nURI: %s\nMethod: %s\nArguments: %d\n"),
      WebServer->uri().c_str(), (WebServer->method() == HTTP_GET) ? "GET" : "POST", WebServer->args());
    for (uint8_t i = 0; i < WebServer->args(); i++) {
      snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s %s: %s\n"), mqtt_data, WebServer->argName(i).c_str(), WebServer->arg(i).c_str());
    }
    SetHeader();
    WebServer->send(404, FPSTR(HDR_CTYPE_PLAIN), mqtt_data);
  }
}


boolean CaptivePortal()
{
  if ((HTTP_MANAGER == webserver_state) && !ValidIpAddress(WebServer->hostHeader())) {
    AddLog_P(LOG_LEVEL_DEBUG, PSTR(D_LOG_HTTP D_REDIRECTED));

    WebServer->sendHeader(F("Location"), String("http://") + WebServer->client().localIP().toString(), true);
    WebServer->send(302, FPSTR(HDR_CTYPE_PLAIN), "");
    WebServer->client().stop();
    return true;
  }
  return false;
}


boolean ValidIpAddress(String str)
{
  for (uint16_t i = 0; i < str.length(); i++) {
    int c = str.charAt(i);
    if (c != '.' && (c < '0' || c > '9')) { return false; }
  }
  return true;
}



String UrlEncode(const String& text)
{
  const char hex[] = "0123456789ABCDEF";

 String encoded = "";
 int len = text.length();
 int i = 0;
 while (i < len) {
  char decodedChar = text.charAt(i++);
# 1854 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xdrv_01_webserver.ino"
    if (' ' == decodedChar) {
      encoded += '%';
   encoded += hex[decodedChar >> 4];
   encoded += hex[decodedChar & 0xF];
    } else {
      encoded += decodedChar;
    }

 }
 return encoded;
}

int WebSend(char *buffer)
{




  char *host;
  char *port;
  char *user;
  char *password;
  char *command;
  uint16_t nport = 80;
  int status = 1;

  host = strtok_r(buffer, "]", &command);
  if (host && command) {
    host = LTrim(host);
    host++;
    host = strtok_r(host, ",", &user);
    host = strtok_r(host, ":", &port);
    if (user) {
      user = strtok_r(user, ":", &password);
    }




    if (port) { nport = atoi(port); }

    String nuri = "";
    if (user && password) {
      nuri += F("user=");
      nuri += user;
      nuri += F("&password=");
      nuri += password;
      nuri += F("&");
    }
    nuri += F("cmnd=");
    nuri += LTrim(command);
    String uri = UrlEncode(nuri);

    IPAddress host_ip;
    if (WiFi.hostByName(host, host_ip)) {
      WiFiClient client;

      bool connected = false;
      byte retry = 2;
      while ((retry > 0) && !connected) {
        --retry;
        connected = client.connect(host_ip, nport);
        if (connected) break;
      }

      if (connected) {
        String url = F("GET /cm?");
        url += uri;
        url += F(" HTTP/1.1\r\n Host: ");
        url += IPAddress(host_ip).toString();
        if (port) {
          url += F(" \r\n Port: ");
          url += port;
        }
        url += F(" \r\n Connection: close\r\n\r\n");




        client.print(url.c_str());
        client.flush();
        client.stop();
        status = 0;
      } else {
        status = 2;
      }
    } else {
      status = 3;
    }
  }
  return status;
}



enum WebCommands { CMND_WEBSERVER, CMND_WEBPASSWORD, CMND_WEBLOG, CMND_WEBREFRESH, CMND_WEBSEND, CMND_EMULATION };
const char kWebCommands[] PROGMEM = D_CMND_WEBSERVER "|" D_CMND_WEBPASSWORD "|" D_CMND_WEBLOG "|" D_CMND_WEBREFRESH "|" D_CMND_WEBSEND "|" D_CMND_EMULATION ;
const char kWebSendStatus[] PROGMEM = D_JSON_DONE "|" D_JSON_WRONG_PARAMETERS "|" D_JSON_CONNECT_FAILED "|" D_JSON_HOST_NOT_FOUND ;

bool WebCommand()
{
  char command[CMDSZ];
  bool serviced = true;

  int command_code = GetCommandCode(command, sizeof(command), XdrvMailbox.topic, kWebCommands);
  if (-1 == command_code) {
    serviced = false;
  }
  if (CMND_WEBSERVER == command_code) {
    if ((XdrvMailbox.payload >= 0) && (XdrvMailbox.payload <= 2)) { Settings.webserver = XdrvMailbox.payload; }
    if (Settings.webserver) {
      snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"" D_CMND_WEBSERVER "\":\"" D_JSON_ACTIVE_FOR " %s " D_JSON_ON_DEVICE " %s " D_JSON_WITH_IP_ADDRESS " %s\"}"),
        (2 == Settings.webserver) ? D_ADMIN : D_USER, my_hostname, WiFi.localIP().toString().c_str());
    } else {
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_SVALUE, command, GetStateText(0));
    }
  }
  else if (CMND_WEBPASSWORD == command_code) {
    if ((XdrvMailbox.data_len > 0) && (XdrvMailbox.data_len < sizeof(Settings.web_password))) {
      strlcpy(Settings.web_password, (SC_CLEAR == Shortcut(XdrvMailbox.data)) ? "" : (SC_DEFAULT == Shortcut(XdrvMailbox.data)) ? WEB_PASSWORD : XdrvMailbox.data, sizeof(Settings.web_password));
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_SVALUE, command, Settings.web_password);
    } else {
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_ASTERIX, command);
    }
  }
  else if (CMND_WEBLOG == command_code) {
    if ((XdrvMailbox.payload >= LOG_LEVEL_NONE) && (XdrvMailbox.payload <= LOG_LEVEL_ALL)) { Settings.weblog_level = XdrvMailbox.payload; }
    snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_NVALUE, command, Settings.weblog_level);
  }
  else if (CMND_WEBREFRESH == command_code) {
    if ((XdrvMailbox.payload > 999) && (XdrvMailbox.payload <= 10000)) { Settings.web_refresh = XdrvMailbox.payload; }
    snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_NVALUE, command, Settings.web_refresh);
  }
  else if (CMND_WEBSEND == command_code) {
    if (XdrvMailbox.data_len > 0) {
      uint8_t result = WebSend(XdrvMailbox.data);
      char stemp1[20];
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_SVALUE, command, GetTextIndexed(stemp1, sizeof(stemp1), result, kWebSendStatus));
    }
  }
#ifdef USE_EMULATION
  else if (CMND_EMULATION == command_code) {
    if ((XdrvMailbox.payload >= EMUL_NONE) && (XdrvMailbox.payload < EMUL_MAX)) {
      Settings.flag2.emulation = XdrvMailbox.payload;
      restart_flag = 2;
    }
    snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_NVALUE, command, Settings.flag2.emulation);
  }
#endif
  else serviced = false;

  return serviced;
}





#define XDRV_01 

boolean Xdrv01(byte function)
{
  boolean result = false;

  switch (function) {
    case FUNC_LOOP:
      PollDnsWebserver();
#ifdef USE_EMULATION
      if (Settings.flag2.emulation) PollUdp();
#endif
      break;
    case FUNC_COMMAND:
      result = WebCommand();
      break;
  }
  return result;
}
#endif
# 1 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xdrv_02_mqtt.ino"
# 30 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xdrv_02_mqtt.ino"
#if (MQTT_LIBRARY_TYPE == MQTT_ESPMQTTARDUINO)
#undef MQTT_LIBRARY_TYPE
#define MQTT_LIBRARY_TYPE MQTT_ARDUINOMQTT
#endif
# 42 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xdrv_02_mqtt.ino"
#ifdef USE_MQTT_TLS

#if (MQTT_LIBRARY_TYPE == MQTT_TASMOTAMQTT)
#undef MQTT_LIBRARY_TYPE
#endif

#ifndef MQTT_LIBRARY_TYPE
#define MQTT_LIBRARY_TYPE MQTT_PUBSUBCLIENT
#endif

#endif



enum MqttCommands {
  CMND_MQTTHOST, CMND_MQTTPORT, CMND_MQTTRETRY, CMND_STATETEXT, CMND_MQTTFINGERPRINT, CMND_MQTTCLIENT,
  CMND_MQTTUSER, CMND_MQTTPASSWORD, CMND_FULLTOPIC, CMND_PREFIX, CMND_GROUPTOPIC, CMND_TOPIC, CMND_PUBLISH,
  CMND_BUTTONTOPIC, CMND_SWITCHTOPIC, CMND_BUTTONRETAIN, CMND_SWITCHRETAIN, CMND_POWERRETAIN, CMND_SENSORRETAIN };
const char kMqttCommands[] PROGMEM =
  D_CMND_MQTTHOST "|" D_CMND_MQTTPORT "|" D_CMND_MQTTRETRY "|" D_CMND_STATETEXT "|" D_CMND_MQTTFINGERPRINT "|" D_CMND_MQTTCLIENT "|"
  D_CMND_MQTTUSER "|" D_CMND_MQTTPASSWORD "|" D_CMND_FULLTOPIC "|" D_CMND_PREFIX "|" D_CMND_GROUPTOPIC "|" D_CMND_TOPIC "|" D_CMND_PUBLISH "|"
  D_CMND_BUTTONTOPIC "|" D_CMND_SWITCHTOPIC "|" D_CMND_BUTTONRETAIN "|" D_CMND_SWITCHRETAIN "|" D_CMND_POWERRETAIN "|" D_CMND_SENSORRETAIN ;

uint8_t mqtt_retry_counter = 1;
uint8_t mqtt_initial_connection_state = 2;
bool mqtt_connected = false;
# 79 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xdrv_02_mqtt.ino"
#if (MQTT_LIBRARY_TYPE == MQTT_PUBSUBCLIENT)

#include <PubSubClient.h>


#if (MQTT_MAX_PACKET_SIZE -TOPSZ -7) < MIN_MESSZ
  #error "MQTT_MAX_PACKET_SIZE is too small in libraries/PubSubClient/src/PubSubClient.h, increase it to at least 1000"
#endif

PubSubClient MqttClient(EspClient);

bool MqttIsConnected()
{
  return MqttClient.connected();
}

void MqttDisconnect()
{
  MqttClient.disconnect();
}

void MqttSubscribeLib(char *topic)
{
  MqttClient.subscribe(topic);
  MqttClient.loop();
}

bool MqttPublishLib(const char* topic, boolean retained)
{
  bool result = MqttClient.publish(topic, mqtt_data, retained);
  yield();
  return result;
}

void MqttLoop()
{
  MqttClient.loop();
}

#elif (MQTT_LIBRARY_TYPE == MQTT_TASMOTAMQTT)

#include <TasmotaMqtt.h>
TasmotaMqtt MqttClient;

bool MqttIsConnected()
{
  return MqttClient.Connected();
}

void MqttDisconnect()
{
  MqttClient.Disconnect();
}

void MqttDisconnectedCb()
{
  MqttDisconnected(MqttClient.State());
}

void MqttSubscribeLib(char *topic)
{
  MqttClient.Subscribe(topic, 0);
}

bool MqttPublishLib(const char* topic, boolean retained)
{
  return MqttClient.Publish(topic, mqtt_data, strlen(mqtt_data), 0, retained);
}

void MqttLoop()
{
}

#elif (MQTT_LIBRARY_TYPE == MQTT_ARDUINOMQTT)

#include <MQTTClient.h>
MQTTClient MqttClient(MQTT_MAX_PACKET_SIZE);

bool MqttIsConnected()
{
  return MqttClient.connected();
}

void MqttDisconnect()
{
  MqttClient.disconnect();
}
# 175 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xdrv_02_mqtt.ino"
void MqttMyDataCb(String &topic, String &data)
{
  MqttDataHandler((char*)topic.c_str(), (byte*)data.c_str(), data.length());
}

void MqttSubscribeLib(char *topic)
{
  MqttClient.subscribe(topic, 0);
}

bool MqttPublishLib(const char* topic, boolean retained)
{
  return MqttClient.publish(topic, mqtt_data, strlen(mqtt_data), retained, 0);
}

void MqttLoop()
{
  MqttClient.loop();

}

#endif



#ifdef USE_DISCOVERY
#ifdef MQTT_HOST_DISCOVERY
boolean MqttDiscoverServer()
{
  if (!mdns_begun) { return false; }

  int n = MDNS.queryService("mqtt", "tcp");

  snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_MDNS D_QUERY_DONE " %d"), n);
  AddLog(LOG_LEVEL_INFO);

  if (n > 0) {

    snprintf_P(Settings.mqtt_host, sizeof(Settings.mqtt_host), MDNS.IP(0).toString().c_str());
    Settings.mqtt_port = MDNS.port(0);

    snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_MDNS D_MQTT_SERVICE_FOUND " %s, " D_IP_ADDRESS " %s, " D_PORT " %d"),
      MDNS.hostname(0).c_str(), Settings.mqtt_host, Settings.mqtt_port);
    AddLog(LOG_LEVEL_INFO);
  }

  return n > 0;
}
#endif
#endif

int MqttLibraryType()
{
  return (int)MQTT_LIBRARY_TYPE;
}

void MqttRetryCounter(uint8_t value)
{
  mqtt_retry_counter = value;
}

void MqttSubscribe(char *topic)
{
  snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_MQTT D_SUBSCRIBE_TO " %s"), topic);
  AddLog(LOG_LEVEL_DEBUG);
  MqttSubscribeLib(topic);
}

void MqttPublishDirect(const char* topic, boolean retained)
{
  char sretained[CMDSZ];
  char slog_type[10];

  ShowFreeMem(PSTR("MqttPublishDirect"));

  sretained[0] = '\0';
  snprintf_P(slog_type, sizeof(slog_type), PSTR(D_LOG_RESULT));

  if (Settings.flag.mqtt_enabled) {
    if (MqttIsConnected()) {
      if (MqttPublishLib(topic, retained)) {
        snprintf_P(slog_type, sizeof(slog_type), PSTR(D_LOG_MQTT));
        if (retained) {
          snprintf_P(sretained, sizeof(sretained), PSTR(" (" D_RETAINED ")"));
        }
      }
    }
  }

  snprintf_P(log_data, sizeof(log_data), PSTR("%s%s = %s"), slog_type, (Settings.flag.mqtt_enabled) ? topic : strrchr(topic,'/')+1, mqtt_data);
  if (strlen(log_data) >= (sizeof(log_data) - strlen(sretained) -1)) {
    log_data[sizeof(log_data) - strlen(sretained) -5] = '\0';
    snprintf_P(log_data, sizeof(log_data), PSTR("%s ..."), log_data);
  }
  snprintf_P(log_data, sizeof(log_data), PSTR("%s%s"), log_data, sretained);

  AddLog(LOG_LEVEL_INFO);
  if (Settings.ledstate &0x04) {
    blinks++;
  }
}

void MqttPublish(const char* topic, boolean retained)
{
  char *me;

  if (!strcmp(Settings.mqtt_prefix[0],Settings.mqtt_prefix[1])) {
    me = strstr(topic,Settings.mqtt_prefix[0]);
    if (me == topic) {
      mqtt_cmnd_publish += 3;
    }
  }
  MqttPublishDirect(topic, retained);
}

void MqttPublish(const char* topic)
{
  MqttPublish(topic, false);
}

void MqttPublishPrefixTopic_P(uint8_t prefix, const char* subtopic, boolean retained)
{







  char romram[33];
  char stopic[TOPSZ];

  snprintf_P(romram, sizeof(romram), ((prefix > 3) && !Settings.flag.mqtt_response) ? S_RSLT_RESULT : subtopic);
  for (byte i = 0; i < strlen(romram); i++) {
    romram[i] = toupper(romram[i]);
  }
  prefix &= 3;
  GetTopic_P(stopic, prefix, mqtt_topic, romram);
  MqttPublish(stopic, retained);
}

void MqttPublishPrefixTopic_P(uint8_t prefix, const char* subtopic)
{
  MqttPublishPrefixTopic_P(prefix, subtopic, false);
}

void MqttPublishPowerState(byte device)
{
  char stopic[TOPSZ];
  char scommand[33];

  if ((device < 1) || (device > devices_present)) { device = 1; }

  if ((SONOFF_IFAN02 == Settings.module) && (device > 1)) {
    if (GetFanspeed() < 4) {
      snprintf_P(scommand, sizeof(scommand), PSTR(D_CMND_FANSPEED));
      GetTopic_P(stopic, STAT, mqtt_topic, (Settings.flag.mqtt_response) ? scommand : S_RSLT_RESULT);
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_NVALUE, scommand, GetFanspeed());
      MqttPublish(stopic);
    }
  } else {
    GetPowerDevice(scommand, device, sizeof(scommand), Settings.flag.device_index_enable);
    GetTopic_P(stopic, STAT, mqtt_topic, (Settings.flag.mqtt_response) ? scommand : S_RSLT_RESULT);
    snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_SVALUE, scommand, GetStateText(bitRead(power, device -1)));
    MqttPublish(stopic);

    GetTopic_P(stopic, STAT, mqtt_topic, scommand);
    snprintf_P(mqtt_data, sizeof(mqtt_data), GetStateText(bitRead(power, device -1)));
    MqttPublish(stopic, Settings.flag.mqtt_power_retain);
  }
}

void MqttPublishPowerBlinkState(byte device)
{
  char scommand[33];

  if ((device < 1) || (device > devices_present)) {
    device = 1;
  }
  snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"%s\":\"" D_JSON_BLINK " %s\"}"),
    GetPowerDevice(scommand, device, sizeof(scommand), Settings.flag.device_index_enable), GetStateText(bitRead(blink_mask, device -1)));

  MqttPublishPrefixTopic_P(RESULT_OR_STAT, S_RSLT_POWER);
}



void MqttDisconnected(int state)
{
  mqtt_connected = false;
  mqtt_retry_counter = Settings.mqtt_retry;

  snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_MQTT D_CONNECT_FAILED_TO " %s:%d, rc %d. " D_RETRY_IN " %d " D_UNIT_SECOND),
    Settings.mqtt_host, Settings.mqtt_port, state, mqtt_retry_counter);
  AddLog(LOG_LEVEL_INFO);
  rules_flag.mqtt_disconnected = 1;
}

void MqttConnected()
{
  char stopic[TOPSZ];

  if (Settings.flag.mqtt_enabled) {
    AddLog_P(LOG_LEVEL_INFO, S_LOG_MQTT, PSTR(D_CONNECTED));
    mqtt_connected = true;
    mqtt_retry_counter = 0;

    GetTopic_P(stopic, TELE, mqtt_topic, S_LWT);
    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR(D_ONLINE));
    MqttPublish(stopic, true);


    mqtt_data[0] = '\0';
    MqttPublishPrefixTopic_P(CMND, S_RSLT_POWER);

    GetTopic_P(stopic, CMND, mqtt_topic, PSTR("#"));
    MqttSubscribe(stopic);
    if (strstr(Settings.mqtt_fulltopic, MQTT_TOKEN_TOPIC) != NULL) {
      GetTopic_P(stopic, CMND, Settings.mqtt_grptopic, PSTR("#"));
      MqttSubscribe(stopic);
      fallback_topic_flag = 1;
      GetTopic_P(stopic, CMND, mqtt_client, PSTR("#"));
      fallback_topic_flag = 0;
      MqttSubscribe(stopic);
    }

    XdrvCall(FUNC_MQTT_SUBSCRIBE);
  }

  if (mqtt_initial_connection_state) {
    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"" D_CMND_MODULE "\":\"%s\",\"" D_JSON_VERSION "\":\"%s\",\"" D_JSON_FALLBACKTOPIC "\":\"%s\",\"" D_CMND_GROUPTOPIC "\":\"%s\"}"),
      my_module.name, my_version, mqtt_client, Settings.mqtt_grptopic);
    MqttPublishPrefixTopic_P(TELE, PSTR(D_RSLT_INFO "1"));
#ifdef USE_WEBSERVER
    if (Settings.webserver) {
      snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"" D_JSON_WEBSERVER_MODE "\":\"%s\",\"" D_CMND_HOSTNAME "\":\"%s\",\"" D_CMND_IPADDRESS "\":\"%s\"}"),
        (2 == Settings.webserver) ? D_ADMIN : D_USER, my_hostname, WiFi.localIP().toString().c_str());
      MqttPublishPrefixTopic_P(TELE, PSTR(D_RSLT_INFO "2"));
    }
#endif
    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"" D_JSON_RESTARTREASON "\":\"%s\"}"),
      (GetResetReason() == "Exception") ? ESP.getResetInfo().c_str() : GetResetReason().c_str());
    MqttPublishPrefixTopic_P(TELE, PSTR(D_RSLT_INFO "3"));
    for (byte i = 1; i <= devices_present; i++) {
      MqttPublishPowerState(i);
      if (SONOFF_IFAN02 == Settings.module) { break; }
    }
    if (Settings.tele_period) { tele_period = Settings.tele_period -9; }
    rules_flag.system_boot = 1;
    XdrvCall(FUNC_MQTT_INIT);
  }
  mqtt_initial_connection_state = 0;

  global_state.mqtt_down = 0;
  if (Settings.flag.mqtt_enabled) {
    rules_flag.mqtt_connected = 1;
  }
}

#ifdef USE_MQTT_TLS
boolean MqttCheckTls()
{
  char fingerprint1[60];
  char fingerprint2[60];
  boolean result = false;

  fingerprint1[0] = '\0';
  fingerprint2[0] = '\0';
  for (byte i = 0; i < sizeof(Settings.mqtt_fingerprint[0]); i++) {
    snprintf_P(fingerprint1, sizeof(fingerprint1), PSTR("%s%s%02X"), fingerprint1, (i) ? " " : "", Settings.mqtt_fingerprint[0][i]);
    snprintf_P(fingerprint2, sizeof(fingerprint2), PSTR("%s%s%02X"), fingerprint2, (i) ? " " : "", Settings.mqtt_fingerprint[1][i]);
  }

  AddLog_P(LOG_LEVEL_INFO, S_LOG_MQTT, PSTR(D_FINGERPRINT));


  EspClient = WiFiClientSecure();


  if (!EspClient.connect(Settings.mqtt_host, Settings.mqtt_port)) {
    snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_MQTT D_TLS_CONNECT_FAILED_TO " %s:%d. " D_RETRY_IN " %d " D_UNIT_SECOND),
      Settings.mqtt_host, Settings.mqtt_port, mqtt_retry_counter);
    AddLog(LOG_LEVEL_DEBUG);
  } else {
    if (EspClient.verify(fingerprint1, Settings.mqtt_host)) {
      AddLog_P(LOG_LEVEL_INFO, S_LOG_MQTT, PSTR(D_VERIFIED "1"));
      result = true;
    }
    else if (EspClient.verify(fingerprint2, Settings.mqtt_host)) {
      AddLog_P(LOG_LEVEL_INFO, S_LOG_MQTT, PSTR(D_VERIFIED "2"));
      result = true;
    }
  }
  if (!result) AddLog_P(LOG_LEVEL_INFO, S_LOG_MQTT, PSTR(D_FAILED));
  EspClient.stop();
  yield();
  return result;
}
#endif

void MqttReconnect()
{
  char stopic[TOPSZ];

  if (!Settings.flag.mqtt_enabled) {
    MqttConnected();
    return;
  }

#if defined(USE_WEBSERVER) && defined(USE_EMULATION)
  UdpDisconnect();
#endif

  AddLog_P(LOG_LEVEL_INFO, S_LOG_MQTT, PSTR(D_ATTEMPTING_CONNECTION));

  mqtt_connected = false;
  mqtt_retry_counter = Settings.mqtt_retry;
  global_state.mqtt_down = 1;

#ifndef USE_MQTT_TLS
#ifdef USE_DISCOVERY
#ifdef MQTT_HOST_DISCOVERY
  if (!strlen(Settings.mqtt_host) && !MqttDiscoverServer()) { return; }
#endif
#endif
#endif

  char *mqtt_user = NULL;
  char *mqtt_pwd = NULL;
  if (strlen(Settings.mqtt_user) > 0) mqtt_user = Settings.mqtt_user;
  if (strlen(Settings.mqtt_pwd) > 0) mqtt_pwd = Settings.mqtt_pwd;

  GetTopic_P(stopic, TELE, mqtt_topic, S_LWT);
  snprintf_P(mqtt_data, sizeof(mqtt_data), S_OFFLINE);


#ifdef USE_MQTT_TLS
  EspClient = WiFiClientSecure();
#else
  EspClient = WiFiClient();
#endif


  if (2 == mqtt_initial_connection_state) {
#ifdef USE_MQTT_TLS
    if (!MqttCheckTls()) return;
#endif

#if (MQTT_LIBRARY_TYPE == MQTT_TASMOTAMQTT)
    MqttClient.InitConnection(Settings.mqtt_host, Settings.mqtt_port);
    MqttClient.InitClient(mqtt_client, mqtt_user, mqtt_pwd, MQTT_KEEPALIVE);
    MqttClient.InitLWT(stopic, mqtt_data, 1, true);
    MqttClient.OnConnected(MqttConnected);
    MqttClient.OnDisconnected(MqttDisconnectedCb);
    MqttClient.OnData(MqttDataHandler);
#elif (MQTT_LIBRARY_TYPE == MQTT_ARDUINOMQTT)
    MqttClient.begin(Settings.mqtt_host, Settings.mqtt_port, EspClient);
    MqttClient.setWill(stopic, mqtt_data, true, 1);
    MqttClient.setOptions(MQTT_KEEPALIVE, true, MQTT_TIMEOUT);

    MqttClient.onMessage(MqttMyDataCb);
#endif

    mqtt_initial_connection_state = 1;
  }

#if (MQTT_LIBRARY_TYPE == MQTT_PUBSUBCLIENT)
  MqttClient.setCallback(MqttDataHandler);
  MqttClient.setServer(Settings.mqtt_host, Settings.mqtt_port);
  if (MqttClient.connect(mqtt_client, mqtt_user, mqtt_pwd, stopic, 1, true, mqtt_data)) {
    MqttConnected();
  } else {
    MqttDisconnected(MqttClient.state());
  }
#elif (MQTT_LIBRARY_TYPE == MQTT_TASMOTAMQTT)
  MqttClient.Connect();
#elif (MQTT_LIBRARY_TYPE == MQTT_ARDUINOMQTT)
  if (MqttClient.connect(mqtt_client, mqtt_user, mqtt_pwd)) {
    MqttConnected();
  } else {
    MqttDisconnected(MqttClient.lastError());
  }
#endif
}

void MqttCheck()
{
  if (Settings.flag.mqtt_enabled) {
    if (!MqttIsConnected()) {
      global_state.mqtt_down = 1;
      if (!mqtt_retry_counter) {
#ifndef USE_MQTT_TLS
#ifdef USE_DISCOVERY
#ifdef MQTT_HOST_DISCOVERY
        if (!strlen(Settings.mqtt_host) && !mdns_begun) { return; }
#endif
#endif
#endif
        MqttReconnect();
      } else {
        mqtt_retry_counter--;
      }
    } else {
      global_state.mqtt_down = 0;
    }
  } else {
    global_state.mqtt_down = 0;
    if (mqtt_initial_connection_state) MqttReconnect();
  }
}



bool MqttCommand()
{
  char command [CMDSZ];
  bool serviced = true;
  char stemp1[TOPSZ];
  char scommand[CMDSZ];
  uint16_t i;

  uint16_t index = XdrvMailbox.index;
  uint16_t data_len = XdrvMailbox.data_len;
  uint16_t payload16 = XdrvMailbox.payload16;
  int16_t payload = XdrvMailbox.payload;
  uint8_t grpflg = XdrvMailbox.grpflg;
  char *type = XdrvMailbox.topic;
  char *dataBuf = XdrvMailbox.data;

  int command_code = GetCommandCode(command, sizeof(command), type, kMqttCommands);
  if (-1 == command_code) {
    serviced = false;
  }
  else if (CMND_MQTTHOST == command_code) {
    if ((data_len > 0) && (data_len < sizeof(Settings.mqtt_host))) {
      strlcpy(Settings.mqtt_host, (SC_CLEAR == Shortcut(dataBuf)) ? "" : (SC_DEFAULT == Shortcut(dataBuf)) ? MQTT_HOST : dataBuf, sizeof(Settings.mqtt_host));
      restart_flag = 2;
    }
    snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_SVALUE, command, Settings.mqtt_host);
  }
  else if (CMND_MQTTPORT == command_code) {
    if (payload16 > 0) {
      Settings.mqtt_port = (1 == payload16) ? MQTT_PORT : payload16;
      restart_flag = 2;
    }
    snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_NVALUE, command, Settings.mqtt_port);
  }
  else if (CMND_MQTTRETRY == command_code) {
    if ((payload >= MQTT_RETRY_SECS) && (payload < 32001)) {
      Settings.mqtt_retry = payload;
      mqtt_retry_counter = Settings.mqtt_retry;
    }
    snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_NVALUE, command, Settings.mqtt_retry);
  }
  else if ((CMND_STATETEXT == command_code) && (index > 0) && (index <= 4)) {
    if ((data_len > 0) && (data_len < sizeof(Settings.state_text[0]))) {
      for(i = 0; i <= data_len; i++) {
        if (dataBuf[i] == ' ') dataBuf[i] = '_';
      }
      strlcpy(Settings.state_text[index -1], dataBuf, sizeof(Settings.state_text[0]));
    }
    snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_INDEX_SVALUE, command, index, GetStateText(index -1));
  }
#ifdef USE_MQTT_TLS
  else if ((CMND_MQTTFINGERPRINT == command_code) && (index > 0) && (index <= 2)) {
    char fingerprint[60];
    if ((data_len > 0) && (data_len < sizeof(fingerprint))) {
      strlcpy(fingerprint, (SC_CLEAR == Shortcut(dataBuf)) ? "" : (SC_DEFAULT == Shortcut(dataBuf)) ? (1 == index) ? MQTT_FINGERPRINT1 : MQTT_FINGERPRINT2 : dataBuf, sizeof(fingerprint));
      char *p = fingerprint;
      for (byte i = 0; i < 20; i++) {
        Settings.mqtt_fingerprint[index -1][i] = strtol(p, &p, 16);
      }
      restart_flag = 2;
    }
    fingerprint[0] = '\0';
    for (byte i = 0; i < sizeof(Settings.mqtt_fingerprint[index -1]); i++) {
      snprintf_P(fingerprint, sizeof(fingerprint), PSTR("%s%s%02X"), fingerprint, (i) ? " " : "", Settings.mqtt_fingerprint[index -1][i]);
    }
    snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_INDEX_SVALUE, command, index, fingerprint);
  }
#endif
  else if ((CMND_MQTTCLIENT == command_code) && !grpflg) {
    if ((data_len > 0) && (data_len < sizeof(Settings.mqtt_client))) {
      strlcpy(Settings.mqtt_client, (SC_DEFAULT == Shortcut(dataBuf)) ? MQTT_CLIENT_ID : dataBuf, sizeof(Settings.mqtt_client));
      restart_flag = 2;
    }
    snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_SVALUE, command, Settings.mqtt_client);
  }
  else if (CMND_MQTTUSER == command_code) {
    if ((data_len > 0) && (data_len < sizeof(Settings.mqtt_user))) {
      strlcpy(Settings.mqtt_user, (SC_CLEAR == Shortcut(dataBuf)) ? "" : (SC_DEFAULT == Shortcut(dataBuf)) ? MQTT_USER : dataBuf, sizeof(Settings.mqtt_user));
      restart_flag = 2;
    }
    snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_SVALUE, command, Settings.mqtt_user);
  }
  else if (CMND_MQTTPASSWORD == command_code) {
    if ((data_len > 0) && (data_len < sizeof(Settings.mqtt_pwd))) {
      strlcpy(Settings.mqtt_pwd, (SC_CLEAR == Shortcut(dataBuf)) ? "" : (SC_DEFAULT == Shortcut(dataBuf)) ? MQTT_PASS : dataBuf, sizeof(Settings.mqtt_pwd));
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_SVALUE, command, Settings.mqtt_pwd);
      restart_flag = 2;
    } else {
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_ASTERIX, command);
    }
  }
  else if (CMND_FULLTOPIC == command_code) {
    if ((data_len > 0) && (data_len < sizeof(Settings.mqtt_fulltopic))) {
      MakeValidMqtt(1, dataBuf);
      if (!strcmp(dataBuf, mqtt_client)) SetShortcut(dataBuf, SC_DEFAULT);
      strlcpy(stemp1, (SC_DEFAULT == Shortcut(dataBuf)) ? MQTT_FULLTOPIC : dataBuf, sizeof(stemp1));
      if (strcmp(stemp1, Settings.mqtt_fulltopic)) {
        snprintf_P(mqtt_data, sizeof(mqtt_data), (Settings.flag.mqtt_offline) ? S_OFFLINE : "");
        MqttPublishPrefixTopic_P(TELE, PSTR(D_LWT), true);
        strlcpy(Settings.mqtt_fulltopic, stemp1, sizeof(Settings.mqtt_fulltopic));
        restart_flag = 2;
      }
    }
    snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_SVALUE, command, Settings.mqtt_fulltopic);
  }
  else if ((CMND_PREFIX == command_code) && (index > 0) && (index <= 3)) {
    if ((data_len > 0) && (data_len < sizeof(Settings.mqtt_prefix[0]))) {
      MakeValidMqtt(0, dataBuf);
      strlcpy(Settings.mqtt_prefix[index -1], (SC_DEFAULT == Shortcut(dataBuf)) ? (1==index)?SUB_PREFIX:(2==index)?PUB_PREFIX:PUB_PREFIX2 : dataBuf, sizeof(Settings.mqtt_prefix[0]));

      restart_flag = 2;
    }
    snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_INDEX_SVALUE, command, index, Settings.mqtt_prefix[index -1]);
  }
  else if (CMND_PUBLISH == command_code) {
    if (data_len > 0) {
      char *mqtt_part = strtok(dataBuf, " ");
      if (mqtt_part) {
        snprintf(stemp1, sizeof(stemp1), mqtt_part);
        mqtt_part = strtok(NULL, " ");
        if (mqtt_part) {
          snprintf(mqtt_data, sizeof(mqtt_data), mqtt_part);
        } else {
          mqtt_data[0] = '\0';
        }
        MqttPublishDirect(stemp1, (index == 2));

        mqtt_data[0] = '\0';
      }
    }
  }
  else if (CMND_GROUPTOPIC == command_code) {
    if ((data_len > 0) && (data_len < sizeof(Settings.mqtt_grptopic))) {
      MakeValidMqtt(0, dataBuf);
      if (!strcmp(dataBuf, mqtt_client)) SetShortcut(dataBuf, SC_DEFAULT);
      strlcpy(Settings.mqtt_grptopic, (SC_DEFAULT == Shortcut(dataBuf)) ? MQTT_GRPTOPIC : dataBuf, sizeof(Settings.mqtt_grptopic));
      restart_flag = 2;
    }
    snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_SVALUE, command, Settings.mqtt_grptopic);
  }
  else if ((CMND_TOPIC == command_code) && !grpflg) {
    if ((data_len > 0) && (data_len < sizeof(Settings.mqtt_topic))) {
      MakeValidMqtt(0, dataBuf);
      if (!strcmp(dataBuf, mqtt_client)) SetShortcut(dataBuf, SC_DEFAULT);
      strlcpy(stemp1, (SC_DEFAULT == Shortcut(dataBuf)) ? MQTT_TOPIC : dataBuf, sizeof(stemp1));
      if (strcmp(stemp1, Settings.mqtt_topic)) {
        snprintf_P(mqtt_data, sizeof(mqtt_data), (Settings.flag.mqtt_offline) ? S_OFFLINE : "");
        MqttPublishPrefixTopic_P(TELE, PSTR(D_LWT), true);
        strlcpy(Settings.mqtt_topic, stemp1, sizeof(Settings.mqtt_topic));
        restart_flag = 2;
      }
    }
    snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_SVALUE, command, Settings.mqtt_topic);
  }
  else if ((CMND_BUTTONTOPIC == command_code) && !grpflg) {
    if ((data_len > 0) && (data_len < sizeof(Settings.button_topic))) {
      MakeValidMqtt(0, dataBuf);
      if (!strcmp(dataBuf, mqtt_client)) SetShortcut(dataBuf, SC_DEFAULT);
      switch (Shortcut(dataBuf)) {
        case SC_CLEAR: strlcpy(Settings.button_topic, "", sizeof(Settings.button_topic)); break;
        case SC_DEFAULT: strlcpy(Settings.button_topic, mqtt_topic, sizeof(Settings.button_topic)); break;
        case SC_USER: strlcpy(Settings.button_topic, MQTT_BUTTON_TOPIC, sizeof(Settings.button_topic)); break;
        default: strlcpy(Settings.button_topic, dataBuf, sizeof(Settings.button_topic));
      }
    }
    snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_SVALUE, command, Settings.button_topic);
  }
  else if (CMND_SWITCHTOPIC == command_code) {
    if ((data_len > 0) && (data_len < sizeof(Settings.switch_topic))) {
      MakeValidMqtt(0, dataBuf);
      if (!strcmp(dataBuf, mqtt_client)) SetShortcut(dataBuf, SC_DEFAULT);
      switch (Shortcut(dataBuf)) {
        case SC_CLEAR: strlcpy(Settings.switch_topic, "", sizeof(Settings.switch_topic)); break;
        case SC_DEFAULT: strlcpy(Settings.switch_topic, mqtt_topic, sizeof(Settings.switch_topic)); break;
        case SC_USER: strlcpy(Settings.switch_topic, MQTT_SWITCH_TOPIC, sizeof(Settings.switch_topic)); break;
        default: strlcpy(Settings.switch_topic, dataBuf, sizeof(Settings.switch_topic));
      }
    }
    snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_SVALUE, command, Settings.switch_topic);
  }
  else if (CMND_BUTTONRETAIN == command_code) {
    if ((payload >= 0) && (payload <= 1)) {
      if (!payload) {
        for(i = 1; i <= MAX_KEYS; i++) {
          SendKey(0, i, 9);
        }
      }
      Settings.flag.mqtt_button_retain = payload;
    }
    snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_SVALUE, command, GetStateText(Settings.flag.mqtt_button_retain));
  }
  else if (CMND_SWITCHRETAIN == command_code) {
    if ((payload >= 0) && (payload <= 1)) {
      if (!payload) {
        for(i = 1; i <= MAX_SWITCHES; i++) {
          SendKey(1, i, 9);
        }
      }
      Settings.flag.mqtt_switch_retain = payload;
    }
    snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_SVALUE, command, GetStateText(Settings.flag.mqtt_switch_retain));
  }
  else if (CMND_POWERRETAIN == command_code) {
    if ((payload >= 0) && (payload <= 1)) {
      if (!payload) {
        for(i = 1; i <= devices_present; i++) {
          GetTopic_P(stemp1, STAT, mqtt_topic, GetPowerDevice(scommand, i, sizeof(scommand), Settings.flag.device_index_enable));
          mqtt_data[0] = '\0';
          MqttPublish(stemp1, Settings.flag.mqtt_power_retain);
        }
      }
      Settings.flag.mqtt_power_retain = payload;
    }
    snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_SVALUE, command, GetStateText(Settings.flag.mqtt_power_retain));
  }
  else if (CMND_SENSORRETAIN == command_code) {
    if ((payload >= 0) && (payload <= 1)) {
      if (!payload) {
        mqtt_data[0] = '\0';
        MqttPublishPrefixTopic_P(TELE, PSTR(D_RSLT_SENSOR), Settings.flag.mqtt_sensor_retain);
        MqttPublishPrefixTopic_P(TELE, PSTR(D_RSLT_ENERGY), Settings.flag.mqtt_sensor_retain);
      }
      Settings.flag.mqtt_sensor_retain = payload;
    }
    snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_SVALUE, command, GetStateText(Settings.flag.mqtt_sensor_retain));
  }
  else serviced = false;

  return serviced;
}





#ifdef USE_WEBSERVER

#define WEB_HANDLE_MQTT "mq"

const char S_CONFIGURE_MQTT[] PROGMEM = D_CONFIGURE_MQTT;

const char HTTP_BTN_MENU_MQTT[] PROGMEM =
  "<br/><form action='" WEB_HANDLE_MQTT "' method='get'><button>" D_CONFIGURE_MQTT "</button></form>";

const char HTTP_FORM_MQTT[] PROGMEM =
  "<fieldset><legend><b>&nbsp;" D_MQTT_PARAMETERS "&nbsp;</b></legend><form method='get' action='" WEB_HANDLE_MQTT "'>"
  "<br/><b>" D_HOST "</b> (" MQTT_HOST ")<br/><input id='mh' name='mh' placeholder='" MQTT_HOST" ' value='{m1'><br/>"
  "<br/><b>" D_PORT "</b> (" STR(MQTT_PORT) ")<br/><input id='ml' name='ml' placeholder='" STR(MQTT_PORT) "' value='{m2'><br/>"
  "<br/><b>" D_CLIENT "</b> ({m0)<br/><input id='mc' name='mc' placeholder='" MQTT_CLIENT_ID "' value='{m3'><br/>"
  "<br/><b>" D_USER "</b> (" MQTT_USER ")<br/><input id='mu' name='mu' placeholder='" MQTT_USER "' value='{m4'><br/>"
  "<br/><b>" D_PASSWORD "</b><br/><input id='mp' name='mp' type='password' placeholder='" MQTT_PASS "' value='{m5'><br/>"
  "<br/><b>" D_TOPIC "</b> = %topic% (" MQTT_TOPIC ")<br/><input id='mt' name='mt' placeholder='" MQTT_TOPIC" ' value='{m6'><br/>"
  "<br/><b>" D_FULL_TOPIC "</b> (" MQTT_FULLTOPIC ")<br/><input id='mf' name='mf' placeholder='" MQTT_FULLTOPIC" ' value='{m7'><br/>";

void HandleMqttConfiguration()
{
  if (HttpUser()) { return; }
  if (!WebAuthenticate()) { return WebServer->requestAuthentication(); }
  AddLog_P(LOG_LEVEL_DEBUG, S_LOG_HTTP, S_CONFIGURE_MQTT);

  if (WebServer->hasArg("save")) {
    MqttSaveSettings();
    WebRestart(1);
    return;
  }

  String page = FPSTR(HTTP_HEAD);
  page.replace(F("{v}"), FPSTR(S_CONFIGURE_MQTT));
  page += FPSTR(HTTP_HEAD_STYLE);

  page += FPSTR(HTTP_FORM_MQTT);
  char str[sizeof(Settings.mqtt_client)];
  page.replace(F("{m0"), Format(str, MQTT_CLIENT_ID, sizeof(Settings.mqtt_client)));
  page.replace(F("{m1"), Settings.mqtt_host);
  page.replace(F("{m2"), String(Settings.mqtt_port));
  page.replace(F("{m3"), Settings.mqtt_client);
  page.replace(F("{m4"), (Settings.mqtt_user[0] == '\0')?"0":Settings.mqtt_user);
  page.replace(F("{m5"), (Settings.mqtt_pwd[0] == '\0')?"0":Settings.mqtt_pwd);
  page.replace(F("{m6"), Settings.mqtt_topic);
  page.replace(F("{m7"), Settings.mqtt_fulltopic);

  page += FPSTR(HTTP_FORM_END);
  page += FPSTR(HTTP_BTN_CONF);
  ShowPage(page);
}

void MqttSaveSettings()
{
  char tmp[100];
  char stemp[TOPSZ];
  char stemp2[TOPSZ];

  WebGetArg("mt", tmp, sizeof(tmp));
  strlcpy(stemp, (!strlen(tmp)) ? MQTT_TOPIC : tmp, sizeof(stemp));
  MakeValidMqtt(0, stemp);
  WebGetArg("mf", tmp, sizeof(tmp));
  strlcpy(stemp2, (!strlen(tmp)) ? MQTT_FULLTOPIC : tmp, sizeof(stemp2));
  MakeValidMqtt(1,stemp2);
  if ((strcmp(stemp, Settings.mqtt_topic)) || (strcmp(stemp2, Settings.mqtt_fulltopic))) {
    snprintf_P(mqtt_data, sizeof(mqtt_data), (Settings.flag.mqtt_offline) ? S_OFFLINE : "");
    MqttPublishPrefixTopic_P(TELE, S_LWT, true);
  }
  strlcpy(Settings.mqtt_topic, stemp, sizeof(Settings.mqtt_topic));
  strlcpy(Settings.mqtt_fulltopic, stemp2, sizeof(Settings.mqtt_fulltopic));
  WebGetArg("mh", tmp, sizeof(tmp));
  strlcpy(Settings.mqtt_host, (!strlen(tmp)) ? MQTT_HOST : (!strcmp(tmp,"0")) ? "" : tmp, sizeof(Settings.mqtt_host));
  WebGetArg("ml", tmp, sizeof(tmp));
  Settings.mqtt_port = (!strlen(tmp)) ? MQTT_PORT : atoi(tmp);
  WebGetArg("mc", tmp, sizeof(tmp));
  strlcpy(Settings.mqtt_client, (!strlen(tmp)) ? MQTT_CLIENT_ID : tmp, sizeof(Settings.mqtt_client));
  WebGetArg("mu", tmp, sizeof(tmp));
  strlcpy(Settings.mqtt_user, (!strlen(tmp)) ? MQTT_USER : (!strcmp(tmp,"0")) ? "" : tmp, sizeof(Settings.mqtt_user));
  WebGetArg("mp", tmp, sizeof(tmp));
  strlcpy(Settings.mqtt_pwd, (!strlen(tmp)) ? MQTT_PASS : (!strcmp(tmp,"0")) ? "" : tmp, sizeof(Settings.mqtt_pwd));
  snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_MQTT D_CMND_MQTTHOST " %s, " D_CMND_MQTTPORT " %d, " D_CMND_MQTTCLIENT " %s, " D_CMND_MQTTUSER " %s, " D_CMND_MQTTPASSWORD " %s, " D_CMND_TOPIC " %s, " D_CMND_FULLTOPIC " %s"),
    Settings.mqtt_host, Settings.mqtt_port, Settings.mqtt_client, Settings.mqtt_user, Settings.mqtt_pwd, Settings.mqtt_topic, Settings.mqtt_fulltopic);
  AddLog(LOG_LEVEL_INFO);
}
#endif





#define XDRV_02 

boolean Xdrv02(byte function)
{
  boolean result = false;

  if (Settings.flag.mqtt_enabled) {
    switch (function) {
#ifdef USE_WEBSERVER
      case FUNC_WEB_ADD_BUTTON:
        strncat_P(mqtt_data, HTTP_BTN_MENU_MQTT, sizeof(mqtt_data));
        break;
      case FUNC_WEB_ADD_HANDLER:
        WebServer->on("/" WEB_HANDLE_MQTT, HandleMqttConfiguration);
        break;
#endif
      case FUNC_LOOP:
        MqttLoop();
        break;
      case FUNC_COMMAND:
        result = MqttCommand();
        break;
    }
  }
  return result;
}
# 1 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xdrv_03_energy.ino"
# 20 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xdrv_03_energy.ino"
#ifdef USE_ENERGY_SENSOR




#define ENERGY_NONE 0

#define FEATURE_POWER_LIMIT true

enum EnergyCommands {
  CMND_POWERDELTA,
  CMND_POWERLOW, CMND_POWERHIGH, CMND_VOLTAGELOW, CMND_VOLTAGEHIGH, CMND_CURRENTLOW, CMND_CURRENTHIGH,
  CMND_POWERSET, CMND_VOLTAGESET, CMND_CURRENTSET, CMND_FREQUENCYSET,
  CMND_ENERGYRESET, CMND_MAXENERGY, CMND_MAXENERGYSTART,
  CMND_MAXPOWER, CMND_MAXPOWERHOLD, CMND_MAXPOWERWINDOW,
  CMND_SAFEPOWER, CMND_SAFEPOWERHOLD, CMND_SAFEPOWERWINDOW };
const char kEnergyCommands[] PROGMEM =
  D_CMND_POWERDELTA "|"
  D_CMND_POWERLOW "|" D_CMND_POWERHIGH "|" D_CMND_VOLTAGELOW "|" D_CMND_VOLTAGEHIGH "|" D_CMND_CURRENTLOW "|" D_CMND_CURRENTHIGH "|"
  D_CMND_POWERSET "|" D_CMND_VOLTAGESET "|" D_CMND_CURRENTSET "|" D_CMND_FREQUENCYSET "|"
  D_CMND_ENERGYRESET "|" D_CMND_MAXENERGY "|" D_CMND_MAXENERGYSTART "|"
  D_CMND_MAXPOWER "|" D_CMND_MAXPOWERHOLD "|" D_CMND_MAXPOWERWINDOW "|"
  D_CMND_SAFEPOWER "|" D_CMND_SAFEPOWERHOLD "|" D_CMND_SAFEPOWERWINDOW ;

float energy_voltage = 0;
float energy_current = 0;
float energy_active_power = 0;
float energy_apparent_power = NAN;
float energy_reactive_power = NAN;
float energy_power_factor = NAN;
float energy_frequency = NAN;
float energy_start = 0;

float energy_daily = 0;
float energy_total = 0;
unsigned long energy_kWhtoday_delta = 0;
unsigned long energy_kWhtoday;
unsigned long energy_period = 0;

float energy_power_last[3] = { 0 };
uint8_t energy_power_delta = 0;

bool energy_type_dc = false;
bool energy_power_on = true;

byte energy_min_power_flag = 0;
byte energy_max_power_flag = 0;
byte energy_min_voltage_flag = 0;
byte energy_max_voltage_flag = 0;
byte energy_min_current_flag = 0;
byte energy_max_current_flag = 0;

byte energy_power_steady_cntr = 8;
byte energy_max_energy_state = 0;

#if FEATURE_POWER_LIMIT
byte energy_mplr_counter = 0;
uint16_t energy_mplh_counter = 0;
uint16_t energy_mplw_counter = 0;
#endif

byte energy_fifth_second = 0;
Ticker ticker_energy;

int energy_command_code = 0;


void EnergyUpdateToday()
{
  if (energy_kWhtoday_delta > 1000) {
    unsigned long delta = energy_kWhtoday_delta / 1000;
    energy_kWhtoday_delta -= (delta * 1000);
    energy_kWhtoday += delta;
  }
  RtcSettings.energy_kWhtoday = energy_kWhtoday;
  energy_daily = (float)energy_kWhtoday / 100000;
  energy_total = (float)(RtcSettings.energy_kWhtotal + energy_kWhtoday) / 100000;
}



void Energy200ms()
{
  energy_power_on = (power != 0) | Settings.flag.no_power_on_check;

  energy_fifth_second++;
  if (5 == energy_fifth_second) {
    energy_fifth_second = 0;

    XnrgCall(FUNC_EVERY_SECOND);

    if (RtcTime.valid) {
      if (LocalTime() == Midnight()) {
        Settings.energy_kWhyesterday = energy_kWhtoday;
        Settings.energy_kWhtotal += energy_kWhtoday;
        RtcSettings.energy_kWhtotal = Settings.energy_kWhtotal;
        energy_kWhtoday = 0;
        energy_kWhtoday_delta = 0;
        energy_period = energy_kWhtoday;
        EnergyUpdateToday();
        energy_max_energy_state = 3;
      }
      if ((RtcTime.hour == Settings.energy_max_energy_start) && (3 == energy_max_energy_state)) {
        energy_max_energy_state = 0;
      }
    }
  }

  XnrgCall(FUNC_EVERY_200_MSECOND);
}

void EnergySaveState()
{
  Settings.energy_kWhdoy = (RtcTime.valid) ? RtcTime.day_of_year : 0;
  Settings.energy_kWhtoday = energy_kWhtoday;
  RtcSettings.energy_kWhtoday = energy_kWhtoday;
  Settings.energy_kWhtotal = RtcSettings.energy_kWhtotal;
}

boolean EnergyMargin(byte type, uint16_t margin, uint16_t value, byte &flag, byte &save_flag)
{
  byte change;

  if (!margin) return false;
  change = save_flag;
  if (type) {
    flag = (value > margin);
  } else {
    flag = (value < margin);
  }
  save_flag = flag;
  return (change != save_flag);
}

void EnergySetPowerSteadyCounter()
{
  energy_power_steady_cntr = 2;
}

void EnergyMarginCheck()
{
  uint16_t energy_daily_u = 0;
  uint16_t energy_power_u = 0;
  uint16_t energy_voltage_u = 0;
  uint16_t energy_current_u = 0;
  boolean flag;
  boolean jsonflg;

  if (energy_power_steady_cntr) {
    energy_power_steady_cntr--;
    return;
  }

  if (Settings.energy_power_delta) {
    float delta = abs(energy_power_last[0] - energy_active_power);

    float min_power = (energy_power_last[0] > energy_active_power) ? energy_active_power : energy_power_last[0];
    if (((delta / min_power) * 100) > Settings.energy_power_delta) {
      energy_power_delta = 1;
      energy_power_last[1] = energy_active_power;
      energy_power_last[2] = energy_active_power;
    }
  }
  energy_power_last[0] = energy_power_last[1];
  energy_power_last[1] = energy_power_last[2];
  energy_power_last[2] = energy_active_power;

  if (energy_power_on && (Settings.energy_min_power || Settings.energy_max_power || Settings.energy_min_voltage || Settings.energy_max_voltage || Settings.energy_min_current || Settings.energy_max_current)) {
    energy_power_u = (uint16_t)(energy_active_power);
    energy_voltage_u = (uint16_t)(energy_voltage);
    energy_current_u = (uint16_t)(energy_current * 1000);




    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{"));
    jsonflg = 0;
    if (EnergyMargin(0, Settings.energy_min_power, energy_power_u, flag, energy_min_power_flag)) {
      snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s%s\"" D_CMND_POWERLOW "\":\"%s\""), mqtt_data, (jsonflg)?",":"", GetStateText(flag));
      jsonflg = 1;
    }
    if (EnergyMargin(1, Settings.energy_max_power, energy_power_u, flag, energy_max_power_flag)) {
      snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s%s\"" D_CMND_POWERHIGH "\":\"%s\""), mqtt_data, (jsonflg)?",":"", GetStateText(flag));
      jsonflg = 1;
    }
    if (EnergyMargin(0, Settings.energy_min_voltage, energy_voltage_u, flag, energy_min_voltage_flag)) {
      snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s%s\"" D_CMND_VOLTAGELOW "\":\"%s\""), mqtt_data, (jsonflg)?",":"", GetStateText(flag));
      jsonflg = 1;
    }
    if (EnergyMargin(1, Settings.energy_max_voltage, energy_voltage_u, flag, energy_max_voltage_flag)) {
      snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s%s\"" D_CMND_VOLTAGEHIGH "\":\"%s\""), mqtt_data, (jsonflg)?",":"", GetStateText(flag));
      jsonflg = 1;
    }
    if (EnergyMargin(0, Settings.energy_min_current, energy_current_u, flag, energy_min_current_flag)) {
      snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s%s\"" D_CMND_CURRENTLOW "\":\"%s\""), mqtt_data, (jsonflg)?",":"", GetStateText(flag));
      jsonflg = 1;
    }
    if (EnergyMargin(1, Settings.energy_max_current, energy_current_u, flag, energy_max_current_flag)) {
      snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s%s\"" D_CMND_CURRENTHIGH "\":\"%s\""), mqtt_data, (jsonflg)?",":"", GetStateText(flag));
      jsonflg = 1;
    }
    if (jsonflg) {
      snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s}"), mqtt_data);
      MqttPublishPrefixTopic_P(TELE, PSTR(D_RSLT_MARGINS), MQTT_TELE_RETAIN);
      EnergyMqttShow();
    }
  }

#if FEATURE_POWER_LIMIT

  if (Settings.energy_max_power_limit) {
    if (energy_active_power > Settings.energy_max_power_limit) {
      if (!energy_mplh_counter) {
        energy_mplh_counter = Settings.energy_max_power_limit_hold;
      } else {
        energy_mplh_counter--;
        if (!energy_mplh_counter) {
          snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"" D_JSON_MAXPOWERREACHED "\":\"%d%s\"}"), energy_power_u, (Settings.flag.value_units) ? " " D_UNIT_WATT : "");
          MqttPublishPrefixTopic_P(STAT, S_RSLT_WARNING);
          EnergyMqttShow();
          ExecuteCommandPower(1, POWER_OFF, SRC_MAXPOWER);
          if (!energy_mplr_counter) {
            energy_mplr_counter = Settings.param[P_MAX_POWER_RETRY] +1;
          }
          energy_mplw_counter = Settings.energy_max_power_limit_window;
        }
      }
    }
    else if (power && (energy_power_u <= Settings.energy_max_power_limit)) {
      energy_mplh_counter = 0;
      energy_mplr_counter = 0;
      energy_mplw_counter = 0;
    }
    if (!power) {
      if (energy_mplw_counter) {
        energy_mplw_counter--;
      } else {
        if (energy_mplr_counter) {
          energy_mplr_counter--;
          if (energy_mplr_counter) {
            snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"" D_JSON_POWERMONITOR "\":\"%s\"}"), GetStateText(1));
            MqttPublishPrefixTopic_P(RESULT_OR_STAT, PSTR(D_JSON_POWERMONITOR));
            ExecuteCommandPower(1, POWER_ON, SRC_MAXPOWER);
          } else {
            snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"" D_JSON_MAXPOWERREACHEDRETRY "\":\"%s\"}"), GetStateText(0));
            MqttPublishPrefixTopic_P(STAT, S_RSLT_WARNING);
            EnergyMqttShow();
          }
        }
      }
    }
  }


  if (Settings.energy_max_energy) {
    energy_daily_u = (uint16_t)(energy_daily * 1000);
    if (!energy_max_energy_state && (RtcTime.hour == Settings.energy_max_energy_start)) {
      energy_max_energy_state = 1;
      snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"" D_JSON_ENERGYMONITOR "\":\"%s\"}"), GetStateText(1));
      MqttPublishPrefixTopic_P(RESULT_OR_STAT, PSTR(D_JSON_ENERGYMONITOR));
      ExecuteCommandPower(1, POWER_ON, SRC_MAXENERGY);
    }
    else if ((1 == energy_max_energy_state) && (energy_daily_u >= Settings.energy_max_energy)) {
      energy_max_energy_state = 2;
      dtostrfd(energy_daily, 3, mqtt_data);
      snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"" D_JSON_MAXENERGYREACHED "\":\"%s%s\"}"), mqtt_data, (Settings.flag.value_units) ? " " D_UNIT_KILOWATTHOUR : "");
      MqttPublishPrefixTopic_P(STAT, S_RSLT_WARNING);
      EnergyMqttShow();
      ExecuteCommandPower(1, POWER_OFF, SRC_MAXENERGY);
    }
  }
#endif

  if (energy_power_delta) EnergyMqttShow();
}

void EnergyMqttShow()
{

  snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"" D_JSON_TIME "\":\"%s\""), GetDateAndTime(DT_LOCAL).c_str());
  int tele_period_save = tele_period;
  tele_period = 2;
  EnergyShow(1);
  tele_period = tele_period_save;
  snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s}"), mqtt_data);
  MqttPublishPrefixTopic_P(TELE, PSTR(D_RSLT_SENSOR), Settings.flag.mqtt_sensor_retain);
  energy_power_delta = 0;
}





boolean EnergyCommand()
{
  char command [CMDSZ];
  char sunit[CMDSZ];
  boolean serviced = true;
  uint8_t status_flag = 0;
  uint8_t unit = 0;
  unsigned long nvalue = 0;

  int command_code = GetCommandCode(command, sizeof(command), XdrvMailbox.topic, kEnergyCommands);
  energy_command_code = command_code;
  if (-1 == command_code) {
    serviced = false;
  }
  else if (CMND_POWERDELTA == command_code) {
    if ((XdrvMailbox.payload >= 0) && (XdrvMailbox.payload < 101)) {
      Settings.energy_power_delta = (1 == XdrvMailbox.payload) ? DEFAULT_POWER_DELTA : XdrvMailbox.payload;
    }
    nvalue = Settings.energy_power_delta;
    unit = UNIT_PERCENTAGE;
  }
  else if (CMND_POWERLOW == command_code) {
    if ((XdrvMailbox.payload >= 0) && (XdrvMailbox.payload < 3601)) {
      Settings.energy_min_power = XdrvMailbox.payload;
    }
    nvalue = Settings.energy_min_power;
    unit = UNIT_WATT;
  }
  else if (CMND_POWERHIGH == command_code) {
    if ((XdrvMailbox.payload >= 0) && (XdrvMailbox.payload < 3601)) {
      Settings.energy_max_power = XdrvMailbox.payload;
    }
    nvalue = Settings.energy_max_power;
    unit = UNIT_WATT;
  }
  else if (CMND_VOLTAGELOW == command_code) {
    if ((XdrvMailbox.payload >= 0) && (XdrvMailbox.payload < 501)) {
      Settings.energy_min_voltage = XdrvMailbox.payload;
    }
    nvalue = Settings.energy_min_voltage;
    unit = UNIT_VOLT;
  }
  else if (CMND_VOLTAGEHIGH == command_code) {
    if ((XdrvMailbox.payload >= 0) && (XdrvMailbox.payload < 501)) {
      Settings.energy_max_voltage = XdrvMailbox.payload;
    }
    nvalue = Settings.energy_max_voltage;
    unit = UNIT_VOLT;
  }
  else if (CMND_CURRENTLOW == command_code) {
    if ((XdrvMailbox.payload >= 0) && (XdrvMailbox.payload < 16001)) {
      Settings.energy_min_current = XdrvMailbox.payload;
    }
    nvalue = Settings.energy_min_current;
    unit = UNIT_MILLIAMPERE;
  }
  else if (CMND_CURRENTHIGH == command_code) {
    if ((XdrvMailbox.payload >= 0) && (XdrvMailbox.payload < 16001)) {
      Settings.energy_max_current = XdrvMailbox.payload;
    }
    nvalue = Settings.energy_max_current;
    unit = UNIT_MILLIAMPERE;
  }
  else if ((CMND_ENERGYRESET == command_code) && (XdrvMailbox.index > 0) && (XdrvMailbox.index <= 3)) {
    char *p;
    unsigned long lnum = strtoul(XdrvMailbox.data, &p, 10);
    if (p != XdrvMailbox.data) {
      switch (XdrvMailbox.index) {
      case 1:
        energy_kWhtoday = lnum *100;
        energy_kWhtoday_delta = 0;
        energy_period = energy_kWhtoday;
        Settings.energy_kWhtoday = energy_kWhtoday;
        RtcSettings.energy_kWhtoday = energy_kWhtoday;
        energy_daily = (float)energy_kWhtoday / 100000;
        break;
      case 2:
        Settings.energy_kWhyesterday = lnum *100;
        break;
      case 3:
        RtcSettings.energy_kWhtotal = lnum *100;
        Settings.energy_kWhtotal = RtcSettings.energy_kWhtotal;
        energy_total = (float)(RtcSettings.energy_kWhtotal + energy_kWhtoday) / 100000;
        if (!energy_total) { Settings.energy_kWhtotal_time = LocalTime(); }
        break;
      }
    }
    char energy_yesterday_chr[10];
    char energy_daily_chr[10];
    char energy_total_chr[10];

    dtostrfd(energy_total, Settings.flag2.energy_resolution, energy_total_chr);
    dtostrfd(energy_daily, Settings.flag2.energy_resolution, energy_daily_chr);
    dtostrfd((float)Settings.energy_kWhyesterday / 100000, Settings.flag2.energy_resolution, energy_yesterday_chr);

    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"%s\":{\"" D_JSON_TOTAL "\":%s,\"" D_JSON_YESTERDAY "\":%s,\"" D_JSON_TODAY "\":%s}}"),
      command, energy_total_chr, energy_yesterday_chr, energy_daily_chr);
    status_flag = 1;
  }
  else if ((CMND_POWERSET == command_code) && XnrgCall(FUNC_COMMAND)) {
    nvalue = Settings.energy_power_calibration;
    unit = UNIT_MICROSECOND;
  }
  else if ((CMND_VOLTAGESET == command_code) && XnrgCall(FUNC_COMMAND)) {
    nvalue = Settings.energy_voltage_calibration;
    unit = UNIT_MICROSECOND;
  }
  else if ((CMND_CURRENTSET == command_code) && XnrgCall(FUNC_COMMAND)) {
    nvalue = Settings.energy_current_calibration;
    unit = UNIT_MICROSECOND;
  }
  else if ((CMND_FREQUENCYSET == command_code) && XnrgCall(FUNC_COMMAND)) {
    nvalue = Settings.energy_frequency_calibration;
    unit = UNIT_MICROSECOND;
  }

#if FEATURE_POWER_LIMIT
  else if (CMND_MAXPOWER == command_code) {
    if ((XdrvMailbox.payload >= 0) && (XdrvMailbox.payload < 3601)) {
      Settings.energy_max_power_limit = XdrvMailbox.payload;
    }
    nvalue = Settings.energy_max_power_limit;
    unit = UNIT_WATT;
  }
  else if (CMND_MAXPOWERHOLD == command_code) {
    if ((XdrvMailbox.payload >= 0) && (XdrvMailbox.payload < 3601)) {
      Settings.energy_max_power_limit_hold = (1 == XdrvMailbox.payload) ? MAX_POWER_HOLD : XdrvMailbox.payload;
    }
    nvalue = Settings.energy_max_power_limit_hold;
    unit = UNIT_SECOND;
  }
  else if (CMND_MAXPOWERWINDOW == command_code) {
    if ((XdrvMailbox.payload >= 0) && (XdrvMailbox.payload < 3601)) {
      Settings.energy_max_power_limit_window = (1 == XdrvMailbox.payload) ? MAX_POWER_WINDOW : XdrvMailbox.payload;
    }
    nvalue = Settings.energy_max_power_limit_window;
    unit = UNIT_SECOND;
  }
  else if (CMND_SAFEPOWER == command_code) {
    if ((XdrvMailbox.payload >= 0) && (XdrvMailbox.payload < 3601)) {
      Settings.energy_max_power_safe_limit = XdrvMailbox.payload;
    }
    nvalue = Settings.energy_max_power_safe_limit;
    unit = UNIT_WATT;
  }
  else if (CMND_SAFEPOWERHOLD == command_code) {
    if ((XdrvMailbox.payload >= 0) && (XdrvMailbox.payload < 3601)) {
      Settings.energy_max_power_safe_limit_hold = (1 == XdrvMailbox.payload) ? SAFE_POWER_HOLD : XdrvMailbox.payload;
    }
    nvalue = Settings.energy_max_power_safe_limit_hold;
    unit = UNIT_SECOND;
  }
  else if (CMND_SAFEPOWERWINDOW == command_code) {
    if ((XdrvMailbox.payload >= 0) && (XdrvMailbox.payload < 1440)) {
      Settings.energy_max_power_safe_limit_window = (1 == XdrvMailbox.payload) ? SAFE_POWER_WINDOW : XdrvMailbox.payload;
    }
    nvalue = Settings.energy_max_power_safe_limit_window;
    unit = UNIT_MINUTE;
  }
  else if (CMND_MAXENERGY == command_code) {
    if ((XdrvMailbox.payload >= 0) && (XdrvMailbox.payload < 3601)) {
      Settings.energy_max_energy = XdrvMailbox.payload;
      energy_max_energy_state = 3;
    }
    nvalue = Settings.energy_max_energy;
    unit = UNIT_WATTHOUR;
  }
  else if (CMND_MAXENERGYSTART == command_code) {
    if ((XdrvMailbox.payload >= 0) && (XdrvMailbox.payload < 24)) {
      Settings.energy_max_energy_start = XdrvMailbox.payload;
    }
    nvalue = Settings.energy_max_energy_start;
    unit = UNIT_HOUR;
  }
#endif
  else serviced = false;

  if (serviced && !status_flag) {

    if (UNIT_MICROSECOND == unit) {
      snprintf_P(command, sizeof(command), PSTR("%sCal"), command);
    }

    if (Settings.flag.value_units) {
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_LVALUE_SPACE_UNIT, command, nvalue, GetTextIndexed(sunit, sizeof(sunit), unit, kUnitNames));
    } else {
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_LVALUE, command, nvalue);
    }
  }

  return serviced;
}

void EnergyDrvInit()
{
  energy_flg = ENERGY_NONE;
  XnrgCall(FUNC_PRE_INIT);
}

void EnergySnsInit()
{
  XnrgCall(FUNC_INIT);

  if (energy_flg) {
    energy_kWhtoday = (RtcSettingsValid()) ? RtcSettings.energy_kWhtoday : (RtcTime.day_of_year == Settings.energy_kWhdoy) ? Settings.energy_kWhtoday : 0;
    energy_kWhtoday_delta = 0;
    energy_period = energy_kWhtoday;
    EnergyUpdateToday();
    ticker_energy.attach_ms(200, Energy200ms);
  }
}

#ifdef USE_WEBSERVER
const char HTTP_ENERGY_SNS1[] PROGMEM = "%s"
  "{s}" D_VOLTAGE "{m}%s " D_UNIT_VOLT "{e}"
  "{s}" D_CURRENT "{m}%s " D_UNIT_AMPERE "{e}"
  "{s}" D_POWERUSAGE "{m}%s " D_UNIT_WATT "{e}";

const char HTTP_ENERGY_SNS2[] PROGMEM = "%s"
  "{s}" D_POWERUSAGE_APPARENT "{m}%s " D_UNIT_VA "{e}"
  "{s}" D_POWERUSAGE_REACTIVE "{m}%s " D_UNIT_VAR "{e}"
  "{s}" D_POWER_FACTOR "{m}%s{e}";

const char HTTP_ENERGY_SNS3[] PROGMEM = "%s"
  "{s}" D_FREQUENCY "{m}%s " D_UNIT_HERTZ "{e}";

const char HTTP_ENERGY_SNS4[] PROGMEM = "%s"
  "{s}" D_ENERGY_TODAY "{m}%s " D_UNIT_KILOWATTHOUR "{e}"
  "{s}" D_ENERGY_YESTERDAY "{m}%s " D_UNIT_KILOWATTHOUR "{e}"
  "{s}" D_ENERGY_TOTAL "{m}%s " D_UNIT_KILOWATTHOUR "{e}";
#endif

void EnergyShow(boolean json)
{
  char voltage_chr[10];
  char current_chr[10];
  char active_power_chr[10];
  char apparent_power_chr[10];
  char reactive_power_chr[10];
  char power_factor_chr[10];
  char frequency_chr[10];
  char energy_daily_chr[10];
  char energy_period_chr[10];
  char energy_yesterday_chr[10];
  char energy_total_chr[10];

  char speriod[20];
  char sfrequency[20];

  bool show_energy_period = (0 == tele_period);

  float power_factor = energy_power_factor;

  if (!energy_type_dc) {
    float apparent_power = energy_apparent_power;
    if (isnan(apparent_power)) {
      apparent_power = energy_voltage * energy_current;
    }
    if (apparent_power < energy_active_power) {
      energy_active_power = apparent_power;
    }

    if (isnan(power_factor)) {
      power_factor = (energy_active_power && apparent_power) ? energy_active_power / apparent_power : 0;
      if (power_factor > 1) power_factor = 1;
    }

    float reactive_power = energy_reactive_power;
    if (isnan(reactive_power)) {
      reactive_power = 0;
      uint32_t difference = ((uint32_t)(apparent_power * 100) - (uint32_t)(energy_active_power * 100)) / 10;
      if ((energy_current > 0.005) && ((difference > 15) || (difference > (uint32_t)(apparent_power * 100 / 1000)))) {


        reactive_power = (float)(RoundSqrtInt((uint32_t)(apparent_power * apparent_power * 100) - (uint32_t)(energy_active_power * energy_active_power * 100))) / 10;
      }
    }

    dtostrfd(apparent_power, Settings.flag2.wattage_resolution, apparent_power_chr);
    dtostrfd(reactive_power, Settings.flag2.wattage_resolution, reactive_power_chr);
    dtostrfd(power_factor, 2, power_factor_chr);
    if (!isnan(energy_frequency)) {
      dtostrfd(energy_frequency, Settings.flag2.frequency_resolution, frequency_chr);
      snprintf_P(sfrequency, sizeof(sfrequency), PSTR(",\"" D_JSON_FREQUENCY "\":%s"), frequency_chr);
    }
  }

  dtostrfd(energy_voltage, Settings.flag2.voltage_resolution, voltage_chr);
  dtostrfd(energy_current, Settings.flag2.current_resolution, current_chr);
  dtostrfd(energy_active_power, Settings.flag2.wattage_resolution, active_power_chr);
  dtostrfd(energy_daily, Settings.flag2.energy_resolution, energy_daily_chr);
  dtostrfd((float)Settings.energy_kWhyesterday / 100000, Settings.flag2.energy_resolution, energy_yesterday_chr);
  dtostrfd(energy_total, Settings.flag2.energy_resolution, energy_total_chr);

  float energy = 0;
  if (show_energy_period) {
    if (energy_period) energy = (float)(energy_kWhtoday - energy_period) / 100;
    energy_period = energy_kWhtoday;
    dtostrfd(energy, Settings.flag2.wattage_resolution, energy_period_chr);
    snprintf_P(speriod, sizeof(speriod), PSTR(",\"" D_JSON_PERIOD "\":%s"), energy_period_chr);
  }

  if (json) {
    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s,\"" D_RSLT_ENERGY "\":{\"" D_JSON_TOTAL_START_TIME "\":\"%s\",\"" D_JSON_TOTAL "\":%s,\"" D_JSON_YESTERDAY "\":%s,\"" D_JSON_TODAY "\":%s%s,\"" D_JSON_POWERUSAGE "\":%s"),
      mqtt_data, GetDateAndTime(DT_ENERGY).c_str(), energy_total_chr, energy_yesterday_chr, energy_daily_chr, (show_energy_period) ? speriod : "", active_power_chr);
    if (!energy_type_dc) {
      snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s,\"" D_JSON_APPARENT_POWERUSAGE "\":%s,\"" D_JSON_REACTIVE_POWERUSAGE "\":%s,\"" D_JSON_POWERFACTOR "\":%s%s"),
        mqtt_data, apparent_power_chr, reactive_power_chr, power_factor_chr, (!isnan(energy_frequency)) ? sfrequency : "");
    }
    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s,\"" D_JSON_VOLTAGE "\":%s,\"" D_JSON_CURRENT "\":%s}"), mqtt_data, voltage_chr, current_chr);

#ifdef USE_DOMOTICZ
    if (show_energy_period) {
      dtostrfd(energy_total * 1000, 1, energy_total_chr);
      DomoticzSensorPowerEnergy((int)energy_active_power, energy_total_chr);
      DomoticzSensor(DZ_VOLTAGE, voltage_chr);
      DomoticzSensor(DZ_CURRENT, current_chr);
    }
#endif
#ifdef USE_KNX
    if (show_energy_period) {
      KnxSensor(KNX_ENERGY_VOLTAGE, energy_voltage);
      KnxSensor(KNX_ENERGY_CURRENT, energy_current);
      KnxSensor(KNX_ENERGY_POWER, energy_active_power);
      if (!energy_type_dc) { KnxSensor(KNX_ENERGY_POWERFACTOR, power_factor); }
      KnxSensor(KNX_ENERGY_DAILY, energy_daily);
      KnxSensor(KNX_ENERGY_TOTAL, energy_total);
      KnxSensor(KNX_ENERGY_START, energy_start);
    }
#endif
#ifdef USE_WEBSERVER
  } else {
    snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_ENERGY_SNS1, mqtt_data, voltage_chr, current_chr, active_power_chr);
    if (!energy_type_dc) {
      snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_ENERGY_SNS2, mqtt_data, apparent_power_chr, reactive_power_chr, power_factor_chr);
      if (!isnan(energy_frequency)) { snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_ENERGY_SNS3, mqtt_data, frequency_chr); }
    }
    snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_ENERGY_SNS4, mqtt_data, energy_daily_chr, energy_yesterday_chr, energy_total_chr);
#endif
  }
}





#define XDRV_03 

boolean Xdrv03(byte function)
{
  boolean result = false;

  if (FUNC_PRE_INIT == function) {
    EnergyDrvInit();
  }
  else if (energy_flg) {
    switch (function) {
      case FUNC_COMMAND:
        result = EnergyCommand();
        break;
      case FUNC_SET_POWER:
        EnergySetPowerSteadyCounter();
        break;
      case FUNC_SERIAL:
        result = XnrgCall(FUNC_SERIAL);
        break;
    }
  }
  return result;
}

#define XSNS_03 

boolean Xsns03(byte function)
{
  boolean result = false;

  if (energy_flg) {
    switch (function) {
      case FUNC_INIT:
        EnergySnsInit();
        break;
      case FUNC_EVERY_SECOND:
        EnergyMarginCheck();
        break;
      case FUNC_JSON_APPEND:
        EnergyShow(1);
        break;
#ifdef USE_WEBSERVER
      case FUNC_WEB_APPEND:
        EnergyShow(0);
        break;
#endif
      case FUNC_SAVE_BEFORE_RESTART:
        EnergySaveState();
        break;
    }
  }
  return result;
}

#endif
# 1 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xdrv_04_light.ino"
# 54 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xdrv_04_light.ino"
#define WS2812_SCHEMES 7

enum LightCommands {
  CMND_COLOR, CMND_COLORTEMPERATURE, CMND_DIMMER, CMND_LED, CMND_LEDTABLE, CMND_FADE,
  CMND_PIXELS, CMND_RGBWWTABLE, CMND_ROTATION, CMND_SCHEME, CMND_SPEED, CMND_WAKEUP, CMND_WAKEUPDURATION,
  CMND_WIDTH, CMND_CHANNEL, CMND_HSBCOLOR, CMND_UNDOCA };
const char kLightCommands[] PROGMEM =
  D_CMND_COLOR "|" D_CMND_COLORTEMPERATURE "|" D_CMND_DIMMER "|" D_CMND_LED "|" D_CMND_LEDTABLE "|" D_CMND_FADE "|"
  D_CMND_PIXELS "|" D_CMND_RGBWWTABLE "|" D_CMND_ROTATION "|" D_CMND_SCHEME "|" D_CMND_SPEED "|" D_CMND_WAKEUP "|" D_CMND_WAKEUPDURATION "|"
  D_CMND_WIDTH "|" D_CMND_CHANNEL "|" D_CMND_HSBCOLOR "|UNDOCA" ;

struct LRgbColor {
  uint8_t R, G, B;
};
#define MAX_FIXED_COLOR 12
const LRgbColor kFixedColor[MAX_FIXED_COLOR] PROGMEM =
  { 255,0,0, 0,255,0, 0,0,255, 228,32,0, 0,228,32, 0,32,228, 188,64,0, 0,160,96, 160,32,240, 255,255,0, 255,0,170, 255,255,255 };

struct LWColor {
  uint8_t W;
};
#define MAX_FIXED_WHITE 4
const LWColor kFixedWhite[MAX_FIXED_WHITE] PROGMEM = { 0, 255, 128, 32 };

struct LCwColor {
  uint8_t C, W;
};
#define MAX_FIXED_COLD_WARM 4
const LCwColor kFixedColdWarm[MAX_FIXED_COLD_WARM] PROGMEM = { 0,0, 255,0, 0,255, 128,128 };

uint8_t ledTable[] = {
  0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 4, 4,
  4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 6, 7, 7, 7, 7, 8,
  8, 8, 9, 9, 9, 10, 10, 10, 11, 11, 12, 12, 12, 13, 13, 14,
 14, 15, 15, 15, 16, 16, 17, 17, 18, 18, 19, 19, 20, 20, 21, 22,
 22, 23, 23, 24, 25, 25, 26, 26, 27, 28, 28, 29, 30, 30, 31, 32,
 33, 33, 34, 35, 36, 36, 37, 38, 39, 40, 40, 41, 42, 43, 44, 45,
 46, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60,
 61, 62, 63, 64, 65, 67, 68, 69, 70, 71, 72, 73, 75, 76, 77, 78,
 80, 81, 82, 83, 85, 86, 87, 89, 90, 91, 93, 94, 95, 97, 98, 99,
101,102,104,105,107,108,110,111,113,114,116,117,119,121,122,124,
125,127,129,130,132,134,135,137,139,141,142,144,146,148,150,151,
153,155,157,159,161,163,165,166,168,170,172,174,176,178,180,182,
184,186,189,191,193,195,197,199,201,204,206,208,210,212,215,217,
219,221,224,226,228,231,233,235,238,240,243,245,248,250,253,255 };

uint8_t light_entry_color[5];
uint8_t light_current_color[5];
uint8_t light_new_color[5];
uint8_t light_last_color[5];
uint8_t light_signal_color[5];

uint8_t light_wheel = 0;
uint8_t light_subtype = 0;
uint8_t light_device = 0;
uint8_t light_power = 0;
uint8_t light_update = 1;
uint8_t light_wakeup_active = 0;
uint8_t light_wakeup_dimmer = 0;
uint16_t light_wakeup_counter = 0;

uint8_t light_fixed_color_index = 1;

unsigned long strip_timer_counter = 0;

#ifdef USE_ARILUX_RF




#define ARILUX_RF_TIME_AVOID_DUPLICATE 1000

#define ARILUX_RF_MAX_CHANGES 51
#define ARILUX_RF_SEPARATION_LIMIT 4300
#define ARILUX_RF_RECEIVE_TOLERANCE 60

unsigned int arilux_rf_timings[ARILUX_RF_MAX_CHANGES];

unsigned long arilux_rf_received_value = 0;
unsigned long arilux_rf_last_received_value = 0;
unsigned long arilux_rf_last_time = 0;
unsigned long arilux_rf_lasttime = 0;

unsigned int arilux_rf_change_count = 0;
unsigned int arilux_rf_repeat_count = 0;

uint8_t arilux_rf_toggle = 0;


#ifndef ARDUINO_ESP8266_RELEASE_2_3_0
#ifndef USE_WS2812_DMA
void AriluxRfInterrupt() ICACHE_RAM_ATTR;
#endif
#endif

void AriluxRfInterrupt()
{
  unsigned long time = micros();
  unsigned int duration = time - arilux_rf_lasttime;

  if (duration > ARILUX_RF_SEPARATION_LIMIT) {
    if (abs(duration - arilux_rf_timings[0]) < 200) {
      arilux_rf_repeat_count++;
      if (arilux_rf_repeat_count == 2) {
        unsigned long code = 0;
        const unsigned int delay = arilux_rf_timings[0] / 31;
        const unsigned int delayTolerance = delay * ARILUX_RF_RECEIVE_TOLERANCE / 100;
        for (unsigned int i = 1; i < arilux_rf_change_count -1; i += 2) {
          code <<= 1;
          if (abs(arilux_rf_timings[i] - (delay *3)) < delayTolerance && abs(arilux_rf_timings[i +1] - delay) < delayTolerance) {
            code |= 1;
          }
        }
        if (arilux_rf_change_count > 49) {
          arilux_rf_received_value = code;
        }
        arilux_rf_repeat_count = 0;
      }
    }
    arilux_rf_change_count = 0;
  }
  if (arilux_rf_change_count >= ARILUX_RF_MAX_CHANGES) {
    arilux_rf_change_count = 0;
    arilux_rf_repeat_count = 0;
  }
  arilux_rf_timings[arilux_rf_change_count++] = duration;
  arilux_rf_lasttime = time;
}

void AriluxRfHandler()
{
  unsigned long now = millis();
  if (arilux_rf_received_value && !((arilux_rf_received_value == arilux_rf_last_received_value) && (now - arilux_rf_last_time < ARILUX_RF_TIME_AVOID_DUPLICATE))) {
    arilux_rf_last_received_value = arilux_rf_received_value;
    arilux_rf_last_time = now;

    uint16_t hostcode = arilux_rf_received_value >> 8 & 0xFFFF;
    if (Settings.rf_code[1][6] == Settings.rf_code[1][7]) {
      Settings.rf_code[1][6] = hostcode >> 8 & 0xFF;
      Settings.rf_code[1][7] = hostcode & 0xFF;
    }
    uint16_t stored_hostcode = Settings.rf_code[1][6] << 8 | Settings.rf_code[1][7];

    snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_RFR D_HOST D_CODE " 0x%04X, " D_RECEIVED " 0x%06X"), stored_hostcode, arilux_rf_received_value);
    AddLog(LOG_LEVEL_DEBUG);

    if (hostcode == stored_hostcode) {
      char command[33];
      char value = '-';
      command[0] = '\0';
      uint8_t keycode = arilux_rf_received_value & 0xFF;
      switch (keycode) {
        case 1:
        case 3:
          snprintf_P(command, sizeof(command), PSTR(D_CMND_POWER " %d"), (1 == keycode) ? 1 : 0);
          break;
        case 2:
          arilux_rf_toggle++;
          arilux_rf_toggle &= 0x3;
          snprintf_P(command, sizeof(command), PSTR(D_CMND_COLOR " %d"), 200 + arilux_rf_toggle);
          break;
        case 4:
          value = '+';
        case 7:
          snprintf_P(command, sizeof(command), PSTR(D_CMND_SPEED " %c"), value);
          break;
        case 5:
          value = '+';
        case 8:
          snprintf_P(command, sizeof(command), PSTR(D_CMND_SCHEME " %c"), value);
          break;
        case 6:
          value = '+';
        case 9:
          snprintf_P(command, sizeof(command), PSTR(D_CMND_DIMMER " %c"), value);
          break;
        default: {
          if ((keycode >= 10) && (keycode <= 21)) {
            snprintf_P(command, sizeof(command), PSTR(D_CMND_COLOR " %d"), keycode -9);
          }
        }
      }
      if (strlen(command)) {
        ExecuteCommand(command, SRC_LIGHT);
      }
    }
  }
  arilux_rf_received_value = 0;
}

void AriluxRfInit()
{
  if ((pin[GPIO_ARIRFRCV] < 99) && (pin[GPIO_LED2] < 99)) {
    if (Settings.last_module != Settings.module) {
      Settings.rf_code[1][6] = 0;
      Settings.rf_code[1][7] = 0;
      Settings.last_module = Settings.module;
    }
    arilux_rf_received_value = 0;
    digitalWrite(pin[GPIO_LED2], !bitRead(led_inverted, 1));
    attachInterrupt(pin[GPIO_ARIRFRCV], AriluxRfInterrupt, CHANGE);
  }
}

void AriluxRfDisable()
{
  if ((pin[GPIO_ARIRFRCV] < 99) && (pin[GPIO_LED2] < 99)) {
    detachInterrupt(pin[GPIO_ARIRFRCV]);
    digitalWrite(pin[GPIO_LED2], bitRead(led_inverted, 1));
  }
}
#endif





extern "C" {
  void os_delay_us(unsigned int);
}

uint8_t light_pdi_pin;
uint8_t light_pdcki_pin;

void LightDiPulse(uint8_t times)
{
  for (uint8_t i = 0; i < times; i++) {
    digitalWrite(light_pdi_pin, HIGH);
    digitalWrite(light_pdi_pin, LOW);
  }
}

void LightDckiPulse(uint8_t times)
{
  for (uint8_t i = 0; i < times; i++) {
    digitalWrite(light_pdcki_pin, HIGH);
    digitalWrite(light_pdcki_pin, LOW);
  }
}

void LightMy92x1Write(uint8_t data)
{
  for (uint8_t i = 0; i < 4; i++) {
    digitalWrite(light_pdcki_pin, LOW);
    digitalWrite(light_pdi_pin, (data & 0x80));
    digitalWrite(light_pdcki_pin, HIGH);
    data = data << 1;
    digitalWrite(light_pdi_pin, (data & 0x80));
    digitalWrite(light_pdcki_pin, LOW);
    digitalWrite(light_pdi_pin, LOW);
    data = data << 1;
  }
}

void LightMy92x1Init()
{
  uint8_t chips = light_type -11;

  LightDckiPulse(chips * 32);
  os_delay_us(12);


  LightDiPulse(12);
  os_delay_us(12);
  for (uint8_t n = 0; n < chips; n++) {
    LightMy92x1Write(0x18);
  }
  os_delay_us(12);


  LightDiPulse(16);
  os_delay_us(12);
}

void LightMy92x1Duty(uint8_t duty_r, uint8_t duty_g, uint8_t duty_b, uint8_t duty_w, uint8_t duty_c)
{
  uint8_t channels[2] = { 4, 6 };
  uint8_t didx = light_type -12;
  uint8_t duty[2][6] = {{ duty_r, duty_g, duty_b, duty_w, 0, 0 },
                        { duty_w, duty_c, 0, duty_g, duty_r, duty_b }};

  os_delay_us(12);
  for (uint8_t channel = 0; channel < channels[didx]; channel++) {
    LightMy92x1Write(duty[didx][channel]);
  }
  os_delay_us(12);
  LightDiPulse(8);
  os_delay_us(12);
}



void LightInit()
{
  uint8_t max_scheme = LS_MAX -1;

  light_device = devices_present;
  light_subtype = light_type &7;

  if (light_type < LT_PWM6) {
    for (byte i = 0; i < light_type; i++) {
      Settings.pwm_value[i] = 0;
      if (pin[GPIO_PWM1 +i] < 99) {
        pinMode(pin[GPIO_PWM1 +i], OUTPUT);
      }
    }
    if (LT_PWM1 == light_type || LT_SERIAL == light_type) {
      Settings.light_color[0] = 255;
    }
    if (SONOFF_LED == Settings.module) {
      if (!my_module.gp.io[4]) {
        pinMode(4, OUTPUT);
        digitalWrite(4, LOW);
      }
      if (!my_module.gp.io[5]) {
        pinMode(5, OUTPUT);
        digitalWrite(5, LOW);
      }
      if (!my_module.gp.io[14]) {
        pinMode(14, OUTPUT);
        digitalWrite(14, LOW);
      }
    }
    if (pin[GPIO_ARIRFRCV] < 99) {
      if (pin[GPIO_LED2] < 99) {
        digitalWrite(pin[GPIO_LED2], bitRead(led_inverted, 1));
      }
    }
  }
#ifdef USE_WS2812
  else if (LT_WS2812 == light_type) {
#if (USE_WS2812_CTYPE > NEO_3LED)
    light_subtype++;
#endif
    Ws2812Init();
    max_scheme = LS_MAX + WS2812_SCHEMES;
  }
#endif
  else if (LT_SERIAL == light_type) {
    light_subtype = LST_SINGLE;
  }
  else {
    light_pdi_pin = pin[GPIO_DI];
    light_pdcki_pin = pin[GPIO_DCKI];

    pinMode(light_pdi_pin, OUTPUT);
    pinMode(light_pdcki_pin, OUTPUT);
    digitalWrite(light_pdi_pin, LOW);
    digitalWrite(light_pdcki_pin, LOW);

    LightMy92x1Init();
  }

  if (light_subtype < LST_RGB) {
    max_scheme = LS_POWER;
  }
  if ((LS_WAKEUP == Settings.light_scheme) || (Settings.light_scheme > max_scheme)) {
    Settings.light_scheme = LS_POWER;
  }
  light_power = 0;
  light_update = 1;
  light_wakeup_active = 0;
}

void LightSetColorTemp(uint16_t ct)
{





  uint16_t my_ct = ct - 153;
  if (my_ct > 347) {
    my_ct = 347;
  }
  uint16_t icold = (100 * (347 - my_ct)) / 136;
  uint16_t iwarm = (100 * my_ct) / 136;
  if (PHILIPS == Settings.module) {


    Settings.light_color[1] = (uint8_t)icold;
  } else
  if (LST_RGBWC == light_subtype) {
    Settings.light_color[0] = 0;
    Settings.light_color[1] = 0;
    Settings.light_color[2] = 0;
    Settings.light_color[3] = (uint8_t)icold;
    Settings.light_color[4] = (uint8_t)iwarm;
  } else {
    Settings.light_color[0] = (uint8_t)icold;
    Settings.light_color[1] = (uint8_t)iwarm;
  }
}

uint16_t LightGetColorTemp()
{
  uint8_t ct_idx = 0;
  if (LST_RGBWC == light_subtype) {
    ct_idx = 3;
  }
  uint16_t my_ct = Settings.light_color[ct_idx +1];
  if (my_ct > 0) {
    return ((my_ct * 136) / 100) + 154;
  } else {
    my_ct = Settings.light_color[ct_idx];
    return 499 - ((my_ct * 136) / 100);
  }
}

void LightSetDimmer(uint8_t myDimmer)
{
  float temp;

  if (PHILIPS == Settings.module) {

    float dimmer = 100 / (float)myDimmer;
    temp = (float)Settings.light_color[0] / dimmer;
    light_current_color[0] = (uint8_t)temp;
    temp = (float)Settings.light_color[1];
    light_current_color[1] = (uint8_t)temp;
    return;
  }
  if (LT_PWM1 == light_type) {
    Settings.light_color[0] = 255;
  }
  float dimmer = 100 / (float)myDimmer;
  for (byte i = 0; i < light_subtype; i++) {
    if (Settings.flag.light_signal) {
      temp = (float)light_signal_color[i] / dimmer;
    } else {
      temp = (float)Settings.light_color[i] / dimmer;
    }
    light_current_color[i] = (uint8_t)temp;
  }
}

void LightSetColor()
{
  uint8_t highest = 0;

  for (byte i = 0; i < light_subtype; i++) {
    if (highest < light_current_color[i]) {
      highest = light_current_color[i];
    }
  }
  float mDim = (float)highest / 2.55;
  Settings.light_dimmer = (uint8_t)mDim;
  float dimmer = 100 / mDim;
  for (byte i = 0; i < light_subtype; i++) {
    float temp = (float)light_current_color[i] * dimmer;
    Settings.light_color[i] = (uint8_t)temp;
  }
}

void LightSetSignal(uint16_t lo, uint16_t hi, uint16_t value)
{



  if (Settings.flag.light_signal) {
    uint16_t signal = 0;
    if (value > lo) {
      signal = (value - lo) * 10 / ((hi - lo) * 10 / 256);
      if (signal > 255) {
        signal = 255;
      }
    }


    light_signal_color[0] = signal;
    light_signal_color[1] = 255 - signal;
    light_signal_color[2] = 0;
    light_signal_color[3] = 0;
    light_signal_color[4] = 0;

    Settings.light_scheme = 0;
    if (!Settings.light_dimmer) {
      Settings.light_dimmer = 20;
    }
  }
}

char* LightGetColor(uint8_t type, char* scolor)
{
  LightSetDimmer(Settings.light_dimmer);
  scolor[0] = '\0';
  for (byte i = 0; i < light_subtype; i++) {
    if (!type && Settings.flag.decimal_text) {
      snprintf_P(scolor, 25, PSTR("%s%s%d"), scolor, (i > 0) ? "," : "", light_current_color[i]);
    } else {
      snprintf_P(scolor, 25, PSTR("%s%02X"), scolor, light_current_color[i]);
    }
  }
  return scolor;
}

void LightPowerOn()
{
  if (Settings.light_dimmer && !(light_power)) {
    ExecuteCommandPower(light_device, POWER_ON, SRC_LIGHT);
  }
}

void LightState(uint8_t append)
{
  char scolor[25];
  char scommand[33];
  float hsb[3];
  int16_t h,s,b;

  if (append) {
    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s,"), mqtt_data);
  } else {
    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{"));
  }
  GetPowerDevice(scommand, light_device, sizeof(scommand), Settings.flag.device_index_enable);
  snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s\"%s\":\"%s\",\"" D_CMND_DIMMER "\":%d"),
    mqtt_data, scommand, GetStateText(light_power), Settings.light_dimmer);
  if (light_subtype > LST_SINGLE) {
    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s,\"" D_CMND_COLOR "\":\"%s\""), mqtt_data, LightGetColor(0, scolor));

    LightGetHsb(&hsb[0],&hsb[1],&hsb[2], false);

    h = round(hsb[0] * 360);
    s = round(hsb[1] * 100);
    b = round(hsb[2] * 100);
    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s,\"" D_CMND_HSBCOLOR "\":\"%d,%d,%d\""), mqtt_data, h,s,b);

    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s,\"" D_CMND_CHANNEL "\":[" ), mqtt_data);
    for (byte i = 0; i < light_subtype; i++) {
      snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s%s%d" ), mqtt_data, (i > 0 ? "," : ""), round(light_current_color[i]/2.55));
    }
    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s]" ), mqtt_data);
  }
  if ((LST_COLDWARM == light_subtype) || (LST_RGBWC == light_subtype)) {
    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s,\"" D_CMND_COLORTEMPERATURE "\":%d"), mqtt_data, LightGetColorTemp());
  }
  if (append) {
    if (light_subtype >= LST_RGB) {
      snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s,\"" D_CMND_SCHEME "\":%d"), mqtt_data, Settings.light_scheme);
    }
    if (LT_WS2812 == light_type) {
      snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s,\"" D_CMND_WIDTH "\":%d"), mqtt_data, Settings.light_width);
    }
    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s,\"" D_CMND_FADE "\":\"%s\",\"" D_CMND_SPEED "\":%d,\"" D_CMND_LEDTABLE "\":\"%s\""),
      mqtt_data, GetStateText(Settings.light_fade), Settings.light_speed, GetStateText(Settings.light_correction));
  } else {
    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s}"), mqtt_data);
  }
}

void LightPreparePower()
{
  if (Settings.light_dimmer && !(light_power)) {
    if (!Settings.flag.not_power_linked) {
      ExecuteCommandPower(light_device, POWER_ON_NO_STATE, SRC_LIGHT);
    }
  }
  else if (!Settings.light_dimmer && light_power) {
    ExecuteCommandPower(light_device, POWER_OFF_NO_STATE, SRC_LIGHT);
  }
#ifdef USE_DOMOTICZ
  DomoticzUpdatePowerState(light_device);
#endif

  LightState(0);
}

void LightFade()
{
  if (0 == Settings.light_fade) {
    for (byte i = 0; i < light_subtype; i++) {
      light_new_color[i] = light_current_color[i];
    }
  } else {
    uint8_t shift = Settings.light_speed;
    if (Settings.light_speed > 6) {
      shift = (strip_timer_counter % (Settings.light_speed -6)) ? 0 : 8;
    }
    if (shift) {
      for (byte i = 0; i < light_subtype; i++) {
        if (light_new_color[i] != light_current_color[i]) {
          if (light_new_color[i] < light_current_color[i]) {
            light_new_color[i] += ((light_current_color[i] - light_new_color[i]) >> shift) +1;
          }
          if (light_new_color[i] > light_current_color[i]) {
            light_new_color[i] -= ((light_new_color[i] - light_current_color[i]) >> shift) +1;
          }
        }
      }
    }
  }
}

void LightWheel(uint8_t wheel_pos)
{
  wheel_pos = 255 - wheel_pos;
  if (wheel_pos < 85) {
    light_entry_color[0] = 255 - wheel_pos * 3;
    light_entry_color[1] = 0;
    light_entry_color[2] = wheel_pos * 3;
  } else if (wheel_pos < 170) {
    wheel_pos -= 85;
    light_entry_color[0] = 0;
    light_entry_color[1] = wheel_pos * 3;
    light_entry_color[2] = 255 - wheel_pos * 3;
  } else {
    wheel_pos -= 170;
    light_entry_color[0] = wheel_pos * 3;
    light_entry_color[1] = 255 - wheel_pos * 3;
    light_entry_color[2] = 0;
  }
  light_entry_color[3] = 0;
  light_entry_color[4] = 0;
  float dimmer = 100 / (float)Settings.light_dimmer;
  for (byte i = 0; i < LST_RGB; i++) {
    float temp = (float)light_entry_color[i] / dimmer;
    light_entry_color[i] = (uint8_t)temp;
  }
}

void LightCycleColor(int8_t direction)
{
  if (strip_timer_counter % (Settings.light_speed * 2)) {
    return;
  }
  light_wheel += direction;
  LightWheel(light_wheel);
  memcpy(light_new_color, light_entry_color, sizeof(light_new_color));
}

void LightRandomColor()
{
  uint8_t light_update = 0;
  for (byte i = 0; i < LST_RGB; i++) {
    if (light_new_color[i] != light_current_color[i]) {
      light_update = 1;
    }
  }
  if (!light_update) {
    light_wheel = random(255);
    LightWheel(light_wheel);
    memcpy(light_current_color, light_entry_color, sizeof(light_current_color));
  }
  LightFade();
}

void LightSetPower()
{

  light_power = bitRead(XdrvMailbox.index, light_device -1);
  if (light_wakeup_active) {
    light_wakeup_active--;
  }
  if (light_power) {
    light_update = 1;
  }
  LightAnimate();
}

void LightAnimate()
{
  uint8_t cur_col[5];
  uint16_t light_still_on = 0;

  strip_timer_counter++;
  if (!light_power) {
    sleep = Settings.sleep;
    strip_timer_counter = 0;
    for (byte i = 0; i < light_subtype; i++) {
      light_still_on += light_new_color[i];
    }
    if (light_still_on && Settings.light_fade && (Settings.light_scheme < LS_MAX)) {
      uint8_t speed = Settings.light_speed;
      if (speed > 6) {
        speed = 6;
      }
      for (byte i = 0; i < light_subtype; i++) {
        if (light_new_color[i] > 0) {
          light_new_color[i] -= (light_new_color[i] >> speed) +1;
        }
      }
    } else {
      for (byte i = 0; i < light_subtype; i++) {
        light_new_color[i] = 0;
      }
    }
  }
  else {
    sleep = 0;
    switch (Settings.light_scheme) {
      case LS_POWER:
        LightSetDimmer(Settings.light_dimmer);
        LightFade();
        break;
      case LS_WAKEUP:
        if (2 == light_wakeup_active) {
          light_wakeup_active = 1;
          for (byte i = 0; i < light_subtype; i++) {
            light_new_color[i] = 0;
          }
          light_wakeup_counter = 0;
          light_wakeup_dimmer = 0;
        }
        light_wakeup_counter++;
        if (light_wakeup_counter > ((Settings.light_wakeup * STATES) / Settings.light_dimmer)) {
          light_wakeup_counter = 0;
          light_wakeup_dimmer++;
          if (light_wakeup_dimmer <= Settings.light_dimmer) {
            LightSetDimmer(light_wakeup_dimmer);
            for (byte i = 0; i < light_subtype; i++) {
              light_new_color[i] = light_current_color[i];
            }
          } else {
            snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"" D_CMND_WAKEUP "\":\"" D_JSON_DONE "\"}"));
            MqttPublishPrefixTopic_P(TELE, PSTR(D_CMND_WAKEUP));
            light_wakeup_active = 0;
            Settings.light_scheme = LS_POWER;
          }
        }
        break;
      case LS_CYCLEUP:
        LightCycleColor(1);
        break;
      case LS_CYCLEDN:
        LightCycleColor(-1);
        break;
      case LS_RANDOM:
        LightRandomColor();
        break;
#ifdef USE_WS2812
      default:
        if (LT_WS2812 == light_type) {
          Ws2812ShowScheme(Settings.light_scheme -LS_MAX);
        }
#endif
    }
  }

  if ((Settings.light_scheme < LS_MAX) || !light_power) {
    for (byte i = 0; i < light_subtype; i++) {
      if (light_last_color[i] != light_new_color[i]) {
        light_update = 1;
      }
    }
    if (light_update) {
      light_update = 0;
      for (byte i = 0; i < light_subtype; i++) {
        light_last_color[i] = light_new_color[i];
        cur_col[i] = light_last_color[i]*Settings.rgbwwTable[i]/255;
        cur_col[i] = (Settings.light_correction) ? ledTable[cur_col[i]] : cur_col[i];
        if (light_type < LT_PWM6) {
          if (pin[GPIO_PWM1 +i] < 99) {
            if (cur_col[i] > 0xFC) {
              cur_col[i] = 0xFC;
            }
            uint16_t curcol = cur_col[i] * (Settings.pwm_range / 255);


            analogWrite(pin[GPIO_PWM1 +i], bitRead(pwm_inverted, i) ? Settings.pwm_range - curcol : curcol);
          }
        }
      }
#ifdef USE_WS2812
      if (LT_WS2812 == light_type) {
        Ws2812SetColor(0, cur_col[0], cur_col[1], cur_col[2], cur_col[3]);
      }
#endif
      if (light_type > LT_WS2812) {
        LightMy92x1Duty(cur_col[0], cur_col[1], cur_col[2], cur_col[3], cur_col[4]);
      }
#ifdef USE_TUYA_DIMMER
      if (light_type == LT_SERIAL) {
        LightSerialDuty(cur_col[0]);
      }
#endif
    }
  }
}





float light_hue = 0.0;
float light_saturation = 0.0;
float light_brightness = 0.0;

void LightRgbToHsb()
{
  LightSetDimmer(Settings.light_dimmer);


  float r = light_current_color[0] / 255.0f;
  float g = light_current_color[1] / 255.0f;
  float b = light_current_color[2] / 255.0f;

  float max = (r > g && r > b) ? r : (g > b) ? g : b;
  float min = (r < g && r < b) ? r : (g < b) ? g : b;

  float d = max - min;

  light_hue = 0.0;
  light_brightness = max;
  light_saturation = (0.0f == light_brightness) ? 0 : (d / light_brightness);

  if (d != 0.0f)
  {
    if (r == max) {
      light_hue = (g - b) / d + (g < b ? 6.0f : 0.0f);
    } else if (g == max) {
      light_hue = (b - r) / d + 2.0f;
    } else {
      light_hue = (r - g) / d + 4.0f;
    }
    light_hue /= 6.0f;
  }
}

void LightHsbToRgb()
{
  float r;
  float g;
  float b;

  float h = light_hue;
  float s = light_saturation;
  float v = light_brightness;

  if (0.0f == light_saturation) {
    r = g = b = v;
  } else {
    if (h < 0.0f) {
      h += 1.0f;
    }
    else if (h >= 1.0f) {
      h -= 1.0f;
    }
    h *= 6.0f;
    int i = (int)h;
    float f = h - i;
    float q = v * (1.0f - s * f);
    float p = v * (1.0f - s);
    float t = v * (1.0f - s * (1.0f - f));
    switch (i) {
      case 0:
        r = v;
        g = t;
        b = p;
        break;
      case 1:
        r = q;
        g = v;
        b = p;
        break;
      case 2:
        r = p;
        g = v;
        b = t;
        break;
      case 3:
        r = p;
        g = q;
        b = v;
        break;
      case 4:
        r = t;
        g = p;
        b = v;
        break;
      default:
        r = v;
        g = p;
        b = q;
        break;
      }
  }

  light_current_color[0] = (uint8_t)(r * 255.0f);
  light_current_color[1] = (uint8_t)(g * 255.0f);
  light_current_color[2] = (uint8_t)(b * 255.0f);
  light_current_color[3] = 0;
  light_current_color[4] = 0;
}



void LightGetHsb(float *hue, float *sat, float *bri, bool gotct)
{
  if (light_subtype > LST_COLDWARM && !gotct) {
    LightRgbToHsb();
    *hue = light_hue;
    *sat = light_saturation;
    *bri = light_brightness;
  } else {
    *hue = 0;
    *sat = 0;
    *bri = (0.01f * (float)Settings.light_dimmer);
  }
}

void LightSetHsb(float hue, float sat, float bri, uint16_t ct, bool gotct)
{
  if (light_subtype > LST_COLDWARM) {
    if ((LST_RGBWC == light_subtype) && (gotct)) {
      uint8_t tmp = (uint8_t)(bri * 100);
      Settings.light_dimmer = tmp;
      if (ct > 0) {
        LightSetColorTemp(ct);
      }
    } else {
      light_hue = hue;
      light_saturation = sat;
      light_brightness = bri;
      LightHsbToRgb();
      LightSetColor();
    }
    LightPreparePower();
    MqttPublishPrefixTopic_P(RESULT_OR_STAT, PSTR(D_CMND_COLOR));
  } else {
    uint8_t tmp = (uint8_t)(bri * 100);
    Settings.light_dimmer = tmp;
    if (LST_COLDWARM == light_subtype) {
      if (ct > 0) {
        LightSetColorTemp(ct);
      }
      LightPreparePower();
      MqttPublishPrefixTopic_P(RESULT_OR_STAT, PSTR(D_CMND_COLOR));
    } else {
      LightPreparePower();
      MqttPublishPrefixTopic_P(RESULT_OR_STAT, PSTR(D_CMND_DIMMER));
    }
  }
}





boolean LightColorEntry(char *buffer, uint8_t buffer_length)
{
  char scolor[10];
  char *p;
  char *str;
  uint8_t entry_type = 0;
  uint8_t value = light_fixed_color_index;

  if (buffer[0] == '#') {
    buffer++;
    buffer_length--;
  }

  if (light_subtype >= LST_RGB) {
    char option = (1 == buffer_length) ? buffer[0] : '\0';
    if (('+' == option) && (light_fixed_color_index < MAX_FIXED_COLOR)) {
      value++;
    }
    else if (('-' == option) && (light_fixed_color_index > 1)) {
      value--;
    } else {
      value = atoi(buffer);
    }
  }

  memset(&light_entry_color, 0x00, sizeof(light_entry_color));
  if (strstr(buffer, ",")) {
    int8_t i = 0;
    for (str = strtok_r(buffer, ",", &p); str && i < 6; str = strtok_r(NULL, ",", &p)) {
      if (i < 5) {
        light_entry_color[i++] = atoi(str);
      }
    }
    entry_type = 2;
  }
  else if (((2 * light_subtype) == buffer_length) || (buffer_length > 3)) {
    for (byte i = 0; i < buffer_length / 2; i++) {
      strlcpy(scolor, buffer + (i *2), 3);
      light_entry_color[i] = (uint8_t)strtol(scolor, &p, 16);
    }
    entry_type = 1;
  }
  else if ((light_subtype >= LST_RGB) && (value > 0) && (value <= MAX_FIXED_COLOR)) {
    light_fixed_color_index = value;
    memcpy_P(&light_entry_color, &kFixedColor[value -1], 3);
    entry_type = 1;
  }
  else if ((value > 199) && (value <= 199 + MAX_FIXED_COLD_WARM)) {
    if (LST_RGBW == light_subtype) {
      memcpy_P(&light_entry_color[3], &kFixedWhite[value -200], 1);
      entry_type = 1;
    }
    else if (LST_COLDWARM == light_subtype) {
      memcpy_P(&light_entry_color, &kFixedColdWarm[value -200], 2);
      entry_type = 1;
    }
    else if (LST_RGBWC == light_subtype) {
      memcpy_P(&light_entry_color[3], &kFixedColdWarm[value -200], 2);
      entry_type = 1;
    }
  }
  if (entry_type) {
    Settings.flag.decimal_text = entry_type -1;
  }
  return (entry_type);
}




boolean LightCommand()
{
  char command [CMDSZ];
  boolean serviced = true;
  boolean coldim = false;
  boolean valid_entry = false;
  char scolor[25];
  char option = (1 == XdrvMailbox.data_len) ? XdrvMailbox.data[0] : '\0';

  int command_code = GetCommandCode(command, sizeof(command), XdrvMailbox.topic, kLightCommands);
  if (-1 == command_code) {
    serviced = false;
  }
  else if ((CMND_COLOR == command_code) && (light_subtype > LST_SINGLE) && (XdrvMailbox.index > 0) && (XdrvMailbox.index <= 6)) {
    if (XdrvMailbox.data_len > 0) {
      valid_entry = LightColorEntry(XdrvMailbox.data, XdrvMailbox.data_len);
      if (valid_entry) {
        if (XdrvMailbox.index <= 2) {
          memcpy(light_current_color, light_entry_color, sizeof(light_current_color));
          uint8_t dimmer = Settings.light_dimmer;
          LightSetColor();
          if (2 == XdrvMailbox.index) {
            Settings.light_dimmer = dimmer;
          }
          Settings.light_scheme = 0;
          coldim = true;
        } else {
          for (byte i = 0; i < LST_RGB; i++) {
            Settings.ws_color[XdrvMailbox.index -3][i] = light_entry_color[i];
          }
        }
      }
    }
    if (!valid_entry && (XdrvMailbox.index <= 2)) {
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_SVALUE, command, LightGetColor(0, scolor));
    }
    if (XdrvMailbox.index >= 3) {
      scolor[0] = '\0';
      for (byte i = 0; i < LST_RGB; i++) {
        if (Settings.flag.decimal_text) {
          snprintf_P(scolor, 25, PSTR("%s%s%d"), scolor, (i > 0) ? "," : "", Settings.ws_color[XdrvMailbox.index -3][i]);
        } else {
          snprintf_P(scolor, 25, PSTR("%s%02X"), scolor, Settings.ws_color[XdrvMailbox.index -3][i]);
        }
      }
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_INDEX_SVALUE, command, XdrvMailbox.index, scolor);
    }
  }
  else if ((CMND_CHANNEL == command_code) && (XdrvMailbox.index > 0) && (XdrvMailbox.index <= light_subtype ) ) {

    if ((XdrvMailbox.payload >= 0) && (XdrvMailbox.payload <= 100)) {
      uint8_t level = XdrvMailbox.payload;
      light_current_color[XdrvMailbox.index-1] = round(level * 2.55);
      LightSetColor();
      coldim = true;
    }
    snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_INDEX_NVALUE, command, XdrvMailbox.index, round(light_current_color[XdrvMailbox.index -1] / 2.55));
  }
  else if ((CMND_HSBCOLOR == command_code) && ( light_subtype >= LST_RGB)) {
    bool validHSB = (XdrvMailbox.data_len > 0);
    if (validHSB) {
      uint16_t HSB[3];
      if (strstr(XdrvMailbox.data, ",")) {
        for (int i = 0; i < 3; i++) {
          char *substr;

          if (0 == i) {
            substr = strtok(XdrvMailbox.data, ",");
          } else {
            substr = strtok(NULL, ",");
          }
          if (substr != NULL) {
            HSB[i] = atoi(substr);
          } else {
            validHSB = false;
          }
        }
      } else {
        float hsb[3];

        LightGetHsb(&hsb[0],&hsb[1],&hsb[2], false);
        HSB[0] = round(hsb[0] * 360);
        HSB[1] = round(hsb[1] * 100);
        HSB[2] = round(hsb[2] * 100);
        if ((XdrvMailbox.index > 0) && (XdrvMailbox.index < 4)) {
          HSB[XdrvMailbox.index -1] = XdrvMailbox.payload;
        } else {
          validHSB = false;
        }
      }
      if (validHSB) {


        LightSetHsb(( (HSB[0]>360) ? (HSB[0] % 360) : HSB[0] ) /360.0,
                    ( (HSB[1]>100) ? (HSB[1] % 100) : HSB[1] ) /100.0,
                    ( (HSB[2]>100) ? (HSB[2] % 100) : HSB[2] ) /100.0,
                    0,
                    false);
      }
    } else {
      LightState(0);
    }
  }
#ifdef USE_WS2812
  else if ((CMND_LED == command_code) && (LT_WS2812 == light_type) && (XdrvMailbox.index > 0) && (XdrvMailbox.index <= Settings.light_pixels)) {
    if (XdrvMailbox.data_len > 0) {
      char *p;
      uint16_t idx = XdrvMailbox.index;
      Ws2812ForceSuspend();
      for (char *color = strtok_r(XdrvMailbox.data, " ", &p); color; color = strtok_r(NULL, " ", &p)) {
        if (LightColorEntry(color, strlen(color))) {
          Ws2812SetColor(idx, light_entry_color[0], light_entry_color[1], light_entry_color[2], light_entry_color[3]);
          idx++;
          if (idx >= Settings.light_pixels) break;
        } else {
          break;
        }
      }

      Ws2812ForceUpdate();
    }
    snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_INDEX_SVALUE, command, XdrvMailbox.index, Ws2812GetColor(XdrvMailbox.index, scolor));
  }
  else if ((CMND_PIXELS == command_code) && (LT_WS2812 == light_type)) {
    if ((XdrvMailbox.payload > 0) && (XdrvMailbox.payload <= WS2812_MAX_LEDS)) {
      Settings.light_pixels = XdrvMailbox.payload;
      Settings.light_rotation = 0;
      Ws2812Clear();
      light_update = 1;
    }
    snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_NVALUE, command, Settings.light_pixels);
  }
  else if ((CMND_ROTATION == command_code) && (LT_WS2812 == light_type)) {
    if ((XdrvMailbox.payload >= 0) && (XdrvMailbox.payload < Settings.light_pixels)) {
      Settings.light_rotation = XdrvMailbox.payload;
    }
    snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_NVALUE, command, Settings.light_rotation);
  }
  else if ((CMND_WIDTH == command_code) && (LT_WS2812 == light_type) && (XdrvMailbox.index > 0) && (XdrvMailbox.index <= 4)) {
    if (1 == XdrvMailbox.index) {
      if ((XdrvMailbox.payload >= 0) && (XdrvMailbox.payload <= 4)) {
        Settings.light_width = XdrvMailbox.payload;
      }
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_NVALUE, command, Settings.light_width);
    } else {
      if ((XdrvMailbox.payload > 0) && (XdrvMailbox.payload < 32)) {
        Settings.ws_width[XdrvMailbox.index -2] = XdrvMailbox.payload;
      }
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_INDEX_NVALUE, command, XdrvMailbox.index, Settings.ws_width[XdrvMailbox.index -2]);
    }
  }
#endif
  else if ((CMND_SCHEME == command_code) && (light_subtype >= LST_RGB)) {
    uint8_t max_scheme = (LT_WS2812 == light_type) ? LS_MAX + WS2812_SCHEMES : LS_MAX -1;
    if (('+' == option) && (Settings.light_scheme < max_scheme)) {
      XdrvMailbox.payload = Settings.light_scheme + ((0 == Settings.light_scheme) ? 2 : 1);
    }
    else if (('-' == option) && (Settings.light_scheme > 0)) {
      XdrvMailbox.payload = Settings.light_scheme - ((2 == Settings.light_scheme) ? 2 : 1);
    }
    if ((XdrvMailbox.payload >= 0) && (XdrvMailbox.payload <= max_scheme)) {
      Settings.light_scheme = XdrvMailbox.payload;
      if (LS_WAKEUP == Settings.light_scheme) {
        light_wakeup_active = 3;
      }
      LightPowerOn();
      strip_timer_counter = 0;
    }
    snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_NVALUE, command, Settings.light_scheme);
  }
  else if (CMND_WAKEUP == command_code) {
    if ((XdrvMailbox.payload >= 0) && (XdrvMailbox.payload <= 100)) {
      Settings.light_dimmer = XdrvMailbox.payload;
    }
    light_wakeup_active = 3;
    Settings.light_scheme = LS_WAKEUP;
    LightPowerOn();
    snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_SVALUE, command, D_JSON_STARTED);
  }
  else if ((CMND_COLORTEMPERATURE == command_code) && ((LST_COLDWARM == light_subtype) || (LST_RGBWC == light_subtype))) {
    if (option != '\0') {
      uint16_t value = LightGetColorTemp();
      if ('+' == option) {
        XdrvMailbox.payload = (value > 466) ? 500 : value + 34;
      }
      else if ('-' == option) {
        XdrvMailbox.payload = (value < 187) ? 153 : value - 34;
      }
    }
    if ((XdrvMailbox.payload >= 153) && (XdrvMailbox.payload <= 500)) {
      LightSetColorTemp(XdrvMailbox.payload);
      coldim = true;
    } else {
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_NVALUE, command, LightGetColorTemp());
    }
  }
  else if (CMND_DIMMER == command_code) {
    if ('+' == option) {
      XdrvMailbox.payload = (Settings.light_dimmer > 89) ? 100 : Settings.light_dimmer + 10;
    }
    else if ('-' == option) {
      XdrvMailbox.payload = (Settings.light_dimmer < 11) ? 1 : Settings.light_dimmer - 10;
    }
    if ((XdrvMailbox.payload >= 0) && (XdrvMailbox.payload <= 100)) {
      Settings.light_dimmer = XdrvMailbox.payload;
      light_update = 1;
      coldim = true;
    } else {
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_NVALUE, command, Settings.light_dimmer);
    }
  }
  else if (CMND_LEDTABLE == command_code) {
    if ((XdrvMailbox.payload >= 0) && (XdrvMailbox.payload <= 2)) {
      switch (XdrvMailbox.payload) {
      case 0:
      case 1:
        Settings.light_correction = XdrvMailbox.payload;
        break;
      case 2:
        Settings.light_correction ^= 1;
        break;
      }
      light_update = 1;
    }
    snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_SVALUE, command, GetStateText(Settings.light_correction));
  }
  else if (CMND_RGBWWTABLE == command_code) {
    bool validtable = (XdrvMailbox.data_len > 0);
    char scolor[25];
    if (validtable) {
      uint16_t HSB[3];
      if (strstr(XdrvMailbox.data, ",")) {
        for (int i = 0; i < LST_RGBWC; i++) {
          char *substr;

          if (0 == i) {
            substr = strtok(XdrvMailbox.data, ",");
          } else {
            substr = strtok(NULL, ",");
          }
          if (substr != NULL) {
            Settings.rgbwwTable[i] = atoi(substr);
          }
        }
      }
      light_update = 1;
    }
    scolor[0] = '\0';
    for (byte i = 0; i < LST_RGBWC; i++) {
      snprintf_P(scolor, 25, PSTR("%s%s%d"), scolor, (i > 0) ? "," : "", Settings.rgbwwTable[i]);
    }
    snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_INDEX_SVALUE, command, XdrvMailbox.index, scolor);
  }
  else if (CMND_FADE == command_code) {
    switch (XdrvMailbox.payload) {
    case 0:
    case 1:
      Settings.light_fade = XdrvMailbox.payload;
      break;
    case 2:
      Settings.light_fade ^= 1;
      break;
    }
    snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_SVALUE, command, GetStateText(Settings.light_fade));
  }
  else if (CMND_SPEED == command_code) {
    if (('+' == option) && (Settings.light_speed > 1)) {
      XdrvMailbox.payload = Settings.light_speed -1;
    }
    else if (('-' == option) && (Settings.light_speed < STATES)) {
      XdrvMailbox.payload = Settings.light_speed +1;
    }
    if ((XdrvMailbox.payload > 0) && (XdrvMailbox.payload <= STATES)) {
      Settings.light_speed = XdrvMailbox.payload;
    }
    snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_NVALUE, command, Settings.light_speed);
  }
  else if (CMND_WAKEUPDURATION == command_code) {
    if ((XdrvMailbox.payload > 0) && (XdrvMailbox.payload < 3001)) {
      Settings.light_wakeup = XdrvMailbox.payload;
      light_wakeup_active = 0;
    }
    snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_NVALUE, command, Settings.light_wakeup);
  }
  else if (CMND_UNDOCA == command_code) {
    LightGetColor(1, scolor);
    scolor[6] = '\0';
    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s,%d,%d,%d,%d,%d"),
      scolor, Settings.light_fade, Settings.light_correction, Settings.light_scheme, Settings.light_speed, Settings.light_width);
    MqttPublishPrefixTopic_P(STAT, XdrvMailbox.topic);
    mqtt_data[0] = '\0';
  }
  else {
    serviced = false;
  }

  if (coldim) {
    LightPreparePower();
  }

  return serviced;
}





#define XDRV_04 

boolean Xdrv04(byte function)
{
  boolean result = false;

  if (light_type) {
    switch (function) {
      case FUNC_PRE_INIT:
        LightInit();
        break;
      case FUNC_EVERY_50_MSECOND:
        LightAnimate();
#ifdef USE_ARILUX_RF
        if (pin[GPIO_ARIRFRCV] < 99) AriluxRfHandler();
#endif
        break;
#ifdef USE_ARILUX_RF
      case FUNC_EVERY_SECOND:
        if (10 == uptime) AriluxRfInit();
        break;
#endif
      case FUNC_COMMAND:
        result = LightCommand();
        break;
      case FUNC_SET_POWER:
        LightSetPower();
        break;
    }
  }
  return result;
}
# 1 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xdrv_05_irremote.ino"
# 20 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xdrv_05_irremote.ino"
#ifdef USE_IR_REMOTE




#include <IRremoteESP8266.h>


const char kIrRemoteProtocols[] PROGMEM =
  "UNKNOWN|RC5|RC6|NEC|SONY|PANASONIC|JVC|SAMSUNG|WHYNTER|AIWA_RC_T501|LG|SANYO|MITSUBISHI|DISH|SHARP";

#ifdef USE_IR_HVAC

#include <ir_Mitsubishi.h>


#define HVAC_TOSHIBA_HDR_MARK 4400
#define HVAC_TOSHIBA_HDR_SPACE 4300
#define HVAC_TOSHIBA_BIT_MARK 543
#define HVAC_TOSHIBA_ONE_SPACE 1623
#define HVAC_MISTUBISHI_ZERO_SPACE 472
#define HVAC_TOSHIBA_RPT_MARK 440
#define HVAC_TOSHIBA_RPT_SPACE 7048
#define HVAC_TOSHIBA_DATALEN 9

IRMitsubishiAC *mitsubir = NULL;

const char kFanSpeedOptions[] = "A12345S";
const char kHvacModeOptions[] = "HDCA";
#endif





#include <IRsend.h>

IRsend *irsend = NULL;

void IrSendInit(void)
{
  irsend = new IRsend(pin[GPIO_IRSEND]);
  irsend->begin();

#ifdef USE_IR_HVAC
  mitsubir = new IRMitsubishiAC(pin[GPIO_IRSEND]);
#endif
}

#ifdef USE_IR_RECEIVE




#include <IRrecv.h>

#define IR_TIME_AVOID_DUPLICATE 500

IRrecv *irrecv = NULL;
unsigned long ir_lasttime = 0;

void IrReceiveInit(void)
{
  irrecv = new IRrecv(pin[GPIO_IRRECV]);
  irrecv->enableIRIn();


}

void IrReceiveCheck()
{
  char sirtype[14];
  char stemp[16];
  int8_t iridx = 0;

  decode_results results;

  if (irrecv->decode(&results)) {

    snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_IRR "RawLen %d, Bits %d, Value %08X, Decode %d"),
               results.rawlen, results.bits, results.value, results.decode_type);
    AddLog(LOG_LEVEL_DEBUG);

    unsigned long now = millis();
    if ((now - ir_lasttime > IR_TIME_AVOID_DUPLICATE) && (UNKNOWN != results.decode_type) && (results.bits > 0)) {
      ir_lasttime = now;

      iridx = results.decode_type;
      if ((iridx < 0) || (iridx > 14)) {
        iridx = 0;
      }

      if (Settings.flag.ir_receive_decimal) {
        snprintf_P(stemp, sizeof(stemp), PSTR("%u"), (uint32_t)results.value);
      } else {
        snprintf_P(stemp, sizeof(stemp), PSTR("\"%lX\""), (uint32_t)results.value);
      }
      snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"" D_JSON_IRRECEIVED "\":{\"" D_JSON_IR_PROTOCOL "\":\"%s\",\"" D_JSON_IR_BITS "\":%d,\"" D_JSON_IR_DATA "\":%s}}"),
        GetTextIndexed(sirtype, sizeof(sirtype), iridx, kIrRemoteProtocols), results.bits, stemp);

      MqttPublishPrefixTopic_P(RESULT_OR_TELE, PSTR(D_JSON_IRRECEIVED));
      XdrvRulesProcess();
#ifdef USE_DOMOTICZ
      unsigned long value = results.value | (iridx << 28);
      DomoticzSensor(DZ_COUNT, value);
#endif
    }

    irrecv->resume();
  }
}
#endif

#ifdef USE_IR_HVAC




boolean IrHvacToshiba(const char *HVAC_Mode, const char *HVAC_FanMode, boolean HVAC_Power, int HVAC_Temp)
{
  uint16_t rawdata[2 + 2 * 8 * HVAC_TOSHIBA_DATALEN + 2];
  byte data[HVAC_TOSHIBA_DATALEN] = {0xF2, 0x0D, 0x03, 0xFC, 0x01, 0x00, 0x00, 0x00, 0x00};

  char *p;
  uint8_t mode;

  if (HVAC_Mode == NULL) {
    p = (char *)kHvacModeOptions;
  }
  else {
    p = strchr(kHvacModeOptions, toupper(HVAC_Mode[0]));
  }
  if (!p) {
    return true;
  }
  data[6] = (p - kHvacModeOptions) ^ 0x03;

  if (!HVAC_Power) {
    data[6] = (byte)0x07;
  }

  if (HVAC_FanMode == NULL) {
    p = (char *)kFanSpeedOptions;
  }
  else {
    p = strchr(kFanSpeedOptions, toupper(HVAC_FanMode[0]));
  }
  if (!p) {
    return true;
  }
  mode = p - kFanSpeedOptions + 1;
  if ((1 == mode) || (7 == mode)) {
    mode = 0;
  }
  mode = mode << 5;
  data[6] = data[6] | mode;

  byte Temp;
  if (HVAC_Temp > 30) {
    Temp = 30;
  }
  else if (HVAC_Temp < 17) {
    Temp = 17;
  }
  else {
    Temp = HVAC_Temp;
  }
  data[5] = (byte)(Temp - 17) << 4;

  data[HVAC_TOSHIBA_DATALEN - 1] = 0;
  for (int x = 0; x < HVAC_TOSHIBA_DATALEN - 1; x++) {
    data[HVAC_TOSHIBA_DATALEN - 1] = (byte)data[x] ^ data[HVAC_TOSHIBA_DATALEN - 1];
  }

  int i = 0;
  byte mask = 1;


  rawdata[i++] = HVAC_TOSHIBA_HDR_MARK;
  rawdata[i++] = HVAC_TOSHIBA_HDR_SPACE;


  for (int b = 0; b < HVAC_TOSHIBA_DATALEN; b++) {
    for (mask = B10000000; mask > 0; mask >>= 1) {
      if (data[b] & mask) {
        rawdata[i++] = HVAC_TOSHIBA_BIT_MARK;
        rawdata[i++] = HVAC_TOSHIBA_ONE_SPACE;
      }
      else {
        rawdata[i++] = HVAC_TOSHIBA_BIT_MARK;
        rawdata[i++] = HVAC_MISTUBISHI_ZERO_SPACE;
      }
    }
  }


  rawdata[i++] = HVAC_TOSHIBA_RPT_MARK;
  rawdata[i++] = HVAC_TOSHIBA_RPT_SPACE;

  noInterrupts();
  irsend->sendRaw(rawdata, i, 38);
  irsend->sendRaw(rawdata, i, 38);
  interrupts();

  return false;
}

boolean IrHvacMitsubishi(const char *HVAC_Mode, const char *HVAC_FanMode, boolean HVAC_Power, int HVAC_Temp)
{
  char *p;
  uint8_t mode;

  mitsubir->stateReset();

  if (HVAC_Mode == NULL) {
    p = (char *)kHvacModeOptions;
  }
  else {
    p = strchr(kHvacModeOptions, toupper(HVAC_Mode[0]));
  }
  if (!p) {
    return true;
  }
  mode = (p - kHvacModeOptions + 1) << 3;
  mitsubir->setMode(mode);

  mitsubir->setPower(HVAC_Power);

  if (HVAC_FanMode == NULL) {
    p = (char *)kFanSpeedOptions;
  }
  else {
    p = strchr(kFanSpeedOptions, toupper(HVAC_FanMode[0]));
  }
  if (!p) {
    return true;
  }
  mode = p - kFanSpeedOptions;
  mitsubir->setFan(mode);

  mitsubir->setTemp(HVAC_Temp);
  mitsubir->setVane(MITSUBISHI_AC_VANE_AUTO);
  mitsubir->send();





  return false;
}
#endif
# 286 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xdrv_05_irremote.ino"
boolean IrSendCommand()
{
  boolean serviced = true;
  boolean error = false;
  char dataBufUc[XdrvMailbox.data_len];
  char protocol_text[20];
  const char *protocol;
  uint32_t bits = 0;
  uint32_t data = 0;

  UpperCase(dataBufUc, XdrvMailbox.data);
  if (!strcasecmp_P(XdrvMailbox.topic, PSTR(D_CMND_IRSEND))) {
    if (XdrvMailbox.data_len) {
      StaticJsonBuffer<128> jsonBuf;
      JsonObject &root = jsonBuf.parseObject(dataBufUc);
      if (!root.success()) {
        snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"" D_CMND_IRSEND "\":\"" D_JSON_INVALID_JSON "\"}"));
      }
      else {
        snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"" D_CMND_IRSEND "\":\"" D_JSON_DONE "\"}"));
        char parm_uc[10];
        protocol = root[UpperCase_P(parm_uc, PSTR(D_JSON_IR_PROTOCOL))];
        bits = root[UpperCase_P(parm_uc, PSTR(D_JSON_IR_BITS))];
        data = strtoul(root[UpperCase_P(parm_uc, PSTR(D_JSON_IR_DATA))], NULL, 0);
        if (protocol && bits) {
          int protocol_code = GetCommandCode(protocol_text, sizeof(protocol_text), protocol, kIrRemoteProtocols);

          snprintf_P(log_data, sizeof(log_data), PSTR("IRS: protocol_text %s, protocol %s, bits %d, data %u (0x%lX), protocol_code %d"),
            protocol_text, protocol, bits, data, data, protocol_code);
          AddLog(LOG_LEVEL_DEBUG);

          switch (protocol_code) {
            case NEC:
              irsend->sendNEC(data, (bits > NEC_BITS) ? NEC_BITS : bits); break;
            case SONY:
              irsend->sendSony(data, (bits > SONY_20_BITS) ? SONY_20_BITS : bits, 2); break;
            case RC5:
              irsend->sendRC5(data, bits); break;
            case RC6:
              irsend->sendRC6(data, bits); break;
            case DISH:
              irsend->sendDISH(data, (bits > DISH_BITS) ? DISH_BITS : bits); break;
            case JVC:
              irsend->sendJVC(data, (bits > JVC_BITS) ? JVC_BITS : bits, 1); break;
            case SAMSUNG:
              irsend->sendSAMSUNG(data, (bits > SAMSUNG_BITS) ? SAMSUNG_BITS : bits); break;
            case PANASONIC:
              irsend->sendPanasonic(bits, data); break;
            default:
              snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"" D_CMND_IRSEND "\":\"" D_JSON_PROTOCOL_NOT_SUPPORTED "\"}"));
          }
        }
        else {
          error = true;
        }
      }
    }
    else {
      error = true;
    }
    if (error) {
      snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"" D_CMND_IRSEND "\":\"" D_JSON_NO " " D_JSON_IR_PROTOCOL ", " D_JSON_IR_BITS " " D_JSON_OR " " D_JSON_IR_DATA "\"}"));
    }
  }
#ifdef USE_IR_HVAC
  else if (!strcasecmp_P(XdrvMailbox.topic, PSTR(D_CMND_IRHVAC))) {
    const char *HVAC_Mode;
    const char *HVAC_FanMode;
    const char *HVAC_Vendor;
    int HVAC_Temp = 21;
    boolean HVAC_Power = true;

    if (XdrvMailbox.data_len) {
      StaticJsonBuffer<164> jsonBufer;
      JsonObject &root = jsonBufer.parseObject(dataBufUc);
      if (!root.success()) {
        snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"" D_CMND_IRHVAC "\":\"" D_JSON_INVALID_JSON "\"}"));
      }
      else {
        snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"" D_CMND_IRHVAC "\":\"" D_JSON_DONE "\"}"));
        HVAC_Vendor = root[D_JSON_IRHVAC_VENDOR];
        HVAC_Power = root[D_JSON_IRHVAC_POWER];
        HVAC_Mode = root[D_JSON_IRHVAC_MODE];
        HVAC_FanMode = root[D_JSON_IRHVAC_FANSPEED];
        HVAC_Temp = root[D_JSON_IRHVAC_TEMP];





        if (HVAC_Vendor == NULL || !strcasecmp_P(HVAC_Vendor, PSTR("TOSHIBA"))) {
          error = IrHvacToshiba(HVAC_Mode, HVAC_FanMode, HVAC_Power, HVAC_Temp);
        }
        else if (!strcasecmp_P(HVAC_Vendor, PSTR("MITSUBISHI"))) {
          error = IrHvacMitsubishi(HVAC_Mode, HVAC_FanMode, HVAC_Power, HVAC_Temp);
        }
        else {
          error = true;
        }
      }
    }
    else {
      error = true;
    }
    if (error) {
      snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"" D_CMND_IRHVAC "\":\"" D_JSON_WRONG " " D_JSON_IRHVAC_VENDOR ", " D_JSON_IRHVAC_MODE " " D_JSON_OR " " D_JSON_IRHVAC_FANSPEED "\"}"));
    }
  }
#endif
  else serviced = false;

  return serviced;
}





#define XDRV_05 

boolean Xdrv05(byte function)
{
  boolean result = false;

  if ((pin[GPIO_IRSEND] < 99) || (pin[GPIO_IRRECV] < 99)) {
    switch (function) {
      case FUNC_PRE_INIT:
        if (pin[GPIO_IRSEND] < 99) {
          IrSendInit();
        }
#ifdef USE_IR_RECEIVE
        if (pin[GPIO_IRRECV] < 99) {
          IrReceiveInit();
        }
#endif
        break;
      case FUNC_EVERY_50_MSECOND:
#ifdef USE_IR_RECEIVE
        if (pin[GPIO_IRRECV] < 99) {
          IrReceiveCheck();
        }
#endif
        break;
      case FUNC_COMMAND:
        if (pin[GPIO_IRSEND] < 99) {
          result = IrSendCommand();
        }
        break;
    }
  }
  return result;
}

#endif
# 1 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xdrv_06_snfbridge.ino"
# 24 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xdrv_06_snfbridge.ino"
#define SFB_TIME_AVOID_DUPLICATE 2000

enum SonoffBridgeCommands {
    CMND_RFSYNC, CMND_RFLOW, CMND_RFHIGH, CMND_RFHOST, CMND_RFCODE, CMND_RFKEY, CMND_RFRAW };
const char kSonoffBridgeCommands[] PROGMEM =
  D_CMND_RFSYNC "|" D_CMND_RFLOW "|" D_CMND_RFHIGH "|" D_CMND_RFHOST "|" D_CMND_RFCODE "|" D_CMND_RFKEY "|" D_CMND_RFRAW;

uint8_t sonoff_bridge_receive_flag = 0;
uint8_t sonoff_bridge_receive_raw_flag = 0;
uint8_t sonoff_bridge_learn_key = 1;
uint8_t sonoff_bridge_learn_active = 0;
uint8_t sonoff_bridge_expected_bytes = 0;
uint32_t sonoff_bridge_last_received_id = 0;
uint32_t sonoff_bridge_last_send_code = 0;
unsigned long sonoff_bridge_last_time = 0;
unsigned long sonoff_bridge_last_learn_time = 0;

#ifdef USE_RF_FLASH







#include "ihx.h"
#include "c2.h"

#define RF_RECORD_NO_START_FOUND -1
#define RF_RECORD_NO_END_FOUND -2

ssize_t rf_find_hex_record_start(uint8_t *buf, size_t size)
{
  for (int i = 0; i < size; i++) {
    if (buf[i] == ':') {
      return i;
    }
  }
  return RF_RECORD_NO_START_FOUND;
}

ssize_t rf_find_hex_record_end(uint8_t *buf, size_t size)
{
  for (ssize_t i = 0; i < size; i++) {
    if (buf[i] == '\n') {
      return i;
    }
  }
  return RF_RECORD_NO_END_FOUND;
}

ssize_t rf_glue_remnant_with_new_data_and_write(const uint8_t *remnant_data, uint8_t *new_data, size_t new_data_len)
{
  ssize_t record_start;
  ssize_t record_end;
  ssize_t glue_record_sz;
  uint8_t *glue_buf;
  ssize_t result;

  if (remnant_data[0] != ':') { return -8; }


  record_end = rf_find_hex_record_end(new_data, new_data_len);
  record_start = rf_find_hex_record_start(new_data, new_data_len);




  if ((record_start != RF_RECORD_NO_START_FOUND) && (record_start < record_end)) {
    return -8;
  }

  glue_record_sz = strlen((const char *) remnant_data) + record_end;

  glue_buf = (uint8_t *) malloc(glue_record_sz);
  if (glue_buf == NULL) { return -2; }


  memcpy(glue_buf, remnant_data, strlen((const char *) remnant_data));
  memcpy(glue_buf + strlen((const char *) remnant_data), new_data, record_end);

  result = rf_decode_and_write(glue_buf, glue_record_sz);
  free(glue_buf);
  return result;
}

ssize_t rf_decode_and_write(uint8_t *record, size_t size)
{
  uint8_t err = ihx_decode(record, size);
  if (err != IHX_SUCCESS) { return -13; }

  ihx_t *h = (ihx_t *) record;
  if (h->record_type == IHX_RT_DATA) {
    int retries = 5;
    uint16_t address = h->address_high * 0x100 + h->address_low;

    do {
      err = c2_programming_init();
      err = c2_block_write(address, h->data, h->len);
    } while (err != C2_SUCCESS && retries--);
  } else if (h->record_type == IHX_RT_END_OF_FILE) {

    err = c2_reset();
  }

  if (err != C2_SUCCESS) { return -12; }

  return 0;
}

ssize_t rf_search_and_write(uint8_t *buf, size_t size)
{

  ssize_t rec_end;
  ssize_t rec_start;
  ssize_t err;

  for (size_t i = 0; i < size; i++) {

    rec_start = rf_find_hex_record_start(buf + i, size - i);
    if (rec_start == RF_RECORD_NO_START_FOUND) {

      return -8;
    }


    rec_start += i;
    rec_end = rf_find_hex_record_end(buf + rec_start, size - rec_start);
    if (rec_end == RF_RECORD_NO_END_FOUND) {

      return rec_start;
    }


    rec_end += rec_start;

    err = rf_decode_and_write(buf + rec_start, rec_end - rec_start);
    if (err < 0) { return err; }
    i = rec_end;
  }

  return 0;
}

uint8_t rf_erase_flash()
{
  uint8_t err;

  for (int i = 0; i < 4; i++) {
    err = c2_programming_init();
    if (err != C2_SUCCESS) {
      return 10;
    }
    err = c2_device_erase();
    if (err != C2_SUCCESS) {
      if (i < 3) {
        c2_reset();
      } else {
        return 11;
      }
    } else {
      break;
    }
  }
  return 0;
}

uint8_t SnfBrUpdateInit()
{
  pinMode(PIN_C2CK, OUTPUT);
  pinMode(PIN_C2D, INPUT);

  return rf_erase_flash();
}
#endif



void SonoffBridgeReceivedRaw()
{

  uint8_t buckets = 0;

  if (0xB1 == serial_in_buffer[1]) { buckets = serial_in_buffer[2] << 1; }

  snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"" D_CMND_RFRAW "\":{\"" D_JSON_DATA "\":\""));
  for (int i = 0; i < serial_in_byte_counter; i++) {
    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s%02X"), mqtt_data, serial_in_buffer[i]);
    if (0xB1 == serial_in_buffer[1]) {
      if ((i > 3) && buckets) { buckets--; }
      if ((i < 3) || (buckets % 2) || (i == serial_in_byte_counter -2)) {
        snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s "), mqtt_data);
      }
    }
  }
  snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s\"}}"), mqtt_data);
  MqttPublishPrefixTopic_P(RESULT_OR_TELE, PSTR(D_CMND_RFRAW));
  XdrvRulesProcess();
}



void SonoffBridgeLearnFailed()
{
  sonoff_bridge_learn_active = 0;
  snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_INDEX_SVALUE, D_CMND_RFKEY, sonoff_bridge_learn_key, D_JSON_LEARN_FAILED);
  MqttPublishPrefixTopic_P(RESULT_OR_STAT, PSTR(D_CMND_RFKEY));
}

void SonoffBridgeReceived()
{
  uint16_t sync_time = 0;
  uint16_t low_time = 0;
  uint16_t high_time = 0;
  uint32_t received_id = 0;
  char rfkey[8];
  char stemp[16];

  AddLogSerial(LOG_LEVEL_DEBUG);

  if (0xA2 == serial_in_buffer[0]) {
    SonoffBridgeLearnFailed();
  }
  else if (0xA3 == serial_in_buffer[0]) {
    sonoff_bridge_learn_active = 0;
    low_time = serial_in_buffer[3] << 8 | serial_in_buffer[4];
    high_time = serial_in_buffer[5] << 8 | serial_in_buffer[6];
    if (low_time && high_time) {
      for (byte i = 0; i < 9; i++) {
        Settings.rf_code[sonoff_bridge_learn_key][i] = serial_in_buffer[i +1];
      }
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_INDEX_SVALUE, D_CMND_RFKEY, sonoff_bridge_learn_key, D_JSON_LEARNED);
      MqttPublishPrefixTopic_P(RESULT_OR_STAT, PSTR(D_CMND_RFKEY));
    } else {
      SonoffBridgeLearnFailed();
    }
  }
  else if (0xA4 == serial_in_buffer[0]) {
    if (sonoff_bridge_learn_active) {
      SonoffBridgeLearnFailed();
    } else {
      sync_time = serial_in_buffer[1] << 8 | serial_in_buffer[2];
      low_time = serial_in_buffer[3] << 8 | serial_in_buffer[4];
      high_time = serial_in_buffer[5] << 8 | serial_in_buffer[6];
      received_id = serial_in_buffer[7] << 16 | serial_in_buffer[8] << 8 | serial_in_buffer[9];

      unsigned long now = millis();
      if (!((received_id == sonoff_bridge_last_received_id) && (now - sonoff_bridge_last_time < SFB_TIME_AVOID_DUPLICATE))) {
        sonoff_bridge_last_received_id = received_id;
        sonoff_bridge_last_time = now;
        strncpy_P(rfkey, PSTR("\"" D_JSON_NONE "\""), sizeof(rfkey));
        for (byte i = 1; i <= 16; i++) {
          if (Settings.rf_code[i][0]) {
            uint32_t send_id = Settings.rf_code[i][6] << 16 | Settings.rf_code[i][7] << 8 | Settings.rf_code[i][8];
            if (send_id == received_id) {
              snprintf_P(rfkey, sizeof(rfkey), PSTR("%d"), i);
              break;
            }
          }
        }
        if (Settings.flag.rf_receive_decimal) {
          snprintf_P(stemp, sizeof(stemp), PSTR("%u"), received_id);
        } else {
          snprintf_P(stemp, sizeof(stemp), PSTR("\"%06X\""), received_id);
        }
        snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"" D_JSON_RFRECEIVED "\":{\"" D_JSON_SYNC "\":%d,\"" D_JSON_LOW "\":%d,\"" D_JSON_HIGH "\":%d,\"" D_JSON_DATA "\":%s,\"" D_CMND_RFKEY "\":%s}}"),
          sync_time, low_time, high_time, stemp, rfkey);
        MqttPublishPrefixTopic_P(RESULT_OR_TELE, PSTR(D_JSON_RFRECEIVED));
        XdrvRulesProcess();
  #ifdef USE_DOMOTICZ
        DomoticzSensor(DZ_COUNT, received_id);
  #endif
      }
    }
  }
}

boolean SonoffBridgeSerialInput()
{

  static int8_t receive_len = 0;

  if (sonoff_bridge_receive_flag) {
    if (sonoff_bridge_receive_raw_flag) {
      if (!serial_in_byte_counter) {
        serial_in_buffer[serial_in_byte_counter++] = 0xAA;
      }
      serial_in_buffer[serial_in_byte_counter++] = serial_in_byte;
      if (serial_in_byte_counter == 3) {
        if ((0xA6 == serial_in_buffer[1]) || (0xAB == serial_in_buffer[1])) {
          receive_len = serial_in_buffer[2] + 4;
        }
      }
      if ((!receive_len && (0x55 == serial_in_byte)) || (receive_len && (serial_in_byte_counter == receive_len))) {
        SonoffBridgeReceivedRaw();
        sonoff_bridge_receive_flag = 0;
        return 1;
      }
    }
    else if (!((0 == serial_in_byte_counter) && (0 == serial_in_byte))) {
      if (0 == serial_in_byte_counter) {
        sonoff_bridge_expected_bytes = 2;
        if (serial_in_byte >= 0xA3) {
          sonoff_bridge_expected_bytes = 11;
        }
        if (serial_in_byte == 0xA6) {
          sonoff_bridge_expected_bytes = 0;
          serial_in_buffer[serial_in_byte_counter++] = 0xAA;
          sonoff_bridge_receive_raw_flag = 1;
        }
      }
      serial_in_buffer[serial_in_byte_counter++] = serial_in_byte;
      if ((sonoff_bridge_expected_bytes == serial_in_byte_counter) && (0x55 == serial_in_byte)) {
        SonoffBridgeReceived();
        sonoff_bridge_receive_flag = 0;
        return 1;
      }
    }
    serial_in_byte = 0;
  }
  if (0xAA == serial_in_byte) {
    serial_in_byte_counter = 0;
    serial_in_byte = 0;
    sonoff_bridge_receive_flag = 1;
    receive_len = 0;
  }
  return 0;
}

void SonoffBridgeSendCommand(byte code)
{
  Serial.write(0xAA);
  Serial.write(code);
  Serial.write(0x55);
}

void SonoffBridgeSendAck()
{
  Serial.write(0xAA);
  Serial.write(0xA0);
  Serial.write(0x55);
}

void SonoffBridgeSendCode(uint32_t code)
{
  Serial.write(0xAA);
  Serial.write(0xA5);
  for (byte i = 0; i < 6; i++) {
    Serial.write(Settings.rf_code[0][i]);
  }
  Serial.write((code >> 16) & 0xff);
  Serial.write((code >> 8) & 0xff);
  Serial.write(code & 0xff);
  Serial.write(0x55);
  Serial.flush();
}

void SonoffBridgeSend(uint8_t idx, uint8_t key)
{
  uint8_t code;

  key--;
  Serial.write(0xAA);
  Serial.write(0xA5);
  for (byte i = 0; i < 8; i++) {
    Serial.write(Settings.rf_code[idx][i]);
  }
  if (0 == idx) {
    code = (0x10 << (key >> 2)) | (1 << (key & 3));
  } else {
    code = Settings.rf_code[idx][8];
  }
  Serial.write(code);
  Serial.write(0x55);
  Serial.flush();
#ifdef USE_DOMOTICZ


#endif
}

void SonoffBridgeLearn(uint8_t key)
{
  sonoff_bridge_learn_key = key;
  sonoff_bridge_learn_active = 1;
  sonoff_bridge_last_learn_time = millis();
  Serial.write(0xAA);
  Serial.write(0xA1);
  Serial.write(0x55);
}





boolean SonoffBridgeCommand()
{
  char command [CMDSZ];
  boolean serviced = true;

  int command_code = GetCommandCode(command, sizeof(command), XdrvMailbox.topic, kSonoffBridgeCommands);
  if (-1 == command_code) {
    serviced = false;
  }
  else if ((command_code >= CMND_RFSYNC) && (command_code <= CMND_RFCODE)) {
    char *p;
    char stemp [10];
    uint32_t code = 0;
    uint8_t radix = 10;

    uint8_t set_index = command_code *2;

    if (XdrvMailbox.data[0] == '#') {
      XdrvMailbox.data++;
      XdrvMailbox.data_len--;
      radix = 16;
    }

    if (XdrvMailbox.data_len) {
      code = strtol(XdrvMailbox.data, &p, radix);
      if (code) {
        if (CMND_RFCODE == command_code) {
          sonoff_bridge_last_send_code = code;
          SonoffBridgeSendCode(code);
        } else {
          if (1 == XdrvMailbox.payload) {
            code = pgm_read_byte(kDefaultRfCode + set_index) << 8 | pgm_read_byte(kDefaultRfCode + set_index +1);
          }
          uint8_t msb = code >> 8;
          uint8_t lsb = code & 0xFF;
          if ((code > 0) && (code < 0x7FFF) && (msb != 0x55) && (lsb != 0x55)) {
            Settings.rf_code[0][set_index] = msb;
            Settings.rf_code[0][set_index +1] = lsb;
          }
        }
      }
    }
    if (CMND_RFCODE == command_code) {
      code = sonoff_bridge_last_send_code;
    } else {
      code = Settings.rf_code[0][set_index] << 8 | Settings.rf_code[0][set_index +1];
    }
    if (10 == radix) {
      snprintf_P(stemp, sizeof(stemp), PSTR("%d"), code);
    } else {
      snprintf_P(stemp, sizeof(stemp), PSTR("\"#%X\""), code);
    }
    snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_XVALUE, command, stemp);
  }
  else if ((CMND_RFKEY == command_code) && (XdrvMailbox.index > 0) && (XdrvMailbox.index <= 16)) {
    unsigned long now = millis();
    if ((!sonoff_bridge_learn_active) || (now - sonoff_bridge_last_learn_time > 60100)) {
      sonoff_bridge_learn_active = 0;
      if (2 == XdrvMailbox.payload) {
        SonoffBridgeLearn(XdrvMailbox.index);
        snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_INDEX_SVALUE, command, XdrvMailbox.index, D_JSON_START_LEARNING);
      }
      else if (3 == XdrvMailbox.payload) {
        Settings.rf_code[XdrvMailbox.index][0] = 0;
        snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_INDEX_SVALUE, command, XdrvMailbox.index, D_JSON_SET_TO_DEFAULT);
      }
      else if (4 == XdrvMailbox.payload) {
        for (byte i = 0; i < 6; i++) {
          Settings.rf_code[XdrvMailbox.index][i] = Settings.rf_code[0][i];
        }
        Settings.rf_code[XdrvMailbox.index][6] = (sonoff_bridge_last_send_code >> 16) & 0xff;
        Settings.rf_code[XdrvMailbox.index][7] = (sonoff_bridge_last_send_code >> 8) & 0xff;
        Settings.rf_code[XdrvMailbox.index][8] = sonoff_bridge_last_send_code & 0xff;
        snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_INDEX_SVALUE, command, XdrvMailbox.index, D_JSON_SAVED);
      } else if (5 == XdrvMailbox.payload) {
        uint8_t key = XdrvMailbox.index;
        uint8_t index = (0 == Settings.rf_code[key][0]) ? 0 : key;
        uint16_t sync_time = (Settings.rf_code[index][0] << 8) | Settings.rf_code[index][1];
        uint16_t low_time = (Settings.rf_code[index][2] << 8) | Settings.rf_code[index][3];
        uint16_t high_time = (Settings.rf_code[index][4] << 8) | Settings.rf_code[index][5];
        uint32_t code = (Settings.rf_code[index][6] << 16) | (Settings.rf_code[index][7] << 8);
        if (0 == index) {
          key--;
          code |= (uint8_t)((0x10 << (key >> 2)) | (1 << (key & 3)));
        } else {
          code |= Settings.rf_code[index][8];
        }
        snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"%s%d\":{\"" D_JSON_SYNC "\":%d,\"" D_JSON_LOW "\":%d,\"" D_JSON_HIGH "\":%d,\"" D_JSON_DATA "\":\"%06X\"}}"),
                   command, XdrvMailbox.index, sync_time, low_time, high_time, code);
      } else {
        if ((1 == XdrvMailbox.payload) || (0 == Settings.rf_code[XdrvMailbox.index][0])) {
          SonoffBridgeSend(0, XdrvMailbox.index);
          snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_INDEX_SVALUE, command, XdrvMailbox.index, D_JSON_DEFAULT_SENT);
        } else {
          SonoffBridgeSend(XdrvMailbox.index, 0);
          snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_INDEX_SVALUE, command, XdrvMailbox.index, D_JSON_LEARNED_SENT);
        }
      }
    } else {
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_INDEX_SVALUE, command, sonoff_bridge_learn_key, D_JSON_LEARNING_ACTIVE);
    }
  }
  else if (CMND_RFRAW == command_code) {
    if (XdrvMailbox.data_len) {
      if (XdrvMailbox.data_len < 6) {
        switch (XdrvMailbox.payload) {
        case 0:
          SonoffBridgeSendCommand(0xA7);
        case 1:
          sonoff_bridge_receive_raw_flag = XdrvMailbox.payload;
          break;
        case 166:
        case 167:
        case 169:
        case 176:
        case 177:
        case 255:
          SonoffBridgeSendCommand(XdrvMailbox.payload);
          sonoff_bridge_receive_raw_flag = 1;
          break;
        case 192:
          char beep[] = "AAC000C055\0";
          SerialSendRaw(beep);
          break;
        }
      } else {
        SerialSendRaw(RemoveSpace(XdrvMailbox.data));
        sonoff_bridge_receive_raw_flag = 1;
      }
    }
    snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_SVALUE, command, GetStateText(sonoff_bridge_receive_raw_flag));
  } else serviced = false;

  return serviced;
}



void SonoffBridgeInit()
{
  sonoff_bridge_receive_raw_flag = 0;
  SonoffBridgeSendCommand(0xA7);
}





#define XDRV_06 

boolean Xdrv06(byte function)
{
  boolean result = false;

  if (SONOFF_BRIDGE == Settings.module) {
    switch (function) {
      case FUNC_INIT:
        SonoffBridgeInit();
        break;
      case FUNC_COMMAND:
        result = SonoffBridgeCommand();
        break;
      case FUNC_SERIAL:
        result = SonoffBridgeSerialInput();
        break;
    }
  }
  return result;
}
# 1 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xdrv_07_domoticz.ino"
# 20 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xdrv_07_domoticz.ino"
#ifdef USE_DOMOTICZ

const char DOMOTICZ_MESSAGE[] PROGMEM = "{\"idx\":%d,\"nvalue\":%d,\"svalue\":\"%s\",\"Battery\":%d,\"RSSI\":%d}";

enum DomoticzCommands { CMND_IDX, CMND_KEYIDX, CMND_SWITCHIDX, CMND_SENSORIDX, CMND_UPDATETIMER };
const char kDomoticzCommands[] PROGMEM = D_CMND_IDX "|" D_CMND_KEYIDX "|" D_CMND_SWITCHIDX "|" D_CMND_SENSORIDX "|" D_CMND_UPDATETIMER ;



#if MAX_DOMOTICZ_SNS_IDX < DZ_MAX_SENSORS
  #error "Domoticz: Too many sensors or change settings.h layout"
#endif

const char kDomoticzSensors[] PROGMEM =
  D_DOMOTICZ_TEMP "|" D_DOMOTICZ_TEMP_HUM "|" D_DOMOTICZ_TEMP_HUM_BARO "|" D_DOMOTICZ_POWER_ENERGY "|" D_DOMOTICZ_ILLUMINANCE "|" D_DOMOTICZ_COUNT "|" D_DOMOTICZ_VOLTAGE "|" D_DOMOTICZ_CURRENT "|" D_DOMOTICZ_AIRQUALITY ;

const char S_JSON_DOMOTICZ_COMMAND_INDEX_NVALUE[] PROGMEM = "{\"" D_CMND_DOMOTICZ "%s%d\":%d}";
const char S_JSON_DOMOTICZ_COMMAND_INDEX_LVALUE[] PROGMEM = "{\"" D_CMND_DOMOTICZ "%s%d\":%lu}";

char domoticz_in_topic[] = DOMOTICZ_IN_TOPIC;
char domoticz_out_topic[] = DOMOTICZ_OUT_TOPIC;

boolean domoticz_subscribe = false;
int domoticz_update_timer = 0;
byte domoticz_update_flag = 1;

int DomoticzBatteryQuality()
{




  int quality = 0;

  uint16_t voltage = ESP.getVcc();
  if (voltage <= 2600) {
    quality = 0;
  } else if (voltage >= 4600) {
    quality = 200;
  } else {
    quality = (voltage - 2600) / 10;
  }
  return quality;
}

int DomoticzRssiQuality()
{


  return WifiGetRssiAsQuality(WiFi.RSSI()) / 10;
}

void MqttPublishDomoticzPowerState(byte device)
{
  char sdimmer[8];

  if ((device < 1) || (device > devices_present)) {
    device = 1;
  }
  if (Settings.flag.mqtt_enabled && Settings.domoticz_relay_idx[device -1]) {
    snprintf_P(sdimmer, sizeof(sdimmer), PSTR("%d"), Settings.light_dimmer);
    snprintf_P(mqtt_data, sizeof(mqtt_data), DOMOTICZ_MESSAGE,
      Settings.domoticz_relay_idx[device -1], (power & (1 << (device -1))) ? 1 : 0, (light_type) ? sdimmer : "", DomoticzBatteryQuality(), DomoticzRssiQuality());
    MqttPublish(domoticz_in_topic);
  }
}

void DomoticzUpdatePowerState(byte device)
{
  if (domoticz_update_flag) {
    MqttPublishDomoticzPowerState(device);
  }
  domoticz_update_flag = 1;
}

void DomoticzMqttUpdate()
{
  if (domoticz_subscribe && (Settings.domoticz_update_timer || domoticz_update_timer)) {
    domoticz_update_timer--;
    if (domoticz_update_timer <= 0) {
      domoticz_update_timer = Settings.domoticz_update_timer;
      for (byte i = 1; i <= devices_present; i++) {
        MqttPublishDomoticzPowerState(i);
      }
    }
  }
}

void DomoticzMqttSubscribe()
{
  uint8_t maxdev = (devices_present > MAX_DOMOTICZ_IDX) ? MAX_DOMOTICZ_IDX : devices_present;
  for (byte i = 0; i < maxdev; i++) {
    if (Settings.domoticz_relay_idx[i]) {
      domoticz_subscribe = true;
    }
  }
  if (domoticz_subscribe) {
    char stopic[TOPSZ];
    snprintf_P(stopic, sizeof(stopic), PSTR("%s/#"), domoticz_out_topic);
    MqttSubscribe(stopic);
  }
}
# 149 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xdrv_07_domoticz.ino"
boolean DomoticzMqttData()
{
  char stemp1[10];
  unsigned long idx = 0;
  int16_t nvalue = -1;
  int16_t found = 0;

  domoticz_update_flag = 1;
  if (!strncmp(XdrvMailbox.topic, domoticz_out_topic, strlen(domoticz_out_topic))) {
    if (XdrvMailbox.data_len < 20) {
      return 1;
    }
    StaticJsonBuffer<400> jsonBuf;
    JsonObject& domoticz = jsonBuf.parseObject(XdrvMailbox.data);
    if (!domoticz.success()) {
      return 1;
    }



    idx = domoticz["idx"];
    if (domoticz.containsKey("nvalue")) {
      nvalue = domoticz["nvalue"];
    }

    snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_DOMOTICZ "idx %d, nvalue %d"), idx, nvalue);
    AddLog(LOG_LEVEL_DEBUG_MORE);

    if ((idx > 0) && (nvalue >= 0) && (nvalue <= 15)) {
      uint8_t maxdev = (devices_present > MAX_DOMOTICZ_IDX) ? MAX_DOMOTICZ_IDX : devices_present;
      for (byte i = 0; i < maxdev; i++) {
        if (idx == Settings.domoticz_relay_idx[i]) {
          bool iscolordimmer = strcmp_P(domoticz["dtype"],PSTR("Color Switch")) == 0;
          snprintf_P(stemp1, sizeof(stemp1), PSTR("%d"), i +1);
          if (iscolordimmer && 10 == nvalue) {
            JsonObject& color = domoticz["Color"];
            uint16_t level = nvalue = domoticz["svalue1"];
            uint16_t r = color["r"]; r = r * level / 100;
            uint16_t g = color["g"]; g = g * level / 100;
            uint16_t b = color["b"]; b = b * level / 100;
            uint16_t cw = color["cw"]; cw = cw * level / 100;
            uint16_t ww = color["ww"]; ww = ww * level / 100;
            snprintf_P(XdrvMailbox.topic, XdrvMailbox.index, PSTR("/" D_CMND_COLOR));
            snprintf_P(XdrvMailbox.data, XdrvMailbox.data_len, PSTR("%02x%02x%02x%02x%02x"), r, g, b, cw, ww);
            found = 1;
          } else if ((!iscolordimmer && 2 == nvalue) ||
                     (iscolordimmer && 15 == nvalue)) {
            if (domoticz.containsKey("svalue1")) {
              nvalue = domoticz["svalue1"];
            } else {
              return 1;
            }
            if (light_type && (Settings.light_dimmer == nvalue) && ((power >> i) &1)) {
              return 1;
            }
            snprintf_P(XdrvMailbox.topic, XdrvMailbox.index, PSTR("/" D_CMND_DIMMER));
            snprintf_P(XdrvMailbox.data, XdrvMailbox.data_len, PSTR("%d"), nvalue);
            found = 1;
          } else if (1 == nvalue || 0 == nvalue) {
            if (((power >> i) &1) == (power_t)nvalue) {
              return 1;
            }
            snprintf_P(XdrvMailbox.topic, XdrvMailbox.index, PSTR("/" D_CMND_POWER "%s"), (devices_present > 1) ? stemp1 : "");
            snprintf_P(XdrvMailbox.data, XdrvMailbox.data_len, PSTR("%d"), nvalue);
            found = 1;
          }
          break;
        }
      }
    }
    if (!found) {
      return 1;
    }

    snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_DOMOTICZ D_RECEIVED_TOPIC " %s, " D_DATA " %s"), XdrvMailbox.topic, XdrvMailbox.data);
    AddLog(LOG_LEVEL_DEBUG_MORE);

    domoticz_update_flag = 0;
  }
  return 0;
}





boolean DomoticzCommand()
{
  char command [CMDSZ];
  boolean serviced = true;
  uint8_t dmtcz_len = strlen(D_CMND_DOMOTICZ);

  if (!strncasecmp_P(XdrvMailbox.topic, PSTR(D_CMND_DOMOTICZ), dmtcz_len)) {
    int command_code = GetCommandCode(command, sizeof(command), XdrvMailbox.topic +dmtcz_len, kDomoticzCommands);
    if (-1 == command_code) {
      serviced = false;
    }
    else if ((CMND_IDX == command_code) && (XdrvMailbox.index > 0) && (XdrvMailbox.index <= MAX_DOMOTICZ_IDX)) {
      if (XdrvMailbox.payload >= 0) {
        Settings.domoticz_relay_idx[XdrvMailbox.index -1] = XdrvMailbox.payload;
        restart_flag = 2;
      }
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_DOMOTICZ_COMMAND_INDEX_LVALUE, command, XdrvMailbox.index, Settings.domoticz_relay_idx[XdrvMailbox.index -1]);
    }
    else if ((CMND_KEYIDX == command_code) && (XdrvMailbox.index > 0) && (XdrvMailbox.index <= MAX_DOMOTICZ_IDX)) {
      if (XdrvMailbox.payload >= 0) {
        Settings.domoticz_key_idx[XdrvMailbox.index -1] = XdrvMailbox.payload;
      }
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_DOMOTICZ_COMMAND_INDEX_LVALUE, command, XdrvMailbox.index, Settings.domoticz_key_idx[XdrvMailbox.index -1]);
    }
    else if ((CMND_SWITCHIDX == command_code) && (XdrvMailbox.index > 0) && (XdrvMailbox.index <= MAX_DOMOTICZ_IDX)) {
      if (XdrvMailbox.payload >= 0) {
        Settings.domoticz_switch_idx[XdrvMailbox.index -1] = XdrvMailbox.payload;
      }
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_DOMOTICZ_COMMAND_INDEX_NVALUE, command, XdrvMailbox.index, Settings.domoticz_switch_idx[XdrvMailbox.index -1]);
    }
    else if ((CMND_SENSORIDX == command_code) && (XdrvMailbox.index > 0) && (XdrvMailbox.index <= DZ_MAX_SENSORS)) {
      if (XdrvMailbox.payload >= 0) {
        Settings.domoticz_sensor_idx[XdrvMailbox.index -1] = XdrvMailbox.payload;
      }
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_DOMOTICZ_COMMAND_INDEX_NVALUE, command, XdrvMailbox.index, Settings.domoticz_sensor_idx[XdrvMailbox.index -1]);
    }
    else if (CMND_UPDATETIMER == command_code) {
      if ((XdrvMailbox.payload >= 0) && (XdrvMailbox.payload < 3601)) {
        Settings.domoticz_update_timer = XdrvMailbox.payload;
      }
      snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"" D_CMND_DOMOTICZ "%s\":%d}"), command, Settings.domoticz_update_timer);
    }
    else serviced = false;
  }
  else serviced = false;

  return serviced;
}

boolean DomoticzSendKey(byte key, byte device, byte state, byte svalflg)
{
  boolean result = 0;

  if (device <= MAX_DOMOTICZ_IDX) {
    if ((Settings.domoticz_key_idx[device -1] || Settings.domoticz_switch_idx[device -1]) && (svalflg)) {
      snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"command\":\"switchlight\",\"idx\":%d,\"switchcmd\":\"%s\"}"),
        (key) ? Settings.domoticz_switch_idx[device -1] : Settings.domoticz_key_idx[device -1], (state) ? (2 == state) ? "Toggle" : "On" : "Off");
      MqttPublish(domoticz_in_topic);
      result = 1;
    }
  }
  return result;
}
# 316 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xdrv_07_domoticz.ino"
uint8_t DomoticzHumidityState(char *hum)
{
  uint8_t h = atoi(hum);
  return (!h) ? 0 : (h < 40) ? 2 : (h > 70) ? 3 : 1;
}

void DomoticzSensor(byte idx, char *data)
{
  if (Settings.domoticz_sensor_idx[idx]) {
    char dmess[90];

    memcpy(dmess, mqtt_data, sizeof(dmess));
    if (DZ_AIRQUALITY == idx) {
      snprintf_P(mqtt_data, sizeof(dmess), PSTR("{\"idx\":%d,\"nvalue\":%s,\"Battery\":%d,\"RSSI\":%d}"),
        Settings.domoticz_sensor_idx[idx], data, DomoticzBatteryQuality(), DomoticzRssiQuality());
    } else {
      snprintf_P(mqtt_data, sizeof(dmess), DOMOTICZ_MESSAGE,
        Settings.domoticz_sensor_idx[idx], 0, data, DomoticzBatteryQuality(), DomoticzRssiQuality());
    }
    MqttPublish(domoticz_in_topic);
    memcpy(mqtt_data, dmess, sizeof(dmess));
  }
}

void DomoticzSensor(byte idx, uint32_t value)
{
  char data[16];
  snprintf_P(data, sizeof(data), PSTR("%d"), value);
  DomoticzSensor(idx, data);
}

void DomoticzTempHumSensor(char *temp, char *hum)
{
  char data[16];
  snprintf_P(data, sizeof(data), PSTR("%s;%s;%d"), temp, hum, DomoticzHumidityState(hum));
  DomoticzSensor(DZ_TEMP_HUM, data);
}

void DomoticzTempHumPressureSensor(char *temp, char *hum, char *baro)
{
  char data[32];
  snprintf_P(data, sizeof(data), PSTR("%s;%s;%d;%s;5"), temp, hum, DomoticzHumidityState(hum), baro);
  DomoticzSensor(DZ_TEMP_HUM_BARO, data);
}

void DomoticzSensorPowerEnergy(int power, char *energy)
{
  char data[16];
  snprintf_P(data, sizeof(data), PSTR("%d;%s"), power, energy);
  DomoticzSensor(DZ_POWER_ENERGY, data);
}





#ifdef USE_WEBSERVER

#define WEB_HANDLE_DOMOTICZ "dm"

const char S_CONFIGURE_DOMOTICZ[] PROGMEM = D_CONFIGURE_DOMOTICZ;

const char HTTP_BTN_MENU_DOMOTICZ[] PROGMEM =
  "<br/><form action='" WEB_HANDLE_DOMOTICZ "' method='get'><button>" D_CONFIGURE_DOMOTICZ "</button></form>";

const char HTTP_FORM_DOMOTICZ[] PROGMEM =
  "<fieldset><legend><b>&nbsp;" D_DOMOTICZ_PARAMETERS "&nbsp;</b></legend><form method='post' action='" WEB_HANDLE_DOMOTICZ "'>"
  "<br/><table>";
const char HTTP_FORM_DOMOTICZ_RELAY[] PROGMEM =
  "<tr><td style='width:260px'><b>" D_DOMOTICZ_IDX " {1</b></td><td style='width:70px'><input id='r{1' name='r{1' placeholder='0' value='{2'></td></tr>"
  "<tr><td style='width:260px'><b>" D_DOMOTICZ_KEY_IDX " {1</b></td><td style='width:70px'><input id='k{1' name='k{1' placeholder='0' value='{3'></td></tr>";
  const char HTTP_FORM_DOMOTICZ_SWITCH[] PROGMEM =
  "<tr><td style='width:260px'><b>" D_DOMOTICZ_SWITCH_IDX " {1</b></td><td style='width:70px'><input id='s{1' name='s{1' placeholder='0' value='{4'></td></tr>";
const char HTTP_FORM_DOMOTICZ_SENSOR[] PROGMEM =
  "<tr><td style='width:260px'><b>" D_DOMOTICZ_SENSOR_IDX " {1</b> {2</td><td style='width:70px'><input id='l{1' name='l{1' placeholder='0' value='{5'></td></tr>";
const char HTTP_FORM_DOMOTICZ_TIMER[] PROGMEM =
  "<tr><td style='width:260px'><b>" D_DOMOTICZ_UPDATE_TIMER "</b> (" STR(DOMOTICZ_UPDATE_TIMER) ")</td><td style='width:70px'><input id='ut' name='ut' placeholder='" STR(DOMOTICZ_UPDATE_TIMER) "' value='{6'</td></tr>";

void HandleDomoticzConfiguration()
{
  if (HttpUser()) { return; }
  if (!WebAuthenticate()) { return WebServer->requestAuthentication(); }
  AddLog_P(LOG_LEVEL_DEBUG, S_LOG_HTTP, S_CONFIGURE_DOMOTICZ);

  if (WebServer->hasArg("save")) {
    DomoticzSaveSettings();
    WebRestart(1);
    return;
  }

  char stemp[32];

  String page = FPSTR(HTTP_HEAD);
  page.replace(F("{v}"), FPSTR(S_CONFIGURE_DOMOTICZ));
  page += FPSTR(HTTP_HEAD_STYLE);
  page += FPSTR(HTTP_FORM_DOMOTICZ);
  for (int i = 0; i < MAX_DOMOTICZ_IDX; i++) {
    if (i < devices_present) {
      page += FPSTR(HTTP_FORM_DOMOTICZ_RELAY);
      page.replace("{2", String((int)Settings.domoticz_relay_idx[i]));
      page.replace("{3", String((int)Settings.domoticz_key_idx[i]));
    }
    if (pin[GPIO_SWT1 +i] < 99) {
      page += FPSTR(HTTP_FORM_DOMOTICZ_SWITCH);
      page.replace("{4", String((int)Settings.domoticz_switch_idx[i]));
    }
    page.replace("{1", String(i +1));
  }
  for (int i = 0; i < DZ_MAX_SENSORS; i++) {
    page += FPSTR(HTTP_FORM_DOMOTICZ_SENSOR);
    page.replace("{1", String(i +1));
    page.replace("{2", GetTextIndexed(stemp, sizeof(stemp), i, kDomoticzSensors));
    page.replace("{5", String((int)Settings.domoticz_sensor_idx[i]));
  }
  page += FPSTR(HTTP_FORM_DOMOTICZ_TIMER);
  page.replace("{6", String((int)Settings.domoticz_update_timer));
  page += F("</table>");
  page += FPSTR(HTTP_FORM_END);
  page += FPSTR(HTTP_BTN_CONF);
  ShowPage(page);
}

void DomoticzSaveSettings()
{
  char stemp[20];
  char ssensor_indices[6 * MAX_DOMOTICZ_SNS_IDX];
  char tmp[100];

  for (byte i = 0; i < MAX_DOMOTICZ_IDX; i++) {
    snprintf_P(stemp, sizeof(stemp), PSTR("r%d"), i +1);
    WebGetArg(stemp, tmp, sizeof(tmp));
    Settings.domoticz_relay_idx[i] = (!strlen(tmp)) ? 0 : atoi(tmp);
    snprintf_P(stemp, sizeof(stemp), PSTR("k%d"), i +1);
    WebGetArg(stemp, tmp, sizeof(tmp));
    Settings.domoticz_key_idx[i] = (!strlen(tmp)) ? 0 : atoi(tmp);
    snprintf_P(stemp, sizeof(stemp), PSTR("s%d"), i +1);
    WebGetArg(stemp, tmp, sizeof(tmp));
    Settings.domoticz_switch_idx[i] = (!strlen(tmp)) ? 0 : atoi(tmp);
  }
  ssensor_indices[0] = '\0';
  for (byte i = 0; i < DZ_MAX_SENSORS; i++) {
    snprintf_P(stemp, sizeof(stemp), PSTR("l%d"), i +1);
    WebGetArg(stemp, tmp, sizeof(tmp));
    Settings.domoticz_sensor_idx[i] = (!strlen(tmp)) ? 0 : atoi(tmp);
    snprintf_P(ssensor_indices, sizeof(ssensor_indices), PSTR("%s%s%d"), ssensor_indices, (strlen(ssensor_indices)) ? "," : "", Settings.domoticz_sensor_idx[i]);
  }
  WebGetArg("ut", tmp, sizeof(tmp));
  Settings.domoticz_update_timer = (!strlen(tmp)) ? DOMOTICZ_UPDATE_TIMER : atoi(tmp);

  snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_DOMOTICZ D_CMND_IDX " %d,%d,%d,%d, " D_CMND_KEYIDX " %d,%d,%d,%d, " D_CMND_SWITCHIDX " %d,%d,%d,%d, " D_CMND_SENSORIDX " %s, " D_CMND_UPDATETIMER " %d"),
    Settings.domoticz_relay_idx[0], Settings.domoticz_relay_idx[1], Settings.domoticz_relay_idx[2], Settings.domoticz_relay_idx[3],
    Settings.domoticz_key_idx[0], Settings.domoticz_key_idx[1], Settings.domoticz_key_idx[2], Settings.domoticz_key_idx[3],
    Settings.domoticz_switch_idx[0], Settings.domoticz_switch_idx[1], Settings.domoticz_switch_idx[2], Settings.domoticz_switch_idx[3],
    ssensor_indices, Settings.domoticz_update_timer);
  AddLog(LOG_LEVEL_INFO);
}
#endif





#define XDRV_07 

boolean Xdrv07(byte function)
{
  boolean result = false;

  if (Settings.flag.mqtt_enabled) {
    switch (function) {
#ifdef USE_WEBSERVER
      case FUNC_WEB_ADD_BUTTON:
        strncat_P(mqtt_data, HTTP_BTN_MENU_DOMOTICZ, sizeof(mqtt_data));
        break;
      case FUNC_WEB_ADD_HANDLER:
        WebServer->on("/" WEB_HANDLE_DOMOTICZ, HandleDomoticzConfiguration);
        break;
#endif
      case FUNC_COMMAND:
        result = DomoticzCommand();
        break;
      case FUNC_MQTT_SUBSCRIBE:
        DomoticzMqttSubscribe();
        break;
      case FUNC_MQTT_INIT:
        domoticz_update_timer = 2;
        break;
      case FUNC_MQTT_DATA:
        result = DomoticzMqttData();
        break;
      case FUNC_EVERY_SECOND:
        DomoticzMqttUpdate();
        break;
      case FUNC_SHOW_SENSOR:

        break;
    }
  }
  return result;
}

#endif
# 1 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xdrv_08_serial_bridge.ino"
# 20 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xdrv_08_serial_bridge.ino"
#ifdef USE_SERIAL_BRIDGE



#define SERIAL_BRIDGE_BUFFER_SIZE 130

#include <TasmotaSerial.h>

enum SerialBridgeCommands { CMND_SSERIALSEND, CMND_SBAUDRATE };
const char kSerialBridgeCommands[] PROGMEM = D_CMND_SSERIALSEND "|" D_CMND_SBAUDRATE;

TasmotaSerial *SerialBridgeSerial;

uint8_t serial_bridge_active = 1;
uint8_t serial_bridge_in_byte_counter = 0;
unsigned long serial_bridge_polling_window = 0;
char serial_bridge_buffer[SERIAL_BRIDGE_BUFFER_SIZE];

void SerialBridgeInput()
{
  while (SerialBridgeSerial->available()) {
    yield();
    uint8_t serial_in_byte = SerialBridgeSerial->read();

    if (serial_in_byte > 127) {
      serial_bridge_in_byte_counter = 0;
      SerialBridgeSerial->flush();
      return;
    }
    if (serial_in_byte) {
      if ((serial_in_byte_counter < sizeof(serial_bridge_buffer) -1) && (serial_in_byte != Settings.serial_delimiter)) {
        serial_bridge_buffer[serial_bridge_in_byte_counter++] = serial_in_byte;
        serial_bridge_polling_window = millis();
      } else {
        serial_bridge_polling_window = 0;
        break;
      }
    }
  }

  if (serial_bridge_in_byte_counter && (millis() > (serial_bridge_polling_window + SERIAL_POLLING))) {
    serial_bridge_buffer[serial_bridge_in_byte_counter] = 0;
    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"" D_JSON_SSERIALRECEIVED "\":\"%s\"}"), serial_bridge_buffer);
    MqttPublishPrefixTopic_P(RESULT_OR_TELE, PSTR(D_JSON_SSERIALRECEIVED));

    serial_bridge_in_byte_counter = 0;
  }
}



void SerialBridgeInit(void)
{
  serial_bridge_active = 0;
  if ((pin[GPIO_SBR_RX] < 99) && (pin[GPIO_SBR_TX] < 99)) {
    SerialBridgeSerial = new TasmotaSerial(pin[GPIO_SBR_RX], pin[GPIO_SBR_TX]);
    if (SerialBridgeSerial->begin(Settings.sbaudrate * 1200)) {
      serial_bridge_active = 1;
      SerialBridgeSerial->flush();
    }
  }
}





boolean SerialBridgeCommand()
{
  char command [CMDSZ];
  boolean serviced = true;

  int command_code = GetCommandCode(command, sizeof(command), XdrvMailbox.topic, kSerialBridgeCommands);
  if (-1 == command_code) {
    serviced = false;
  }
  else if ((CMND_SSERIALSEND == command_code) && (XdrvMailbox.index > 0) && (XdrvMailbox.index <= 3)) {
    if (XdrvMailbox.data_len > 0) {
      if (1 == XdrvMailbox.index) {
        SerialBridgeSerial->write(XdrvMailbox.data, XdrvMailbox.data_len);
        SerialBridgeSerial->write("\n");
      }
      else if (2 == XdrvMailbox.index) {
        SerialBridgeSerial->write(XdrvMailbox.data, XdrvMailbox.data_len);
      }
      else if (3 == XdrvMailbox.index) {
        SerialBridgeSerial->write(Unescape(XdrvMailbox.data, &XdrvMailbox.data_len), XdrvMailbox.data_len);
      }
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_SVALUE, command, D_JSON_DONE);
    }
  }
  else if (CMND_SBAUDRATE == command_code) {
    char *p;
    int baud = strtol(XdrvMailbox.data, &p, 10);
    if (baud > 0) {
      baud /= 1200;
      Settings.sbaudrate = (1 == XdrvMailbox.payload) ? SOFT_BAUDRATE / 1200 : baud;
      SerialBridgeSerial->begin(Settings.sbaudrate * 1200);
    }
    snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_LVALUE, command, Settings.sbaudrate * 1200);
  }
  else serviced = false;

  return serviced;
}





#define XDRV_08 

boolean Xdrv08(byte function)
{
  boolean result = false;

  if (serial_bridge_active) {
    switch (function) {
      case FUNC_PRE_INIT:
        SerialBridgeInit();
        break;
      case FUNC_LOOP:
        SerialBridgeInput();
        break;
      case FUNC_COMMAND:
        result = SerialBridgeCommand();
        break;
    }
  }
  return result;
}

#endif
# 1 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xdrv_09_timers.ino"
# 20 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xdrv_09_timers.ino"
#ifdef USE_TIMERS
# 38 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xdrv_09_timers.ino"
enum TimerCommands { CMND_TIMER, CMND_TIMERS
#ifdef USE_SUNRISE
, CMND_LATITUDE, CMND_LONGITUDE
#endif
 };
const char kTimerCommands[] PROGMEM = D_CMND_TIMER "|" D_CMND_TIMERS
#ifdef USE_SUNRISE
"|" D_CMND_LATITUDE "|" D_CMND_LONGITUDE
#endif
;

uint16_t timer_last_minute = 60;
int8_t timer_window[MAX_TIMERS] = { 0 };

#ifdef USE_SUNRISE
# 61 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xdrv_09_timers.ino"
const double pi2 = TWO_PI;
const double pi = PI;
const double RAD = DEG_TO_RAD;

double JulianischesDatum()
{

  int Gregor;
  int Jahr = RtcTime.year;
  int Monat = RtcTime.month;
  int Tag = RtcTime.day_of_month;

  if (Monat <= 2) {
    Monat += 12;
    Jahr -= 1;
  }
  Gregor = (Jahr / 400) - (Jahr / 100) + (Jahr / 4);
  return 2400000.5 + 365.0*Jahr - 679004.0 + Gregor + (int)(30.6001 * (Monat +1)) + Tag + 0.5;
}

double InPi(double x)
{
  int n = (int)(x / pi2);
  x = x - n*pi2;
  if (x < 0) x += pi2;
  return x;
}

double eps(double T)
{

  return RAD * (23.43929111 + (-46.8150*T - 0.00059*T*T + 0.001813*T*T*T)/3600.0);
}

double BerechneZeitgleichung(double *DK,double T)
{
  double RA_Mittel = 18.71506921 + 2400.0513369*T +(2.5862e-5 - 1.72e-9*T)*T*T;
  double M = InPi(pi2 * (0.993133 + 99.997361*T));
  double L = InPi(pi2 * (0.7859453 + M/pi2 + (6893.0*sin(M)+72.0*sin(2.0*M)+6191.2*T) / 1296.0e3));
  double e = eps(T);
  double RA = atan(tan(L)*cos(e));
  if (RA < 0.0) RA += pi;
  if (L > pi) RA += pi;
  RA = 24.0*RA/pi2;
  *DK = asin(sin(e)*sin(L));

  RA_Mittel = 24.0 * InPi(pi2*RA_Mittel/24.0)/pi2;
  double dRA = RA_Mittel - RA;
  if (dRA < -12.0) dRA += 24.0;
  if (dRA > 12.0) dRA -= 24.0;
  dRA = dRA * 1.0027379;
  return dRA;
}

void DuskTillDawn(uint8_t *hour_up,uint8_t *minute_up, uint8_t *hour_down, uint8_t *minute_down)
{
  double JD2000 = 2451545.0;
  double JD = JulianischesDatum();
  double T = (JD - JD2000) / 36525.0;
  double DK;







  double h = SUNRISE_DAWN_ANGLE *RAD;
  double B = (((double)Settings.latitude)/1000000) * RAD;
  double GeographischeLaenge = ((double)Settings.longitude)/1000000;



  double Zeitzone = ((double)time_timezone) / 10;
  double Zeitgleichung = BerechneZeitgleichung(&DK, T);
  double Minuten = Zeitgleichung * 60.0;
  double Zeitdifferenz = 12.0*acos((sin(h) - sin(B)*sin(DK)) / (cos(B)*cos(DK)))/pi;
  double AufgangOrtszeit = 12.0 - Zeitdifferenz - Zeitgleichung;
  double UntergangOrtszeit = 12.0 + Zeitdifferenz - Zeitgleichung;
  double AufgangWeltzeit = AufgangOrtszeit - GeographischeLaenge / 15.0;
  double UntergangWeltzeit = UntergangOrtszeit - GeographischeLaenge / 15.0;
  double Aufgang = AufgangWeltzeit + Zeitzone;
  if (Aufgang < 0.0) {
    Aufgang += 24.0;
  } else {
    if (Aufgang >= 24.0) Aufgang -= 24.0;
  }
  double Untergang = UntergangWeltzeit + Zeitzone;
  if (Untergang < 0.0) {
    Untergang += 24.0;
  } else {
    if (Untergang >= 24.0) Untergang -= 24.0;
  }
  int AufgangMinuten = (int)(60.0*(Aufgang - (int)Aufgang)+0.5);
  int AufgangStunden = (int)Aufgang;
  if (AufgangMinuten >= 60.0) {
    AufgangMinuten -= 60.0;
    AufgangStunden++;
  } else {
    if (AufgangMinuten < 0.0) {
      AufgangMinuten += 60.0;
      AufgangStunden--;
      if (AufgangStunden < 0.0) AufgangStunden += 24.0;
    }
  }
  int UntergangMinuten = (int)(60.0*(Untergang - (int)Untergang)+0.5);
  int UntergangStunden = (int)Untergang;
  if (UntergangMinuten >= 60.0) {
    UntergangMinuten -= 60.0;
    UntergangStunden++;
  } else {
    if (UntergangMinuten<0) {
      UntergangMinuten += 60.0;
      UntergangStunden--;
      if (UntergangStunden < 0.0) UntergangStunden += 24.0;
    }
  }
  *hour_up = AufgangStunden;
  *minute_up = AufgangMinuten;
  *hour_down = UntergangStunden;
  *minute_down = UntergangMinuten;
}

void ApplyTimerOffsets(Timer *duskdawn)
{
  uint8_t hour[2];
  uint8_t minute[2];
  Timer stored = (Timer)*duskdawn;


  DuskTillDawn(&hour[0], &minute[0], &hour[1], &minute[1]);
  uint8_t mode = (duskdawn->mode -1) &1;
  duskdawn->time = (hour[mode] *60) + minute[mode];


  uint16_t timeBuffer;
  if ((uint16_t)stored.time > 719) {

    timeBuffer = (uint16_t)stored.time - 720;

    if (timeBuffer > (uint16_t)duskdawn->time) {
      timeBuffer = 1440 - (timeBuffer - (uint16_t)duskdawn->time);
      duskdawn->days = duskdawn->days >> 1;
      duskdawn->days |= (stored.days << 6);
    } else {
      timeBuffer = (uint16_t)duskdawn->time - timeBuffer;
    }
  } else {

    timeBuffer = (uint16_t)duskdawn->time + (uint16_t)stored.time;

    if (timeBuffer > 1440) {
      timeBuffer -= 1440;
      duskdawn->days = duskdawn->days << 1;
      duskdawn->days |= (stored.days >> 6);
    }
  }
  duskdawn->time = timeBuffer;
}

String GetSun(byte dawn)
{
  char stime[6];

  uint8_t hour[2];
  uint8_t minute[2];

  DuskTillDawn(&hour[0], &minute[0], &hour[1], &minute[1]);
  dawn &= 1;
  snprintf_P(stime, sizeof(stime), PSTR("%02d:%02d"), hour[dawn], minute[dawn]);
  return String(stime);
}

uint16_t GetSunMinutes(byte dawn)
{
  uint8_t hour[2];
  uint8_t minute[2];

  DuskTillDawn(&hour[0], &minute[0], &hour[1], &minute[1]);
  dawn &= 1;
  return (hour[dawn] *60) + minute[dawn];
}

#endif



void TimerSetRandomWindow(byte index)
{
  timer_window[index] = 0;
  if (Settings.timer[index].window) {
    timer_window[index] = (random(0, (Settings.timer[index].window << 1) +1)) - Settings.timer[index].window;
  }
}

void TimerSetRandomWindows()
{
  for (byte i = 0; i < MAX_TIMERS; i++) { TimerSetRandomWindow(i); }
}

void TimerEverySecond()
{
  if (RtcTime.valid) {
    if (!RtcTime.hour && !RtcTime.minute && !RtcTime.second) { TimerSetRandomWindows(); }
    if (Settings.flag3.timers_enable && (uptime > 60) && (RtcTime.minute != timer_last_minute)) {
      timer_last_minute = RtcTime.minute;
      int16_t time = (RtcTime.hour *60) + RtcTime.minute;
      uint8_t days = 1 << (RtcTime.day_of_week -1);

      for (byte i = 0; i < MAX_TIMERS; i++) {

        Timer xtimer = Settings.timer[i];
        uint16_t set_time = xtimer.time;
#ifdef USE_SUNRISE
        if ((1 == xtimer.mode) || (2 == xtimer.mode)) {
          ApplyTimerOffsets(&xtimer);
          set_time = xtimer.time;
        }
#endif
        if (xtimer.arm) {
          set_time += timer_window[i];
          if (set_time < 0) { set_time = 0; }
          if (set_time > 1439) { set_time = 1439; }
          if (time == set_time) {
            if (xtimer.days & days) {
              Settings.timer[i].arm = xtimer.repeat;
#ifdef USE_RULES
              if (3 == xtimer.power) {
                snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"Clock\":{\"Timer\":%d}}"), i +1);
                XdrvRulesProcess();
              } else
#endif
                if (devices_present) { ExecuteCommandPower(xtimer.device +1, xtimer.power, SRC_TIMER); }
            }
          }
        }
      }
    }
  }
}

void PrepShowTimer(uint8_t index)
{
  char days[8] = { 0 };
  char sign[2] = { 0 };
  char soutput[80];

  Timer xtimer = Settings.timer[index -1];

  for (byte i = 0; i < 7; i++) {
    uint8_t mask = 1 << i;
    snprintf(days, sizeof(days), "%s%d", days, ((xtimer.days & mask) > 0));
  }

  soutput[0] = '\0';
  if (devices_present) {
    snprintf_P(soutput, sizeof(soutput), PSTR(",\"" D_JSON_TIMER_OUTPUT "\":%d"), xtimer.device +1);
  }
#ifdef USE_SUNRISE
  int16_t hour = xtimer.time / 60;
  if ((1 == xtimer.mode) || (2 == xtimer.mode)) {
    if (hour > 11) {
      hour -= 12;
      sign[0] = '-';
    }
  }
  snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s\"" D_CMND_TIMER "%d\":{\"" D_JSON_TIMER_ARM "\":%d,\"" D_JSON_TIMER_MODE "\":%d,\"" D_JSON_TIMER_TIME "\":\"%s%02d:%02d\",\"" D_JSON_TIMER_WINDOW "\":%d,\"" D_JSON_TIMER_DAYS "\":\"%s\",\"" D_JSON_TIMER_REPEAT "\":%d%s,\"" D_JSON_TIMER_ACTION "\":%d}"),
    mqtt_data, index, xtimer.arm, xtimer.mode, sign, hour, xtimer.time % 60, xtimer.window, days, xtimer.repeat, soutput, xtimer.power);
#else
  snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s\"" D_CMND_TIMER "%d\":{\"" D_JSON_TIMER_ARM "\":%d,\"" D_JSON_TIMER_TIME "\":\"%02d:%02d\",\"" D_JSON_TIMER_WINDOW "\":%d,\"" D_JSON_TIMER_DAYS "\":\"%s\",\"" D_JSON_TIMER_REPEAT "\":%d%s,\"" D_JSON_TIMER_ACTION "\":%d}"),
    mqtt_data, index, xtimer.arm, xtimer.time / 60, xtimer.time % 60, xtimer.window, days, xtimer.repeat, soutput, xtimer.power);
#endif
}





boolean TimerCommand()
{
  char command[CMDSZ];
  char dataBufUc[XdrvMailbox.data_len];
  boolean serviced = true;
  uint8_t index = XdrvMailbox.index;

  UpperCase(dataBufUc, XdrvMailbox.data);
  int command_code = GetCommandCode(command, sizeof(command), XdrvMailbox.topic, kTimerCommands);
  if (-1 == command_code) {
    serviced = false;
  }
  else if ((CMND_TIMER == command_code) && (index > 0) && (index <= MAX_TIMERS)) {
    uint8_t error = 0;
    if (XdrvMailbox.data_len) {
      if ((XdrvMailbox.payload >= 0) && (XdrvMailbox.payload <= MAX_TIMERS)) {
        if (XdrvMailbox.payload == 0) {
          Settings.timer[index -1].data = 0;
        } else {
          Settings.timer[index -1].data = Settings.timer[XdrvMailbox.payload -1].data;
        }
      } else {
#ifndef USE_RULES
        if (devices_present) {
#endif
          StaticJsonBuffer<256> jsonBuffer;
          JsonObject& root = jsonBuffer.parseObject(dataBufUc);
          if (!root.success()) {
            snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"" D_CMND_TIMER "%d\":\"" D_JSON_INVALID_JSON "\"}"), index);
            error = 1;
          }
          else {
            char parm_uc[10];
            index--;
            if (root[UpperCase_P(parm_uc, PSTR(D_JSON_TIMER_ARM))].success()) {
              Settings.timer[index].arm = (root[parm_uc] != 0);
            }
#ifdef USE_SUNRISE
            if (root[UpperCase_P(parm_uc, PSTR(D_JSON_TIMER_MODE))].success()) {
              Settings.timer[index].mode = (uint8_t)root[parm_uc] & 0x03;
            }
#endif
            if (root[UpperCase_P(parm_uc, PSTR(D_JSON_TIMER_TIME))].success()) {
              uint16_t itime = 0;
              int8_t value = 0;
              uint8_t sign = 0;
              char time_str[10];

              snprintf(time_str, sizeof(time_str), root[parm_uc]);
              const char *substr = strtok(time_str, ":");
              if (substr != NULL) {
                if (strchr(substr, '-')) {
                  sign = 1;
                  substr++;
                }
                value = atoi(substr);
                if (sign) { value += 12; }
                if (value > 23) { value = 23; }
                itime = value * 60;
                substr = strtok(NULL, ":");
                if (substr != NULL) {
                  value = atoi(substr);
                  if (value < 0) { value = 0; }
                  if (value > 59) { value = 59; }
                  itime += value;
                }
              }
              Settings.timer[index].time = itime;
            }
            if (root[UpperCase_P(parm_uc, PSTR(D_JSON_TIMER_WINDOW))].success()) {
              Settings.timer[index].window = (uint8_t)root[parm_uc] & 0x0F;
              TimerSetRandomWindow(index);
            }
            if (root[UpperCase_P(parm_uc, PSTR(D_JSON_TIMER_DAYS))].success()) {

              Settings.timer[index].days = 0;
              const char *tday = root[parm_uc];
              uint8_t i = 0;
              char ch = *tday++;
              while ((ch != '\0') && (i < 7)) {
                if (ch == '-') { ch = '0'; }
                uint8_t mask = 1 << i++;
                Settings.timer[index].days |= (ch == '0') ? 0 : mask;
                ch = *tday++;
              }
            }
            if (root[UpperCase_P(parm_uc, PSTR(D_JSON_TIMER_REPEAT))].success()) {
              Settings.timer[index].repeat = (root[parm_uc] != 0);
            }
            if (root[UpperCase_P(parm_uc, PSTR(D_JSON_TIMER_OUTPUT))].success()) {
              uint8_t device = ((uint8_t)root[parm_uc] -1) & 0x0F;
              Settings.timer[index].device = (device < devices_present) ? device : 0;
            }
            if (root[UpperCase_P(parm_uc, PSTR(D_JSON_TIMER_ACTION))].success()) {
              uint8_t action = (uint8_t)root[parm_uc] & 0x03;
              Settings.timer[index].power = (devices_present) ? action : 3;
            }

            index++;
          }
#ifndef USE_RULES
        } else {
          snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"" D_CMND_TIMER "%d\":\"" D_JSON_TIMER_NO_DEVICE "\"}"), index);
          error = 1;
        }
#endif
      }
    }
    if (!error) {
      snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{"));
      PrepShowTimer(index);
      snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s}"), mqtt_data);
    }
  }
  else if (CMND_TIMERS == command_code) {
    if (XdrvMailbox.data_len) {
      if ((XdrvMailbox.payload >= 0) && (XdrvMailbox.payload <= 1)) {
        Settings.flag3.timers_enable = XdrvMailbox.payload;
      }
      if (XdrvMailbox.payload == 2) {
        Settings.flag3.timers_enable = !Settings.flag3.timers_enable;
      }
    }

    snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_SVALUE, command, GetStateText(Settings.flag3.timers_enable));
    MqttPublishPrefixTopic_P(RESULT_OR_STAT, command);

    byte jsflg = 0;
    byte lines = 1;
    for (byte i = 0; i < MAX_TIMERS; i++) {
      if (!jsflg) {
        snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"" D_CMND_TIMERS "%d\":{"), lines++);
      } else {
        snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s,"), mqtt_data);
      }
      jsflg++;
      PrepShowTimer(i +1);
      if (jsflg > 3) {
        snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s}}"), mqtt_data);
        MqttPublishPrefixTopic_P(RESULT_OR_STAT, PSTR(D_CMND_TIMERS));
        jsflg = 0;
      }
    }
    mqtt_data[0] = '\0';
  }
#ifdef USE_SUNRISE
  else if (CMND_LONGITUDE == command_code) {
    if (XdrvMailbox.data_len) {
      Settings.longitude = (int)(CharToDouble(XdrvMailbox.data) *1000000);
    }
    char lbuff[32];
    dtostrfd(((double)Settings.longitude) /1000000, 6, lbuff);
    snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_SVALUE, command, lbuff);
  }
  else if (CMND_LATITUDE == command_code) {
    if (XdrvMailbox.data_len) {
      Settings.latitude = (int)(CharToDouble(XdrvMailbox.data) *1000000);
    }
    char lbuff[32];
    dtostrfd(((double)Settings.latitude) /1000000, 6, lbuff);
    snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_SVALUE, command, lbuff);
  }
#endif
  else serviced = false;

  return serviced;
}





#ifdef USE_WEBSERVER
#ifdef USE_TIMERS_WEB

#define WEB_HANDLE_TIMER "tm"

const char S_CONFIGURE_TIMER[] PROGMEM = D_CONFIGURE_TIMER;

const char HTTP_BTN_MENU_TIMER[] PROGMEM =
  "<br/><form action='" WEB_HANDLE_TIMER "' method='get'><button>" D_CONFIGURE_TIMER "</button></form>";

const char HTTP_TIMER_SCRIPT[] PROGMEM =
  "var pt=[],ct=99;"
  "function qs(s){"
    "return document.querySelector(s);"
  "}"
  "function ce(i,q){"
    "var o=document.createElement('option');"
    "o.textContent=i;"
    "q.appendChild(o);"
  "}"
#ifdef USE_SUNRISE
  "function gt(){"
    "var m,p,q;"
    "m=qs('input[name=\"rd\"]:checked').value;"
    "p=pt[ct]&0x7FF;"
    "if(m==0){"
      "so(0);"
      "q=Math.floor(p/60);if(q<10){q='0'+q;}qs('#ho').value=q;"
      "q=p%60;if(q<10){q='0'+q;}qs('#mi').value=q;"
    "}"
    "if((m==1)||(m==2)){"
      "so(1);"
      "q=Math.floor(p/60);"
      "if(q>=12){q-=12;qs('#dr').selectedIndex=1;}"
        "else{qs('#dr').selectedIndex=0;}"
      "if(q<10){q='0'+q;}qs('#ho').value=q;"
      "q=p%60;if(q<10){q='0'+q;}qs('#mi').value=q;"
    "}"
  "}"
  "function so(b){"
    "o=qs('#ho');"
    "e=o.childElementCount;"
    "if(b==1){"
      "qs('#dr').disabled='';"
      "if(e>12){for(i=12;i<=23;i++){o.removeChild(o.lastElementChild);}}"
    "}else{"
      "qs('#dr').disabled='disabled';"
      "if(e<23){for(i=12;i<=23;i++){ce(i,o);}}"
    "}"
  "}"
#endif
  "function st(){"
    "var i,l,m,n,p,s;"
    "m=0;s=0;"
    "n=1<<31;if(eb('a0').checked){s|=n;}"
    "n=1<<15;if(eb('r0').checked){s|=n;}"
    "for(i=0;i<7;i++){n=1<<(16+i);if(eb('w'+i).checked){s|=n;}}"
#ifdef USE_SUNRISE
    "m=qs('input[name=\"rd\"]:checked').value;"
    "s|=(qs('input[name=\"rd\"]:checked').value<<29);"
#endif
    "if(}1>0){"
      "i=qs('#d1').selectedIndex;if(i>=0){s|=(i<<23);}"
      "s|=(qs('#p1').selectedIndex<<27);"
    "}else{"
      "s|=3<<27;"
    "}"
    "l=((qs('#ho').selectedIndex*60)+qs('#mi').selectedIndex)&0x7FF;"
    "if(m==0){s|=l;}"
#ifdef USE_SUNRISE
    "if((m==1)||(m==2)){"
      "if(qs('#dr').selectedIndex>0){l+=720;}"
      "s|=l&0x7FF;"
    "}"
#endif
    "s|=((qs('#mw').selectedIndex)&0x0F)<<11;"
    "pt[ct]=s;"
    "eb('t0').value=pt.join();"
  "}"
  "function ot(t,e){"
    "var i,n,o,p,q,s;"
    "if(ct<99){st();}"
    "ct=t;"
    "o=document.getElementsByClassName('tl');"
    "for(i=0;i<o.length;i++){o[i].style.cssText=\"background-color:#ccc;color:#fff;font-weight:normal;\"}"
    "e.style.cssText=\"background-color:#fff;color:#000;font-weight:bold;\";"
    "s=pt[ct];"
#ifdef USE_SUNRISE
    "p=(s>>29)&3;eb('b'+p).checked=1;"
    "gt();"
#else
    "p=s&0x7FF;"
    "q=Math.floor(p/60);if(q<10){q='0'+q;}qs('#ho').value=q;"
    "q=p%60;if(q<10){q='0'+q;}qs('#mi').value=q;"
#endif
    "q=(s>>11)&0xF;if(q<10){q='0'+q;}qs('#mw').value=q;"
    "for(i=0;i<7;i++){p=(s>>(16+i))&1;eb('w'+i).checked=p;}"
    "if(}1>0){"
      "p=(s>>23)&0xF;qs('#d1').value=p+1;"
      "p=(s>>27)&3;qs('#p1').selectedIndex=p;"
    "}"
    "p=(s>>15)&1;eb('r0').checked=p;"
    "p=(s>>31)&1;eb('a0').checked=p;"
  "}"
  "function it(){"
    "var b,i,o,s;"
    "pt=eb('t0').value.split(',').map(Number);"
    "s='';for(i=0;i<" STR(MAX_TIMERS) ";i++){b='';if(0==i){b=\" id='dP'\";}s+=\"<button type='button' class='tl' onclick='ot(\"+i+\",this)'\"+b+\">\"+(i+1)+\"</button>\"}"
    "eb('bt').innerHTML=s;"
    "if(}1>0){"
      "eb('oa').innerHTML=\"<b>" D_TIMER_OUTPUT "</b>&nbsp;<span><select style='width:60px;' id='d1' name='d1'></select></span>&emsp;<b>" D_TIMER_ACTION "</b>&nbsp;<select style='width:99px;' id='p1' name='p1'></select>\";"
      "o=qs('#p1');ce('" D_OFF "',o);ce('" D_ON "',o);ce('" D_TOGGLE "',o);"
#ifdef USE_RULES
      "ce('" D_RULE "',o);"
#else
      "ce('" D_BLINK "',o);"
#endif
    "}else{"
      "eb('oa').innerHTML=\"<b>" D_TIMER_ACTION "</b> " D_RULE "\";"
    "}"
#ifdef USE_SUNRISE
    "o=qs('#dr');ce('+',o);ce('-',o);"
#endif
    "o=qs('#ho');for(i=0;i<=23;i++){ce((i<10)?('0'+i):i,o);}"
    "o=qs('#mi');for(i=0;i<=59;i++){ce((i<10)?('0'+i):i,o);}"
    "o=qs('#mw');for(i=0;i<=15;i++){ce((i<10)?('0'+i):i,o);}"
    "o=qs('#d1');for(i=0;i<}1;i++){ce(i+1,o);}"
    "var a='" D_DAY3LIST "';"
    "s='';for(i=0;i<7;i++){s+=\"<input style='width:5%;' id='w\"+i+\"' name='w\"+i+\"' type='checkbox'><b>\"+a.substring(i*3,(i*3)+3)+\"</b>\"}"
    "eb('ds').innerHTML=s;"
    "eb('dP').click();"
  "}";
const char HTTP_TIMER_STYLE[] PROGMEM =
  ".tl{float:left;border-radius:0;border:1px solid #fff;padding:1px;width:6.25%;}"
#ifdef USE_SUNRISE
  "input[type='radio']{width:13px;height:24px;margin-top:-1px;margin-right:8px;vertical-align:middle;}"
#endif
  "</style>";
const char HTTP_FORM_TIMER[] PROGMEM =
  "<fieldset style='min-width:470px;text-align:center;'><legend style='text-align:left;'><b>&nbsp;" D_TIMER_PARAMETERS "&nbsp;</b></legend><form method='post' action='" WEB_HANDLE_TIMER "'>"
  "<br/><input style='width:5%;' id='e0' name='e0' type='checkbox'{e0><b>" D_TIMER_ENABLE "</b><br/><br/><hr/>"
  "<input id='t0' name='t0' value='";
const char HTTP_FORM_TIMER1[] PROGMEM =
  "' hidden><div id='bt' name='bt'></div><br/><br/><br/>"
  "<div id='oa' name='oa'></div><br/>"
  "<div>"
  "<input style='width:5%;' id='a0' name='a0' type='checkbox'><b>" D_TIMER_ARM "</b>&emsp;"
  "<input style='width:5%;' id='r0' name='r0' type='checkbox'><b>" D_TIMER_REPEAT "</b>"
  "</div><br/>"
  "<div>"
#ifdef USE_SUNRISE
  "<fieldset style='width:299px;margin:auto;text-align:left;border:0;'>"
  "<input id='b0' name='rd' type='radio' value='0' onclick='gt();'><b>" D_TIMER_TIME "</b><br/>"
  "<input id='b1' name='rd' type='radio' value='1' onclick='gt();'><b>" D_SUNRISE "</b> (}8)<br/>"
  "<input id='b2' name='rd' type='radio' value='2' onclick='gt();'><b>" D_SUNSET "</b> (}9)<br/>"
  "</fieldset>"
  "<span><select style='width:46px;' id='dr' name='dr'></select></span>"
  "&nbsp;"
#else
  "<b>" D_TIMER_TIME "</b>&nbsp;"
#endif
  "<span><select style='width:60px;' id='ho' name='ho'></select></span>"
  "&nbsp;" D_HOUR_MINUTE_SEPARATOR "&nbsp;"
  "<span><select style='width:60px;' id='mi' name='mi'></select></span>"
  "&emsp;<b>+/-</b>&nbsp;"
  "<span><select style='width:60px;' id='mw' name='mw'></select></span>"
  "</div><br/>"
  "<div id='ds' name='ds'></div>";
const char HTTP_FORM_TIMER2[] PROGMEM =
  "type='submit' onclick='st();this.form.submit();'";

void HandleTimerConfiguration()
{
  if (HttpUser()) { return; }
  if (!WebAuthenticate()) { return WebServer->requestAuthentication(); }
  AddLog_P(LOG_LEVEL_DEBUG, S_LOG_HTTP, S_CONFIGURE_TIMER);

  if (WebServer->hasArg("save")) {
    TimerSaveSettings();
    HandleConfiguration();
    return;
  }

  String page = FPSTR(HTTP_HEAD);
  page.replace(F("{v}"), FPSTR(S_CONFIGURE_TIMER));
  page += FPSTR(HTTP_TIMER_SCRIPT);
  page += FPSTR(HTTP_HEAD_STYLE);
  page.replace(F("</style>"), FPSTR(HTTP_TIMER_STYLE));
  page += FPSTR(HTTP_FORM_TIMER);
  page.replace(F("{e0"), (Settings.flag3.timers_enable) ? F(" checked") : F(""));
  for (byte i = 0; i < MAX_TIMERS; i++) {
    if (i > 0) { page += F(","); }
    page += String(Settings.timer[i].data);
  }
  page += FPSTR(HTTP_FORM_TIMER1);
  page.replace(F("}1"), String(devices_present));
#ifdef USE_SUNRISE
  page.replace(F("}8"), GetSun(0));
  page.replace(F("}9"), GetSun(1));
  page.replace(F("299"), String(100 + (strlen(D_SUNSET) *12)));
#endif
  page += FPSTR(HTTP_FORM_END);
  page.replace(F("type='submit'"), FPSTR(HTTP_FORM_TIMER2));
  page += F("<script>it();</script>");
  page += FPSTR(HTTP_BTN_CONF);
  ShowPage(page);
}

void TimerSaveSettings()
{
  char tmp[MAX_TIMERS *12];
  Timer timer;

  Settings.flag3.timers_enable = WebServer->hasArg("e0");
  WebGetArg("t0", tmp, sizeof(tmp));
  char *p = tmp;
  snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_MQTT D_CMND_TIMERS " %d"), Settings.flag3.timers_enable);
  for (byte i = 0; i < MAX_TIMERS; i++) {
    timer.data = strtol(p, &p, 10);
    p++;
    if (timer.time < 1440) {
      bool flag = (timer.window != Settings.timer[i].window);
      Settings.timer[i].data = timer.data;
      if (flag) TimerSetRandomWindow(i);
    }
    snprintf_P(log_data, sizeof(log_data), PSTR("%s,0x%08X"), log_data, Settings.timer[i].data);
  }
  AddLog(LOG_LEVEL_DEBUG);
}
#endif
#endif





#define XDRV_09 

boolean Xdrv09(byte function)
{
  boolean result = false;

  switch (function) {
    case FUNC_PRE_INIT:
      TimerSetRandomWindows();
      break;
#ifdef USE_WEBSERVER
#ifdef USE_TIMERS_WEB
    case FUNC_WEB_ADD_BUTTON:
#ifdef USE_RULES
      strncat_P(mqtt_data, HTTP_BTN_MENU_TIMER, sizeof(mqtt_data));
#else
      if (devices_present) { strncat_P(mqtt_data, HTTP_BTN_MENU_TIMER, sizeof(mqtt_data)); }
#endif
      break;
    case FUNC_WEB_ADD_HANDLER:
      WebServer->on("/" WEB_HANDLE_TIMER, HandleTimerConfiguration);
      break;
#endif
#endif
    case FUNC_EVERY_SECOND:
      TimerEverySecond();
      break;
    case FUNC_COMMAND:
      result = TimerCommand();
      break;
  }
  return result;
}

#endif
# 1 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xdrv_10_rules.ino"
# 20 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xdrv_10_rules.ino"
#ifdef USE_RULES
# 66 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xdrv_10_rules.ino"
#define D_CMND_RULE "Rule"
#define D_CMND_RULETIMER "RuleTimer"
#define D_CMND_EVENT "Event"
#define D_CMND_VAR "Var"
#define D_CMND_MEM "Mem"
#define D_CMND_ADD "Add"
#define D_CMND_SUB "Sub"
#define D_CMND_MULT "Mult"
#define D_CMND_SCALE "Scale"

#define D_JSON_INITIATED "Initiated"

enum RulesCommands { CMND_RULE, CMND_RULETIMER, CMND_EVENT, CMND_VAR, CMND_MEM, CMND_ADD, CMND_SUB, CMND_MULT, CMND_SCALE };
const char kRulesCommands[] PROGMEM = D_CMND_RULE "|" D_CMND_RULETIMER "|" D_CMND_EVENT "|" D_CMND_VAR "|" D_CMND_MEM "|" D_CMND_ADD "|" D_CMND_SUB "|" D_CMND_MULT "|" D_CMND_SCALE ;

String rules_event_value;
unsigned long rules_timer[MAX_RULE_TIMERS] = { 0 };
uint8_t rules_quota = 0;
long rules_new_power = -1;
long rules_old_power = -1;
long rules_old_dimm = -1;

uint32_t rules_triggers[MAX_RULE_SETS] = { 0 };
uint16_t rules_last_minute = 60;
uint8_t rules_trigger_count[MAX_RULE_SETS] = { 0 };
uint8_t rules_teleperiod = 0;

char event_data[100];
char vars[MAX_RULE_VARS][10] = { 0 };



bool RulesRuleMatch(byte rule_set, String &event, String &rule)
{




  bool match = false;
  char stemp[10];


  int pos = rule.indexOf('#');
  if (pos == -1) { return false; }

  String rule_task = rule.substring(0, pos);
  if (rules_teleperiod) {
    int ppos = rule_task.indexOf("TELE-");
    if (ppos == -1) { return false; }
    rule_task = rule.substring(5, pos);
  }

  String rule_name = rule.substring(pos +1);

  char compare = ' ';
  pos = rule_name.indexOf(">");
  if (pos > 0) {
    compare = '>';
  } else {
    pos = rule_name.indexOf("<");
    if (pos > 0) {
      compare = '<';
    } else {
      pos = rule_name.indexOf("=");
      if (pos > 0) {
        compare = '=';
      } else {
        pos = rule_name.indexOf("|");
        if (pos > 0) {
          compare = '%';
        }
      }
    }
  }

  char rule_svalue[CMDSZ] = { 0 };
  double rule_value = 0;
  if (pos > 0) {
    String rule_param = rule_name.substring(pos + 1);
    for (byte i = 0; i < MAX_RULE_VARS; i++) {
      snprintf_P(stemp, sizeof(stemp), PSTR("%%VAR%d%%"), i +1);
      if (rule_param.startsWith(stemp)) {
        rule_param = vars[i];
        break;
      }
    }
    for (byte i = 0; i < MAX_RULE_MEMS; i++) {
      snprintf_P(stemp, sizeof(stemp), PSTR("%%MEM%d%%"), i +1);
      if (rule_param.startsWith(stemp)) {
        rule_param = Settings.mems[i];
        break;
      }
    }
    snprintf_P(stemp, sizeof(stemp), PSTR("%%TIME%%"));
    if (rule_param.startsWith(stemp)) {
      rule_param = String(GetMinutesPastMidnight());
    }
    snprintf_P(stemp, sizeof(stemp), PSTR("%%UPTIME%%"));
    if (rule_param.startsWith(stemp)) {
      rule_param = String(GetMinutesUptime());
    }
#if defined(USE_TIMERS) && defined(USE_SUNRISE)
    snprintf_P(stemp, sizeof(stemp), PSTR("%%SUNRISE%%"));
    if (rule_param.startsWith(stemp)) {
      rule_param = String(GetSunMinutes(0));
    }
    snprintf_P(stemp, sizeof(stemp), PSTR("%%SUNSET%%"));
    if (rule_param.startsWith(stemp)) {
      rule_param = String(GetSunMinutes(1));
    }
#endif
    rule_param.toUpperCase();
    snprintf(rule_svalue, sizeof(rule_svalue), rule_param.c_str());

    int temp_value = GetStateNumber(rule_svalue);
    if (temp_value > -1) {
      rule_value = temp_value;
    } else {
      rule_value = CharToDouble((char*)rule_svalue);
    }
    rule_name = rule_name.substring(0, pos);
  }


  StaticJsonBuffer<1024> jsonBuf;
  JsonObject &root = jsonBuf.parseObject(event);
  if (!root.success()) { return false; }

  double value = 0;
  const char* str_value = root[rule_task][rule_name];





  if (!root[rule_task][rule_name].success()) { return false; }


  rules_event_value = str_value;


  if (str_value) {
    value = CharToDouble((char*)str_value);
    int int_value = int(value);
    int int_rule_value = int(rule_value);
    switch (compare) {
      case '%':
        if ((int_value > 0) && (int_rule_value > 0)) {
          if ((int_value % int_rule_value) == 0) { match = true; }
        }
        break;
      case '>':
        if (value > rule_value) { match = true; }
        break;
      case '<':
        if (value < rule_value) { match = true; }
        break;
      case '=':

        if (!strcasecmp(str_value, rule_svalue)) { match = true; }
        break;
      case ' ':
        match = true;
        break;
    }
  } else match = true;

  if (bitRead(Settings.rule_once, rule_set)) {
    if (match) {
      if (!bitRead(rules_triggers[rule_set], rules_trigger_count[rule_set])) {
        bitSet(rules_triggers[rule_set], rules_trigger_count[rule_set]);
      } else {
        match = false;
      }
    } else {
      bitClear(rules_triggers[rule_set], rules_trigger_count[rule_set]);
    }
  }

  return match;
}



bool RuleSetProcess(byte rule_set, String &event_saved)
{
  bool serviced = false;
  char stemp[10];

  delay(0);




  String rules = Settings.rules[rule_set];

  rules_trigger_count[rule_set] = 0;
  int plen = 0;
  while (true) {
    rules = rules.substring(plen);
    rules.trim();
    if (!rules.length()) { return serviced; }

    String rule = rules;
    rule.toUpperCase();
    if (!rule.startsWith("ON ")) { return serviced; }

    int pevt = rule.indexOf(" DO ");
    if (pevt == -1) { return serviced; }
    String event_trigger = rule.substring(3, pevt);

    plen = rule.indexOf(" ENDON");
    if (plen == -1) { return serviced; }
    String commands = rules.substring(pevt +4, plen);
    plen += 6;
    rules_event_value = "";
    String event = event_saved;




    if (RulesRuleMatch(rule_set, event, event_trigger)) {
      commands.trim();
      String ucommand = commands;
      ucommand.toUpperCase();

      if (ucommand.indexOf("EVENT ") != -1) { commands = "backlog " + commands; }
      commands.replace(F("%value%"), rules_event_value);
      for (byte i = 0; i < MAX_RULE_VARS; i++) {
        snprintf_P(stemp, sizeof(stemp), PSTR("%%var%d%%"), i +1);
        commands.replace(stemp, vars[i]);
      }
      for (byte i = 0; i < MAX_RULE_MEMS; i++) {
        snprintf_P(stemp, sizeof(stemp), PSTR("%%mem%d%%"), i +1);
        commands.replace(stemp, Settings.mems[i]);
      }
      commands.replace(F("%time%"), String(GetMinutesPastMidnight()));
      commands.replace(F("%uptime%"), String(GetMinutesUptime()));
#if defined(USE_TIMERS) && defined(USE_SUNRISE)
      commands.replace(F("%sunrise%"), String(GetSunMinutes(0)));
      commands.replace(F("%sunset%"), String(GetSunMinutes(1)));
#endif

      char command[commands.length() +1];
      snprintf(command, sizeof(command), commands.c_str());

      snprintf_P(log_data, sizeof(log_data), PSTR("RUL: %s performs \"%s\""), event_trigger.c_str(), command);
      AddLog(LOG_LEVEL_INFO);




      ExecuteCommand(command, SRC_RULE);
      serviced = true;
    }
    rules_trigger_count[rule_set]++;
  }
  return serviced;
}



bool RulesProcessEvent(char *json_event)
{
  bool serviced = false;

  ShowFreeMem(PSTR("RulesProcessEvent"));

  String event_saved = json_event;
  event_saved.toUpperCase();




  for (byte i = 0; i < MAX_RULE_SETS; i++) {
    if (strlen(Settings.rules[i]) && bitRead(Settings.rule_enabled, i)) {
      if (RuleSetProcess(i, event_saved)) { serviced = true; }
    }
  }
  return serviced;
}

bool RulesProcess()
{
  return RulesProcessEvent(mqtt_data);
}

void RulesInit()
{
  rules_flag.data = 0;
  for (byte i = 0; i < MAX_RULE_SETS; i++) {
    if (Settings.rules[i][0] == '\0') {
      bitWrite(Settings.rule_enabled, i, 0);
      bitWrite(Settings.rule_once, i, 0);
    }
  }
  rules_teleperiod = 0;
}

void RulesEvery50ms()
{
  if (Settings.rule_enabled) {
    char json_event[120];

    if (-1 == rules_new_power) { rules_new_power = power; }
    if (rules_new_power != rules_old_power) {
      if (rules_old_power != -1) {
        for (byte i = 0; i < devices_present; i++) {
          uint8_t new_state = (rules_new_power >> i) &1;
          if (new_state != ((rules_old_power >> i) &1)) {
            snprintf_P(json_event, sizeof(json_event), PSTR("{\"Power%d\":{\"State\":%d}}"), i +1, new_state);
            RulesProcessEvent(json_event);
          }
        }
      } else {

        for (byte i = 0; i < devices_present; i++) {
          uint8_t new_state = (rules_new_power >> i) &1;
          snprintf_P(json_event, sizeof(json_event), PSTR("{\"Power%d\":{\"Boot\":%d}}"), i +1, new_state);
          RulesProcessEvent(json_event);
        }

        for (byte i = 0; i < MAX_SWITCHES; i++) {
#ifdef USE_TM1638
          if ((pin[GPIO_SWT1 +i] < 99) || ((pin[GPIO_TM16CLK] < 99) && (pin[GPIO_TM16DIO] < 99) && (pin[GPIO_TM16STB] < 99))) {
#else
          if (pin[GPIO_SWT1 +i] < 99) {
#endif
            boolean swm = ((FOLLOW_INV == Settings.switchmode[i]) || (PUSHBUTTON_INV == Settings.switchmode[i]) || (PUSHBUTTONHOLD_INV == Settings.switchmode[i]));
            snprintf_P(json_event, sizeof(json_event), PSTR("{\"" D_JSON_SWITCH "%d\":{\"Boot\":%d}}"), i +1, (swm ^ lastwallswitch[i]));
            RulesProcessEvent(json_event);
          }
        }
      }
      rules_old_power = rules_new_power;
    }
    else if (rules_old_dimm != Settings.light_dimmer) {
      if (rules_old_dimm != -1) {
        snprintf_P(json_event, sizeof(json_event), PSTR("{\"Dimmer\":{\"State\":%d}}"), Settings.light_dimmer);
      } else {

        snprintf_P(json_event, sizeof(json_event), PSTR("{\"Dimmer\":{\"Boot\":%d}}"), Settings.light_dimmer);
      }
      RulesProcessEvent(json_event);
      rules_old_dimm = Settings.light_dimmer;
    }
    else if (event_data[0]) {
      char *event;
      char *parameter;
      event = strtok_r(event_data, "=", &parameter);
      if (event) {
        event = Trim(event);
        if (parameter) {
          parameter = Trim(parameter);
        } else {
          parameter = event + strlen(event);
        }
        snprintf_P(json_event, sizeof(json_event), PSTR("{\"Event\":{\"%s\":\"%s\"}}"), event, parameter);
        event_data[0] ='\0';
        RulesProcessEvent(json_event);
      } else {
        event_data[0] ='\0';
      }
    }
    else if (rules_flag.data) {
      uint16_t mask = 1;
      for (byte i = 0; i < MAX_RULES_FLAG; i++) {
        if (rules_flag.data & mask) {
          rules_flag.data ^= mask;
          json_event[0] = '\0';
          switch (i) {
            case 0: strncpy_P(json_event, PSTR("{\"System\":{\"Boot\":1}}"), sizeof(json_event)); break;
            case 1: snprintf_P(json_event, sizeof(json_event), PSTR("{\"Time\":{\"Initialized\":%d}}"), GetMinutesPastMidnight()); break;
            case 2: snprintf_P(json_event, sizeof(json_event), PSTR("{\"Time\":{\"Set\":%d}}"), GetMinutesPastMidnight()); break;
            case 3: strncpy_P(json_event, PSTR("{\"MQTT\":{\"Connected\":1}}"), sizeof(json_event)); break;
            case 4: strncpy_P(json_event, PSTR("{\"MQTT\":{\"Disconnected\":1}}"), sizeof(json_event)); break;
            case 5: strncpy_P(json_event, PSTR("{\"WIFI\":{\"Connected\":1}}"), sizeof(json_event)); break;
            case 6: strncpy_P(json_event, PSTR("{\"WIFI\":{\"Disconnected\":1}}"), sizeof(json_event)); break;
          }
          if (json_event[0]) {
            RulesProcessEvent(json_event);
            break;
          }
        }
        mask <<= 1;
      }
    }
  }
}

void RulesEvery100ms()
{
  if (Settings.rule_enabled && (uptime > 4)) {
    mqtt_data[0] = '\0';
    int tele_period_save = tele_period;
    tele_period = 2;
    XsnsNextCall(FUNC_JSON_APPEND);
    tele_period = tele_period_save;
    if (strlen(mqtt_data)) {
      mqtt_data[0] = '{';
      snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s}"), mqtt_data);
      RulesProcess();
    }
  }
}

void RulesEverySecond()
{
  if (Settings.rule_enabled) {
    char json_event[120];

    if (RtcTime.valid) {
      if ((uptime > 60) && (RtcTime.minute != rules_last_minute)) {
        rules_last_minute = RtcTime.minute;
        snprintf_P(json_event, sizeof(json_event), PSTR("{\"Time\":{\"Minute\":%d}}"), GetMinutesPastMidnight());
        RulesProcessEvent(json_event);
      }
    }
    for (byte i = 0; i < MAX_RULE_TIMERS; i++) {
      if (rules_timer[i] != 0L) {
        if (TimeReached(rules_timer[i])) {
          rules_timer[i] = 0L;
          snprintf_P(json_event, sizeof(json_event), PSTR("{\"Rules\":{\"Timer\":%d}}"), i +1);
          RulesProcessEvent(json_event);
        }
      }
    }
  }
}

void RulesSetPower()
{
  rules_new_power = XdrvMailbox.index;
}

void RulesTeleperiod()
{
  rules_teleperiod = 1;
  RulesProcess();
  rules_teleperiod = 0;
}

boolean RulesCommand()
{
  char command[CMDSZ];
  boolean serviced = true;
  uint8_t index = XdrvMailbox.index;

  int command_code = GetCommandCode(command, sizeof(command), XdrvMailbox.topic, kRulesCommands);
  if (-1 == command_code) {
    serviced = false;
  }
  else if ((CMND_RULE == command_code) && (index > 0) && (index <= MAX_RULE_SETS)) {
    if ((XdrvMailbox.data_len > 0) && (XdrvMailbox.data_len < sizeof(Settings.rules[index -1]))) {
      if ((XdrvMailbox.payload >= 0) && (XdrvMailbox.payload <= 10)) {
        switch (XdrvMailbox.payload) {
        case 0:
        case 1:
          bitWrite(Settings.rule_enabled, index -1, XdrvMailbox.payload);
          break;
        case 2:
          bitWrite(Settings.rule_enabled, index -1, bitRead(Settings.rule_enabled, index -1) ^1);
          break;
        case 4:
        case 5:
          bitWrite(Settings.rule_once, index -1, XdrvMailbox.payload &1);
          break;
        case 6:
          bitWrite(Settings.rule_once, index -1, bitRead(Settings.rule_once, index -1) ^1);
          break;
        case 8:
        case 9:
          bitWrite(Settings.rule_stop, index -1, XdrvMailbox.payload &1);
          break;
        case 10:
          bitWrite(Settings.rule_stop, index -1, bitRead(Settings.rule_stop, index -1) ^1);
          break;
        }
      } else {
        int offset = 0;
        if ('+' == XdrvMailbox.data[0]) {
          offset = strlen(Settings.rules[index -1]);
          if (XdrvMailbox.data_len < (sizeof(Settings.rules[index -1]) - offset -1)) {
            XdrvMailbox.data[0] = ' ';
          } else {
            offset = -1;
          }
        }
        if (offset != -1) {
          strlcpy(Settings.rules[index -1] + offset, ('"' == XdrvMailbox.data[0]) ? "" : XdrvMailbox.data, sizeof(Settings.rules[index -1]));
        }
      }
      rules_triggers[index -1] = 0;
    }
    snprintf_P (mqtt_data, sizeof(mqtt_data), PSTR("{\"%s%d\":\"%s\",\"Once\":\"%s\",\"StopOnError\":\"%s\",\"Free\":%d,\"Rules\":\"%s\"}"),
      command, index, GetStateText(bitRead(Settings.rule_enabled, index -1)), GetStateText(bitRead(Settings.rule_once, index -1)),
      GetStateText(bitRead(Settings.rule_stop, index -1)), sizeof(Settings.rules[index -1]) - strlen(Settings.rules[index -1]) -1, Settings.rules[index -1]);
  }
  else if ((CMND_RULETIMER == command_code) && (index > 0) && (index <= MAX_RULE_TIMERS)) {
    if (XdrvMailbox.data_len > 0) {
      rules_timer[index -1] = (XdrvMailbox.payload > 0) ? millis() + (1000 * XdrvMailbox.payload) : 0;
    }
    mqtt_data[0] = '\0';
    for (byte i = 0; i < MAX_RULE_TIMERS; i++) {
      snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s%c\"T%d\":%d"), mqtt_data, (i) ? ',' : '{', i +1, (rules_timer[i]) ? (rules_timer[i] - millis()) / 1000 : 0);
    }
    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s}"), mqtt_data);
  }
  else if (CMND_EVENT == command_code) {
    if (XdrvMailbox.data_len > 0) {
      strlcpy(event_data, XdrvMailbox.data, sizeof(event_data));
    }
    snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_SVALUE, command, D_JSON_DONE);
  }
  else if ((CMND_VAR == command_code) && (index > 0) && (index <= MAX_RULE_VARS)) {
    if (XdrvMailbox.data_len > 0) {
      strlcpy(vars[index -1], ('"' == XdrvMailbox.data[0]) ? "" : XdrvMailbox.data, sizeof(vars[index -1]));
    }
    snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_INDEX_SVALUE, command, index, vars[index -1]);
  }
  else if ((CMND_MEM == command_code) && (index > 0) && (index <= MAX_RULE_MEMS)) {
    if (XdrvMailbox.data_len > 0) {
      strlcpy(Settings.mems[index -1], ('"' == XdrvMailbox.data[0]) ? "" : XdrvMailbox.data, sizeof(Settings.mems[index -1]));
    }
    snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_INDEX_SVALUE, command, index, Settings.mems[index -1]);
  }
  else if ((CMND_ADD == command_code) && (index > 0) && (index <= MAX_RULE_VARS)) {
    if (XdrvMailbox.data_len > 0) {
      double tempvar = CharToDouble(vars[index -1]) + CharToDouble(XdrvMailbox.data);
      dtostrfd(tempvar, 2, vars[index -1]);
    }
    snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_INDEX_SVALUE, command, index, vars[index -1]);
  }
  else if ((CMND_SUB == command_code) && (index > 0) && (index <= MAX_RULE_VARS)) {
    if (XdrvMailbox.data_len > 0) {
      double tempvar = CharToDouble(vars[index -1]) - CharToDouble(XdrvMailbox.data);
      dtostrfd(tempvar, 2, vars[index -1]);
    }
    snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_INDEX_SVALUE, command, index, vars[index -1]);
  }
  else if ((CMND_MULT == command_code) && (index > 0) && (index <= MAX_RULE_VARS)) {
    if (XdrvMailbox.data_len > 0) {
      double tempvar = CharToDouble(vars[index -1]) * CharToDouble(XdrvMailbox.data);
      dtostrfd(tempvar, 2, vars[index -1]);
    }
    snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_INDEX_SVALUE, command, index, vars[index -1]);
  }
  else if ((CMND_SCALE == command_code) && (index > 0) && (index <= MAX_RULE_VARS)) {
    if (XdrvMailbox.data_len > 0) {
      if (strstr(XdrvMailbox.data, ",")) {
        char sub_string[XdrvMailbox.data_len +1];

        double valueIN = CharToDouble(subStr(sub_string, XdrvMailbox.data, ",", 1));
        double fromLow = CharToDouble(subStr(sub_string, XdrvMailbox.data, ",", 2));
        double fromHigh = CharToDouble(subStr(sub_string, XdrvMailbox.data, ",", 3));
        double toLow = CharToDouble(subStr(sub_string, XdrvMailbox.data, ",", 4));
        double toHigh = CharToDouble(subStr(sub_string, XdrvMailbox.data, ",", 5));
        double value = map_double(valueIN, fromLow, fromHigh, toLow, toHigh);
        dtostrfd(value, 2, vars[index -1]);
      }
    }
    snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_COMMAND_INDEX_SVALUE, command, index, vars[index -1]);
  }
  else serviced = false;

  return serviced;
}

double map_double(double x, double in_min, double in_max, double out_min, double out_max)
{
 return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}





#define XDRV_10 

boolean Xdrv10(byte function)
{
  boolean result = false;

  switch (function) {
    case FUNC_PRE_INIT:
      RulesInit();
      break;
    case FUNC_EVERY_50_MSECOND:
      RulesEvery50ms();
      break;
    case FUNC_EVERY_100_MSECOND:
      RulesEvery100ms();
      break;
    case FUNC_EVERY_SECOND:
      RulesEverySecond();
      break;
    case FUNC_SET_POWER:
      RulesSetPower();
      break;
    case FUNC_COMMAND:
      result = RulesCommand();
      break;
    case FUNC_RULES_PROCESS:
      result = RulesProcess();
      break;
  }
  return result;
}

#endif
# 1 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xdrv_11_knx.ino"
# 20 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xdrv_11_knx.ino"
#ifdef USE_KNX
# 51 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xdrv_11_knx.ino"
#include <esp-knx-ip.h>

address_t KNX_physs_addr;
address_t KNX_addr;

#define KNX_Empty 255

#define TOGGLE_INHIBIT_TIME 15

float last_temp;
float last_hum;
byte toggle_inhibit;

typedef struct __device_parameters
{
  byte type;




  bool show;

  bool last_state;

  callback_id_t CB_id;





} device_parameters_t;


device_parameters_t device_param[] = {
  { 1, false, false, KNX_Empty },
  { 2, false, false, KNX_Empty },
  { 3, false, false, KNX_Empty },
  { 4, false, false, KNX_Empty },
  { 5, false, false, KNX_Empty },
  { 6, false, false, KNX_Empty },
  { 7, false, false, KNX_Empty },
  { 8, false, false, KNX_Empty },
  { 9, false, false, KNX_Empty },
  { 10, false, false, KNX_Empty },
  { 11, false, false, KNX_Empty },
  { 12, false, false, KNX_Empty },
  { 13, false, false, KNX_Empty },
  { 14, false, false, KNX_Empty },
  { 15, false, false, KNX_Empty },
  { 16, false, false, KNX_Empty },
  { KNX_TEMPERATURE, false, false, KNX_Empty },
  { KNX_HUMIDITY , false, false, KNX_Empty },
  { KNX_ENERGY_VOLTAGE , false, false, KNX_Empty },
  { KNX_ENERGY_CURRENT , false, false, KNX_Empty },
  { KNX_ENERGY_POWER , false, false, KNX_Empty },
  { KNX_ENERGY_POWERFACTOR , false, false, KNX_Empty },
  { KNX_ENERGY_DAILY , false, false, KNX_Empty },
  { KNX_ENERGY_START , false, false, KNX_Empty },
  { KNX_ENERGY_TOTAL , false, false, KNX_Empty },
  { KNX_SLOT1 , false, false, KNX_Empty },
  { KNX_SLOT2 , false, false, KNX_Empty },
  { KNX_SLOT3 , false, false, KNX_Empty },
  { KNX_SLOT4 , false, false, KNX_Empty },
  { KNX_SLOT5 , false, false, KNX_Empty },
  { KNX_Empty, false, false, KNX_Empty}
};


const char * device_param_ga[] = {
  D_TIMER_OUTPUT " 1",
  D_TIMER_OUTPUT " 2",
  D_TIMER_OUTPUT " 3",
  D_TIMER_OUTPUT " 4",
  D_TIMER_OUTPUT " 5",
  D_TIMER_OUTPUT " 6",
  D_TIMER_OUTPUT " 7",
  D_TIMER_OUTPUT " 8",
  D_SENSOR_BUTTON " 1",
  D_SENSOR_BUTTON " 2",
  D_SENSOR_BUTTON " 3",
  D_SENSOR_BUTTON " 4",
  D_SENSOR_BUTTON " 5",
  D_SENSOR_BUTTON " 6",
  D_SENSOR_BUTTON " 7",
  D_SENSOR_BUTTON " 8",
  D_TEMPERATURE ,
  D_HUMIDITY ,
  D_VOLTAGE ,
  D_CURRENT ,
  D_POWERUSAGE ,
  D_POWER_FACTOR ,
  D_ENERGY_TODAY ,
  D_ENERGY_YESTERDAY ,
  D_ENERGY_TOTAL ,
  D_KNX_TX_SLOT " 1",
  D_KNX_TX_SLOT " 2",
  D_KNX_TX_SLOT " 3",
  D_KNX_TX_SLOT " 4",
  D_KNX_TX_SLOT " 5",
  nullptr
};


const char *device_param_cb[] = {
  D_TIMER_OUTPUT " 1",
  D_TIMER_OUTPUT " 2",
  D_TIMER_OUTPUT " 3",
  D_TIMER_OUTPUT " 4",
  D_TIMER_OUTPUT " 5",
  D_TIMER_OUTPUT " 6",
  D_TIMER_OUTPUT " 7",
  D_TIMER_OUTPUT " 8",
  D_TIMER_OUTPUT " 1 " D_BUTTON_TOGGLE,
  D_TIMER_OUTPUT " 2 " D_BUTTON_TOGGLE,
  D_TIMER_OUTPUT " 3 " D_BUTTON_TOGGLE,
  D_TIMER_OUTPUT " 4 " D_BUTTON_TOGGLE,
  D_TIMER_OUTPUT " 5 " D_BUTTON_TOGGLE,
  D_TIMER_OUTPUT " 6 " D_BUTTON_TOGGLE,
  D_TIMER_OUTPUT " 7 " D_BUTTON_TOGGLE,
  D_TIMER_OUTPUT " 8 " D_BUTTON_TOGGLE,
  D_REPLY " " D_TEMPERATURE,
  D_REPLY " " D_HUMIDITY,
  D_REPLY " " D_VOLTAGE ,
  D_REPLY " " D_CURRENT ,
  D_REPLY " " D_POWERUSAGE ,
  D_REPLY " " D_POWER_FACTOR ,
  D_REPLY " " D_ENERGY_TODAY ,
  D_REPLY " " D_ENERGY_YESTERDAY ,
  D_REPLY " " D_ENERGY_TOTAL ,
  D_KNX_RX_SLOT " 1",
  D_KNX_RX_SLOT " 2",
  D_KNX_RX_SLOT " 3",
  D_KNX_RX_SLOT " 4",
  D_KNX_RX_SLOT " 5",
  nullptr
};


#define D_CMND_KNXTXCMND "KnxTx_Cmnd"
#define D_CMND_KNXTXVAL "KnxTx_Val"
#define D_CMND_KNX_ENABLED "Knx_Enabled"
#define D_CMND_KNX_ENHANCED "Knx_Enhanced"
#define D_CMND_KNX_PA "Knx_PA"
#define D_CMND_KNX_GA "Knx_GA"
#define D_CMND_KNX_CB "Knx_CB"
enum KnxCommands { CMND_KNXTXCMND, CMND_KNXTXVAL, CMND_KNX_ENABLED, CMND_KNX_ENHANCED, CMND_KNX_PA,
                   CMND_KNX_GA, CMND_KNX_CB } ;
const char kKnxCommands[] PROGMEM = D_CMND_KNXTXCMND "|" D_CMND_KNXTXVAL "|" D_CMND_KNX_ENABLED "|"
                   D_CMND_KNX_ENHANCED "|" D_CMND_KNX_PA "|" D_CMND_KNX_GA "|" D_CMND_KNX_CB ;

byte KNX_GA_Search( byte param, byte start = 0 )
{
  for (byte i = start; i < Settings.knx_GA_registered; ++i)
  {
    if ( Settings.knx_GA_param[i] == param )
    {
      if ( Settings.knx_GA_addr[i] != 0 )
      {
         if ( i >= start ) { return i; }
      }
    }
  }
  return KNX_Empty;
}


byte KNX_CB_Search( byte param, byte start = 0 )
{
  for (byte i = start; i < Settings.knx_CB_registered; ++i)
  {
    if ( Settings.knx_CB_param[i] == param )
    {
      if ( Settings.knx_CB_addr[i] != 0 )
      {
         if ( i >= start ) { return i; }
      }
    }
  }
  return KNX_Empty;
}


void KNX_ADD_GA( byte GAop, byte GA_FNUM, byte GA_AREA, byte GA_FDEF )
{

  if ( Settings.knx_GA_registered >= MAX_KNX_GA ) { return; }
  if ( GA_FNUM == 0 && GA_AREA == 0 && GA_FDEF == 0 ) { return; }


  Settings.knx_GA_param[Settings.knx_GA_registered] = GAop;
  KNX_addr.ga.area = GA_FNUM;
  KNX_addr.ga.line = GA_AREA;
  KNX_addr.ga.member = GA_FDEF;
  Settings.knx_GA_addr[Settings.knx_GA_registered] = KNX_addr.value;

  Settings.knx_GA_registered++;

  snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_KNX D_ADD " GA #%d: %s " D_TO " %d/%d/%d"),
   Settings.knx_GA_registered,
   device_param_ga[GAop-1],
   GA_FNUM, GA_AREA, GA_FDEF );
  AddLog(LOG_LEVEL_DEBUG);
}


void KNX_DEL_GA( byte GAnum )
{

  byte dest_offset = 0;
  byte src_offset = 0;
  byte len = 0;


  Settings.knx_GA_param[GAnum-1] = 0;

  if (GAnum == 1)
  {

    src_offset = 1;



    len = (Settings.knx_GA_registered - 1);
  }
  else if (GAnum == Settings.knx_GA_registered)
  {

  }
  else
  {




    dest_offset = GAnum -1 ;
    src_offset = dest_offset + 1;
    len = (Settings.knx_GA_registered - GAnum);
  }

  if (len > 0)
  {
    memmove(Settings.knx_GA_param + dest_offset, Settings.knx_GA_param + src_offset, len * sizeof(byte));
    memmove(Settings.knx_GA_addr + dest_offset, Settings.knx_GA_addr + src_offset, len * sizeof(uint16_t));
  }

  Settings.knx_GA_registered--;

  snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_KNX D_DELETE " GA #%d"),
    GAnum );
  AddLog(LOG_LEVEL_DEBUG);
}


void KNX_ADD_CB( byte CBop, byte CB_FNUM, byte CB_AREA, byte CB_FDEF )
{

  if ( Settings.knx_CB_registered >= MAX_KNX_CB ) { return; }
  if ( CB_FNUM == 0 && CB_AREA == 0 && CB_FDEF == 0 ) { return; }


  if ( device_param[CBop-1].CB_id == KNX_Empty )
  {

    device_param[CBop-1].CB_id = knx.callback_register("", KNX_CB_Action, &device_param[CBop-1]);




  }

  Settings.knx_CB_param[Settings.knx_CB_registered] = CBop;
  KNX_addr.ga.area = CB_FNUM;
  KNX_addr.ga.line = CB_AREA;
  KNX_addr.ga.member = CB_FDEF;
  Settings.knx_CB_addr[Settings.knx_CB_registered] = KNX_addr.value;

  knx.callback_assign( device_param[CBop-1].CB_id, KNX_addr );

  Settings.knx_CB_registered++;

  snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_KNX D_ADD " CB #%d: %d/%d/%d " D_TO " %s"),
   Settings.knx_CB_registered,
   CB_FNUM, CB_AREA, CB_FDEF,
   device_param_cb[CBop-1] );
  AddLog(LOG_LEVEL_DEBUG);
}


void KNX_DEL_CB( byte CBnum )
{
  byte oldparam = Settings.knx_CB_param[CBnum-1];
  byte dest_offset = 0;
  byte src_offset = 0;
  byte len = 0;


  knx.callback_unassign(CBnum-1);
  Settings.knx_CB_param[CBnum-1] = 0;

  if (CBnum == 1)
  {

    src_offset = 1;



    len = (Settings.knx_CB_registered - 1);
  }
  else if (CBnum == Settings.knx_CB_registered)
  {

  }
  else
  {




    dest_offset = CBnum -1 ;
    src_offset = dest_offset + 1;
    len = (Settings.knx_CB_registered - CBnum);
  }

  if (len > 0)
  {
    memmove(Settings.knx_CB_param + dest_offset, Settings.knx_CB_param + src_offset, len * sizeof(byte));
    memmove(Settings.knx_CB_addr + dest_offset, Settings.knx_CB_addr + src_offset, len * sizeof(uint16_t));
  }

  Settings.knx_CB_registered--;


  if ( KNX_CB_Search( oldparam ) == KNX_Empty ) {
    knx.callback_deregister( device_param[oldparam-1].CB_id );
    device_param[oldparam-1].CB_id = KNX_Empty;
  }

  snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_KNX D_DELETE " CB #%d"), CBnum );
  AddLog(LOG_LEVEL_DEBUG);
}


bool KNX_CONFIG_NOT_MATCH()
{

  for (byte i = 0; i < KNX_MAX_device_param; ++i)
  {
    if ( !device_param[i].show ) {



      if ( KNX_GA_Search(i+1) != KNX_Empty ) { return true; }

      if ( i < 8 )
      {
        if ( KNX_CB_Search(i+1) != KNX_Empty ) { return true; }
        if ( KNX_CB_Search(i+9) != KNX_Empty ) { return true; }
      }

      if ( i > 15 )
      {
        if ( KNX_CB_Search(i+1) != KNX_Empty ) { return true; }
      }
    }
  }


  for (byte i = 0; i < Settings.knx_GA_registered; ++i)
  {
    if ( Settings.knx_GA_param[i] != 0 )
    {
      if ( Settings.knx_GA_addr[i] == 0 )
      {
         return true;
      }
    }
  }
  for (byte i = 0; i < Settings.knx_CB_registered; ++i)
  {
    if ( Settings.knx_CB_param[i] != 0 )
    {
      if ( Settings.knx_CB_addr[i] == 0 )
      {
         return true;
      }
    }
  }

  return false;
}


void KNXStart()
{
  knx.start(nullptr);
  snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_KNX D_START));
  AddLog(LOG_LEVEL_DEBUG);
}


void KNX_INIT()
{

  if (Settings.knx_GA_registered > MAX_KNX_GA) { Settings.knx_GA_registered = MAX_KNX_GA; }
  if (Settings.knx_CB_registered > MAX_KNX_CB) { Settings.knx_CB_registered = MAX_KNX_CB; }


  KNX_physs_addr.value = Settings.knx_physsical_addr;
  knx.physical_address_set( KNX_physs_addr );
# 472 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xdrv_11_knx.ino"
  for (int i = 0; i < devices_present; ++i)
  {
    device_param[i].show = true;
  }
  for (int i = GPIO_SWT1; i < GPIO_SWT4 + 1; ++i)
  {
    if (GetUsedInModule(i, my_module.gp.io)) { device_param[i - GPIO_SWT1 + 8].show = true; }
  }
  for (int i = GPIO_KEY1; i < GPIO_KEY4 + 1; ++i)
  {
    if (GetUsedInModule(i, my_module.gp.io)) { device_param[i - GPIO_KEY1 + 8].show = true; }
  }
  if (GetUsedInModule(GPIO_DHT11, my_module.gp.io)) { device_param[KNX_TEMPERATURE-1].show = true; }
  if (GetUsedInModule(GPIO_DHT22, my_module.gp.io)) { device_param[KNX_TEMPERATURE-1].show = true; }
  if (GetUsedInModule(GPIO_SI7021, my_module.gp.io)) { device_param[KNX_TEMPERATURE-1].show = true; }
  if (GetUsedInModule(GPIO_DSB, my_module.gp.io)) { device_param[KNX_TEMPERATURE-1].show = true; }
  if (GetUsedInModule(GPIO_DHT11, my_module.gp.io)) { device_param[KNX_HUMIDITY-1].show = true; }
  if (GetUsedInModule(GPIO_DHT22, my_module.gp.io)) { device_param[KNX_HUMIDITY-1].show = true; }
  if (GetUsedInModule(GPIO_SI7021, my_module.gp.io)) { device_param[KNX_HUMIDITY-1].show = true; }


  if ( ( SONOFF_S31 == Settings.module ) || ( SONOFF_POW_R2 == Settings.module ) || ( energy_flg != ENERGY_NONE ) ) {
    device_param[KNX_ENERGY_POWER-1].show = true;
    device_param[KNX_ENERGY_DAILY-1].show = true;
    device_param[KNX_ENERGY_START-1].show = true;
    device_param[KNX_ENERGY_TOTAL-1].show = true;
    device_param[KNX_ENERGY_VOLTAGE-1].show = true;
    device_param[KNX_ENERGY_CURRENT-1].show = true;
    device_param[KNX_ENERGY_POWERFACTOR-1].show = true;
  }

#ifdef USE_RULES
  device_param[KNX_SLOT1-1].show = true;
  device_param[KNX_SLOT2-1].show = true;
  device_param[KNX_SLOT3-1].show = true;
  device_param[KNX_SLOT4-1].show = true;
  device_param[KNX_SLOT5-1].show = true;
#endif


  if (KNX_CONFIG_NOT_MATCH()) {
    Settings.knx_GA_registered = 0;
    Settings.knx_CB_registered = 0;
    snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_KNX D_DELETE " " D_KNX_PARAMETERS ));
    AddLog(LOG_LEVEL_DEBUG);
  }




  byte j;
  for (byte i = 0; i < Settings.knx_CB_registered; ++i)
  {
    j = Settings.knx_CB_param[i];
    if ( j > 0 )
    {
      device_param[j-1].CB_id = knx.callback_register("", KNX_CB_Action, &device_param[j-1]);



      KNX_addr.value = Settings.knx_CB_addr[i];
      knx.callback_assign( device_param[j-1].CB_id, KNX_addr );
    }
  }
}


void KNX_CB_Action(message_t const &msg, void *arg)
{
  device_parameters_t *chan = (device_parameters_t *)arg;
  if (!(Settings.flag.knx_enabled)) { return; }

  char tempchar[25];

  if (msg.data_len == 1) {

    snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_KNX D_RECEIVED_FROM " %d.%d.%d " D_COMMAND " %s: %d " D_TO " %s"),
     msg.received_on.ga.area, msg.received_on.ga.line, msg.received_on.ga.member,
     (msg.ct == KNX_CT_WRITE) ? D_KNX_COMMAND_WRITE : (msg.ct == KNX_CT_READ) ? D_KNX_COMMAND_READ : D_KNX_COMMAND_OTHER,
     msg.data[0],
     device_param_cb[(chan->type)-1]);
  } else {

    float tempvar = knx.data_to_2byte_float(msg.data);
    dtostrfd(tempvar,2,tempchar);
    snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_KNX D_RECEIVED_FROM " %d.%d.%d " D_COMMAND " %s: %s " D_TO " %s"),
     msg.received_on.ga.area, msg.received_on.ga.line, msg.received_on.ga.member,
     (msg.ct == KNX_CT_WRITE) ? D_KNX_COMMAND_WRITE : (msg.ct == KNX_CT_READ) ? D_KNX_COMMAND_READ : D_KNX_COMMAND_OTHER,
     tempchar,
     device_param_cb[(chan->type)-1]);
  }
  AddLog(LOG_LEVEL_INFO);

  switch (msg.ct)
  {
    case KNX_CT_WRITE:
      if (chan->type < 9)
      {
        ExecuteCommandPower(chan->type, msg.data[0], SRC_KNX);
      }
      else if (chan->type < 17)
      {
        if (!toggle_inhibit) {
          ExecuteCommandPower((chan->type) -8, 2, SRC_KNX);
          if (Settings.flag.knx_enable_enhancement) {
            toggle_inhibit = TOGGLE_INHIBIT_TIME;
          }
        }
      }
#ifdef USE_RULES
      else if ((chan->type >= KNX_SLOT1) && (chan->type <= KNX_SLOT5))
      {
        if (!toggle_inhibit) {
          char command[25];
          if (msg.data_len == 1) {

            snprintf_P(command, sizeof(command), PSTR("event KNXRX_CMND%d=%d"), ((chan->type) - KNX_SLOT1 + 1 ), msg.data[0]);
          } else {

            snprintf_P(command, sizeof(command), PSTR("event KNXRX_VAL%d=%s"), ((chan->type) - KNX_SLOT1 + 1 ), tempchar);
          }
          ExecuteCommand(command, SRC_KNX);
          if (Settings.flag.knx_enable_enhancement) {
            toggle_inhibit = TOGGLE_INHIBIT_TIME;
          }
        }
      }
#endif
      break;

    case KNX_CT_READ:
      if (chan->type < 9)
      {
        knx.answer_1bit(msg.received_on, chan->last_state);
        if (Settings.flag.knx_enable_enhancement) {
          knx.answer_1bit(msg.received_on, chan->last_state);
          knx.answer_1bit(msg.received_on, chan->last_state);
        }
      }
      else if (chan->type == KNX_TEMPERATURE)
      {
        knx.answer_2byte_float(msg.received_on, last_temp);
        if (Settings.flag.knx_enable_enhancement) {
          knx.answer_2byte_float(msg.received_on, last_temp);
          knx.answer_2byte_float(msg.received_on, last_temp);
        }
      }
      else if (chan->type == KNX_HUMIDITY)
      {
        knx.answer_2byte_float(msg.received_on, last_hum);
        if (Settings.flag.knx_enable_enhancement) {
          knx.answer_2byte_float(msg.received_on, last_hum);
          knx.answer_2byte_float(msg.received_on, last_hum);
        }
      }
#ifdef USE_RULES
      else if ((chan->type >= KNX_SLOT1) && (chan->type <= KNX_SLOT5))
      {
        if (!toggle_inhibit) {
          char command[25];
          snprintf_P(command, sizeof(command), PSTR("event KNXRX_REQ%d"), ((chan->type) - KNX_SLOT1 + 1 ) );
          ExecuteCommand(command, SRC_KNX);
          if (Settings.flag.knx_enable_enhancement) {
            toggle_inhibit = TOGGLE_INHIBIT_TIME;
          }
        }
      }
#endif
      break;
  }
}


void KnxUpdatePowerState(byte device, power_t state)
{
  if (!(Settings.flag.knx_enabled)) { return; }

  device_param[device -1].last_state = bitRead(state, device -1);


  byte i = KNX_GA_Search(device);
  while ( i != KNX_Empty ) {
    KNX_addr.value = Settings.knx_GA_addr[i];
    knx.write_1bit(KNX_addr, device_param[device -1].last_state);
    if (Settings.flag.knx_enable_enhancement) {
      knx.write_1bit(KNX_addr, device_param[device -1].last_state);
      knx.write_1bit(KNX_addr, device_param[device -1].last_state);
    }

    snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_KNX "%s = %d " D_SENT_TO " %d.%d.%d"),
     device_param_ga[device -1], device_param[device -1].last_state,
     KNX_addr.ga.area, KNX_addr.ga.line, KNX_addr.ga.member);
    AddLog(LOG_LEVEL_INFO);

    i = KNX_GA_Search(device, i + 1);
  }
}


void KnxSendButtonPower(byte key, byte device, byte state)
{







  if (!(Settings.flag.knx_enabled)) { return; }




  byte i = KNX_GA_Search(device + 8);
  while ( i != KNX_Empty ) {
    KNX_addr.value = Settings.knx_GA_addr[i];
    knx.write_1bit(KNX_addr, !(state == 0));
    if (Settings.flag.knx_enable_enhancement) {
      knx.write_1bit(KNX_addr, !(state == 0));
      knx.write_1bit(KNX_addr, !(state == 0));
    }

    snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_KNX "%s = %d " D_SENT_TO " %d.%d.%d"),
     device_param_ga[device + 7], !(state == 0),
     KNX_addr.ga.area, KNX_addr.ga.line, KNX_addr.ga.member);
    AddLog(LOG_LEVEL_INFO);

    i = KNX_GA_Search(device + 8, i + 1);
  }

}


void KnxSensor(byte sensor_type, float value)
{
  if (sensor_type == KNX_TEMPERATURE)
  {
    last_temp = value;
  } else if (sensor_type == KNX_HUMIDITY)
  {
    last_hum = value;
  }

  if (!(Settings.flag.knx_enabled)) { return; }

  byte i = KNX_GA_Search(sensor_type);
  while ( i != KNX_Empty ) {
    KNX_addr.value = Settings.knx_GA_addr[i];
    knx.write_2byte_float(KNX_addr, value);
    if (Settings.flag.knx_enable_enhancement) {
      knx.write_2byte_float(KNX_addr, value);
      knx.write_2byte_float(KNX_addr, value);
    }

    snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_KNX "%s " D_SENT_TO " %d.%d.%d "),
     device_param_ga[sensor_type -1],
     KNX_addr.ga.area, KNX_addr.ga.line, KNX_addr.ga.member);
    AddLog(LOG_LEVEL_INFO);

    i = KNX_GA_Search(sensor_type, i+1);
  }
}






#ifdef USE_WEBSERVER
#ifdef USE_KNX_WEB_MENU
const char S_CONFIGURE_KNX[] PROGMEM = D_CONFIGURE_KNX;

const char HTTP_BTN_MENU_KNX[] PROGMEM =
  "<br/><form action='kn' method='get'><button>" D_CONFIGURE_KNX "</button></form>";

const char HTTP_FORM_KNX[] PROGMEM =
  "<fieldset><legend style='text-align:left;'><b>&nbsp;" D_KNX_PARAMETERS "&nbsp;</b></legend><form method='post' action='kn'>"
  "<br/><center>"
  "<b>" D_KNX_PHYSICAL_ADDRESS " </b>"
  "<input style='width:12%;' type='number' name='area' min='0' max='15' value='{kna'> . "
  "<input style='width:12%;' type='number' name='line' min='0' max='15' value='{knl'> . "
  "<input style='width:12%;' type='number' name='member' min='0' max='255' value='{knm'>"
  "<br/><br/>" D_KNX_PHYSICAL_ADDRESS_NOTE "<br/><br/>"
  "<input style='width:10%;' id='b1' name='b1' type='checkbox'";

const char HTTP_FORM_KNX1[] PROGMEM =
  "><b>" D_KNX_ENABLE "   </b><input style='width:10%;' id='b2' name='b2' type='checkbox'";

const char HTTP_FORM_KNX2[] PROGMEM =
  "><b>" D_KNX_ENHANCEMENT "</b><br/></center><br/>"

  "<fieldset><center>"
  "<b>" D_KNX_GROUP_ADDRESS_TO_WRITE "</b><hr>"

  "<select name='GAop' style='width:25%;'>";

const char HTTP_FORM_KNX_OPT[] PROGMEM =
  "<option value='{vop}'>{nop}</option>";

const char HTTP_FORM_KNX_GA[] PROGMEM =
  "<input style='width:12%;' type='number' id='GAfnum' name='GAfnum' min='0' max='31' value='0'> / "
  "<input style='width:12%;' type='number' id='GAarea' name='GAarea' min='0' max='7' value='0'> / "
  "<input style='width:12%;' type='number' id='GAfdef' name='GAfdef' min='0' max='255' value='0'> ";

const char HTTP_FORM_KNX_ADD_BTN[] PROGMEM =
  "<button type='submit' onclick='fncbtnadd()' btndis name='btn_add' value='{btnval}' style='width:18%;'>" D_ADD "</button><br/><br/>"
  "<table style='width:80%; font-size: 14px;'><col width='250'><col width='30'>";

const char HTTP_FORM_KNX_ADD_TABLE_ROW[] PROGMEM =
  "<tr><td><b>{optex} -> GAfnum / GAarea / GAfdef </b></td>"
  "<td><button type='submit' name='btn_del_ga' value='{opval}' class='button bred'> " D_DELETE " </button></td></tr>";

const char HTTP_FORM_KNX3[] PROGMEM =
  "</table></center></fieldset><br/>"
  "<fieldset><form method='post' action='kn'><center>"
  "<b>" D_KNX_GROUP_ADDRESS_TO_READ "</b><hr>";

const char HTTP_FORM_KNX4[] PROGMEM =
  "-> <select name='CBop' style='width:25%;'>";

const char HTTP_FORM_KNX_ADD_TABLE_ROW2[] PROGMEM =
  "<tr><td><b>GAfnum / GAarea / GAfdef -> {optex}</b></td>"
  "<td><button type='submit' name='btn_del_cb' value='{opval}' class='button bred'> " D_DELETE " </button></td></tr>";

void HandleKNXConfiguration()
{
  if (HttpUser()) { return; }
  if (!WebAuthenticate()) { return WebServer->requestAuthentication(); }
  AddLog_P(LOG_LEVEL_DEBUG, S_LOG_HTTP, S_CONFIGURE_KNX);

  char tmp[100];
  String stmp;

  if ( WebServer->hasArg("save") ) {
    KNX_Save_Settings();
    HandleConfiguration();
  }
  else
  {
    if ( WebServer->hasArg("btn_add") ) {
      if ( WebServer->arg("btn_add") == "1" ) {

        stmp = WebServer->arg("GAop");
        byte GAop = stmp.toInt();
        stmp = WebServer->arg("GA_FNUM");
        byte GA_FNUM = stmp.toInt();
        stmp = WebServer->arg("GA_AREA");
        byte GA_AREA = stmp.toInt();
        stmp = WebServer->arg("GA_FDEF");
        byte GA_FDEF = stmp.toInt();

        if (GAop) {
          KNX_ADD_GA( GAop, GA_FNUM, GA_AREA, GA_FDEF );
        }
      }
      else
      {

        stmp = WebServer->arg("CBop");
        byte CBop = stmp.toInt();
        stmp = WebServer->arg("CB_FNUM");
        byte CB_FNUM = stmp.toInt();
        stmp = WebServer->arg("CB_AREA");
        byte CB_AREA = stmp.toInt();
        stmp = WebServer->arg("CB_FDEF");
        byte CB_FDEF = stmp.toInt();

        if (CBop) {
          KNX_ADD_CB( CBop, CB_FNUM, CB_AREA, CB_FDEF );
        }
      }
    }
    else if ( WebServer->hasArg("btn_del_ga") )
    {

      stmp = WebServer->arg("btn_del_ga");
      byte GA_NUM = stmp.toInt();

      KNX_DEL_GA(GA_NUM);

    }
    else if ( WebServer->hasArg("btn_del_cb") )
    {

      stmp = WebServer->arg("btn_del_cb");
      byte CB_NUM = stmp.toInt();

      KNX_DEL_CB(CB_NUM);

    }

    String page = FPSTR(HTTP_HEAD);
    page.replace(F("{v}"), FPSTR(S_CONFIGURE_KNX));
    page += FPSTR(HTTP_HEAD_STYLE);
    page.replace(F("340px"), F("530px"));
    page += FPSTR(HTTP_FORM_KNX);
    KNX_physs_addr.value = Settings.knx_physsical_addr;
    page.replace(F("{kna"), String(KNX_physs_addr.pa.area));
    page.replace(F("{knl"), String(KNX_physs_addr.pa.line));
    page.replace(F("{knm"), String(KNX_physs_addr.pa.member));
    if ( Settings.flag.knx_enabled ) { page += F(" checked"); }
    page += FPSTR(HTTP_FORM_KNX1);
    if ( Settings.flag.knx_enable_enhancement ) { page += F(" checked"); }

    page += FPSTR(HTTP_FORM_KNX2);
    for (byte i = 0; i < KNX_MAX_device_param ; i++)
    {
      if ( device_param[i].show )
      {
        page += FPSTR(HTTP_FORM_KNX_OPT);
        page.replace(F("{vop}"), String(device_param[i].type));
        page.replace(F("{nop}"), String(device_param_ga[i]));
      }
    }
    page += F("</select> -> ");
    page += FPSTR(HTTP_FORM_KNX_GA);
    page.replace(F("GAfnum"), F("GA_FNUM"));
    page.replace(F("GAarea"), F("GA_AREA"));
    page.replace(F("GAfdef"), F("GA_FDEF"));
    page.replace(F("GAfnum"), F("GA_FNUM"));
    page.replace(F("GAarea"), F("GA_AREA"));
    page.replace(F("GAfdef"), F("GA_FDEF"));
    page += FPSTR(HTTP_FORM_KNX_ADD_BTN);
    page.replace(F("{btnval}"), String(1));
    if (Settings.knx_GA_registered < MAX_KNX_GA) {
      page.replace(F("btndis"), F(" "));
    }
    else
    {
      page.replace(F("btndis"), F("disabled"));
    }
    page.replace(F("fncbtnadd"), F("GAwarning"));
    for (byte i = 0; i < Settings.knx_GA_registered ; ++i)
    {
      if ( Settings.knx_GA_param[i] )
      {
        page += FPSTR(HTTP_FORM_KNX_ADD_TABLE_ROW);
        page.replace(F("{opval}"), String(i+1));
        page.replace(F("{optex}"), String(device_param_ga[Settings.knx_GA_param[i]-1]));
        KNX_addr.value = Settings.knx_GA_addr[i];
        page.replace(F("GAfnum"), String(KNX_addr.ga.area));
        page.replace(F("GAarea"), String(KNX_addr.ga.line));
        page.replace(F("GAfdef"), String(KNX_addr.ga.member));
      }
    }

    page += FPSTR(HTTP_FORM_KNX3);
    page += FPSTR(HTTP_FORM_KNX_GA);
    page.replace(F("GAfnum"), F("CB_FNUM"));
    page.replace(F("GAarea"), F("CB_AREA"));
    page.replace(F("GAfdef"), F("CB_FDEF"));
    page.replace(F("GAfnum"), F("CB_FNUM"));
    page.replace(F("GAarea"), F("CB_AREA"));
    page.replace(F("GAfdef"), F("CB_FDEF"));
    page += FPSTR(HTTP_FORM_KNX4);
    byte j;
    for (byte i = 0; i < KNX_MAX_device_param ; i++)
    {

      if ( (i > 8) && (i < 16) ) { j=i-8; } else { j=i; }
      if ( i == 8 ) { j = 0; }
      if ( device_param[j].show )
      {
        page += FPSTR(HTTP_FORM_KNX_OPT);
        page.replace(F("{vop}"), String(device_param[i].type));
        page.replace(F("{nop}"), String(device_param_cb[i]));
      }
    }
    page += F("</select> ");
    page += FPSTR(HTTP_FORM_KNX_ADD_BTN);
    page.replace(F("{btnval}"), String(2));
    if (Settings.knx_CB_registered < MAX_KNX_CB) {
      page.replace(F("btndis"), F(" "));
    }
    else
    {
      page.replace(F("btndis"), F("disabled"));
    }
    page.replace(F("fncbtnadd"), F("CBwarning"));

    for (byte i = 0; i < Settings.knx_CB_registered ; ++i)
    {
      if ( Settings.knx_CB_param[i] )
      {
        page += FPSTR(HTTP_FORM_KNX_ADD_TABLE_ROW2);
        page.replace(F("{opval}"), String(i+1));
        page.replace(F("{optex}"), String(device_param_cb[Settings.knx_CB_param[i]-1]));
        KNX_addr.value = Settings.knx_CB_addr[i];
        page.replace(F("GAfnum"), String(KNX_addr.ga.area));
        page.replace(F("GAarea"), String(KNX_addr.ga.line));
        page.replace(F("GAfdef"), String(KNX_addr.ga.member));
      }
    }
    page += F("</table></center></fieldset>");
    page += F("<br/><button name='save' type='submit' class='button bgrn'>" D_SAVE "</button></form></fieldset>");
    page += FPSTR(HTTP_BTN_CONF);

    page.replace( F("</script>"),
      F("function GAwarning()"
        "{"
          "var GA_FNUM = document.getElementById('GA_FNUM');"
          "var GA_AREA = document.getElementById('GA_AREA');"
          "var GA_FDEF = document.getElementById('GA_FDEF');"
          "if ( GA_FNUM != null && GA_FNUM.value == '0' && GA_AREA.value == '0' && GA_FDEF.value == '0' ) {"
            "alert('" D_KNX_WARNING "');"
          "}"
        "}"
        "function CBwarning()"
        "{"
          "var CB_FNUM = document.getElementById('CB_FNUM');"
          "var CB_AREA = document.getElementById('CB_AREA');"
          "var CB_FDEF = document.getElementById('CB_FDEF');"
          "if ( CB_FNUM != null && CB_FNUM.value == '0' && CB_AREA.value == '0' && CB_FDEF.value == '0' ) {"
            "alert('" D_KNX_WARNING "');"
          "}"
        "}"
      "</script>") );
    ShowPage(page);
  }

}


void KNX_Save_Settings()
{
  String stmp;
  address_t KNX_addr;

  Settings.flag.knx_enabled = WebServer->hasArg("b1");
  Settings.flag.knx_enable_enhancement = WebServer->hasArg("b2");
  snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_KNX D_ENABLED ": %d, " D_KNX_ENHANCEMENT ": %d"),
   Settings.flag.knx_enabled, Settings.flag.knx_enable_enhancement );
  AddLog(LOG_LEVEL_DEBUG);

  stmp = WebServer->arg("area");
  KNX_addr.pa.area = stmp.toInt();
  stmp = WebServer->arg("line");
  KNX_addr.pa.line = stmp.toInt();
  stmp = WebServer->arg("member");
  KNX_addr.pa.member = stmp.toInt();
  Settings.knx_physsical_addr = KNX_addr.value;
  knx.physical_address_set( KNX_addr );
  snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_KNX D_KNX_PHYSICAL_ADDRESS ": %d.%d.%d "),
   KNX_addr.pa.area, KNX_addr.pa.line, KNX_addr.pa.member );
  AddLog(LOG_LEVEL_DEBUG);

  snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_KNX "GA: %d"),
   Settings.knx_GA_registered );
  AddLog(LOG_LEVEL_DEBUG);
  for (byte i = 0; i < Settings.knx_GA_registered ; ++i)
  {
    KNX_addr.value = Settings.knx_GA_addr[i];
    snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_KNX "GA #%d: %s " D_TO " %d/%d/%d"),
     i+1, device_param_ga[Settings.knx_GA_param[i]-1],
     KNX_addr.ga.area, KNX_addr.ga.line, KNX_addr.ga.member );
    AddLog(LOG_LEVEL_DEBUG);
  }

  snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_KNX "CB: %d"),
   Settings.knx_CB_registered );
  AddLog(LOG_LEVEL_DEBUG);
  for (byte i = 0; i < Settings.knx_CB_registered ; ++i)
  {
    KNX_addr.value = Settings.knx_CB_addr[i];
    snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_KNX "CB #%d: %d/%d/%d " D_TO " %s"),
     i+1,
     KNX_addr.ga.area, KNX_addr.ga.line, KNX_addr.ga.member,
     device_param_cb[Settings.knx_CB_param[i]-1] );
    AddLog(LOG_LEVEL_DEBUG);
  }
}

#endif
#endif


boolean KnxCommand()
{
  char command[CMDSZ];
  uint8_t index = XdrvMailbox.index;
  int command_code = GetCommandCode(command, sizeof(command), XdrvMailbox.topic, kKnxCommands);

  if (-1 == command_code) { return false; }

  else if ((CMND_KNXTXCMND == command_code) && (index > 0) && (index <= MAX_KNXTX_CMNDS) && (XdrvMailbox.data_len > 0)) {


    if (!(Settings.flag.knx_enabled)) { return false; }

    byte i = KNX_GA_Search(index + KNX_SLOT1 -1);
    while ( i != KNX_Empty ) {
      KNX_addr.value = Settings.knx_GA_addr[i];
      knx.write_1bit(KNX_addr, !(XdrvMailbox.payload == 0));
      if (Settings.flag.knx_enable_enhancement) {
        knx.write_1bit(KNX_addr, !(XdrvMailbox.payload == 0));
        knx.write_1bit(KNX_addr, !(XdrvMailbox.payload == 0));
      }

      snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_KNX "%s = %d " D_SENT_TO " %d.%d.%d"),
       device_param_ga[index + KNX_SLOT1 -2], !(XdrvMailbox.payload == 0),
       KNX_addr.ga.area, KNX_addr.ga.line, KNX_addr.ga.member);
      AddLog(LOG_LEVEL_INFO);

      i = KNX_GA_Search(index + KNX_SLOT1 -1, i + 1);
    }
    snprintf_P (mqtt_data, sizeof(mqtt_data), PSTR("{\"%s%d\":\"%s\"}"),
      command, index, XdrvMailbox.data );
  }

  else if ((CMND_KNXTXVAL == command_code) && (index > 0) && (index <= MAX_KNXTX_CMNDS) && (XdrvMailbox.data_len > 0)) {


    if (!(Settings.flag.knx_enabled)) { return false; }

    byte i = KNX_GA_Search(index + KNX_SLOT1 -1);
    while ( i != KNX_Empty ) {
      KNX_addr.value = Settings.knx_GA_addr[i];

      float tempvar = CharToDouble(XdrvMailbox.data);
      dtostrfd(tempvar,2,XdrvMailbox.data);

      knx.write_2byte_float(KNX_addr, tempvar);
      if (Settings.flag.knx_enable_enhancement) {
        knx.write_2byte_float(KNX_addr, tempvar);
        knx.write_2byte_float(KNX_addr, tempvar);
      }

      snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_KNX "%s = %s " D_SENT_TO " %d.%d.%d"),
       device_param_ga[index + KNX_SLOT1 -2], XdrvMailbox.data,
       KNX_addr.ga.area, KNX_addr.ga.line, KNX_addr.ga.member);
      AddLog(LOG_LEVEL_INFO);

      i = KNX_GA_Search(index + KNX_SLOT1 -1, i + 1);
    }
    snprintf_P (mqtt_data, sizeof(mqtt_data), PSTR("{\"%s%d\":\"%s\"}"),
      command, index, XdrvMailbox.data );
  }

  else if (CMND_KNX_ENABLED == command_code) {
    if (!XdrvMailbox.data_len) {
      if (Settings.flag.knx_enabled) {
        snprintf_P(XdrvMailbox.data, sizeof(XdrvMailbox.data), PSTR("1"));
      } else {
        snprintf_P(XdrvMailbox.data, sizeof(XdrvMailbox.data), PSTR("0"));
      }
    } else {
      if (XdrvMailbox.payload == 1) {
        Settings.flag.knx_enabled = 1;
      } else if (XdrvMailbox.payload == 0) {
        Settings.flag.knx_enabled = 0;
      } else { return false; }
    }
    snprintf_P (mqtt_data, sizeof(mqtt_data), PSTR("{\"%s\":\"%s\"}"),
      command, XdrvMailbox.data );
  }

  else if (CMND_KNX_ENHANCED == command_code) {
    if (!XdrvMailbox.data_len) {
      if (Settings.flag.knx_enable_enhancement) {
        snprintf_P(XdrvMailbox.data, sizeof(XdrvMailbox.data), PSTR("1"));
      } else {
        snprintf_P(XdrvMailbox.data, sizeof(XdrvMailbox.data), PSTR("0"));
      }
    } else {
      if (XdrvMailbox.payload == 1) {
        Settings.flag.knx_enable_enhancement = 1;
      } else if (XdrvMailbox.payload == 0) {
        Settings.flag.knx_enable_enhancement = 0;
      } else { return false; }
    }
    snprintf_P (mqtt_data, sizeof(mqtt_data), PSTR("{\"%s\":\"%s\"}"),
      command, XdrvMailbox.data );
  }

  else if (CMND_KNX_PA == command_code) {
    if (XdrvMailbox.data_len) {
      if (strstr(XdrvMailbox.data, ".")) {
        char sub_string[XdrvMailbox.data_len];

        int pa_area = atoi(subStr(sub_string, XdrvMailbox.data, ".", 1));
        int pa_line = atoi(subStr(sub_string, XdrvMailbox.data, ".", 2));
        int pa_member = atoi(subStr(sub_string, XdrvMailbox.data, ".", 3));

        if ( ((pa_area == 0) && (pa_line == 0) && (pa_member == 0))
             || (pa_area > 15) || (pa_line > 15) || (pa_member > 255) ) {
               snprintf_P (mqtt_data, sizeof(mqtt_data), PSTR("{\"%s\":\"" D_ERROR "\"}"),
                 command );
               return true;
        }

        KNX_addr.pa.area = pa_area;
        KNX_addr.pa.line = pa_line;
        KNX_addr.pa.member = pa_member;
        Settings.knx_physsical_addr = KNX_addr.value;
      }
    }
    KNX_addr.value = Settings.knx_physsical_addr;
    snprintf_P (mqtt_data, sizeof(mqtt_data), PSTR("{\"%s\":\"%d.%d.%d\"}"),
      command, KNX_addr.pa.area, KNX_addr.pa.line, KNX_addr.pa.member );
  }

  else if ((CMND_KNX_GA == command_code) && (index > 0) && (index <= MAX_KNX_GA)) {
    if (XdrvMailbox.data_len) {
      if (strstr(XdrvMailbox.data, ",")) {
        char sub_string[XdrvMailbox.data_len];

        int ga_option = atoi(subStr(sub_string, XdrvMailbox.data, ",", 1));
        int ga_area = atoi(subStr(sub_string, XdrvMailbox.data, ",", 2));
        int ga_line = atoi(subStr(sub_string, XdrvMailbox.data, ",", 3));
        int ga_member = atoi(subStr(sub_string, XdrvMailbox.data, ",", 4));

        if ( ((ga_area == 0) && (ga_line == 0) && (ga_member == 0))
          || (ga_area > 31) || (ga_line > 7) || (ga_member > 255)
          || (ga_option < 0) || ((ga_option > KNX_MAX_device_param ) && (ga_option != KNX_Empty))
          || (!device_param[ga_option-1].show) ) {
               snprintf_P (mqtt_data, sizeof(mqtt_data), PSTR("{\"%s\":\"" D_ERROR "\"}"), command );
               return true;
        }

        KNX_addr.ga.area = ga_area;
        KNX_addr.ga.line = ga_line;
        KNX_addr.ga.member = ga_member;

        if ( index > Settings.knx_GA_registered ) {
          Settings.knx_GA_registered ++;
          index = Settings.knx_GA_registered;
        }

        Settings.knx_GA_addr[index -1] = KNX_addr.value;
        Settings.knx_GA_param[index -1] = ga_option;
      } else {
        if ( (XdrvMailbox.payload <= Settings.knx_GA_registered) && (XdrvMailbox.payload > 0) ) {
          index = XdrvMailbox.payload;
        } else {
          snprintf_P (mqtt_data, sizeof(mqtt_data), PSTR("{\"%s\":\"" D_ERROR "\"}"), command );
          return true;
        }
      }
      if ( index <= Settings.knx_GA_registered ) {
        KNX_addr.value = Settings.knx_GA_addr[index -1];
        snprintf_P (mqtt_data, sizeof(mqtt_data), PSTR("{\"%s%d\":\"%s, %d/%d/%d\"}"),
          command, index, device_param_ga[Settings.knx_GA_param[index-1]-1],
          KNX_addr.ga.area, KNX_addr.ga.line, KNX_addr.ga.member );
      }
    } else {
      snprintf_P (mqtt_data, sizeof(mqtt_data), PSTR("{\"%s\":\"%d\"}"),
        command, Settings.knx_GA_registered );
    }
  }

  else if ((CMND_KNX_CB == command_code) && (index > 0) && (index <= MAX_KNX_CB)) {
    if (XdrvMailbox.data_len) {
      if (strstr(XdrvMailbox.data, ",")) {
        char sub_string[XdrvMailbox.data_len];

        int cb_option = atoi(subStr(sub_string, XdrvMailbox.data, ",", 1));
        int cb_area = atoi(subStr(sub_string, XdrvMailbox.data, ",", 2));
        int cb_line = atoi(subStr(sub_string, XdrvMailbox.data, ",", 3));
        int cb_member = atoi(subStr(sub_string, XdrvMailbox.data, ",", 4));

        if ( ((cb_area == 0) && (cb_line == 0) && (cb_member == 0))
          || (cb_area > 31) || (cb_line > 7) || (cb_member > 255)
          || (cb_option < 0) || ((cb_option > KNX_MAX_device_param ) && (cb_option != KNX_Empty))
          || (!device_param[cb_option-1].show) ) {
               snprintf_P (mqtt_data, sizeof(mqtt_data), PSTR("{\"%s\":\"" D_ERROR "\"}"), command );
               return true;
        }

        KNX_addr.ga.area = cb_area;
        KNX_addr.ga.line = cb_line;
        KNX_addr.ga.member = cb_member;

        if ( index > Settings.knx_CB_registered ) {
          Settings.knx_CB_registered ++;
          index = Settings.knx_CB_registered;
        }

        Settings.knx_CB_addr[index -1] = KNX_addr.value;
        Settings.knx_CB_param[index -1] = cb_option;
      } else {
        if ( (XdrvMailbox.payload <= Settings.knx_CB_registered) && (XdrvMailbox.payload > 0) ) {
          index = XdrvMailbox.payload;
        } else {
          snprintf_P (mqtt_data, sizeof(mqtt_data), PSTR("{\"%s\":\"" D_ERROR "\"}"), command );
          return true;
        }
      }
      if ( index <= Settings.knx_CB_registered ) {
        KNX_addr.value = Settings.knx_CB_addr[index -1];
        snprintf_P (mqtt_data, sizeof(mqtt_data), PSTR("{\"%s%d\":\"%s, %d/%d/%d\"}"),
          command, index, device_param_cb[Settings.knx_CB_param[index-1]-1],
          KNX_addr.ga.area, KNX_addr.ga.line, KNX_addr.ga.member );
      }
    } else {
      snprintf_P (mqtt_data, sizeof(mqtt_data), PSTR("{\"%s\":\"%d\"}"),
        command, Settings.knx_CB_registered );
    }
  }

  else { return false; }

  return true;
}






#define XDRV_11 

boolean Xdrv11(byte function)
{
  boolean result = false;
    switch (function) {
      case FUNC_PRE_INIT:
        KNX_INIT();
        break;
#ifdef USE_WEBSERVER
#ifdef USE_KNX_WEB_MENU
      case FUNC_WEB_ADD_BUTTON:
        strncat_P(mqtt_data, HTTP_BTN_MENU_KNX, sizeof(mqtt_data));
        break;
      case FUNC_WEB_ADD_HANDLER:
        WebServer->on("/kn", HandleKNXConfiguration);
        break;
#endif
#endif
      case FUNC_LOOP:
        knx.loop();
        break;
      case FUNC_EVERY_50_MSECOND:
        if (toggle_inhibit) {
          toggle_inhibit--;
        }
        break;
      case FUNC_COMMAND:
        result = KnxCommand();
        break;


    }
  return result;
}

#endif
# 1 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xdrv_12_home_assistant.ino"
# 20 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xdrv_12_home_assistant.ino"
#ifdef USE_HOME_ASSISTANT

const char HASS_DISCOVER_RELAY[] PROGMEM =
  "{\"name\":\"%s\","
  "\"command_topic\":\"%s\","
  "\"state_topic\":\"%s\","
  "\"value_template\":\"{{value_json.%s}}\","
  "\"payload_off\":\"%s\","
  "\"payload_on\":\"%s\","

  "\"availability_topic\":\"%s\","
  "\"payload_available\":\"" D_ONLINE "\","
  "\"payload_not_available\":\"" D_OFFLINE "\"";

const char HASS_DISCOVER_BUTTON[] PROGMEM =
  "{\"name\":\"%s\","
  "\"state_topic\":\"%s\","

  "\"payload_on\":\"%s\","

  "\"availability_topic\":\"%s\","
  "\"payload_available\":\"" D_ONLINE "\","
  "\"payload_not_available\":\"" D_OFFLINE "\","
  "\"force_update\":true";

const char HASS_DISCOVER_LIGHT_DIMMER[] PROGMEM =
  "%s,\"brightness_command_topic\":\"%s\","
  "\"brightness_state_topic\":\"%s\","
  "\"brightness_scale\":100,"
  "\"on_command_type\":\"brightness\","
  "\"brightness_value_template\":\"{{value_json." D_CMND_DIMMER "}}\"";

const char HASS_DISCOVER_LIGHT_COLOR[] PROGMEM =
  "%s,\"rgb_command_topic\":\"%s2\","
  "\"rgb_state_topic\":\"%s\","
  "\"rgb_value_template\":\"{{value_json." D_CMND_COLOR "}}\"";


const char HASS_DISCOVER_LIGHT_CT[] PROGMEM =
  "%s,\"color_temp_command_topic\":\"%s\","
  "\"color_temp_state_topic\":\"%s\","
  "\"color_temp_value_template\":\"{{value_json." D_CMND_COLORTEMPERATURE "}}\"";
# 70 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xdrv_12_home_assistant.ino"
void HAssDiscoverRelay()
{
  char sidx[8];
  char stopic[TOPSZ];
  bool is_light = false;
  bool is_topic_light = false;

  for (int i = 1; i <= MAX_RELAYS; i++) {
    is_light = ((i == devices_present) && (light_type));
    is_topic_light = Settings.flag.hass_light || is_light;

    mqtt_data[0] = '\0';

    snprintf_P(sidx, sizeof(sidx), PSTR("_%d"), i);

    snprintf_P(stopic, sizeof(stopic), PSTR(HOME_ASSISTANT_DISCOVERY_PREFIX "/%s/%s%s/config"), (is_topic_light) ? "switch" : "light", mqtt_topic, sidx);
    MqttPublish(stopic, true);

    snprintf_P(stopic, sizeof(stopic), PSTR(HOME_ASSISTANT_DISCOVERY_PREFIX "/%s/%s%s/config"), (is_topic_light) ? "light" : "switch", mqtt_topic, sidx);

    if (Settings.flag.hass_discovery && (i <= devices_present)) {
      char name[33];
      char value_template[33];
      char command_topic[TOPSZ];
      char state_topic[TOPSZ];
      char availability_topic[TOPSZ];

      if (i > MAX_FRIENDLYNAMES) {
        snprintf_P(name, sizeof(name), PSTR("%s %d"), Settings.friendlyname[0], i);
      } else {
        snprintf_P(name, sizeof(name), Settings.friendlyname[i -1]);
      }
      GetPowerDevice(value_template, i, sizeof(value_template), Settings.flag.device_index_enable);
      GetTopic_P(command_topic, CMND, mqtt_topic, value_template);
      GetTopic_P(state_topic, STAT, mqtt_topic, S_RSLT_RESULT);
      GetTopic_P(availability_topic, TELE, mqtt_topic, S_LWT);
      snprintf_P(mqtt_data, sizeof(mqtt_data), HASS_DISCOVER_RELAY, name, command_topic, state_topic, value_template, Settings.state_text[0], Settings.state_text[1], availability_topic);

      if (is_light) {
        char brightness_command_topic[TOPSZ];

        GetTopic_P(brightness_command_topic, CMND, mqtt_topic, D_CMND_DIMMER);
        snprintf_P(mqtt_data, sizeof(mqtt_data), HASS_DISCOVER_LIGHT_DIMMER, mqtt_data, brightness_command_topic, state_topic);

        if (light_subtype >= LST_RGB) {
          char rgb_command_topic[TOPSZ];

          GetTopic_P(rgb_command_topic, CMND, mqtt_topic, D_CMND_COLOR);
          snprintf_P(mqtt_data, sizeof(mqtt_data), HASS_DISCOVER_LIGHT_COLOR, mqtt_data, rgb_command_topic, state_topic);






        }
        if ((LST_COLDWARM == light_subtype) || (LST_RGBWC == light_subtype)) {
          char color_temp_command_topic[TOPSZ];

          GetTopic_P(color_temp_command_topic, CMND, mqtt_topic, D_CMND_COLORTEMPERATURE);
          snprintf_P(mqtt_data, sizeof(mqtt_data), HASS_DISCOVER_LIGHT_CT, mqtt_data, color_temp_command_topic, state_topic);
        }
      }
      snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s}"), mqtt_data);
    }
    MqttPublish(stopic, true);
  }
}

void HAssDiscoverButton()
{
  char sidx[8];
  char stopic[TOPSZ];
  char key_topic[sizeof(Settings.button_topic)];


  char *tmp = Settings.button_topic;
  Format(key_topic, tmp, sizeof(key_topic));
  if ((strlen(key_topic) != 0) && strcmp(key_topic, "0")) {
    for (byte button_index = 0; button_index < MAX_KEYS; button_index++) {
      uint8_t button_present = 0;

      if (!button_index && ((SONOFF_DUAL == Settings.module) || (CH4 == Settings.module))) {
        button_present = 1;
      } else {
        if (pin[GPIO_KEY1 + button_index] < 99) {
          button_present = 1;
        }
      }

      mqtt_data[0] = '\0';


      snprintf_P(sidx, sizeof(sidx), PSTR("_%d"), button_index+1);
      snprintf_P(stopic, sizeof(stopic), PSTR(HOME_ASSISTANT_DISCOVERY_PREFIX "/%s/%s%s/config"), "binary_sensor", key_topic, sidx);

      if (Settings.flag.hass_discovery && button_present) {
        char name[33];
        char value_template[33];
        char state_topic[TOPSZ];
        char availability_topic[TOPSZ];

        if (button_index+1 > MAX_FRIENDLYNAMES) {
          snprintf_P(name, sizeof(name), PSTR("%s %d BTN"), Settings.friendlyname[0], button_index+1);
        } else {
          snprintf_P(name, sizeof(name), PSTR("%s BTN"), Settings.friendlyname[button_index]);
        }
        GetPowerDevice(value_template, button_index+1, sizeof(value_template), Settings.flag.device_index_enable);
        GetTopic_P(state_topic, CMND, key_topic, value_template);
        GetTopic_P(availability_topic, TELE, mqtt_topic, S_LWT);
        snprintf_P(mqtt_data, sizeof(mqtt_data), HASS_DISCOVER_BUTTON, name, state_topic, Settings.state_text[2], availability_topic);

        snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s}"), mqtt_data);
      }
      MqttPublish(stopic, true);
    }
  }
}

void HAssDiscovery(uint8_t mode)
{

  if (Settings.flag.hass_discovery) {
    Settings.flag.mqtt_response = 0;
    Settings.flag.decimal_text = 1;


  }

  if (Settings.flag.hass_discovery || (1 == mode)) {

    HAssDiscoverRelay();

    HAssDiscoverButton();



  }
}
# 258 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xdrv_12_home_assistant.ino"
#define XDRV_12 

boolean Xdrv12(byte function)
{
  boolean result = false;

  if (Settings.flag.mqtt_enabled) {
    switch (function) {
      case FUNC_MQTT_INIT:
        HAssDiscovery(0);
        break;





    }
  }
  return result;
}

#endif
# 1 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xdrv_13_display.ino"
# 20 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xdrv_13_display.ino"
#if defined(USE_I2C) || defined(USE_SPI)
#ifdef USE_DISPLAY

#define DISPLAY_MAX_DRIVERS 16
#define DISPLAY_MAX_COLS 40
#define DISPLAY_MAX_ROWS 32

#define DISPLAY_LOG_ROWS 32

#define D_CMND_DISPLAY "Display"
#define D_CMND_DISP_ADDRESS "Address"
#define D_CMND_DISP_COLS "Cols"
#define D_CMND_DISP_DIMMER "Dimmer"
#define D_CMND_DISP_MODE "Mode"
#define D_CMND_DISP_MODEL "Model"
#define D_CMND_DISP_REFRESH "Refresh"
#define D_CMND_DISP_ROWS "Rows"
#define D_CMND_DISP_SIZE "Size"
#define D_CMND_DISP_FONT "Font"
#define D_CMND_DISP_ROTATE "Rotate"
#define D_CMND_DISP_TEXT "Text"

enum XdspFunctions { FUNC_DISPLAY_INIT_DRIVER, FUNC_DISPLAY_INIT, FUNC_DISPLAY_EVERY_50_MSECOND, FUNC_DISPLAY_EVERY_SECOND,
                     FUNC_DISPLAY_MODEL, FUNC_DISPLAY_MODE, FUNC_DISPLAY_POWER,
                     FUNC_DISPLAY_CLEAR, FUNC_DISPLAY_DRAW_FRAME,
                     FUNC_DISPLAY_DRAW_HLINE, FUNC_DISPLAY_DRAW_VLINE, FUNC_DISPLAY_DRAW_LINE,
                     FUNC_DISPLAY_DRAW_CIRCLE, FUNC_DISPLAY_FILL_CIRCLE,
                     FUNC_DISPLAY_DRAW_RECTANGLE, FUNC_DISPLAY_FILL_RECTANGLE,
                     FUNC_DISPLAY_TEXT_SIZE, FUNC_DISPLAY_FONT_SIZE, FUNC_DISPLAY_ROTATION, FUNC_DISPLAY_DRAW_STRING, FUNC_DISPLAY_ONOFF };

enum DisplayInitModes { DISPLAY_INIT_MODE, DISPLAY_INIT_PARTIAL, DISPLAY_INIT_FULL };

enum DisplayCommands { CMND_DISPLAY, CMND_DISP_MODEL, CMND_DISP_MODE, CMND_DISP_REFRESH, CMND_DISP_DIMMER, CMND_DISP_COLS, CMND_DISP_ROWS,
  CMND_DISP_SIZE, CMND_DISP_FONT, CMND_DISP_ROTATE, CMND_DISP_TEXT, CMND_DISP_ADDRESS };
const char kDisplayCommands[] PROGMEM =
  "|" D_CMND_DISP_MODEL "|" D_CMND_DISP_MODE "|" D_CMND_DISP_REFRESH "|" D_CMND_DISP_DIMMER "|" D_CMND_DISP_COLS "|" D_CMND_DISP_ROWS "|"
  D_CMND_DISP_SIZE "|" D_CMND_DISP_FONT "|" D_CMND_DISP_ROTATE "|" D_CMND_DISP_TEXT "|" D_CMND_DISP_ADDRESS ;

const char S_JSON_DISPLAY_COMMAND_VALUE[] PROGMEM = "{\"" D_CMND_DISPLAY "%s\":\"%s\"}";
const char S_JSON_DISPLAY_COMMAND_NVALUE[] PROGMEM = "{\"" D_CMND_DISPLAY "%s\":%d}";
const char S_JSON_DISPLAY_COMMAND_INDEX_NVALUE[] PROGMEM = "{\"" D_CMND_DISPLAY "%s%d\":%d}";

uint8_t disp_power = 0;
uint8_t disp_device = 0;
uint8_t disp_refresh = 1;

int16_t disp_xpos = 0;
int16_t disp_ypos = 0;
uint8_t disp_autodraw = 1;

uint8_t dsp_init;
uint8_t dsp_font;
uint8_t dsp_flag;
uint8_t dsp_on;
uint16_t dsp_x;
uint16_t dsp_y;
uint16_t dsp_x2;
uint16_t dsp_y2;
uint16_t dsp_rad;
uint16_t dsp_color;
int16_t dsp_len;
char *dsp_str;

#ifdef USE_DISPLAY_MODES1TO5

char disp_temp[2];
uint8_t disp_subscribed = 0;

char **disp_log_buffer;
uint8_t disp_log_buffer_cols = 0;
uint8_t disp_log_buffer_idx = 0;
uint8_t disp_log_buffer_ptr = 0;

char **disp_screen_buffer;
uint8_t disp_screen_buffer_cols = 0;
uint8_t disp_screen_buffer_rows = 0;

#endif



void DisplayInit(uint8_t mode)
{
  dsp_init = mode;
  XdspCall(FUNC_DISPLAY_INIT);
}

void DisplayClear()
{
  XdspCall(FUNC_DISPLAY_CLEAR);
}

void DisplayDrawHLine(uint16_t x, uint16_t y, int16_t len, uint16_t color)
{
  dsp_x = x;
  dsp_y = y;
  dsp_len = len;
  dsp_color = color;
  XdspCall(FUNC_DISPLAY_DRAW_HLINE);
}

void DisplayDrawVLine(uint16_t x, uint16_t y, int16_t len, uint16_t color)
{
  dsp_x = x;
  dsp_y = y;
  dsp_len = len;
  dsp_color = color;
  XdspCall(FUNC_DISPLAY_DRAW_VLINE);
}

void DisplayDrawLine(uint16_t x, uint16_t y, uint16_t x2, uint16_t y2, uint16_t color)
{
  dsp_x = x;
  dsp_y = y;
  dsp_x2 = x2;
  dsp_y2 = y2;
  dsp_color = color;
  XdspCall(FUNC_DISPLAY_DRAW_LINE);
}

void DisplayDrawCircle(uint16_t x, uint16_t y, uint16_t rad, uint16_t color)
{
  dsp_x = x;
  dsp_y = y;
  dsp_rad = rad;
  dsp_color = color;
  XdspCall(FUNC_DISPLAY_DRAW_CIRCLE);
}

void DisplayDrawFilledCircle(uint16_t x, uint16_t y, uint16_t rad, uint16_t color)
{
  dsp_x = x;
  dsp_y = y;
  dsp_rad = rad;
  dsp_color = color;
  XdspCall(FUNC_DISPLAY_FILL_CIRCLE);
}

void DisplayDrawRectangle(uint16_t x, uint16_t y, uint16_t x2, uint16_t y2, uint16_t color)
{
  dsp_x = x;
  dsp_y = y;
  dsp_x2 = x2;
  dsp_y2 = y2;
  dsp_color = color;
  XdspCall(FUNC_DISPLAY_DRAW_RECTANGLE);
}

void DisplayDrawFilledRectangle(uint16_t x, uint16_t y, uint16_t x2, uint16_t y2, uint16_t color)
{
  dsp_x = x;
  dsp_y = y;
  dsp_x2 = x2;
  dsp_y2 = y2;
  dsp_color = color;
  XdspCall(FUNC_DISPLAY_FILL_RECTANGLE);
}

void DisplayDrawFrame()
{
  XdspCall(FUNC_DISPLAY_DRAW_FRAME);
}

void DisplaySetSize(uint8_t size)
{
  Settings.display_size = size &3;
  XdspCall(FUNC_DISPLAY_TEXT_SIZE);
}

void DisplaySetFont(uint8_t font)
{
  Settings.display_font = font &3;
  XdspCall(FUNC_DISPLAY_FONT_SIZE);
}

void DisplaySetRotation(uint8_t rotation)
{
  Settings.display_rotate = rotation &3;
  XdspCall(FUNC_DISPLAY_ROTATION);
}

void DisplayDrawStringAt(uint16_t x, uint16_t y, char *str, uint16_t color, uint8_t flag)
{
  dsp_x = x;
  dsp_y = y;
  dsp_str = str;
  dsp_color = color;
  dsp_flag = flag;
  XdspCall(FUNC_DISPLAY_DRAW_STRING);
}

void DisplayOnOff(uint8_t on)
{
  dsp_on = on;
  XdspCall(FUNC_DISPLAY_ONOFF);
}




uint8_t atoiv(char *cp, int16_t *res)
{
  uint8_t index = 0;
  *res = atoi(cp);
  while (*cp) {
    if ((*cp>='0' && *cp<='9') || (*cp=='-')) {
      cp++;
      index++;
    } else {
      break;
    }
  }
  return index;
}


uint8_t atoiV(char *cp, uint16_t *res)
{
  uint8_t index = 0;
  *res = atoi(cp);
  while (*cp) {
    if (*cp>='0' && *cp<='9') {
      cp++;
      index++;
    } else {
      break;
    }
  }
  return index;
}



#define DISPLAY_BUFFER_COLS 128

void DisplayText()
{
  uint8_t lpos;
  uint8_t escape = 0;
  uint8_t var;
  uint8_t font_x = 6;
  uint8_t font_y = 8;
  uint8_t fontnumber = 1;
  int16_t lin = 0;
  int16_t col = 0;
  int16_t fill = 0;
  int16_t temp;
  int16_t temp1;
  uint16_t color = 0;

  char linebuf[DISPLAY_BUFFER_COLS];
  char *dp = linebuf;
  char *cp = XdrvMailbox.data;

  memset(linebuf, ' ', sizeof(linebuf));
  linebuf[sizeof(linebuf)-1] = 0;
  *dp = 0;

  while (*cp) {
    if (!escape) {

      if (*cp == '[') {
        escape = 1;
        cp++;

        if ((uint32_t)dp - (uint32_t)linebuf) {
          if (!fill) { *dp = 0; }
          if (col > 0 && lin > 0) {

            DisplayDrawStringAt(col, lin, linebuf, color, 1);
          } else {

            DisplayDrawStringAt(disp_xpos, disp_ypos, linebuf, color, 0);
          }
          memset(linebuf, ' ', sizeof(linebuf));
          linebuf[sizeof(linebuf)-1] = 0;
          dp = linebuf;
        }
      } else {

        if (dp < (linebuf + DISPLAY_BUFFER_COLS)) { *dp++ = *cp++; }
      }
    } else {

      if (*cp == ']') {
        escape = 0;
        cp++;
      } else {

        switch (*cp++) {
          case 'z':

            DisplayClear();
            disp_xpos = 0;
            disp_ypos = 0;
            col = 0;
            lin = 0;
            break;
          case 'i':

            DisplayInit(DISPLAY_INIT_PARTIAL);
            break;
          case 'I':

            DisplayInit(DISPLAY_INIT_FULL);
            break;
          case 'o':
            DisplayOnOff(0);
            break;
          case 'O':
            DisplayOnOff(1);
            break;
          case 'x':

            var = atoiv(cp, &disp_xpos);
            cp += var;
            break;
          case 'y':

            var = atoiv(cp, &disp_ypos);
            cp += var;
            break;
          case 'l':

            var = atoiv(cp, &lin);
            cp += var;

            break;
          case 'c':

            var = atoiv(cp, &col);
            cp += var;

            break;
          case 'C':

            var = atoiV(cp, &color);
            cp += var;
            break;
          case 'p':

            var = atoiv(cp, &fill);
            cp += var;
            linebuf[fill] = 0;
            break;
          case 'h':

            var = atoiv(cp, &temp);
            cp += var;
            if (temp < 0) {
              DisplayDrawHLine(disp_xpos + temp, disp_ypos, -temp, color);
            } else {
              DisplayDrawHLine(disp_xpos, disp_ypos, temp, color);
            }
            disp_xpos += temp;
            break;
          case 'v':

            var = atoiv(cp, &temp);
            cp += var;
            if (temp < 0) {
              DisplayDrawVLine(disp_xpos, disp_ypos + temp, -temp, color);
            } else {
              DisplayDrawVLine(disp_xpos, disp_ypos, temp, color);
            }
            disp_ypos += temp;
            break;
          case 'L':

            var = atoiv(cp, &temp);
            cp += var;
            cp++;
            var = atoiv(cp, &temp1);
            cp += var;
            DisplayDrawLine(disp_xpos, disp_ypos, temp, temp1, color);
            disp_xpos += temp;
            disp_ypos += temp1;
            break;
          case 'k':

            var = atoiv(cp, &temp);
            cp += var;
            DisplayDrawCircle(disp_xpos, disp_ypos, temp, color);
            break;
          case 'K':

            var = atoiv(cp, &temp);
            cp += var;
            DisplayDrawFilledCircle(disp_xpos, disp_ypos, temp, color);
            break;
          case 'r':

            var = atoiv(cp, &temp);
            cp += var;
            cp++;
            var = atoiv(cp, &temp1);
            cp += var;
            DisplayDrawRectangle(disp_xpos, disp_ypos, temp, temp1, color);
            break;
          case 'R':

            var = atoiv(cp, &temp);
            cp += var;
            cp++;
            var = atoiv(cp, &temp1);
            cp += var;
            DisplayDrawFilledRectangle(disp_xpos, disp_ypos, temp, temp1, color);
            break;
          case 't': {
            if (dp < (linebuf + DISPLAY_BUFFER_COLS) -5) {
              snprintf_P(dp, 5, PSTR("%02d" D_HOUR_MINUTE_SEPARATOR "%02d"), RtcTime.hour, RtcTime.minute);
              dp += 5;
            }
            break;
          }
          case 'd':

            DisplayDrawFrame();
            break;
          case 'D':

            disp_autodraw = *cp&1;
            cp += 1;
            break;
          case 's':

            DisplaySetSize(*cp&3);
            cp += 1;
            break;
          case 'f':

            DisplaySetFont(*cp&3);
            cp += 1;
            break;
          case 'a':

            DisplaySetRotation(*cp&3);
            cp+=1;
            break;
          default:

            snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("Unknown Escape"));
            goto exit;
            break;
        }
      }
    }
  }
  exit:

  if ((uint32_t)dp - (uint32_t)linebuf) {
    if (!fill) { *dp = 0; }
    if (col > 0 && lin > 0) {

      DisplayDrawStringAt(col, lin, linebuf, color, 1);
    } else {

      DisplayDrawStringAt(disp_xpos, disp_ypos, linebuf, color, 0);
    }
  }

  if (disp_autodraw) { DisplayDrawFrame(); }
}



#ifdef USE_DISPLAY_MODES1TO5

void DisplayClearScreenBuffer()
{
  if (disp_screen_buffer_cols) {
    for (byte i = 0; i < disp_screen_buffer_rows; i++) {
      memset(disp_screen_buffer[i], 0, disp_screen_buffer_cols);
    }
  }
}

void DisplayFreeScreenBuffer()
{
  if (disp_screen_buffer != NULL) {
    for (byte i = 0; i < disp_screen_buffer_rows; i++) {
      if (disp_screen_buffer[i] != NULL) { free(disp_screen_buffer[i]); }
    }
    free(disp_screen_buffer);
    disp_screen_buffer_cols = 0;
    disp_screen_buffer_rows = 0;
  }
}

void DisplayAllocScreenBuffer()
{
  if (!disp_screen_buffer_cols) {
    disp_screen_buffer_rows = Settings.display_rows;
    disp_screen_buffer = (char**)malloc(sizeof(*disp_screen_buffer) * disp_screen_buffer_rows);
    if (disp_screen_buffer != NULL) {
      for (byte i = 0; i < disp_screen_buffer_rows; i++) {
        disp_screen_buffer[i] = (char*)malloc(sizeof(*disp_screen_buffer[i]) * (Settings.display_cols[0] +1));
        if (disp_screen_buffer[i] == NULL) {
          DisplayFreeScreenBuffer();
          break;
        }
      }
    }
    if (disp_screen_buffer != NULL) {
      disp_screen_buffer_cols = Settings.display_cols[0] +1;
      DisplayClearScreenBuffer();
    }
  }
}

void DisplayReAllocScreenBuffer()
{
  DisplayFreeScreenBuffer();
  DisplayAllocScreenBuffer();
}

void DisplayFillScreen(uint8_t line)
{
  byte len = disp_screen_buffer_cols - strlen(disp_screen_buffer[line]);
  if (len) {
    memset(disp_screen_buffer[line] + strlen(disp_screen_buffer[line]), 0x20, len);
    disp_screen_buffer[line][disp_screen_buffer_cols -1] = 0;
  }
}



void DisplayClearLogBuffer()
{
  if (disp_log_buffer_cols) {
    for (byte i = 0; i < DISPLAY_LOG_ROWS; i++) {
      memset(disp_log_buffer[i], 0, disp_log_buffer_cols);
    }
  }
}

void DisplayFreeLogBuffer()
{
  if (disp_log_buffer != NULL) {
    for (byte i = 0; i < DISPLAY_LOG_ROWS; i++) {
      if (disp_log_buffer[i] != NULL) { free(disp_log_buffer[i]); }
    }
    free(disp_log_buffer);
    disp_log_buffer_cols = 0;
  }
}

void DisplayAllocLogBuffer()
{
  if (!disp_log_buffer_cols) {
    disp_log_buffer = (char**)malloc(sizeof(*disp_log_buffer) * DISPLAY_LOG_ROWS);
    if (disp_log_buffer != NULL) {
      for (byte i = 0; i < DISPLAY_LOG_ROWS; i++) {
        disp_log_buffer[i] = (char*)malloc(sizeof(*disp_log_buffer[i]) * (Settings.display_cols[0] +1));
        if (disp_log_buffer[i] == NULL) {
          DisplayFreeLogBuffer();
          break;
        }
      }
    }
    if (disp_log_buffer != NULL) {
      disp_log_buffer_cols = Settings.display_cols[0] +1;
      DisplayClearLogBuffer();
    }
  }
}

void DisplayReAllocLogBuffer()
{
  DisplayFreeLogBuffer();
  DisplayAllocLogBuffer();
}

void DisplayLogBufferAdd(char* txt)
{
  if (disp_log_buffer_cols) {
    strlcpy(disp_log_buffer[disp_log_buffer_idx], txt, disp_log_buffer_cols);
    disp_log_buffer_idx++;
    if (DISPLAY_LOG_ROWS == disp_log_buffer_idx) { disp_log_buffer_idx = 0; }
  }
}

char* DisplayLogBuffer(char temp_code)
{
  char* result = NULL;
  if (disp_log_buffer_cols) {
    if (disp_log_buffer_idx != disp_log_buffer_ptr) {
      result = disp_log_buffer[disp_log_buffer_ptr];
      disp_log_buffer_ptr++;
      if (DISPLAY_LOG_ROWS == disp_log_buffer_ptr) { disp_log_buffer_ptr = 0; }

      char *pch = strchr(result, '~');
      if (pch != NULL) { result[pch - result] = temp_code; }
    }
  }
  return result;
}

void DisplayLogBufferInit()
{
  if (Settings.display_mode) {
    disp_log_buffer_idx = 0;
    disp_log_buffer_ptr = 0;
    disp_refresh = Settings.display_refresh;

    snprintf_P(disp_temp, sizeof(disp_temp), PSTR("%c"), TempUnit());

    DisplayReAllocLogBuffer();

    char buffer[40];
    snprintf_P(buffer, sizeof(buffer), PSTR(D_VERSION " %s"), my_version);
    DisplayLogBufferAdd(buffer);
    snprintf_P(buffer, sizeof(buffer), PSTR("Display mode %d"), Settings.display_mode);
    DisplayLogBufferAdd(buffer);

    snprintf_P(buffer, sizeof(buffer), PSTR(D_CMND_HOSTNAME " %s"), my_hostname);
    DisplayLogBufferAdd(buffer);
    snprintf_P(buffer, sizeof(buffer), PSTR(D_JSON_SSID " %s"), Settings.sta_ssid[Settings.sta_active]);
    DisplayLogBufferAdd(buffer);
    snprintf_P(buffer, sizeof(buffer), PSTR(D_JSON_MAC " %s"), WiFi.macAddress().c_str());
    DisplayLogBufferAdd(buffer);
    if (!global_state.wifi_down && (static_cast<uint32_t>(WiFi.localIP()) != 0)) {
      snprintf_P(buffer, sizeof(buffer), PSTR("IP %s"), WiFi.localIP().toString().c_str());
      DisplayLogBufferAdd(buffer);
      snprintf_P(buffer, sizeof(buffer), PSTR(D_JSON_RSSI " %d%%"), WifiGetRssiAsQuality(WiFi.RSSI()));
      DisplayLogBufferAdd(buffer);
    }
  }
}





enum SensorQuantity {
  JSON_TEMPERATURE,
  JSON_HUMIDITY, JSON_LIGHT, JSON_NOISE, JSON_AIRQUALITY,
  JSON_PRESSURE, JSON_PRESSUREATSEALEVEL,
  JSON_ILLUMINANCE,
  JSON_GAS,
  JSON_YESTERDAY, JSON_TOTAL, JSON_TODAY,
  JSON_PERIOD,
  JSON_POWERFACTOR, JSON_COUNTER, JSON_ANALOG_INPUT, JSON_UV_LEVEL,
  JSON_CURRENT,
  JSON_VOLTAGE,
  JSON_POWERUSAGE,
  JSON_CO2,
  JSON_FREQUENCY };
const char kSensorQuantity[] PROGMEM =
  D_JSON_TEMPERATURE "|"
  D_JSON_HUMIDITY "|" D_JSON_LIGHT "|" D_JSON_NOISE "|" D_JSON_AIRQUALITY "|"
  D_JSON_PRESSURE "|" D_JSON_PRESSUREATSEALEVEL "|"
  D_JSON_ILLUMINANCE "|"
  D_JSON_GAS "|"
  D_JSON_YESTERDAY "|" D_JSON_TOTAL "|" D_JSON_TODAY "|"
  D_JSON_PERIOD "|"
  D_JSON_POWERFACTOR "|" D_JSON_COUNTER "|" D_JSON_ANALOG_INPUT "|" D_JSON_UV_LEVEL "|"
  D_JSON_CURRENT "|"
  D_JSON_VOLTAGE "|"
  D_JSON_POWERUSAGE "|"
  D_JSON_CO2 "|"
  D_JSON_FREQUENCY ;

void DisplayJsonValue(const char *topic, const char* device, const char* mkey, const char* value)
{
  char quantity[TOPSZ];
  char buffer[Settings.display_cols[0] +1];
  char spaces[Settings.display_cols[0]];
  char source[Settings.display_cols[0] - Settings.display_cols[1]];
  char svalue[Settings.display_cols[1] +1];

  ShowFreeMem(PSTR("DisplayJsonValue"));

  memset(spaces, 0x20, sizeof(spaces));
  spaces[sizeof(spaces) -1] = '\0';
  snprintf_P(source, sizeof(source), PSTR("%s/%s%s"), topic, mkey, spaces);

  int quantity_code = GetCommandCode(quantity, sizeof(quantity), mkey, kSensorQuantity);
  if ((-1 == quantity_code) || !strcmp_P(mkey, S_RSLT_POWER)) {
    return;
  }
  if (JSON_TEMPERATURE == quantity_code) {
    snprintf_P(svalue, sizeof(svalue), PSTR("%s~%s"), value, disp_temp);
  }
  else if ((quantity_code >= JSON_HUMIDITY) && (quantity_code <= JSON_AIRQUALITY)) {
    snprintf_P(svalue, sizeof(svalue), PSTR("%s%%"), value);
  }
  else if ((quantity_code >= JSON_PRESSURE) && (quantity_code <= JSON_PRESSUREATSEALEVEL)) {
    snprintf_P(svalue, sizeof(svalue), PSTR("%s" D_UNIT_PRESSURE), value);
  }
  else if (JSON_ILLUMINANCE == quantity_code) {
    snprintf_P(svalue, sizeof(svalue), PSTR("%s" D_UNIT_LUX), value);
  }
  else if (JSON_GAS == quantity_code) {
    snprintf_P(svalue, sizeof(svalue), PSTR("%s" D_UNIT_KILOOHM), value);
  }
  else if ((quantity_code >= JSON_YESTERDAY) && (quantity_code <= JSON_TODAY)) {
    snprintf_P(svalue, sizeof(svalue), PSTR("%s" D_UNIT_KILOWATTHOUR), value);
  }
  else if (JSON_PERIOD == quantity_code) {
    snprintf_P(svalue, sizeof(svalue), PSTR("%s" D_UNIT_WATTHOUR), value);
  }
  else if ((quantity_code >= JSON_POWERFACTOR) && (quantity_code <= JSON_UV_LEVEL)) {
    snprintf_P(svalue, sizeof(svalue), PSTR("%s"), value);
  }
  else if (JSON_CURRENT == quantity_code) {
    snprintf_P(svalue, sizeof(svalue), PSTR("%s" D_UNIT_AMPERE), value);
  }
  else if (JSON_VOLTAGE == quantity_code) {
    snprintf_P(svalue, sizeof(svalue), PSTR("%s" D_UNIT_VOLT), value);
  }
  else if (JSON_POWERUSAGE == quantity_code) {
    snprintf_P(svalue, sizeof(svalue), PSTR("%s" D_UNIT_WATT), value);
  }
  else if (JSON_CO2 == quantity_code) {
    snprintf_P(svalue, sizeof(svalue), PSTR("%s" D_UNIT_PARTS_PER_MILLION), value);
  }
  else if (JSON_FREQUENCY == quantity_code) {
    snprintf_P(svalue, sizeof(svalue), PSTR("%s" D_UNIT_HERTZ), value);
  }
  snprintf_P(buffer, sizeof(buffer), PSTR("%s %s"), source, svalue);




  DisplayLogBufferAdd(buffer);
}

void DisplayAnalyzeJson(char *topic, char *json)
{
# 760 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xdrv_13_display.ino"
  const char *tempunit;



  String jsonStr = json;

  StaticJsonBuffer<1024> jsonBuf;
  JsonObject &root = jsonBuf.parseObject(jsonStr);
  if (root.success()) {

    tempunit = root[D_JSON_TEMPERATURE_UNIT];
    if (tempunit) {
      snprintf_P(disp_temp, sizeof(disp_temp), PSTR("%s"), tempunit);


    }

    for (JsonObject::iterator it = root.begin(); it != root.end(); ++it) {
      JsonVariant value = it->value;
      if (value.is<JsonObject>()) {
        JsonObject& Object2 = value;
        for (JsonObject::iterator it2 = Object2.begin(); it2 != Object2.end(); ++it2) {
          JsonVariant value2 = it2->value;
          if (value2.is<JsonObject>()) {
            JsonObject& Object3 = value2;
            for (JsonObject::iterator it3 = Object3.begin(); it3 != Object3.end(); ++it3) {
              DisplayJsonValue(topic, it->key, it3->key, it3->value.as<const char*>());
            }
          } else {
            DisplayJsonValue(topic, it->key, it2->key, it2->value.as<const char*>());
          }
        }
      } else {
        DisplayJsonValue(topic, it->key, it->key, it->value.as<const char*>());
      }
    }
  }
}

void DisplayMqttSubscribe()
{







  if (Settings.display_model) {

    char stopic[TOPSZ];
    char ntopic[TOPSZ];

    ntopic[0] = '\0';
    strlcpy(stopic, Settings.mqtt_fulltopic, sizeof(stopic));
    char *tp = strtok(stopic, "/");
    while (tp != NULL) {
      if (!strcmp_P(tp, PSTR(MQTT_TOKEN_PREFIX))) {
        break;
      }
      strncat_P(ntopic, PSTR("+/"), sizeof(ntopic));
      tp = strtok(NULL, "/");
    }
    strncat(ntopic, Settings.mqtt_prefix[2], sizeof(ntopic));
    strncat_P(ntopic, PSTR("/#"), sizeof(ntopic));
    MqttSubscribe(ntopic);
    disp_subscribed = 1;
  } else {
    disp_subscribed = 0;
  }
}

boolean DisplayMqttData()
{
  if (disp_subscribed) {
    char stopic[TOPSZ];

    snprintf_P(stopic, sizeof(stopic) , PSTR("%s/"), Settings.mqtt_prefix[2]);
    char *tp = strstr(XdrvMailbox.topic, stopic);
    if (tp) {
      if (Settings.display_mode &0x04) {
        tp = tp + strlen(stopic);
        char *topic = strtok(tp, "/");
        DisplayAnalyzeJson(topic, XdrvMailbox.data);
      }
      return true;
    }
  }
  return false;
}

void DisplayLocalSensor()
{
  if ((Settings.display_mode &0x02) && (0 == tele_period)) {
    DisplayAnalyzeJson(mqtt_topic, mqtt_data);
  }
}

#endif





void DisplayInitDriver()
{
  XdspCall(FUNC_DISPLAY_INIT_DRIVER);




  if (Settings.display_model) {
    devices_present++;
    disp_device = devices_present;

#ifndef USE_DISPLAY_MODES1TO5
    Settings.display_mode = 0;
#else
    DisplayLogBufferInit();
#endif
  }
}

void DisplaySetPower()
{
  disp_power = bitRead(XdrvMailbox.index, disp_device -1);
  if (Settings.display_model) {
    XdspCall(FUNC_DISPLAY_POWER);
  }
}





boolean DisplayCommand()
{
  char command [CMDSZ];
  boolean serviced = true;
  uint8_t disp_len = strlen(D_CMND_DISPLAY);

  if (!strncasecmp_P(XdrvMailbox.topic, PSTR(D_CMND_DISPLAY), disp_len)) {
    int command_code = GetCommandCode(command, sizeof(command), XdrvMailbox.topic +disp_len, kDisplayCommands);
    if (-1 == command_code) {
      serviced = false;
    }
    else if (CMND_DISPLAY == command_code) {
      snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"" D_CMND_DISPLAY "\":{\"" D_CMND_DISP_MODEL "\":%d,\"" D_CMND_DISP_MODE "\":%d,\"" D_CMND_DISP_DIMMER "\":%d,\""
         D_CMND_DISP_SIZE "\":%d,\"" D_CMND_DISP_FONT "\":%d,\"" D_CMND_DISP_ROTATE "\":%d,\"" D_CMND_DISP_REFRESH "\":%d,\"" D_CMND_DISP_COLS "\":[%d,%d],\"" D_CMND_DISP_ROWS "\":%d}}"),
        Settings.display_model, Settings.display_mode, Settings.display_dimmer, Settings.display_size, Settings.display_font, Settings.display_rotate, Settings.display_refresh,
        Settings.display_cols[0], Settings.display_cols[1], Settings.display_rows);
    }
    else if (CMND_DISP_MODEL == command_code) {
      if ((XdrvMailbox.payload >= 0) && (XdrvMailbox.payload < DISPLAY_MAX_DRIVERS)) {
        uint8_t last_display_model = Settings.display_model;
        Settings.display_model = XdrvMailbox.payload;
        if (XdspCall(FUNC_DISPLAY_MODEL)) {
          restart_flag = 2;
        } else {
          Settings.display_model = last_display_model;
        }
      }
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_DISPLAY_COMMAND_NVALUE, command, Settings.display_model);
    }
    else if (CMND_DISP_MODE == command_code) {
#ifdef USE_DISPLAY_MODES1TO5







      if ((XdrvMailbox.payload >= 0) && (XdrvMailbox.payload <= 5)) {
        uint8_t last_display_mode = Settings.display_mode;
        Settings.display_mode = XdrvMailbox.payload;
        if (!disp_subscribed) {
          restart_flag = 2;
        } else {
          if (last_display_mode && !Settings.display_mode) {
            DisplayInit(DISPLAY_INIT_MODE);
            DisplayClear();
          } else {

            DisplayLogBufferInit();
            DisplayInit(DISPLAY_INIT_MODE);
          }
        }
      }
#endif
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_DISPLAY_COMMAND_NVALUE, command, Settings.display_mode);
    }
    else if (CMND_DISP_DIMMER == command_code) {
      if ((XdrvMailbox.payload >= 0) && (XdrvMailbox.payload <= 100)) {
        Settings.display_dimmer = ((XdrvMailbox.payload +1) * 100) / 666;
        if (Settings.display_dimmer && !(disp_power)) {
          ExecuteCommandPower(disp_device, POWER_ON, SRC_DISPLAY);
        }
        else if (!Settings.display_dimmer && disp_power) {
          ExecuteCommandPower(disp_device, POWER_OFF, SRC_DISPLAY);
        }
      }
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_DISPLAY_COMMAND_NVALUE, command, Settings.display_dimmer);
    }
    else if (CMND_DISP_SIZE == command_code) {
      if ((XdrvMailbox.payload > 0) && (XdrvMailbox.payload <= 4)) {
        Settings.display_size = XdrvMailbox.payload;
      }
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_DISPLAY_COMMAND_NVALUE, command, Settings.display_size);
    }
    else if (CMND_DISP_FONT == command_code) {
      if ((XdrvMailbox.payload > 0) && (XdrvMailbox.payload <= 4)) {
        Settings.display_font = XdrvMailbox.payload;
      }
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_DISPLAY_COMMAND_NVALUE, command, Settings.display_font);
    }
    else if (CMND_DISP_ROTATE == command_code) {
      if ((XdrvMailbox.payload >= 0) && (XdrvMailbox.payload < 4)) {
        if (Settings.display_rotate != XdrvMailbox.payload) {
# 990 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xdrv_13_display.ino"
          Settings.display_rotate = XdrvMailbox.payload;
          DisplayInit(DISPLAY_INIT_MODE);
#ifdef USE_DISPLAY_MODES1TO5
          DisplayLogBufferInit();
#endif
        }
      }
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_DISPLAY_COMMAND_NVALUE, command, Settings.display_rotate);
    }
    else if (CMND_DISP_TEXT == command_code) {
      mqtt_data[0] = '\0';
      if (disp_device && XdrvMailbox.data_len > 0) {
#ifndef USE_DISPLAY_MODES1TO5
        DisplayText();
#else
        if (!Settings.display_mode) {
          DisplayText();
        } else {
          DisplayLogBufferAdd(XdrvMailbox.data);
        }
#endif
      } else {
        snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("No Text"));
      }
      if (mqtt_data[0] == '\0') {
        snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_DISPLAY_COMMAND_VALUE, command, XdrvMailbox.data);
      }
    }
    else if ((CMND_DISP_ADDRESS == command_code) && (XdrvMailbox.index > 0) && (XdrvMailbox.index <= 8)) {
      if ((XdrvMailbox.payload >= 0) && (XdrvMailbox.payload <= 255)) {
        Settings.display_address[XdrvMailbox.index -1] = XdrvMailbox.payload;
      }
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_DISPLAY_COMMAND_INDEX_NVALUE, command, XdrvMailbox.index, Settings.display_address[XdrvMailbox.index -1]);
    }
    else if (CMND_DISP_REFRESH == command_code) {
      if ((XdrvMailbox.payload >= 1) && (XdrvMailbox.payload <= 7)) {
        Settings.display_refresh = XdrvMailbox.payload;
      }
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_DISPLAY_COMMAND_NVALUE, command, Settings.display_refresh);
    }
    else if ((CMND_DISP_COLS == command_code) && (XdrvMailbox.index > 0) && (XdrvMailbox.index <= 2)) {
      if ((XdrvMailbox.payload > 0) && (XdrvMailbox.payload <= DISPLAY_MAX_COLS)) {
        Settings.display_cols[XdrvMailbox.index -1] = XdrvMailbox.payload;
#ifdef USE_DISPLAY_MODES1TO5
        if (1 == XdrvMailbox.index) {
          DisplayLogBufferInit();
          DisplayReAllocScreenBuffer();
        }
#endif
      }
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_DISPLAY_COMMAND_INDEX_NVALUE, command, XdrvMailbox.index, Settings.display_cols[XdrvMailbox.index -1]);
    }
    else if (CMND_DISP_ROWS == command_code) {
      if ((XdrvMailbox.payload > 0) && (XdrvMailbox.payload <= DISPLAY_MAX_ROWS)) {
        Settings.display_rows = XdrvMailbox.payload;
#ifdef USE_DISPLAY_MODES1TO5
        DisplayLogBufferInit();
        DisplayReAllocScreenBuffer();
#endif
      }
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_DISPLAY_COMMAND_NVALUE, command, Settings.display_rows);
    }
    else serviced = false;
  }
  else serviced = false;

  return serviced;
}





#define XDRV_13 

boolean Xdrv13(byte function)
{
  boolean result = false;

  if ((i2c_flg || spi_flg) && XdspPresent()) {
    switch (function) {
      case FUNC_PRE_INIT:
        DisplayInitDriver();
        break;
      case FUNC_EVERY_50_MSECOND:
        if (Settings.display_model) { XdspCall(FUNC_DISPLAY_EVERY_50_MSECOND); }
        break;
      case FUNC_COMMAND:
        result = DisplayCommand();
        break;
      case FUNC_SET_POWER:
        DisplaySetPower();
        break;
#ifdef USE_DISPLAY_MODES1TO5
      case FUNC_EVERY_SECOND:
        if (Settings.display_model && Settings.display_mode) { XdspCall(FUNC_DISPLAY_EVERY_SECOND); }
        break;
      case FUNC_MQTT_SUBSCRIBE:
        DisplayMqttSubscribe();
        break;
      case FUNC_MQTT_DATA:
        result = DisplayMqttData();
        break;
      case FUNC_SHOW_SENSOR:
        DisplayLocalSensor();
        break;
#endif
    }
  }
  return result;
}

#endif
#endif
# 1 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xdrv_14_mp3.ino"
# 64 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xdrv_14_mp3.ino"
#ifdef USE_MP3_PLAYER





#include <TasmotaSerial.h>

TasmotaSerial *MP3Player;





#define D_CMND_MP3 "MP3"

const char S_JSON_MP3_COMMAND_NVALUE[] PROGMEM = "{\"" D_CMND_MP3 "%s\":%d}";
const char S_JSON_MP3_COMMAND[] PROGMEM = "{\"" D_CMND_MP3 "%s\"}";
const char kMP3_Commands[] PROGMEM = "Track|Play|Pause|Stop|Volume|EQ|Device|Reset|DAC";





enum MP3_Commands {
  CMND_MP3_TRACK,
  CMND_MP3_PLAY,
  CMND_MP3_PAUSE,
  CMND_MP3_STOP,
  CMND_MP3_VOLUME,
  CMND_MP3_EQ,
  CMND_MP3_DEVICE,
  CMND_MP3_RESET,
  CMND_MP3_DAC };






#define MP3_CMD_RESET_VALUE 0

#define MP3_CMD_TRACK 0x03
#define MP3_CMD_PLAY 0x0d
#define MP3_CMD_PAUSE 0x0e
#define MP3_CMD_STOP 0x16
#define MP3_CMD_VOLUME 0x06
#define MP3_CMD_EQ 0x07
#define MP3_CMD_DEVICE 0x09
#define MP3_CMD_RESET 0x0C
#define MP3_CMD_DAC 0x1A






uint16_t MP3_Checksum(uint8_t *array)
{
  uint16_t checksum = 0;
  for (uint8_t i = 0; i < 6; i++) {
    checksum += array[i];
  }
  checksum = checksum^0xffff;
  return (checksum+1);
}






void MP3PlayerInit(void) {
  MP3Player = new TasmotaSerial(-1, pin[GPIO_MP3_DFR562]);

  if (MP3Player->begin(9600)) {
    MP3Player->flush();
    delay(1000);
    MP3_CMD(MP3_CMD_RESET, MP3_CMD_RESET_VALUE);
    delay(3000);
    MP3_CMD(MP3_CMD_VOLUME, MP3_VOLUME);
  }
  return;
}
# 157 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xdrv_14_mp3.ino"
void MP3_CMD(uint8_t mp3cmd,uint16_t val) {
  uint8_t i = 0;
  uint8_t cmd[10] = {0x7e,0xff,6,0,0,0,0,0,0,0xef};
  cmd[3] = mp3cmd;
  cmd[4] = 0;
  cmd[5] = val>>8;
  cmd[6] = val;
  uint16_t chks = MP3_Checksum(&cmd[1]);
  cmd[7] = chks>>8;
  cmd[8] = chks;
  MP3Player->write(cmd, sizeof(cmd));
  delay(1000);
  if (mp3cmd == MP3_CMD_RESET) {
    MP3_CMD(MP3_CMD_VOLUME, MP3_VOLUME);
  }
  return;
}





boolean MP3PlayerCmd(void) {
  char command[CMDSZ];
  boolean serviced = true;
  uint8_t disp_len = strlen(D_CMND_MP3);

  if (!strncasecmp_P(XdrvMailbox.topic, PSTR(D_CMND_MP3), disp_len)) {
    int command_code = GetCommandCode(command, sizeof(command), XdrvMailbox.topic + disp_len, kMP3_Commands);

    switch (command_code) {
      case CMND_MP3_TRACK:
      case CMND_MP3_VOLUME:
      case CMND_MP3_EQ:
      case CMND_MP3_DEVICE:
      case CMND_MP3_DAC:

        if (XdrvMailbox.data_len > 0) {
          if (command_code == CMND_MP3_TRACK) { MP3_CMD(MP3_CMD_TRACK, XdrvMailbox.payload); }
          if (command_code == CMND_MP3_VOLUME) { MP3_CMD(MP3_CMD_VOLUME, XdrvMailbox.payload * 30 / 100); }
          if (command_code == CMND_MP3_EQ) { MP3_CMD(MP3_CMD_EQ, XdrvMailbox.payload); }
          if (command_code == CMND_MP3_DEVICE) { MP3_CMD(MP3_CMD_DEVICE, XdrvMailbox.payload); }
          if (command_code == CMND_MP3_DAC) { MP3_CMD(MP3_CMD_DAC, XdrvMailbox.payload); }
        }
        snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_MP3_COMMAND_NVALUE, command, XdrvMailbox.payload);
        break;
      case CMND_MP3_PLAY:
      case CMND_MP3_PAUSE:
      case CMND_MP3_STOP:
      case CMND_MP3_RESET:

        if (command_code == CMND_MP3_PLAY) { MP3_CMD(MP3_CMD_PLAY, 0); }
        if (command_code == CMND_MP3_PAUSE) { MP3_CMD(MP3_CMD_PAUSE, 0); }
        if (command_code == CMND_MP3_STOP) { MP3_CMD(MP3_CMD_STOP, 0); }
        if (command_code == CMND_MP3_RESET) { MP3_CMD(MP3_CMD_RESET, 0); }
        snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_MP3_COMMAND, command, XdrvMailbox.payload);
        break;
      default:

       serviced = false;
     break;
    }
  }
  return serviced;
}





#define XDRV_14 

boolean Xdrv14(byte function)
{
  boolean result = false;

  switch (function) {
    case FUNC_PRE_INIT:
      MP3PlayerInit();
      break;
    case FUNC_COMMAND:
      result = MP3PlayerCmd();
      break;
  }
  return result;
}

#endif
# 1 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xdrv_15_pca9685.ino"
# 20 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xdrv_15_pca9685.ino"
#ifdef USE_I2C
#ifdef USE_PCA9685

#define XDRV_15 15

#define PCA9685_REG_MODE1 0x00
#define PCA9685_REG_LED0_ON_L 0x06
#define PCA9685_REG_PRE_SCALE 0xFE

#ifndef USE_PCA9685_FREQ
  #define USE_PCA9685_FREQ 50
#endif

uint8_t pca9685_detected = 0;
uint16_t pca9685_freq = USE_PCA9685_FREQ;
uint16_t pca9685_pin_pwm_value[16];

void PCA9685_Detect(void)
{
  if (pca9685_detected) { return; }

  uint8_t buffer;

  if (I2cValidRead8(&buffer, USE_PCA9685_ADDR, PCA9685_REG_MODE1)) {
    I2cWrite8(USE_PCA9685_ADDR, PCA9685_REG_MODE1, 0x20);
    if (I2cValidRead8(&buffer, USE_PCA9685_ADDR, PCA9685_REG_MODE1)) {
      if (0x20 == buffer) {
        pca9685_detected = 1;
        snprintf_P(log_data, sizeof(log_data), S_LOG_I2C_FOUND_AT, "PCA9685", USE_PCA9685_ADDR);
        AddLog(LOG_LEVEL_DEBUG);
        PCA9685_Reset();
      }
    }
  }
}

void PCA9685_Reset(void)
{
  I2cWrite8(USE_PCA9685_ADDR, PCA9685_REG_MODE1, 0x80);
  PCA9685_SetPWMfreq(USE_PCA9685_FREQ);
  for (uint8_t pin=0;pin<16;pin++) {
    PCA9685_SetPWM(pin,0,false);
    pca9685_pin_pwm_value[pin] = 0;
  }
  snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"PCA9685\":{\"RESET\":\"OK\"}}"));
}

void PCA9685_SetPWMfreq(double freq) {




  if (freq > 23 && freq < 1527) {
   pca9685_freq=freq;
  } else {
   pca9685_freq=50;
  }
  uint8_t pre_scale_osc = round(25000000/(4096*pca9685_freq))-1;
  if (1526 == pca9685_freq) pre_scale_osc=0xFF;
  uint8_t current_mode1 = I2cRead8(USE_PCA9685_ADDR, PCA9685_REG_MODE1);
  uint8_t sleep_mode1 = (current_mode1&0x7F) | 0x10;
  I2cWrite8(USE_PCA9685_ADDR, PCA9685_REG_MODE1, sleep_mode1);
  I2cWrite8(USE_PCA9685_ADDR, PCA9685_REG_PRE_SCALE, pre_scale_osc);
  I2cWrite8(USE_PCA9685_ADDR, PCA9685_REG_MODE1, current_mode1 | 0xA0);
}

void PCA9685_SetPWM_Reg(uint8_t pin, uint16_t on, uint16_t off) {
  uint8_t led_reg = PCA9685_REG_LED0_ON_L + 4 * pin;
  uint32_t led_data = 0;
  I2cWrite8(USE_PCA9685_ADDR, led_reg, on);
  I2cWrite8(USE_PCA9685_ADDR, led_reg+1, (on >> 8));
  I2cWrite8(USE_PCA9685_ADDR, led_reg+2, off);
  I2cWrite8(USE_PCA9685_ADDR, led_reg+3, (off >> 8));
}

void PCA9685_SetPWM(uint8_t pin, uint16_t pwm, bool inverted) {
  if (4096 == pwm) {
    PCA9685_SetPWM_Reg(pin, 4096, 0);
  } else {
    PCA9685_SetPWM_Reg(pin, 0, pwm);
  }
  pca9685_pin_pwm_value[pin] = pwm;
}

bool PCA9685_Command(void)
{
  boolean serviced = true;
  boolean validpin = false;
  uint8_t paramcount = 0;
  if (XdrvMailbox.data_len > 0) {
    paramcount=1;
  } else {
    serviced = false;
    return serviced;
  }
  char sub_string[XdrvMailbox.data_len];
  for (uint8_t ca=0;ca<XdrvMailbox.data_len;ca++) {
    if ((' ' == XdrvMailbox.data[ca]) || ('=' == XdrvMailbox.data[ca])) { XdrvMailbox.data[ca] = ','; }
    if (',' == XdrvMailbox.data[ca]) { paramcount++; }
  }
  UpperCase(XdrvMailbox.data,XdrvMailbox.data);

  if (!strcmp(subStr(sub_string, XdrvMailbox.data, ",", 1),"RESET")) { PCA9685_Reset(); return serviced; }

  if (!strcmp(subStr(sub_string, XdrvMailbox.data, ",", 1),"STATUS")) { PCA9685_OutputTelemetry(false); return serviced; }

  if (!strcmp(subStr(sub_string, XdrvMailbox.data, ",", 1),"PWMF")) {
    if (paramcount > 1) {
      uint16_t new_freq = atoi(subStr(sub_string, XdrvMailbox.data, ",", 2));
      if ((new_freq >= 24) && (new_freq <= 1526)) {
        PCA9685_SetPWMfreq(new_freq);
        snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"PCA9685\":{\"PWMF\":%i, \"Result\":\"OK\"}}"),new_freq);
        return serviced;
      }
    } else {
      snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"PCA9685\":{\"PWMF\":%i}}"),pca9685_freq);
      return serviced;
    }
  }
  if (!strcmp(subStr(sub_string, XdrvMailbox.data, ",", 1),"PWM")) {
    if (paramcount > 1) {
      uint8_t pin = atoi(subStr(sub_string, XdrvMailbox.data, ",", 2));
      if (paramcount > 2) {
        if (!strcmp(subStr(sub_string, XdrvMailbox.data, ",", 3), "ON")) {
          PCA9685_SetPWM(pin, 4096, false);
          snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"PCA9685\":{\"PIN\":%i,\"PWM\":%i}}"),pin,4096);
          serviced = true;
          return serviced;
        }
        if (!strcmp(subStr(sub_string, XdrvMailbox.data, ",", 3), "OFF")) {
          PCA9685_SetPWM(pin, 0, false);
          snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"PCA9685\":{\"PIN\":%i,\"PWM\":%i}}"),pin,0);
          serviced = true;
          return serviced;
        }
        uint16_t pwm = atoi(subStr(sub_string, XdrvMailbox.data, ",", 3));
        if ((pin >= 0 && pin <= 15) && (pwm >= 0 && pwm <= 4096)) {
          PCA9685_SetPWM(pin, pwm, false);
          snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"PCA9685\":{\"PIN\":%i,\"PWM\":%i}}"),pin,pwm);
          serviced = true;
          return serviced;
        }
      }
    }
  }
  return serviced;
}

void PCA9685_OutputTelemetry(bool telemetry) {
  if (0 == pca9685_detected) { return; }
  snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"" D_JSON_TIME "\":\"%s\",\"PCA9685\": {"), GetDateAndTime(DT_LOCAL).c_str());
  snprintf_P(mqtt_data,sizeof(mqtt_data), PSTR("%s\"PWM_FREQ\":%i,"),mqtt_data,pca9685_freq);
  for (uint8_t pin=0;pin<16;pin++) {
    snprintf_P(mqtt_data,sizeof(mqtt_data), PSTR("%s\"PWM%i\":%i,"),mqtt_data,pin,pca9685_pin_pwm_value[pin]);
  }
  snprintf_P(mqtt_data,sizeof(mqtt_data),PSTR("%s\"END\":1}}"),mqtt_data);
  if (telemetry) {
    MqttPublishPrefixTopic_P(TELE, PSTR(D_RSLT_SENSOR), Settings.flag.mqtt_sensor_retain);
  }
}

boolean Xdrv15(byte function)
{
  boolean result = false;

  if (i2c_flg) {
    switch (function) {
      case FUNC_EVERY_SECOND:
        PCA9685_Detect();
        if (tele_period == 0) {
          PCA9685_OutputTelemetry(true);
        }
        break;
      case FUNC_COMMAND:
        if (XDRV_15 == XdrvMailbox.index) {
          PCA9685_Command();
        }
        break;
      default:
        break;
    }
  }
  return result;
}

#endif
#endif
# 1 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xdrv_16_tuyadimmer.ino"
# 20 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xdrv_16_tuyadimmer.ino"
#ifdef USE_TUYA_DIMMER

#ifndef TUYA_DIMMER_ID
#define TUYA_DIMMER_ID 3
#endif
#define TUYA_BUFFER_SIZE 256

#include <TasmotaSerial.h>

TasmotaSerial *TuyaSerial = nullptr;

uint8_t tuya_new_dim = 0;
boolean tuya_ignore_dim = false;
uint8_t tuya_cmd_status = 0;
uint8_t tuya_cmd_checksum = 0;
uint8_t tuya_data_len = 0;
bool tuya_wifi_state = false;

char tuya_buffer[TUYA_BUFFER_SIZE];
int tuya_byte_counter = 0;

boolean TuyaSetPower()
{
  boolean status = false;

  uint8_t rpower = XdrvMailbox.index;
  int16_t source = XdrvMailbox.payload;

  if (source != SRC_SWITCH && TuyaSerial) {

    snprintf_P(log_data, sizeof(log_data), PSTR("TYA: SetDevicePower.rpower=%d"), rpower);
    AddLog(LOG_LEVEL_DEBUG);

    TuyaSerial->write((uint8_t)0x55);
    TuyaSerial->write((uint8_t)0xAA);
    TuyaSerial->write((uint8_t)0x00);
    TuyaSerial->write((uint8_t)0x06);
    TuyaSerial->write((uint8_t)0x00);
    TuyaSerial->write((uint8_t)0x05);
    TuyaSerial->write((uint8_t)0x01);
    TuyaSerial->write((uint8_t)0x01);
    TuyaSerial->write((uint8_t)0x00);
    TuyaSerial->write((uint8_t)0x01);
    TuyaSerial->write((uint8_t)rpower);
    TuyaSerial->write((uint8_t)0x0D + rpower);
    TuyaSerial->flush();

    status = true;
  }
  return status;
}

void LightSerialDuty(uint8_t duty)
{
  if (duty > 0 && !tuya_ignore_dim && TuyaSerial) {
    if (duty < 25) {
      duty = 25;
    }
    TuyaSerial->write((uint8_t)0x55);
    TuyaSerial->write((uint8_t)0xAA);
    TuyaSerial->write((uint8_t)0x00);
    TuyaSerial->write((uint8_t)0x06);
    TuyaSerial->write((uint8_t)0x00);
    TuyaSerial->write((uint8_t)0x08);
    TuyaSerial->write((uint8_t)Settings.param[P_TUYA_DIMMER_ID]);
    TuyaSerial->write((uint8_t)0x02);
    TuyaSerial->write((uint8_t)0x00);
    TuyaSerial->write((uint8_t)0x04);
    TuyaSerial->write((uint8_t)0x00);
    TuyaSerial->write((uint8_t)0x00);
    TuyaSerial->write((uint8_t)0x00);
    TuyaSerial->write((uint8_t) duty );
    TuyaSerial->write((uint8_t) byte(Settings.param[P_TUYA_DIMMER_ID] + 19 + duty) );
    TuyaSerial->flush();

    snprintf_P(log_data, sizeof(log_data), PSTR( "TYA: Send Serial Packet Dim Value=%d (id=%d)"), duty, Settings.param[P_TUYA_DIMMER_ID]);
    AddLog(LOG_LEVEL_DEBUG);

  } else {
    tuya_ignore_dim = false;

    snprintf_P(log_data, sizeof(log_data), PSTR( "TYA: Send Dim Level skipped due to 0 or already set. Value=%d"), duty);
    AddLog(LOG_LEVEL_DEBUG);

  }
}

void TuyaPacketProcess()
{
  char scmnd[20];

  snprintf_P(log_data, sizeof(log_data), PSTR("TYA: Packet Size=%d"), tuya_byte_counter);
  AddLog(LOG_LEVEL_DEBUG);

  if (tuya_byte_counter == 7 && tuya_buffer[3] == 14 ) {
    AddLog_P(LOG_LEVEL_DEBUG, PSTR("TYA: Heartbeat"));
  }
  else if (tuya_byte_counter == 12 && tuya_buffer[3] == 7 && tuya_buffer[5] == 5) {

    snprintf_P(log_data, sizeof(log_data),PSTR("TYA: Rcvd - %s State"),tuya_buffer[10]?"On":"Off");
    AddLog(LOG_LEVEL_DEBUG);

    if((power || Settings.light_dimmer > 0) && (power != tuya_buffer[10])) {
      ExecuteCommandPower(1, tuya_buffer[10], SRC_SWITCH);
    }
  }
  else if (tuya_byte_counter == 15 && tuya_buffer[3] == 7 && tuya_buffer[5] == 8) {

    snprintf_P(log_data, sizeof(log_data), PSTR("TYA: Rcvd Dim State=%d"), tuya_buffer[13]);
    AddLog(LOG_LEVEL_DEBUG);

    tuya_new_dim = round(tuya_buffer[13] * (100. / 255.));
    if((power) && (tuya_new_dim > 0) && (abs(tuya_new_dim - Settings.light_dimmer) > 2)) {

      snprintf_P(log_data, sizeof(log_data), PSTR("TYA: Send CMND_DIMMER=%d"), tuya_new_dim );
      AddLog(LOG_LEVEL_DEBUG);

      snprintf_P(scmnd, sizeof(scmnd), PSTR(D_CMND_DIMMER " %d"), tuya_new_dim );

      snprintf_P(log_data, sizeof(log_data), PSTR("TYA: Send CMND_DIMMER_STR=%s"), scmnd );
      AddLog(LOG_LEVEL_DEBUG);

      tuya_ignore_dim = true;
      ExecuteCommand(scmnd, SRC_SWITCH);
    }
  }
  else if (tuya_byte_counter == 8 && tuya_buffer[3] == 5 && tuya_buffer[5] == 1 && tuya_buffer[7] == 5 ) {

    AddLog_P(LOG_LEVEL_DEBUG, PSTR("TYA: WiFi Reset Rcvd"));
    TuyaResetWifi();
  }
  else if (tuya_byte_counter == 7 && tuya_buffer[3] == 3 && tuya_buffer[6] == 2) {

    AddLog_P(LOG_LEVEL_DEBUG, PSTR("TYA: WiFi LED reset ACK"));
    tuya_wifi_state = true;
  }
}

void TuyaSerialInput()
{
  while (TuyaSerial->available()) {
    yield();
    byte serial_in_byte = TuyaSerial->read();




    if (serial_in_byte == 0x55) {
      tuya_cmd_status = 1;
      tuya_buffer[tuya_byte_counter++] = serial_in_byte;
      tuya_cmd_checksum += serial_in_byte;
    }
    else if (tuya_cmd_status == 1 && serial_in_byte == 0xAA){
      tuya_cmd_status = 2;

      AddLog_P(LOG_LEVEL_DEBUG, PSTR("TYA: 0x55AA Packet Start"));

      tuya_byte_counter = 0;
      tuya_buffer[tuya_byte_counter++] = 0x55;
      tuya_buffer[tuya_byte_counter++] = 0xAA;
      tuya_cmd_checksum = 0xFF;
    }
    else if (tuya_cmd_status == 2){
      if(tuya_byte_counter == 5){
        tuya_cmd_status = 3;
        tuya_data_len = serial_in_byte;
      }
      tuya_cmd_checksum += serial_in_byte;
      tuya_buffer[tuya_byte_counter++] = serial_in_byte;
    }
    else if ((tuya_cmd_status == 3) && (tuya_byte_counter == (6 + tuya_data_len)) && (tuya_cmd_checksum == serial_in_byte)){
      tuya_buffer[tuya_byte_counter++] = serial_in_byte;

      snprintf_P(log_data, sizeof(log_data), PSTR("TYA: 0x55 Packet End: \""));
      for (int i = 0; i < tuya_byte_counter; i++) {
        snprintf_P(log_data, sizeof(log_data), PSTR("%s%02x"), log_data, tuya_buffer[i]);
      }
      snprintf_P(log_data, sizeof(log_data), PSTR("%s\""), log_data);
      AddLog(LOG_LEVEL_DEBUG);

      TuyaPacketProcess();
      tuya_byte_counter = 0;
      tuya_cmd_status = 0;
      tuya_cmd_checksum = 0;
      tuya_data_len = 0;
    }
    else if(tuya_byte_counter < TUYA_BUFFER_SIZE -1) {
      tuya_buffer[tuya_byte_counter++] = serial_in_byte;
      tuya_cmd_checksum += serial_in_byte;
    } else {
      tuya_byte_counter = 0;
      tuya_cmd_status = 0;
      tuya_cmd_checksum = 0;
      tuya_data_len = 0;
    }
  }
}

boolean TuyaModuleSelected()
{
  if (!(pin[GPIO_TUYA_RX] < 99) || !(pin[GPIO_TUYA_TX] < 99)) {
    pin[GPIO_TUYA_RX] = 1;
    pin[GPIO_TUYA_TX] = 3;
  }
  light_type = LT_SERIAL;
  return true;
}

void TuyaResetWifiLed(){
    snprintf_P(log_data, sizeof(log_data), "TYA: Reset WiFi LED");
    AddLog(LOG_LEVEL_DEBUG);

    TuyaSerial->write((uint8_t)0x55);
    TuyaSerial->write((uint8_t)0xAA);
    TuyaSerial->write((uint8_t)0x00);
    TuyaSerial->write((uint8_t)0x03);
    TuyaSerial->write((uint8_t)0x00);
    TuyaSerial->write((uint8_t)0x01);
    TuyaSerial->write((uint8_t)0x03);
    TuyaSerial->write((uint8_t)0x06);
    TuyaSerial->flush();
}

void TuyaInit()
{
  if (!Settings.param[P_TUYA_DIMMER_ID]) {
    Settings.param[P_TUYA_DIMMER_ID] = TUYA_DIMMER_ID;
  }
  TuyaSerial = new TasmotaSerial(pin[GPIO_TUYA_RX], pin[GPIO_TUYA_TX], 1);
  if (TuyaSerial->begin(9600)) {
    if (TuyaSerial->hardwareSerial()) { ClaimSerial(); }


    snprintf_P(log_data, sizeof(log_data), "TYA: Request MCU state");
    AddLog(LOG_LEVEL_DEBUG);

    TuyaSerial->write((uint8_t)0x55);
    TuyaSerial->write((uint8_t)0xAA);
    TuyaSerial->write((uint8_t)0x00);
    TuyaSerial->write((uint8_t)0x08);
    TuyaSerial->write((uint8_t)0x00);
    TuyaSerial->write((uint8_t)0x00);
    TuyaSerial->write((uint8_t)0x07);
    TuyaSerial->flush();
  }
}

void TuyaResetWifi()
{
  if (!Settings.flag.button_restrict) {
    char scmnd[20];
    snprintf_P(scmnd, sizeof(scmnd), D_CMND_WIFICONFIG " %d", 2);
    ExecuteCommand(scmnd, SRC_BUTTON);
    tuya_wifi_state = false;
  }
}

boolean TuyaButtonPressed()
{
  if ((PRESSED == XdrvMailbox.payload) && (NOT_PRESSED == lastbutton[XdrvMailbox.index])) {

    snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_APPLICATION D_BUTTON "%d " D_LEVEL_10), XdrvMailbox.index +1);
    AddLog(LOG_LEVEL_DEBUG);
    TuyaResetWifi();

  }
  return true;
}





#define XDRV_16 

boolean Xdrv16(byte function)
{
  boolean result = false;

  if (TUYA_DIMMER == Settings.module) {
    switch (function) {
      case FUNC_MODULE_INIT:
        result = TuyaModuleSelected();
        break;
      case FUNC_INIT:
        TuyaInit();
        break;
      case FUNC_LOOP:
        if (TuyaSerial) { TuyaSerialInput(); }
        break;
      case FUNC_SET_DEVICE_POWER:
        result = TuyaSetPower();
        break;
      case FUNC_BUTTON_PRESSED:
        result = TuyaButtonPressed();
        break;
      case FUNC_EVERY_SECOND:
        if(TuyaSerial && !tuya_wifi_state) { TuyaResetWifiLed(); }
        break;
    }
  }
  return result;
}

#endif
# 1 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xdrv_17_rcswitch.ino"
# 20 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xdrv_17_rcswitch.ino"
#ifdef USE_RC_SWITCH




#define D_JSON_RF_PROTOCOL "Protocol"
#define D_JSON_RF_BITS "Bits"
#define D_JSON_RF_DATA "Data"

#define D_CMND_RFSEND "RFSend"
#define D_JSON_RF_PULSE "Pulse"
#define D_JSON_RF_REPEAT "Repeat"

#include <RCSwitch.h>

RCSwitch mySwitch = RCSwitch();

#define RF_TIME_AVOID_DUPLICATE 1000

uint32_t rf_lasttime = 0;

void RfReceiveCheck()
{
  if (mySwitch.available()) {

    unsigned long data = mySwitch.getReceivedValue();
    unsigned int bits = mySwitch.getReceivedBitlength();
    int protocol = mySwitch.getReceivedProtocol();
    int delay = mySwitch.getReceivedDelay();

    snprintf_P(log_data, sizeof(log_data), PSTR("RFR: Data %lX (%u), Bits %d, Protocol %d, Delay %d"), data, data, bits, protocol, delay);
    AddLog(LOG_LEVEL_DEBUG);

    uint32_t now = millis();
    if ((now - rf_lasttime > RF_TIME_AVOID_DUPLICATE) && (data > 0)) {
      rf_lasttime = now;

      char stemp[16];
      if (Settings.flag.rf_receive_decimal) {
        snprintf_P(stemp, sizeof(stemp), PSTR("%u"), (uint32_t)data);
      } else {
        snprintf_P(stemp, sizeof(stemp), PSTR("\"%lX\""), (uint32_t)data);
      }
      snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"" D_JSON_RFRECEIVED "\":{\"" D_JSON_RF_DATA "\":%s,\"" D_JSON_RF_BITS "\":%d,\"" D_JSON_RF_PROTOCOL "\":%d}}"),
          stemp, bits, protocol);
      MqttPublishPrefixTopic_P(RESULT_OR_TELE, PSTR(D_JSON_RFRECEIVED));
      XdrvRulesProcess();
#ifdef USE_DOMOTICZ
      DomoticzSensor(DZ_COUNT, data);
#endif
    }
    mySwitch.resetAvailable();
  }
}

void RfInit()
{
  if (pin[GPIO_RFSEND] < 99) {
    mySwitch.enableTransmit(pin[GPIO_RFSEND]);
  }
  if (pin[GPIO_RFRECV] < 99) {
    mySwitch.enableReceive(pin[GPIO_RFRECV]);
  }
}





boolean RfSendCommand()
{
  boolean serviced = true;
  boolean error = false;

  if (!strcasecmp_P(XdrvMailbox.topic, PSTR(D_CMND_RFSEND))) {
    if (XdrvMailbox.data_len) {
      unsigned long data = 0;
      unsigned int bits = 24;
      int protocol = 1;
      int repeat = 10;
      int pulse = 350;

      char dataBufUc[XdrvMailbox.data_len];
      UpperCase(dataBufUc, XdrvMailbox.data);
      StaticJsonBuffer<150> jsonBuf;
      JsonObject &root = jsonBuf.parseObject(dataBufUc);
      if (root.success()) {

        char parm_uc[10];
        data = strtoul(root[UpperCase_P(parm_uc, PSTR(D_JSON_RF_DATA))], NULL, 0);
        bits = root[UpperCase_P(parm_uc, PSTR(D_JSON_RF_BITS))];
        protocol = root[UpperCase_P(parm_uc, PSTR(D_JSON_RF_PROTOCOL))];
        repeat = root[UpperCase_P(parm_uc, PSTR(D_JSON_RF_REPEAT))];
        pulse = root[UpperCase_P(parm_uc, PSTR(D_JSON_RF_PULSE))];
      } else {

        char *p;
        byte i = 0;
        for (char *str = strtok_r(XdrvMailbox.data, ", ", &p); str && i < 5; str = strtok_r(NULL, ", ", &p)) {
          switch (i++) {
          case 0:
            data = strtoul(str, NULL, 0);
            break;
          case 1:
            bits = atoi(str);
            break;
          case 2:
            protocol = atoi(str);
            break;
          case 3:
            repeat = atoi(str);
            break;
          case 4:
            pulse = atoi(str);
          }
        }
      }

      if (!protocol) { protocol = 1; }
      mySwitch.setProtocol(protocol);
      if (!pulse) { pulse = 350; }
      mySwitch.setPulseLength(pulse);
      if (!repeat) { repeat = 10; }
      mySwitch.setRepeatTransmit(repeat);
      if (!bits) { bits = 24; }
      if (data) {
        mySwitch.send(data, bits);
        snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"" D_CMND_RFSEND "\":\"" D_JSON_DONE "\"}"));
      } else {
        error = true;
      }
    } else {
      error = true;
    }
    if (error) {
      snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"" D_CMND_RFSEND "\":\"" D_JSON_NO " " D_JSON_RF_DATA ", " D_JSON_RF_BITS ", " D_JSON_RF_PROTOCOL ", " D_JSON_RF_REPEAT " " D_JSON_OR " " D_JSON_RF_PULSE "\"}"));
    }
  }
  else serviced = false;

  return serviced;
}





#define XDRV_17 

boolean Xdrv17(byte function)
{
  boolean result = false;

  if ((pin[GPIO_RFSEND] < 99) || (pin[GPIO_RFRECV] < 99)) {
    switch (function) {
      case FUNC_INIT:
        RfInit();
        break;
      case FUNC_EVERY_50_MSECOND:
        if (pin[GPIO_RFRECV] < 99) {
          RfReceiveCheck();
        }
        break;
      case FUNC_COMMAND:
        if (pin[GPIO_RFSEND] < 99) {
          result = RfSendCommand();
        }
        break;
    }
  }
  return result;
}

#endif
# 1 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xdrv_interface.ino"
# 20 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xdrv_interface.ino"
boolean (* const xdrv_func_ptr[])(byte) PROGMEM = {
#ifdef XDRV_01
  &Xdrv01,
#endif

#ifdef XDRV_02
  &Xdrv02,
#endif

#ifdef XDRV_03
  &Xdrv03,
#endif

#ifdef XDRV_04
  &Xdrv04,
#endif

#ifdef XDRV_05
  &Xdrv05,
#endif

#ifdef XDRV_06
  &Xdrv06,
#endif

#ifdef XDRV_07
  &Xdrv07,
#endif

#ifdef XDRV_08
  &Xdrv08,
#endif

#ifdef XDRV_09
  &Xdrv09,
#endif

#ifdef XDRV_10
  &Xdrv10,
#endif

#ifdef XDRV_11
  &Xdrv11,
#endif

#ifdef XDRV_12
  &Xdrv12,
#endif

#ifdef XDRV_13
  &Xdrv13,
#endif

#ifdef XDRV_14
  &Xdrv14,
#endif

#ifdef XDRV_15
  &Xdrv15,
#endif

#ifdef XDRV_16
  &Xdrv16,
#endif

#ifdef XDRV_17
  &Xdrv17,
#endif

#ifdef XDRV_18
  &Xdrv18,
#endif

#ifdef XDRV_19
  &Xdrv19,
#endif

#ifdef XDRV_20
  &Xdrv20,
#endif

#ifdef XDRV_21
  &Xdrv21,
#endif

#ifdef XDRV_22
  &Xdrv22,
#endif

#ifdef XDRV_23
  &Xdrv23,
#endif

#ifdef XDRV_24
  &Xdrv24,
#endif

#ifdef XDRV_25
  &Xdrv25,
#endif

#ifdef XDRV_26
  &Xdrv26,
#endif

#ifdef XDRV_27
  &Xdrv27,
#endif

#ifdef XDRV_28
  &Xdrv28,
#endif

#ifdef XDRV_29
  &Xdrv29,
#endif

#ifdef XDRV_30
  &Xdrv30,
#endif

#ifdef XDRV_31
  &Xdrv31,
#endif

#ifdef XDRV_32
  &Xdrv32,
#endif



#ifdef XDRV_91
  &Xdrv91,
#endif

#ifdef XDRV_92
  &Xdrv92,
#endif

#ifdef XDRV_93
  &Xdrv93,
#endif

#ifdef XDRV_94
  &Xdrv94,
#endif

#ifdef XDRV_95
  &Xdrv95,
#endif

#ifdef XDRV_96
  &Xdrv96,
#endif

#ifdef XDRV_97
  &Xdrv97,
#endif

#ifdef XDRV_98
  &Xdrv98,
#endif

#ifdef XDRV_99
  &Xdrv99
#endif
};

const uint8_t xdrv_present = sizeof(xdrv_func_ptr) / sizeof(xdrv_func_ptr[0]);

boolean XdrvCommand(uint8_t grpflg, char *type, uint16_t index, char *dataBuf, uint16_t data_len, int16_t payload, uint16_t payload16)
{

  XdrvMailbox.index = index;
  XdrvMailbox.data_len = data_len;
  XdrvMailbox.payload16 = payload16;
  XdrvMailbox.payload = payload;
  XdrvMailbox.grpflg = grpflg;
  XdrvMailbox.topic = type;
  XdrvMailbox.data = dataBuf;

  return XdrvCall(FUNC_COMMAND);
}

boolean XdrvMqttData(char *topicBuf, uint16_t stopicBuf, char *dataBuf, uint16_t sdataBuf)
{
  XdrvMailbox.index = stopicBuf;
  XdrvMailbox.data_len = sdataBuf;
  XdrvMailbox.topic = topicBuf;
  XdrvMailbox.data = dataBuf;

  return XdrvCall(FUNC_MQTT_DATA);
}

boolean XdrvRulesProcess()
{
  return XdrvCall(FUNC_RULES_PROCESS);
}

void ShowFreeMem(const char *where)
{
  char stemp[20];
  snprintf_P(stemp, sizeof(stemp), where);
  XdrvMailbox.data = stemp;
  XdrvCall(FUNC_FREE_MEM);
}
# 247 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xdrv_interface.ino"
boolean XdrvCall(byte Function)
{
  boolean result = false;

  for (byte x = 0; x < xdrv_present; x++) {
    result = xdrv_func_ptr[x](Function);
    if (result) break;
  }

  return result;
}
# 1 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xdsp_01_lcd.ino"
# 20 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xdsp_01_lcd.ino"
#ifdef USE_I2C
#ifdef USE_DISPLAY
#ifdef USE_DISPLAY_LCD

#define XDSP_01 1

#define LCD_ADDRESS1 0x27
#define LCD_ADDRESS2 0x3F

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C *lcd;



void LcdInitMode()
{
  lcd->init();
  lcd->clear();
}

void LcdInit(uint8_t mode)
{
  switch(mode) {
    case DISPLAY_INIT_MODE:
      LcdInitMode();
#ifdef USE_DISPLAY_MODES1TO5
      DisplayClearScreenBuffer();
#endif
      break;
    case DISPLAY_INIT_PARTIAL:
    case DISPLAY_INIT_FULL:
      break;
  }
}

void LcdInitDriver()
{
  if (!Settings.display_model) {
    if (I2cDevice(LCD_ADDRESS1)) {
      Settings.display_address[0] = LCD_ADDRESS1;
      Settings.display_model = XDSP_01;
    }
    else if (I2cDevice(LCD_ADDRESS2)) {
      Settings.display_address[0] = LCD_ADDRESS2;
      Settings.display_model = XDSP_01;
    }
  }

  if (XDSP_01 == Settings.display_model) {
    lcd = new LiquidCrystal_I2C(Settings.display_address[0], Settings.display_cols[0], Settings.display_rows);

#ifdef USE_DISPLAY_MODES1TO5
    DisplayAllocScreenBuffer();
#endif

    LcdInitMode();
  }
}

void LcdDrawStringAt()
{
  lcd->setCursor(dsp_x, dsp_y);
  lcd->print(dsp_str);
}

void LcdDisplayOnOff(uint8_t on)
{
  if (on) {
    lcd->backlight();
  } else {
    lcd->noBacklight();
  }
}



#ifdef USE_DISPLAY_MODES1TO5

void LcdCenter(byte row, char* txt)
{
  int offset;
  int len;
  char line[Settings.display_cols[0] +2];

  memset(line, 0x20, Settings.display_cols[0]);
  line[Settings.display_cols[0]] = 0;
  len = strlen(txt);
  offset = (len < Settings.display_cols[0]) ? offset = (Settings.display_cols[0] - len) / 2 : 0;
  strlcpy(line +offset, txt, len);
  lcd->setCursor(0, row);
  lcd->print(line);
}

boolean LcdPrintLog()
{
  boolean result = false;

  disp_refresh--;
  if (!disp_refresh) {
    disp_refresh = Settings.display_refresh;
    if (!disp_screen_buffer_cols) { DisplayAllocScreenBuffer(); }

    char* txt = DisplayLogBuffer('\337');
    if (txt != NULL) {
      uint8_t last_row = Settings.display_rows -1;

      for (byte i = 0; i < last_row; i++) {
        strlcpy(disp_screen_buffer[i], disp_screen_buffer[i +1], disp_screen_buffer_cols);
        lcd->setCursor(0, i);
        lcd->print(disp_screen_buffer[i +1]);
      }
      strlcpy(disp_screen_buffer[last_row], txt, disp_screen_buffer_cols);
      DisplayFillScreen(last_row);

      snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_DEBUG "[%s]"), disp_screen_buffer[last_row]);
      AddLog(LOG_LEVEL_DEBUG);

      lcd->setCursor(0, last_row);
      lcd->print(disp_screen_buffer[last_row]);

      result = true;
    }
  }
  return result;
}

void LcdTime()
{
  char line[Settings.display_cols[0] +1];

  snprintf_P(line, sizeof(line), PSTR("%02d" D_HOUR_MINUTE_SEPARATOR "%02d" D_MINUTE_SECOND_SEPARATOR "%02d"), RtcTime.hour, RtcTime.minute, RtcTime.second);
  LcdCenter(0, line);
  snprintf_P(line, sizeof(line), PSTR("%02d" D_MONTH_DAY_SEPARATOR "%02d" D_YEAR_MONTH_SEPARATOR "%04d"), RtcTime.day_of_month, RtcTime.month, RtcTime.year);
  LcdCenter(1, line);
}

void LcdRefresh()
{
  if (Settings.display_mode) {
    switch (Settings.display_mode) {
      case 1:
        LcdTime();
        break;
      case 2:
      case 4:
        LcdPrintLog();
        break;
      case 3:
      case 5: {
        if (!LcdPrintLog()) { LcdTime(); }
        break;
      }
    }
  }
}

#endif





boolean Xdsp01(byte function)
{
  boolean result = false;

  if (i2c_flg) {
    if (FUNC_DISPLAY_INIT_DRIVER == function) {
      LcdInitDriver();
    }
    else if (XDSP_01 == Settings.display_model) {
      switch (function) {
        case FUNC_DISPLAY_MODEL:
          result = true;
          break;
        case FUNC_DISPLAY_INIT:
          LcdInit(dsp_init);
          break;
        case FUNC_DISPLAY_POWER:
          LcdDisplayOnOff(disp_power);
          break;
        case FUNC_DISPLAY_CLEAR:
          lcd->clear();
          break;
# 224 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xdsp_01_lcd.ino"
        case FUNC_DISPLAY_DRAW_STRING:
          LcdDrawStringAt();
          break;
        case FUNC_DISPLAY_ONOFF:
          LcdDisplayOnOff(dsp_on);
          break;


#ifdef USE_DISPLAY_MODES1TO5
        case FUNC_DISPLAY_EVERY_SECOND:
          LcdRefresh();
          break;
#endif
      }
    }
  }
  return result;
}

#endif
#endif
#endif
# 1 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xdsp_02_ssd1306.ino"
# 20 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xdsp_02_ssd1306.ino"
#ifdef USE_I2C
#ifdef USE_DISPLAY
#ifdef USE_DISPLAY_SSD1306

#define XDSP_02 2

#define OLED_ADDRESS1 0x3C
#define OLED_ADDRESS2 0x3D

#define OLED_FONT_WIDTH 6
#define OLED_FONT_HEIGTH 8




#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

Adafruit_SSD1306 *oled;

uint8_t ssd1306_font_x = OLED_FONT_WIDTH;
uint8_t ssd1306_font_y = OLED_FONT_HEIGTH;



void Ssd1306InitMode()
{
  oled->setRotation(Settings.display_rotate);
  oled->invertDisplay(false);
  oled->clearDisplay();
  oled->setTextWrap(false);
  oled->cp437(true);

  oled->setTextSize(Settings.display_size);
  oled->setTextColor(WHITE);
  oled->setCursor(0,0);
  oled->display();
}

void Ssd1306Init(uint8_t mode)
{
  switch(mode) {
    case DISPLAY_INIT_MODE:
      Ssd1306InitMode();
#ifdef USE_DISPLAY_MODES1TO5
      DisplayClearScreenBuffer();
#endif
      break;
    case DISPLAY_INIT_PARTIAL:
    case DISPLAY_INIT_FULL:
      break;
  }
}

void Ssd1306InitDriver()
{
  if (!Settings.display_model) {
    if (I2cDevice(OLED_ADDRESS1)) {
      Settings.display_address[0] = OLED_ADDRESS1;
      Settings.display_model = XDSP_02;
    }
    else if (I2cDevice(OLED_ADDRESS2)) {
      Settings.display_address[0] = OLED_ADDRESS2;
      Settings.display_model = XDSP_02;
    }
  }

  if (XDSP_02 == Settings.display_model) {
    oled = new Adafruit_SSD1306();
    oled->begin(SSD1306_SWITCHCAPVCC, Settings.display_address[0]);

#ifdef USE_DISPLAY_MODES1TO5
    DisplayAllocScreenBuffer();
#endif

    Ssd1306InitMode();
  }
}

void Ssd1306Clear()
{
  oled->clearDisplay();
  oled->setCursor(0, 0);
  oled->display();
}

void Ssd1306DrawStringAt(uint16_t x, uint16_t y, char *str, uint16_t color, uint8_t flag)
{
  if (!flag) {
    oled->setCursor(x, y);
  } else {
    oled->setCursor((x-1) * ssd1306_font_x * Settings.display_size, (y-1) * ssd1306_font_y * Settings.display_size);
  }
  oled->println(str);
}

void Ssd1306DisplayOnOff(uint8_t on)
{
  if (on) {
    oled->ssd1306_command(SSD1306_DISPLAYON);
  } else {
    oled->ssd1306_command(SSD1306_DISPLAYOFF);
  }
}

void Ssd1306OnOff()
{
  Ssd1306DisplayOnOff(disp_power);
  oled->display();
}



#ifdef USE_DISPLAY_MODES1TO5

void Ssd1306PrintLog()
{
  disp_refresh--;
  if (!disp_refresh) {
    disp_refresh = Settings.display_refresh;
    if (!disp_screen_buffer_cols) { DisplayAllocScreenBuffer(); }

    char* txt = DisplayLogBuffer('\370');
    if (txt != NULL) {
      uint8_t last_row = Settings.display_rows -1;

      oled->clearDisplay();
      oled->setTextSize(Settings.display_size);
      oled->setCursor(0,0);
      for (byte i = 0; i < last_row; i++) {
        strlcpy(disp_screen_buffer[i], disp_screen_buffer[i +1], disp_screen_buffer_cols);
        oled->println(disp_screen_buffer[i]);
      }
      strlcpy(disp_screen_buffer[last_row], txt, disp_screen_buffer_cols);
      DisplayFillScreen(last_row);

      snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_DEBUG "[%s]"), disp_screen_buffer[last_row]);
      AddLog(LOG_LEVEL_DEBUG);

      oled->println(disp_screen_buffer[last_row]);
      oled->display();
    }
  }
}

void Ssd1306Time()
{
  char line[12];

  oled->clearDisplay();
  oled->setTextSize(2);
  oled->setCursor(0, 0);
  snprintf_P(line, sizeof(line), PSTR(" %02d" D_HOUR_MINUTE_SEPARATOR "%02d" D_MINUTE_SECOND_SEPARATOR "%02d"), RtcTime.hour, RtcTime.minute, RtcTime.second);
  oled->println(line);
  snprintf_P(line, sizeof(line), PSTR("%02d" D_MONTH_DAY_SEPARATOR "%02d" D_YEAR_MONTH_SEPARATOR "%04d"), RtcTime.day_of_month, RtcTime.month, RtcTime.year);
  oled->println(line);
  oled->display();
}

void Ssd1306Refresh()
{
  if (Settings.display_mode) {
    switch (Settings.display_mode) {
      case 1:
        Ssd1306Time();
        break;
      case 2:
      case 3:
      case 4:
      case 5:
        Ssd1306PrintLog();
        break;
    }
  }
}

#endif





boolean Xdsp02(byte function)
{
  boolean result = false;

  if (i2c_flg) {
    if (FUNC_DISPLAY_INIT_DRIVER == function) {
      Ssd1306InitDriver();
    }
    else if (XDSP_02 == Settings.display_model) {

      if (!dsp_color) { dsp_color = WHITE; }

      switch (function) {
        case FUNC_DISPLAY_MODEL:
          result = true;
          break;
        case FUNC_DISPLAY_INIT:
          Ssd1306Init(dsp_init);
          break;
        case FUNC_DISPLAY_POWER:
          Ssd1306OnOff();
          break;
        case FUNC_DISPLAY_CLEAR:
          Ssd1306Clear();
          break;
        case FUNC_DISPLAY_DRAW_HLINE:
          oled->writeFastHLine(dsp_x, dsp_y, dsp_len, dsp_color);
          break;
        case FUNC_DISPLAY_DRAW_VLINE:
          oled->writeFastVLine(dsp_x, dsp_y, dsp_len, dsp_color);
          break;
        case FUNC_DISPLAY_DRAW_LINE:
          oled->writeLine(dsp_x, dsp_y, dsp_x2, dsp_y2, dsp_color);
          break;
        case FUNC_DISPLAY_DRAW_CIRCLE:
          oled->drawCircle(dsp_x, dsp_y, dsp_rad, dsp_color);
          break;
        case FUNC_DISPLAY_FILL_CIRCLE:
          oled->fillCircle(dsp_x, dsp_y, dsp_rad, dsp_color);
          break;
        case FUNC_DISPLAY_DRAW_RECTANGLE:
          oled->drawRect(dsp_x, dsp_y, dsp_x2, dsp_y2, dsp_color);
          break;
        case FUNC_DISPLAY_FILL_RECTANGLE:
          oled->fillRect(dsp_x, dsp_y, dsp_x2, dsp_y2, dsp_color);
          break;
        case FUNC_DISPLAY_DRAW_FRAME:
          oled->display();
          break;
        case FUNC_DISPLAY_TEXT_SIZE:
          oled->setTextSize(Settings.display_size);
          break;
        case FUNC_DISPLAY_FONT_SIZE:

          break;
        case FUNC_DISPLAY_DRAW_STRING:
          Ssd1306DrawStringAt(dsp_x, dsp_y, dsp_str, dsp_color, dsp_flag);
          break;
        case FUNC_DISPLAY_ONOFF:
          Ssd1306DisplayOnOff(dsp_on);
          break;
        case FUNC_DISPLAY_ROTATION:
          oled->setRotation(Settings.display_rotate);
          break;
#ifdef USE_DISPLAY_MODES1TO5
        case FUNC_DISPLAY_EVERY_SECOND:
          Ssd1306Refresh();
          break;
#endif
      }
    }
  }
  return result;
}

#endif
#endif
#endif
# 1 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xdsp_03_matrix.ino"
# 20 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xdsp_03_matrix.ino"
#ifdef USE_I2C
#ifdef USE_DISPLAY
#ifdef USE_DISPLAY_MATRIX

#define XDSP_03 3

#define MTX_MAX_SCREEN_BUFFER 80

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_LEDBackpack.h>

Adafruit_8x8matrix *matrix[8];
uint8_t mtx_matrices = 0;
uint8_t mtx_state = 0;
uint8_t mtx_counter = 0;
int16_t mtx_x = 0;
int16_t mtx_y = 0;

char mtx_buffer[MTX_MAX_SCREEN_BUFFER];
uint8_t mtx_mode = 0;
uint8_t mtx_loop = 0;
uint8_t mtx_done = 0;



void MatrixWrite()
{
  for (byte i = 0; i < mtx_matrices; i++) {
    matrix[i]->writeDisplay();
  }
}

void MatrixClear()
{
  for (byte i = 0; i < mtx_matrices; i++) {
    matrix[i]->clear();
  }
  MatrixWrite();
}

void MatrixFixed(char* txt)
{
  for (byte i = 0; i < mtx_matrices; i++) {
    matrix[i]->clear();
    matrix[i]->setCursor(-i *8, 0);
    matrix[i]->print(txt);
    matrix[i]->setBrightness(Settings.display_dimmer);
  }
  MatrixWrite();
}

void MatrixCenter(char* txt)
{
  int offset;

  int len = strlen(txt);
  offset = (len < 8) ? offset = ((mtx_matrices *8) - (len *6)) / 2 : 0;
  for (byte i = 0; i < mtx_matrices; i++) {
    matrix[i]->clear();
    matrix[i]->setCursor(-(i *8)+offset, 0);
    matrix[i]->print(txt);
    matrix[i]->setBrightness(Settings.display_dimmer);
  }
  MatrixWrite();
}

void MatrixScrollLeft(char* txt, int loop)
{
  switch (mtx_state) {
  case 1:
    mtx_state = 2;

    mtx_x = 8 * mtx_matrices;

    snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_DEBUG "[%s]"), txt);
    AddLog(LOG_LEVEL_DEBUG);

    disp_refresh = Settings.display_refresh;
  case 2:
  disp_refresh--;
    if (!disp_refresh) {
      disp_refresh = Settings.display_refresh;
      for (byte i = 0; i < mtx_matrices; i++) {
        matrix[i]->clear();
        matrix[i]->setCursor(mtx_x - i *8, 0);
        matrix[i]->print(txt);
        matrix[i]->setBrightness(Settings.display_dimmer);
      }
      MatrixWrite();

      mtx_x--;
      int16_t len = strlen(txt);
      if (mtx_x < -(len *6)) { mtx_state = loop; }
    }
    break;
  }
}

void MatrixScrollUp(char* txt, int loop)
{
  int wordcounter = 0;
  char tmpbuf[200];
  char *words[100];



  char separators[] = " /";

  switch (mtx_state) {
  case 1:
    mtx_state = 2;

    mtx_y = 8;
    mtx_counter = 0;
    disp_refresh = Settings.display_refresh;
  case 2:
    disp_refresh--;
    if (!disp_refresh) {
      disp_refresh = Settings.display_refresh;
      strlcpy(tmpbuf, txt, sizeof(tmpbuf));
      char *p = strtok(tmpbuf, separators);
      while (p != NULL && wordcounter < 40) {
        words[wordcounter++] = p;
        p = strtok(NULL, separators);
      }
      for (byte i = 0; i < mtx_matrices; i++) {
        matrix[i]->clear();
        for (byte j = 0; j < wordcounter; j++) {
          matrix[i]->setCursor(-i *8, mtx_y + (j *8));
          matrix[i]->println(words[j]);
        }
        matrix[i]->setBrightness(Settings.display_dimmer);
      }
      MatrixWrite();
      if (((mtx_y %8) == 0) && mtx_counter) {
        mtx_counter--;
      } else {
        mtx_y--;
        mtx_counter = STATES * 1;
      }
      if (mtx_y < -(wordcounter *8)) { mtx_state = loop; }
    }
    break;
  }
}



void MatrixInitMode()
{
  for (byte i = 0; i < mtx_matrices; i++) {
    matrix[i]->setRotation(Settings.display_rotate);
    matrix[i]->setBrightness(Settings.display_dimmer);
    matrix[i]->blinkRate(0);
    matrix[i]->setTextWrap(false);


    matrix[i]->cp437(true);
  }
  MatrixClear();
}

void MatrixInit(uint8_t mode)
{
  switch(mode) {
    case DISPLAY_INIT_MODE:
      MatrixInitMode();
      break;
    case DISPLAY_INIT_PARTIAL:
    case DISPLAY_INIT_FULL:
      break;
  }
}

void MatrixInitDriver()
{
  if (!Settings.display_model) {
    if (I2cDevice(Settings.display_address[1])) {
      Settings.display_model = XDSP_03;
    }
  }

  if (XDSP_03 == Settings.display_model) {
    mtx_state = 1;
    for (mtx_matrices = 0; mtx_matrices < 8; mtx_matrices++) {
      if (Settings.display_address[mtx_matrices]) {
        matrix[mtx_matrices] = new Adafruit_8x8matrix();
        matrix[mtx_matrices]->begin(Settings.display_address[mtx_matrices]);
      } else {
        break;
      }
    }

    MatrixInitMode();
  }
}

void MatrixOnOff()
{
  if (!disp_power) { MatrixClear(); }
}

void MatrixDrawStringAt(uint16_t x, uint16_t y, char *str, uint16_t color, uint8_t flag)
{
  snprintf(mtx_buffer, sizeof(mtx_buffer), str);
  mtx_mode = x &1;
  mtx_loop = y &1;
  if (!mtx_state) { mtx_state = 1; }
}



#ifdef USE_DISPLAY_MODES1TO5

void MatrixPrintLog(uint8_t direction)
{
  char* txt = (!mtx_done) ? DisplayLogBuffer('\370') : mtx_buffer;
  if (txt != NULL) {
    if (!mtx_state) { mtx_state = 1; }

    if (!mtx_done) {

      uint8_t space = 0;
      uint8_t max_cols = (disp_log_buffer_cols < MTX_MAX_SCREEN_BUFFER) ? disp_log_buffer_cols : MTX_MAX_SCREEN_BUFFER;
      mtx_buffer[0] = '\0';
      uint8_t i = 0;
      while ((txt[i] != '\0') && (i < max_cols)) {
        if (txt[i] == ' ') {
          space++;
        } else {
          space = 0;
        }
        if (space < 2) {
          strncat(mtx_buffer, (const char*)txt +i, 1);
        }
        i++;
      }

      snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_APPLICATION "[%s]"), mtx_buffer);
      AddLog(LOG_LEVEL_DEBUG);

      mtx_done = 1;
    }

    if (direction) {
      MatrixScrollUp(mtx_buffer, 0);
    } else {
      MatrixScrollLeft(mtx_buffer, 0);
    }
    if (!mtx_state) { mtx_done = 0; }
  } else {
    char disp_time[9];

    snprintf_P(disp_time, sizeof(disp_time), PSTR("%02d" D_HOUR_MINUTE_SEPARATOR "%02d" D_MINUTE_SECOND_SEPARATOR "%02d"), RtcTime.hour, RtcTime.minute, RtcTime.second);
    MatrixFixed(disp_time);
  }
}

#endif

void MatrixRefresh()
{
  if (disp_power) {
    switch (Settings.display_mode) {
      case 0: {
        switch (mtx_mode) {
          case 0:
            MatrixScrollLeft(mtx_buffer, mtx_loop);
            break;
          case 1:
            MatrixScrollUp(mtx_buffer, mtx_loop);
            break;
        }
        break;
      }
#ifdef USE_DISPLAY_MODES1TO5
      case 2: {
        char disp_date[9];
        snprintf_P(disp_date, sizeof(disp_date), PSTR("%02d" D_MONTH_DAY_SEPARATOR "%02d" D_YEAR_MONTH_SEPARATOR "%02d"), RtcTime.day_of_month, RtcTime.month, RtcTime.year -2000);
        MatrixFixed(disp_date);
        break;
      }
      case 3: {
        char disp_day[10];
        snprintf_P(disp_day, sizeof(disp_day), PSTR("%d %s"), RtcTime.day_of_month, RtcTime.name_of_month);
        MatrixCenter(disp_day);
        break;
      }
      case 4:
        MatrixPrintLog(0);
        break;
      case 1:
      case 5:
        MatrixPrintLog(1);
        break;
#endif
    }
  }
}





boolean Xdsp03(byte function)
{
  boolean result = false;

  if (i2c_flg) {
    if (FUNC_DISPLAY_INIT_DRIVER == function) {
      MatrixInitDriver();
    }
    else if (XDSP_03 == Settings.display_model) {
      switch (function) {
        case FUNC_DISPLAY_MODEL:
          result = true;
          break;
        case FUNC_DISPLAY_INIT:
          MatrixInit(dsp_init);
          break;
        case FUNC_DISPLAY_EVERY_50_MSECOND:
          MatrixRefresh();
          break;
        case FUNC_DISPLAY_POWER:
          MatrixOnOff();
          break;
        case FUNC_DISPLAY_DRAW_STRING:
          MatrixDrawStringAt(dsp_x, dsp_y, dsp_str, dsp_color, dsp_flag);
          break;
      }
    }
  }
  return result;
}

#endif
#endif
#endif
# 1 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xdsp_04_ili9341.ino"
# 20 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xdsp_04_ili9341.ino"
#ifdef USE_SPI
#ifdef USE_DISPLAY
#ifdef USE_DISPLAY_ILI9341

#define XDSP_04 4

#define TFT_TOP 16
#define TFT_BOTTOM 16
#define TFT_FONT_WIDTH 6
#define TFT_FONT_HEIGTH 8

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

Adafruit_ILI9341 *tft;

uint16_t tft_scroll;



void Ili9341InitMode()
{
  tft->setRotation(Settings.display_rotate);
  tft->invertDisplay(0);
  tft->fillScreen(ILI9341_BLACK);
  tft->setTextWrap(false);
  tft->cp437(true);
  if (!Settings.display_mode) {
    tft->setCursor(0, 0);
    tft->setTextColor(ILI9341_WHITE, ILI9341_BLACK);
    tft->setTextSize(1);
  } else {
    tft->setScrollMargins(TFT_TOP, TFT_BOTTOM);
    tft->setCursor(0, 0);
    tft->setTextColor(ILI9341_YELLOW, ILI9341_BLACK);
    tft->setTextSize(2);


    tft_scroll = TFT_TOP;
  }
}

void Ili9341Init(uint8_t mode)
{
  switch(mode) {
    case DISPLAY_INIT_MODE:
      Ili9341InitMode();
#ifdef USE_DISPLAY_MODES1TO5
      if (Settings.display_rotate) {
        DisplayClearScreenBuffer();
      }
#endif
      break;
    case DISPLAY_INIT_PARTIAL:
    case DISPLAY_INIT_FULL:
      break;
  }
}

void Ili9341InitDriver()
{
  if (!Settings.display_model) {
    Settings.display_model = XDSP_04;
  }

  if (XDSP_04 == Settings.display_model) {
    tft = new Adafruit_ILI9341(pin[GPIO_SPI_CS], pin[GPIO_SPI_DC]);
    tft->begin();

#ifdef USE_DISPLAY_MODES1TO5
    if (Settings.display_rotate) {
      DisplayAllocScreenBuffer();
    }
#endif

    Ili9341InitMode();
  }
}

void Ili9341Clear()
{
  tft->fillScreen(ILI9341_BLACK);
  tft->setCursor(0, 0);
}

void Ili9341DrawStringAt(uint16_t x, uint16_t y, char *str, uint16_t color, uint8_t flag)
{
  uint16_t active_color = ILI9341_WHITE;

  tft->setTextSize(Settings.display_size);
  if (!flag) {
    tft->setCursor(x, y);
  } else {
    tft->setCursor((x-1) * TFT_FONT_WIDTH * Settings.display_size, (y-1) * TFT_FONT_HEIGTH * Settings.display_size);
  }
  if (color) { active_color = color; }
  tft->setTextColor(active_color, ILI9341_BLACK);
  tft->println(str);
}

void Ili9341DisplayOnOff(uint8_t on)
{


  if (pin[GPIO_BACKLIGHT] < 99) {
    pinMode(pin[GPIO_BACKLIGHT], OUTPUT);
    digitalWrite(pin[GPIO_BACKLIGHT], on);
  }
}

void Ili9341OnOff()
{
  Ili9341DisplayOnOff(disp_power);
}



#ifdef USE_DISPLAY_MODES1TO5

void Ili9341PrintLog()
{
  disp_refresh--;
  if (!disp_refresh) {
    disp_refresh = Settings.display_refresh;
    if (Settings.display_rotate) {
      if (!disp_screen_buffer_cols) { DisplayAllocScreenBuffer(); }
    }

    char* txt = DisplayLogBuffer('\370');
    if (txt != NULL) {
      byte size = Settings.display_size;
      uint16_t theight = size * TFT_FONT_HEIGTH;

      tft->setTextSize(size);
      tft->setTextColor(ILI9341_CYAN, ILI9341_BLACK);
      if (!Settings.display_rotate) {
        tft->setCursor(0, tft_scroll);
        tft->fillRect(0, tft_scroll, tft->width(), theight, ILI9341_BLACK);
        tft->print(txt);
        tft_scroll += theight;
        if (tft_scroll >= (tft->height() - TFT_BOTTOM)) {
          tft_scroll = TFT_TOP;
        }
        tft->scrollTo(tft_scroll);
      } else {
        uint8_t last_row = Settings.display_rows -1;

        tft_scroll = theight;
        tft->setCursor(0, tft_scroll);
        for (byte i = 0; i < last_row; i++) {
          strlcpy(disp_screen_buffer[i], disp_screen_buffer[i +1], disp_screen_buffer_cols);

          tft->print(disp_screen_buffer[i]);
          tft_scroll += theight;
          tft->setCursor(0, tft_scroll);
        }
        strlcpy(disp_screen_buffer[last_row], txt, disp_screen_buffer_cols);
        DisplayFillScreen(last_row);
        tft->print(disp_screen_buffer[last_row]);
      }
      snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_APPLICATION "[%s]"), txt);
      AddLog(LOG_LEVEL_DEBUG);
    }
  }
}

void Ili9341Refresh()
{
  if (Settings.display_mode) {
    char tftdt[Settings.display_cols[0] +1];
    char date4[11];
    char space[Settings.display_cols[0] - 17];
    char time[9];

    tft->setTextSize(2);
    tft->setTextColor(ILI9341_YELLOW, ILI9341_RED);
    tft->setCursor(0, 0);

    snprintf_P(date4, sizeof(date4), PSTR("%02d" D_MONTH_DAY_SEPARATOR "%02d" D_YEAR_MONTH_SEPARATOR "%04d"), RtcTime.day_of_month, RtcTime.month, RtcTime.year);
    memset(space, 0x20, sizeof(space));
    space[sizeof(space) -1] = '\0';
    snprintf_P(time, sizeof(time), PSTR("%02d" D_HOUR_MINUTE_SEPARATOR "%02d" D_MINUTE_SECOND_SEPARATOR "%02d"), RtcTime.hour, RtcTime.minute, RtcTime.second);
    snprintf_P(tftdt, sizeof(tftdt), PSTR("%s%s%s"), date4, space, time);

    tft->print(tftdt);

    switch (Settings.display_mode) {
      case 1:
      case 2:
      case 3:
      case 4:
      case 5:
        Ili9341PrintLog();
        break;
    }
  }
}

#endif





boolean Xdsp04(byte function)
{
  boolean result = false;

  if (spi_flg) {
    if (FUNC_DISPLAY_INIT_DRIVER == function) {
      Ili9341InitDriver();
    }
    else if (XDSP_04 == Settings.display_model) {

      if (!dsp_color) { dsp_color = ILI9341_WHITE; }

      switch (function) {
        case FUNC_DISPLAY_MODEL:
          result = true;
          break;
        case FUNC_DISPLAY_INIT:
          Ili9341Init(dsp_init);
          break;
        case FUNC_DISPLAY_POWER:
          Ili9341OnOff();
          break;
        case FUNC_DISPLAY_CLEAR:
          Ili9341Clear();
          break;
        case FUNC_DISPLAY_DRAW_HLINE:
          tft->writeFastHLine(dsp_x, dsp_y, dsp_len, dsp_color);
          break;
        case FUNC_DISPLAY_DRAW_VLINE:
          tft->writeFastVLine(dsp_x, dsp_y, dsp_len, dsp_color);
          break;
        case FUNC_DISPLAY_DRAW_LINE:
          tft->writeLine(dsp_x, dsp_y, dsp_x2, dsp_y2, dsp_color);
          break;
        case FUNC_DISPLAY_DRAW_CIRCLE:
          tft->drawCircle(dsp_x, dsp_y, dsp_rad, dsp_color);
          break;
        case FUNC_DISPLAY_FILL_CIRCLE:
          tft->fillCircle(dsp_x, dsp_y, dsp_rad, dsp_color);
          break;
        case FUNC_DISPLAY_DRAW_RECTANGLE:
          tft->drawRect(dsp_x, dsp_y, dsp_x2, dsp_y2, dsp_color);
          break;
        case FUNC_DISPLAY_FILL_RECTANGLE:
          tft->fillRect(dsp_x, dsp_y, dsp_x2, dsp_y2, dsp_color);
          break;



        case FUNC_DISPLAY_TEXT_SIZE:
          tft->setTextSize(Settings.display_size);
          break;
        case FUNC_DISPLAY_FONT_SIZE:

          break;
        case FUNC_DISPLAY_DRAW_STRING:
          Ili9341DrawStringAt(dsp_x, dsp_y, dsp_str, dsp_color, dsp_flag);
          break;
        case FUNC_DISPLAY_ONOFF:
          Ili9341DisplayOnOff(dsp_on);
          break;
        case FUNC_DISPLAY_ROTATION:
          tft->setRotation(Settings.display_rotate);
          break;
#ifdef USE_DISPLAY_MODES1TO5
        case FUNC_DISPLAY_EVERY_SECOND:
          Ili9341Refresh();
          break;
#endif
      }
    }
  }
  return result;
}

#endif
#endif
#endif
# 1 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xdsp_interface.ino"
# 20 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xdsp_interface.ino"
boolean (* const xdsp_func_ptr[])(byte) PROGMEM = {
#ifdef XDSP_01
  &Xdsp01,
#endif

#ifdef XDSP_02
  &Xdsp02,
#endif

#ifdef XDSP_03
  &Xdsp03,
#endif

#ifdef XDSP_04
  &Xdsp04,
#endif

#ifdef XDSP_05
  &Xdsp05,
#endif

#ifdef XDSP_06
  &Xdsp06,
#endif

#ifdef XDSP_07
  &Xdsp07,
#endif

#ifdef XDSP_08
  &Xdsp08,
#endif

#ifdef XDSP_09
  &Xdsp09,
#endif

#ifdef XDSP_10
  &Xdsp10,
#endif

#ifdef XDSP_11
  &Xdsp11,
#endif

#ifdef XDSP_12
  &Xdsp12,
#endif

#ifdef XDSP_13
  &Xdsp13,
#endif

#ifdef XDSP_14
  &Xdsp14,
#endif

#ifdef XDSP_15
  &Xdsp15,
#endif

#ifdef XDSP_16
  &Xdsp16
#endif
};

const uint8_t xdsp_present = sizeof(xdsp_func_ptr) / sizeof(xdsp_func_ptr[0]);
# 110 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xdsp_interface.ino"
uint8_t XdspPresent()
{
  return xdsp_present;
}

boolean XdspCall(byte Function)
{
  boolean result = false;

  for (byte x = 0; x < xdsp_present; x++) {
    result = xdsp_func_ptr[x](Function);
    if (result) break;
  }

  return result;
}
# 1 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xnrg_01_hlw8012.ino"
# 20 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xnrg_01_hlw8012.ino"
#ifdef USE_ENERGY_SENSOR
#ifdef USE_HLW8012






#define XNRG_01 1


#define HLW_PREF 10000
#define HLW_UREF 2200
#define HLW_IREF 4545


#define HJL_PREF 1362
#define HJL_UREF 822
#define HJL_IREF 3300

#define HLW_POWER_PROBE_TIME 10

byte hlw_select_ui_flag;
byte hlw_ui_flag = 1;
byte hlw_model_type = 0;
byte hlw_load_off;
byte hlw_cf1_timer;
unsigned long hlw_cf_pulse_length;
unsigned long hlw_cf_pulse_last_time;
unsigned long hlw_cf1_pulse_length;
unsigned long hlw_cf1_pulse_last_time;
unsigned long hlw_cf1_summed_pulse_length;
unsigned long hlw_cf1_pulse_counter;
unsigned long hlw_cf1_voltage_pulse_length;
unsigned long hlw_cf1_current_pulse_length;
unsigned long hlw_energy_period_counter;

unsigned long hlw_power_ratio = 0;
unsigned long hlw_voltage_ratio = 0;
unsigned long hlw_current_ratio = 0;

unsigned long hlw_cf1_voltage_max_pulse_counter;
unsigned long hlw_cf1_current_max_pulse_counter;

#ifndef USE_WS2812_DMA
void HlwCfInterrupt() ICACHE_RAM_ATTR;
void HlwCf1Interrupt() ICACHE_RAM_ATTR;
#endif

void HlwCfInterrupt()
{
  unsigned long us = micros();

  if (hlw_load_off) {
    hlw_cf_pulse_last_time = us;
    hlw_load_off = 0;
  } else {
    hlw_cf_pulse_length = us - hlw_cf_pulse_last_time;
    hlw_cf_pulse_last_time = us;
    hlw_energy_period_counter++;
  }
}

void HlwCf1Interrupt()
{
  unsigned long us = micros();

  hlw_cf1_pulse_length = us - hlw_cf1_pulse_last_time;
  hlw_cf1_pulse_last_time = us;
  if ((hlw_cf1_timer > 2) && (hlw_cf1_timer < 8)) {
    hlw_cf1_summed_pulse_length += hlw_cf1_pulse_length;
    hlw_cf1_pulse_counter++;
    if (10 == hlw_cf1_pulse_counter) {
      hlw_cf1_timer = 8;
    }
  }
}



void HlwEvery200ms()
{
  unsigned long hlw_w = 0;
  unsigned long hlw_u = 0;
  unsigned long hlw_i = 0;

  if (micros() - hlw_cf_pulse_last_time > (HLW_POWER_PROBE_TIME * 1000000)) {
    hlw_cf_pulse_length = 0;
    hlw_load_off = 1;
  }

  if (hlw_cf_pulse_length && energy_power_on && !hlw_load_off) {
    hlw_w = (hlw_power_ratio * Settings.energy_power_calibration) / hlw_cf_pulse_length;
    energy_active_power = (float)hlw_w / 10;
  } else {
    energy_active_power = 0;
  }

  hlw_cf1_timer++;
  if (hlw_cf1_timer >= 8) {
    hlw_cf1_timer = 0;
    hlw_select_ui_flag = (hlw_select_ui_flag) ? 0 : 1;
    digitalWrite(pin[GPIO_NRG_SEL], hlw_select_ui_flag);

    if (hlw_cf1_pulse_counter) {
      hlw_cf1_pulse_length = hlw_cf1_summed_pulse_length / hlw_cf1_pulse_counter;
    } else {
      hlw_cf1_pulse_length = 0;
    }
    if (hlw_select_ui_flag == hlw_ui_flag) {
      hlw_cf1_voltage_pulse_length = hlw_cf1_pulse_length;
      hlw_cf1_voltage_max_pulse_counter = hlw_cf1_pulse_counter;

      if (hlw_cf1_voltage_pulse_length && energy_power_on) {
        hlw_u = (hlw_voltage_ratio * Settings.energy_voltage_calibration) / hlw_cf1_voltage_pulse_length;
        energy_voltage = (float)hlw_u / 10;
      } else {
        energy_voltage = 0;
      }

    } else {
      hlw_cf1_current_pulse_length = hlw_cf1_pulse_length;
      hlw_cf1_current_max_pulse_counter = hlw_cf1_pulse_counter;

      if (hlw_cf1_current_pulse_length && energy_active_power) {
        hlw_i = (hlw_current_ratio * Settings.energy_current_calibration) / hlw_cf1_current_pulse_length;
        energy_current = (float)hlw_i / 1000;
      } else {
        energy_current = 0;
      }

    }
    hlw_cf1_summed_pulse_length = 0;
    hlw_cf1_pulse_counter = 0;
  }
}

void HlwEverySecond()
{
  unsigned long hlw_len;

  if (hlw_energy_period_counter) {
    hlw_len = 10000 / hlw_energy_period_counter;
    hlw_energy_period_counter = 0;
    if (hlw_len) {
      energy_kWhtoday_delta += ((hlw_power_ratio * Settings.energy_power_calibration) / hlw_len) / 36;
      EnergyUpdateToday();
    }
  }
}

void HlwSnsInit()
{
  if (!Settings.energy_power_calibration || (4975 == Settings.energy_power_calibration)) {
    Settings.energy_power_calibration = HLW_PREF_PULSE;
    Settings.energy_voltage_calibration = HLW_UREF_PULSE;
    Settings.energy_current_calibration = HLW_IREF_PULSE;
  }

  if (hlw_model_type) {
    hlw_power_ratio = HJL_PREF;
    hlw_voltage_ratio = HJL_UREF;
    hlw_current_ratio = HJL_IREF;
  } else {
    hlw_power_ratio = HLW_PREF;
    hlw_voltage_ratio = HLW_UREF;
    hlw_current_ratio = HLW_IREF;
  }

  hlw_cf_pulse_length = 0;
  hlw_cf_pulse_last_time = 0;
  hlw_cf1_pulse_length = 0;
  hlw_cf1_pulse_last_time = 0;
  hlw_cf1_voltage_pulse_length = 0;
  hlw_cf1_current_pulse_length = 0;
  hlw_cf1_voltage_max_pulse_counter = 0;
  hlw_cf1_current_max_pulse_counter = 0;

  hlw_load_off = 1;
  hlw_energy_period_counter = 0;

  hlw_select_ui_flag = 0;

  pinMode(pin[GPIO_NRG_SEL], OUTPUT);
  digitalWrite(pin[GPIO_NRG_SEL], hlw_select_ui_flag);
  pinMode(pin[GPIO_NRG_CF1], INPUT_PULLUP);
  attachInterrupt(pin[GPIO_NRG_CF1], HlwCf1Interrupt, FALLING);
  pinMode(pin[GPIO_HLW_CF], INPUT_PULLUP);
  attachInterrupt(pin[GPIO_HLW_CF], HlwCfInterrupt, FALLING);

  hlw_cf1_timer = 0;
}

void HlwDrvInit()
{
  if (!energy_flg) {
    hlw_model_type = 0;
    if (pin[GPIO_HJL_CF] < 99) {
      pin[GPIO_HLW_CF] = pin[GPIO_HJL_CF];
      pin[GPIO_HJL_CF] = 99;
      hlw_model_type = 1;
    }

    hlw_ui_flag = 1;
    if (pin[GPIO_NRG_SEL_INV] < 99) {
      pin[GPIO_NRG_SEL] = pin[GPIO_NRG_SEL_INV];
      pin[GPIO_NRG_SEL_INV] = 99;
      hlw_ui_flag = 0;
    }

    if ((pin[GPIO_NRG_SEL] < 99) && (pin[GPIO_NRG_CF1] < 99) && (pin[GPIO_HLW_CF] < 99)) {
      energy_flg = XNRG_01;
    }
  }
}

boolean HlwCommand()
{
  boolean serviced = true;

  if (CMND_POWERSET == energy_command_code) {
    if (XdrvMailbox.data_len && hlw_cf_pulse_length) {
      Settings.energy_power_calibration = ((unsigned long)(CharToDouble(XdrvMailbox.data) * 10) * hlw_cf_pulse_length) / hlw_power_ratio;
    }
  }
  else if (CMND_VOLTAGESET == energy_command_code) {
    if (XdrvMailbox.data_len && hlw_cf1_voltage_pulse_length) {
      Settings.energy_voltage_calibration = ((unsigned long)(CharToDouble(XdrvMailbox.data) * 10) * hlw_cf1_voltage_pulse_length) / hlw_voltage_ratio;
    }
  }
  else if (CMND_CURRENTSET == energy_command_code) {
    if (XdrvMailbox.data_len && hlw_cf1_current_pulse_length) {
      Settings.energy_current_calibration = ((unsigned long)(CharToDouble(XdrvMailbox.data)) * hlw_cf1_current_pulse_length) / hlw_current_ratio;
    }
  }
  else serviced = false;

  return serviced;
}





int Xnrg01(byte function)
{
  int result = 0;

  if (FUNC_PRE_INIT == function) {
    HlwDrvInit();
  }
  else if (XNRG_01 == energy_flg) {
    switch (function) {
      case FUNC_INIT:
        HlwSnsInit();
        break;
      case FUNC_EVERY_SECOND:
        HlwEverySecond();
        break;
      case FUNC_EVERY_200_MSECOND:
        HlwEvery200ms();
        break;
      case FUNC_COMMAND:
        result = HlwCommand();
        break;
    }
  }
  return result;
}

#endif
#endif
# 1 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xnrg_02_cse7766.ino"
# 20 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xnrg_02_cse7766.ino"
#ifdef USE_ENERGY_SENSOR
#ifdef USE_CSE7766






#define XNRG_02 2

#define CSE_MAX_INVALID_POWER 128

#define CSE_NOT_CALIBRATED 0xAA

#define CSE_PULSES_NOT_INITIALIZED -1

#define CSE_PREF 1000
#define CSE_UREF 100

uint8_t cse_receive_flag = 0;

long voltage_cycle = 0;
long current_cycle = 0;
long power_cycle = 0;
unsigned long power_cycle_first = 0;
long cf_pulses = 0;
long cf_pulses_last_time = CSE_PULSES_NOT_INITIALIZED;
uint8_t cse_power_invalid = CSE_MAX_INVALID_POWER;

void CseReceived()
{





  uint8_t header = serial_in_buffer[0];
  if ((header & 0xFC) == 0xFC) {
    AddLog_P(LOG_LEVEL_DEBUG, PSTR("CSE: Abnormal hardware"));
    return;
  }


  if (HLW_UREF_PULSE == Settings.energy_voltage_calibration) {
    long voltage_coefficient = 191200;
    if (CSE_NOT_CALIBRATED != header) {
      voltage_coefficient = serial_in_buffer[2] << 16 | serial_in_buffer[3] << 8 | serial_in_buffer[4];
    }
    Settings.energy_voltage_calibration = voltage_coefficient / CSE_UREF;
  }
  if (HLW_IREF_PULSE == Settings.energy_current_calibration) {
    long current_coefficient = 16140;
    if (CSE_NOT_CALIBRATED != header) {
      current_coefficient = serial_in_buffer[8] << 16 | serial_in_buffer[9] << 8 | serial_in_buffer[10];
    }
    Settings.energy_current_calibration = current_coefficient;
  }
  if (HLW_PREF_PULSE == Settings.energy_power_calibration) {
    long power_coefficient = 5364000;
    if (CSE_NOT_CALIBRATED != header) {
      power_coefficient = serial_in_buffer[14] << 16 | serial_in_buffer[15] << 8 | serial_in_buffer[16];
    }
    Settings.energy_power_calibration = power_coefficient / CSE_PREF;
  }

  uint8_t adjustement = serial_in_buffer[20];
  voltage_cycle = serial_in_buffer[5] << 16 | serial_in_buffer[6] << 8 | serial_in_buffer[7];
  current_cycle = serial_in_buffer[11] << 16 | serial_in_buffer[12] << 8 | serial_in_buffer[13];
  power_cycle = serial_in_buffer[17] << 16 | serial_in_buffer[18] << 8 | serial_in_buffer[19];
  cf_pulses = serial_in_buffer[21] << 8 | serial_in_buffer[22];

  if (energy_power_on) {
    if (adjustement & 0x40) {
      energy_voltage = (float)(Settings.energy_voltage_calibration * CSE_UREF) / (float)voltage_cycle;
    }
    if (adjustement & 0x10) {
      cse_power_invalid = 0;
      if ((header & 0xF2) == 0xF2) {
        energy_active_power = 0;
      } else {
        if (0 == power_cycle_first) { power_cycle_first = power_cycle; }
        if (power_cycle_first != power_cycle) {
          power_cycle_first = -1;
          energy_active_power = (float)(Settings.energy_power_calibration * CSE_PREF) / (float)power_cycle;
        } else {
          energy_active_power = 0;
        }
      }
    } else {
      if (cse_power_invalid < CSE_MAX_INVALID_POWER) {
        cse_power_invalid++;
      } else {
        power_cycle_first = 0;
        energy_active_power = 0;
      }
    }
    if (adjustement & 0x20) {
      if (0 == energy_active_power) {
        energy_current = 0;
      } else {
        energy_current = (float)Settings.energy_current_calibration / (float)current_cycle;
      }
    }
  } else {
    power_cycle_first = 0;
    energy_voltage = 0;
    energy_active_power = 0;
    energy_current = 0;
  }
}

bool CseSerialInput()
{
  if (cse_receive_flag) {
    serial_in_buffer[serial_in_byte_counter++] = serial_in_byte;
    if (24 == serial_in_byte_counter) {

      AddLogSerial(LOG_LEVEL_DEBUG_MORE);

      uint8_t checksum = 0;
      for (byte i = 2; i < 23; i++) { checksum += serial_in_buffer[i]; }
      if (checksum == serial_in_buffer[23]) {
        CseReceived();
        cse_receive_flag = 0;
        return 1;
      } else {
        AddLog_P(LOG_LEVEL_DEBUG, PSTR("CSE: " D_CHECKSUM_FAILURE));
        do {
          memmove(serial_in_buffer, serial_in_buffer +1, 24);
          serial_in_byte_counter--;
        } while ((serial_in_byte_counter > 2) && (0x5A != serial_in_buffer[1]));
        if (0x5A != serial_in_buffer[1]) {
          cse_receive_flag = 0;
          serial_in_byte_counter = 0;
        }
      }
    }
  } else {
    if ((0x5A == serial_in_byte) && (1 == serial_in_byte_counter)) {
      cse_receive_flag = 1;
    } else {
      serial_in_byte_counter = 0;
    }
    serial_in_buffer[serial_in_byte_counter++] = serial_in_byte;
  }
  serial_in_byte = 0;
  return 0;
}



void CseEverySecond()
{
  long cf_frequency = 0;

  if (CSE_PULSES_NOT_INITIALIZED == cf_pulses_last_time) {
    cf_pulses_last_time = cf_pulses;
  } else {
    if (cf_pulses < cf_pulses_last_time) {
      cf_frequency = (65536 - cf_pulses_last_time) + cf_pulses;
    } else {
      cf_frequency = cf_pulses - cf_pulses_last_time;
    }
    if (cf_frequency && energy_active_power) {
      cf_pulses_last_time = cf_pulses;
      energy_kWhtoday_delta += (cf_frequency * Settings.energy_power_calibration) / 36;
      EnergyUpdateToday();
    }
  }
}

void CseDrvInit()
{
  if (!energy_flg) {
    if ((SONOFF_S31 == Settings.module) || (SONOFF_POW_R2 == Settings.module)) {
      baudrate = 4800;
      serial_config = SERIAL_8E1;
      energy_flg = XNRG_02;
    }
  }
}

boolean CseCommand()
{
  boolean serviced = true;

  if (CMND_POWERSET == energy_command_code) {
    if (XdrvMailbox.data_len && power_cycle) {
      Settings.energy_power_calibration = ((unsigned long)CharToDouble(XdrvMailbox.data) * power_cycle) / CSE_PREF;
    }
  }
  else if (CMND_VOLTAGESET == energy_command_code) {
    if (XdrvMailbox.data_len && voltage_cycle) {
      Settings.energy_voltage_calibration = ((unsigned long)CharToDouble(XdrvMailbox.data) * voltage_cycle) / CSE_UREF;
    }
  }
  else if (CMND_CURRENTSET == energy_command_code) {
    if (XdrvMailbox.data_len && current_cycle) {
      Settings.energy_current_calibration = ((unsigned long)CharToDouble(XdrvMailbox.data) * current_cycle) / 1000;
    }
  }
  else serviced = false;

  return serviced;
}





int Xnrg02(byte function)
{
  int result = 0;

  if (FUNC_PRE_INIT == function) {
    CseDrvInit();
  }
  else if (XNRG_02 == energy_flg) {
    switch (function) {
      case FUNC_EVERY_SECOND:
        CseEverySecond();
        break;
      case FUNC_COMMAND:
        result = CseCommand();
        break;
      case FUNC_SERIAL:
        result = CseSerialInput();
        break;
    }
  }
  return result;
}

#endif
#endif
# 1 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xnrg_03_pzem004t.ino"
# 20 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xnrg_03_pzem004t.ino"
#ifdef USE_ENERGY_SENSOR
#ifdef USE_PZEM004T
# 31 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xnrg_03_pzem004t.ino"
#define XNRG_03 3

#include <TasmotaSerial.h>

TasmotaSerial *PzemSerial;

#define PZEM_VOLTAGE (uint8_t)0xB0
#define RESP_VOLTAGE (uint8_t)0xA0

#define PZEM_CURRENT (uint8_t)0xB1
#define RESP_CURRENT (uint8_t)0xA1

#define PZEM_POWER (uint8_t)0xB2
#define RESP_POWER (uint8_t)0xA2

#define PZEM_ENERGY (uint8_t)0xB3
#define RESP_ENERGY (uint8_t)0xA3

#define PZEM_SET_ADDRESS (uint8_t)0xB4
#define RESP_SET_ADDRESS (uint8_t)0xA4

#define PZEM_POWER_ALARM (uint8_t)0xB5
#define RESP_POWER_ALARM (uint8_t)0xA5

#define PZEM_DEFAULT_READ_TIMEOUT 500



struct PZEMCommand {
  uint8_t command;
  uint8_t addr[4];
  uint8_t data;
  uint8_t crc;
};

IPAddress pzem_ip(192, 168, 1, 1);

uint8_t PzemCrc(uint8_t *data)
{
  uint16_t crc = 0;
  for (uint8_t i = 0; i < sizeof(PZEMCommand) -1; i++) crc += *data++;
  return (uint8_t)(crc & 0xFF);
}

void PzemSend(uint8_t cmd)
{
  PZEMCommand pzem;

  pzem.command = cmd;
  for (uint8_t i = 0; i < sizeof(pzem.addr); i++) pzem.addr[i] = pzem_ip[i];
  pzem.data = 0;

  uint8_t *bytes = (uint8_t*)&pzem;
  pzem.crc = PzemCrc(bytes);

  PzemSerial->flush();
  PzemSerial->write(bytes, sizeof(pzem));
}

bool PzemReceiveReady()
{
  return PzemSerial->available() >= (int)sizeof(PZEMCommand);
}

bool PzemRecieve(uint8_t resp, float *data)
{
# 107 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xnrg_03_pzem004t.ino"
  uint8_t buffer[sizeof(PZEMCommand)] = { 0 };

  unsigned long start = millis();
  uint8_t len = 0;
  while ((len < sizeof(PZEMCommand)) && (millis() - start < PZEM_DEFAULT_READ_TIMEOUT)) {
    if (PzemSerial->available() > 0) {
      uint8_t c = (uint8_t)PzemSerial->read();
      if (!c && !len) {
        continue;
      }
      if ((1 == len) && (buffer[0] == c)) {
        len--;
        continue;
      }
      buffer[len++] = c;
    }
  }

  AddLogSerial(LOG_LEVEL_DEBUG_MORE, buffer, len);

  if (len != sizeof(PZEMCommand)) {

    return false;
  }
  if (buffer[6] != PzemCrc(buffer)) {

    return false;
  }
  if (buffer[0] != resp) {

    return false;
  }

  switch (resp) {
    case RESP_VOLTAGE:
      *data = (float)(buffer[1] << 8) + buffer[2] + (buffer[3] / 10.0);
      break;
    case RESP_CURRENT:
      *data = (float)(buffer[1] << 8) + buffer[2] + (buffer[3] / 100.0);
      break;
    case RESP_POWER:
      *data = (float)(buffer[1] << 8) + buffer[2];
      break;
    case RESP_ENERGY:
      *data = (float)((uint32_t)buffer[1] << 16) + ((uint16_t)buffer[2] << 8) + buffer[3];
      break;
  }
  return true;
}



const uint8_t pzem_commands[] { PZEM_SET_ADDRESS, PZEM_VOLTAGE, PZEM_CURRENT, PZEM_POWER, PZEM_ENERGY };
const uint8_t pzem_responses[] { RESP_SET_ADDRESS, RESP_VOLTAGE, RESP_CURRENT, RESP_POWER, RESP_ENERGY };

uint8_t pzem_read_state = 0;
uint8_t pzem_sendRetry = 0;

void PzemEvery200ms()
{
  bool data_ready = PzemReceiveReady();

  if (data_ready) {
    float value = 0;
    if (PzemRecieve(pzem_responses[pzem_read_state], &value)) {
      switch (pzem_read_state) {
        case 1:
          energy_voltage = value;
          break;
        case 2:
          energy_current = value;
          break;
        case 3:
          energy_active_power = value;
          break;
        case 4:
          if (!energy_start || (value < energy_start)) energy_start = value;
          energy_kWhtoday += (value - energy_start) * 100;
          energy_start = value;
          EnergyUpdateToday();
          break;
      }
      pzem_read_state++;
      if (5 == pzem_read_state) pzem_read_state = 1;
    }
  }

  if (0 == pzem_sendRetry || data_ready) {
    pzem_sendRetry = 5;
    PzemSend(pzem_commands[pzem_read_state]);
  }
  else {
    pzem_sendRetry--;
  }
}

void PzemSnsInit()
{

  PzemSerial = new TasmotaSerial(pin[GPIO_PZEM004_RX], pin[GPIO_PZEM0XX_TX], 1);
  if (PzemSerial->begin(9600)) {
    if (PzemSerial->hardwareSerial()) { ClaimSerial(); }
  } else {
    energy_flg = ENERGY_NONE;
  }
}

void PzemDrvInit()
{
  if (!energy_flg) {
    if ((pin[GPIO_PZEM004_RX] < 99) && (pin[GPIO_PZEM0XX_TX] < 99)) {
      energy_flg = XNRG_03;
    }
  }
}





int Xnrg03(byte function)
{
  int result = 0;

  if (FUNC_PRE_INIT == function) {
    PzemDrvInit();
  }
  else if (XNRG_03 == energy_flg) {
    switch (function) {
      case FUNC_INIT:
        PzemSnsInit();
        break;
      case FUNC_EVERY_200_MSECOND:
        PzemEvery200ms();
        break;
    }
  }
  return result;
}

#endif
#endif
# 1 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xnrg_04_mcp39f501.ino"
# 20 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xnrg_04_mcp39f501.ino"
#ifdef USE_ENERGY_SENSOR
#ifdef USE_MCP39F501







#define XNRG_04 4

#define MCP_TIMEOUT 4
#define MCP_CALIBRATION_TIMEOUT 2

#define MCP_CALIBRATE_POWER 0x001
#define MCP_CALIBRATE_VOLTAGE 0x002
#define MCP_CALIBRATE_CURRENT 0x004
#define MCP_CALIBRATE_FREQUENCY 0x008
#define MCP_SINGLE_WIRE_FLAG 0x100

#define MCP_START_FRAME 0xA5
#define MCP_ACK_FRAME 0x06
#define MCP_ERROR_NAK 0x15
#define MCP_ERROR_CRC 0x51

#define MCP_SINGLE_WIRE 0xAB

#define MCP_SET_ADDRESS 0x41

#define MCP_READ 0x4E
#define MCP_READ_16 0x52
#define MCP_READ_32 0x44

#define MCP_WRITE 0x4D
#define MCP_WRITE_16 0x57
#define MCP_WRITE_32 0x45

#define MCP_SAVE_REGISTERS 0x53

#define MCP_CALIBRATION_BASE 0x0028
#define MCP_CALIBRATION_LEN 52

#define MCP_FREQUENCY_REF_BASE 0x0094
#define MCP_FREQUENCY_GAIN_BASE 0x00AE
#define MCP_FREQUENCY_LEN 4

typedef struct mcp_cal_registers_type {
  uint16_t gain_current_rms;
  uint16_t gain_voltage_rms;
  uint16_t gain_active_power;
  uint16_t gain_reactive_power;
  sint32_t offset_current_rms;
  sint32_t offset_active_power;
  sint32_t offset_reactive_power;
  sint16_t dc_offset_current;
  sint16_t phase_compensation;
  uint16_t apparent_power_divisor;

  uint32_t system_configuration;
  uint16_t dio_configuration;
  uint32_t range;

  uint32_t calibration_current;
  uint16_t calibration_voltage;
  uint32_t calibration_active_power;
  uint32_t calibration_reactive_power;
  uint16_t accumulation_interval;
} mcp_cal_registers_type;

unsigned long mcp_kWhcounter = 0;
uint32_t mcp_system_configuration = 0x03000000;
uint32_t mcp_active_power;


uint32_t mcp_current_rms;
uint16_t mcp_voltage_rms;
uint16_t mcp_line_frequency;

uint8_t mcp_address = 0;
uint8_t mcp_calibration_active = 0;
uint8_t mcp_init = 0;
uint8_t mcp_timeout = 0;
uint8_t mcp_calibrate = 0;






uint8_t McpChecksum(uint8_t *data)
{
  uint8_t checksum = 0;
  uint8_t offset = 0;
  uint8_t len = data[1] -1;

  for (byte i = offset; i < len; i++) { checksum += data[i]; }
  return checksum;
}

unsigned long McpExtractInt(char *data, uint8_t offset, uint8_t size)
{
 unsigned long result = 0;
 unsigned long pow = 1;

 for (byte i = 0; i < size; i++) {
  result = result + (uint8_t)data[offset + i] * pow;
  pow = pow * 256;
 }
 return result;
}

void McpSetInt(unsigned long value, uint8_t *data, uint8_t offset, size_t size)
{
 for (byte i = 0; i < size; i++) {
  data[offset + i] = ((value >> (i * 8)) & 0xFF);
 }
}

void McpSend(uint8_t *data)
{
  if (mcp_timeout) { return; }
  mcp_timeout = MCP_TIMEOUT;

  data[0] = MCP_START_FRAME;
  data[data[1] -1] = McpChecksum(data);



  for (byte i = 0; i < data[1]; i++) {
    Serial.write(data[i]);
  }
}



void McpGetAddress(void)
{
  uint8_t data[] = { MCP_START_FRAME, 7, MCP_SET_ADDRESS, 0x00, 0x26, MCP_READ_16, 0x00 };

  McpSend(data);
}

void McpAddressReceive(void)
{

  mcp_address = serial_in_buffer[3];
}



void McpGetCalibration(void)
{
  if (mcp_calibration_active) { return; }
  mcp_calibration_active = MCP_CALIBRATION_TIMEOUT;

  uint8_t data[] = { MCP_START_FRAME, 8, MCP_SET_ADDRESS, (MCP_CALIBRATION_BASE >> 8) & 0xFF, MCP_CALIBRATION_BASE & 0xFF, MCP_READ, MCP_CALIBRATION_LEN, 0x00 };

  McpSend(data);
}

void McpParseCalibration(void)
{
  bool action = false;
  mcp_cal_registers_type cal_registers;


  cal_registers.gain_current_rms = McpExtractInt(serial_in_buffer, 2, 2);
  cal_registers.gain_voltage_rms = McpExtractInt(serial_in_buffer, 4, 2);
  cal_registers.gain_active_power = McpExtractInt(serial_in_buffer, 6, 2);
  cal_registers.gain_reactive_power = McpExtractInt(serial_in_buffer, 8, 2);
  cal_registers.offset_current_rms = McpExtractInt(serial_in_buffer, 10, 4);
  cal_registers.offset_active_power = McpExtractInt(serial_in_buffer, 14, 4);
  cal_registers.offset_reactive_power = McpExtractInt(serial_in_buffer, 18, 4);
  cal_registers.dc_offset_current = McpExtractInt(serial_in_buffer, 22, 2);
  cal_registers.phase_compensation = McpExtractInt(serial_in_buffer, 24, 2);
  cal_registers.apparent_power_divisor = McpExtractInt(serial_in_buffer, 26, 2);

  cal_registers.system_configuration = McpExtractInt(serial_in_buffer, 28, 4);
  cal_registers.dio_configuration = McpExtractInt(serial_in_buffer, 32, 2);
  cal_registers.range = McpExtractInt(serial_in_buffer, 34, 4);

  cal_registers.calibration_current = McpExtractInt(serial_in_buffer, 38, 4);
  cal_registers.calibration_voltage = McpExtractInt(serial_in_buffer, 42, 2);
  cal_registers.calibration_active_power = McpExtractInt(serial_in_buffer, 44, 4);
  cal_registers.calibration_reactive_power = McpExtractInt(serial_in_buffer, 48, 4);
  cal_registers.accumulation_interval = McpExtractInt(serial_in_buffer, 52, 2);

  if (mcp_calibrate & MCP_CALIBRATE_POWER) {
    cal_registers.calibration_active_power = Settings.energy_power_calibration;
    if (McpCalibrationCalc(&cal_registers, 16)) { action = true; }
  }
  if (mcp_calibrate & MCP_CALIBRATE_VOLTAGE) {
    cal_registers.calibration_voltage = Settings.energy_voltage_calibration;
    if (McpCalibrationCalc(&cal_registers, 0)) { action = true; }
  }
  if (mcp_calibrate & MCP_CALIBRATE_CURRENT) {
    cal_registers.calibration_current = Settings.energy_current_calibration;
    if (McpCalibrationCalc(&cal_registers, 8)) { action = true; }
  }
  mcp_timeout = 0;
  if (action) { McpSetCalibration(&cal_registers); }

  mcp_calibrate = 0;

  Settings.energy_power_calibration = cal_registers.calibration_active_power;
  Settings.energy_voltage_calibration = cal_registers.calibration_voltage;
  Settings.energy_current_calibration = cal_registers.calibration_current;

  mcp_system_configuration = cal_registers.system_configuration;

  if (mcp_system_configuration & MCP_SINGLE_WIRE_FLAG) {
    mcp_system_configuration &= ~MCP_SINGLE_WIRE_FLAG;
   McpSetSystemConfiguration(2);
  }
}

bool McpCalibrationCalc(struct mcp_cal_registers_type *cal_registers, uint8_t range_shift)
{
  uint32_t measured;
  uint32_t expected;
  uint16_t *gain;
  uint32_t new_gain;

  if (range_shift == 0) {
    measured = mcp_voltage_rms;
    expected = cal_registers->calibration_voltage;
    gain = &(cal_registers->gain_voltage_rms);
  } else if (range_shift == 8) {
    measured = mcp_current_rms;
    expected = cal_registers->calibration_current;
    gain = &(cal_registers->gain_current_rms);
  } else if (range_shift == 16) {
    measured = mcp_active_power;
    expected = cal_registers->calibration_active_power;
    gain = &(cal_registers->gain_active_power);
  } else {
    return false;
  }

  if (measured == 0) {
    return false;
  }

  uint32_t range = (cal_registers->range >> range_shift) & 0xFF;

calc:
  new_gain = (*gain) * expected / measured;

  if (new_gain < 25000) {
    range++;
    if (measured > 6) {
      measured = measured / 2;
      goto calc;
    }
  }

  if (new_gain > 55000) {
    range--;
    measured = measured * 2;
    goto calc;
  }

  *gain = new_gain;
  uint32_t old_range = (cal_registers->range >> range_shift) & 0xFF;
  cal_registers->range = cal_registers->range ^ (old_range << range_shift);
  cal_registers->range = cal_registers->range | (range << range_shift);

  return true;
}






void McpSetCalibration(struct mcp_cal_registers_type *cal_registers)
{
  uint8_t data[7 + MCP_CALIBRATION_LEN + 2 + 1];

  data[1] = sizeof(data);
  data[2] = MCP_SET_ADDRESS;
  data[3] = (MCP_CALIBRATION_BASE >> 8) & 0xFF;
  data[4] = (MCP_CALIBRATION_BASE >> 0) & 0xFF;

  data[5] = MCP_WRITE;
  data[6] = MCP_CALIBRATION_LEN;

  McpSetInt(cal_registers->gain_current_rms, data, 0+7, 2);
  McpSetInt(cal_registers->gain_voltage_rms, data, 2+7, 2);
  McpSetInt(cal_registers->gain_active_power, data, 4+7, 2);
  McpSetInt(cal_registers->gain_reactive_power, data, 6+7, 2);
  McpSetInt(cal_registers->offset_current_rms, data, 8+7, 4);
  McpSetInt(cal_registers->offset_active_power, data, 12+7, 4);
  McpSetInt(cal_registers->offset_reactive_power, data, 16+7, 4);
  McpSetInt(cal_registers->dc_offset_current, data, 20+7, 2);
  McpSetInt(cal_registers->phase_compensation, data, 22+7, 2);
  McpSetInt(cal_registers->apparent_power_divisor, data, 24+7, 2);

  McpSetInt(cal_registers->system_configuration, data, 26+7, 4);
  McpSetInt(cal_registers->dio_configuration, data, 30+7, 2);
  McpSetInt(cal_registers->range, data, 32+7, 4);

  McpSetInt(cal_registers->calibration_current, data, 36+7, 4);
  McpSetInt(cal_registers->calibration_voltage, data, 40+7, 2);
  McpSetInt(cal_registers->calibration_active_power, data, 42+7, 4);
  McpSetInt(cal_registers->calibration_reactive_power, data, 46+7, 4);
  McpSetInt(cal_registers->accumulation_interval, data, 50+7, 2);

  data[MCP_CALIBRATION_LEN+7] = MCP_SAVE_REGISTERS;
  data[MCP_CALIBRATION_LEN+8] = mcp_address;

  McpSend(data);
}



void McpSetSystemConfiguration(uint16 interval)
{

  uint8_t data[17];

  data[ 1] = sizeof(data);
  data[ 2] = MCP_SET_ADDRESS;
  data[ 3] = 0x00;
  data[ 4] = 0x42;
  data[ 5] = MCP_WRITE_32;
  data[ 6] = (mcp_system_configuration >> 24) & 0xFF;
  data[ 7] = (mcp_system_configuration >> 16) & 0xFF;
  data[ 8] = (mcp_system_configuration >> 8) & 0xFF;
  data[ 9] = (mcp_system_configuration >> 0) & 0xFF;
  data[10] = MCP_SET_ADDRESS;
  data[11] = 0x00;
  data[12] = 0x5A;
  data[13] = MCP_WRITE_16;
  data[14] = (interval >> 8) & 0xFF;
  data[15] = (interval >> 0) & 0xFF;

  McpSend(data);
}



void McpGetFrequency(void)
{
  if (mcp_calibration_active) { return; }
  mcp_calibration_active = MCP_CALIBRATION_TIMEOUT;

  uint8_t data[] = { MCP_START_FRAME, 11, MCP_SET_ADDRESS, (MCP_FREQUENCY_REF_BASE >> 8) & 0xFF, MCP_FREQUENCY_REF_BASE & 0xFF, MCP_READ_16,
                                          MCP_SET_ADDRESS, (MCP_FREQUENCY_GAIN_BASE >> 8) & 0xFF, MCP_FREQUENCY_GAIN_BASE & 0xFF, MCP_READ_16, 0x00 };

  McpSend(data);
}

void McpParseFrequency(void)
{

  uint16_t line_frequency_ref = serial_in_buffer[2] * 256 + serial_in_buffer[3];
  uint16_t gain_line_frequency = serial_in_buffer[4] * 256 + serial_in_buffer[5];

  if (mcp_calibrate & MCP_CALIBRATE_FREQUENCY) {
    line_frequency_ref = Settings.energy_frequency_calibration;

    if ((0xFFFF == mcp_line_frequency) || (0 == gain_line_frequency)) {
      mcp_line_frequency = 50000;
      gain_line_frequency = 0x8000;
    }
    gain_line_frequency = gain_line_frequency * line_frequency_ref / mcp_line_frequency;

    mcp_timeout = 0;
    McpSetFrequency(line_frequency_ref, gain_line_frequency);
  }

  Settings.energy_frequency_calibration = line_frequency_ref;

  mcp_calibrate = 0;
}

void McpSetFrequency(uint16_t line_frequency_ref, uint16_t gain_line_frequency)
{

  uint8_t data[17];

  data[ 1] = sizeof(data);
  data[ 2] = MCP_SET_ADDRESS;
  data[ 3] = (MCP_FREQUENCY_REF_BASE >> 8) & 0xFF;
  data[ 4] = (MCP_FREQUENCY_REF_BASE >> 0) & 0xFF;

  data[ 5] = MCP_WRITE_16;
  data[ 6] = (line_frequency_ref >> 8) & 0xFF;
  data[ 7] = (line_frequency_ref >> 0) & 0xFF;

  data[ 8] = MCP_SET_ADDRESS;
  data[ 9] = (MCP_FREQUENCY_GAIN_BASE >> 8) & 0xFF;
  data[10] = (MCP_FREQUENCY_GAIN_BASE >> 0) & 0xFF;

  data[11] = MCP_WRITE_16;
  data[12] = (gain_line_frequency >> 8) & 0xFF;
  data[13] = (gain_line_frequency >> 0) & 0xFF;

  data[14] = MCP_SAVE_REGISTERS;
  data[15] = mcp_address;

  McpSend(data);
}



void McpGetData(void)
{
  uint8_t data[] = { MCP_START_FRAME, 8, MCP_SET_ADDRESS, 0x00, 0x04, MCP_READ, 22, 0x00 };

  McpSend(data);
}

void McpParseData(void)
{





  mcp_current_rms = McpExtractInt(serial_in_buffer, 2, 4);
  mcp_voltage_rms = McpExtractInt(serial_in_buffer, 6, 2);
  mcp_active_power = McpExtractInt(serial_in_buffer, 8, 4);


  mcp_line_frequency = McpExtractInt(serial_in_buffer, 22, 2);

  if (energy_power_on) {
    energy_frequency = (float)mcp_line_frequency / 1000;
    energy_voltage = (float)mcp_voltage_rms / 10;
    energy_active_power = (float)mcp_active_power / 100;
    if (0 == energy_active_power) {
      energy_current = 0;
    } else {
      energy_current = (float)mcp_current_rms / 10000;
    }
  } else {
    energy_frequency = 0;
    energy_voltage = 0;
    energy_active_power = 0;
    energy_current = 0;
  }
}



bool McpSerialInput(void)
{
  serial_in_buffer[serial_in_byte_counter++] = serial_in_byte;
  unsigned long start = millis();
  while (millis() - start < 20) {
    yield();
    if (Serial.available()) {
      serial_in_buffer[serial_in_byte_counter++] = Serial.read();
      start = millis();
    }
  }

  AddLogSerial(LOG_LEVEL_DEBUG_MORE);

  if (1 == serial_in_byte_counter) {
    if (MCP_ERROR_CRC == serial_in_buffer[0]) {

      mcp_timeout = 0;
    }
    else if (MCP_ERROR_NAK == serial_in_buffer[0]) {

      mcp_timeout = 0;
    }
  }
  else if (MCP_ACK_FRAME == serial_in_buffer[0]) {
    if (serial_in_byte_counter == serial_in_buffer[1]) {

      if (McpChecksum((uint8_t *)serial_in_buffer) != serial_in_buffer[serial_in_byte_counter -1]) {
        AddLog_P(LOG_LEVEL_DEBUG, PSTR("MCP: " D_CHECKSUM_FAILURE));
      } else {
        if (5 == serial_in_buffer[1]) { McpAddressReceive(); }
        if (25 == serial_in_buffer[1]) { McpParseData(); }
        if (MCP_CALIBRATION_LEN + 3 == serial_in_buffer[1]) { McpParseCalibration(); }
        if (MCP_FREQUENCY_LEN + 3 == serial_in_buffer[1]) { McpParseFrequency(); }
      }

    }
    mcp_timeout = 0;
  }
  else if (MCP_SINGLE_WIRE == serial_in_buffer[0]) {
    mcp_timeout = 0;
  }
  return 1;
}



void McpEverySecond(void)
{
  if (mcp_active_power) {
    energy_kWhtoday_delta += ((mcp_active_power * 10) / 36);
    EnergyUpdateToday();
  }

  if (mcp_timeout) {
    mcp_timeout--;
  }
  else if (mcp_calibration_active) {
    mcp_calibration_active--;
  }
  else if (mcp_init) {
    if (2 == mcp_init) {
      McpGetCalibration();
    }
    else if (1 == mcp_init) {
      McpGetFrequency();
    }
    mcp_init--;
  }
  else if (!mcp_address) {
    McpGetAddress();
  }
  else {
    McpGetData();
  }
}

void McpSnsInit(void)
{
  SetSeriallog(LOG_LEVEL_NONE);
  digitalWrite(15, 1);
}

void McpDrvInit(void)
{
  if (!energy_flg) {
    if (SHELLY2 == Settings.module) {
      pinMode(15, OUTPUT);
      digitalWrite(15, 0);
      baudrate = 4800;
      mcp_calibrate = 0;
      mcp_timeout = 2;
      mcp_init = 2;
      energy_flg = XNRG_04;
    }
  }
}

boolean McpCommand(void)
{
  boolean serviced = true;
  unsigned long value = 0;

  if (CMND_POWERSET == energy_command_code) {
    if (XdrvMailbox.data_len && mcp_active_power) {
      value = (unsigned long)(CharToDouble(XdrvMailbox.data) * 100);
      if ((value > 100) && (value < 200000)) {
        Settings.energy_power_calibration = value;
        mcp_calibrate |= MCP_CALIBRATE_POWER;
        McpGetCalibration();
      }
    }
  }
  else if (CMND_VOLTAGESET == energy_command_code) {
    if (XdrvMailbox.data_len && mcp_voltage_rms) {
      value = (unsigned long)(CharToDouble(XdrvMailbox.data) * 10);
      if ((value > 1000) && (value < 2600)) {
        Settings.energy_voltage_calibration = value;
        mcp_calibrate |= MCP_CALIBRATE_VOLTAGE;
        McpGetCalibration();
      }
    }
  }
  else if (CMND_CURRENTSET == energy_command_code) {
    if (XdrvMailbox.data_len && mcp_current_rms) {
      value = (unsigned long)(CharToDouble(XdrvMailbox.data) * 10);
      if ((value > 100) && (value < 80000)) {
        Settings.energy_current_calibration = value;
        mcp_calibrate |= MCP_CALIBRATE_CURRENT;
        McpGetCalibration();
      }
    }
  }
  else if (CMND_FREQUENCYSET == energy_command_code) {
    if (XdrvMailbox.data_len && mcp_line_frequency) {
      value = (unsigned long)(CharToDouble(XdrvMailbox.data) * 1000);
      if ((value > 45000) && (value < 65000)) {
        Settings.energy_frequency_calibration = value;
        mcp_calibrate |= MCP_CALIBRATE_FREQUENCY;
        McpGetFrequency();
      }
    }
  }
  else serviced = false;

  return serviced;
}





int Xnrg04(byte function)
{
  int result = 0;

  if (FUNC_PRE_INIT == function) {
    McpDrvInit();
  }
  else if (XNRG_04 == energy_flg) {
    switch (function) {
      case FUNC_INIT:
        McpSnsInit();
        break;
      case FUNC_EVERY_SECOND:
        McpEverySecond();
        break;
      case FUNC_COMMAND:
        result = McpCommand();
        break;
      case FUNC_SERIAL:
        result = McpSerialInput();
        break;
    }
  }
  return result;
}

#endif
#endif
# 1 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xnrg_05_pzem_ac.ino"
# 20 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xnrg_05_pzem_ac.ino"
#ifdef USE_ENERGY_SENSOR
#ifdef USE_PZEM_AC
# 32 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xnrg_05_pzem_ac.ino"
#define XNRG_05 5

#define PZEM_AC_DEVICE_ADDRESS 0x01

#include <TasmotaModbus.h>
TasmotaModbus *PzemAcModbus;

void PzemAcEverySecond()
{
  static uint8_t send_retry = 0;

  bool data_ready = PzemAcModbus->ReceiveReady();

  if (data_ready) {
    uint8_t buffer[26];

    uint8_t error = PzemAcModbus->ReceiveBuffer(buffer, 10);
    AddLogSerial(LOG_LEVEL_DEBUG_MORE, buffer, (buffer[2]) ? buffer[2] +5 : sizeof(buffer));

    if (error) {
      snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_DEBUG "PzemAc response error %d"), error);
      AddLog(LOG_LEVEL_DEBUG);
    } else {



      energy_voltage = (float)((buffer[3] << 8) + buffer[4]) / 10.0;
      energy_current = (float)((buffer[7] << 24) + (buffer[8] << 16) + (buffer[5] << 8) + buffer[6]) / 1000.0;
      energy_active_power = (float)((buffer[11] << 24) + (buffer[12] << 16) + (buffer[9] << 8) + buffer[10]) / 10.0;
      energy_frequency = (float)((buffer[17] << 8) + buffer[18]) / 10.0;
      energy_power_factor = (float)((buffer[19] << 8) + buffer[20]) / 100.0;
      float energy = (float)((buffer[15] << 24) + (buffer[16] << 16) + (buffer[13] << 8) + buffer[14]);

      if (!energy_start || (energy < energy_start)) { energy_start = energy; }
      energy_kWhtoday += (energy - energy_start) * 100;
      energy_start = energy;
      EnergyUpdateToday();
    }
  }

  if (0 == send_retry || data_ready) {
    send_retry = 5;
    PzemAcModbus->Send(PZEM_AC_DEVICE_ADDRESS, 0x04, 0, 10);
  }
  else {
    send_retry--;
  }
}

void PzemAcSnsInit()
{
  PzemAcModbus = new TasmotaModbus(pin[GPIO_PZEM016_RX], pin[GPIO_PZEM0XX_TX]);
  uint8_t result = PzemAcModbus->Begin(9600);
  if (result) {
    if (2 == result) { ClaimSerial(); }
  } else {
    energy_flg = ENERGY_NONE;
  }
}

void PzemAcDrvInit()
{
  if (!energy_flg) {
    if ((pin[GPIO_PZEM016_RX] < 99) && (pin[GPIO_PZEM0XX_TX] < 99)) {
      energy_flg = XNRG_05;
    }
  }
}





int Xnrg05(byte function)
{
  int result = 0;

  if (FUNC_PRE_INIT == function) {
    PzemAcDrvInit();
  }
  else if (XNRG_05 == energy_flg) {
    switch (function) {
      case FUNC_INIT:
        PzemAcSnsInit();
        break;
      case FUNC_EVERY_SECOND:
        PzemAcEverySecond();
        break;
    }
  }
  return result;
}

#endif
#endif
# 1 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xnrg_06_pzem_dc.ino"
# 20 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xnrg_06_pzem_dc.ino"
#ifdef USE_ENERGY_SENSOR
#ifdef USE_PZEM_DC
# 32 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xnrg_06_pzem_dc.ino"
#define XNRG_06 6

#define PZEM_DC_DEVICE_ADDRESS 0x01

#include <TasmotaModbus.h>
TasmotaModbus *PzemDcModbus;

void PzemDcEverySecond()
{
  static uint8_t send_retry = 0;

  bool data_ready = PzemDcModbus->ReceiveReady();

  if (data_ready) {
    uint8_t buffer[22];

    uint8_t error = PzemDcModbus->ReceiveBuffer(buffer, 8);
    AddLogSerial(LOG_LEVEL_DEBUG_MORE, buffer, (buffer[2]) ? buffer[2] +5 : sizeof(buffer));

    if (error) {
      snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_DEBUG "PzemDc response error %d"), error);
      AddLog(LOG_LEVEL_DEBUG);
    } else {



      energy_voltage = (float)((buffer[3] << 8) + buffer[4]) / 100.0;
      energy_current = (float)((buffer[5] << 8) + buffer[6]) / 100.0;
      energy_active_power = (float)((buffer[9] << 24) + (buffer[10] << 16) + (buffer[7] << 8) + buffer[8]) / 10.0;
      float energy = (float)((buffer[13] << 24) + (buffer[14] << 16) + (buffer[11] << 8) + buffer[12]);

      if (!energy_start || (energy < energy_start)) { energy_start = energy; }
      energy_kWhtoday += (energy - energy_start) * 100;
      energy_start = energy;
      EnergyUpdateToday();
    }
  }

  if (0 == send_retry || data_ready) {
    send_retry = 5;
    PzemDcModbus->Send(PZEM_DC_DEVICE_ADDRESS, 0x04, 0, 8);
  }
  else {
    send_retry--;
  }
}

void PzemDcSnsInit()
{
  PzemDcModbus = new TasmotaModbus(pin[GPIO_PZEM017_RX], pin[GPIO_PZEM0XX_TX]);
  uint8_t result = PzemDcModbus->Begin(9600, 2);
  if (result) {
    if (2 == result) { ClaimSerial(); }
    energy_type_dc = true;
  } else {
    energy_flg = ENERGY_NONE;
  }
}

void PzemDcDrvInit()
{
  if (!energy_flg) {
    if ((pin[GPIO_PZEM017_RX] < 99) && (pin[GPIO_PZEM0XX_TX] < 99)) {
      energy_flg = XNRG_06;
    }
  }
}





int Xnrg06(byte function)
{
  int result = 0;

  if (FUNC_PRE_INIT == function) {
    PzemDcDrvInit();
  }
  else if (XNRG_06 == energy_flg) {
    switch (function) {
      case FUNC_INIT:
        PzemDcSnsInit();
        break;
      case FUNC_EVERY_SECOND:
        PzemDcEverySecond();
        break;
    }
  }
  return result;
}

#endif
#endif
# 1 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xnrg_interface.ino"
# 20 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xnrg_interface.ino"
int (* const xnrg_func_ptr[])(byte) PROGMEM = {
#ifdef XNRG_01
  &Xnrg01,
#endif

#ifdef XNRG_02
  &Xnrg02,
#endif

#ifdef XNRG_03
  &Xnrg03,
#endif

#ifdef XNRG_04
  &Xnrg04,
#endif

#ifdef XNRG_05
  &Xnrg05,
#endif

#ifdef XNRG_06
  &Xnrg06,
#endif

#ifdef XNRG_07
  &Xnrg07,
#endif

#ifdef XNRG_08
  &Xnrg08,
#endif

#ifdef XNRG_09
  &Xnrg09,
#endif

#ifdef XNRG_10
  &Xnrg10,
#endif

#ifdef XNRG_11
  &Xnrg11,
#endif

#ifdef XNRG_12
  &Xnrg12,
#endif

#ifdef XNRG_13
  &Xnrg13,
#endif

#ifdef XNRG_14
  &Xnrg14,
#endif

#ifdef XNRG_15
  &Xnrg15,
#endif

#ifdef XNRG_16
  &Xnrg16
#endif
};

const uint8_t xnrg_present = sizeof(xnrg_func_ptr) / sizeof(xnrg_func_ptr[0]);

int XnrgCall(byte Function)
{
  int result = 0;

  for (byte x = 0; x < xnrg_present; x++) {
    result = xnrg_func_ptr[x](Function);
    if (result) break;
  }
  return result;
}
# 1 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xplg_wemohue.ino"
# 20 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xplg_wemohue.ino"
#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))

#if defined(USE_WEBSERVER) && defined(USE_EMULATION)




#define UDP_BUFFER_SIZE 200
#define UDP_MSEARCH_SEND_DELAY 1500

#include <Ticker.h>
Ticker TickerMSearch;

boolean udp_connected = false;

char packet_buffer[UDP_BUFFER_SIZE];
IPAddress ipMulticast(239,255,255,250);
uint32_t port_multicast = 1900;

bool udp_response_mutex = false;
IPAddress udp_remote_ip;
uint16_t udp_remote_port;





const char WEMO_MSEARCH[] PROGMEM =
  "HTTP/1.1 200 OK\r\n"
  "CACHE-CONTROL: max-age=86400\r\n"
  "DATE: Fri, 15 Apr 2016 04:56:29 GMT\r\n"
  "EXT:\r\n"
  "LOCATION: http://{r1:80/setup.xml\r\n"
  "OPT: \"http://schemas.upnp.org/upnp/1/0/\"; ns=01\r\n"
  "01-NLS: b9200ebb-736d-4b93-bf03-835149d13983\r\n"
  "SERVER: Unspecified, UPnP/1.0, Unspecified\r\n"
  "ST: {r3\r\n"
  "USN: uuid:{r2::{r3\r\n"
  "X-User-Agent: redsonic\r\n"
  "\r\n";

String WemoSerialnumber()
{
  char serial[16];

  snprintf_P(serial, sizeof(serial), PSTR("201612K%08X"), ESP.getChipId());
  return String(serial);
}

String WemoUuid()
{
  char uuid[27];

  snprintf_P(uuid, sizeof(uuid), PSTR("Socket-1_0-%s"), WemoSerialnumber().c_str());
  return String(uuid);
}

void WemoRespondToMSearch(int echo_type)
{
  char message[TOPSZ];

  TickerMSearch.detach();
  if (PortUdp.beginPacket(udp_remote_ip, udp_remote_port)) {
    String response = FPSTR(WEMO_MSEARCH);
    response.replace("{r1", WiFi.localIP().toString());
    response.replace("{r2", WemoUuid());
    if (1 == echo_type) {
      response.replace("{r3", F("urn:Belkin:device:**"));
    } else {
      response.replace("{r3", F("upnp:rootdevice"));
    }
    PortUdp.write(response.c_str());
    PortUdp.endPacket();
    snprintf_P(message, sizeof(message), PSTR(D_RESPONSE_SENT));
  } else {
    snprintf_P(message, sizeof(message), PSTR(D_FAILED_TO_SEND_RESPONSE));
  }
  snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_UPNP D_WEMO " " D_JSON_TYPE " %d, %s " D_TO " %s:%d"),
    echo_type, message, udp_remote_ip.toString().c_str(), udp_remote_port);
  AddLog(LOG_LEVEL_DEBUG);

  udp_response_mutex = false;
}
# 113 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xplg_wemohue.ino"
const char HUE_RESPONSE[] PROGMEM =
  "HTTP/1.1 200 OK\r\n"
  "HOST: 239.255.255.250:1900\r\n"
  "CACHE-CONTROL: max-age=100\r\n"
  "EXT:\r\n"
  "LOCATION: http://{r1:80/description.xml\r\n"
  "SERVER: Linux/3.14.0 UPnP/1.0 IpBridge/1.17.0\r\n"
  "hue-bridgeid: {r2\r\n";
const char HUE_ST1[] PROGMEM =
  "ST: upnp:rootdevice\r\n"
  "USN: uuid:{r3::upnp:rootdevice\r\n"
  "\r\n";
const char HUE_ST2[] PROGMEM =
  "ST: uuid:{r3\r\n"
  "USN: uuid:{r3\r\n"
  "\r\n";
const char HUE_ST3[] PROGMEM =
  "ST: urn:schemas-upnp-org:device:basic:1\r\n"
  "USN: uuid:{r3\r\n"
  "\r\n";

String HueBridgeId()
{
  String temp = WiFi.macAddress();
  temp.replace(":", "");
  String bridgeid = temp.substring(0, 6) + "FFFE" + temp.substring(6);
  return bridgeid;
}

String HueSerialnumber()
{
  String serial = WiFi.macAddress();
  serial.replace(":", "");
  serial.toLowerCase();
  return serial;
}

String HueUuid()
{
  String uuid = F("f6543a06-da50-11ba-8d8f-");
  uuid += HueSerialnumber();
  return uuid;
}

void HueRespondToMSearch()
{
  char message[TOPSZ];

  TickerMSearch.detach();
  if (PortUdp.beginPacket(udp_remote_ip, udp_remote_port)) {
    String response1 = FPSTR(HUE_RESPONSE);
    response1.replace("{r1", WiFi.localIP().toString());
    response1.replace("{r2", HueBridgeId());

    String response = response1;
    response += FPSTR(HUE_ST1);
    response.replace("{r3", HueUuid());
    PortUdp.write(response.c_str());
    PortUdp.endPacket();

    response = response1;
    response += FPSTR(HUE_ST2);
    response.replace("{r3", HueUuid());
    PortUdp.write(response.c_str());
    PortUdp.endPacket();

    response = response1;
    response += FPSTR(HUE_ST3);
    response.replace("{r3", HueUuid());
    PortUdp.write(response.c_str());
    PortUdp.endPacket();

    snprintf_P(message, sizeof(message), PSTR(D_3_RESPONSE_PACKETS_SENT));
  } else {
    snprintf_P(message, sizeof(message), PSTR(D_FAILED_TO_SEND_RESPONSE));
  }
  snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_UPNP D_HUE " %s " D_TO " %s:%d"),
    message, udp_remote_ip.toString().c_str(), udp_remote_port);
  AddLog(LOG_LEVEL_DEBUG);

  udp_response_mutex = false;
}





boolean UdpDisconnect()
{
  if (udp_connected) {
    WiFiUDP::stopAll();
    AddLog_P(LOG_LEVEL_DEBUG, PSTR(D_LOG_UPNP D_MULTICAST_DISABLED));
    udp_connected = false;
  }
  return udp_connected;
}

boolean UdpConnect()
{
  if (!udp_connected) {
    if (PortUdp.beginMulticast(WiFi.localIP(), ipMulticast, port_multicast)) {
      AddLog_P(LOG_LEVEL_INFO, PSTR(D_LOG_UPNP D_MULTICAST_REJOINED));
      udp_response_mutex = false;
      udp_connected = true;
    } else {
      AddLog_P(LOG_LEVEL_INFO, PSTR(D_LOG_UPNP D_MULTICAST_JOIN_FAILED));
      udp_connected = false;
    }
  }
  return udp_connected;
}

void PollUdp()
{
  if (udp_connected && !udp_response_mutex) {
    if (PortUdp.parsePacket()) {
      int len = PortUdp.read(packet_buffer, UDP_BUFFER_SIZE -1);
      if (len > 0) {
        packet_buffer[len] = 0;
      }
      String request = packet_buffer;




      if (request.indexOf("M-SEARCH") >= 0) {
        request.toLowerCase();
        request.replace(" ", "");




        udp_remote_ip = PortUdp.remoteIP();
        udp_remote_port = PortUdp.remotePort();
        if (EMUL_WEMO == Settings.flag2.emulation) {
          if (request.indexOf(F("urn:belkin:device:**")) > 0) {
            udp_response_mutex = true;
            TickerMSearch.attach_ms(UDP_MSEARCH_SEND_DELAY, WemoRespondToMSearch, 1);
          }
          else if ((request.indexOf(F("upnp:rootdevice")) > 0) ||
                   (request.indexOf(F("ssdpsearch:all")) > 0) ||
                   (request.indexOf(F("ssdp:all")) > 0)) {
            udp_response_mutex = true;
            TickerMSearch.attach_ms(UDP_MSEARCH_SEND_DELAY, WemoRespondToMSearch, 2);
          }
        }
        else if ((EMUL_HUE == Settings.flag2.emulation) &&
                ((request.indexOf(F("urn:schemas-upnp-org:device:basic:1")) > 0) ||
                 (request.indexOf(F("upnp:rootdevice")) > 0) ||
                 (request.indexOf(F("ssdpsearch:all")) > 0) ||
                 (request.indexOf(F("ssdp:all")) > 0))) {
            udp_response_mutex = true;
            TickerMSearch.attach_ms(UDP_MSEARCH_SEND_DELAY, HueRespondToMSearch);
        }
      }
    }
  }
}





const char WEMO_EVENTSERVICE_XML[] PROGMEM =
  "<scpd xmlns=\"urn:Belkin:service-1-0\">"
    "<actionList>"
      "<action>"
        "<name>SetBinaryState</name>"
        "<argumentList>"
          "<argument>"
            "<retval/>"
            "<name>BinaryState</name>"
            "<relatedStateVariable>BinaryState</relatedStateVariable>"
            "<direction>in</direction>"
          "</argument>"
        "</argumentList>"
      "</action>"
      "<action>"
        "<name>GetBinaryState</name>"
        "<argumentList>"
          "<argument>"
            "<retval/>"
            "<name>BinaryState</name>"
            "<relatedStateVariable>BinaryState</relatedStateVariable>"
            "<direction>out</direction>"
          "</argument>"
        "</argumentList>"
      "</action>"
    "</actionList>"
    "<serviceStateTable>"
      "<stateVariable sendEvents=\"yes\">"
        "<name>BinaryState</name>"
        "<dataType>Boolean</dataType>"
        "<defaultValue>0</defaultValue>"
      "</stateVariable>"
      "<stateVariable sendEvents=\"yes\">"
        "<name>level</name>"
        "<dataType>string</dataType>"
        "<defaultValue>0</defaultValue>"
      "</stateVariable>"
    "</serviceStateTable>"
  "</scpd>\r\n\r\n";

const char WEMO_METASERVICE_XML[] PROGMEM =
  "<scpd xmlns=\"urn:Belkin:service-1-0\">"
    "<specVersion>"
      "<major>1</major>"
      "<minor>0</minor>"
    "</specVersion>"
    "<actionList>"
      "<action>"
        "<name>GetMetaInfo</name>"
        "<argumentList>"
          "<retval />"
          "<name>GetMetaInfo</name>"
          "<relatedStateVariable>MetaInfo</relatedStateVariable>"
          "<direction>in</direction>"
        "</argumentList>"
      "</action>"
    "</actionList>"
    "<serviceStateTable>"
      "<stateVariable sendEvents=\"yes\">"
        "<name>MetaInfo</name>"
        "<dataType>string</dataType>"
        "<defaultValue>0</defaultValue>"
      "</stateVariable>"
    "</serviceStateTable>"
  "</scpd>\r\n\r\n";

const char WEMO_RESPONSE_STATE_SOAP[] PROGMEM =
  "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
    "<s:Body>"
      "<u:SetBinaryStateResponse xmlns:u=\"urn:Belkin:service:basicevent:1\">"
        "<BinaryState>{x1</BinaryState>"
      "</u:SetBinaryStateResponse>"
    "</s:Body>"
  "</s:Envelope>\r\n";

const char WEMO_SETUP_XML[] PROGMEM =
  "<?xml version=\"1.0\"?>"
  "<root xmlns=\"urn:Belkin:device-1-0\">"
    "<device>"
      "<deviceType>urn:Belkin:device:controllee:1</deviceType>"
      "<friendlyName>{x1</friendlyName>"
      "<manufacturer>Belkin International Inc.</manufacturer>"
      "<modelName>Socket</modelName>"
      "<modelNumber>3.1415</modelNumber>"
      "<UDN>uuid:{x2</UDN>"
      "<serialNumber>{x3</serialNumber>"
      "<binaryState>0</binaryState>"
      "<serviceList>"
        "<service>"
          "<serviceType>urn:Belkin:service:basicevent:1</serviceType>"
          "<serviceId>urn:Belkin:serviceId:basicevent1</serviceId>"
          "<controlURL>/upnp/control/basicevent1</controlURL>"
          "<eventSubURL>/upnp/event/basicevent1</eventSubURL>"
          "<SCPDURL>/eventservice.xml</SCPDURL>"
        "</service>"
        "<service>"
          "<serviceType>urn:Belkin:service:metainfo:1</serviceType>"
          "<serviceId>urn:Belkin:serviceId:metainfo1</serviceId>"
          "<controlURL>/upnp/control/metainfo1</controlURL>"
          "<eventSubURL>/upnp/event/metainfo1</eventSubURL>"
          "<SCPDURL>/metainfoservice.xml</SCPDURL>"
        "</service>"
      "</serviceList>"
    "</device>"
  "</root>\r\n";



void HandleUpnpEvent()
{
  AddLog_P(LOG_LEVEL_DEBUG, S_LOG_HTTP, PSTR(D_WEMO_BASIC_EVENT));

  String request = WebServer->arg(0);
  String state_xml = FPSTR(WEMO_RESPONSE_STATE_SOAP);

  if (request.indexOf(F("SetBinaryState")) > 0) {
    uint8_t power = POWER_TOGGLE;
    if (request.indexOf(F("State>1</Binary")) > 0) {
      power = POWER_ON;
    }
    else if (request.indexOf(F("State>0</Binary")) > 0) {
      power = POWER_OFF;
    }
    if (power != POWER_TOGGLE) {
      uint8_t device = (light_type) ? devices_present : 1;
      ExecuteCommandPower(device, power, SRC_WEMO);
    }
  }
  else if(request.indexOf(F("GetBinaryState")) > 0){
    state_xml.replace(F("Set"), F("Get"));
  }
  state_xml.replace("{x1", String(bitRead(power, devices_present -1)));
  WebServer->send(200, FPSTR(HDR_CTYPE_XML), state_xml);
}

void HandleUpnpService()
{
  AddLog_P(LOG_LEVEL_DEBUG, S_LOG_HTTP, PSTR(D_WEMO_EVENT_SERVICE));

  WebServer->send(200, FPSTR(HDR_CTYPE_PLAIN), FPSTR(WEMO_EVENTSERVICE_XML));
}

void HandleUpnpMetaService()
{
  AddLog_P(LOG_LEVEL_DEBUG, S_LOG_HTTP, PSTR(D_WEMO_META_SERVICE));

  WebServer->send(200, FPSTR(HDR_CTYPE_PLAIN), FPSTR(WEMO_METASERVICE_XML));
}

void HandleUpnpSetupWemo()
{
  AddLog_P(LOG_LEVEL_DEBUG, S_LOG_HTTP, PSTR(D_WEMO_SETUP));

  String setup_xml = FPSTR(WEMO_SETUP_XML);
  setup_xml.replace("{x1", Settings.friendlyname[0]);
  setup_xml.replace("{x2", WemoUuid());
  setup_xml.replace("{x3", WemoSerialnumber());
  WebServer->send(200, FPSTR(HDR_CTYPE_XML), setup_xml);
}





const char HUE_DESCRIPTION_XML[] PROGMEM =
  "<?xml version=\"1.0\"?>"
  "<root xmlns=\"urn:schemas-upnp-org:device-1-0\">"
  "<specVersion>"
    "<major>1</major>"
    "<minor>0</minor>"
  "</specVersion>"

  "<URLBase>http://{x1:80/</URLBase>"
  "<device>"
    "<deviceType>urn:schemas-upnp-org:device:Basic:1</deviceType>"
    "<friendlyName>Amazon-Echo-HA-Bridge ({x1)</friendlyName>"

    "<manufacturer>Royal Philips Electronics</manufacturer>"
    "<modelDescription>Philips hue Personal Wireless Lighting</modelDescription>"
    "<modelName>Philips hue bridge 2012</modelName>"
    "<modelNumber>929000226503</modelNumber>"
    "<serialNumber>{x3</serialNumber>"
    "<UDN>uuid:{x2</UDN>"
  "</device>"
  "</root>\r\n"
  "\r\n";
const char HUE_LIGHTS_STATUS_JSON[] PROGMEM =
  "{\"on\":{state},"
  "\"bri\":{b},"
  "\"hue\":{h},"
  "\"sat\":{s},"
  "\"xy\":[0.5, 0.5],"
  "\"ct\":{t},"
  "\"alert\":\"none\","
  "\"effect\":\"none\","
  "\"colormode\":\"{m}\","
  "\"reachable\":true}";
const char HUE_LIGHTS_STATUS_JSON2[] PROGMEM =
  ",\"type\":\"Extended color light\","
  "\"name\":\"{j1\","
  "\"modelid\":\"LCT007\","
  "\"uniqueid\":\"{j2\","
  "\"swversion\":\"5.50.1.19085\"}";
const char HUE_GROUP0_STATUS_JSON[] PROGMEM =
  "{\"name\":\"Group 0\","
   "\"lights\":[{l1],"
   "\"type\":\"LightGroup\","
   "\"action\":";

const char HueConfigResponse_JSON[] PROGMEM =
  "{\"name\":\"Philips hue\","
   "\"mac\":\"{ma\","
   "\"dhcp\":true,"
   "\"ipaddress\":\"{ip\","
   "\"netmask\":\"{ms\","
   "\"gateway\":\"{gw\","
   "\"proxyaddress\":\"none\","
   "\"proxyport\":0,"
   "\"bridgeid\":\"{br\","
   "\"UTC\":\"{dt\","
   "\"whitelist\":{\"{id\":{"
     "\"last use date\":\"{dt\","
     "\"create date\":\"{dt\","
     "\"name\":\"Remote\"}},"
   "\"swversion\":\"01041302\","
   "\"apiversion\":\"1.17.0\","
   "\"swupdate\":{\"updatestate\":0,\"url\":\"\",\"text\":\"\",\"notify\": false},"
   "\"linkbutton\":false,"
   "\"portalservices\":false"
  "}";
const char HUE_LIGHT_RESPONSE_JSON[] PROGMEM =
  "{\"success\":{\"/lights/{id/state/{cm\":{re}}";
const char HUE_ERROR_JSON[] PROGMEM =
  "[{\"error\":{\"type\":901,\"address\":\"/\",\"description\":\"Internal Error\"}}]";



String GetHueDeviceId(uint8_t id)
{
  String deviceid = WiFi.macAddress() + F(":00:11-") + String(id);
  deviceid.toLowerCase();
  return deviceid;
}

String GetHueUserId()
{
  char userid[7];

  snprintf_P(userid, sizeof(userid), PSTR("%03x"), ESP.getChipId());
  return String(userid);
}

void HandleUpnpSetupHue()
{
  AddLog_P(LOG_LEVEL_DEBUG, S_LOG_HTTP, PSTR(D_HUE_BRIDGE_SETUP));
  String description_xml = FPSTR(HUE_DESCRIPTION_XML);
  description_xml.replace("{x1", WiFi.localIP().toString());
  description_xml.replace("{x2", HueUuid());
  description_xml.replace("{x3", HueSerialnumber());
  WebServer->send(200, FPSTR(HDR_CTYPE_XML), description_xml);
}

void HueNotImplemented(String *path)
{
  snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_HTTP D_HUE_API_NOT_IMPLEMENTED " (%s)"), path->c_str());
  AddLog(LOG_LEVEL_DEBUG_MORE);

  WebServer->send(200, FPSTR(HDR_CTYPE_JSON), "{}");
}

void HueConfigResponse(String *response)
{
  *response += FPSTR(HueConfigResponse_JSON);
  response->replace("{ma", WiFi.macAddress());
  response->replace("{ip", WiFi.localIP().toString());
  response->replace("{ms", WiFi.subnetMask().toString());
  response->replace("{gw", WiFi.gatewayIP().toString());
  response->replace("{br", HueBridgeId());
  response->replace("{dt", GetDateAndTime(DT_UTC));
  response->replace("{id", GetHueUserId());
}

void HueConfig(String *path)
{
  String response = "";
  HueConfigResponse(&response);
  WebServer->send(200, FPSTR(HDR_CTYPE_JSON), response);
}

bool g_gotct = false;

void HueLightStatus1(byte device, String *response)
{
  float hue = 0;
  float sat = 0;
  float bri = 0;
  uint16_t ct = 500;

  if (light_type) {
    LightGetHsb(&hue, &sat, &bri, g_gotct);
    ct = LightGetColorTemp();
  }
  *response += FPSTR(HUE_LIGHTS_STATUS_JSON);
  response->replace("{state}", (power & (1 << (device-1))) ? "true" : "false");
  response->replace("{h}", String((uint16_t)(65535.0f * hue)));
  response->replace("{s}", String((uint8_t)(254.0f * sat)));
  response->replace("{b}", String((uint8_t)(254.0f * bri)));
  response->replace("{t}", String(ct));
  response->replace("{m}", g_gotct?"ct":"hs");
}

void HueLightStatus2(byte device, String *response)
{
  *response += FPSTR(HUE_LIGHTS_STATUS_JSON2);
  response->replace("{j1", Settings.friendlyname[device-1]);
  response->replace("{j2", GetHueDeviceId(device));
}

void HueGlobalConfig(String *path)
{
  String response;
  uint8_t maxhue = (devices_present > MAX_FRIENDLYNAMES) ? MAX_FRIENDLYNAMES : devices_present;

  path->remove(0,1);
  response = F("{\"lights\":{\"");
  for (uint8_t i = 1; i <= maxhue; i++) {
    response += i;
    response += F("\":{\"state\":");
    HueLightStatus1(i, &response);
    HueLightStatus2(i, &response);
    if (i < maxhue) {
      response += ",\"";
    }
  }
  response += F("},\"groups\":{},\"schedules\":{},\"config\":");
  HueConfigResponse(&response);
  response += "}";
  WebServer->send(200, FPSTR(HDR_CTYPE_JSON), response);
}

void HueAuthentication(String *path)
{
  char response[38];

  snprintf_P(response, sizeof(response), PSTR("[{\"success\":{\"username\":\"%s\"}}]"), GetHueUserId().c_str());
  WebServer->send(200, FPSTR(HDR_CTYPE_JSON), response);
}

void HueLights(String *path)
{



  String response;
  uint8_t device = 1;
  uint16_t tmp = 0;
  float bri = 0;
  float hue = 0;
  float sat = 0;
  uint16_t ct = 0;
  bool resp = false;
  bool on = false;
  bool change = false;
  uint8_t maxhue = (devices_present > MAX_FRIENDLYNAMES) ? MAX_FRIENDLYNAMES : devices_present;

  path->remove(0,path->indexOf("/lights"));
  if (path->endsWith("/lights")) {
    response = "{\"";
    for (uint8_t i = 1; i <= maxhue; i++) {
      response += i;
      response += F("\":{\"state\":");
      HueLightStatus1(i, &response);
      HueLightStatus2(i, &response);
      if (i < maxhue) {
        response += ",\"";
      }
    }
    response += "}";
    WebServer->send(200, FPSTR(HDR_CTYPE_JSON), response);
  }
  else if (path->endsWith("/state")) {
    path->remove(0,8);
    path->remove(path->indexOf("/state"));
    device = atoi(path->c_str());
    if ((device < 1) || (device > maxhue)) {
      device = 1;
    }
    if (WebServer->args()) {
      response = "[";

      StaticJsonBuffer<400> jsonBuffer;
      JsonObject &hue_json = jsonBuffer.parseObject(WebServer->arg((WebServer->args())-1));
      if (hue_json.containsKey("on")) {

        response += FPSTR(HUE_LIGHT_RESPONSE_JSON);
        response.replace("{id", String(device));
        response.replace("{cm", "on");

        on = hue_json["on"];
        switch(on)
        {
          case false : ExecuteCommandPower(device, POWER_OFF, SRC_HUE);
                       response.replace("{re", "false");
                       break;
          case true : ExecuteCommandPower(device, POWER_ON, SRC_HUE);
                       response.replace("{re", "true");
                       break;
          default : response.replace("{re", (power & (1 << (device-1))) ? "true" : "false");
                       break;
        }
        resp = true;
      }

      if (light_type) {
        LightGetHsb(&hue, &sat, &bri, g_gotct);
      }

      if (hue_json.containsKey("bri")) {
        tmp = hue_json["bri"];
        tmp = max(tmp, 1);
        tmp = min(tmp, 254);
        bri = (float)tmp / 254.0f;
        if (resp) {
          response += ",";
        }
        response += FPSTR(HUE_LIGHT_RESPONSE_JSON);
        response.replace("{id", String(device));
        response.replace("{cm", "bri");
        response.replace("{re", String(tmp));
        resp = true;
        change = true;
      }
      if (hue_json.containsKey("hue")) {
        tmp = hue_json["hue"];
        hue = (float)tmp / 65535.0f;
        if (resp) {
          response += ",";
        }
        response += FPSTR(HUE_LIGHT_RESPONSE_JSON);
        response.replace("{id", String(device));
        response.replace("{cm", "hue");
        response.replace("{re", String(tmp));
        g_gotct = false;
        resp = true;
        change = true;
      }
      if (hue_json.containsKey("sat")) {
        tmp = hue_json["sat"];
        tmp = max(tmp, 0);
        tmp = min(tmp, 254);
        sat = (float)tmp / 254.0f;
        if (resp) {
          response += ",";
        }
        response += FPSTR(HUE_LIGHT_RESPONSE_JSON);
        response.replace("{id", String(device));
        response.replace("{cm", "sat");
        response.replace("{re", String(tmp));
        g_gotct = false;
        resp = true;
        change = true;
      }
      if (hue_json.containsKey("ct")) {
        ct = hue_json["ct"];
        if (resp) {
          response += ",";
        }
        response += FPSTR(HUE_LIGHT_RESPONSE_JSON);
        response.replace("{id", String(device));
        response.replace("{cm", "ct");
        response.replace("{re", String(ct));
        g_gotct = true;
        change = true;
      }
      if (change) {
        if (light_type) {
          LightSetHsb(hue, sat, bri, ct, g_gotct);
        }
        change = false;
      }
      response += "]";
      if (2 == response.length()) {
        response = FPSTR(HUE_ERROR_JSON);
      }
    }
    else {
      response = FPSTR(HUE_ERROR_JSON);
    }

    WebServer->send(200, FPSTR(HDR_CTYPE_JSON), response);
  }
  else if(path->indexOf("/lights/") >= 0) {
    path->remove(0,8);
    device = atoi(path->c_str());
    if ((device < 1) || (device > maxhue)) {
      device = 1;
    }
    response += F("{\"state\":");
    HueLightStatus1(device, &response);
    HueLightStatus2(device, &response);
    WebServer->send(200, FPSTR(HDR_CTYPE_JSON), response);
  }
  else {
    WebServer->send(406, FPSTR(HDR_CTYPE_JSON), "{}");
  }
}

void HueGroups(String *path)
{



  String response = "{}";
  uint8_t maxhue = (devices_present > MAX_FRIENDLYNAMES) ? MAX_FRIENDLYNAMES : devices_present;

  if (path->endsWith("/0")) {
    response = FPSTR(HUE_GROUP0_STATUS_JSON);
    String lights = F("\"1\"");
    for (uint8_t i = 2; i <= maxhue; i++) {
      lights += ",\"" + String(i) + "\"";
    }
    response.replace("{l1", lights);
    HueLightStatus1(1, &response);
    response += F("}");
  }

  WebServer->send(200, FPSTR(HDR_CTYPE_JSON), response);
}

void HandleHueApi(String *path)
{
# 819 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xplg_wemohue.ino"
  uint8_t args = 0;

  path->remove(0, 4);
  uint16_t apilen = path->length();
  snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_HTTP D_HUE_API " (%s)"), path->c_str());
  AddLog(LOG_LEVEL_DEBUG_MORE);
  for (args = 0; args < WebServer->args(); args++) {
    String json = WebServer->arg(args);
    snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_HTTP D_HUE_POST_ARGS " (%s)"), json.c_str());
    AddLog(LOG_LEVEL_DEBUG_MORE);
  }

  if (path->endsWith("/invalid/")) {}
  else if (!apilen) HueAuthentication(path);
  else if (path->endsWith("/")) HueAuthentication(path);
  else if (path->endsWith("/config")) HueConfig(path);
  else if (path->indexOf("/lights") >= 0) HueLights(path);
  else if (path->indexOf("/groups") >= 0) HueGroups(path);
  else if (path->endsWith("/schedules")) HueNotImplemented(path);
  else if (path->endsWith("/sensors")) HueNotImplemented(path);
  else if (path->endsWith("/scenes")) HueNotImplemented(path);
  else if (path->endsWith("/rules")) HueNotImplemented(path);
  else HueGlobalConfig(path);
}

void HueWemoAddHandlers()
{
  if (EMUL_WEMO == Settings.flag2.emulation) {
    WebServer->on("/upnp/control/basicevent1", HTTP_POST, HandleUpnpEvent);
    WebServer->on("/eventservice.xml", HandleUpnpService);
    WebServer->on("/metainfoservice.xml", HandleUpnpMetaService);
    WebServer->on("/setup.xml", HandleUpnpSetupWemo);
  }
  if (EMUL_HUE == Settings.flag2.emulation) {
    WebServer->on("/description.xml", HandleUpnpSetupHue);
  }
}

#endif
# 1 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xplg_ws2812.ino"
# 20 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xplg_ws2812.ino"
#ifdef USE_WS2812




#include <NeoPixelBus.h>

#ifdef USE_WS2812_DMA
#if (USE_WS2812_CTYPE == NEO_GRB)
  NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> *strip = NULL;
#elif (USE_WS2812_CTYPE == NEO_BRG)
  NeoPixelBus<NeoBrgFeature, Neo800KbpsMethod> *strip = NULL;
#elif (USE_WS2812_CTYPE == NEO_RBG)
  NeoPixelBus<NeoRbgFeature, Neo800KbpsMethod> *strip = NULL;
#elif (USE_WS2812_CTYPE == NEO_RGBW)
  NeoPixelBus<NeoRgbwFeature, Neo800KbpsMethod> *strip = NULL;
#elif (USE_WS2812_CTYPE == NEO_GRBW)
  NeoPixelBus<NeoGrbwFeature, Neo800KbpsMethod> *strip = NULL;
#else
  NeoPixelBus<NeoRgbFeature, Neo800KbpsMethod> *strip = NULL;
#endif
#else
#if (USE_WS2812_CTYPE == NEO_GRB)
  NeoPixelBus<NeoGrbFeature, NeoEsp8266BitBang800KbpsMethod> *strip = NULL;
#elif (USE_WS2812_CTYPE == NEO_BRG)
  NeoPixelBus<NeoBrgFeature, NeoEsp8266BitBang800KbpsMethod> *strip = NULL;
#elif (USE_WS2812_CTYPE == NEO_RBG)
  NeoPixelBus<NeoRbgFeature, NeoEsp8266BitBang800KbpsMethod> *strip = NULL;
#elif (USE_WS2812_CTYPE == NEO_RGBW)
  NeoPixelBus<NeoRgbwFeature, NeoEsp8266BitBang800KbpsMethod> *strip = NULL;
#elif (USE_WS2812_CTYPE == NEO_GRBW)
  NeoPixelBus<NeoGrbwFeature, NeoEsp8266BitBang800KbpsMethod> *strip = NULL;
#else
  NeoPixelBus<NeoRgbFeature, NeoEsp8266BitBang800KbpsMethod> *strip = NULL;
#endif
#endif

struct WsColor {
  uint8_t red, green, blue;
};

struct ColorScheme {
  WsColor* colors;
  uint8_t count;
};

WsColor kIncandescent[2] = { 255,140,20, 0,0,0 };
WsColor kRgb[3] = { 255,0,0, 0,255,0, 0,0,255 };
WsColor kChristmas[2] = { 255,0,0, 0,255,0 };
WsColor kHanukkah[2] = { 0,0,255, 255,255,255 };
WsColor kwanzaa[3] = { 255,0,0, 0,0,0, 0,255,0 };
WsColor kRainbow[7] = { 255,0,0, 255,128,0, 255,255,0, 0,255,0, 0,0,255, 128,0,255, 255,0,255 };
WsColor kFire[3] = { 255,0,0, 255,102,0, 255,192,0 };
ColorScheme kSchemes[WS2812_SCHEMES] = {
  kIncandescent, 2,
  kRgb, 3,
  kChristmas, 2,
  kHanukkah, 2,
  kwanzaa, 3,
  kRainbow, 7,
  kFire, 3 };

uint8_t kWidth[5] = {
    1,
    2,
    4,
    8,
  255 };
uint8_t kRepeat[5] = {
    8,
    6,
    4,
    2,
    1 };

uint8_t ws_show_next = 1;
bool ws_suspend_update = false;


void Ws2812StripShow()
{
#if (USE_WS2812_CTYPE > NEO_3LED)
  RgbwColor c;
#else
  RgbColor c;
#endif

  if (Settings.light_correction) {
    for (uint16_t i = 0; i < Settings.light_pixels; i++) {
      c = strip->GetPixelColor(i);
      c.R = ledTable[c.R];
      c.G = ledTable[c.G];
      c.B = ledTable[c.B];
#if (USE_WS2812_CTYPE > NEO_3LED)
      c.W = ledTable[c.W];
#endif
      strip->SetPixelColor(i, c);
    }
  }
  strip->Show();
}

int mod(int a, int b)
{
   int ret = a % b;
   if (ret < 0) ret += b;
   return ret;
}


void Ws2812UpdatePixelColor(int position, struct WsColor hand_color, float offset)
{
#if (USE_WS2812_CTYPE > NEO_3LED)
  RgbwColor color;
#else
  RgbColor color;
#endif

  uint16_t mod_position = mod(position, (int)Settings.light_pixels);

  color = strip->GetPixelColor(mod_position);
  float dimmer = 100 / (float)Settings.light_dimmer;
  color.R = tmin(color.R + ((hand_color.red / dimmer) * offset), 255);
  color.G = tmin(color.G + ((hand_color.green / dimmer) * offset), 255);
  color.B = tmin(color.B + ((hand_color.blue / dimmer) * offset), 255);
  strip->SetPixelColor(mod_position, color);
}

void Ws2812UpdateHand(int position, uint8_t index)
{
  position = (position + Settings.light_rotation) % Settings.light_pixels;

  if (Settings.flag.ws_clock_reverse) position = Settings.light_pixels -position;
  WsColor hand_color = { Settings.ws_color[index][WS_RED], Settings.ws_color[index][WS_GREEN], Settings.ws_color[index][WS_BLUE] };

  Ws2812UpdatePixelColor(position, hand_color, 1);

  uint8_t range = 1;
  if (index < WS_MARKER) range = ((Settings.ws_width[index] -1) / 2) +1;
  for (uint8_t h = 1; h < range; h++) {
    float offset = (float)(range - h) / (float)range;
    Ws2812UpdatePixelColor(position -h, hand_color, offset);
    Ws2812UpdatePixelColor(position +h, hand_color, offset);
  }
}

void Ws2812Clock()
{
  strip->ClearTo(0);
  int clksize = 60000 / (int)Settings.light_pixels;

  Ws2812UpdateHand((RtcTime.second * 1000) / clksize, WS_SECOND);
  Ws2812UpdateHand((RtcTime.minute * 1000) / clksize, WS_MINUTE);
  Ws2812UpdateHand(((RtcTime.hour % 12) * (5000 / clksize)) + ((RtcTime.minute * 1000) / (12 * clksize)), WS_HOUR);
  if (Settings.ws_color[WS_MARKER][WS_RED] + Settings.ws_color[WS_MARKER][WS_GREEN] + Settings.ws_color[WS_MARKER][WS_BLUE]) {
    for (byte i = 0; i < 12; i++) {
      Ws2812UpdateHand((i * 5000) / clksize, WS_MARKER);
    }
  }

  Ws2812StripShow();
}

void Ws2812GradientColor(uint8_t schemenr, struct WsColor* mColor, uint16_t range, uint16_t gradRange, uint16_t i)
{




  ColorScheme scheme = kSchemes[schemenr];
  uint16_t curRange = i / range;
  uint16_t rangeIndex = i % range;
  uint16_t colorIndex = rangeIndex / gradRange;
  uint16_t start = colorIndex;
  uint16_t end = colorIndex +1;
  if (curRange % 2 != 0) {
    start = (scheme.count -1) - start;
    end = (scheme.count -1) - end;
  }
  float dimmer = 100 / (float)Settings.light_dimmer;
  float fmyRed = (float)map(rangeIndex % gradRange, 0, gradRange, scheme.colors[start].red, scheme.colors[end].red) / dimmer;
  float fmyGrn = (float)map(rangeIndex % gradRange, 0, gradRange, scheme.colors[start].green, scheme.colors[end].green) / dimmer;
  float fmyBlu = (float)map(rangeIndex % gradRange, 0, gradRange, scheme.colors[start].blue, scheme.colors[end].blue) / dimmer;
  mColor->red = (uint8_t)fmyRed;
  mColor->green = (uint8_t)fmyGrn;
  mColor->blue = (uint8_t)fmyBlu;
}

void Ws2812Gradient(uint8_t schemenr)
{





#if (USE_WS2812_CTYPE > NEO_3LED)
  RgbwColor c;
  c.W = 0;
#else
  RgbColor c;
#endif

  ColorScheme scheme = kSchemes[schemenr];
  if (scheme.count < 2) return;

  uint8_t repeat = kRepeat[Settings.light_width];
  uint16_t range = (uint16_t)ceil((float)Settings.light_pixels / (float)repeat);
  uint16_t gradRange = (uint16_t)ceil((float)range / (float)(scheme.count - 1));
  uint16_t speed = ((Settings.light_speed * 2) -1) * (STATES / 10);
  uint16_t offset = speed > 0 ? strip_timer_counter / speed : 0;

  WsColor oldColor, currentColor;
  Ws2812GradientColor(schemenr, &oldColor, range, gradRange, offset);
  currentColor = oldColor;
  for (uint16_t i = 0; i < Settings.light_pixels; i++) {
    if (kRepeat[Settings.light_width] > 1) {
      Ws2812GradientColor(schemenr, &currentColor, range, gradRange, i +offset);
    }
    if (Settings.light_speed > 0) {

      c.R = map(strip_timer_counter % speed, 0, speed, oldColor.red, currentColor.red);
      c.G = map(strip_timer_counter % speed, 0, speed, oldColor.green, currentColor.green);
      c.B = map(strip_timer_counter % speed, 0, speed, oldColor.blue, currentColor.blue);
    }
    else {

      c.R = currentColor.red;
      c.G = currentColor.green;
      c.B = currentColor.blue;
    }
    strip->SetPixelColor(i, c);
    oldColor = currentColor;
  }
  Ws2812StripShow();
}

void Ws2812Bars(uint8_t schemenr)
{





#if (USE_WS2812_CTYPE > NEO_3LED)
  RgbwColor c;
  c.W = 0;
#else
  RgbColor c;
#endif
  uint16_t i;

  ColorScheme scheme = kSchemes[schemenr];

  uint16_t maxSize = Settings.light_pixels / scheme.count;
  if (kWidth[Settings.light_width] > maxSize) maxSize = 0;

  uint16_t speed = ((Settings.light_speed * 2) -1) * (STATES / 10);
  uint8_t offset = speed > 0 ? strip_timer_counter / speed : 0;

  WsColor mcolor[scheme.count];
  memcpy(mcolor, scheme.colors, sizeof(mcolor));
  float dimmer = 100 / (float)Settings.light_dimmer;
  for (i = 0; i < scheme.count; i++) {
    float fmyRed = (float)mcolor[i].red / dimmer;
    float fmyGrn = (float)mcolor[i].green / dimmer;
    float fmyBlu = (float)mcolor[i].blue / dimmer;
    mcolor[i].red = (uint8_t)fmyRed;
    mcolor[i].green = (uint8_t)fmyGrn;
    mcolor[i].blue = (uint8_t)fmyBlu;
  }
  uint8_t colorIndex = offset % scheme.count;
  for (i = 0; i < Settings.light_pixels; i++) {
    if (maxSize) colorIndex = ((i + offset) % (scheme.count * kWidth[Settings.light_width])) / kWidth[Settings.light_width];
    c.R = mcolor[colorIndex].red;
    c.G = mcolor[colorIndex].green;
    c.B = mcolor[colorIndex].blue;
    strip->SetPixelColor(i, c);
  }
  Ws2812StripShow();
}





void Ws2812Init()
{
#ifdef USE_WS2812_DMA
#if (USE_WS2812_CTYPE == NEO_GRB)
  strip = new NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod>(WS2812_MAX_LEDS);
#elif (USE_WS2812_CTYPE == NEO_BRG)
  strip = new NeoPixelBus<NeoBrgFeature, Neo800KbpsMethod>(WS2812_MAX_LEDS);
#elif (USE_WS2812_CTYPE == NEO_RBG)
  strip = new NeoPixelBus<NeoRbgFeature, Neo800KbpsMethod>(WS2812_MAX_LEDS);
#elif (USE_WS2812_CTYPE == NEO_RGBW)
  strip = new NeoPixelBus<NeoRgbwFeature, Neo800KbpsMethod>(WS2812_MAX_LEDS);
#elif (USE_WS2812_CTYPE == NEO_GRBW)
  strip = new NeoPixelBus<NeoGrbwFeature, Neo800KbpsMethod>(WS2812_MAX_LEDS);
#else
  strip = new NeoPixelBus<NeoRgbFeature, Neo800KbpsMethod>(WS2812_MAX_LEDS);
#endif
#else
#if (USE_WS2812_CTYPE == NEO_GRB)
  strip = new NeoPixelBus<NeoGrbFeature, NeoEsp8266BitBang800KbpsMethod>(WS2812_MAX_LEDS, pin[GPIO_WS2812]);
#elif (USE_WS2812_CTYPE == NEO_BRG)
  strip = new NeoPixelBus<NeoBrgFeature, NeoEsp8266BitBang800KbpsMethod>(WS2812_MAX_LEDS, pin[GPIO_WS2812]);
#elif (USE_WS2812_CTYPE == NEO_RBG)
  strip = new NeoPixelBus<NeoRbgFeature, NeoEsp8266BitBang800KbpsMethod>(WS2812_MAX_LEDS, pin[GPIO_WS2812]);
#elif (USE_WS2812_CTYPE == NEO_RGBW)
  strip = new NeoPixelBus<NeoRgbwFeature, NeoEsp8266BitBang800KbpsMethod>(WS2812_MAX_LEDS, pin[GPIO_WS2812]);
#elif (USE_WS2812_CTYPE == NEO_GRBW)
  strip = new NeoPixelBus<NeoGrbwFeature, NeoEsp8266BitBang800KbpsMethod>(WS2812_MAX_LEDS, pin[GPIO_WS2812]);
#else
  strip = new NeoPixelBus<NeoRgbFeature, NeoEsp8266BitBang800KbpsMethod>(WS2812_MAX_LEDS, pin[GPIO_WS2812]);
#endif
#endif
  strip->Begin();
  Ws2812Clear();
}

void Ws2812Clear()
{
  strip->ClearTo(0);
  strip->Show();
  ws_show_next = 1;
}

void Ws2812SetColor(uint16_t led, uint8_t red, uint8_t green, uint8_t blue, uint8_t white)
{
#if (USE_WS2812_CTYPE > NEO_3LED)
  RgbwColor lcolor;
  lcolor.W = white;
#else
  RgbColor lcolor;
#endif

  lcolor.R = red;
  lcolor.G = green;
  lcolor.B = blue;
  if (led) {
    strip->SetPixelColor(led -1, lcolor);
  } else {

    for (uint16_t i = 0; i < Settings.light_pixels; i++) {
      strip->SetPixelColor(i, lcolor);
    }
  }

  if (!ws_suspend_update) {
    strip->Show();
    ws_show_next = 1;
  }
}

void Ws2812ForceSuspend () {
  ws_suspend_update = true;
}

void Ws2812ForceUpdate () {
  ws_suspend_update = false;
  strip->Show();
  ws_show_next = 1;
}

char* Ws2812GetColor(uint16_t led, char* scolor)
{
  uint8_t sl_ledcolor[4];

 #if (USE_WS2812_CTYPE > NEO_3LED)
  RgbwColor lcolor = strip->GetPixelColor(led -1);
  sl_ledcolor[3] = lcolor.W;
 #else
  RgbColor lcolor = strip->GetPixelColor(led -1);
 #endif
  sl_ledcolor[0] = lcolor.R;
  sl_ledcolor[1] = lcolor.G;
  sl_ledcolor[2] = lcolor.B;
  scolor[0] = '\0';
  for (byte i = 0; i < light_subtype; i++) {
    if (Settings.flag.decimal_text) {
      snprintf_P(scolor, 25, PSTR("%s%s%d"), scolor, (i > 0) ? "," : "", sl_ledcolor[i]);
    } else {
      snprintf_P(scolor, 25, PSTR("%s%02X"), scolor, sl_ledcolor[i]);
    }
  }
  return scolor;
}

void Ws2812ShowScheme(uint8_t scheme)
{
  switch (scheme) {
    case 0:
      if ((1 == state_250mS) || (ws_show_next)) {
        Ws2812Clock();
        ws_show_next = 0;
      }
      break;
    default:
      if (1 == Settings.light_fade) {
        Ws2812Gradient(scheme -1);
      } else {
        Ws2812Bars(scheme -1);
      }
      ws_show_next = 1;
      break;
  }
}

#endif
# 1 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_01_counter.ino"
# 24 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_01_counter.ino"
unsigned long last_counter_timer[MAX_COUNTERS];

void CounterUpdate(byte index)
{
  unsigned long counter_debounce_time = micros() - last_counter_timer[index -1];
  if (counter_debounce_time > Settings.pulse_counter_debounce * 1000) {
    last_counter_timer[index -1] = micros();
    if (bitRead(Settings.pulse_counter_type, index -1)) {
      RtcSettings.pulse_counter[index -1] = counter_debounce_time;
    } else {
      RtcSettings.pulse_counter[index -1]++;
    }



  }
}

void CounterUpdate1()
{
  CounterUpdate(1);
}

void CounterUpdate2()
{
  CounterUpdate(2);
}

void CounterUpdate3()
{
  CounterUpdate(3);
}

void CounterUpdate4()
{
  CounterUpdate(4);
}



void CounterSaveState()
{
  for (byte i = 0; i < MAX_COUNTERS; i++) {
    if (pin[GPIO_CNTR1 +i] < 99) {
      Settings.pulse_counter[i] = RtcSettings.pulse_counter[i];
    }
  }
}

void CounterInit()
{
  typedef void (*function) () ;
  function counter_callbacks[] = { CounterUpdate1, CounterUpdate2, CounterUpdate3, CounterUpdate4 };

  for (byte i = 0; i < MAX_COUNTERS; i++) {
    if (pin[GPIO_CNTR1 +i] < 99) {
      pinMode(pin[GPIO_CNTR1 +i], bitRead(counter_no_pullup, i) ? INPUT : INPUT_PULLUP);
      attachInterrupt(pin[GPIO_CNTR1 +i], counter_callbacks[i], FALLING);
    }
  }
}

#ifdef USE_WEBSERVER
const char HTTP_SNS_COUNTER[] PROGMEM =
  "%s{s}" D_COUNTER "%d{m}%s%s{e}";
#endif

void CounterShow(boolean json)
{
  char stemp[10];
  char counter[16];

  byte dsxflg = 0;
  byte header = 0;
  for (byte i = 0; i < MAX_COUNTERS; i++) {
    if (pin[GPIO_CNTR1 +i] < 99) {
      if (bitRead(Settings.pulse_counter_type, i)) {
        dtostrfd((double)RtcSettings.pulse_counter[i] / 1000000, 6, counter);
      } else {
        dsxflg++;
        dtostrfd(RtcSettings.pulse_counter[i], 0, counter);
      }

      if (json) {
        if (!header) {
          snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s,\"COUNTER\":{"), mqtt_data);
          stemp[0] = '\0';
        }
        header++;
        snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s%s\"C%d\":%s"), mqtt_data, stemp, i +1, counter);
        strlcpy(stemp, ",", sizeof(stemp));
#ifdef USE_DOMOTICZ
        if ((0 == tele_period) && (1 == dsxflg)) {
          DomoticzSensor(DZ_COUNT, RtcSettings.pulse_counter[i]);
          dsxflg++;
        }
#endif
#ifdef USE_WEBSERVER
      } else {
        snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_SNS_COUNTER, mqtt_data, i +1, counter, (bitRead(Settings.pulse_counter_type, i)) ? " " D_UNIT_SECOND : "");
#endif
      }
    }
    if (bitRead(Settings.pulse_counter_type, i)) {
      RtcSettings.pulse_counter[i] = 0xFFFFFFFF;
    }
  }
  if (json) {
    if (header) {
      snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s}"), mqtt_data);
    }
  }
}





#define XSNS_01 

boolean Xsns01(byte function)
{
  boolean result = false;

  switch (function) {
    case FUNC_INIT:
      CounterInit();
      break;
    case FUNC_JSON_APPEND:
      CounterShow(1);
      break;
#ifdef USE_WEBSERVER
    case FUNC_WEB_APPEND:
      CounterShow(0);
      break;
#endif
    case FUNC_SAVE_BEFORE_RESTART:
      CounterSaveState();
      break;
  }
  return result;
}
# 1 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_04_snfsc.ino"
# 56 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_04_snfsc.ino"
uint16_t sc_value[5] = { 0 };

void SonoffScSend(const char *data)
{
  Serial.write(data);
  Serial.write('\x1B');
  snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_SERIAL D_TRANSMIT " %s"), data);
  AddLog(LOG_LEVEL_DEBUG);
}

void SonoffScInit()
{

  SonoffScSend("AT+START");

}

void SonoffScSerialInput(char *rcvstat)
{
  char *p;
  char *str;
  uint16_t value[5] = { 0 };

  snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_SERIAL D_RECEIVED " %s"), rcvstat);
  AddLog(LOG_LEVEL_DEBUG);

  if (!strncasecmp_P(rcvstat, PSTR("AT+UPDATE="), 10)) {
    int8_t i = -1;
    for (str = strtok_r(rcvstat, ":", &p); str && i < 5; str = strtok_r(NULL, ":", &p)) {
      value[i++] = atoi(str);
    }
    if (value[0] > 0) {
      for (byte i = 0; i < 5; i++) {
        sc_value[i] = value[i];
      }
      sc_value[2] = (11 - sc_value[2]) * 10;
      sc_value[3] *= 10;
      sc_value[4] = (11 - sc_value[4]) * 10;
      SonoffScSend("AT+SEND=ok");
    } else {
      SonoffScSend("AT+SEND=fail");
    }
  }
  else if (!strcasecmp_P(rcvstat, PSTR("AT+STATUS?"))) {
    SonoffScSend("AT+STATUS=4");
  }
}



#ifdef USE_WEBSERVER
const char HTTP_SNS_SCPLUS[] PROGMEM =
  "%s{s}" D_LIGHT "{m}%d%%{e}{s}" D_NOISE "{m}%d%%{e}{s}" D_AIR_QUALITY "{m}%d%%{e}";
#endif

void SonoffScShow(boolean json)
{
  if (sc_value[0] > 0) {
    char temperature[10];
    char humidity[10];

    float t = ConvertTemp(sc_value[1]);
    float h = sc_value[0];
    dtostrfd(t, Settings.flag2.temperature_resolution, temperature);
    dtostrfd(h, Settings.flag2.humidity_resolution, humidity);

    if (json) {
      snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s,\"SonoffSC\":{\"" D_JSON_TEMPERATURE "\":%s,\"" D_JSON_HUMIDITY "\":%s,\"" D_JSON_LIGHT "\":%d,\"" D_JSON_NOISE "\":%d,\"" D_JSON_AIRQUALITY "\":%d}"),
        mqtt_data, temperature, humidity, sc_value[2], sc_value[3], sc_value[4]);
#ifdef USE_DOMOTICZ
      if (0 == tele_period) {
        DomoticzTempHumSensor(temperature, humidity);
        DomoticzSensor(DZ_ILLUMINANCE, sc_value[2]);
        DomoticzSensor(DZ_COUNT, sc_value[3]);
        DomoticzSensor(DZ_AIRQUALITY, 500 + ((100 - sc_value[4]) * 20));
      }
#endif

#ifdef USE_KNX
      if (0 == tele_period) {
        KnxSensor(KNX_TEMPERATURE, t);
        KnxSensor(KNX_HUMIDITY, h);
      }
#endif

#ifdef USE_WEBSERVER
    } else {
      snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_SNS_TEMP, mqtt_data, "", temperature, TempUnit());
      snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_SNS_HUM, mqtt_data, "", humidity);
      snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_SNS_SCPLUS, mqtt_data, sc_value[2], sc_value[3], sc_value[4]);
#endif
    }
  }
}





#define XSNS_04 

boolean Xsns04(byte function)
{
  boolean result = false;

  if (SONOFF_SC == Settings.module) {
    switch (function) {
      case FUNC_INIT:
        SonoffScInit();
        break;
      case FUNC_JSON_APPEND:
        SonoffScShow(1);
        break;
#ifdef USE_WEBSERVER
      case FUNC_WEB_APPEND:
        SonoffScShow(0);
        break;
#endif
    }
  }
  return result;
}
# 1 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_05_ds18b20.ino"
# 20 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_05_ds18b20.ino"
#ifdef USE_DS18B20




#define W1_SKIP_ROM 0xCC
#define W1_CONVERT_TEMP 0x44
#define W1_READ_SCRATCHPAD 0xBE

float ds18b20_temperature = 0;
uint8_t ds18b20_valid = 0;
uint8_t ds18x20_pin = 0;
char ds18b20_types[] = "DS18B20";





uint8_t OneWireReset()
{
  uint8_t retries = 125;


  pinMode(ds18x20_pin, INPUT);
  do {
    if (--retries == 0) {
      return 0;
    }
    delayMicroseconds(2);
  } while (!digitalRead(ds18x20_pin));
  pinMode(ds18x20_pin, OUTPUT);
  digitalWrite(ds18x20_pin, LOW);
  delayMicroseconds(480);
  pinMode(ds18x20_pin, INPUT);
  delayMicroseconds(70);
  uint8_t r = !digitalRead(ds18x20_pin);

  delayMicroseconds(410);
  return r;
}

void OneWireWriteBit(uint8_t v)
{
  static const uint8_t delay_low[2] = { 65, 10 };
  static const uint8_t delay_high[2] = { 5, 55 };

  v &= 1;

  digitalWrite(ds18x20_pin, LOW);
  pinMode(ds18x20_pin, OUTPUT);
  delayMicroseconds(delay_low[v]);
  digitalWrite(ds18x20_pin, HIGH);

  delayMicroseconds(delay_high[v]);
}

uint8_t OneWireReadBit()
{

  pinMode(ds18x20_pin, OUTPUT);
  digitalWrite(ds18x20_pin, LOW);
  delayMicroseconds(3);
  pinMode(ds18x20_pin, INPUT);
  delayMicroseconds(10);
  uint8_t r = digitalRead(ds18x20_pin);

  delayMicroseconds(53);
  return r;
}

void OneWireWrite(uint8_t v)
{
  for (uint8_t bit_mask = 0x01; bit_mask; bit_mask <<= 1) {
    OneWireWriteBit((bit_mask & v) ? 1 : 0);
  }
}

uint8_t OneWireRead()
{
  uint8_t r = 0;

  for (uint8_t bit_mask = 0x01; bit_mask; bit_mask <<= 1) {
    if (OneWireReadBit()) {
      r |= bit_mask;
    }
  }
  return r;
}

boolean OneWireCrc8(uint8_t *addr)
{
  uint8_t crc = 0;
  uint8_t len = 8;

  while (len--) {
    uint8_t inbyte = *addr++;
    for (uint8_t i = 8; i; i--) {
      uint8_t mix = (crc ^ inbyte) & 0x01;
      crc >>= 1;
      if (mix) {
        crc ^= 0x8C;
      }
      inbyte >>= 1;
    }
  }
  return (crc == *addr);
}



void Ds18b20Convert()
{
  OneWireReset();
  OneWireWrite(W1_SKIP_ROM);
  OneWireWrite(W1_CONVERT_TEMP);

}

boolean Ds18b20Read()
{
  uint8_t data[9];
  int8_t sign = 1;

  if (ds18b20_valid) { ds18b20_valid--; }






  for (uint8_t retry = 0; retry < 3; retry++) {
    OneWireReset();
    OneWireWrite(W1_SKIP_ROM);
    OneWireWrite(W1_READ_SCRATCHPAD);
    for (uint8_t i = 0; i < 9; i++) {
      data[i] = OneWireRead();
    }
    if (OneWireCrc8(data)) {
      uint16_t temp12 = (data[1] << 8) + data[0];
      if (temp12 > 2047) {
        temp12 = (~temp12) +1;
        sign = -1;
      }
      ds18b20_temperature = ConvertTemp(sign * temp12 * 0.0625);
      ds18b20_valid = SENSOR_MAX_MISS;
      return true;
    }
  }
  AddLog_P(LOG_LEVEL_DEBUG, PSTR(D_LOG_DSB D_SENSOR_CRC_ERROR));
  return false;
}



void Ds18b20EverySecond()
{
  ds18x20_pin = pin[GPIO_DSB];
  if (uptime &1) {

    Ds18b20Convert();
  } else {

    if (!Ds18b20Read()) {
      AddLogMissed(ds18b20_types, ds18b20_valid);
    }
  }
}

void Ds18b20Show(boolean json)
{
  if (ds18b20_valid) {
    char temperature[10];

    dtostrfd(ds18b20_temperature, Settings.flag2.temperature_resolution, temperature);
    if(json) {
      snprintf_P(mqtt_data, sizeof(mqtt_data), JSON_SNS_TEMP, mqtt_data, ds18b20_types, temperature);
#ifdef USE_DOMOTICZ
      if (0 == tele_period) {
        DomoticzSensor(DZ_TEMP, temperature);
      }
#endif
#ifdef USE_KNX
      if (0 == tele_period) {
        KnxSensor(KNX_TEMPERATURE, ds18b20_temperature);
      }
#endif
#ifdef USE_WEBSERVER
    } else {
      snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_SNS_TEMP, mqtt_data, ds18b20_types, temperature, TempUnit());
#endif
    }
  }
}





#define XSNS_05 

boolean Xsns05(byte function)
{
  boolean result = false;

  if (pin[GPIO_DSB] < 99) {
    switch (function) {
      case FUNC_EVERY_SECOND:
        Ds18b20EverySecond();
        break;
      case FUNC_JSON_APPEND:
        Ds18b20Show(1);
        break;
#ifdef USE_WEBSERVER
      case FUNC_WEB_APPEND:
        Ds18b20Show(0);
        break;
#endif
    }
  }
  return result;
}

#endif
# 1 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_05_ds18x20.ino"
# 20 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_05_ds18x20.ino"
#ifdef USE_DS18x20





#define DS18S20_CHIPID 0x10
#define DS1822_CHIPID 0x22
#define DS18B20_CHIPID 0x28
#define MAX31850_CHIPID 0x3B

#define W1_SKIP_ROM 0xCC
#define W1_CONVERT_TEMP 0x44
#define W1_WRITE_EEPROM 0x48
#define W1_WRITE_SCRATCHPAD 0x4E
#define W1_READ_SCRATCHPAD 0xBE

#define DS18X20_MAX_SENSORS 8

const char kDs18x20Types[] PROGMEM = "DS18x20|DS18S20|DS1822|DS18B20|MAX31850";

uint8_t ds18x20_chipids[] = { 0, DS18S20_CHIPID, DS1822_CHIPID, DS18B20_CHIPID, MAX31850_CHIPID };

struct DS18X20STRUCT {
  uint8_t address[8];
  uint8_t index;
  uint8_t valid;
  float temperature;
} ds18x20_sensor[DS18X20_MAX_SENSORS];
uint8_t ds18x20_sensors = 0;
uint8_t ds18x20_pin = 0;
char ds18x20_types[12];
#ifdef W1_PARASITE_POWER
uint8_t ds18x20_sensor_curr = 0;
unsigned long w1_power_until = 0;
#endif





#define W1_MATCH_ROM 0x55
#define W1_SEARCH_ROM 0xF0

uint8_t onewire_last_discrepancy = 0;
uint8_t onewire_last_family_discrepancy = 0;
bool onewire_last_device_flag = false;
unsigned char onewire_rom_id[8] = { 0 };

uint8_t OneWireReset()
{
  uint8_t retries = 125;


  pinMode(ds18x20_pin, INPUT);
  do {
    if (--retries == 0) {
      return 0;
    }
    delayMicroseconds(2);
  } while (!digitalRead(ds18x20_pin));
  pinMode(ds18x20_pin, OUTPUT);
  digitalWrite(ds18x20_pin, LOW);
  delayMicroseconds(480);
  pinMode(ds18x20_pin, INPUT);
  delayMicroseconds(70);
  uint8_t r = !digitalRead(ds18x20_pin);

  delayMicroseconds(410);
  return r;
}

void OneWireWriteBit(uint8_t v)
{
  static const uint8_t delay_low[2] = { 65, 10 };
  static const uint8_t delay_high[2] = { 5, 55 };

  v &= 1;

  digitalWrite(ds18x20_pin, LOW);
  pinMode(ds18x20_pin, OUTPUT);
  delayMicroseconds(delay_low[v]);
  digitalWrite(ds18x20_pin, HIGH);

  delayMicroseconds(delay_high[v]);
}

uint8_t OneWireReadBit()
{

  pinMode(ds18x20_pin, OUTPUT);
  digitalWrite(ds18x20_pin, LOW);
  delayMicroseconds(3);
  pinMode(ds18x20_pin, INPUT);
  delayMicroseconds(10);
  uint8_t r = digitalRead(ds18x20_pin);

  delayMicroseconds(53);
  return r;
}

void OneWireWrite(uint8_t v)
{
  for (uint8_t bit_mask = 0x01; bit_mask; bit_mask <<= 1) {
    OneWireWriteBit((bit_mask & v) ? 1 : 0);
  }
}

uint8_t OneWireRead()
{
  uint8_t r = 0;

  for (uint8_t bit_mask = 0x01; bit_mask; bit_mask <<= 1) {
    if (OneWireReadBit()) {
      r |= bit_mask;
    }
  }
  return r;
}

void OneWireSelect(const uint8_t rom[8])
{
  OneWireWrite(W1_MATCH_ROM);
  for (uint8_t i = 0; i < 8; i++) {
    OneWireWrite(rom[i]);
  }
}

void OneWireResetSearch()
{
  onewire_last_discrepancy = 0;
  onewire_last_device_flag = false;
  onewire_last_family_discrepancy = 0;
  for (uint8_t i = 0; i < 8; i++) {
    onewire_rom_id[i] = 0;
  }
}

uint8_t OneWireSearch(uint8_t *newAddr)
{
  uint8_t id_bit_number = 1;
  uint8_t last_zero = 0;
  uint8_t rom_byte_number = 0;
  uint8_t search_result = 0;
  uint8_t id_bit;
  uint8_t cmp_id_bit;
  unsigned char rom_byte_mask = 1;
  unsigned char search_direction;

  if (!onewire_last_device_flag) {
    if (!OneWireReset()) {
      onewire_last_discrepancy = 0;
      onewire_last_device_flag = false;
      onewire_last_family_discrepancy = 0;
      return false;
    }
    OneWireWrite(W1_SEARCH_ROM);
    do {
      id_bit = OneWireReadBit();
      cmp_id_bit = OneWireReadBit();

      if ((id_bit == 1) && (cmp_id_bit == 1)) {
        break;
      } else {
        if (id_bit != cmp_id_bit) {
          search_direction = id_bit;
        } else {
          if (id_bit_number < onewire_last_discrepancy) {
            search_direction = ((onewire_rom_id[rom_byte_number] & rom_byte_mask) > 0);
          } else {
            search_direction = (id_bit_number == onewire_last_discrepancy);
          }
          if (search_direction == 0) {
            last_zero = id_bit_number;
            if (last_zero < 9) {
              onewire_last_family_discrepancy = last_zero;
            }
          }
        }
        if (search_direction == 1) {
          onewire_rom_id[rom_byte_number] |= rom_byte_mask;
        } else {
          onewire_rom_id[rom_byte_number] &= ~rom_byte_mask;
        }
        OneWireWriteBit(search_direction);
        id_bit_number++;
        rom_byte_mask <<= 1;
        if (rom_byte_mask == 0) {
          rom_byte_number++;
          rom_byte_mask = 1;
        }
      }
    } while (rom_byte_number < 8);
    if (!(id_bit_number < 65)) {
      onewire_last_discrepancy = last_zero;
      if (onewire_last_discrepancy == 0) {
        onewire_last_device_flag = true;
      }
      search_result = true;
    }
  }
  if (!search_result || !onewire_rom_id[0]) {
    onewire_last_discrepancy = 0;
    onewire_last_device_flag = false;
    onewire_last_family_discrepancy = 0;
    search_result = false;
  }
  for (uint8_t i = 0; i < 8; i++) {
    newAddr[i] = onewire_rom_id[i];
  }
  return search_result;
}

boolean OneWireCrc8(uint8_t *addr)
{
  uint8_t crc = 0;
  uint8_t len = 8;

  while (len--) {
    uint8_t inbyte = *addr++;
    for (uint8_t i = 8; i; i--) {
      uint8_t mix = (crc ^ inbyte) & 0x01;
      crc >>= 1;
      if (mix) {
        crc ^= 0x8C;
      }
      inbyte >>= 1;
    }
  }
  return (crc == *addr);
}



void Ds18x20Init()
{
  uint64_t ids[DS18X20_MAX_SENSORS];

  ds18x20_pin = pin[GPIO_DSB];

  OneWireResetSearch();
  for (ds18x20_sensors = 0; ds18x20_sensors < DS18X20_MAX_SENSORS; ds18x20_sensors) {
    if (!OneWireSearch(ds18x20_sensor[ds18x20_sensors].address)) {
      break;
    }
    if (OneWireCrc8(ds18x20_sensor[ds18x20_sensors].address) &&
       ((ds18x20_sensor[ds18x20_sensors].address[0] == DS18S20_CHIPID) ||
        (ds18x20_sensor[ds18x20_sensors].address[0] == DS1822_CHIPID) ||
        (ds18x20_sensor[ds18x20_sensors].address[0] == DS18B20_CHIPID) ||
        (ds18x20_sensor[ds18x20_sensors].address[0] == MAX31850_CHIPID))) {
      ds18x20_sensor[ds18x20_sensors].index = ds18x20_sensors;
      ids[ds18x20_sensors] = ds18x20_sensor[ds18x20_sensors].address[0];
      for (uint8_t j = 6; j > 0; j--) {
        ids[ds18x20_sensors] = ids[ds18x20_sensors] << 8 | ds18x20_sensor[ds18x20_sensors].address[j];
      }
      ds18x20_sensors++;
    }
  }
  for (uint8_t i = 0; i < ds18x20_sensors; i++) {
    for (uint8_t j = i + 1; j < ds18x20_sensors; j++) {
      if (ids[ds18x20_sensor[i].index] > ids[ds18x20_sensor[j].index]) {
        std::swap(ds18x20_sensor[i].index, ds18x20_sensor[j].index);
      }
    }
  }
  snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_DSB D_SENSORS_FOUND " %d"), ds18x20_sensors);
  AddLog(LOG_LEVEL_DEBUG);
}

void Ds18x20Convert()
{
  OneWireReset();
#ifdef W1_PARASITE_POWER

  if (++ds18x20_sensor_curr >= ds18x20_sensors)
    ds18x20_sensor_curr = 0;
  OneWireSelect(ds18x20_sensor[ds18x20_sensor_curr].address);
#else
  OneWireWrite(W1_SKIP_ROM);
#endif
  OneWireWrite(W1_CONVERT_TEMP);

}

bool Ds18x20Read(uint8_t sensor)
{
  uint8_t data[9];
  int8_t sign = 1;
  uint16_t temp12 = 0;
  int16_t temp14 = 0;
  float temp9 = 0.0;

  uint8_t index = ds18x20_sensor[sensor].index;
  if (ds18x20_sensor[index].valid) { ds18x20_sensor[index].valid--; }
  for (uint8_t retry = 0; retry < 3; retry++) {
    OneWireReset();
    OneWireSelect(ds18x20_sensor[index].address);
    OneWireWrite(W1_READ_SCRATCHPAD);
    for (uint8_t i = 0; i < 9; i++) {
      data[i] = OneWireRead();
    }
    if (OneWireCrc8(data)) {
      switch(ds18x20_sensor[index].address[0]) {
      case DS18S20_CHIPID:
        if (data[1] > 0x80) {
          data[0] = (~data[0]) +1;
          sign = -1;
        }
        if (data[0] & 1) {
          temp9 = ((data[0] >> 1) + 0.5) * sign;
        } else {
          temp9 = (data[0] >> 1) * sign;
        }
        ds18x20_sensor[index].temperature = ConvertTemp((temp9 - 0.25) + ((16.0 - data[6]) / 16.0));
        ds18x20_sensor[index].valid = SENSOR_MAX_MISS;
        return true;
      case DS1822_CHIPID:
      case DS18B20_CHIPID:
        if (data[4] != 0x7F) {
          data[4] = 0x7F;
          OneWireReset();
          OneWireSelect(ds18x20_sensor[index].address);
          OneWireWrite(W1_WRITE_SCRATCHPAD);
          OneWireWrite(data[2]);
          OneWireWrite(data[3]);
          OneWireWrite(data[4]);
          OneWireSelect(ds18x20_sensor[index].address);
          OneWireWrite(W1_WRITE_EEPROM);
#ifdef W1_PARASITE_POWER
          w1_power_until = millis() + 10;
#endif
        }
        temp12 = (data[1] << 8) + data[0];
        if (temp12 > 2047) {
          temp12 = (~temp12) +1;
          sign = -1;
        }
        ds18x20_sensor[index].temperature = ConvertTemp(sign * temp12 * 0.0625);
        ds18x20_sensor[index].valid = SENSOR_MAX_MISS;
        return true;
      case MAX31850_CHIPID:
        temp14 = (data[1] << 8) + (data[0] & 0xFC);
        ds18x20_sensor[index].temperature = ConvertTemp(temp14 * 0.0625);
        ds18x20_sensor[index].valid = SENSOR_MAX_MISS;
        return true;
      }
    }
  }
  AddLog_P(LOG_LEVEL_DEBUG, PSTR(D_LOG_DSB D_SENSOR_CRC_ERROR));
  return false;
}

void Ds18x20Name(uint8_t sensor)
{
  uint8_t index = sizeof(ds18x20_chipids);
  while (index) {
    if (ds18x20_sensor[ds18x20_sensor[sensor].index].address[0] == ds18x20_chipids[index]) {
      break;
    }
    index--;
  }
  GetTextIndexed(ds18x20_types, sizeof(ds18x20_types), index, kDs18x20Types);
  if (ds18x20_sensors > 1) {
    snprintf_P(ds18x20_types, sizeof(ds18x20_types), PSTR("%s-%d"), ds18x20_types, sensor +1);
  }
}



void Ds18x20EverySecond()
{
#ifdef W1_PARASITE_POWER

  unsigned long now = millis();
  if (now < w1_power_until)
    return;
#endif
  if (uptime & 1
#ifdef W1_PARASITE_POWER

      || ds18x20_sensors >= 2
#endif
  ) {

    Ds18x20Convert();
  } else {
    for (uint8_t i = 0; i < ds18x20_sensors; i++) {

      if (!Ds18x20Read(i)) {
        Ds18x20Name(i);
        AddLogMissed(ds18x20_types, ds18x20_sensor[ds18x20_sensor[i].index].valid);
#ifdef USE_DS18x20_RECONFIGURE
        if (!ds18x20_sensor[ds18x20_sensor[i].index].valid) {
          memset(&ds18x20_sensor, 0, sizeof(ds18x20_sensor));
          Ds18x20Init();
        }
#endif
      }
    }
  }
}

void Ds18x20Show(boolean json)
{
  char temperature[10];

  for (uint8_t i = 0; i < ds18x20_sensors; i++) {
    uint8_t index = ds18x20_sensor[i].index;

    if (ds18x20_sensor[index].valid) {
      dtostrfd(ds18x20_sensor[index].temperature, Settings.flag2.temperature_resolution, temperature);

      Ds18x20Name(i);

      if (json) {
        if (1 == ds18x20_sensors) {
          snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s,\"%s\":{\"" D_JSON_TEMPERATURE "\":%s}"), mqtt_data, ds18x20_types, temperature);
        } else {
          char address[17];
          for (byte j = 0; j < 6; j++) {
            sprintf(address+2*j, "%02X", ds18x20_sensor[index].address[6-j]);
          }
          snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s,\"%s\":{\"" D_JSON_ID "\":\"%s\",\"" D_JSON_TEMPERATURE "\":%s}"), mqtt_data, ds18x20_types, address, temperature);
        }
#ifdef USE_DOMOTICZ
        if ((0 == tele_period) && (0 == i)) {
          DomoticzSensor(DZ_TEMP, temperature);
        }
#endif
#ifdef USE_KNX
        if ((0 == tele_period) && (0 == i)) {
          KnxSensor(KNX_TEMPERATURE, ds18x20_sensor[index].temperature);
        }
#endif
#ifdef USE_WEBSERVER
      } else {
        snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_SNS_TEMP, mqtt_data, ds18x20_types, temperature, TempUnit());
#endif
      }
    }
  }
}





#define XSNS_05 

boolean Xsns05(byte function)
{
  boolean result = false;

  if (pin[GPIO_DSB] < 99) {
    switch (function) {
      case FUNC_INIT:
        Ds18x20Init();
        break;
      case FUNC_EVERY_SECOND:
        Ds18x20EverySecond();
        break;
      case FUNC_JSON_APPEND:
        Ds18x20Show(1);
        break;
#ifdef USE_WEBSERVER
      case FUNC_WEB_APPEND:
        Ds18x20Show(0);
        break;
#endif
    }
  }
  return result;
}

#endif
# 1 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_05_ds18x20_legacy.ino"
# 20 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_05_ds18x20_legacy.ino"
#ifdef USE_DS18x20_LEGACY




#define DS18S20_CHIPID 0x10
#define DS18B20_CHIPID 0x28
#define MAX31850_CHIPID 0x3B

#define W1_SKIP_ROM 0xCC
#define W1_CONVERT_TEMP 0x44
#define W1_READ_SCRATCHPAD 0xBE

#define DS18X20_MAX_SENSORS 8

#include <OneWire.h>

OneWire *ds = NULL;

uint8_t ds18x20_address[DS18X20_MAX_SENSORS][8];
uint8_t ds18x20_index[DS18X20_MAX_SENSORS];
uint8_t ds18x20_sensors = 0;
char ds18x20_types[9];

void Ds18x20Init()
{
  ds = new OneWire(pin[GPIO_DSB]);
}

void Ds18x20Search()
{
  uint8_t num_sensors=0;
  uint8_t sensor = 0;

  ds->reset_search();
  for (num_sensors = 0; num_sensors < DS18X20_MAX_SENSORS; num_sensors) {
    if (!ds->search(ds18x20_address[num_sensors])) {
      ds->reset_search();
      break;
    }

    if ((OneWire::crc8(ds18x20_address[num_sensors], 7) == ds18x20_address[num_sensors][7]) &&
       ((ds18x20_address[num_sensors][0]==DS18S20_CHIPID) || (ds18x20_address[num_sensors][0]==DS18B20_CHIPID) || (ds18x20_address[num_sensors][0]==MAX31850_CHIPID))) {
      num_sensors++;
    }
  }
  for (byte i = 0; i < num_sensors; i++) {
    ds18x20_index[i] = i;
  }
  for (byte i = 0; i < num_sensors; i++) {
    for (byte j = i + 1; j < num_sensors; j++) {
      if (uint32_t(ds18x20_address[ds18x20_index[i]]) > uint32_t(ds18x20_address[ds18x20_index[j]])) {
        std::swap(ds18x20_index[i], ds18x20_index[j]);
      }
    }
  }
  ds18x20_sensors = num_sensors;
}

uint8_t Ds18x20Sensors()
{
  return ds18x20_sensors;
}

String Ds18x20Addresses(uint8_t sensor)
{
  char address[20];

  for (byte i = 0; i < 8; i++) {
    sprintf(address+2*i, "%02X", ds18x20_address[ds18x20_index[sensor]][i]);
  }
  return String(address);
}

void Ds18x20Convert()
{
  ds->reset();
  ds->write(W1_SKIP_ROM);
  ds->write(W1_CONVERT_TEMP);

}

boolean Ds18x20Read(uint8_t sensor, float &t)
{
  byte data[12];
  int8_t sign = 1;
  uint16_t temp12 = 0;
  int16_t temp14 = 0;
  float temp9 = 0.0;
  uint8_t present = 0;

  t = NAN;

  ds->reset();
  ds->select(ds18x20_address[ds18x20_index[sensor]]);
  ds->write(W1_READ_SCRATCHPAD);

  for (byte i = 0; i < 9; i++) {
    data[i] = ds->read();
  }
  if (OneWire::crc8(data, 8) == data[8]) {
    switch(ds18x20_address[ds18x20_index[sensor]][0]) {
    case DS18S20_CHIPID:
      if (data[1] > 0x80) {
        data[0] = (~data[0]) +1;
        sign = -1;
      }
      if (data[0] & 1) {
        temp9 = ((data[0] >> 1) + 0.5) * sign;
      } else {
        temp9 = (data[0] >> 1) * sign;
      }
      t = ConvertTemp((temp9 - 0.25) + ((16.0 - data[6]) / 16.0));
      break;
    case DS18B20_CHIPID:
      temp12 = (data[1] << 8) + data[0];
      if (temp12 > 2047) {
        temp12 = (~temp12) +1;
        sign = -1;
      }
      t = ConvertTemp(sign * temp12 * 0.0625);
      break;
    case MAX31850_CHIPID:
        temp14 = (data[1] << 8) + (data[0] & 0xFC);
        t = ConvertTemp(temp14 * 0.0625);
      break;
    }
  }
  return (!isnan(t));
}



void Ds18x20Type(uint8_t sensor)
{
  strcpy_P(ds18x20_types, PSTR("DS18x20"));
  switch(ds18x20_address[ds18x20_index[sensor]][0]) {
    case DS18S20_CHIPID:
      strcpy_P(ds18x20_types, PSTR("DS18S20"));
      break;
    case DS18B20_CHIPID:
      strcpy_P(ds18x20_types, PSTR("DS18B20"));
      break;
    case MAX31850_CHIPID:
      strcpy_P(ds18x20_types, PSTR("MAX31850"));
      break;
  }
}

void Ds18x20Show(boolean json)
{
  char temperature[10];
  char stemp[10];
  float t;

  byte dsxflg = 0;
  for (byte i = 0; i < Ds18x20Sensors(); i++) {
    if (Ds18x20Read(i, t)) {
      Ds18x20Type(i);
      dtostrfd(t, Settings.flag2.temperature_resolution, temperature);

      if (json) {
        if (!dsxflg) {
          snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s,\"DS18x20\":{"), mqtt_data);
          stemp[0] = '\0';
        }
        dsxflg++;
        snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s%s\"DS%d\":{\"" D_JSON_TYPE "\":\"%s\",\"" D_JSON_ADDRESS "\":\"%s\",\"" D_JSON_TEMPERATURE "\":%s}"),
          mqtt_data, stemp, i +1, ds18x20_types, Ds18x20Addresses(i).c_str(), temperature);
        strlcpy(stemp, ",", sizeof(stemp));
#ifdef USE_DOMOTICZ
        if ((0 == tele_period) && (1 == dsxflg)) {
          DomoticzSensor(DZ_TEMP, temperature);
        }
#endif
#ifdef USE_KNX
        if ((0 == tele_period) && (1 == dsxflg)) {
          KnxSensor(KNX_TEMPERATURE, t);
        }
#endif
#ifdef USE_WEBSERVER
      } else {
        snprintf_P(stemp, sizeof(stemp), PSTR("%s-%d"), ds18x20_types, i +1);
        snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_SNS_TEMP, mqtt_data, stemp, temperature, TempUnit());
#endif
      }
    }
  }
  if (json) {
    if (dsxflg) {
      snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s}"), mqtt_data);
    }
  }
  Ds18x20Search();
  Ds18x20Convert();
}





#define XSNS_05 

boolean Xsns05(byte function)
{
  boolean result = false;

  if (pin[GPIO_DSB] < 99) {
    switch (function) {
      case FUNC_INIT:
        Ds18x20Init();
        break;
      case FUNC_PREP_BEFORE_TELEPERIOD:
        Ds18x20Search();
        Ds18x20Convert();
        break;
      case FUNC_JSON_APPEND:
        Ds18x20Show(1);
        break;
#ifdef USE_WEBSERVER
      case FUNC_WEB_APPEND:
        Ds18x20Show(0);
        break;
#endif
    }
  }
  return result;
}

#endif
# 1 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_06_dht.ino"
# 20 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_06_dht.ino"
#ifdef USE_DHT
# 29 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_06_dht.ino"
#define DHT_MAX_SENSORS 3
#define DHT_MAX_RETRY 8

uint32_t dht_max_cycles;
uint8_t dht_data[5];
byte dht_sensors = 0;

struct DHTSTRUCT {
  byte pin;
  byte type;
  char stype[12];
  uint32_t lastreadtime;
  uint8_t lastresult;
  float t = NAN;
  float h = NAN;
} Dht[DHT_MAX_SENSORS];

void DhtReadPrep()
{
  for (byte i = 0; i < dht_sensors; i++) {
    digitalWrite(Dht[i].pin, HIGH);
  }
}

int32_t DhtExpectPulse(byte sensor, bool level)
{
  int32_t count = 0;

  while (digitalRead(Dht[sensor].pin) == level) {
    if (count++ >= (int32_t)dht_max_cycles) {
      return -1;
    }
  }
  return count;
}

boolean DhtRead(byte sensor)
{
  int32_t cycles[80];
  uint8_t error = 0;

  dht_data[0] = dht_data[1] = dht_data[2] = dht_data[3] = dht_data[4] = 0;




  if (Dht[sensor].lastresult > DHT_MAX_RETRY) {
    Dht[sensor].lastresult = 0;
    digitalWrite(Dht[sensor].pin, HIGH);
    delay(250);
  }
  pinMode(Dht[sensor].pin, OUTPUT);
  digitalWrite(Dht[sensor].pin, LOW);

  if (GPIO_SI7021 == Dht[sensor].type) {
    delayMicroseconds(500);
  } else {
    delay(20);
  }

  noInterrupts();
  digitalWrite(Dht[sensor].pin, HIGH);
  delayMicroseconds(40);
  pinMode(Dht[sensor].pin, INPUT_PULLUP);
  delayMicroseconds(10);
  if (-1 == DhtExpectPulse(sensor, LOW)) {
    AddLog_P(LOG_LEVEL_DEBUG, PSTR(D_LOG_DHT D_TIMEOUT_WAITING_FOR " " D_START_SIGNAL_LOW " " D_PULSE));
    error = 1;
  }
  else if (-1 == DhtExpectPulse(sensor, HIGH)) {
    AddLog_P(LOG_LEVEL_DEBUG, PSTR(D_LOG_DHT D_TIMEOUT_WAITING_FOR " " D_START_SIGNAL_HIGH " " D_PULSE));
    error = 1;
  }
  else {
    for (int i = 0; i < 80; i += 2) {
      cycles[i] = DhtExpectPulse(sensor, LOW);
      cycles[i+1] = DhtExpectPulse(sensor, HIGH);
    }
  }
  interrupts();
  if (error) { return false; }

  for (int i = 0; i < 40; ++i) {
    int32_t lowCycles = cycles[2*i];
    int32_t highCycles = cycles[2*i+1];
    if ((-1 == lowCycles) || (-1 == highCycles)) {
      AddLog_P(LOG_LEVEL_DEBUG, PSTR(D_LOG_DHT D_TIMEOUT_WAITING_FOR " " D_PULSE));
      return false;
    }
    dht_data[i/8] <<= 1;
    if (highCycles > lowCycles) {
      dht_data[i / 8] |= 1;
    }
  }

  uint8_t checksum = (dht_data[0] + dht_data[1] + dht_data[2] + dht_data[3]) & 0xFF;
  if (dht_data[4] != checksum) {
    snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_DHT D_CHECKSUM_FAILURE " %02X, %02X, %02X, %02X, %02X =? %02X"),
      dht_data[0], dht_data[1], dht_data[2], dht_data[3], dht_data[4], checksum);
    AddLog(LOG_LEVEL_DEBUG);
    return false;
  }

  return true;
}

void DhtReadTempHum(byte sensor)
{
  if ((NAN == Dht[sensor].h) || (Dht[sensor].lastresult > DHT_MAX_RETRY)) {
    Dht[sensor].t = NAN;
    Dht[sensor].h = NAN;
  }
  if (DhtRead(sensor)) {
    switch (Dht[sensor].type) {
    case GPIO_DHT11:
      Dht[sensor].h = dht_data[0];
      Dht[sensor].t = dht_data[2] + ((float)dht_data[3] * 0.1f);
      break;
    case GPIO_DHT22:
    case GPIO_SI7021:
      Dht[sensor].h = ((dht_data[0] << 8) | dht_data[1]) * 0.1;
      Dht[sensor].t = (((dht_data[2] & 0x7F) << 8 ) | dht_data[3]) * 0.1;
      if (dht_data[2] & 0x80) {
        Dht[sensor].t *= -1;
      }
      break;
    }
    Dht[sensor].t = ConvertTemp(Dht[sensor].t);
    Dht[sensor].lastresult = 0;
  } else {
    Dht[sensor].lastresult++;
  }
}

boolean DhtSetup(byte pin, byte type)
{
  boolean success = false;

  if (dht_sensors < DHT_MAX_SENSORS) {
    Dht[dht_sensors].pin = pin;
    Dht[dht_sensors].type = type;
    dht_sensors++;
    success = true;
  }
  return success;
}



void DhtInit()
{
  dht_max_cycles = microsecondsToClockCycles(1000);

  for (byte i = 0; i < dht_sensors; i++) {
    pinMode(Dht[i].pin, INPUT_PULLUP);
    Dht[i].lastreadtime = 0;
    Dht[i].lastresult = 0;
    GetTextIndexed(Dht[i].stype, sizeof(Dht[i].stype), Dht[i].type, kSensorNames);
    if (dht_sensors > 1) {
      snprintf_P(Dht[i].stype, sizeof(Dht[i].stype), PSTR("%s-%02d"), Dht[i].stype, Dht[i].pin);
    }
  }
}

void DhtEverySecond()
{
  if (uptime &1) {

    DhtReadPrep();
  } else {
    for (byte i = 0; i < dht_sensors; i++) {

      DhtReadTempHum(i);
    }
  }
}

void DhtShow(boolean json)
{
  char temperature[10];
  char humidity[10];

  for (byte i = 0; i < dht_sensors; i++) {
    dtostrfd(Dht[i].t, Settings.flag2.temperature_resolution, temperature);
    dtostrfd(Dht[i].h, Settings.flag2.humidity_resolution, humidity);

    if (json) {
      snprintf_P(mqtt_data, sizeof(mqtt_data), JSON_SNS_TEMPHUM, mqtt_data, Dht[i].stype, temperature, humidity);
#ifdef USE_DOMOTICZ
      if ((0 == tele_period) && (0 == i)) {
        DomoticzTempHumSensor(temperature, humidity);
      }
#endif
#ifdef USE_KNX
      if ((0 == tele_period) && (0 == i)) {
        KnxSensor(KNX_TEMPERATURE, Dht[i].t);
        KnxSensor(KNX_HUMIDITY, Dht[i].h);
      }
#endif
#ifdef USE_WEBSERVER
    } else {
      snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_SNS_TEMP, mqtt_data, Dht[i].stype, temperature, TempUnit());
      snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_SNS_HUM, mqtt_data, Dht[i].stype, humidity);
#endif
    }
  }
}





#define XSNS_06 

boolean Xsns06(byte function)
{
  boolean result = false;

  if (dht_flg) {
    switch (function) {
      case FUNC_INIT:
        DhtInit();
        break;
      case FUNC_EVERY_SECOND:
        DhtEverySecond();
        break;
      case FUNC_JSON_APPEND:
        DhtShow(1);
        break;
#ifdef USE_WEBSERVER
      case FUNC_WEB_APPEND:
        DhtShow(0);
        break;
#endif
    }
  }
  return result;
}

#endif
# 1 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_07_sht1x.ino"
# 20 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_07_sht1x.ino"
#ifdef USE_I2C
#ifdef USE_SHT
# 31 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_07_sht1x.ino"
enum {
  SHT1X_CMD_MEASURE_TEMP = B00000011,
  SHT1X_CMD_MEASURE_RH = B00000101,
  SHT1X_CMD_SOFT_RESET = B00011110
};

uint8_t sht_sda_pin;
uint8_t sht_scl_pin;
uint8_t sht_type = 0;
char sht_types[] = "SHT1X";
uint8_t sht_valid = 0;
float sht_temperature = 0;
float sht_humidity = 0;

boolean ShtReset()
{
  pinMode(sht_sda_pin, INPUT_PULLUP);
  pinMode(sht_scl_pin, OUTPUT);
  delay(11);
  for (byte i = 0; i < 9; i++) {
    digitalWrite(sht_scl_pin, HIGH);
    digitalWrite(sht_scl_pin, LOW);
  }
  boolean success = ShtSendCommand(SHT1X_CMD_SOFT_RESET);
  delay(11);
  return success;
}

boolean ShtSendCommand(const byte cmd)
{
  pinMode(sht_sda_pin, OUTPUT);

  digitalWrite(sht_sda_pin, HIGH);
  digitalWrite(sht_scl_pin, HIGH);
  digitalWrite(sht_sda_pin, LOW);
  digitalWrite(sht_scl_pin, LOW);
  digitalWrite(sht_scl_pin, HIGH);
  digitalWrite(sht_sda_pin, HIGH);
  digitalWrite(sht_scl_pin, LOW);

  shiftOut(sht_sda_pin, sht_scl_pin, MSBFIRST, cmd);

  boolean ackerror = false;
  digitalWrite(sht_scl_pin, HIGH);
  pinMode(sht_sda_pin, INPUT_PULLUP);
  if (digitalRead(sht_sda_pin) != LOW) {
    ackerror = true;
  }
  digitalWrite(sht_scl_pin, LOW);
  delayMicroseconds(1);
  if (digitalRead(sht_sda_pin) != HIGH) {
    ackerror = true;
  }
  if (ackerror) {
    sht_type = 0;
    AddLog_P(LOG_LEVEL_DEBUG, PSTR(D_LOG_SHT1 D_SENSOR_DID_NOT_ACK_COMMAND));
  }
  return (!ackerror);
}

boolean ShtAwaitResult()
{

  for (byte i = 0; i < 16; i++) {
    if (LOW == digitalRead(sht_sda_pin)) {
      return true;
    }
    delay(20);
  }
  AddLog_P(LOG_LEVEL_DEBUG, PSTR(D_LOG_SHT1 D_SENSOR_BUSY));
  sht_type = 0;
  return false;
}

int ShtReadData()
{
  int val = 0;


  val = shiftIn(sht_sda_pin, sht_scl_pin, 8);
  val <<= 8;

  pinMode(sht_sda_pin, OUTPUT);
  digitalWrite(sht_sda_pin, LOW);
  digitalWrite(sht_scl_pin, HIGH);
  digitalWrite(sht_scl_pin, LOW);
  pinMode(sht_sda_pin, INPUT_PULLUP);

  val |= shiftIn(sht_sda_pin, sht_scl_pin, 8);

  digitalWrite(sht_scl_pin, HIGH);
  digitalWrite(sht_scl_pin, LOW);
  return val;
}

boolean ShtRead()
{
  if (sht_valid) { sht_valid--; }
  if (!ShtReset()) { return false; }
  if (!ShtSendCommand(SHT1X_CMD_MEASURE_TEMP)) { return false; }
  if (!ShtAwaitResult()) { return false; }
  float tempRaw = ShtReadData();
  if (!ShtSendCommand(SHT1X_CMD_MEASURE_RH)) { return false; }
  if (!ShtAwaitResult()) { return false; }
  float humRaw = ShtReadData();


  const float d1 = -39.7;
  const float d2 = 0.01;
  sht_temperature = d1 + (tempRaw * d2);
  const float c1 = -2.0468;
  const float c2 = 0.0367;
  const float c3 = -1.5955E-6;
  const float t1 = 0.01;
  const float t2 = 0.00008;
  float rhLinear = c1 + c2 * humRaw + c3 * humRaw * humRaw;
  sht_humidity = (sht_temperature - 25) * (t1 + t2 * humRaw) + rhLinear;
  sht_temperature = ConvertTemp(sht_temperature);

  SetGlobalValues(sht_temperature, sht_humidity);

  sht_valid = SENSOR_MAX_MISS;
  return true;
}



void ShtDetect()
{
  if (sht_type) {
    return;
  }

  sht_sda_pin = pin[GPIO_I2C_SDA];
  sht_scl_pin = pin[GPIO_I2C_SCL];
  if (ShtRead()) {
    sht_type = 1;
    AddLog_P(LOG_LEVEL_DEBUG, PSTR(D_LOG_I2C D_SHT1X_FOUND));
  } else {
    Wire.begin(sht_sda_pin, sht_scl_pin);
    sht_type = 0;
  }
}

void ShtEverySecond()
{
  if (sht_type && !(uptime %4)) {

    if (!ShtRead()) {
      AddLogMissed(sht_types, sht_valid);

    }
  }
}

void ShtShow(boolean json)
{
  if (sht_valid) {
    char temperature[10];
    char humidity[10];

    dtostrfd(sht_temperature, Settings.flag2.temperature_resolution, temperature);
    dtostrfd(sht_humidity, Settings.flag2.humidity_resolution, humidity);

    if (json) {
      snprintf_P(mqtt_data, sizeof(mqtt_data), JSON_SNS_TEMPHUM, mqtt_data, sht_types, temperature, humidity);
#ifdef USE_DOMOTICZ
      if (0 == tele_period) {
        DomoticzTempHumSensor(temperature, humidity);
      }
#endif
#ifdef USE_KNX
      if (0 == tele_period) {
        KnxSensor(KNX_TEMPERATURE, sht_temperature);
        KnxSensor(KNX_HUMIDITY, sht_humidity);
      }
#endif
#ifdef USE_WEBSERVER
    } else {
      snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_SNS_TEMP, mqtt_data, sht_types, temperature, TempUnit());
      snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_SNS_HUM, mqtt_data, sht_types, humidity);
#endif
    }
  }
}





#define XSNS_07 

boolean Xsns07(byte function)
{
  boolean result = false;

  if (i2c_flg) {
    switch (function) {

      case FUNC_INIT:
        ShtDetect();
        break;
      case FUNC_EVERY_SECOND:
        ShtEverySecond();
        break;
      case FUNC_JSON_APPEND:
        ShtShow(1);
        break;
#ifdef USE_WEBSERVER
      case FUNC_WEB_APPEND:
        ShtShow(0);
        break;
#endif
    }
  }
  return result;
}

#endif
#endif
# 1 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_08_htu21.ino"
# 20 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_08_htu21.ino"
#ifdef USE_I2C
#ifdef USE_HTU
# 30 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_08_htu21.ino"
#define HTU21_ADDR 0x40

#define SI7013_CHIPID 0x0D
#define SI7020_CHIPID 0x14
#define SI7021_CHIPID 0x15
#define HTU21_CHIPID 0x32

#define HTU21_READTEMP 0xE3
#define HTU21_READHUM 0xE5
#define HTU21_WRITEREG 0xE6
#define HTU21_READREG 0xE7
#define HTU21_RESET 0xFE
#define HTU21_HEATER_WRITE 0x51
#define HTU21_HEATER_READ 0x11
#define HTU21_SERIAL2_READ1 0xFC
#define HTU21_SERIAL2_READ2 0xC9

#define HTU21_HEATER_ON 0x04
#define HTU21_HEATER_OFF 0xFB

#define HTU21_RES_RH12_T14 0x00
#define HTU21_RES_RH8_T12 0x01
#define HTU21_RES_RH10_T13 0x80
#define HTU21_RES_RH11_T11 0x81

#define HTU21_CRC8_POLYNOM 0x13100

const char kHtuTypes[] PROGMEM = "HTU21|SI7013|SI7020|SI7021|T/RH?";

uint8_t htu_address;
uint8_t htu_type = 0;
uint8_t htu_delay_temp;
uint8_t htu_delay_humidity = 50;
uint8_t htu_valid = 0;
float htu_temperature = 0;
float htu_humidity = 0;
char htu_types[7];

uint8_t HtuCheckCrc8(uint16_t data)
{
  for (uint8_t bit = 0; bit < 16; bit++) {
    if (data & 0x8000) {
      data = (data << 1) ^ HTU21_CRC8_POLYNOM;
    } else {
      data <<= 1;
    }
  }
  return data >>= 8;
}

uint8_t HtuReadDeviceId(void)
{
  uint16_t deviceID = 0;
  uint8_t checksum = 0;

  Wire.beginTransmission(HTU21_ADDR);
  Wire.write(HTU21_SERIAL2_READ1);
  Wire.write(HTU21_SERIAL2_READ2);
  Wire.endTransmission();

  Wire.requestFrom(HTU21_ADDR, 3);
  deviceID = Wire.read() << 8;
  deviceID |= Wire.read();
  checksum = Wire.read();
  if (HtuCheckCrc8(deviceID) == checksum) {
    deviceID = deviceID >> 8;
  } else {
    deviceID = 0;
  }
  return (uint8_t)deviceID;
}

void HtuSetResolution(uint8_t resolution)
{
  uint8_t current = I2cRead8(HTU21_ADDR, HTU21_READREG);
  current &= 0x7E;
  current |= resolution;
  I2cWrite8(HTU21_ADDR, HTU21_WRITEREG, current);
}

void HtuReset(void)
{
  Wire.beginTransmission(HTU21_ADDR);
  Wire.write(HTU21_RESET);
  Wire.endTransmission();
  delay(15);
}

void HtuHeater(uint8_t heater)
{
  uint8_t current = I2cRead8(HTU21_ADDR, HTU21_READREG);

  switch(heater)
  {
    case HTU21_HEATER_ON : current |= heater;
                            break;
    case HTU21_HEATER_OFF : current &= heater;
                            break;
    default : current &= heater;
                            break;
  }
  I2cWrite8(HTU21_ADDR, HTU21_WRITEREG, current);
}

void HtuInit()
{
  HtuReset();
  HtuHeater(HTU21_HEATER_OFF);
  HtuSetResolution(HTU21_RES_RH12_T14);
}

boolean HtuRead()
{
  uint8_t checksum = 0;
  uint16_t sensorval = 0;

  if (htu_valid) { htu_valid--; }

  Wire.beginTransmission(HTU21_ADDR);
  Wire.write(HTU21_READTEMP);
  if (Wire.endTransmission() != 0) { return false; }
  delay(htu_delay_temp);

  Wire.requestFrom(HTU21_ADDR, 3);
  if (3 == Wire.available()) {
    sensorval = Wire.read() << 8;
    sensorval |= Wire.read();
    checksum = Wire.read();
  }
  if (HtuCheckCrc8(sensorval) != checksum) { return false; }

  htu_temperature = ConvertTemp(0.002681 * (float)sensorval - 46.85);

  Wire.beginTransmission(HTU21_ADDR);
  Wire.write(HTU21_READHUM);
  if (Wire.endTransmission() != 0) { return false; }
  delay(htu_delay_humidity);

  Wire.requestFrom(HTU21_ADDR, 3);
  if (3 <= Wire.available()) {
    sensorval = Wire.read() << 8;
    sensorval |= Wire.read();
    checksum = Wire.read();
  }
  if (HtuCheckCrc8(sensorval) != checksum) { return false; }

  sensorval ^= 0x02;
  htu_humidity = 0.001907 * (float)sensorval - 6;
  if (htu_humidity > 100) { htu_humidity = 100.0; }
  if (htu_humidity < 0) { htu_humidity = 0.01; }

  if ((0.00 == htu_humidity) && (0.00 == htu_temperature)) {
    htu_humidity = 0.0;
  }
  if ((htu_temperature > 0.00) && (htu_temperature < 80.00)) {
    htu_humidity = (-0.15) * (25 - htu_temperature) + htu_humidity;
  }

  SetGlobalValues(htu_temperature, htu_humidity);

  htu_valid = SENSOR_MAX_MISS;
  return true;
}



void HtuDetect()
{
  if (htu_type) { return; }

  htu_address = HTU21_ADDR;
  htu_type = HtuReadDeviceId();
  if (htu_type) {
    uint8_t index = 0;
    HtuInit();
    switch (htu_type) {
      case HTU21_CHIPID:
        htu_delay_temp = 50;
        htu_delay_humidity = 16;
        break;
      case SI7021_CHIPID:
        index++;
      case SI7020_CHIPID:
        index++;
      case SI7013_CHIPID:
        index++;
        htu_delay_temp = 12;
        htu_delay_humidity = 23;
        break;
      default:
        index = 4;
        htu_delay_temp = 50;
        htu_delay_humidity = 23;
    }
    GetTextIndexed(htu_types, sizeof(htu_types), index, kHtuTypes);
    snprintf_P(log_data, sizeof(log_data), S_LOG_I2C_FOUND_AT, htu_types, htu_address);
    AddLog(LOG_LEVEL_DEBUG);
  }
}

void HtuEverySecond()
{
  if (92 == (uptime %100)) {

    HtuDetect();
  }
  else if (uptime &1) {

    if (htu_type) {
      if (!HtuRead()) {
        AddLogMissed(htu_types, htu_valid);

      }
    }
  }
}

void HtuShow(boolean json)
{
  if (htu_valid) {
    char temperature[10];
    char humidity[10];

    dtostrfd(htu_temperature, Settings.flag2.temperature_resolution, temperature);
    dtostrfd(htu_humidity, Settings.flag2.humidity_resolution, humidity);

    if (json) {
      snprintf_P(mqtt_data, sizeof(mqtt_data), JSON_SNS_TEMPHUM, mqtt_data, htu_types, temperature, humidity);
#ifdef USE_DOMOTICZ
      if (0 == tele_period) {
        DomoticzTempHumSensor(temperature, humidity);
      }
#endif
#ifdef USE_KNX
      if (0 == tele_period) {
        KnxSensor(KNX_TEMPERATURE, htu_temperature);
        KnxSensor(KNX_HUMIDITY, htu_humidity);
      }
#endif
#ifdef USE_WEBSERVER
    } else {
      snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_SNS_TEMP, mqtt_data, htu_types, temperature, TempUnit());
      snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_SNS_HUM, mqtt_data, htu_types, humidity);
#endif
    }
  }
}





#define XSNS_08 

boolean Xsns08(byte function)
{
  boolean result = false;

  if (i2c_flg) {
    switch (function) {
      case FUNC_INIT:
        HtuDetect();
        break;
      case FUNC_EVERY_SECOND:
        HtuEverySecond();
        break;
      case FUNC_JSON_APPEND:
        HtuShow(1);
        break;
#ifdef USE_WEBSERVER
      case FUNC_WEB_APPEND:
        HtuShow(0);
        break;
#endif
    }
  }
  return result;
}

#endif
#endif
# 1 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_09_bmp.ino"
# 20 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_09_bmp.ino"
#ifdef USE_I2C
#ifdef USE_BMP
# 30 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_09_bmp.ino"
#define BMP_ADDR1 0x76
#define BMP_ADDR2 0x77

#define BMP180_CHIPID 0x55
#define BMP280_CHIPID 0x58
#define BME280_CHIPID 0x60
#define BME680_CHIPID 0x61

#define BMP_REGISTER_CHIPID 0xD0

#define BMP_MAX_SENSORS 2

const char kBmpTypes[] PROGMEM = "BMP180|BMP280|BME280|BME680";
uint8_t bmp_addresses[] = { BMP_ADDR1, BMP_ADDR2 };
uint8_t bmp_count = 0;
uint8_t bmp_once = 1;

struct BMPSTRUCT {
  uint8_t bmp_address;
  char bmp_name[7];
  uint8_t bmp_type = 0;
  uint8_t bmp_model = 0;

  uint8_t bmp_valid = 0;
#ifdef USE_BME680
  uint8_t bme680_state = 0;
  float bmp_gas_resistance = 0.0;
#endif
  float bmp_temperature = 0.0;
  float bmp_pressure = 0.0;
  float bmp_humidity = 0.0;
} bmp_sensors[BMP_MAX_SENSORS];





#define BMP180_REG_CONTROL 0xF4
#define BMP180_REG_RESULT 0xF6
#define BMP180_TEMPERATURE 0x2E
#define BMP180_PRESSURE3 0xF4

#define BMP180_AC1 0xAA
#define BMP180_AC2 0xAC
#define BMP180_AC3 0xAE
#define BMP180_AC4 0xB0
#define BMP180_AC5 0xB2
#define BMP180_AC6 0xB4
#define BMP180_VB1 0xB6
#define BMP180_VB2 0xB8
#define BMP180_MB 0xBA
#define BMP180_MC 0xBC
#define BMP180_MD 0xBE

#define BMP180_OSS 3

struct BMP180CALIBDATA {
  int16_t cal_ac1;
  int16_t cal_ac2;
  int16_t cal_ac3;
  int16_t cal_b1;
  int16_t cal_b2;
  int16_t cal_mc;
  int16_t cal_md;
  uint16_t cal_ac4;
  uint16_t cal_ac5;
  uint16_t cal_ac6;
} bmp180_cal_data[BMP_MAX_SENSORS];

boolean Bmp180Calibration(uint8_t bmp_idx)
{
  bmp180_cal_data[bmp_idx].cal_ac1 = I2cRead16(bmp_sensors[bmp_idx].bmp_address, BMP180_AC1);
  bmp180_cal_data[bmp_idx].cal_ac2 = I2cRead16(bmp_sensors[bmp_idx].bmp_address, BMP180_AC2);
  bmp180_cal_data[bmp_idx].cal_ac3 = I2cRead16(bmp_sensors[bmp_idx].bmp_address, BMP180_AC3);
  bmp180_cal_data[bmp_idx].cal_ac4 = I2cRead16(bmp_sensors[bmp_idx].bmp_address, BMP180_AC4);
  bmp180_cal_data[bmp_idx].cal_ac5 = I2cRead16(bmp_sensors[bmp_idx].bmp_address, BMP180_AC5);
  bmp180_cal_data[bmp_idx].cal_ac6 = I2cRead16(bmp_sensors[bmp_idx].bmp_address, BMP180_AC6);
  bmp180_cal_data[bmp_idx].cal_b1 = I2cRead16(bmp_sensors[bmp_idx].bmp_address, BMP180_VB1);
  bmp180_cal_data[bmp_idx].cal_b2 = I2cRead16(bmp_sensors[bmp_idx].bmp_address, BMP180_VB2);
  bmp180_cal_data[bmp_idx].cal_mc = I2cRead16(bmp_sensors[bmp_idx].bmp_address, BMP180_MC);
  bmp180_cal_data[bmp_idx].cal_md = I2cRead16(bmp_sensors[bmp_idx].bmp_address, BMP180_MD);


  if (!bmp180_cal_data[bmp_idx].cal_ac1 |
      !bmp180_cal_data[bmp_idx].cal_ac2 |
      !bmp180_cal_data[bmp_idx].cal_ac3 |
      !bmp180_cal_data[bmp_idx].cal_ac4 |
      !bmp180_cal_data[bmp_idx].cal_ac5 |
      !bmp180_cal_data[bmp_idx].cal_ac6 |
      !bmp180_cal_data[bmp_idx].cal_b1 |
      !bmp180_cal_data[bmp_idx].cal_b2 |
      !bmp180_cal_data[bmp_idx].cal_mc |
      !bmp180_cal_data[bmp_idx].cal_md) {
    return false;
  }

  if ((bmp180_cal_data[bmp_idx].cal_ac1 == (int16_t)0xFFFF) |
      (bmp180_cal_data[bmp_idx].cal_ac2 == (int16_t)0xFFFF) |
      (bmp180_cal_data[bmp_idx].cal_ac3 == (int16_t)0xFFFF) |
      (bmp180_cal_data[bmp_idx].cal_ac4 == 0xFFFF) |
      (bmp180_cal_data[bmp_idx].cal_ac5 == 0xFFFF) |
      (bmp180_cal_data[bmp_idx].cal_ac6 == 0xFFFF) |
      (bmp180_cal_data[bmp_idx].cal_b1 == (int16_t)0xFFFF) |
      (bmp180_cal_data[bmp_idx].cal_b2 == (int16_t)0xFFFF) |
      (bmp180_cal_data[bmp_idx].cal_mc == (int16_t)0xFFFF) |
      (bmp180_cal_data[bmp_idx].cal_md == (int16_t)0xFFFF)) {
    return false;
  }
  return true;
}

void Bmp180Read(uint8_t bmp_idx)
{
  I2cWrite8(bmp_sensors[bmp_idx].bmp_address, BMP180_REG_CONTROL, BMP180_TEMPERATURE);
  delay(5);
  int ut = I2cRead16(bmp_sensors[bmp_idx].bmp_address, BMP180_REG_RESULT);
  int32_t xt1 = (ut - (int32_t)bmp180_cal_data[bmp_idx].cal_ac6) * ((int32_t)bmp180_cal_data[bmp_idx].cal_ac5) >> 15;
  int32_t xt2 = ((int32_t)bmp180_cal_data[bmp_idx].cal_mc << 11) / (xt1 + (int32_t)bmp180_cal_data[bmp_idx].cal_md);
  int32_t bmp180_b5 = xt1 + xt2;
  bmp_sensors[bmp_idx].bmp_temperature = ((bmp180_b5 + 8) >> 4) / 10.0;

  I2cWrite8(bmp_sensors[bmp_idx].bmp_address, BMP180_REG_CONTROL, BMP180_PRESSURE3);
  delay(2 + (4 << BMP180_OSS));
  uint32_t up = I2cRead24(bmp_sensors[bmp_idx].bmp_address, BMP180_REG_RESULT);
  up >>= (8 - BMP180_OSS);

  int32_t b6 = bmp180_b5 - 4000;
  int32_t x1 = ((int32_t)bmp180_cal_data[bmp_idx].cal_b2 * ((b6 * b6) >> 12)) >> 11;
  int32_t x2 = ((int32_t)bmp180_cal_data[bmp_idx].cal_ac2 * b6) >> 11;
  int32_t x3 = x1 + x2;
  int32_t b3 = ((((int32_t)bmp180_cal_data[bmp_idx].cal_ac1 * 4 + x3) << BMP180_OSS) + 2) >> 2;

  x1 = ((int32_t)bmp180_cal_data[bmp_idx].cal_ac3 * b6) >> 13;
  x2 = ((int32_t)bmp180_cal_data[bmp_idx].cal_b1 * ((b6 * b6) >> 12)) >> 16;
  x3 = ((x1 + x2) + 2) >> 2;
  uint32_t b4 = ((uint32_t)bmp180_cal_data[bmp_idx].cal_ac4 * (uint32_t)(x3 + 32768)) >> 15;
  uint32_t b7 = ((uint32_t)up - b3) * (uint32_t)(50000UL >> BMP180_OSS);

  int32_t p;
  if (b7 < 0x80000000) {
    p = (b7 * 2) / b4;
  }
  else {
    p = (b7 / b4) * 2;
  }
  x1 = (p >> 8) * (p >> 8);
  x1 = (x1 * 3038) >> 16;
  x2 = (-7357 * p) >> 16;
  p += ((x1 + x2 + (int32_t)3791) >> 4);
  bmp_sensors[bmp_idx].bmp_pressure = (float)p / 100.0;
}







#define BME280_REGISTER_CONTROLHUMID 0xF2
#define BME280_REGISTER_CONTROL 0xF4
#define BME280_REGISTER_CONFIG 0xF5
#define BME280_REGISTER_PRESSUREDATA 0xF7
#define BME280_REGISTER_TEMPDATA 0xFA
#define BME280_REGISTER_HUMIDDATA 0xFD

#define BME280_REGISTER_DIG_T1 0x88
#define BME280_REGISTER_DIG_T2 0x8A
#define BME280_REGISTER_DIG_T3 0x8C
#define BME280_REGISTER_DIG_P1 0x8E
#define BME280_REGISTER_DIG_P2 0x90
#define BME280_REGISTER_DIG_P3 0x92
#define BME280_REGISTER_DIG_P4 0x94
#define BME280_REGISTER_DIG_P5 0x96
#define BME280_REGISTER_DIG_P6 0x98
#define BME280_REGISTER_DIG_P7 0x9A
#define BME280_REGISTER_DIG_P8 0x9C
#define BME280_REGISTER_DIG_P9 0x9E
#define BME280_REGISTER_DIG_H1 0xA1
#define BME280_REGISTER_DIG_H2 0xE1
#define BME280_REGISTER_DIG_H3 0xE3
#define BME280_REGISTER_DIG_H4 0xE4
#define BME280_REGISTER_DIG_H5 0xE5
#define BME280_REGISTER_DIG_H6 0xE7

struct BME280CALIBDATA
{
  uint16_t dig_T1;
  int16_t dig_T2;
  int16_t dig_T3;
  uint16_t dig_P1;
  int16_t dig_P2;
  int16_t dig_P3;
  int16_t dig_P4;
  int16_t dig_P5;
  int16_t dig_P6;
  int16_t dig_P7;
  int16_t dig_P8;
  int16_t dig_P9;
  int16_t dig_H2;
  int16_t dig_H4;
  int16_t dig_H5;
  uint8_t dig_H1;
  uint8_t dig_H3;
  int8_t dig_H6;
} Bme280CalibrationData[BMP_MAX_SENSORS];

boolean Bmx280Calibrate(uint8_t bmp_idx)
{


  Bme280CalibrationData[bmp_idx].dig_T1 = I2cRead16LE(bmp_sensors[bmp_idx].bmp_address, BME280_REGISTER_DIG_T1);
  Bme280CalibrationData[bmp_idx].dig_T2 = I2cReadS16_LE(bmp_sensors[bmp_idx].bmp_address, BME280_REGISTER_DIG_T2);
  Bme280CalibrationData[bmp_idx].dig_T3 = I2cReadS16_LE(bmp_sensors[bmp_idx].bmp_address, BME280_REGISTER_DIG_T3);
  Bme280CalibrationData[bmp_idx].dig_P1 = I2cRead16LE(bmp_sensors[bmp_idx].bmp_address, BME280_REGISTER_DIG_P1);
  Bme280CalibrationData[bmp_idx].dig_P2 = I2cReadS16_LE(bmp_sensors[bmp_idx].bmp_address, BME280_REGISTER_DIG_P2);
  Bme280CalibrationData[bmp_idx].dig_P3 = I2cReadS16_LE(bmp_sensors[bmp_idx].bmp_address, BME280_REGISTER_DIG_P3);
  Bme280CalibrationData[bmp_idx].dig_P4 = I2cReadS16_LE(bmp_sensors[bmp_idx].bmp_address, BME280_REGISTER_DIG_P4);
  Bme280CalibrationData[bmp_idx].dig_P5 = I2cReadS16_LE(bmp_sensors[bmp_idx].bmp_address, BME280_REGISTER_DIG_P5);
  Bme280CalibrationData[bmp_idx].dig_P6 = I2cReadS16_LE(bmp_sensors[bmp_idx].bmp_address, BME280_REGISTER_DIG_P6);
  Bme280CalibrationData[bmp_idx].dig_P7 = I2cReadS16_LE(bmp_sensors[bmp_idx].bmp_address, BME280_REGISTER_DIG_P7);
  Bme280CalibrationData[bmp_idx].dig_P8 = I2cReadS16_LE(bmp_sensors[bmp_idx].bmp_address, BME280_REGISTER_DIG_P8);
  Bme280CalibrationData[bmp_idx].dig_P9 = I2cReadS16_LE(bmp_sensors[bmp_idx].bmp_address, BME280_REGISTER_DIG_P9);
  if (BME280_CHIPID == bmp_sensors[bmp_idx].bmp_type) {
    Bme280CalibrationData[bmp_idx].dig_H1 = I2cRead8(bmp_sensors[bmp_idx].bmp_address, BME280_REGISTER_DIG_H1);
    Bme280CalibrationData[bmp_idx].dig_H2 = I2cReadS16_LE(bmp_sensors[bmp_idx].bmp_address, BME280_REGISTER_DIG_H2);
    Bme280CalibrationData[bmp_idx].dig_H3 = I2cRead8(bmp_sensors[bmp_idx].bmp_address, BME280_REGISTER_DIG_H3);
    Bme280CalibrationData[bmp_idx].dig_H4 = (I2cRead8(bmp_sensors[bmp_idx].bmp_address, BME280_REGISTER_DIG_H4) << 4) | (I2cRead8(bmp_sensors[bmp_idx].bmp_address, BME280_REGISTER_DIG_H4 + 1) & 0xF);
    Bme280CalibrationData[bmp_idx].dig_H5 = (I2cRead8(bmp_sensors[bmp_idx].bmp_address, BME280_REGISTER_DIG_H5 + 1) << 4) | (I2cRead8(bmp_sensors[bmp_idx].bmp_address, BME280_REGISTER_DIG_H5) >> 4);
    Bme280CalibrationData[bmp_idx].dig_H6 = (int8_t)I2cRead8(bmp_sensors[bmp_idx].bmp_address, BME280_REGISTER_DIG_H6);
    I2cWrite8(bmp_sensors[bmp_idx].bmp_address, BME280_REGISTER_CONTROL, 0x00);

    I2cWrite8(bmp_sensors[bmp_idx].bmp_address, BME280_REGISTER_CONTROLHUMID, 0x01);
    I2cWrite8(bmp_sensors[bmp_idx].bmp_address, BME280_REGISTER_CONFIG, 0xA0);
    I2cWrite8(bmp_sensors[bmp_idx].bmp_address, BME280_REGISTER_CONTROL, 0x27);
  } else {
    I2cWrite8(bmp_sensors[bmp_idx].bmp_address, BME280_REGISTER_CONTROL, 0xB7);
  }

  return true;
}

void Bme280Read(uint8_t bmp_idx)
{
  int32_t adc_T = I2cRead24(bmp_sensors[bmp_idx].bmp_address, BME280_REGISTER_TEMPDATA);
  adc_T >>= 4;

  int32_t vart1 = ((((adc_T >> 3) - ((int32_t)Bme280CalibrationData[bmp_idx].dig_T1 << 1))) * ((int32_t)Bme280CalibrationData[bmp_idx].dig_T2)) >> 11;
  int32_t vart2 = (((((adc_T >> 4) - ((int32_t)Bme280CalibrationData[bmp_idx].dig_T1)) * ((adc_T >> 4) - ((int32_t)Bme280CalibrationData[bmp_idx].dig_T1))) >> 12) *
    ((int32_t)Bme280CalibrationData[bmp_idx].dig_T3)) >> 14;
  int32_t t_fine = vart1 + vart2;
  float T = (t_fine * 5 + 128) >> 8;
  bmp_sensors[bmp_idx].bmp_temperature = T / 100.0;

  int32_t adc_P = I2cRead24(bmp_sensors[bmp_idx].bmp_address, BME280_REGISTER_PRESSUREDATA);
  adc_P >>= 4;

  int64_t var1 = ((int64_t)t_fine) - 128000;
  int64_t var2 = var1 * var1 * (int64_t)Bme280CalibrationData[bmp_idx].dig_P6;
  var2 = var2 + ((var1 * (int64_t)Bme280CalibrationData[bmp_idx].dig_P5) << 17);
  var2 = var2 + (((int64_t)Bme280CalibrationData[bmp_idx].dig_P4) << 35);
  var1 = ((var1 * var1 * (int64_t)Bme280CalibrationData[bmp_idx].dig_P3) >> 8) + ((var1 * (int64_t)Bme280CalibrationData[bmp_idx].dig_P2) << 12);
  var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)Bme280CalibrationData[bmp_idx].dig_P1) >> 33;
  if (0 == var1) {
    return;
  }
  int64_t p = 1048576 - adc_P;
  p = (((p << 31) - var2) * 3125) / var1;
  var1 = (((int64_t)Bme280CalibrationData[bmp_idx].dig_P9) * (p >> 13) * (p >> 13)) >> 25;
  var2 = (((int64_t)Bme280CalibrationData[bmp_idx].dig_P8) * p) >> 19;
  p = ((p + var1 + var2) >> 8) + (((int64_t)Bme280CalibrationData[bmp_idx].dig_P7) << 4);
  bmp_sensors[bmp_idx].bmp_pressure = (float)p / 25600.0;

  if (BMP280_CHIPID == bmp_sensors[bmp_idx].bmp_type) { return; }

  int32_t adc_H = I2cRead16(bmp_sensors[bmp_idx].bmp_address, BME280_REGISTER_HUMIDDATA);

  int32_t v_x1_u32r = (t_fine - ((int32_t)76800));
  v_x1_u32r = (((((adc_H << 14) - (((int32_t)Bme280CalibrationData[bmp_idx].dig_H4) << 20) -
    (((int32_t)Bme280CalibrationData[bmp_idx].dig_H5) * v_x1_u32r)) + ((int32_t)16384)) >> 15) *
    (((((((v_x1_u32r * ((int32_t)Bme280CalibrationData[bmp_idx].dig_H6)) >> 10) *
    (((v_x1_u32r * ((int32_t)Bme280CalibrationData[bmp_idx].dig_H3)) >> 11) + ((int32_t)32768))) >> 10) +
    ((int32_t)2097152)) * ((int32_t)Bme280CalibrationData[bmp_idx].dig_H2) + 8192) >> 14));
  v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) *
    ((int32_t)Bme280CalibrationData[bmp_idx].dig_H1)) >> 4));
  v_x1_u32r = (v_x1_u32r < 0) ? 0 : v_x1_u32r;
  v_x1_u32r = (v_x1_u32r > 419430400) ? 419430400 : v_x1_u32r;
  float h = (v_x1_u32r >> 12);
  bmp_sensors[bmp_idx].bmp_humidity = h / 1024.0;
}

#ifdef USE_BME680




#include <bme680.h>

struct bme680_dev gas_sensor[BMP_MAX_SENSORS];

static void BmeDelayMs(uint32_t ms)
{
  delay(ms);
}

boolean Bme680Init(uint8_t bmp_idx)
{
  gas_sensor[bmp_idx].dev_id = bmp_sensors[bmp_idx].bmp_address;
  gas_sensor[bmp_idx].intf = BME680_I2C_INTF;
  gas_sensor[bmp_idx].read = &I2cReadBuffer;
  gas_sensor[bmp_idx].write = &I2cWriteBuffer;
  gas_sensor[bmp_idx].delay_ms = BmeDelayMs;



  gas_sensor[bmp_idx].amb_temp = 25;

  int8_t rslt = BME680_OK;
  rslt = bme680_init(&gas_sensor[bmp_idx]);
  if (rslt != BME680_OK) { return false; }


  gas_sensor[bmp_idx].tph_sett.os_hum = BME680_OS_2X;
  gas_sensor[bmp_idx].tph_sett.os_pres = BME680_OS_4X;
  gas_sensor[bmp_idx].tph_sett.os_temp = BME680_OS_8X;
  gas_sensor[bmp_idx].tph_sett.filter = BME680_FILTER_SIZE_3;


  gas_sensor[bmp_idx].gas_sett.run_gas = BME680_ENABLE_GAS_MEAS;

  gas_sensor[bmp_idx].gas_sett.heatr_temp = 320;
  gas_sensor[bmp_idx].gas_sett.heatr_dur = 150;



  gas_sensor[bmp_idx].power_mode = BME680_FORCED_MODE;


  uint8_t set_required_settings = BME680_OST_SEL | BME680_OSP_SEL | BME680_OSH_SEL | BME680_FILTER_SEL | BME680_GAS_SENSOR_SEL;


  rslt = bme680_set_sensor_settings(set_required_settings,&gas_sensor[bmp_idx]);
  if (rslt != BME680_OK) { return false; }

  bmp_sensors[bmp_idx].bme680_state = 0;

  return true;
}

void Bme680Read(uint8_t bmp_idx)
{
  int8_t rslt = BME680_OK;

  if (BME680_CHIPID == bmp_sensors[bmp_idx].bmp_type) {
    if (0 == bmp_sensors[bmp_idx].bme680_state) {

      rslt = bme680_set_sensor_mode(&gas_sensor[bmp_idx]);
      if (rslt != BME680_OK) { return; }







      bmp_sensors[bmp_idx].bme680_state = 1;
    } else {
      bmp_sensors[bmp_idx].bme680_state = 0;

      struct bme680_field_data data;
      rslt = bme680_get_sensor_data(&data, &gas_sensor[bmp_idx]);
      if (rslt != BME680_OK) { return; }

      bmp_sensors[bmp_idx].bmp_temperature = data.temperature / 100.0;
      bmp_sensors[bmp_idx].bmp_humidity = data.humidity / 1000.0;
      bmp_sensors[bmp_idx].bmp_pressure = data.pressure / 100.0;

      if (data.status & BME680_GASM_VALID_MSK) {
        bmp_sensors[bmp_idx].bmp_gas_resistance = data.gas_resistance / 1000.0;
      } else {
        bmp_sensors[bmp_idx].bmp_gas_resistance = 0;
      }
    }
  }
  return;
}

#endif



void BmpDetect()
{
  if (bmp_count) return;

  for (byte i = 0; i < BMP_MAX_SENSORS; i++) {
    uint8_t bmp_type = I2cRead8(bmp_addresses[i], BMP_REGISTER_CHIPID);
    if (bmp_type) {
      bmp_sensors[bmp_count].bmp_address = bmp_addresses[i];
      bmp_sensors[bmp_count].bmp_type = bmp_type;
      bmp_sensors[bmp_count].bmp_model = 0;

      boolean success = false;
      switch (bmp_type) {
        case BMP180_CHIPID:
          success = Bmp180Calibration(bmp_count);
          break;
        case BME280_CHIPID:
          bmp_sensors[bmp_count].bmp_model++;
        case BMP280_CHIPID:
          bmp_sensors[bmp_count].bmp_model++;
          success = Bmx280Calibrate(bmp_count);
          break;
#ifdef USE_BME680
        case BME680_CHIPID:
          bmp_sensors[bmp_count].bmp_model = 3;
          success = Bme680Init(bmp_count);
          break;
#endif
      }
      if (success) {
        GetTextIndexed(bmp_sensors[bmp_count].bmp_name, sizeof(bmp_sensors[bmp_count].bmp_name), bmp_sensors[bmp_count].bmp_model, kBmpTypes);
        snprintf_P(log_data, sizeof(log_data), S_LOG_I2C_FOUND_AT, bmp_sensors[bmp_count].bmp_name, bmp_sensors[bmp_count].bmp_address);
        AddLog(LOG_LEVEL_DEBUG);
        bmp_count++;
      }
    }
  }
}

void BmpRead()
{
  for (byte bmp_idx = 0; bmp_idx < bmp_count; bmp_idx++) {
    switch (bmp_sensors[bmp_idx].bmp_type) {
      case BMP180_CHIPID:
        Bmp180Read(bmp_idx);
        break;
      case BMP280_CHIPID:
      case BME280_CHIPID:
        Bme280Read(bmp_idx);
        break;
#ifdef USE_BME680
      case BME680_CHIPID:
        Bme680Read(bmp_idx);
        break;
#endif
    }
    if (bmp_sensors[bmp_idx].bmp_temperature != 0.0) {
      bmp_sensors[bmp_idx].bmp_temperature = ConvertTemp(bmp_sensors[bmp_idx].bmp_temperature);
    }
  }

  SetGlobalValues(bmp_sensors[0].bmp_temperature, bmp_sensors[0].bmp_humidity);
}

void BmpEverySecond()
{
  if (91 == (uptime %100)) {

    BmpDetect();
  }
  else {

    BmpRead();
  }
}

void BmpShow(boolean json)
{
  for (byte bmp_idx = 0; bmp_idx < bmp_count; bmp_idx++) {
    if (bmp_sensors[bmp_idx].bmp_type) {
      float bmp_sealevel = 0.0;
      char temperature[10];
      char pressure[10];
      char sea_pressure[10];
      char humidity[10];
      char name[10];

      if (bmp_sensors[bmp_idx].bmp_pressure != 0.0) {
        bmp_sealevel = (bmp_sensors[bmp_idx].bmp_pressure / FastPrecisePow(1.0 - ((float)Settings.altitude / 44330.0), 5.255)) - 21.6;
      }

      snprintf(name, sizeof(name), bmp_sensors[bmp_idx].bmp_name);
      if (bmp_count > 1) {
        snprintf_P(name, sizeof(name), PSTR("%s-%02X"), name, bmp_sensors[bmp_idx].bmp_address);
      }

      dtostrfd(bmp_sensors[bmp_idx].bmp_temperature, Settings.flag2.temperature_resolution, temperature);
      dtostrfd(bmp_sensors[bmp_idx].bmp_pressure, Settings.flag2.pressure_resolution, pressure);
      dtostrfd(bmp_sealevel, Settings.flag2.pressure_resolution, sea_pressure);
      dtostrfd(bmp_sensors[bmp_idx].bmp_humidity, Settings.flag2.humidity_resolution, humidity);
#ifdef USE_BME680
      char gas_resistance[10];
      dtostrfd(bmp_sensors[bmp_idx].bmp_gas_resistance, 2, gas_resistance);
#endif

      if (json) {
        char json_humidity[40];
        snprintf_P(json_humidity, sizeof(json_humidity), PSTR(",\"" D_JSON_HUMIDITY "\":%s"), humidity);
        char json_sealevel[40];
        snprintf_P(json_sealevel, sizeof(json_sealevel), PSTR(",\"" D_JSON_PRESSUREATSEALEVEL "\":%s"), sea_pressure);
#ifdef USE_BME680
        char json_gas[40];
        snprintf_P(json_gas, sizeof(json_gas), PSTR(",\"" D_JSON_GAS "\":%s"), gas_resistance);

        snprintf_P(mqtt_data,
        sizeof(mqtt_data),
        PSTR("%s,\"%s\":{\"" D_JSON_TEMPERATURE "\":%s%s,\"" D_JSON_PRESSURE "\":%s%s%s}"),
          mqtt_data,
          name,
          temperature,
          (bmp_sensors[bmp_idx].bmp_model >= 2) ? json_humidity : "",
          pressure,
          (Settings.altitude != 0) ? json_sealevel : "",
          (bmp_sensors[bmp_idx].bmp_model >= 3) ? json_gas : ""
          );
#else
        snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s,\"%s\":{\"" D_JSON_TEMPERATURE "\":%s%s,\"" D_JSON_PRESSURE "\":%s%s}"),
          mqtt_data, name, temperature, (bmp_sensors[bmp_idx].bmp_model >= 2) ? json_humidity : "", pressure, (Settings.altitude != 0) ? json_sealevel : "");
#endif

#ifdef USE_DOMOTICZ
        if ((0 == tele_period) && (0 == bmp_idx)) {
          DomoticzTempHumPressureSensor(temperature, humidity, pressure);
#ifdef USE_BME680
          if (bmp_sensors[bmp_idx].bmp_model >= 3) { DomoticzSensor(DZ_AIRQUALITY, (uint32_t)bmp_sensors[bmp_idx].bmp_gas_resistance); }
#endif
        }
#endif

#ifdef USE_KNX
        if (0 == tele_period) {
          KnxSensor(KNX_TEMPERATURE, bmp_sensors[bmp_idx].bmp_temperature);
          KnxSensor(KNX_HUMIDITY, bmp_sensors[bmp_idx].bmp_humidity);
        }
#endif

#ifdef USE_WEBSERVER
      } else {
        snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_SNS_TEMP, mqtt_data, name, temperature, TempUnit());
        if (bmp_sensors[bmp_idx].bmp_model >= 2) {
          snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_SNS_HUM, mqtt_data, name, humidity);
        }
        snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_SNS_PRESSURE, mqtt_data, name, pressure);
        if (Settings.altitude != 0) {
          snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_SNS_SEAPRESSURE, mqtt_data, name, sea_pressure);
        }
#ifdef USE_BME680
        if (bmp_sensors[bmp_idx].bmp_model >= 3) {
          snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s{s}%s " D_GAS "{m}%s " D_UNIT_KILOOHM "{e}"), mqtt_data, name, gas_resistance);
        }
#endif
#endif
      }
    }
  }
}





#define XSNS_09 

boolean Xsns09(byte function)
{
  boolean result = false;

  if (i2c_flg) {
    switch (function) {
      case FUNC_INIT:
        BmpDetect();
        break;
      case FUNC_EVERY_SECOND:
        BmpEverySecond();
        break;
      case FUNC_JSON_APPEND:
        BmpShow(1);
        break;
#ifdef USE_WEBSERVER
      case FUNC_WEB_APPEND:
        BmpShow(0);
        break;
#endif
    }
  }
  return result;
}

#endif
#endif
# 1 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_10_bh1750.ino"
# 20 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_10_bh1750.ino"
#ifdef USE_I2C
#ifdef USE_BH1750






#define BH1750_ADDR1 0x23
#define BH1750_ADDR2 0x5C

#define BH1750_CONTINUOUS_HIGH_RES_MODE 0x10

uint8_t bh1750_address;
uint8_t bh1750_addresses[] = { BH1750_ADDR1, BH1750_ADDR2 };
uint8_t bh1750_type = 0;
uint8_t bh1750_valid = 0;
uint16_t bh1750_illuminance = 0;
char bh1750_types[] = "BH1750";

bool Bh1750Read()
{
  if (bh1750_valid) { bh1750_valid--; }

  if (2 != Wire.requestFrom(bh1750_address, (uint8_t)2)) { return false; }
  byte msb = Wire.read();
  byte lsb = Wire.read();
  bh1750_illuminance = ((msb << 8) | lsb) / 1.2;
  bh1750_valid = SENSOR_MAX_MISS;
  return true;
}



void Bh1750Detect()
{
  if (bh1750_type) {
    return;
  }

  for (byte i = 0; i < sizeof(bh1750_addresses); i++) {
    bh1750_address = bh1750_addresses[i];
    Wire.beginTransmission(bh1750_address);
    Wire.write(BH1750_CONTINUOUS_HIGH_RES_MODE);
    if (!Wire.endTransmission()) {
      bh1750_type = 1;
      snprintf_P(log_data, sizeof(log_data), S_LOG_I2C_FOUND_AT, bh1750_types, bh1750_address);
      AddLog(LOG_LEVEL_DEBUG);
      break;
    }
  }
}

void Bh1750EverySecond()
{
  if (90 == (uptime %100)) {

    Bh1750Detect();
  }
  else {

    if (bh1750_type) {
      if (!Bh1750Read()) {
        AddLogMissed(bh1750_types, bh1750_valid);

      }
    }
  }
}

#ifdef USE_WEBSERVER
const char HTTP_SNS_ILLUMINANCE[] PROGMEM =
  "%s{s}%s " D_ILLUMINANCE "{m}%d " D_UNIT_LUX "{e}";
#endif

void Bh1750Show(boolean json)
{
  if (bh1750_valid) {
    if (json) {
      snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s,\"%s\":{\"" D_JSON_ILLUMINANCE "\":%d}"), mqtt_data, bh1750_types, bh1750_illuminance);
#ifdef USE_DOMOTICZ
      if (0 == tele_period) {
        DomoticzSensor(DZ_ILLUMINANCE, bh1750_illuminance);
      }
#endif
#ifdef USE_WEBSERVER
    } else {
      snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_SNS_ILLUMINANCE, mqtt_data, bh1750_types, bh1750_illuminance);
#endif
    }
  }
}





#define XSNS_10 

boolean Xsns10(byte function)
{
  boolean result = false;

  if (i2c_flg) {
    switch (function) {
      case FUNC_INIT:
        Bh1750Detect();
        break;
      case FUNC_EVERY_SECOND:
        Bh1750EverySecond();
        break;
      case FUNC_JSON_APPEND:
        Bh1750Show(1);
        break;
#ifdef USE_WEBSERVER
      case FUNC_WEB_APPEND:
        Bh1750Show(0);
        break;
#endif
    }
  }
  return result;
}

#endif
#endif
# 1 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_11_veml6070.ino"
# 89 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_11_veml6070.ino"
#ifdef USE_I2C
#ifdef USE_VEML6070






#define VEML6070_ADDR_H 0x39
#define VEML6070_ADDR_L 0x38
#define VEML6070_INTEGRATION_TIME 3
#define VEML6070_ENABLE 1
#define VEML6070_DISABLE 0
#define VEML6070_RSET_DEFAULT 270000
#define VEML6070_UV_MAX_INDEX 15
#define VEML6070_UV_MAX_DEFAULT 11
#define VEML6070_POWER_COEFFCIENT 0.025
#define VEML6070_TABLE_COEFFCIENT 32.86270591





const char kVemlTypes[] PROGMEM = "VEML6070";
double uv_risk_map[VEML6070_UV_MAX_INDEX] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
double uvrisk = 0;
double uvpower = 0;
uint16_t uvlevel = 0;
uint8_t veml6070_addr_low = VEML6070_ADDR_L;
uint8_t veml6070_addr_high = VEML6070_ADDR_H;
uint8_t itime = VEML6070_INTEGRATION_TIME;
uint8_t veml6070_type = 0;
uint8_t veml6070_valid = 0;
char veml6070_name[9];
char str_uvrisk_text[10];



void Veml6070Detect(void)
{
  if (veml6070_type) {
    return;
  }

  Wire.beginTransmission(VEML6070_ADDR_L);
  Wire.write((itime << 2) | 0x02);
  uint8_t status = Wire.endTransmission();

  if (!status) {
    veml6070_type = 1;
    uint8_t veml_model = 0;
    GetTextIndexed(veml6070_name, sizeof(veml6070_name), veml_model, kVemlTypes);
    snprintf_P(log_data, sizeof(log_data), S_LOG_I2C_FOUND_AT, "VEML6070", VEML6070_ADDR_L);
    AddLog(LOG_LEVEL_DEBUG);
  }
}



void Veml6070UvTableInit(void)
{

  for (uint8_t i = 0; i < VEML6070_UV_MAX_INDEX; i++) {
#ifdef USE_VEML6070_RSET
    if ( (USE_VEML6070_RSET >= 220000) && (USE_VEML6070_RSET <= 1000000) ) {
      uv_risk_map[i] = ( (USE_VEML6070_RSET / VEML6070_TABLE_COEFFCIENT) / VEML6070_UV_MAX_DEFAULT ) * (i+1);
    } else {
      uv_risk_map[i] = ( (VEML6070_RSET_DEFAULT / VEML6070_TABLE_COEFFCIENT) / VEML6070_UV_MAX_DEFAULT ) * (i+1);
      snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_DEBUG "VEML6070 resistor error %d"), USE_VEML6070_RSET);
      AddLog(LOG_LEVEL_DEBUG);
    }
#else
    uv_risk_map[i] = ( (VEML6070_RSET_DEFAULT / VEML6070_TABLE_COEFFCIENT) / VEML6070_UV_MAX_DEFAULT ) * (i+1);
    snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_DEBUG "VEML6070 resistor default used %d"), VEML6070_RSET_DEFAULT);
    AddLog(LOG_LEVEL_DEBUG);
#endif
  }
}



void Veml6070EverySecond(void)
{

  if (11 == (uptime %100)) {
    Veml6070ModeCmd(1);
    Veml6070Detect();
    Veml6070ModeCmd(0);
  } else {
    Veml6070ModeCmd(1);
    uvlevel = Veml6070ReadUv();
    uvrisk = Veml6070UvRiskLevel(uvlevel);
    uvpower = Veml6070UvPower(uvrisk);
    Veml6070ModeCmd(0);
  }
}



void Veml6070ModeCmd(boolean mode_cmd)
{


  Wire.beginTransmission(VEML6070_ADDR_L);
  Wire.write((mode_cmd << 0) | 0x02 | (itime << 2));
  uint8_t status = Wire.endTransmission();

  if (!status) {
    snprintf_P(log_data, sizeof(log_data), S_LOG_I2C_FOUND_AT, "VEML6070 mode_cmd", VEML6070_ADDR_L);
    AddLog(LOG_LEVEL_DEBUG);
  }
}



uint16_t Veml6070ReadUv(void)
{
  uint16_t uv_raw = 0;

  if (Wire.requestFrom(VEML6070_ADDR_H, 1) != 1) {
    return -1;
  }
  uv_raw = Wire.read();
  uv_raw <<= 8;

  if (Wire.requestFrom(VEML6070_ADDR_L, 1) != 1) {
    return -1;
  }
  uv_raw |= Wire.read();

  return uv_raw;
}



double Veml6070UvRiskLevel(uint16_t uv_level)
{
  double risk = 0;
  if (uv_level < uv_risk_map[VEML6070_UV_MAX_INDEX-1]) {
    risk = (double)uv_level / uv_risk_map[0];

    if ( (risk >= 0) && (risk <= 2.9) ) { snprintf_P(str_uvrisk_text, sizeof(str_uvrisk_text), D_UV_INDEX_1); }
    else if ( (risk >= 3.0) && (risk <= 5.9) ) { snprintf_P(str_uvrisk_text, sizeof(str_uvrisk_text), D_UV_INDEX_2); }
    else if ( (risk >= 6.0) && (risk <= 7.9) ) { snprintf_P(str_uvrisk_text, sizeof(str_uvrisk_text), D_UV_INDEX_3); }
    else if ( (risk >= 8.0) && (risk <= 10.9) ) { snprintf_P(str_uvrisk_text, sizeof(str_uvrisk_text), D_UV_INDEX_4); }
    else if ( (risk >= 11.0) && (risk <= 12.9) ) { snprintf_P(str_uvrisk_text, sizeof(str_uvrisk_text), D_UV_INDEX_5); }
    else if ( (risk >= 13.0) && (risk <= 25.0) ) { snprintf_P(str_uvrisk_text, sizeof(str_uvrisk_text), D_UV_INDEX_6); }
    else { snprintf_P(str_uvrisk_text, sizeof(str_uvrisk_text), D_UV_INDEX_7); }
    return risk;
  } else {

    snprintf_P(str_uvrisk_text, sizeof(str_uvrisk_text), D_UV_INDEX_7);
    return ( risk = 99 );
    snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_DEBUG "VEML6070 out of range %d"), risk);
    AddLog(LOG_LEVEL_DEBUG);
  }
}



double Veml6070UvPower(double uvrisk)
{

  double power = 0;
  return ( power = VEML6070_POWER_COEFFCIENT * uvrisk );
}




#ifdef USE_WEBSERVER

#ifdef USE_VEML6070_SHOW_RAW
  const char HTTP_SNS_UV_LEVEL[] PROGMEM = "%s{s}VEML6070 " D_UV_LEVEL "{m}%s " D_UNIT_INCREMENTS "{e}";
#endif

  const char HTTP_SNS_UV_INDEX[] PROGMEM = "%s{s}VEML6070 " D_UV_INDEX " {m}%s %s{e}";
  const char HTTP_SNS_UV_POWER[] PROGMEM = "%s{s}VEML6070 " D_UV_POWER "{m}%s " D_UNIT_WATT_METER_QUADRAT "{e}";
#endif



void Veml6070Show(boolean json)
{
  if (veml6070_type) {
    char str_uvlevel[6];
    char str_uvrisk[6];
    char str_uvpower[6];

    dtostrfd((double)uvlevel, 0, str_uvlevel);
    dtostrfd(uvrisk, 2, str_uvrisk);
    dtostrfd(uvpower, 3, str_uvpower);
    if (json) {
#ifdef USE_VEML6070_SHOW_RAW
      snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s,\"%s\":{\"" D_JSON_UV_LEVEL "\":%s,\"" D_JSON_UV_INDEX "\":%s,\"" D_JSON_UV_INDEX_TEXT "\":\"%s\",\"" D_JSON_UV_POWER "\":%s}"),
        mqtt_data, veml6070_name, str_uvlevel, str_uvrisk, str_uvrisk_text, str_uvpower);
#else
      snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s,\"%s\":{\"" D_JSON_UV_INDEX "\":%s,\"" D_JSON_UV_INDEX_TEXT "\":\"%s\",\"" D_JSON_UV_POWER "\":%s}"),
        mqtt_data, veml6070_name, str_uvrisk, str_uvrisk_text, str_uvpower);
#endif
#ifdef USE_DOMOTICZ
    if (0 == tele_period) { DomoticzSensor(DZ_ILLUMINANCE, uvlevel); }
#endif
#ifdef USE_WEBSERVER
    } else {
#ifdef USE_VEML6070_SHOW_RAW
      snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_SNS_UV_LEVEL, mqtt_data, str_uvlevel);
#endif
      snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_SNS_UV_INDEX, mqtt_data, str_uvrisk, str_uvrisk_text);
      snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_SNS_UV_POWER, mqtt_data, str_uvpower);
#endif
    }
  }
}





#define XSNS_11 

boolean Xsns11(byte function)
{
  boolean result = false;

  if (i2c_flg) {
    switch (function) {
      case FUNC_INIT:
        Veml6070Detect();
 Veml6070UvTableInit();
        break;
      case FUNC_EVERY_SECOND:
        Veml6070EverySecond();
        break;
      case FUNC_JSON_APPEND:
        Veml6070Show(1);
        break;
#ifdef USE_WEBSERVER
      case FUNC_WEB_APPEND:
        Veml6070Show(0);
        break;
#endif
    }
  }
  return result;
}

#endif
#endif
# 1 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_12_ads1115.ino"
# 20 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_12_ads1115.ino"
#ifdef USE_I2C
#ifdef USE_ADS1115
# 43 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_12_ads1115.ino"
#define ADS1115_ADDRESS_ADDR_GND 0x48
#define ADS1115_ADDRESS_ADDR_VDD 0x49
#define ADS1115_ADDRESS_ADDR_SDA 0x4A
#define ADS1115_ADDRESS_ADDR_SCL 0x4B

#define ADS1115_CONVERSIONDELAY (8)




#define ADS1115_REG_POINTER_MASK (0x03)
#define ADS1115_REG_POINTER_CONVERT (0x00)
#define ADS1115_REG_POINTER_CONFIG (0x01)
#define ADS1115_REG_POINTER_LOWTHRESH (0x02)
#define ADS1115_REG_POINTER_HITHRESH (0x03)




#define ADS1115_REG_CONFIG_OS_MASK (0x8000)
#define ADS1115_REG_CONFIG_OS_SINGLE (0x8000)
#define ADS1115_REG_CONFIG_OS_BUSY (0x0000)
#define ADS1115_REG_CONFIG_OS_NOTBUSY (0x8000)

#define ADS1115_REG_CONFIG_MUX_MASK (0x7000)
#define ADS1115_REG_CONFIG_MUX_DIFF_0_1 (0x0000)
#define ADS1115_REG_CONFIG_MUX_DIFF_0_3 (0x1000)
#define ADS1115_REG_CONFIG_MUX_DIFF_1_3 (0x2000)
#define ADS1115_REG_CONFIG_MUX_DIFF_2_3 (0x3000)
#define ADS1115_REG_CONFIG_MUX_SINGLE_0 (0x4000)
#define ADS1115_REG_CONFIG_MUX_SINGLE_1 (0x5000)
#define ADS1115_REG_CONFIG_MUX_SINGLE_2 (0x6000)
#define ADS1115_REG_CONFIG_MUX_SINGLE_3 (0x7000)

#define ADS1115_REG_CONFIG_PGA_MASK (0x0E00)
#define ADS1115_REG_CONFIG_PGA_6_144V (0x0000)
#define ADS1115_REG_CONFIG_PGA_4_096V (0x0200)
#define ADS1115_REG_CONFIG_PGA_2_048V (0x0400)
#define ADS1115_REG_CONFIG_PGA_1_024V (0x0600)
#define ADS1115_REG_CONFIG_PGA_0_512V (0x0800)
#define ADS1115_REG_CONFIG_PGA_0_256V (0x0A00)

#define ADS1115_REG_CONFIG_MODE_MASK (0x0100)
#define ADS1115_REG_CONFIG_MODE_CONTIN (0x0000)
#define ADS1115_REG_CONFIG_MODE_SINGLE (0x0100)

#define ADS1115_REG_CONFIG_DR_MASK (0x00E0)
#define ADS1115_REG_CONFIG_DR_128SPS (0x0000)
#define ADS1115_REG_CONFIG_DR_250SPS (0x0020)
#define ADS1115_REG_CONFIG_DR_490SPS (0x0040)
#define ADS1115_REG_CONFIG_DR_920SPS (0x0060)
#define ADS1115_REG_CONFIG_DR_1600SPS (0x0080)
#define ADS1115_REG_CONFIG_DR_2400SPS (0x00A0)
#define ADS1115_REG_CONFIG_DR_3300SPS (0x00C0)
#define ADS1115_REG_CONFIG_DR_6000SPS (0x00E0)

#define ADS1115_REG_CONFIG_CMODE_MASK (0x0010)
#define ADS1115_REG_CONFIG_CMODE_TRAD (0x0000)
#define ADS1115_REG_CONFIG_CMODE_WINDOW (0x0010)

#define ADS1115_REG_CONFIG_CPOL_MASK (0x0008)
#define ADS1115_REG_CONFIG_CPOL_ACTVLOW (0x0000)
#define ADS1115_REG_CONFIG_CPOL_ACTVHI (0x0008)

#define ADS1115_REG_CONFIG_CLAT_MASK (0x0004)
#define ADS1115_REG_CONFIG_CLAT_NONLAT (0x0000)
#define ADS1115_REG_CONFIG_CLAT_LATCH (0x0004)

#define ADS1115_REG_CONFIG_CQUE_MASK (0x0003)
#define ADS1115_REG_CONFIG_CQUE_1CONV (0x0000)
#define ADS1115_REG_CONFIG_CQUE_2CONV (0x0001)
#define ADS1115_REG_CONFIG_CQUE_4CONV (0x0002)
#define ADS1115_REG_CONFIG_CQUE_NONE (0x0003)

uint8_t ads1115_type = 0;
uint8_t ads1115_address;
uint8_t ads1115_addresses[] = { ADS1115_ADDRESS_ADDR_GND, ADS1115_ADDRESS_ADDR_VDD, ADS1115_ADDRESS_ADDR_SDA, ADS1115_ADDRESS_ADDR_SCL };



void Ads1115StartComparator(uint8_t channel, uint16_t mode)
{

  uint16_t config = mode |
                    ADS1115_REG_CONFIG_CQUE_NONE |
                    ADS1115_REG_CONFIG_CLAT_NONLAT |
                    ADS1115_REG_CONFIG_PGA_6_144V |
                    ADS1115_REG_CONFIG_CPOL_ACTVLOW |
                    ADS1115_REG_CONFIG_CMODE_TRAD |
                    ADS1115_REG_CONFIG_DR_6000SPS;


  config |= (ADS1115_REG_CONFIG_MUX_SINGLE_0 + (0x1000 * channel));


  I2cWrite16(ads1115_address, ADS1115_REG_POINTER_CONFIG, config);
}

int16_t Ads1115GetConversion(uint8_t channel)
{
  Ads1115StartComparator(channel, ADS1115_REG_CONFIG_MODE_SINGLE);

  delay(ADS1115_CONVERSIONDELAY);

  I2cRead16(ads1115_address, ADS1115_REG_POINTER_CONVERT);

  Ads1115StartComparator(channel, ADS1115_REG_CONFIG_MODE_CONTIN);
  delay(ADS1115_CONVERSIONDELAY);

  uint16_t res = I2cRead16(ads1115_address, ADS1115_REG_POINTER_CONVERT);
  return (int16_t)res;
}



void Ads1115Detect()
{
  uint16_t buffer;

  if (ads1115_type) {
    return;
  }

  for (byte i = 0; i < sizeof(ads1115_addresses); i++) {
    ads1115_address = ads1115_addresses[i];
    if (I2cValidRead16(&buffer, ads1115_address, ADS1115_REG_POINTER_CONVERT)) {
      Ads1115StartComparator(i, ADS1115_REG_CONFIG_MODE_CONTIN);
      ads1115_type = 1;
      snprintf_P(log_data, sizeof(log_data), S_LOG_I2C_FOUND_AT, "ADS1115", ads1115_address);
      AddLog(LOG_LEVEL_DEBUG);
      break;
    }
  }
}

void Ads1115Show(boolean json)
{
  if (ads1115_type) {
    char stemp[10];

    byte dsxflg = 0;
    for (byte i = 0; i < 4; i++) {
      int16_t adc_value = Ads1115GetConversion(i);

      if (json) {
        if (!dsxflg ) {
          snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s,\"ADS1115\":{"), mqtt_data);
          stemp[0] = '\0';
        }
        dsxflg++;
        snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s%s\"A%d\":%d"), mqtt_data, stemp, i, adc_value);
        strlcpy(stemp, ",", sizeof(stemp));
#ifdef USE_WEBSERVER
      } else {
        snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_SNS_ANALOG, mqtt_data, "ADS1115", i, adc_value);
#endif
      }
    }
    if (json) {
      if (dsxflg) {
        snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s}"), mqtt_data);
      }
    }
  }
}





#define XSNS_12 

boolean Xsns12(byte function)
{
  boolean result = false;

  if (i2c_flg) {
    switch (function) {
      case FUNC_PREP_BEFORE_TELEPERIOD:
        Ads1115Detect();
        break;
      case FUNC_JSON_APPEND:
        Ads1115Show(1);
        break;
#ifdef USE_WEBSERVER
      case FUNC_WEB_APPEND:
        Ads1115Show(0);
        break;
#endif
    }
  }
  return result;
}

#endif
#endif
# 1 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_12_ads1115_i2cdev.ino"
# 20 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_12_ads1115_i2cdev.ino"
#ifdef USE_I2C
#ifdef USE_ADS1115_I2CDEV
# 43 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_12_ads1115_i2cdev.ino"
#include <ADS1115.h>

ADS1115 adc0;

uint8_t ads1115_type = 0;
uint8_t ads1115_address;
uint8_t ads1115_addresses[] = {
  ADS1115_ADDRESS_ADDR_GND,
  ADS1115_ADDRESS_ADDR_VDD,
  ADS1115_ADDRESS_ADDR_SDA,
  ADS1115_ADDRESS_ADDR_SCL
};

int16_t Ads1115GetConversion(byte channel)
{
  switch (channel) {
    case 0:
      adc0.getConversionP0GND();
      break;
    case 1:
      adc0.getConversionP1GND();
      break;
    case 2:
      adc0.getConversionP2GND();
      break;
    case 3:
      adc0.getConversionP3GND();
      break;
  }
}



void Ads1115Detect()
{
  if (ads1115_type) {
    return;
  }

  for (byte i = 0; i < sizeof(ads1115_addresses); i++) {
    ads1115_address = ads1115_addresses[i];
    ADS1115 adc0(ads1115_address);
    if (adc0.testConnection()) {
      adc0.initialize();
      adc0.setGain(ADS1115_PGA_6P144);
      adc0.setRate(ADS1115_RATE_860);
      adc0.setMode(ADS1115_MODE_CONTINUOUS);
      ads1115_type = 1;
      snprintf_P(log_data, sizeof(log_data), S_LOG_I2C_FOUND_AT, "ADS1115", ads1115_address);
      AddLog(LOG_LEVEL_DEBUG);
      break;
    }
  }
}

void Ads1115Show(boolean json)
{
  if (ads1115_type) {
    char stemp[10];

    byte dsxflg = 0;
    for (byte i = 0; i < 4; i++) {
      int16_t adc_value = Ads1115GetConversion(i);

      if (json) {
        if (!dsxflg ) {
          snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s,\"ADS1115\":{"), mqtt_data);
          stemp[0] = '\0';
        }
        dsxflg++;
        snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s%s\"A%d\":%d"), mqtt_data, stemp, i, adc_value);
        strlcpy(stemp, ",", sizeof(stemp));
#ifdef USE_WEBSERVER
      } else {
        snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_SNS_ANALOG, mqtt_data, "ADS1115", i, adc_value);
#endif
      }
    }
    if (json) {
      if (dsxflg) {
        snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s}"), mqtt_data);
      }
    }
  }
}





#define XSNS_12 

boolean Xsns12(byte function)
{
  boolean result = false;

  if (i2c_flg) {
    switch (function) {
      case FUNC_PREP_BEFORE_TELEPERIOD:
        Ads1115Detect();
        break;
      case FUNC_JSON_APPEND:
        Ads1115Show(1);
        break;
#ifdef USE_WEBSERVER
      case FUNC_WEB_APPEND:
        Ads1115Show(0);
        break;
#endif
    }
  }
  return result;
}

#endif
#endif
# 1 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_13_ina219.ino"
# 20 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_13_ina219.ino"
#ifdef USE_I2C
#ifdef USE_INA219

#define XSNS_13 13
# 33 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_13_ina219.ino"
#define INA219_ADDRESS1 (0x40)
#define INA219_ADDRESS2 (0x41)
#define INA219_ADDRESS3 (0x44)
#define INA219_ADDRESS4 (0x45)

#define INA219_READ (0x01)
#define INA219_REG_CONFIG (0x00)

#define INA219_CONFIG_RESET (0x8000)

#define INA219_CONFIG_BVOLTAGERANGE_MASK (0x2000)
#define INA219_CONFIG_BVOLTAGERANGE_16V (0x0000)
#define INA219_CONFIG_BVOLTAGERANGE_32V (0x2000)

#define INA219_CONFIG_GAIN_MASK (0x1800)
#define INA219_CONFIG_GAIN_1_40MV (0x0000)
#define INA219_CONFIG_GAIN_2_80MV (0x0800)
#define INA219_CONFIG_GAIN_4_160MV (0x1000)
#define INA219_CONFIG_GAIN_8_320MV (0x1800)

#define INA219_CONFIG_BADCRES_MASK (0x0780)
#define INA219_CONFIG_BADCRES_9BIT (0x0080)
#define INA219_CONFIG_BADCRES_10BIT (0x0100)
#define INA219_CONFIG_BADCRES_11BIT (0x0200)
#define INA219_CONFIG_BADCRES_12BIT (0x0400)

#define INA219_CONFIG_SADCRES_MASK (0x0078)
#define INA219_CONFIG_SADCRES_9BIT_1S_84US (0x0000)
#define INA219_CONFIG_SADCRES_10BIT_1S_148US (0x0008)
#define INA219_CONFIG_SADCRES_11BIT_1S_276US (0x0010)
#define INA219_CONFIG_SADCRES_12BIT_1S_532US (0x0018)
#define INA219_CONFIG_SADCRES_12BIT_2S_1060US (0x0048)
#define INA219_CONFIG_SADCRES_12BIT_4S_2130US (0x0050)
#define INA219_CONFIG_SADCRES_12BIT_8S_4260US (0x0058)
#define INA219_CONFIG_SADCRES_12BIT_16S_8510US (0x0060)
#define INA219_CONFIG_SADCRES_12BIT_32S_17MS (0x0068)
#define INA219_CONFIG_SADCRES_12BIT_64S_34MS (0x0070)
#define INA219_CONFIG_SADCRES_12BIT_128S_69MS (0x0078)

#define INA219_CONFIG_MODE_MASK (0x0007)
#define INA219_CONFIG_MODE_POWERDOWN (0x0000)
#define INA219_CONFIG_MODE_SVOLT_TRIGGERED (0x0001)
#define INA219_CONFIG_MODE_BVOLT_TRIGGERED (0x0002)
#define INA219_CONFIG_MODE_SANDBVOLT_TRIGGERED (0x0003)
#define INA219_CONFIG_MODE_ADCOFF (0x0004)
#define INA219_CONFIG_MODE_SVOLT_CONTINUOUS (0x0005)
#define INA219_CONFIG_MODE_BVOLT_CONTINUOUS (0x0006)
#define INA219_CONFIG_MODE_SANDBVOLT_CONTINUOUS (0x0007)

#define INA219_REG_SHUNTVOLTAGE (0x01)
#define INA219_REG_BUSVOLTAGE (0x02)
#define INA219_REG_POWER (0x03)
#define INA219_REG_CURRENT (0x04)
#define INA219_REG_CALIBRATION (0x05)

uint8_t ina219_type = 0;
uint8_t ina219_address;
uint8_t ina219_addresses[] = { INA219_ADDRESS1, INA219_ADDRESS2, INA219_ADDRESS3, INA219_ADDRESS4 };

uint32_t ina219_cal_value = 0;

uint32_t ina219_current_divider_ma = 0;

uint8_t ina219_valid = 0;
float ina219_voltage = 0;
float ina219_current = 0;
char ina219_types[] = "INA219";

bool Ina219SetCalibration(uint8_t mode)
{
  uint16_t config = 0;

  switch (mode &3) {
    case 0:
    case 3:
      ina219_cal_value = 4096;
      ina219_current_divider_ma = 10;
      config = INA219_CONFIG_BVOLTAGERANGE_32V | INA219_CONFIG_GAIN_8_320MV | INA219_CONFIG_BADCRES_12BIT | INA219_CONFIG_SADCRES_12BIT_1S_532US | INA219_CONFIG_MODE_SANDBVOLT_CONTINUOUS;
      break;
    case 1:
      ina219_cal_value = 10240;
      ina219_current_divider_ma = 25;
      config |= INA219_CONFIG_BVOLTAGERANGE_32V | INA219_CONFIG_GAIN_8_320MV | INA219_CONFIG_BADCRES_12BIT | INA219_CONFIG_SADCRES_12BIT_1S_532US | INA219_CONFIG_MODE_SANDBVOLT_CONTINUOUS;
      break;
    case 2:
      ina219_cal_value = 8192;
      ina219_current_divider_ma = 20;
      config |= INA219_CONFIG_BVOLTAGERANGE_16V | INA219_CONFIG_GAIN_1_40MV | INA219_CONFIG_BADCRES_12BIT | INA219_CONFIG_SADCRES_12BIT_1S_532US | INA219_CONFIG_MODE_SANDBVOLT_CONTINUOUS;
      break;
  }

  bool success = I2cWrite16(ina219_address, INA219_REG_CALIBRATION, ina219_cal_value);
  if (success) {

    I2cWrite16(ina219_address, INA219_REG_CONFIG, config);
  }
  return success;
}

float Ina219GetShuntVoltage_mV()
{

  int16_t value = I2cReadS16(ina219_address, INA219_REG_SHUNTVOLTAGE);

  return value * 0.01;
}

float Ina219GetBusVoltage_V()
{


  int16_t value = (int16_t)(((uint16_t)I2cReadS16(ina219_address, INA219_REG_BUSVOLTAGE) >> 3) * 4);

  return value * 0.001;
}

float Ina219GetCurrent_mA()
{



  I2cWrite16(ina219_address, INA219_REG_CALIBRATION, ina219_cal_value);


  float value = I2cReadS16(ina219_address, INA219_REG_CURRENT);
  value /= ina219_current_divider_ma;

  return value;
}

bool Ina219Read()
{
  ina219_voltage = Ina219GetBusVoltage_V() + (Ina219GetShuntVoltage_mV() / 1000);
  ina219_current = Ina219GetCurrent_mA() / 1000;
  ina219_valid = SENSOR_MAX_MISS;
  return true;
}
# 179 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_13_ina219.ino"
bool Ina219CommandSensor()
{
  boolean serviced = true;

  if ((XdrvMailbox.payload >= 0) && (XdrvMailbox.payload <= 2)) {
    Settings.ina219_mode = XdrvMailbox.payload;
    restart_flag = 2;
  }
  snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_SENSOR_INDEX_NVALUE, XSNS_13, Settings.ina219_mode);

  return serviced;
}



void Ina219Detect()
{
  if (ina219_type) { return; }

  for (byte i = 0; i < sizeof(ina219_addresses); i++) {
    ina219_address = ina219_addresses[i];
    if (Ina219SetCalibration(Settings.ina219_mode)) {
      ina219_type = 1;
      snprintf_P(log_data, sizeof(log_data), S_LOG_I2C_FOUND_AT, ina219_types, ina219_address);
      AddLog(LOG_LEVEL_DEBUG);
      break;
    }
  }
}

void Ina219EverySecond()
{
  if (87 == (uptime %100)) {

    Ina219Detect();
  }
  else {

    if (ina219_type) {
      if (!Ina219Read()) {
        AddLogMissed(ina219_types, ina219_valid);

      }
    }
  }
}

#ifdef USE_WEBSERVER
const char HTTP_SNS_INA219_DATA[] PROGMEM = "%s"
  "{s}INA219 " D_VOLTAGE "{m}%s " D_UNIT_VOLT "{e}"
  "{s}INA219 " D_CURRENT "{m}%s " D_UNIT_AMPERE "{e}"
  "{s}INA219 " D_POWERUSAGE "{m}%s " D_UNIT_WATT "{e}";
#endif

void Ina219Show(boolean json)
{
  if (ina219_valid) {
    char voltage[10];
    char current[10];
    char power[10];

    float fpower = ina219_voltage * ina219_current;
    dtostrfd(ina219_voltage, Settings.flag2.voltage_resolution, voltage);
    dtostrfd(fpower, Settings.flag2.wattage_resolution, power);
    dtostrfd(ina219_current, Settings.flag2.current_resolution, current);

    if (json) {
      snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s,\"%s\":{\"" D_JSON_VOLTAGE "\":%s,\"" D_JSON_CURRENT "\":%s,\"" D_JSON_POWERUSAGE "\":%s}"),
        mqtt_data, ina219_types, voltage, current, power);
#ifdef USE_DOMOTICZ
      if (0 == tele_period) {
        DomoticzSensor(DZ_VOLTAGE, voltage);
        DomoticzSensor(DZ_CURRENT, current);
      }
#endif
#ifdef USE_WEBSERVER
    } else {
      snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_SNS_INA219_DATA, mqtt_data, voltage, current, power);
#endif
    }
  }
}





boolean Xsns13(byte function)
{
  boolean result = false;

  if (i2c_flg) {
    switch (function) {
      case FUNC_COMMAND:
        if ((XSNS_13 == XdrvMailbox.index) && (ina219_type)) {
          result = Ina219CommandSensor();
        }
        break;
      case FUNC_INIT:
        Ina219Detect();
        break;
      case FUNC_EVERY_SECOND:
        Ina219EverySecond();
        break;
      case FUNC_JSON_APPEND:
        Ina219Show(1);
        break;
#ifdef USE_WEBSERVER
      case FUNC_WEB_APPEND:
        Ina219Show(0);
        break;
#endif
    }
  }
  return result;
}

#endif
#endif
# 1 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_14_sht3x.ino"
# 20 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_14_sht3x.ino"
#ifdef USE_I2C
#ifdef USE_SHT3X






#define SHT3X_ADDR_GND 0x44
#define SHT3X_ADDR_VDD 0x45
#define SHTC3_ADDR 0x70

#define SHT3X_MAX_SENSORS 3

const char kShtTypes[] PROGMEM = "SHT3X|SHT3X|SHTC3";
uint8_t sht3x_addresses[] = { SHT3X_ADDR_GND, SHT3X_ADDR_VDD, SHTC3_ADDR };

uint8_t sht3x_count = 0;
struct SHT3XSTRUCT {
  uint8_t address;
  char types[6];
} sht3x_sensors[SHT3X_MAX_SENSORS];

bool Sht3xRead(float &t, float &h, uint8_t sht3x_address)
{
  unsigned int data[6];

  t = NAN;
  h = NAN;

  Wire.beginTransmission(sht3x_address);
  if (SHTC3_ADDR == sht3x_address) {
    Wire.write(0x35);
    Wire.write(0x17);
    Wire.endTransmission();
    Wire.beginTransmission(sht3x_address);
    Wire.write(0x78);
    Wire.write(0x66);
  } else {
    Wire.write(0x2C);
    Wire.write(0x06);
  }
  if (Wire.endTransmission() != 0) {
    return false;
  }
  delay(30);
  Wire.requestFrom(sht3x_address, (uint8_t)6);
  for (int i = 0; i < 6; i++) {
    data[i] = Wire.read();
  };
  t = ConvertTemp((float)((((data[0] << 8) | data[1]) * 175) / 65535.0) - 45);
  h = (float)((((data[3] << 8) | data[4]) * 100) / 65535.0);
  return (!isnan(t) && !isnan(h));
}



void Sht3xDetect()
{
  if (sht3x_count) return;

  float t;
  float h;
  for (byte i = 0; i < SHT3X_MAX_SENSORS; i++) {
    if (Sht3xRead(t, h, sht3x_addresses[i])) {
      sht3x_sensors[sht3x_count].address = sht3x_addresses[i];
      GetTextIndexed(sht3x_sensors[sht3x_count].types, sizeof(sht3x_sensors[sht3x_count].types), i, kShtTypes);
      snprintf_P(log_data, sizeof(log_data), S_LOG_I2C_FOUND_AT, sht3x_sensors[sht3x_count].types, sht3x_sensors[sht3x_count].address);
      AddLog(LOG_LEVEL_DEBUG);
      sht3x_count++;
    }
  }
}

void Sht3xShow(boolean json)
{
  if (sht3x_count) {
    float t;
    float h;
    char temperature[10];
    char humidity[10];
    char types[11];
    for (byte i = 0; i < sht3x_count; i++) {
      if (Sht3xRead(t, h, sht3x_sensors[i].address)) {

        if (0 == i) { SetGlobalValues(t, h); }

        dtostrfd(t, Settings.flag2.temperature_resolution, temperature);
        dtostrfd(h, Settings.flag2.humidity_resolution, humidity);
        snprintf_P(types, sizeof(types), PSTR("%s-0x%02X"), sht3x_sensors[i].types, sht3x_sensors[i].address);

        if (json) {
          snprintf_P(mqtt_data, sizeof(mqtt_data), JSON_SNS_TEMPHUM, mqtt_data, types, temperature, humidity);
#ifdef USE_DOMOTICZ
          if ((0 == tele_period) && (0 == i)) {
            DomoticzTempHumSensor(temperature, humidity);
          }
#endif

#ifdef USE_KNX
        if (0 == tele_period) {
          KnxSensor(KNX_TEMPERATURE, t);
          KnxSensor(KNX_HUMIDITY, h);
        }
#endif

#ifdef USE_WEBSERVER
        } else {
          snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_SNS_TEMP, mqtt_data, types, temperature, TempUnit());
          snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_SNS_HUM, mqtt_data, types, humidity);
#endif
        }
      }
    }
  }
}





#define XSNS_14 

boolean Xsns14(byte function)
{
  boolean result = false;

  if (i2c_flg) {
    switch (function) {
      case FUNC_INIT:
        Sht3xDetect();
        break;
      case FUNC_JSON_APPEND:
        Sht3xShow(1);
        break;
#ifdef USE_WEBSERVER
      case FUNC_WEB_APPEND:
        Sht3xShow(0);
        break;
#endif
    }
  }
  return result;
}

#endif
#endif
# 1 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_15_mhz19.ino"
# 20 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_15_mhz19.ino"
#ifdef USE_MHZ19

#define XSNS_15 15
# 36 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_15_mhz19.ino"
enum MhzFilterOptions {MHZ19_FILTER_OFF, MHZ19_FILTER_OFF_ALLSAMPLES, MHZ19_FILTER_FAST, MHZ19_FILTER_MEDIUM, MHZ19_FILTER_SLOW};

#define MHZ19_FILTER_OPTION MHZ19_FILTER_FAST
# 56 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_15_mhz19.ino"
#define MHZ19_ABC_ENABLE 1



#include <TasmotaSerial.h>

#ifndef CO2_LOW
#define CO2_LOW 800
#endif
#ifndef CO2_HIGH
#define CO2_HIGH 1200
#endif

#define MHZ19_READ_TIMEOUT 400
#define MHZ19_RETRY_COUNT 8

TasmotaSerial *MhzSerial;

const char kMhzTypes[] PROGMEM = "MHZ19|MHZ19B";

enum MhzCommands { MHZ_CMND_READPPM, MHZ_CMND_ABCENABLE, MHZ_CMND_ABCDISABLE, MHZ_CMND_ZEROPOINT, MHZ_CMND_RESET };
const uint8_t kMhzCommands[][2] PROGMEM = {
  {0x86,0x00},
  {0x79,0xA0},
  {0x79,0x00},
  {0x87,0x00},
  {0x8D,0x00}};

uint8_t mhz_type = 1;
uint16_t mhz_last_ppm = 0;
uint8_t mhz_filter = MHZ19_FILTER_OPTION;
bool mhz_abc_enable = MHZ19_ABC_ENABLE;
bool mhz_abc_must_apply = false;
char mhz_types[7];

float mhz_temperature = 0;
uint8_t mhz_retry = MHZ19_RETRY_COUNT;
uint8_t mhz_received = 0;
uint8_t mhz_state = 0;



byte MhzCalculateChecksum(byte *array)
{
  byte checksum = 0;
  for (byte i = 1; i < 8; i++) {
    checksum += array[i];
  }
  checksum = 255 - checksum;
  return (checksum +1);
}

size_t MhzSendCmd(byte command_id)
{
  uint8_t mhz_send[9] = { 0 };

  mhz_send[0] = 0xFF;
  mhz_send[1] = 0x01;
  memcpy_P(&mhz_send[2], kMhzCommands[command_id], sizeof(kMhzCommands[0]));






  mhz_send[8] = MhzCalculateChecksum(mhz_send);

  return MhzSerial->write(mhz_send, sizeof(mhz_send));
}



bool MhzCheckAndApplyFilter(uint16_t ppm, uint8_t s)
{
  if (1 == s) {
    return false;
  }
  if (mhz_last_ppm < 400 || mhz_last_ppm > 5000) {


    mhz_last_ppm = ppm;
    return true;
  }
  int32_t difference = ppm - mhz_last_ppm;
  if (s > 0 && s < 64 && mhz_filter != MHZ19_FILTER_OFF) {
# 149 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_15_mhz19.ino"
    difference *= s;
    difference /= 64;
  }
  if (MHZ19_FILTER_OFF == mhz_filter) {
    if (s != 0 && s != 64) {
      return false;
    }
  } else {
    difference >>= (mhz_filter -1);
  }
  mhz_last_ppm = static_cast<uint16_t>(mhz_last_ppm + difference);
  return true;
}

void MhzEverySecond()
{
  mhz_state++;
  if (8 == mhz_state) {
    mhz_state = 0;

    if (mhz_retry) {
      mhz_retry--;
      if (!mhz_retry) {
        mhz_last_ppm = 0;
        mhz_temperature = 0;
      }
    }

    MhzSerial->flush();
    MhzSendCmd(MHZ_CMND_READPPM);
    mhz_received = 0;
  }

  if ((mhz_state > 2) && !mhz_received) {
    uint8_t mhz_response[9];

    unsigned long start = millis();
    uint8_t counter = 0;
    while (((millis() - start) < MHZ19_READ_TIMEOUT) && (counter < 9)) {
      if (MhzSerial->available() > 0) {
        mhz_response[counter++] = MhzSerial->read();
      } else {
        delay(5);
      }
    }

    AddLogSerial(LOG_LEVEL_DEBUG_MORE, mhz_response, counter);

    if (counter < 9) {

      return;
    }

    byte crc = MhzCalculateChecksum(mhz_response);
    if (mhz_response[8] != crc) {

      return;
    }
    if (0xFF != mhz_response[0] || 0x86 != mhz_response[1]) {

      return;
    }

    mhz_received = 1;

    uint16_t u = (mhz_response[6] << 8) | mhz_response[7];
    if (15000 == u) {
      if (!mhz_abc_enable) {


        mhz_abc_must_apply = true;
      }
    } else {
      uint16_t ppm = (mhz_response[2] << 8) | mhz_response[3];
      mhz_temperature = ConvertTemp((float)mhz_response[4] - 40);
      uint8_t s = mhz_response[5];
      mhz_type = (s) ? 1 : 2;
      if (MhzCheckAndApplyFilter(ppm, s)) {
        mhz_retry = MHZ19_RETRY_COUNT;
        LightSetSignal(CO2_LOW, CO2_HIGH, mhz_last_ppm);

        if (0 == s || 64 == s) {
          if (mhz_abc_must_apply) {
            mhz_abc_must_apply = false;
            if (mhz_abc_enable) {
              MhzSendCmd(MHZ_CMND_ABCENABLE);
            } else {
              MhzSendCmd(MHZ_CMND_ABCDISABLE);
            }
          }
        }

      }
    }

  }
}
# 257 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_15_mhz19.ino"
bool MhzCommandSensor()
{
  boolean serviced = true;

  switch (XdrvMailbox.payload) {
    case 2:
      MhzSendCmd(MHZ_CMND_ZEROPOINT);
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_SENSOR_INDEX_SVALUE, XSNS_15, D_JSON_ZERO_POINT_CALIBRATION);
      break;
    case 9:
      MhzSendCmd(MHZ_CMND_RESET);
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_SENSOR_INDEX_SVALUE, XSNS_15, D_JSON_RESET);
      break;
    default:
      serviced = false;
  }

  return serviced;
}



void MhzInit()
{
  mhz_type = 0;
  if ((pin[GPIO_MHZ_RXD] < 99) && (pin[GPIO_MHZ_TXD] < 99)) {
    MhzSerial = new TasmotaSerial(pin[GPIO_MHZ_RXD], pin[GPIO_MHZ_TXD], 1);
    if (MhzSerial->begin(9600)) {
      if (MhzSerial->hardwareSerial()) { ClaimSerial(); }
      mhz_type = 1;
    }

  }
}

void MhzShow(boolean json)
{
  char temperature[10];
  dtostrfd(mhz_temperature, Settings.flag2.temperature_resolution, temperature);
  GetTextIndexed(mhz_types, sizeof(mhz_types), mhz_type -1, kMhzTypes);

  if (json) {
    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s,\"%s\":{\"" D_JSON_CO2 "\":%d,\"" D_JSON_TEMPERATURE "\":%s}"), mqtt_data, mhz_types, mhz_last_ppm, temperature);
#ifdef USE_DOMOTICZ
    if (0 == tele_period) DomoticzSensor(DZ_AIRQUALITY, mhz_last_ppm);
#endif
#ifdef USE_WEBSERVER
  } else {
    snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_SNS_CO2, mqtt_data, mhz_types, mhz_last_ppm);
    snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_SNS_TEMP, mqtt_data, mhz_types, temperature, TempUnit());
#endif
  }
}





boolean Xsns15(byte function)
{
  boolean result = false;

  if (mhz_type) {
    switch (function) {
      case FUNC_INIT:
        MhzInit();
        break;
      case FUNC_EVERY_SECOND:
        MhzEverySecond();
        break;
      case FUNC_COMMAND:
        if (XSNS_15 == XdrvMailbox.index) {
          result = MhzCommandSensor();
        }
        break;
      case FUNC_JSON_APPEND:
        MhzShow(1);
        break;
#ifdef USE_WEBSERVER
      case FUNC_WEB_APPEND:
        MhzShow(0);
        break;
#endif
    }
  }
  return result;
}

#endif
# 1 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_16_tsl2561.ino"
# 20 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_16_tsl2561.ino"
#ifdef USE_I2C
#ifdef USE_TSL2561
# 30 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_16_tsl2561.ino"
#include <Tsl2561Util.h>

Tsl2561 Tsl(Wire);

uint8_t tsl2561_type = 0;
uint8_t tsl2561_valid = 0;
uint32_t tsl2561_milliLux = 0;
char tsl2561_types[] = "TSL2561";

bool Tsl2561Read()
{
  if (tsl2561_valid) { tsl2561_valid--; }

  uint8_t id;
  bool gain;
  Tsl2561::exposure_t exposure;
  uint16_t scaledFull, scaledIr;
  uint32_t full, ir;

  if (Tsl.available()) {
    if (Tsl.on()) {
      if (Tsl.id(id)
        && Tsl2561Util::autoGain(Tsl, gain, exposure, scaledFull, scaledIr)
        && Tsl2561Util::normalizedLuminosity(gain, exposure, full = scaledFull, ir = scaledIr)
        && Tsl2561Util::milliLux(full, ir, tsl2561_milliLux, Tsl2561::packageCS(id))) {
      } else{
        tsl2561_milliLux = 0;
      }
    }
  }
  tsl2561_valid = SENSOR_MAX_MISS;
  return true;
}

void Tsl2561Detect()
{
  if (tsl2561_type) { return; }

  if (!Tsl.available()) {
    Tsl.begin();
    if (Tsl.available()) {
      tsl2561_type = 1;
      snprintf_P(log_data, sizeof(log_data), S_LOG_I2C_FOUND_AT, tsl2561_types, Tsl.address());
      AddLog(LOG_LEVEL_DEBUG);
    }
  }
}

void Tsl2561EverySecond()
{
  if (90 == (uptime %100)) {

    Tsl2561Detect();
  }
  else if (!(uptime %2)) {

    if (tsl2561_type) {
      if (!Tsl2561Read()) {
        AddLogMissed(tsl2561_types, tsl2561_valid);

      }
    }
  }
}

#ifdef USE_WEBSERVER
const char HTTP_SNS_TSL2561[] PROGMEM =
  "%s{s}TSL2561 " D_ILLUMINANCE "{m}%u.%03u " D_UNIT_LUX "{e}";
#endif

void Tsl2561Show(boolean json)
{
  if (tsl2561_valid) {
    if (json) {
      snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s,\"TSL2561\":{\"" D_JSON_ILLUMINANCE "\":%u.%03u}"),
        mqtt_data, tsl2561_milliLux / 1000, tsl2561_milliLux % 1000);
#ifdef USE_DOMOTICZ
      if (0 == tele_period) { DomoticzSensor(DZ_ILLUMINANCE, (tsl2561_milliLux + 500) / 1000); }
#endif
#ifdef USE_WEBSERVER
    } else {
      snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_SNS_TSL2561, mqtt_data, tsl2561_milliLux / 1000, tsl2561_milliLux % 1000);
#endif
    }
  }
}





#define XSNS_16 

boolean Xsns16(byte function)
{
  boolean result = false;

  if (i2c_flg) {
    switch (function) {
      case FUNC_INIT:
        Tsl2561Detect();
        break;
      case FUNC_EVERY_SECOND:
        Tsl2561EverySecond();
        break;
      case FUNC_JSON_APPEND:
        Tsl2561Show(1);
        break;
#ifdef USE_WEBSERVER
      case FUNC_WEB_APPEND:
        Tsl2561Show(0);
        break;
#endif
    }
  }
  return result;
}

#endif
#endif
# 1 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_17_senseair.ino"
# 20 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_17_senseair.ino"
#ifdef USE_SENSEAIR
# 29 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_17_senseair.ino"
#define SENSEAIR_MODBUS_SPEED 9600
#define SENSEAIR_DEVICE_ADDRESS 0xFE
#define SENSEAIR_READ_REGISTER 0x04

#ifndef CO2_LOW
#define CO2_LOW 800
#endif
#ifndef CO2_HIGH
#define CO2_HIGH 1200
#endif

#include <TasmotaModbus.h>
TasmotaModbus *SenseairModbus;

const char kSenseairTypes[] PROGMEM = "Kx0|S8";

uint8_t senseair_type = 1;
char senseair_types[7];

uint16_t senseair_co2 = 0;
float senseair_temperature = 0;
float senseair_humidity = 0;



const uint8_t start_addresses[] { 0x1A, 0x00, 0x03, 0x04, 0x05, 0x1C, 0x0A };

uint8_t senseair_read_state = 0;
uint8_t senseair_send_retry = 0;

void Senseair250ms()
{




    uint16_t value = 0;
    bool data_ready = SenseairModbus->ReceiveReady();

    if (data_ready) {
      uint8_t error = SenseairModbus->Receive16BitRegister(&value);
      if (error) {
        snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_DEBUG "SenseAir response error %d"), error);
        AddLog(LOG_LEVEL_DEBUG);
      } else {
        switch(senseair_read_state) {
          case 0:
            senseair_type = 2;
            snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_DEBUG "SenseAir type id low %04X"), value);
            AddLog(LOG_LEVEL_DEBUG);
            break;
          case 1:
            if (value) {
              snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_DEBUG "SenseAir error %04X"), value);
              AddLog(LOG_LEVEL_DEBUG);
            }
            break;
          case 2:
            senseair_co2 = value;
            LightSetSignal(CO2_LOW, CO2_HIGH, senseair_co2);
            break;
          case 3:
            senseair_temperature = ConvertTemp((float)value / 100);
            break;
          case 4:
            senseair_humidity = (float)value / 100;
            break;
          case 5:
          {
            bool relay_state = value >> 8 & 1;
            snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_DEBUG "SenseAir relay state %d"), relay_state);
            AddLog(LOG_LEVEL_DEBUG);
            break;
          }
          case 6:
            snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_DEBUG "SenseAir temp adjustment %d"), value);
            AddLog(LOG_LEVEL_DEBUG);
            break;
        }
      }
      senseair_read_state++;
      if (2 == senseair_type) {
        if (3 == senseair_read_state) {
          senseair_read_state = 1;
        }
      } else {
        if (sizeof(start_addresses) == senseair_read_state) {
          senseair_read_state = 1;
        }
      }
    }

    if (0 == senseair_send_retry || data_ready) {
      senseair_send_retry = 5;
      SenseairModbus->Send(SENSEAIR_DEVICE_ADDRESS, SENSEAIR_READ_REGISTER, (uint16_t)start_addresses[senseair_read_state], 1);
    } else {
      senseair_send_retry--;
    }


}



void SenseairInit()
{
  senseair_type = 0;
  if ((pin[GPIO_SAIR_RX] < 99) && (pin[GPIO_SAIR_TX] < 99)) {
    SenseairModbus = new TasmotaModbus(pin[GPIO_SAIR_RX], pin[GPIO_SAIR_TX]);
    uint8_t result = SenseairModbus->Begin(SENSEAIR_MODBUS_SPEED);
    if (result) {
      if (2 == result) { ClaimSerial(); }
      senseair_type = 1;
    }
  }
}

void SenseairShow(boolean json)
{
  char temperature[10];
  char humidity[10];
  dtostrfd(senseair_temperature, Settings.flag2.temperature_resolution, temperature);
  dtostrfd(senseair_humidity, Settings.flag2.temperature_resolution, humidity);
  GetTextIndexed(senseair_types, sizeof(senseair_types), senseair_type -1, kSenseairTypes);

  if (json) {
    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s,\"%s\":{\"" D_JSON_CO2 "\":%d"), mqtt_data, senseair_types, senseair_co2);
    if (senseair_type != 2) {
      snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s,\"" D_JSON_TEMPERATURE "\":%s,\"" D_JSON_HUMIDITY "\":%s"), mqtt_data, temperature, humidity);
    }
    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s}"), mqtt_data);
#ifdef USE_DOMOTICZ
    if (0 == tele_period) DomoticzSensor(DZ_AIRQUALITY, senseair_co2);
#endif
#ifdef USE_WEBSERVER
  } else {
    snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_SNS_CO2, mqtt_data, senseair_types, senseair_co2);
    if (senseair_type != 2) {
      snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_SNS_TEMP, mqtt_data, senseair_types, temperature, TempUnit());
      snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_SNS_HUM, mqtt_data, senseair_types, humidity);
    }
#endif
  }
}





#define XSNS_17 

boolean Xsns17(byte function)
{
  boolean result = false;

  if (senseair_type) {
    switch (function) {
      case FUNC_INIT:
        SenseairInit();
        break;
      case FUNC_EVERY_250_MSECOND:
        Senseair250ms();
        break;
      case FUNC_JSON_APPEND:
        SenseairShow(1);
        break;
#ifdef USE_WEBSERVER
      case FUNC_WEB_APPEND:
        SenseairShow(0);
        break;
#endif
    }
  }
  return result;
}

#endif
# 1 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_18_pms5003.ino"
# 20 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_18_pms5003.ino"
#ifdef USE_PMS5003







#include <TasmotaSerial.h>

TasmotaSerial *PmsSerial;

uint8_t pms_type = 1;
uint8_t pms_valid = 0;

struct pms5003data {
  uint16_t framelen;
  uint16_t pm10_standard, pm25_standard, pm100_standard;
  uint16_t pm10_env, pm25_env, pm100_env;
  uint16_t particles_03um, particles_05um, particles_10um, particles_25um, particles_50um, particles_100um;
  uint16_t unused;
  uint16_t checksum;
} pms_data;



boolean PmsReadData()
{
  if (! PmsSerial->available()) {
    return false;
  }
  while ((PmsSerial->peek() != 0x42) && PmsSerial->available()) {
    PmsSerial->read();
  }
  if (PmsSerial->available() < 32) {
    return false;
  }

  uint8_t buffer[32];
  uint16_t sum = 0;
  PmsSerial->readBytes(buffer, 32);
  PmsSerial->flush();

  AddLogSerial(LOG_LEVEL_DEBUG_MORE, buffer, 32);


  for (uint8_t i = 0; i < 30; i++) {
    sum += buffer[i];
  }

  uint16_t buffer_u16[15];
  for (uint8_t i = 0; i < 15; i++) {
    buffer_u16[i] = buffer[2 + i*2 + 1];
    buffer_u16[i] += (buffer[2 + i*2] << 8);
  }
  if (sum != buffer_u16[14]) {
    AddLog_P(LOG_LEVEL_DEBUG, PSTR("PMS: " D_CHECKSUM_FAILURE));
    return false;
  }

  memcpy((void *)&pms_data, (void *)buffer_u16, 30);
  pms_valid = 10;

  return true;
}



void PmsSecond()
{
  if (PmsReadData()) {
    pms_valid = 10;
  } else {
    if (pms_valid) {
      pms_valid--;
    }
  }
}



void PmsInit()
{
  pms_type = 0;
  if (pin[GPIO_PMS5003] < 99) {
    PmsSerial = new TasmotaSerial(pin[GPIO_PMS5003], -1, 1);
    if (PmsSerial->begin(9600)) {
      if (PmsSerial->hardwareSerial()) { ClaimSerial(); }
      pms_type = 1;
    }
  }
}

#ifdef USE_WEBSERVER
const char HTTP_PMS5003_SNS[] PROGMEM = "%s"



  "{s}PMS5003 " D_ENVIRONMENTAL_CONCENTRATION " 1 " D_UNIT_MICROMETER "{m}%d " D_UNIT_MICROGRAM_PER_CUBIC_METER "{e}"
  "{s}PMS5003 " D_ENVIRONMENTAL_CONCENTRATION " 2.5 " D_UNIT_MICROMETER "{m}%d " D_UNIT_MICROGRAM_PER_CUBIC_METER "{e}"
  "{s}PMS5003 " D_ENVIRONMENTAL_CONCENTRATION " 10 " D_UNIT_MICROMETER "{m}%d " D_UNIT_MICROGRAM_PER_CUBIC_METER "{e}"
  "{s}PMS5003 " D_PARTICALS_BEYOND " 0.3 " D_UNIT_MICROMETER "{m}%d " D_UNIT_PARTS_PER_DECILITER "{e}"
  "{s}PMS5003 " D_PARTICALS_BEYOND " 0.5 " D_UNIT_MICROMETER "{m}%d " D_UNIT_PARTS_PER_DECILITER "{e}"
  "{s}PMS5003 " D_PARTICALS_BEYOND " 1 " D_UNIT_MICROMETER "{m}%d " D_UNIT_PARTS_PER_DECILITER "{e}"
  "{s}PMS5003 " D_PARTICALS_BEYOND " 2.5 " D_UNIT_MICROMETER "{m}%d " D_UNIT_PARTS_PER_DECILITER "{e}"
  "{s}PMS5003 " D_PARTICALS_BEYOND " 5 " D_UNIT_MICROMETER "{m}%d " D_UNIT_PARTS_PER_DECILITER "{e}"
  "{s}PMS5003 " D_PARTICALS_BEYOND " 10 " D_UNIT_MICROMETER "{m}%d " D_UNIT_PARTS_PER_DECILITER "{e}";
#endif

void PmsShow(boolean json)
{
  if (pms_valid) {
    if (json) {
      snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s,\"PMS5003\":{\"CF1\":%d,\"CF2.5\":%d,\"CF10\":%d,\"PM1\":%d,\"PM2.5\":%d,\"PM10\":%d,\"PB0.3\":%d,\"PB0.5\":%d,\"PB1\":%d,\"PB2.5\":%d,\"PB5\":%d,\"PB10\":%d}"), mqtt_data,
        pms_data.pm10_standard, pms_data.pm25_standard, pms_data.pm100_standard,
        pms_data.pm10_env, pms_data.pm25_env, pms_data.pm100_env,
        pms_data.particles_03um, pms_data.particles_05um, pms_data.particles_10um, pms_data.particles_25um, pms_data.particles_50um, pms_data.particles_100um);
#ifdef USE_DOMOTICZ
      if (0 == tele_period) {
        DomoticzSensor(DZ_COUNT, pms_data.pm10_env);
        DomoticzSensor(DZ_VOLTAGE, pms_data.pm25_env);
        DomoticzSensor(DZ_CURRENT, pms_data.pm100_env);
      }
#endif
#ifdef USE_WEBSERVER
    } else {
      snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_PMS5003_SNS, mqtt_data,

        pms_data.pm10_env, pms_data.pm25_env, pms_data.pm100_env,
        pms_data.particles_03um, pms_data.particles_05um, pms_data.particles_10um, pms_data.particles_25um, pms_data.particles_50um, pms_data.particles_100um);
#endif
    }
  }
}





#define XSNS_18 

boolean Xsns18(byte function)
{
  boolean result = false;

  if (pms_type) {
    switch (function) {
      case FUNC_INIT:
        PmsInit();
        break;
      case FUNC_EVERY_SECOND:
        PmsSecond();
        break;
      case FUNC_JSON_APPEND:
        PmsShow(1);
        break;
#ifdef USE_WEBSERVER
      case FUNC_WEB_APPEND:
        PmsShow(0);
        break;
#endif
    }
  }
  return result;
}

#endif
# 1 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_19_mgs.ino"
# 20 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_19_mgs.ino"
#ifdef USE_I2C
#ifdef USE_MGS







#ifndef MGS_SENSOR_ADDR
#define MGS_SENSOR_ADDR 0x04
#endif

#include "MutichannelGasSensor.h"

void MGSInit() {
  gas.begin(MGS_SENSOR_ADDR);
}

boolean MGSPrepare()
{
  gas.begin(MGS_SENSOR_ADDR);
  if (!gas.isError()) {
    snprintf_P(log_data, sizeof(log_data), S_LOG_I2C_FOUND_AT, "MultiGasSensor", MGS_SENSOR_ADDR);
    AddLog(LOG_LEVEL_DEBUG);
    return true;
  } else {
    return false;
  }
}

char* measure_gas(int gas_type, char* buffer)
{
  float f = gas.calcGas(gas_type);
  dtostrfd(f, 2, buffer);
  return buffer;
}

#ifdef USE_WEBSERVER
const char HTTP_MGS_GAS[] PROGMEM = "%s{s}MGS %s{m}%s " D_UNIT_PARTS_PER_MILLION "{e}";
#endif

void MGSShow(boolean json)
{
  char buffer[25];
  if (json) {
    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s,\"MGS\":{\"NH3\":%s"), mqtt_data, measure_gas(NH3, buffer));
    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s,\"CO\":%s"), mqtt_data, measure_gas(CO, buffer));
    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s,\"NO2\":%s"), mqtt_data, measure_gas(NO2, buffer));
    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s,\"C3H8\":%s"), mqtt_data, measure_gas(C3H8, buffer));
    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s,\"C4H10\":%s"), mqtt_data, measure_gas(C4H10, buffer));
    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s,\"CH4\":%s"), mqtt_data, measure_gas(GAS_CH4, buffer));
    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s,\"H2\":%s"), mqtt_data, measure_gas(H2, buffer));
    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s,\"C2H5OH\":%s}"), mqtt_data, measure_gas(C2H5OH, buffer));
#ifdef USE_WEBSERVER
  } else {
    snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_MGS_GAS, mqtt_data, "NH3", measure_gas(NH3, buffer));
    snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_MGS_GAS, mqtt_data, "CO", measure_gas(CO, buffer));
    snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_MGS_GAS, mqtt_data, "NO2", measure_gas(NO2, buffer));
    snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_MGS_GAS, mqtt_data, "C3H8", measure_gas(C3H8, buffer));
    snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_MGS_GAS, mqtt_data, "C4H10", measure_gas(C4H10, buffer));
    snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_MGS_GAS, mqtt_data, "CH4", measure_gas(GAS_CH4, buffer));
    snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_MGS_GAS, mqtt_data, "H2", measure_gas(H2, buffer));
    snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_MGS_GAS, mqtt_data, "C2H5OH", measure_gas(C2H5OH, buffer));
#endif
  }
}





#define XSNS_19 

boolean Xsns19(byte function)
{
  boolean result = false;
  static int detected = false;

  if (i2c_flg) {
    switch (function) {
      case FUNC_INIT:

        break;
      case FUNC_PREP_BEFORE_TELEPERIOD:
        detected = MGSPrepare();
        break;
      case FUNC_JSON_APPEND:
        if (detected) MGSShow(1);
        break;
#ifdef USE_WEBSERVER
      case FUNC_WEB_APPEND:
        if (detected) MGSShow(0);
        break;
#endif
    }
  }
  return result;
}

#endif
#endif
# 1 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_20_novasds.ino"
# 20 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_20_novasds.ino"
#ifdef USE_NOVA_SDS







#include <TasmotaSerial.h>

#ifndef WORKING_PERIOD
 #define WORKING_PERIOD 5
#endif

TasmotaSerial *NovaSdsSerial;

uint8_t novasds_type = 1;
uint8_t novasds_valid = 0;

uint8_t novasds_workperiod[19] = {0xAA, 0xB4, 0x08, 0x01, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x0C, 0xAB};
uint8_t novasds_setquerymode[19] = {0xAA, 0xB4, 0x02, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x02, 0xAB};
uint8_t novasds_querydata[19] = {0xAA, 0xB4, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x02, 0xAB};


struct sds011data {
  uint16_t pm100;
  uint16_t pm25;
} novasds_data;

void NovaSdsSetWorkPeriod()
{

  while (NovaSdsSerial->available() > 0) {
    NovaSdsSerial->read();
  }

  novasds_workperiod[4] = WORKING_PERIOD;
  novasds_workperiod[17] = ((novasds_workperiod[2] + novasds_workperiod[3] + novasds_workperiod[4] + novasds_workperiod[15] + novasds_workperiod[16]) & 0xFF);

  NovaSdsSerial->write(novasds_workperiod, sizeof(novasds_workperiod));
  NovaSdsSerial->flush();

  while (NovaSdsSerial->available() > 0) {
    NovaSdsSerial->read();
  }

  NovaSdsSerial->write(novasds_setquerymode, sizeof(novasds_setquerymode));
  NovaSdsSerial->flush();

  while (NovaSdsSerial->available() > 0) {
    NovaSdsSerial->read();
  }
}



bool NovaSdsReadData()
{
  if (! NovaSdsSerial->available()) return false;

  NovaSdsSerial->write(novasds_querydata, sizeof(novasds_querydata));
  NovaSdsSerial->flush();

  while ((NovaSdsSerial->peek() != 0xAA) && NovaSdsSerial->available()) {
    NovaSdsSerial->read();
  }

  byte d[8] = { 0 };
  NovaSdsSerial->read();
  NovaSdsSerial->readBytes(d, 8);
  NovaSdsSerial->flush();

  AddLogSerial(LOG_LEVEL_DEBUG_MORE, d, 8);

  if (d[7] == ((d[1] + d[2] + d[3] + d[4] + d[5] + d[6]) & 0xFF)) {
    novasds_data.pm25 = (d[1] + 256 * d[2]);
    novasds_data.pm100 = (d[3] + 256 * d[4]);
  } else {
    AddLog_P(LOG_LEVEL_DEBUG, PSTR("SDS: " D_CHECKSUM_FAILURE));
    return false;
  }

  novasds_valid = 10;

  return true;
}



void NovaSdsSecond()
{
  if (NovaSdsReadData()) {
    novasds_valid = 10;
  } else {
    if (novasds_valid) {
      novasds_valid--;
    }
  }
}



void NovaSdsInit()
{
  novasds_type = 0;
  if (pin[GPIO_SDS0X1_RX] < 99 && pin[GPIO_SDS0X1_TX] < 99) {
    NovaSdsSerial = new TasmotaSerial(pin[GPIO_SDS0X1_RX], pin[GPIO_SDS0X1_TX], 1);
    if (NovaSdsSerial->begin(9600)) {
      if (NovaSdsSerial->hardwareSerial()) {
        ClaimSerial();
      }
      novasds_type = 1;
      NovaSdsSetWorkPeriod();
    }
  }
}

#ifdef USE_WEBSERVER
const char HTTP_SDS0X1_SNS[] PROGMEM = "%s"
  "{s}SDS0X1 " D_ENVIRONMENTAL_CONCENTRATION " 2.5 " D_UNIT_MICROMETER "{m}%s " D_UNIT_MICROGRAM_PER_CUBIC_METER "{e}"
  "{s}SDS0X1 " D_ENVIRONMENTAL_CONCENTRATION " 10 " D_UNIT_MICROMETER "{m}%s " D_UNIT_MICROGRAM_PER_CUBIC_METER "{e}";
#endif

void NovaSdsShow(boolean json)
{
  if (novasds_valid) {
    char pm10[10];
    char pm2_5[10];
    float pm10f = (float)(novasds_data.pm100) / 10.0f;
    float pm2_5f = (float)(novasds_data.pm25) / 10.0f;
    dtostrfd(pm10f, 1, pm10);
    dtostrfd(pm2_5f, 1, pm2_5);
    if (json) {
      snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s,\"SDS0X1\":{\"PM2.5\":%s,\"PM10\":%s}"), mqtt_data, pm2_5, pm10);
#ifdef USE_DOMOTICZ
      if (0 == tele_period) {
        DomoticzSensor(DZ_VOLTAGE, pm2_5);
        DomoticzSensor(DZ_CURRENT, pm10);
      }
#endif
#ifdef USE_WEBSERVER
    } else {
      snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_SDS0X1_SNS, mqtt_data, pm2_5, pm10);
#endif
    }
  }
}





#define XSNS_20 

boolean Xsns20(byte function)
{
  boolean result = false;

  if (novasds_type) {
    switch (function) {
      case FUNC_INIT:
        NovaSdsInit();
        break;
      case FUNC_EVERY_SECOND:
        NovaSdsSecond();
        break;
      case FUNC_JSON_APPEND:
        NovaSdsShow(1);
        break;
#ifdef USE_WEBSERVER
      case FUNC_WEB_APPEND:
        NovaSdsShow(0);
        break;
#endif
    }
  }
  return result;
}

#endif
# 1 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_21_sgp30.ino"
# 20 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_21_sgp30.ino"
#ifdef USE_I2C
#ifdef USE_SGP30
# 30 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_21_sgp30.ino"
#include "Adafruit_SGP30.h"
Adafruit_SGP30 sgp;

uint8_t sgp30_type = 0;
uint8_t sgp30_ready = 0;
uint8_t sgp30_counter = 0;



void Sgp30Update()
{
  sgp30_ready = 0;
  if (!sgp30_type) {
    if (sgp.begin()) {
      sgp30_type = 1;


      snprintf_P(log_data, sizeof(log_data), S_LOG_I2C_FOUND_AT, "SGP30", 0x58);
      AddLog(LOG_LEVEL_DEBUG);
    }
  } else {
    if (!sgp.IAQmeasure()) return;
    sgp30_counter++;
    if (30 == sgp30_counter) {
      sgp30_counter = 0;

      uint16_t TVOC_base;
      uint16_t eCO2_base;

      if (!sgp.getIAQBaseline(&eCO2_base, &TVOC_base)) return;


    }
    sgp30_ready = 1;
  }
}

const char HTTP_SNS_SGP30[] PROGMEM = "%s"
  "{s}SGP30 " D_ECO2 "{m}%d " D_UNIT_PARTS_PER_MILLION "{e}"
  "{s}SGP30 " D_TVOC "{m}%d " D_UNIT_PARTS_PER_BILLION "{e}";

void Sgp30Show(boolean json)
{
  if (sgp30_ready) {
    if (json) {
      snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s,\"SGP30\":{\"" D_JSON_ECO2 "\":%d,\"" D_JSON_TVOC "\":%d}"), mqtt_data, sgp.eCO2, sgp.TVOC);
#ifdef USE_DOMOTICZ
      if (0 == tele_period) DomoticzSensor(DZ_AIRQUALITY, sgp.eCO2);
#endif
#ifdef USE_WEBSERVER
    } else {
      snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_SNS_SGP30, mqtt_data, sgp.eCO2, sgp.TVOC);
#endif
    }
  }
}





#define XSNS_21 

boolean Xsns21(byte function)
{
  boolean result = false;

  if (i2c_flg) {
    switch (function) {
      case FUNC_EVERY_SECOND:
        Sgp30Update();
        break;
      case FUNC_JSON_APPEND:
        Sgp30Show(1);
        break;
#ifdef USE_WEBSERVER
      case FUNC_WEB_APPEND:
        Sgp30Show(0);
        break;
#endif
    }
  }
  return result;
}

#endif
#endif
# 1 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_22_sr04.ino"
# 20 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_22_sr04.ino"
#ifdef USE_SR04
# 29 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_22_sr04.ino"
uint8_t sr04_echo_pin = 0;
uint8_t sr04_trig_pin = 0;





#define US_ROUNDTRIP_CM 58
#define US_ROUNDTRIP_IN 148
#define PING_MEDIAN_DELAY 29000
#define MAX_SENSOR_DISTANCE 500
#define PING_OVERHEAD 5


#define EchoConvert(echoTime,conversionFactor) (tmax(((unsigned int)echoTime + conversionFactor / 2) / conversionFactor, (echoTime ? 1 : 0)))



void Sr04Init()
{
  sr04_echo_pin = pin[GPIO_SR04_ECHO];
  sr04_trig_pin = pin[GPIO_SR04_TRIG];
  pinMode(sr04_trig_pin, OUTPUT);
  pinMode(sr04_echo_pin, INPUT_PULLUP);
}

boolean Sr04Read(uint16_t *distance)
{
  uint16_t duration = 0;

  *distance = 0;


  duration = Sr04GetSamples(9, 250);


  *distance = EchoConvert(duration, US_ROUNDTRIP_CM);

  return 1;
}

uint16_t Sr04Ping(uint16_t max_cm_distance)
{
  uint16_t duration = 0;
  uint16_t maxEchoTime;

  maxEchoTime = tmin(max_cm_distance + 1, (uint16_t) MAX_SENSOR_DISTANCE + 1) * US_ROUNDTRIP_CM;



  digitalWrite(sr04_trig_pin, LOW);
  delayMicroseconds(2);
  digitalWrite(sr04_trig_pin, HIGH);
  delayMicroseconds(10);
  digitalWrite(sr04_trig_pin, LOW);


  duration = pulseIn(sr04_echo_pin, HIGH, 26000) - PING_OVERHEAD;

  return (duration > maxEchoTime) ? 0 : duration;
}

uint16_t Sr04GetSamples(uint8_t it, uint16_t max_cm_distance)
{
  uint16_t uS[it];
  uint16_t last;
  uint8_t j;
  uint8_t i = 0;
  uint16_t t;
  uS[0] = 0;

  while (i < it) {
    t = micros();
    last = Sr04Ping(max_cm_distance);

    if (last != 0) {
      if (i > 0) {
        for (j = i; j > 0 && uS[j - 1] < last; j--) {
          uS[j] = uS[j - 1];
        }
      } else {
        j = 0;
      }
      uS[j] = last;
      i++;
    } else {
      it--;
    }
    if (i < it && micros() - t < PING_MEDIAN_DELAY) {
      delay((PING_MEDIAN_DELAY + t - micros()) / 1000);
    }
  }

  return (uS[1]);
}

#ifdef USE_WEBSERVER
const char HTTP_SNS_DISTANCE[] PROGMEM =
  "%s{s}SR04 " D_DISTANCE "{m}%d" D_UNIT_CENTIMETER "{e}";
#endif

void Sr04Show(boolean json)
{
  uint16_t distance;

  if (Sr04Read(&distance)) {
    if(json) {
      snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s,\"SR04\":{\"" D_JSON_DISTANCE "\":%d}"), mqtt_data, distance);
#ifdef USE_WEBSERVER
    } else {
      snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_SNS_DISTANCE, mqtt_data, distance);
#endif
    }
  }
}





#define XSNS_22 

boolean Xsns22(byte function)
{
  boolean result = false;

  if ((pin[GPIO_SR04_ECHO] < 99) && (pin[GPIO_SR04_TRIG] < 99)) {
    switch (function) {
      case FUNC_INIT:
        Sr04Init();
        break;
      case FUNC_JSON_APPEND:
        Sr04Show(1);
        break;
#ifdef USE_WEBSERVER
      case FUNC_WEB_APPEND:
        Sr04Show(0);
        break;
#endif
    }
  }
  return result;
}

#endif
# 1 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_23_sdm120.ino"
# 20 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_23_sdm120.ino"
#ifdef USE_SDM120







#include <TasmotaSerial.h>

TasmotaSerial *SDM120Serial;

uint8_t sdm120_type = 1;


float sdm120_voltage = 0;
float sdm120_current = 0;
float sdm120_active_power = 0;
float sdm120_apparent_power = 0;
float sdm120_reactive_power = 0;
float sdm120_power_factor = 0;
float sdm120_frequency = 0;
float sdm120_energy_total = 0;

bool SDM120_ModbusReceiveReady()
{
  return (SDM120Serial->available() > 1);
}

void SDM120_ModbusSend(uint8_t function_code, uint16_t start_address, uint16_t register_count)
{
  uint8_t frame[8];

  frame[0] = 0x01;
  frame[1] = function_code;
  frame[2] = (uint8_t)(start_address >> 8);
  frame[3] = (uint8_t)(start_address);
  frame[4] = (uint8_t)(register_count >> 8);
  frame[5] = (uint8_t)(register_count);

  uint16_t crc = SDM120_calculateCRC(frame, 6);
  frame[6] = lowByte(crc);
  frame[7] = highByte(crc);

  while (SDM120Serial->available() > 0) {
    SDM120Serial->read();
  }

  SDM120Serial->flush();
  SDM120Serial->write(frame, sizeof(frame));
}

uint8_t SDM120_ModbusReceive(float *value)
{
  uint8_t buffer[9];

  *value = NAN;
  uint8_t len = 0;
  while (SDM120Serial->available() > 0) {
    buffer[len++] = (uint8_t)SDM120Serial->read();
  }

  if (len < 9)
    return 3;

  if (len == 9) {

    if (buffer[0] == 0x01 && buffer[1] == 0x04 && buffer[2] == 4) {

      if((SDM120_calculateCRC(buffer, 7)) == ((buffer[8] << 8) | buffer[7])) {

        ((uint8_t*)value)[3] = buffer[3];
        ((uint8_t*)value)[2] = buffer[4];
        ((uint8_t*)value)[1] = buffer[5];
        ((uint8_t*)value)[0] = buffer[6];

      } else return 1;

    } else return 2;
  }

  return 0;
}

uint16_t SDM120_calculateCRC(uint8_t *frame, uint8_t num)
{
  uint16_t crc, flag;
  crc = 0xFFFF;
  for (uint8_t i = 0; i < num; i++) {
    crc ^= frame[i];
    for (uint8_t j = 8; j; j--) {
      if ((crc & 0x0001) != 0) {
        crc >>= 1;
        crc ^= 0xA001;
      } else {
        crc >>= 1;
      }
    }
  }
  return crc;
}



const uint16_t sdm120_start_addresses[] {
  0x0000,
  0x0006,
  0x000C,
  0x0012,
  0x0018,
  0x001E,
  0x0046,
  0x0156
};

uint8_t sdm120_read_state = 0;
uint8_t sdm120_send_retry = 0;

void SDM120250ms()
{




    float value = 0;
    bool data_ready = SDM120_ModbusReceiveReady();

    if (data_ready) {
      uint8_t error = SDM120_ModbusReceive(&value);
      if (error) {
        snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_DEBUG "SDM120 response error %d"), error);
        AddLog(LOG_LEVEL_DEBUG);
      } else {
        switch(sdm120_read_state) {
          case 0:
            sdm120_voltage = value;
            break;

          case 1:
            sdm120_current = value;
            break;

          case 2:
            sdm120_active_power = value;
            break;

          case 3:
            sdm120_apparent_power = value;
            break;

          case 4:
            sdm120_reactive_power = value;
            break;

          case 5:
            sdm120_power_factor = value;
            break;

          case 6:
            sdm120_frequency = value;
            break;

          case 7:
            sdm120_energy_total = value;
            break;
        }

        sdm120_read_state++;

        if (sizeof(sdm120_start_addresses)/2 == sdm120_read_state) {
          sdm120_read_state = 0;
        }
      }
    }

    if (0 == sdm120_send_retry || data_ready) {
      sdm120_send_retry = 5;
       SDM120_ModbusSend(0x04, sdm120_start_addresses[sdm120_read_state], 2);
    } else {
      sdm120_send_retry--;
    }

}

void SDM120Init()
{
  sdm120_type = 0;
  if ((pin[GPIO_SDM120_RX] < 99) && (pin[GPIO_SDM120_TX] < 99)) {
    SDM120Serial = new TasmotaSerial(pin[GPIO_SDM120_RX], pin[GPIO_SDM120_TX], 1);
#ifdef SDM120_SPEED
    if (SDM120Serial->begin(SDM120_SPEED)) {
#else
    if (SDM120Serial->begin(2400)) {
#endif
      if (SDM120Serial->hardwareSerial()) { ClaimSerial(); }
      sdm120_type = 1;
    }
  }
}

#ifdef USE_WEBSERVER
const char HTTP_SNS_SDM120_DATA[] PROGMEM = "%s"
  "{s}SDM120 " D_VOLTAGE "{m}%s " D_UNIT_VOLT "{e}"
  "{s}SDM120 " D_CURRENT "{m}%s " D_UNIT_AMPERE "{e}"
  "{s}SDM120 " D_POWERUSAGE_ACTIVE "{m}%s " D_UNIT_WATT "{e}"
  "{s}SDM120 " D_POWERUSAGE_APPARENT "{m}%s " D_UNIT_VA "{e}"
  "{s}SDM120 " D_POWERUSAGE_REACTIVE "{m}%s " D_UNIT_VAR "{e}"
  "{s}SDM120 " D_POWER_FACTOR "{m}%s{e}"
  "{s}SDM120 " D_FREQUENCY "{m}%s " D_UNIT_HERTZ "{e}"
  "{s}SDM120 " D_ENERGY_TOTAL "{m}%s " D_UNIT_KILOWATTHOUR "{e}";
#endif

void SDM120Show(boolean json)
{
  char voltage[10];
  char current[10];
  char active_power[10];
  char apparent_power[10];
  char reactive_power[10];
  char power_factor[10];
  char frequency[10];
  char energy_total[10];

  dtostrfd(sdm120_voltage, Settings.flag2.voltage_resolution, voltage);
  dtostrfd(sdm120_current, Settings.flag2.current_resolution, current);
  dtostrfd(sdm120_active_power, Settings.flag2.wattage_resolution, active_power);
  dtostrfd(sdm120_apparent_power, Settings.flag2.wattage_resolution, apparent_power);
  dtostrfd(sdm120_reactive_power, Settings.flag2.wattage_resolution, reactive_power);
  dtostrfd(sdm120_power_factor, 2, power_factor);
  dtostrfd(sdm120_frequency, Settings.flag2.frequency_resolution, frequency);
  dtostrfd(sdm120_energy_total, Settings.flag2.energy_resolution, energy_total);

  if (json) {
    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s,\"" D_RSLT_ENERGY "\":{\"" D_JSON_TOTAL "\":%s,\"" D_JSON_ACTIVE_POWERUSAGE "\":%s,\"" D_JSON_APPARENT_POWERUSAGE "\":%s,\"" D_JSON_REACTIVE_POWERUSAGE "\":%s,\"" D_JSON_FREQUENCY "\":%s,\"" D_JSON_POWERFACTOR "\":%s,\"" D_JSON_VOLTAGE "\":%s,\"" D_JSON_CURRENT "\":%s}"),
      mqtt_data, energy_total, active_power, apparent_power, reactive_power, frequency, power_factor, voltage, current);
#ifdef USE_DOMOTICZ
    if (0 == tele_period) {
      DomoticzSensor(DZ_VOLTAGE, voltage);
      DomoticzSensor(DZ_CURRENT, current);
      DomoticzSensorPowerEnergy((int)sdm120_active_power, energy_total);
    }
#endif
#ifdef USE_WEBSERVER
  } else {
    snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_SNS_SDM120_DATA, mqtt_data, voltage, current, active_power, apparent_power, reactive_power, power_factor, frequency, energy_total);
#endif
  }
}





#define XSNS_23 

boolean Xsns23(byte function)
{
  boolean result = false;

  if (sdm120_type) {
    switch (function) {
      case FUNC_INIT:
        SDM120Init();
        break;
      case FUNC_EVERY_250_MSECOND:
        SDM120250ms();
        break;
      case FUNC_JSON_APPEND:
        SDM120Show(1);
        break;
#ifdef USE_WEBSERVER
      case FUNC_WEB_APPEND:
        SDM120Show(0);
        break;
#endif
    }
  }
  return result;
}

#endif
# 1 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_24_si1145.ino"
# 20 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_24_si1145.ino"
#ifdef USE_I2C
#ifdef USE_SI1145
# 30 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_24_si1145.ino"
uint8_t si1145_type = 0;



#define SI114X_ADDR 0X60



#define SI114X_QUERY 0X80
#define SI114X_SET 0XA0
#define SI114X_NOP 0X00
#define SI114X_RESET 0X01
#define SI114X_BUSADDR 0X02
#define SI114X_PS_FORCE 0X05
#define SI114X_GET_CAL 0X12
#define SI114X_ALS_FORCE 0X06
#define SI114X_PSALS_FORCE 0X07
#define SI114X_PS_PAUSE 0X09
#define SI114X_ALS_PAUSE 0X0A
#define SI114X_PSALS_PAUSE 0X0B
#define SI114X_PS_AUTO 0X0D
#define SI114X_ALS_AUTO 0X0E
#define SI114X_PSALS_AUTO 0X0F



#define SI114X_PART_ID 0X00
#define SI114X_REV_ID 0X01
#define SI114X_SEQ_ID 0X02
#define SI114X_INT_CFG 0X03
#define SI114X_IRQ_ENABLE 0X04
#define SI114X_IRQ_MODE1 0x05
#define SI114X_IRQ_MODE2 0x06
#define SI114X_HW_KEY 0X07
#define SI114X_MEAS_RATE0 0X08
#define SI114X_MEAS_RATE1 0X09
#define SI114X_PS_RATE 0X0A
#define SI114X_PS_LED21 0X0F
#define SI114X_PS_LED3 0X10
#define SI114X_UCOEFF0 0X13
#define SI114X_UCOEFF1 0X14
#define SI114X_UCOEFF2 0X15
#define SI114X_UCOEFF3 0X16
#define SI114X_WR 0X17
#define SI114X_COMMAND 0X18
#define SI114X_RESPONSE 0X20
#define SI114X_IRQ_STATUS 0X21
#define SI114X_ALS_VIS_DATA0 0X22
#define SI114X_ALS_VIS_DATA1 0X23
#define SI114X_ALS_IR_DATA0 0X24
#define SI114X_ALS_IR_DATA1 0X25
#define SI114X_PS1_DATA0 0X26
#define SI114X_PS1_DATA1 0X27
#define SI114X_PS2_DATA0 0X28
#define SI114X_PS2_DATA1 0X29
#define SI114X_PS3_DATA0 0X2A
#define SI114X_PS3_DATA1 0X2B
#define SI114X_AUX_DATA0_UVINDEX0 0X2C
#define SI114X_AUX_DATA1_UVINDEX1 0X2D
#define SI114X_RD 0X2E
#define SI114X_CHIP_STAT 0X30



#define SI114X_CHLIST 0X01
#define SI114X_CHLIST_ENUV 0x80
#define SI114X_CHLIST_ENAUX 0x40
#define SI114X_CHLIST_ENALSIR 0x20
#define SI114X_CHLIST_ENALSVIS 0x10
#define SI114X_CHLIST_ENPS1 0x01
#define SI114X_CHLIST_ENPS2 0x02
#define SI114X_CHLIST_ENPS3 0x04

#define SI114X_PSLED12_SELECT 0X02
#define SI114X_PSLED3_SELECT 0X03

#define SI114X_PS_ENCODE 0X05
#define SI114X_ALS_ENCODE 0X06

#define SI114X_PS1_ADCMUX 0X07
#define SI114X_PS2_ADCMUX 0X08
#define SI114X_PS3_ADCMUX 0X09

#define SI114X_PS_ADC_COUNTER 0X0A
#define SI114X_PS_ADC_GAIN 0X0B
#define SI114X_PS_ADC_MISC 0X0C

#define SI114X_ALS_IR_ADC_MUX 0X0E
#define SI114X_AUX_ADC_MUX 0X0F

#define SI114X_ALS_VIS_ADC_COUNTER 0X10
#define SI114X_ALS_VIS_ADC_GAIN 0X11
#define SI114X_ALS_VIS_ADC_MISC 0X12

#define SI114X_LED_REC 0X1C

#define SI114X_ALS_IR_ADC_COUNTER 0X1D
#define SI114X_ALS_IR_ADC_GAIN 0X1E
#define SI114X_ALS_IR_ADC_MISC 0X1F




#define SI114X_ADCMUX_SMALL_IR 0x00
#define SI114X_ADCMUX_VISIABLE 0x02
#define SI114X_ADCMUX_LARGE_IR 0x03
#define SI114X_ADCMUX_NO 0x06
#define SI114X_ADCMUX_GND 0x25
#define SI114X_ADCMUX_TEMPERATURE 0x65
#define SI114X_ADCMUX_VDD 0x75

#define SI114X_PSLED12_SELECT_PS1_NONE 0x00
#define SI114X_PSLED12_SELECT_PS1_LED1 0x01
#define SI114X_PSLED12_SELECT_PS1_LED2 0x02
#define SI114X_PSLED12_SELECT_PS1_LED3 0x04
#define SI114X_PSLED12_SELECT_PS2_NONE 0x00
#define SI114X_PSLED12_SELECT_PS2_LED1 0x10
#define SI114X_PSLED12_SELECT_PS2_LED2 0x20
#define SI114X_PSLED12_SELECT_PS2_LED3 0x40
#define SI114X_PSLED3_SELECT_PS2_NONE 0x00
#define SI114X_PSLED3_SELECT_PS2_LED1 0x10
#define SI114X_PSLED3_SELECT_PS2_LED2 0x20
#define SI114X_PSLED3_SELECT_PS2_LED3 0x40

#define SI114X_ADC_GAIN_DIV1 0X00
#define SI114X_ADC_GAIN_DIV2 0X01
#define SI114X_ADC_GAIN_DIV4 0X02
#define SI114X_ADC_GAIN_DIV8 0X03
#define SI114X_ADC_GAIN_DIV16 0X04
#define SI114X_ADC_GAIN_DIV32 0X05

#define SI114X_LED_CURRENT_5MA 0X01
#define SI114X_LED_CURRENT_11MA 0X02
#define SI114X_LED_CURRENT_22MA 0X03
#define SI114X_LED_CURRENT_45MA 0X04

#define SI114X_ADC_COUNTER_1ADCCLK 0X00
#define SI114X_ADC_COUNTER_7ADCCLK 0X01
#define SI114X_ADC_COUNTER_15ADCCLK 0X02
#define SI114X_ADC_COUNTER_31ADCCLK 0X03
#define SI114X_ADC_COUNTER_63ADCCLK 0X04
#define SI114X_ADC_COUNTER_127ADCCLK 0X05
#define SI114X_ADC_COUNTER_255ADCCLK 0X06
#define SI114X_ADC_COUNTER_511ADCCLK 0X07

#define SI114X_ADC_MISC_LOWRANGE 0X00
#define SI114X_ADC_MISC_HIGHRANGE 0X20
#define SI114X_ADC_MISC_ADC_NORMALPROXIMITY 0X00
#define SI114X_ADC_MISC_ADC_RAWADC 0X04

#define SI114X_INT_CFG_INTOE 0X01

#define SI114X_IRQEN_ALS 0x01
#define SI114X_IRQEN_PS1 0x04
#define SI114X_IRQEN_PS2 0x08
#define SI114X_IRQEN_PS3 0x10



uint8_t Si1145ReadByte(uint8_t reg)
{
  return I2cRead8(SI114X_ADDR, reg);
}

uint16_t Si1145ReadHalfWord(uint8_t reg)
{
  return I2cRead16LE(SI114X_ADDR, reg);
}

bool Si1145WriteByte(uint8_t reg, uint16_t val)
{
  I2cWrite8(SI114X_ADDR, reg, val);
}

uint8_t Si1145WriteParamData(uint8_t p, uint8_t v)
{
  Si1145WriteByte(SI114X_WR, v);
  Si1145WriteByte(SI114X_COMMAND, p | SI114X_SET);
  return Si1145ReadByte(SI114X_RD);
}



bool Si1145Present()
{
  return (Si1145ReadByte(SI114X_PART_ID) == 0X45);
}

void Si1145Reset()
{
  Si1145WriteByte(SI114X_MEAS_RATE0, 0);
  Si1145WriteByte(SI114X_MEAS_RATE1, 0);
  Si1145WriteByte(SI114X_IRQ_ENABLE, 0);
  Si1145WriteByte(SI114X_IRQ_MODE1, 0);
  Si1145WriteByte(SI114X_IRQ_MODE2, 0);
  Si1145WriteByte(SI114X_INT_CFG, 0);
  Si1145WriteByte(SI114X_IRQ_STATUS, 0xFF);

  Si1145WriteByte(SI114X_COMMAND, SI114X_RESET);
  delay(10);
  Si1145WriteByte(SI114X_HW_KEY, 0x17);
  delay(10);
}

void Si1145DeInit()
{


  Si1145WriteByte(SI114X_UCOEFF0, 0x29);
  Si1145WriteByte(SI114X_UCOEFF1, 0x89);
  Si1145WriteByte(SI114X_UCOEFF2, 0x02);
  Si1145WriteByte(SI114X_UCOEFF3, 0x00);
  Si1145WriteParamData(SI114X_CHLIST, SI114X_CHLIST_ENUV | SI114X_CHLIST_ENALSIR | SI114X_CHLIST_ENALSVIS | SI114X_CHLIST_ENPS1);



  Si1145WriteParamData(SI114X_PS1_ADCMUX, SI114X_ADCMUX_LARGE_IR);
  Si1145WriteByte(SI114X_PS_LED21, SI114X_LED_CURRENT_22MA);
  Si1145WriteParamData(SI114X_PSLED12_SELECT, SI114X_PSLED12_SELECT_PS1_LED1);



  Si1145WriteParamData(SI114X_PS_ADC_GAIN, SI114X_ADC_GAIN_DIV1);
  Si1145WriteParamData(SI114X_PS_ADC_COUNTER, SI114X_ADC_COUNTER_511ADCCLK);
  Si1145WriteParamData(SI114X_PS_ADC_MISC, SI114X_ADC_MISC_HIGHRANGE | SI114X_ADC_MISC_ADC_RAWADC);



  Si1145WriteParamData(SI114X_ALS_VIS_ADC_GAIN, SI114X_ADC_GAIN_DIV1);
  Si1145WriteParamData(SI114X_ALS_VIS_ADC_COUNTER, SI114X_ADC_COUNTER_511ADCCLK);
  Si1145WriteParamData(SI114X_ALS_VIS_ADC_MISC, SI114X_ADC_MISC_HIGHRANGE);



  Si1145WriteParamData(SI114X_ALS_IR_ADC_GAIN, SI114X_ADC_GAIN_DIV1);
  Si1145WriteParamData(SI114X_ALS_IR_ADC_COUNTER, SI114X_ADC_COUNTER_511ADCCLK);
  Si1145WriteParamData(SI114X_ALS_IR_ADC_MISC, SI114X_ADC_MISC_HIGHRANGE);



  Si1145WriteByte(SI114X_INT_CFG, SI114X_INT_CFG_INTOE);
  Si1145WriteByte(SI114X_IRQ_ENABLE, SI114X_IRQEN_ALS);



  Si1145WriteByte(SI114X_MEAS_RATE0, 0xFF);
  Si1145WriteByte(SI114X_COMMAND, SI114X_PSALS_AUTO);
}

boolean Si1145Begin()
{
  if (!Si1145Present()) { return false; }

  Si1145Reset();
  Si1145DeInit();
  return true;
}


uint16_t Si1145ReadUV()
{
  return Si1145ReadHalfWord(SI114X_AUX_DATA0_UVINDEX0);
}


uint16_t Si1145ReadVisible()
{
  return Si1145ReadHalfWord(SI114X_ALS_VIS_DATA0);
}


uint16_t Si1145ReadIR()
{
  return Si1145ReadHalfWord(SI114X_ALS_IR_DATA0);
}



void Si1145Update()
{
  if (!si1145_type) {
    if (Si1145Begin()) {
      si1145_type = 1;
      snprintf_P(log_data, sizeof(log_data), S_LOG_I2C_FOUND_AT, "SI1145", SI114X_ADDR);
      AddLog(LOG_LEVEL_DEBUG);
    }
  }
}

#ifdef USE_WEBSERVER
const char HTTP_SNS_SI1145[] PROGMEM = "%s"
  "{s}SI1145 " D_ILLUMINANCE "{m}%d " D_UNIT_LUX "{e}"
  "{s}SI1145 " D_INFRARED "{m}%d " D_UNIT_LUX "{e}"
  "{s}SI1145 " D_UV_INDEX "{m}%d.%d{e}";
#endif

void Si1145Show(boolean json)
{
  if (si1145_type && Si1145Present()) {
    uint16_t visible = Si1145ReadVisible();
    uint16_t infrared = Si1145ReadIR();
    uint16_t uvindex = Si1145ReadUV();
    if (json) {
      snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s,\"SI1145\":{\"" D_JSON_ILLUMINANCE "\":%d,\"" D_JSON_INFRARED "\":%d,\"" D_JSON_UVINDEX "\":%d.%d}"),
        mqtt_data, visible, infrared, uvindex /100, uvindex %100);
#ifdef USE_DOMOTICZ
      if (0 == tele_period) DomoticzSensor(DZ_ILLUMINANCE, visible);
#endif
#ifdef USE_WEBSERVER
    } else {
      snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_SNS_SI1145, mqtt_data, visible, infrared, uvindex /100, uvindex %100);
#endif
    }
  } else {
    si1145_type = 0;
  }
}





#define XSNS_24 

boolean Xsns24(byte function)
{
  boolean result = false;

  if (i2c_flg) {
    switch (function) {
      case FUNC_EVERY_SECOND:
        Si1145Update();
        break;
      case FUNC_JSON_APPEND:
        Si1145Show(1);
        break;
#ifdef USE_WEBSERVER
      case FUNC_WEB_APPEND:
        Si1145Show(0);
        break;
#endif
    }
  }
  return result;
}

#endif
#endif
# 1 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_25_sdm630.ino"
# 20 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_25_sdm630.ino"
#ifdef USE_SDM630







#include <TasmotaSerial.h>

TasmotaSerial *SDM630Serial;

uint8_t sdm630_type = 1;


float sdm630_voltage[] = {0,0,0};
float sdm630_current[] = {0,0,0};
float sdm630_active_power[] = {0,0,0};
float sdm630_reactive_power[] = {0,0,0};
float sdm630_power_factor[] = {0,0,0};
float sdm630_energy_total = 0;

bool SDM630_ModbusReceiveReady()
{
  return (SDM630Serial->available() > 1);
}

void SDM630_ModbusSend(uint8_t function_code, uint16_t start_address, uint16_t register_count)
{
  uint8_t frame[8];

  frame[0] = 0x01;
  frame[1] = function_code;
  frame[2] = (uint8_t)(start_address >> 8);
  frame[3] = (uint8_t)(start_address);
  frame[4] = (uint8_t)(register_count >> 8);
  frame[5] = (uint8_t)(register_count);

  uint16_t crc = SDM630_calculateCRC(frame, 6);
  frame[6] = lowByte(crc);
  frame[7] = highByte(crc);

  while (SDM630Serial->available() > 0) {
    SDM630Serial->read();
  }

  SDM630Serial->flush();
  SDM630Serial->write(frame, sizeof(frame));
}

uint8_t SDM630_ModbusReceive(float *value)
{
  uint8_t buffer[9];

  *value = NAN;
  uint8_t len = 0;
  while (SDM630Serial->available() > 0) {
    buffer[len++] = (uint8_t)SDM630Serial->read();
  }

  if (len < 9)
    return 3;

  if (len == 9) {

    if (buffer[0] == 0x01 && buffer[1] == 0x04 && buffer[2] == 4) {

      if((SDM630_calculateCRC(buffer, 7)) == ((buffer[8] << 8) | buffer[7])) {

        ((uint8_t*)value)[3] = buffer[3];
        ((uint8_t*)value)[2] = buffer[4];
        ((uint8_t*)value)[1] = buffer[5];
        ((uint8_t*)value)[0] = buffer[6];

      } else return 1;

    } else return 2;
  }

  return 0;
}

uint16_t SDM630_calculateCRC(uint8_t *frame, uint8_t num)
{
  uint16_t crc, flag;
  crc = 0xFFFF;
  for (uint8_t i = 0; i < num; i++) {
    crc ^= frame[i];
    for (uint8_t j = 8; j; j--) {
      if ((crc & 0x0001) != 0) {
        crc >>= 1;
        crc ^= 0xA001;
      } else {
        crc >>= 1;
      }
    }
  }
  return crc;
}



const uint16_t sdm630_start_addresses[] {
  0x0000,
  0x0002,
  0x0004,
  0x0006,
  0x0008,
  0x000A,
  0x000C,
  0x000E,
  0x0010,
  0x0018,
  0x001A,
  0x001C,
  0x001E,
  0x0020,
  0x0022,
  0x0156
};

uint8_t sdm630_read_state = 0;
uint8_t sdm630_send_retry = 0;

void SDM630250ms()
{




    float value = 0;
    bool data_ready = SDM630_ModbusReceiveReady();

    if (data_ready) {
      uint8_t error = SDM630_ModbusReceive(&value);
      if (error) {
        snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_DEBUG "SDM630 response error %d"), error);
        AddLog(LOG_LEVEL_DEBUG);
      } else {
        switch(sdm630_read_state) {
          case 0:
            sdm630_voltage[0] = value;
            break;

          case 1:
            sdm630_voltage[1] = value;
            break;

          case 2:
            sdm630_voltage[2] = value;
            break;

          case 3:
            sdm630_current[0] = value;
            break;

          case 4:
            sdm630_current[1] = value;
            break;

          case 5:
            sdm630_current[2] = value;
            break;

          case 6:
            sdm630_active_power[0] = value;
            break;

          case 7:
            sdm630_active_power[1] = value;
            break;

          case 8:
            sdm630_active_power[2] = value;
            break;

          case 9:
            sdm630_reactive_power[0] = value;
            break;

          case 10:
            sdm630_reactive_power[1] = value;
            break;

          case 11:
            sdm630_reactive_power[2] = value;
            break;

          case 12:
            sdm630_power_factor[0] = value;
            break;

          case 13:
            sdm630_power_factor[1] = value;
            break;

          case 14:
            sdm630_power_factor[2] = value;
            break;

          case 15:
            sdm630_energy_total = value;
            break;
        }

        sdm630_read_state++;

        if (sizeof(sdm630_start_addresses)/2 == sdm630_read_state) {
          sdm630_read_state = 0;
        }
      }
    }

    if (0 == sdm630_send_retry || data_ready) {
      sdm630_send_retry = 5;
       SDM630_ModbusSend(0x04, sdm630_start_addresses[sdm630_read_state], 2);
    } else {
      sdm630_send_retry--;
    }

}

void SDM630Init()
{
  sdm630_type = 0;
  if ((pin[GPIO_SDM630_RX] < 99) && (pin[GPIO_SDM630_TX] < 99)) {
    SDM630Serial = new TasmotaSerial(pin[GPIO_SDM630_RX], pin[GPIO_SDM630_TX], 1);
#ifdef SDM630_SPEED
    if (SDM630Serial->begin(SDM630_SPEED)) {
#else
    if (SDM630Serial->begin(2400)) {
#endif
      if (SDM630Serial->hardwareSerial()) { ClaimSerial(); }
      sdm630_type = 1;
    }
  }
}

#ifdef USE_WEBSERVER
const char HTTP_SNS_SDM630_DATA[] PROGMEM = "%s"
  "{s}SDM630 " D_VOLTAGE "{m}%s/%s/%s " D_UNIT_VOLT "{e}"
  "{s}SDM630 " D_CURRENT "{m}%s/%s/%s " D_UNIT_AMPERE "{e}"
  "{s}SDM630 " D_POWERUSAGE_ACTIVE "{m}%s/%s/%s " D_UNIT_WATT "{e}"
  "{s}SDM630 " D_POWERUSAGE_REACTIVE "{m}%s/%s/%s " D_UNIT_VAR "{e}"
  "{s}SDM630 " D_POWER_FACTOR "{m}%s/%s/%s{e}"
  "{s}SDM630 " D_ENERGY_TOTAL "{m}%s " D_UNIT_KILOWATTHOUR "{e}";
#endif

void SDM630Show(boolean json)
{
  char voltage_l1[10];
  char voltage_l2[10];
  char voltage_l3[10];
  char current_l1[10];
  char current_l2[10];
  char current_l3[10];
  char active_power_l1[10];
  char active_power_l2[10];
  char active_power_l3[10];
  char reactive_power_l1[10];
  char reactive_power_l2[10];
  char reactive_power_l3[10];
  char power_factor_l1[10];
  char power_factor_l2[10];
  char power_factor_l3[10];
  char energy_total[10];

  dtostrfd(sdm630_voltage[0], Settings.flag2.voltage_resolution, voltage_l1);
  dtostrfd(sdm630_voltage[1], Settings.flag2.voltage_resolution, voltage_l2);
  dtostrfd(sdm630_voltage[2], Settings.flag2.voltage_resolution, voltage_l3);
  dtostrfd(sdm630_current[0], Settings.flag2.current_resolution, current_l1);
  dtostrfd(sdm630_current[1], Settings.flag2.current_resolution, current_l2);
  dtostrfd(sdm630_current[2], Settings.flag2.current_resolution, current_l3);
  dtostrfd(sdm630_active_power[0], Settings.flag2.wattage_resolution, active_power_l1);
  dtostrfd(sdm630_active_power[1], Settings.flag2.wattage_resolution, active_power_l2);
  dtostrfd(sdm630_active_power[2], Settings.flag2.wattage_resolution, active_power_l3);
  dtostrfd(sdm630_reactive_power[0], Settings.flag2.wattage_resolution, reactive_power_l1);
  dtostrfd(sdm630_reactive_power[1], Settings.flag2.wattage_resolution, reactive_power_l2);
  dtostrfd(sdm630_reactive_power[2], Settings.flag2.wattage_resolution, reactive_power_l3);
  dtostrfd(sdm630_power_factor[0], 2, power_factor_l1);
  dtostrfd(sdm630_power_factor[1], 2, power_factor_l2);
  dtostrfd(sdm630_power_factor[2], 2, power_factor_l3);
  dtostrfd(sdm630_energy_total, Settings.flag2.energy_resolution, energy_total);

  if (json) {
    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s,\"" D_RSLT_ENERGY "\":{\"" D_JSON_TOTAL "\":%s,\""
      D_JSON_ACTIVE_POWERUSAGE "\":[%s,%s,%s],\"" D_JSON_REACTIVE_POWERUSAGE "\":[%s,%s,%s],\""
      D_JSON_POWERFACTOR "\":[%s,%s,%s],\"" D_JSON_VOLTAGE "\":[%s,%s,%s],\"" D_JSON_CURRENT "\":[%s,%s,%s]}"),
      mqtt_data, energy_total, active_power_l1, active_power_l2, active_power_l3,
      reactive_power_l1, reactive_power_l2, reactive_power_l3,
      power_factor_l1, power_factor_l2, power_factor_l3,
      voltage_l1, voltage_l2, voltage_l3,
      current_l1, current_l2, current_l3);
#ifdef USE_WEBSERVER
  } else {
    snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_SNS_SDM630_DATA, mqtt_data,
    voltage_l1, voltage_l2, voltage_l3, current_l1, current_l2, current_l3,
    active_power_l1, active_power_l2, active_power_l3,
    reactive_power_l1, reactive_power_l2, reactive_power_l3,
    power_factor_l1, power_factor_l2, power_factor_l3, energy_total);
#endif
  }
}





#define XSNS_25 

boolean Xsns25(byte function)
{
  boolean result = false;

  if (sdm630_type) {
    switch (function) {
      case FUNC_INIT:
        SDM630Init();
        break;
      case FUNC_EVERY_250_MSECOND:
        SDM630250ms();
        break;
      case FUNC_JSON_APPEND:
        SDM630Show(1);
        break;
#ifdef USE_WEBSERVER
      case FUNC_WEB_APPEND:
        SDM630Show(0);
        break;
#endif
    }
  }
  return result;
}

#endif
# 1 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_26_lm75ad.ino"
# 20 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_26_lm75ad.ino"
#ifdef USE_I2C
#ifdef USE_LM75AD
# 31 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_26_lm75ad.ino"
#define LM75AD_ADDRESS1 0x48
#define LM75AD_ADDRESS2 0x49
#define LM75AD_ADDRESS3 0x4A
#define LM75AD_ADDRESS4 0x4B
#define LM75AD_ADDRESS5 0x4C
#define LM75AD_ADDRESS6 0x4D
#define LM75AD_ADDRESS7 0x4E
#define LM75AD_ADDRESS8 0x4F

#define LM75_TEMP_REGISTER 0x00
#define LM75_CONF_REGISTER 0x01
#define LM75_THYST_REGISTER 0x02
#define LM75_TOS_REGISTER 0x03

uint8_t lm75ad_type = 0;
uint8_t lm75ad_address;
uint8_t lm75ad_addresses[] = { LM75AD_ADDRESS1, LM75AD_ADDRESS2, LM75AD_ADDRESS3, LM75AD_ADDRESS4, LM75AD_ADDRESS5, LM75AD_ADDRESS6, LM75AD_ADDRESS7, LM75AD_ADDRESS8 };

void LM75ADDetect(void)
{
  if (lm75ad_type) { return; }

  uint16_t buffer;
  for (byte i = 0; i < sizeof(lm75ad_addresses); i++) {
    lm75ad_address = lm75ad_addresses[i];
    if (I2cValidRead16(&buffer, lm75ad_address, LM75_THYST_REGISTER)) {
      if (buffer == 0x4B00) {
        lm75ad_type = 1;
        snprintf_P(log_data, sizeof(log_data), S_LOG_I2C_FOUND_AT, "LM75AD", lm75ad_address);
        AddLog(LOG_LEVEL_DEBUG);
        break;
      }
    }
  }
}

float LM75ADGetTemp(void) {
  int16_t sign = 1;

  uint16_t t = I2cRead16(lm75ad_address, LM75_TEMP_REGISTER);
  if (t & 0x8000) {
    t = (~t) +0x20;
    sign = -1;
  }
  t = t >> 5;
  return ConvertTemp(sign * t * 0.125);
}

void LM75ADShow(boolean json)
{
  if (lm75ad_type) {
    char temperature[10];

    float t = LM75ADGetTemp();
    dtostrfd(t, Settings.flag2.temperature_resolution, temperature);

    if (json) {
      snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s,\"LM75AD\":{\"" D_JSON_TEMPERATURE "\":%s}"), mqtt_data, temperature);
#ifdef USE_DOMOTICZ
      if (0 == tele_period) DomoticzSensor(DZ_TEMP, temperature);
#endif
#ifdef USE_WEBSERVER
    } else {
      snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_SNS_TEMP, mqtt_data, "LM75AD", temperature, TempUnit());
#endif
    }
  }
}





#define XSNS_26 

boolean Xsns26(byte function)
{
  boolean result = false;

  if (i2c_flg) {
    switch (function) {
      case FUNC_EVERY_SECOND:
        LM75ADDetect();
        break;
      case FUNC_JSON_APPEND:
        LM75ADShow(1);
        break;
#ifdef USE_WEBSERVER
      case FUNC_WEB_APPEND:
        LM75ADShow(0);
        break;
#endif
    }
  }
  return result;
}

#endif
#endif
# 1 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_27_apds9960.ino"
# 28 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_27_apds9960.ino"
#ifdef USE_I2C
#ifdef USE_APDS9960

#define XSNS_27 27
# 42 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_27_apds9960.ino"
#if defined(USE_SHT) || defined(USE_VEML6070) || defined(USE_TSL2561)
  #warning **** Turned off conflicting drivers SHT and VEML6070 ****
  #ifdef USE_SHT
  #undef USE_SHT
  #endif
  #ifdef USE_VEML6070
  #undef USE_VEML6070
  #endif
  #ifdef USE_TSL2561
  #undef USE_TSL2561
  #endif
#endif

#define APDS9960_I2C_ADDR 0x39

#define APDS9960_CHIPID_1 0xAB
#define APDS9960_CHIPID_2 0x9C

#define APDS9930_CHIPID_1 0x12
#define APDS9930_CHIPID_2 0x39


#define GESTURE_THRESHOLD_OUT 10
#define GESTURE_SENSITIVITY_1 50
#define GESTURE_SENSITIVITY_2 20

uint8_t APDS9960addr;
uint8_t APDS9960type = 0;
char APDS9960stype[9];
char currentGesture[6];
uint8_t gesture_mode = 1;


volatile uint8_t recovery_loop_counter = 0;
#define APDS9960_LONG_RECOVERY 50
#define APDS9960_MAX_GESTURE_CYCLES 50
bool APDS9960_overload = false;

#ifdef USE_WEBSERVER
const char HTTP_APDS_9960_SNS[] PROGMEM = "%s"
  "{s}" "Red" "{m}%s{e}"
  "{s}" "Green" "{m}%s{e}"
  "{s}" "Blue" "{m}%s{e}"
  "{s}" "Ambient" "{m}%s " D_UNIT_LUX "{e}"
  "{s}" "CCT" "{m}%s " "K" "{e}"
  "{s}" "Proximity" "{m}%s{e}";
#endif
# 97 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_27_apds9960.ino"
#define FIFO_PAUSE_TIME 30


#define APDS9960_ENABLE 0x80
#define APDS9960_ATIME 0x81
#define APDS9960_WTIME 0x83
#define APDS9960_AILTL 0x84
#define APDS9960_AILTH 0x85
#define APDS9960_AIHTL 0x86
#define APDS9960_AIHTH 0x87
#define APDS9960_PILT 0x89
#define APDS9960_PIHT 0x8B
#define APDS9960_PERS 0x8C
#define APDS9960_CONFIG1 0x8D
#define APDS9960_PPULSE 0x8E
#define APDS9960_CONTROL 0x8F
#define APDS9960_CONFIG2 0x90
#define APDS9960_ID 0x92
#define APDS9960_STATUS 0x93
#define APDS9960_CDATAL 0x94
#define APDS9960_CDATAH 0x95
#define APDS9960_RDATAL 0x96
#define APDS9960_RDATAH 0x97
#define APDS9960_GDATAL 0x98
#define APDS9960_GDATAH 0x99
#define APDS9960_BDATAL 0x9A
#define APDS9960_BDATAH 0x9B
#define APDS9960_PDATA 0x9C
#define APDS9960_POFFSET_UR 0x9D
#define APDS9960_POFFSET_DL 0x9E
#define APDS9960_CONFIG3 0x9F
#define APDS9960_GPENTH 0xA0
#define APDS9960_GEXTH 0xA1
#define APDS9960_GCONF1 0xA2
#define APDS9960_GCONF2 0xA3
#define APDS9960_GOFFSET_U 0xA4
#define APDS9960_GOFFSET_D 0xA5
#define APDS9960_GOFFSET_L 0xA7
#define APDS9960_GOFFSET_R 0xA9
#define APDS9960_GPULSE 0xA6
#define APDS9960_GCONF3 0xAA
#define APDS9960_GCONF4 0xAB
#define APDS9960_GFLVL 0xAE
#define APDS9960_GSTATUS 0xAF
#define APDS9960_IFORCE 0xE4
#define APDS9960_PICLEAR 0xE5
#define APDS9960_CICLEAR 0xE6
#define APDS9960_AICLEAR 0xE7
#define APDS9960_GFIFO_U 0xFC
#define APDS9960_GFIFO_D 0xFD
#define APDS9960_GFIFO_L 0xFE
#define APDS9960_GFIFO_R 0xFF


#define APDS9960_PON 0b00000001
#define APDS9960_AEN 0b00000010
#define APDS9960_PEN 0b00000100
#define APDS9960_WEN 0b00001000
#define APSD9960_AIEN 0b00010000
#define APDS9960_PIEN 0b00100000
#define APDS9960_GEN 0b01000000
#define APDS9960_GVALID 0b00000001


#define OFF 0
#define ON 1


#define POWER 0
#define AMBIENT_LIGHT 1
#define PROXIMITY 2
#define WAIT 3
#define AMBIENT_LIGHT_INT 4
#define PROXIMITY_INT 5
#define GESTURE 6
#define ALL 7


#define LED_DRIVE_100MA 0
#define LED_DRIVE_50MA 1
#define LED_DRIVE_25MA 2
#define LED_DRIVE_12_5MA 3


#define PGAIN_1X 0
#define PGAIN_2X 1
#define PGAIN_4X 2
#define PGAIN_8X 3


#define AGAIN_1X 0
#define AGAIN_4X 1
#define AGAIN_16X 2
#define AGAIN_64X 3


#define GGAIN_1X 0
#define GGAIN_2X 1
#define GGAIN_4X 2
#define GGAIN_8X 3


#define LED_BOOST_100 0
#define LED_BOOST_150 1
#define LED_BOOST_200 2
#define LED_BOOST_300 3


#define GWTIME_0MS 0
#define GWTIME_2_8MS 1
#define GWTIME_5_6MS 2
#define GWTIME_8_4MS 3
#define GWTIME_14_0MS 4
#define GWTIME_22_4MS 5
#define GWTIME_30_8MS 6
#define GWTIME_39_2MS 7


#define DEFAULT_ATIME 0xdb
#define DEFAULT_WTIME 246
#define DEFAULT_PROX_PPULSE 0x87
#define DEFAULT_GESTURE_PPULSE 0x89
#define DEFAULT_POFFSET_UR 0
#define DEFAULT_POFFSET_DL 0
#define DEFAULT_CONFIG1 0x60
#define DEFAULT_LDRIVE LED_DRIVE_100MA
#define DEFAULT_PGAIN PGAIN_4X
#define DEFAULT_AGAIN AGAIN_4X
#define DEFAULT_PILT 0
#define DEFAULT_PIHT 50
#define DEFAULT_AILT 0xFFFF
#define DEFAULT_AIHT 0
#define DEFAULT_PERS 0x11
#define DEFAULT_CONFIG2 0x01
#define DEFAULT_CONFIG3 0
#define DEFAULT_GPENTH 40
#define DEFAULT_GEXTH 30
#define DEFAULT_GCONF1 0x40
#define DEFAULT_GGAIN GGAIN_4X
#define DEFAULT_GLDRIVE LED_DRIVE_100MA
#define DEFAULT_GWTIME GWTIME_2_8MS
#define DEFAULT_GOFFSET 0
#define DEFAULT_GPULSE 0xC9
#define DEFAULT_GCONF3 0
#define DEFAULT_GIEN 0

#define ERROR 0xFF


enum {
  DIR_NONE,
  DIR_LEFT,
  DIR_RIGHT,
  DIR_UP,
  DIR_DOWN,
  DIR_ALL
};


enum {
  APDS9960_NA_STATE,
  APDS9960_ALL_STATE
};


typedef struct gesture_data_type {
    uint8_t u_data[32];
    uint8_t d_data[32];
    uint8_t l_data[32];
    uint8_t r_data[32];
    uint8_t index;
    uint8_t total_gestures;
    uint8_t in_threshold;
    uint8_t out_threshold;
} gesture_data_type;


 gesture_data_type gesture_data_;
 int16_t gesture_ud_delta_ = 0;
 int16_t gesture_lr_delta_ = 0;
 int16_t gesture_ud_count_ = 0;
 int16_t gesture_lr_count_ = 0;
 int16_t gesture_state_ = 0;
 int16_t gesture_motion_ = DIR_NONE;

 typedef struct color_data_type {
    uint16_t a;
    uint16_t r;
    uint16_t g;
    uint16_t b;
    uint8_t p;
    uint16_t cct;
    uint16_t lux;
 } color_data_type;

 color_data_type color_data;
 uint8_t APDS9960_aTime = DEFAULT_ATIME;
# 306 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_27_apds9960.ino"
 bool wireWriteByte(uint8_t val)
 {
     Wire.beginTransmission(APDS9960_I2C_ADDR);
     Wire.write(val);
     if( Wire.endTransmission() != 0 ) {
         return false;
     }

     return true;
 }
# 325 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_27_apds9960.ino"
int8_t wireReadDataBlock( uint8_t reg,
                                        uint8_t *val,
                                        uint16_t len)
{
    unsigned char i = 0;


    if (!wireWriteByte(reg)) {
        return -1;
    }


    Wire.requestFrom(APDS9960_I2C_ADDR, len);
    while (Wire.available()) {
        if (i >= len) {
            return -1;
        }
        val[i] = Wire.read();
        i++;
    }

    return i;
}







void calculateColorTemperature()
{
  float X, Y, Z;
  float xc, yc;
  float n;
  float cct;





  X = (-0.14282F * color_data.r) + (1.54924F * color_data.g) + (-0.95641F * color_data.b);
  Y = (-0.32466F * color_data.r) + (1.57837F * color_data.g) + (-0.73191F * color_data.b);
  Z = (-0.68202F * color_data.r) + (0.77073F * color_data.g) + ( 0.56332F * color_data.b);


  xc = (X) / (X + Y + Z);
  yc = (Y) / (X + Y + Z);


  n = (xc - 0.3320F) / (0.1858F - yc);


  color_data.cct = (449.0F * powf(n, 3)) + (3525.0F * powf(n, 2)) + (6823.3F * n) + 5520.33F;

  return;
}






float powf(const float x, const float y)
{
  return (float)(pow((double)x, (double)y));
}
# 402 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_27_apds9960.ino"
 uint8_t getProxIntLowThresh()
 {
     uint8_t val;


     val = I2cRead8(APDS9960_I2C_ADDR, APDS9960_PILT) ;
     return val;
 }






  void setProxIntLowThresh(uint8_t threshold)
  {
        I2cWrite8(APDS9960_I2C_ADDR, APDS9960_PILT, threshold);
  }






 uint8_t getProxIntHighThresh()
 {
     uint8_t val;


     val = I2cRead8(APDS9960_I2C_ADDR, APDS9960_PIHT) ;
     return val;
 }







 void setProxIntHighThresh(uint8_t threshold)
   {
         I2cWrite8(APDS9960_I2C_ADDR, APDS9960_PIHT, threshold);
   }
# 458 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_27_apds9960.ino"
 uint8_t getLEDDrive()
 {
     uint8_t val;


     val = I2cRead8(APDS9960_I2C_ADDR, APDS9960_CONTROL) ;

     val = (val >> 6) & 0b00000011;

     return val;
 }
# 481 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_27_apds9960.ino"
  void setLEDDrive(uint8_t drive)
  {
     uint8_t val;


     val = I2cRead8(APDS9960_I2C_ADDR, APDS9960_CONTROL);


      drive &= 0b00000011;
      drive = drive << 6;
      val &= 0b00111111;
      val |= drive;


      I2cWrite8(APDS9960_I2C_ADDR, APDS9960_CONTROL, val);
  }
# 510 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_27_apds9960.ino"
 uint8_t getProximityGain()
 {
     uint8_t val;


     val = I2cRead8(APDS9960_I2C_ADDR, APDS9960_CONTROL) ;

     val = (val >> 2) & 0b00000011;

     return val;
 }
# 533 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_27_apds9960.ino"
  void setProximityGain(uint8_t drive)
 {
     uint8_t val;


   val = I2cRead8(APDS9960_I2C_ADDR, APDS9960_CONTROL);


     drive &= 0b00000011;
     drive = drive << 2;
     val &= 0b11110011;
     val |= drive;


     I2cWrite8(APDS9960_I2C_ADDR, APDS9960_CONTROL, val);
 }
# 574 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_27_apds9960.ino"
  void setAmbientLightGain(uint8_t drive)
  {
      uint8_t val;


      val = I2cRead8(APDS9960_I2C_ADDR, APDS9960_CONTROL);


      drive &= 0b00000011;
      val &= 0b11111100;
      val |= drive;


      I2cWrite8(APDS9960_I2C_ADDR, APDS9960_CONTROL, val);
  }
# 601 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_27_apds9960.ino"
 uint8_t getLEDBoost()
 {
     uint8_t val;


   val = I2cRead8(APDS9960_I2C_ADDR, APDS9960_CONFIG2) ;


     val = (val >> 4) & 0b00000011;

     return val;
 }
# 625 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_27_apds9960.ino"
 void setLEDBoost(uint8_t boost)
 {
     uint8_t val;


     val = I2cRead8(APDS9960_I2C_ADDR, APDS9960_CONFIG2) ;

     boost &= 0b00000011;
     boost = boost << 4;
     val &= 0b11001111;
     val |= boost;


    I2cWrite8(APDS9960_I2C_ADDR, APDS9960_CONFIG2, val) ;
 }






 uint8_t getProxGainCompEnable()
 {
     uint8_t val;


     val = I2cRead8(APDS9960_I2C_ADDR, APDS9960_CONFIG3) ;


     val = (val >> 5) & 0b00000001;

     return val;
 }






  void setProxGainCompEnable(uint8_t enable)
 {
     uint8_t val;


     val = I2cRead8(APDS9960_I2C_ADDR, APDS9960_CONFIG3) ;


     enable &= 0b00000001;
     enable = enable << 5;
     val &= 0b11011111;
     val |= enable;


     I2cWrite8(APDS9960_I2C_ADDR, APDS9960_CONFIG3, val) ;
 }
# 693 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_27_apds9960.ino"
 uint8_t getProxPhotoMask()
 {
     uint8_t val;


     val = I2cRead8(APDS9960_I2C_ADDR, APDS9960_CONFIG3) ;


     val &= 0b00001111;

     return val;
 }
# 718 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_27_apds9960.ino"
 void setProxPhotoMask(uint8_t mask)
 {
     uint8_t val;


     val = I2cRead8(APDS9960_I2C_ADDR, APDS9960_CONFIG3) ;


     mask &= 0b00001111;
     val &= 0b11110000;
     val |= mask;


     I2cWrite8(APDS9960_I2C_ADDR, APDS9960_CONFIG3, val) ;
 }






 uint8_t getGestureEnterThresh()
 {
     uint8_t val;


     val = I2cRead8(APDS9960_I2C_ADDR, APDS9960_GPENTH) ;

     return val;
 }






 void setGestureEnterThresh(uint8_t threshold)
 {
    I2cWrite8(APDS9960_I2C_ADDR, APDS9960_GPENTH, threshold) ;

 }






 uint8_t getGestureExitThresh()
 {
     uint8_t val;


     val = I2cRead8(APDS9960_I2C_ADDR, APDS9960_GEXTH) ;

     return val;
 }






 void setGestureExitThresh(uint8_t threshold)
 {
     I2cWrite8(APDS9960_I2C_ADDR, APDS9960_GEXTH, threshold) ;
 }
# 796 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_27_apds9960.ino"
 uint8_t getGestureGain()
 {
     uint8_t val;


     val = I2cRead8(APDS9960_I2C_ADDR, APDS9960_GCONF2) ;


     val = (val >> 5) & 0b00000011;

     return val;
 }
# 820 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_27_apds9960.ino"
 void setGestureGain(uint8_t gain)
 {
     uint8_t val;


     val = I2cRead8(APDS9960_I2C_ADDR, APDS9960_GCONF2) ;


     gain &= 0b00000011;
     gain = gain << 5;
     val &= 0b10011111;
     val |= gain;


     I2cWrite8(APDS9960_I2C_ADDR, APDS9960_GCONF2, val) ;
 }
# 848 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_27_apds9960.ino"
 uint8_t getGestureLEDDrive()
 {
     uint8_t val;


     val = I2cRead8(APDS9960_I2C_ADDR, APDS9960_GCONF2) ;


     val = (val >> 3) & 0b00000011;

     return val;
 }
# 872 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_27_apds9960.ino"
 void setGestureLEDDrive(uint8_t drive)
 {
     uint8_t val;


     val = I2cRead8(APDS9960_I2C_ADDR, APDS9960_GCONF2) ;


     drive &= 0b00000011;
     drive = drive << 3;
     val &= 0b11100111;
     val |= drive;


     I2cWrite8(APDS9960_I2C_ADDR, APDS9960_GCONF2, val) ;
 }
# 904 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_27_apds9960.ino"
 uint8_t getGestureWaitTime()
 {
     uint8_t val;


     val = I2cRead8(APDS9960_I2C_ADDR, APDS9960_GCONF2) ;


     val &= 0b00000111;

     return val;
 }
# 932 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_27_apds9960.ino"
 void setGestureWaitTime(uint8_t time)
 {
     uint8_t val;


     val = I2cRead8(APDS9960_I2C_ADDR, APDS9960_GCONF2) ;


     time &= 0b00000111;
     val &= 0b11111000;
     val |= time;


    I2cWrite8(APDS9960_I2C_ADDR, APDS9960_GCONF2, val) ;
 }






 void getLightIntLowThreshold(uint16_t &threshold)
 {
     uint8_t val_byte;
     threshold = 0;


     val_byte = I2cRead8(APDS9960_I2C_ADDR, APDS9960_AILTL) ;
     threshold = val_byte;


     I2cWrite8(APDS9960_I2C_ADDR, APDS9960_AILTH, val_byte) ;
     threshold = threshold + ((uint16_t)val_byte << 8);
 }







   void setLightIntLowThreshold(uint16_t threshold)
   {
       uint8_t val_low;
       uint8_t val_high;


       val_low = threshold & 0x00FF;
       val_high = (threshold & 0xFF00) >> 8;


       I2cWrite8(APDS9960_I2C_ADDR, APDS9960_AILTL, val_low) ;


       I2cWrite8(APDS9960_I2C_ADDR, APDS9960_AILTH, val_high) ;

   }







 void getLightIntHighThreshold(uint16_t &threshold)
 {
     uint8_t val_byte;
     threshold = 0;


     val_byte = I2cRead8(APDS9960_I2C_ADDR, APDS9960_AIHTL);
     threshold = val_byte;


     I2cWrite8(APDS9960_I2C_ADDR, APDS9960_AIHTH, val_byte) ;
     threshold = threshold + ((uint16_t)val_byte << 8);
 }






  void setLightIntHighThreshold(uint16_t threshold)
  {
      uint8_t val_low;
      uint8_t val_high;


      val_low = threshold & 0x00FF;
      val_high = (threshold & 0xFF00) >> 8;


      I2cWrite8(APDS9960_I2C_ADDR, APDS9960_AIHTL, val_low);


      I2cWrite8(APDS9960_I2C_ADDR, APDS9960_AIHTH, val_high) ;
  }







 void getProximityIntLowThreshold(uint8_t &threshold)
 {
     threshold = 0;


     threshold = I2cRead8(APDS9960_I2C_ADDR, APDS9960_PILT);

 }






 void setProximityIntLowThreshold(uint8_t threshold)
 {


     I2cWrite8(APDS9960_I2C_ADDR, APDS9960_PILT, threshold) ;
 }
# 1065 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_27_apds9960.ino"
 void getProximityIntHighThreshold(uint8_t &threshold)
 {
     threshold = 0;


     threshold = I2cRead8(APDS9960_I2C_ADDR, APDS9960_PIHT) ;

 }






 void setProximityIntHighThreshold(uint8_t threshold)
 {


   I2cWrite8(APDS9960_I2C_ADDR, APDS9960_PIHT, threshold) ;
 }






 uint8_t getAmbientLightIntEnable()
 {
     uint8_t val;


     val = I2cRead8(APDS9960_I2C_ADDR, APDS9960_ENABLE) ;


     val = (val >> 4) & 0b00000001;

     return val;
 }






 void setAmbientLightIntEnable(uint8_t enable)
 {
     uint8_t val;


     val = I2cRead8(APDS9960_I2C_ADDR, APDS9960_ENABLE);


     enable &= 0b00000001;
     enable = enable << 4;
     val &= 0b11101111;
     val |= enable;


     I2cWrite8(APDS9960_I2C_ADDR, APDS9960_ENABLE, val) ;
 }






 uint8_t getProximityIntEnable()
 {
     uint8_t val;


     val = I2cRead8(APDS9960_I2C_ADDR, APDS9960_ENABLE) ;


     val = (val >> 5) & 0b00000001;

     return val;
 }






 void setProximityIntEnable(uint8_t enable)
 {
     uint8_t val;


     val = I2cRead8(APDS9960_I2C_ADDR, APDS9960_ENABLE) ;


     enable &= 0b00000001;
     enable = enable << 5;
     val &= 0b11011111;
     val |= enable;


     I2cWrite8(APDS9960_I2C_ADDR, APDS9960_ENABLE, val) ;
 }






 uint8_t getGestureIntEnable()
 {
     uint8_t val;


     val = I2cRead8(APDS9960_I2C_ADDR, APDS9960_GCONF4) ;


     val = (val >> 1) & 0b00000001;

     return val;
 }






 void setGestureIntEnable(uint8_t enable)
 {
     uint8_t val;


     val = I2cRead8(APDS9960_I2C_ADDR, APDS9960_GCONF4) ;


     enable &= 0b00000001;
     enable = enable << 1;
     val &= 0b11111101;
     val |= enable;


     I2cWrite8(APDS9960_I2C_ADDR, APDS9960_GCONF4, val) ;
 }





 void clearAmbientLightInt()
 {
     uint8_t throwaway;
     throwaway = I2cRead8(APDS9960_I2C_ADDR, APDS9960_AICLEAR);
 }





 void clearProximityInt()
 {
     uint8_t throwaway;
     throwaway = I2cRead8(APDS9960_I2C_ADDR, APDS9960_PICLEAR) ;

 }






 uint8_t getGestureMode()
 {
     uint8_t val;


     val = I2cRead8(APDS9960_I2C_ADDR, APDS9960_GCONF4) ;


     val &= 0b00000001;

     return val;
 }






 void setGestureMode(uint8_t mode)
 {
     uint8_t val;


     val = I2cRead8(APDS9960_I2C_ADDR, APDS9960_GCONF4) ;


     mode &= 0b00000001;
     val &= 0b11111110;
     val |= mode;


     I2cWrite8(APDS9960_I2C_ADDR, APDS9960_GCONF4, val) ;
 }


bool APDS9960_init()
{


    I2cWrite8(APDS9960_I2C_ADDR, APDS9960_ATIME, DEFAULT_ATIME) ;

    I2cWrite8(APDS9960_I2C_ADDR, APDS9960_WTIME, DEFAULT_WTIME) ;

    I2cWrite8(APDS9960_I2C_ADDR, APDS9960_PPULSE, DEFAULT_PROX_PPULSE) ;

    I2cWrite8(APDS9960_I2C_ADDR, APDS9960_POFFSET_UR, DEFAULT_POFFSET_UR) ;

    I2cWrite8(APDS9960_I2C_ADDR, APDS9960_POFFSET_DL, DEFAULT_POFFSET_DL) ;

    I2cWrite8(APDS9960_I2C_ADDR, APDS9960_CONFIG1, DEFAULT_CONFIG1) ;

    setLEDDrive(DEFAULT_LDRIVE);

    setProximityGain(DEFAULT_PGAIN);

    setAmbientLightGain(DEFAULT_AGAIN);

    setProxIntLowThresh(DEFAULT_PILT) ;

    setProxIntHighThresh(DEFAULT_PIHT);

    setLightIntLowThreshold(DEFAULT_AILT) ;

    setLightIntHighThreshold(DEFAULT_AIHT) ;

    I2cWrite8(APDS9960_I2C_ADDR, APDS9960_PERS, DEFAULT_PERS) ;

    I2cWrite8(APDS9960_I2C_ADDR, APDS9960_CONFIG2, DEFAULT_CONFIG2) ;

    I2cWrite8(APDS9960_I2C_ADDR, APDS9960_CONFIG3, DEFAULT_CONFIG3) ;


    setGestureEnterThresh(DEFAULT_GPENTH);

    setGestureExitThresh(DEFAULT_GEXTH) ;

    I2cWrite8(APDS9960_I2C_ADDR, APDS9960_GCONF1, DEFAULT_GCONF1) ;

    setGestureGain(DEFAULT_GGAIN) ;

    setGestureLEDDrive(DEFAULT_GLDRIVE) ;

    setGestureWaitTime(DEFAULT_GWTIME) ;

    I2cWrite8(APDS9960_I2C_ADDR, APDS9960_GOFFSET_U, DEFAULT_GOFFSET) ;

    I2cWrite8(APDS9960_I2C_ADDR, APDS9960_GOFFSET_D, DEFAULT_GOFFSET) ;

    I2cWrite8(APDS9960_I2C_ADDR, APDS9960_GOFFSET_L, DEFAULT_GOFFSET) ;

    I2cWrite8(APDS9960_I2C_ADDR, APDS9960_GOFFSET_R, DEFAULT_GOFFSET) ;

    I2cWrite8(APDS9960_I2C_ADDR, APDS9960_GPULSE, DEFAULT_GPULSE) ;

    I2cWrite8(APDS9960_I2C_ADDR, APDS9960_GCONF3, DEFAULT_GCONF3) ;

    setGestureIntEnable(DEFAULT_GIEN);

    disablePower();

    return true;
}
# 1343 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_27_apds9960.ino"
uint8_t getMode()
{
    uint8_t enable_value;


     enable_value = I2cRead8(APDS9960_I2C_ADDR, APDS9960_ENABLE) ;

    return enable_value;
}







void setMode(uint8_t mode, uint8_t enable)
{
    uint8_t reg_val;


    reg_val = getMode();



    enable = enable & 0x01;
    if( mode >= 0 && mode <= 6 ) {
        if (enable) {
            reg_val |= (1 << mode);
        } else {
            reg_val &= ~(1 << mode);
        }
    } else if( mode == ALL ) {
        if (enable) {
            reg_val = 0x7F;
        } else {
            reg_val = 0x00;
        }
    }


      I2cWrite8(APDS9960_I2C_ADDR, APDS9960_ENABLE, reg_val) ;
}






void enableLightSensor()
{

    setAmbientLightGain(DEFAULT_AGAIN);
    setAmbientLightIntEnable(0);
    enablePower() ;
    setMode(AMBIENT_LIGHT, 1) ;
}





void disableLightSensor()
{
    setAmbientLightIntEnable(0) ;
    setMode(AMBIENT_LIGHT, 0) ;
}






void enableProximitySensor()
{

    setProximityGain(DEFAULT_PGAIN);
    setLEDDrive(DEFAULT_LDRIVE) ;
    setProximityIntEnable(0) ;
    enablePower();
    setMode(PROXIMITY, 1) ;
}





void disableProximitySensor()
{
 setProximityIntEnable(0) ;
  setMode(PROXIMITY, 0) ;
}






void enableGestureSensor()
{







    resetGestureParameters();
    I2cWrite8(APDS9960_I2C_ADDR, APDS9960_WTIME, 0xFF) ;
    I2cWrite8(APDS9960_I2C_ADDR, APDS9960_PPULSE, DEFAULT_GESTURE_PPULSE) ;
    setLEDBoost(LED_BOOST_100);
    setGestureIntEnable(0) ;
    setGestureMode(1);
    enablePower() ;
    setMode(WAIT, 1) ;
    setMode(PROXIMITY, 1) ;
    setMode(GESTURE, 1);
}





void disableGestureSensor()
{
    resetGestureParameters();
    setGestureIntEnable(0) ;
    setGestureMode(0) ;
    setMode(GESTURE, 0) ;
}






bool isGestureAvailable()
{
    uint8_t val;


     val = I2cRead8(APDS9960_I2C_ADDR, APDS9960_GSTATUS) ;


    val &= APDS9960_GVALID;


    if( val == 1) {
        return true;
    } else {
        return false;
    }
}






int16_t readGesture()
{
    uint8_t fifo_level = 0;
    uint8_t bytes_read = 0;
    uint8_t fifo_data[128];
    uint8_t gstatus;
    uint16_t motion;
    uint16_t i;
    uint8_t gesture_loop_counter = 0;


    if( !isGestureAvailable() || !(getMode() & 0b01000001) ) {
        return DIR_NONE;
    }


    while(1) {
        if (gesture_loop_counter == APDS9960_MAX_GESTURE_CYCLES){
          disableGestureSensor();
          APDS9960_overload = true;
          char log[LOGSZ];
          snprintf_P(log, sizeof(log), PSTR("Sensor overload"));
          AddLog_P(LOG_LEVEL_DEBUG, log);
        }
        gesture_loop_counter += 1;

        delay(FIFO_PAUSE_TIME);


       gstatus = I2cRead8(APDS9960_I2C_ADDR, APDS9960_GSTATUS);


        if( (gstatus & APDS9960_GVALID) == APDS9960_GVALID ) {


            fifo_level = I2cRead8(APDS9960_I2C_ADDR,APDS9960_GFLVL) ;


            if( fifo_level > 0) {
                bytes_read = wireReadDataBlock( APDS9960_GFIFO_U,
                                                (uint8_t*)fifo_data,
                                                (fifo_level * 4) );
                if( bytes_read == -1 ) {
                    return ERROR;
                }


                if( bytes_read >= 4 ) {
                    for( i = 0; i < bytes_read; i += 4 ) {
                        gesture_data_.u_data[gesture_data_.index] = \
                                                            fifo_data[i + 0];
                        gesture_data_.d_data[gesture_data_.index] = \
                                                            fifo_data[i + 1];
                        gesture_data_.l_data[gesture_data_.index] = \
                                                            fifo_data[i + 2];
                        gesture_data_.r_data[gesture_data_.index] = \
                                                            fifo_data[i + 3];
                        gesture_data_.index++;
                        gesture_data_.total_gestures++;
                    }

                    if( processGestureData() ) {
                        if( decodeGesture() ) {

                        }
                    }

                    gesture_data_.index = 0;
                    gesture_data_.total_gestures = 0;
                }
            }
        } else {


            delay(FIFO_PAUSE_TIME);
            decodeGesture();
            motion = gesture_motion_;
            resetGestureParameters();
            return motion;
        }
    }
}





void enablePower()
{
    setMode(POWER, 1) ;
}





void disablePower()
{
    setMode(POWER, 0) ;
}
# 1612 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_27_apds9960.ino"
void readAllColorAndProximityData()
{
  if (I2cReadBuffer(APDS9960_I2C_ADDR, APDS9960_CDATAL, (uint8_t *) &color_data, (uint16_t)9))
  {


  }
}
# 1628 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_27_apds9960.ino"
void resetGestureParameters()
{
    gesture_data_.index = 0;
    gesture_data_.total_gestures = 0;

    gesture_ud_delta_ = 0;
    gesture_lr_delta_ = 0;

    gesture_ud_count_ = 0;
    gesture_lr_count_ = 0;

    gesture_state_ = 0;
    gesture_motion_ = DIR_NONE;
}






bool processGestureData()
{
    uint8_t u_first = 0;
    uint8_t d_first = 0;
    uint8_t l_first = 0;
    uint8_t r_first = 0;
    uint8_t u_last = 0;
    uint8_t d_last = 0;
    uint8_t l_last = 0;
    uint8_t r_last = 0;
    uint16_t ud_ratio_first;
    uint16_t lr_ratio_first;
    uint16_t ud_ratio_last;
    uint16_t lr_ratio_last;
    uint16_t ud_delta;
    uint16_t lr_delta;
    uint16_t i;


    if( gesture_data_.total_gestures <= 4 ) {
        return false;
    }


    if( (gesture_data_.total_gestures <= 32) && \
        (gesture_data_.total_gestures > 0) ) {


        for( i = 0; i < gesture_data_.total_gestures; i++ ) {
            if( (gesture_data_.u_data[i] > GESTURE_THRESHOLD_OUT) &&
                (gesture_data_.d_data[i] > GESTURE_THRESHOLD_OUT) &&
                (gesture_data_.l_data[i] > GESTURE_THRESHOLD_OUT) &&
                (gesture_data_.r_data[i] > GESTURE_THRESHOLD_OUT) ) {

                u_first = gesture_data_.u_data[i];
                d_first = gesture_data_.d_data[i];
                l_first = gesture_data_.l_data[i];
                r_first = gesture_data_.r_data[i];
                break;
            }
        }


        if( (u_first == 0) || (d_first == 0) || \
            (l_first == 0) || (r_first == 0) ) {

            return false;
        }

        for( i = gesture_data_.total_gestures - 1; i >= 0; i-- ) {

            if( (gesture_data_.u_data[i] > GESTURE_THRESHOLD_OUT) &&
                (gesture_data_.d_data[i] > GESTURE_THRESHOLD_OUT) &&
                (gesture_data_.l_data[i] > GESTURE_THRESHOLD_OUT) &&
                (gesture_data_.r_data[i] > GESTURE_THRESHOLD_OUT) ) {

                u_last = gesture_data_.u_data[i];
                d_last = gesture_data_.d_data[i];
                l_last = gesture_data_.l_data[i];
                r_last = gesture_data_.r_data[i];
                break;
            }
        }
    }


    ud_ratio_first = ((u_first - d_first) * 100) / (u_first + d_first);
    lr_ratio_first = ((l_first - r_first) * 100) / (l_first + r_first);
    ud_ratio_last = ((u_last - d_last) * 100) / (u_last + d_last);
    lr_ratio_last = ((l_last - r_last) * 100) / (l_last + r_last);


    ud_delta = ud_ratio_last - ud_ratio_first;
    lr_delta = lr_ratio_last - lr_ratio_first;


    gesture_ud_delta_ += ud_delta;
    gesture_lr_delta_ += lr_delta;


    if( gesture_ud_delta_ >= GESTURE_SENSITIVITY_1 ) {
        gesture_ud_count_ = 1;
    } else if( gesture_ud_delta_ <= -GESTURE_SENSITIVITY_1 ) {
        gesture_ud_count_ = -1;
    } else {
        gesture_ud_count_ = 0;
    }


    if( gesture_lr_delta_ >= GESTURE_SENSITIVITY_1 ) {
        gesture_lr_count_ = 1;
    } else if( gesture_lr_delta_ <= -GESTURE_SENSITIVITY_1 ) {
        gesture_lr_count_ = -1;
    } else {
        gesture_lr_count_ = 0;
    }
    return false;
}






bool decodeGesture()
{


    if( (gesture_ud_count_ == -1) && (gesture_lr_count_ == 0) ) {
        gesture_motion_ = DIR_UP;
    } else if( (gesture_ud_count_ == 1) && (gesture_lr_count_ == 0) ) {
        gesture_motion_ = DIR_DOWN;
    } else if( (gesture_ud_count_ == 0) && (gesture_lr_count_ == 1) ) {
        gesture_motion_ = DIR_RIGHT;
    } else if( (gesture_ud_count_ == 0) && (gesture_lr_count_ == -1) ) {
        gesture_motion_ = DIR_LEFT;
    } else if( (gesture_ud_count_ == -1) && (gesture_lr_count_ == 1) ) {
        if( abs(gesture_ud_delta_) > abs(gesture_lr_delta_) ) {
            gesture_motion_ = DIR_UP;
        } else {
            gesture_motion_ = DIR_RIGHT;
        }
    } else if( (gesture_ud_count_ == 1) && (gesture_lr_count_ == -1) ) {
        if( abs(gesture_ud_delta_) > abs(gesture_lr_delta_) ) {
            gesture_motion_ = DIR_DOWN;
        } else {
            gesture_motion_ = DIR_LEFT;
        }
    } else if( (gesture_ud_count_ == -1) && (gesture_lr_count_ == -1) ) {
        if( abs(gesture_ud_delta_) > abs(gesture_lr_delta_) ) {
            gesture_motion_ = DIR_UP;
        } else {
            gesture_motion_ = DIR_LEFT;
        }
    } else if( (gesture_ud_count_ == 1) && (gesture_lr_count_ == 1) ) {
        if( abs(gesture_ud_delta_) > abs(gesture_lr_delta_) ) {
            gesture_motion_ = DIR_DOWN;
        } else {
            gesture_motion_ = DIR_RIGHT;
        }
    } else {
        return false;
    }

    return true;
}

void handleGesture() {
    if (isGestureAvailable() ) {
    char log[LOGSZ];
    switch (readGesture()) {
      case DIR_UP:
        snprintf_P(log, sizeof(log), PSTR("UP"));
        snprintf_P(currentGesture, sizeof(currentGesture), PSTR("Up"));
        break;
      case DIR_DOWN:
        snprintf_P(log, sizeof(log), PSTR("DOWN"));
        snprintf_P(currentGesture, sizeof(currentGesture), PSTR("Down"));
        break;
      case DIR_LEFT:
        snprintf_P(log, sizeof(log), PSTR("LEFT"));
        snprintf_P(currentGesture, sizeof(currentGesture), PSTR("Left"));
        break;
      case DIR_RIGHT:
        snprintf_P(log, sizeof(log), PSTR("RIGHT"));
        snprintf_P(currentGesture, sizeof(currentGesture), PSTR("Right"));
        break;
      default:
      if(APDS9960_overload)
      {
        snprintf_P(log, sizeof(log), PSTR("LONG"));
        snprintf_P(currentGesture, sizeof(currentGesture), PSTR("Long"));
      }
      else{
        snprintf_P(log, sizeof(log), PSTR("NONE"));
        snprintf_P(currentGesture, sizeof(currentGesture), PSTR("None"));
      }
    }
    AddLog_P(LOG_LEVEL_DEBUG, log);

    mqtt_data[0] = '\0';
    if (MqttShowSensor()) {
      MqttPublishPrefixTopic_P(TELE, PSTR(D_RSLT_SENSOR), Settings.flag.mqtt_sensor_retain);
#ifdef USE_RULES
      RulesTeleperiod();
#endif
    }
  }
}

void APDS9960_adjustATime(void)
{

  I2cValidRead16LE(&color_data.a, APDS9960_I2C_ADDR, APDS9960_CDATAL);


  if (color_data.a < (uint16_t)20){
    APDS9960_aTime = 0x40;
  }
  else if (color_data.a < (uint16_t)40){
    APDS9960_aTime = 0x80;
  }
  else if (color_data.a < (uint16_t)50){
    APDS9960_aTime = DEFAULT_ATIME;
  }
  else if (color_data.a < (uint16_t)70){
    APDS9960_aTime = 0xc0;
  }
  if (color_data.a < 200){
    APDS9960_aTime = 0xe9;
  }



  else{
    APDS9960_aTime = 0xff;
  }


  I2cWrite8(APDS9960_I2C_ADDR, APDS9960_ATIME, APDS9960_aTime);
  enablePower();
  enableLightSensor();
  delay(20);
}


void APDS9960_loop()
{
  if (recovery_loop_counter > 0){
    recovery_loop_counter -= 1;
  }
  if (recovery_loop_counter == 1 && APDS9960_overload){
    enableGestureSensor();
    APDS9960_overload = false;
    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"Gesture\":\"On\"}"));
    MqttPublishPrefixTopic_P(RESULT_OR_TELE, mqtt_data);
    gesture_mode = 1;
  }

  if (gesture_mode) {
      if (recovery_loop_counter == 0){
      handleGesture();

        if (APDS9960_overload)
        {
        disableGestureSensor();
        recovery_loop_counter = APDS9960_LONG_RECOVERY;
        snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"Gesture\":\"Off\"}"));
        MqttPublishPrefixTopic_P(RESULT_OR_TELE, mqtt_data);
        gesture_mode = 0;
        }
      }
  }
}

bool APDS9960_detect(void)
{
  if (APDS9960type) {
    return true;
  }

  boolean success = false;
  APDS9960type = I2cRead8(APDS9960_I2C_ADDR, APDS9960_ID);

  if (APDS9960type == APDS9960_CHIPID_1 || APDS9960type == APDS9960_CHIPID_2) {
    strcpy_P(APDS9960stype, PSTR("APDS9960"));
    snprintf_P(log_data, sizeof(log_data), S_LOG_I2C_FOUND_AT, APDS9960stype, APDS9960_I2C_ADDR);
    AddLog(LOG_LEVEL_DEBUG);
    if (APDS9960_init()) {
      success = true;
      AddLog_P(LOG_LEVEL_DEBUG, PSTR(D_LOG_DEBUG "APDS9960 initialized"));
      enableProximitySensor();
      enableGestureSensor();
    }
  }
  else {
    if (APDS9960type == APDS9930_CHIPID_1 || APDS9960type == APDS9930_CHIPID_2) {
      snprintf_P(log_data, sizeof(log_data), PSTR("APDS9930 found at address 0x%x, unsupported chip"), APDS9960_I2C_ADDR);
      AddLog(LOG_LEVEL_DEBUG);
    }
    else{
      snprintf_P(log_data, sizeof(log_data), PSTR("APDS9960 not found at address 0x%x"), APDS9960_I2C_ADDR);
      AddLog(LOG_LEVEL_DEBUG);
    }
  }
  currentGesture[0] = '\0';
  return success;
}





void APDS9960_show(boolean json)
{
  if (!APDS9960type) {
    return;
  }
  if (!gesture_mode && !APDS9960_overload) {
    char red_chr[10];
    char green_chr[10];
    char blue_chr[10];
    char ambient_chr[10];
    char cct_chr[10];
    char prox_chr[10];

    readAllColorAndProximityData();

    sprintf (ambient_chr, "%u", color_data.a/4);
    sprintf (red_chr, "%u", color_data.r);
    sprintf (green_chr, "%u", color_data.g);
    sprintf (blue_chr, "%u", color_data.b );
    sprintf (prox_chr, "%u", color_data.p );





    calculateColorTemperature();
    sprintf (cct_chr, "%u", color_data.cct);

    if (json) {
      snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s,\"%s\":{\"Red\":%s,\"Green\":%s,\"Blue\":%s,\"Ambient\":%s,\"CCT\":%s,\"Proximity\":%s}"),
        mqtt_data, APDS9960stype, red_chr, green_chr, blue_chr, ambient_chr, cct_chr, prox_chr);
#ifdef USE_WEBSERVER
    } else {
      snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_APDS_9960_SNS, mqtt_data, red_chr, green_chr, blue_chr, ambient_chr, cct_chr, prox_chr );
#endif
    }
  }
  else {
    if (json && (currentGesture[0] != '\0' )) {
      snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s,\"%s\":{\"%s\":1}"), mqtt_data, APDS9960stype, currentGesture);
      currentGesture[0] = '\0';
    }
  }
}
# 1997 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_27_apds9960.ino"
bool APDS9960CommandSensor()
{
  boolean serviced = true;

  switch (XdrvMailbox.payload) {
    case 0:
      disableGestureSensor();
      gesture_mode = 0;
      enableLightSensor();
      APDS9960_overload = false;
      break;
    case 1:
      if (APDS9960type) {
        setGestureGain(DEFAULT_GGAIN);
        setProximityGain(DEFAULT_PGAIN);
        disableLightSensor();
        enableGestureSensor();
        gesture_mode = 1;
      }
      break;
    case 2:
      if (APDS9960type) {
        setGestureGain(GGAIN_2X);
        setProximityGain(PGAIN_2X);
        disableLightSensor();
        enableGestureSensor();
        gesture_mode = 1;
      }
      break;
    default:
      int temp_aTime = (uint8_t)XdrvMailbox.payload;
      if (temp_aTime > 2 && temp_aTime < 256){
        disablePower();
        I2cWrite8(APDS9960_I2C_ADDR, APDS9960_ATIME, temp_aTime);
        enablePower();
        enableLightSensor();
      }
    break;
  }
  snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_SENSOR_INDEX_SVALUE, XSNS_27, GetStateText(gesture_mode));

  return serviced;
}





boolean Xsns27(byte function)
{
  boolean result = false;

  if (i2c_flg) {
    if (FUNC_INIT == function) {
      APDS9960_detect();
    } else if (APDS9960type) {
      switch (function) {
        case FUNC_EVERY_50_MSECOND:
            APDS9960_loop();
            break;
        case FUNC_COMMAND:
            if (XSNS_27 == XdrvMailbox.index) {
            result = APDS9960CommandSensor();
            }
            break;
        case FUNC_JSON_APPEND:
            APDS9960_show(1);
            break;
#ifdef USE_WEBSERVER
        case FUNC_WEB_APPEND:
          APDS9960_show(0);
          break;
#endif
      }
    }
  }
  return result;
}
#endif
#endif
# 1 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_28_tm1638.ino"
# 20 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_28_tm1638.ino"
#ifdef USE_TM1638






#define TM1638_COLOR_NONE 0
#define TM1638_COLOR_RED 1
#define TM1638_COLOR_GREEN 2

#define TM1638_CLOCK_DELAY 1

uint8_t tm1638_type = 1;
uint8_t tm1638_clock_pin = 0;
uint8_t tm1638_data_pin = 0;
uint8_t tm1638_strobe_pin = 0;
uint8_t tm1638_displays = 8;
uint8_t tm1638_active_display = 1;
uint8_t tm1638_intensity = 0;
uint8_t tm1638_state = 0;






void Tm16XXSend(byte data)
{
 for (uint8_t i = 0; i < 8; i++) {
    digitalWrite(tm1638_data_pin, !!(data & (1 << i)));
    digitalWrite(tm1638_clock_pin, LOW);
    delayMicroseconds(TM1638_CLOCK_DELAY);
    digitalWrite(tm1638_clock_pin, HIGH);
  }
}

void Tm16XXSendCommand(byte cmd)
{
  digitalWrite(tm1638_strobe_pin, LOW);
  Tm16XXSend(cmd);
  digitalWrite(tm1638_strobe_pin, HIGH);
}

void TM16XXSendData(byte address, byte data)
{
  Tm16XXSendCommand(0x44);
  digitalWrite(tm1638_strobe_pin, LOW);
  Tm16XXSend(0xC0 | address);
  Tm16XXSend(data);
  digitalWrite(tm1638_strobe_pin, HIGH);
}

byte Tm16XXReceive()
{
  byte temp = 0;


  pinMode(tm1638_data_pin, INPUT);
  digitalWrite(tm1638_data_pin, HIGH);

  for (uint8_t i = 0; i < 8; ++i) {
    digitalWrite(tm1638_clock_pin, LOW);
    delayMicroseconds(TM1638_CLOCK_DELAY);
    temp |= digitalRead(tm1638_data_pin) << i;
    digitalWrite(tm1638_clock_pin, HIGH);
  }


  pinMode(tm1638_data_pin, OUTPUT);
  digitalWrite(tm1638_data_pin, LOW);

  return temp;
}



void Tm16XXClearDisplay()
{
  for (int i = 0; i < tm1638_displays; i++) {
    TM16XXSendData(i << 1, 0);
  }
}

void Tm1638SetLED(byte color, byte pos)
{
  TM16XXSendData((pos << 1) + 1, color);
}

void Tm1638SetLEDs(word leds)
{
  for (int i = 0; i < tm1638_displays; i++) {
    byte color = 0;

    if ((leds & (1 << i)) != 0) {
      color |= TM1638_COLOR_RED;
    }

    if ((leds & (1 << (i + 8))) != 0) {
      color |= TM1638_COLOR_GREEN;
    }

    Tm1638SetLED(color, i);
  }
}

byte Tm1638GetButtons()
{
  byte keys = 0;

  digitalWrite(tm1638_strobe_pin, LOW);
  Tm16XXSend(0x42);
  for (int i = 0; i < 4; i++) {
    keys |= Tm16XXReceive() << i;
  }
  digitalWrite(tm1638_strobe_pin, HIGH);

  return keys;
}



void TmInit()
{
  tm1638_type = 0;
  if ((pin[GPIO_TM16CLK] < 99) && (pin[GPIO_TM16DIO] < 99) && (pin[GPIO_TM16STB] < 99)) {
    tm1638_clock_pin = pin[GPIO_TM16CLK];
    tm1638_data_pin = pin[GPIO_TM16DIO];
    tm1638_strobe_pin = pin[GPIO_TM16STB];

    pinMode(tm1638_data_pin, OUTPUT);
    pinMode(tm1638_clock_pin, OUTPUT);
    pinMode(tm1638_strobe_pin, OUTPUT);

    digitalWrite(tm1638_strobe_pin, HIGH);
    digitalWrite(tm1638_clock_pin, HIGH);

    Tm16XXSendCommand(0x40);
    Tm16XXSendCommand(0x80 | (tm1638_active_display ? 8 : 0) | tmin(7, tm1638_intensity));

    digitalWrite(tm1638_strobe_pin, LOW);
    Tm16XXSend(0xC0);
    for (int i = 0; i < 16; i++) {
      Tm16XXSend(0x00);
    }
    digitalWrite(tm1638_strobe_pin, HIGH);

    tm1638_type = 1;
    tm1638_state = 1;
  }
}

void TmLoop()
{
  if (tm1638_state) {
    byte buttons = Tm1638GetButtons();
    for (byte i = 0; i < MAX_SWITCHES; i++) {
      virtualswitch[i] = (buttons &1) ^1;
      byte color = (virtualswitch[i]) ? TM1638_COLOR_NONE : TM1638_COLOR_RED;
      Tm1638SetLED(color, i);
      buttons >>= 1;
    }
    SwitchHandler(1);
  }
}
# 199 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_28_tm1638.ino"
#define XSNS_28 

boolean Xsns28(byte function)
{
  boolean result = false;

  if (tm1638_type) {
    switch (function) {
      case FUNC_INIT:
        TmInit();
        break;
      case FUNC_EVERY_50_MSECOND:
        TmLoop();
        break;
# 223 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_28_tm1638.ino"
    }
  }
  return result;
}

#endif
# 1 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_29_mcp230xx.ino"
# 20 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_29_mcp230xx.ino"
#ifdef USE_I2C
#ifdef USE_MCP230xx
# 32 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_29_mcp230xx.ino"
#define XSNS_29 29





uint8_t MCP230xx_IODIR = 0x00;
uint8_t MCP230xx_GPINTEN = 0x02;
uint8_t MCP230xx_IOCON = 0x05;
uint8_t MCP230xx_GPPU = 0x06;
uint8_t MCP230xx_INTF = 0x07;
uint8_t MCP230xx_INTCAP = 0x08;
uint8_t MCP230xx_GPIO = 0x09;

uint8_t mcp230xx_type = 0;
uint8_t mcp230xx_pincount = 0;
uint8_t mcp230xx_int_en = 0;
uint8_t mcp230xx_int_prio_counter = 0;
uint8_t mcp230xx_int_counter_en = 0;
uint8_t mcp230xx_int_sec_counter = 0;

uint8_t mcp230xx_int_report_defer_counter[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

uint16_t mcp230xx_int_counter[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

unsigned long int_millis[16];

const char MCP230XX_SENSOR_RESPONSE[] PROGMEM = "{\"Sensor29_D%i\":{\"MODE\":%i,\"PULL_UP\":\"%s\",\"INT_MODE\":\"%s\",\"STATE\":\"%s\"}}";

const char MCP230XX_INTCFG_RESPONSE[] PROGMEM = "{\"MCP230xx_INT%s\":{\"D_%i\":%i}}";

#ifdef USE_MCP230xx_OUTPUT
const char MCP230XX_CMND_RESPONSE[] PROGMEM = "{\"S29cmnd_D%i\":{\"COMMAND\":\"%s\",\"STATE\":\"%s\"}}";
#endif

void MCP230xx_CheckForIntCounter(void) {
  uint8_t en = 0;
  for (uint8_t ca=0;ca<16;ca++) {
    if (Settings.mcp230xx_config[ca].int_count_en) {
      en=1;
    }
  }
  if (!Settings.mcp230xx_int_timer) en=0;
  mcp230xx_int_counter_en=en;
  if (!mcp230xx_int_counter_en) {
    for (uint8_t ca=0;ca<16;ca++) {
      mcp230xx_int_counter[ca] = 0;
    }
  }
}


const char* ConvertNumTxt(uint8_t statu, uint8_t pinmod=0) {
#ifdef USE_MCP230xx_OUTPUT
if ((6 == pinmod) && (statu < 2)) { statu = abs(statu-1); }
#endif
  switch (statu) {
    case 0:
      return "OFF";
      break;
    case 1:
      return "ON";
      break;
#ifdef USE_MCP230xx_OUTPUT
    case 2:
      return "TOGGLE";
      break;
#endif
  }
  return "";
}

const char* IntModeTxt(uint8_t intmo) {
  switch (intmo) {
    case 0:
      return "ALL";
      break;
    case 1:
      return "EVENT";
      break;
    case 2:
      return "TELE";
      break;
    case 3:
      return "DISABLED";
      break;
  }
  return "";
}

uint8_t MCP230xx_readGPIO(uint8_t port) {
  return I2cRead8(USE_MCP230xx_ADDR, MCP230xx_GPIO + port);
}

void MCP230xx_ApplySettings(void) {
  uint8_t int_en = 0;
  for (uint8_t mcp230xx_port=0;mcp230xx_port<mcp230xx_type;mcp230xx_port++) {
    uint8_t reg_gppu = 0;
    uint8_t reg_gpinten = 0;
    uint8_t reg_iodir = 0xFF;
#ifdef USE_MCP230xx_OUTPUT
    uint8_t reg_portpins = 0x00;
#endif
    for (uint8_t idx = 0; idx < 8; idx++) {
      switch (Settings.mcp230xx_config[idx+(mcp230xx_port*8)].pinmode) {
        case 0 ... 1:
          reg_iodir |= (1 << idx);
          break;
        case 2 ... 4:
          reg_iodir |= (1 << idx);
          reg_gpinten |= (1 << idx);
          int_en = 1;
          break;
#ifdef USE_MCP230xx_OUTPUT
        case 5 ... 6:
          reg_iodir &= ~(1 << idx);
          if (Settings.flag.save_state) {
            reg_portpins |= (Settings.mcp230xx_config[idx+(mcp230xx_port*8)].saved_state << idx);
          } else {
            if (Settings.mcp230xx_config[idx+(mcp230xx_port*8)].pullup) {
              reg_portpins |= (1 << idx);
            }
          }
          break;
#endif
        default:
          break;
      }
#ifdef USE_MCP230xx_OUTPUT
      if ((Settings.mcp230xx_config[idx+(mcp230xx_port*8)].pullup) && (Settings.mcp230xx_config[idx+(mcp230xx_port*8)].pinmode < 5)) {
        reg_gppu |= (1 << idx);
      }
#else
      if (Settings.mcp230xx_config[idx+(mcp230xx_port*8)].pullup) {
        reg_gppu |= (1 << idx);
      }
#endif
    }
    I2cWrite8(USE_MCP230xx_ADDR, MCP230xx_GPPU+mcp230xx_port, reg_gppu);
    I2cWrite8(USE_MCP230xx_ADDR, MCP230xx_GPINTEN+mcp230xx_port, reg_gpinten);
    I2cWrite8(USE_MCP230xx_ADDR, MCP230xx_IODIR+mcp230xx_port, reg_iodir);
#ifdef USE_MCP230xx_OUTPUT
    I2cWrite8(USE_MCP230xx_ADDR, MCP230xx_GPIO+mcp230xx_port, reg_portpins);
#endif
  }
  for (uint8_t idx=0;idx<mcp230xx_pincount;idx++) {
    int_millis[idx]=millis();
  }
  mcp230xx_int_en = int_en;
  MCP230xx_CheckForIntCounter();
}

void MCP230xx_Detect(void)
{
  if (mcp230xx_type) {
    return;
  }

  uint8_t buffer;

  I2cWrite8(USE_MCP230xx_ADDR, MCP230xx_IOCON, 0x80);
  if (I2cValidRead8(&buffer, USE_MCP230xx_ADDR, MCP230xx_IOCON)) {
    if (0x00 == buffer) {
      mcp230xx_type = 1;
      snprintf_P(log_data, sizeof(log_data), S_LOG_I2C_FOUND_AT, "MCP23008", USE_MCP230xx_ADDR);
      AddLog(LOG_LEVEL_DEBUG);
      mcp230xx_pincount = 8;
      MCP230xx_ApplySettings();
    } else {
      if (0x80 == buffer) {
        mcp230xx_type = 2;
        snprintf_P(log_data, sizeof(log_data), S_LOG_I2C_FOUND_AT, "MCP23017", USE_MCP230xx_ADDR);
        AddLog(LOG_LEVEL_DEBUG);
        mcp230xx_pincount = 16;

        I2cWrite8(USE_MCP230xx_ADDR, MCP230xx_IOCON, 0x00);

        MCP230xx_GPINTEN = 0x04;
        MCP230xx_GPPU = 0x0C;
        MCP230xx_INTF = 0x0E;
        MCP230xx_INTCAP = 0x10;
        MCP230xx_GPIO = 0x12;
        MCP230xx_ApplySettings();
      }
    }
  }
}

void MCP230xx_CheckForInterrupt(void) {
  uint8_t intf;
  uint8_t mcp230xx_intcap = 0;
  uint8_t report_int;
  for (uint8_t mcp230xx_port=0;mcp230xx_port<mcp230xx_type;mcp230xx_port++) {
    if (I2cValidRead8(&intf,USE_MCP230xx_ADDR,MCP230xx_INTF+mcp230xx_port)) {
      if (intf > 0) {
        if (I2cValidRead8(&mcp230xx_intcap, USE_MCP230xx_ADDR, MCP230xx_INTCAP+mcp230xx_port)) {
          for (uint8_t intp = 0; intp < 8; intp++) {
            if ((intf >> intp) & 0x01) {
              report_int = 0;
              if (Settings.mcp230xx_config[intp+(mcp230xx_port*8)].pinmode > 1) {
                switch (Settings.mcp230xx_config[intp+(mcp230xx_port*8)].pinmode) {
                  case 2:
                    report_int = 1;
                    break;
                  case 3:
                    if (((mcp230xx_intcap >> intp) & 0x01) == 0) report_int = 1;
                    break;
                  case 4:
                    if (((mcp230xx_intcap >> intp) & 0x01) == 1) report_int = 1;
                    break;
                  default:
                    break;
                }

                if ((mcp230xx_int_counter_en) && (report_int)) {
                  if (Settings.mcp230xx_config[intp+(mcp230xx_port*8)].int_count_en) {
                    mcp230xx_int_counter[intp+(mcp230xx_port*8)]++;
                  }
                }

                if (report_int) {
                  if (Settings.mcp230xx_config[intp+(mcp230xx_port*8)].int_report_defer) {
                    mcp230xx_int_report_defer_counter[intp+(mcp230xx_port*8)]++;
                    if (mcp230xx_int_report_defer_counter[intp+(mcp230xx_port*8)] >= Settings.mcp230xx_config[intp+(mcp230xx_port*8)].int_report_defer) {
                      mcp230xx_int_report_defer_counter[intp+(mcp230xx_port*8)]=0;
                    } else {
                      report_int = 0;
                    }
                  }
                }
                if (Settings.mcp230xx_config[intp+(mcp230xx_port*8)].int_count_en) {
                  report_int = 0;
                }
                if (report_int) {
                  bool int_tele = false;
                  bool int_event = false;
                  unsigned long millis_now = millis();
                  unsigned long millis_since_last_int = millis_now - int_millis[intp+(mcp230xx_port*8)];
                  int_millis[intp+(mcp230xx_port*8)]=millis_now;
                  switch (Settings.mcp230xx_config[intp+(mcp230xx_port*8)].int_report_mode) {
                    case 0:
                      int_tele=true;
                      int_event=true;
                      break;
                    case 1:
                      int_event=true;
                      break;
                    case 2:
                      int_tele=true;
                      break;
                  }
                  if (int_tele) {
                    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"" D_JSON_TIME "\":\"%s\""), GetDateAndTime(DT_LOCAL).c_str());
                    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s,\"MCP230XX_INT\":{\"D%i\":%i,\"MS\":%lu}"), mqtt_data, intp+(mcp230xx_port*8), ((mcp230xx_intcap >> intp) & 0x01),millis_since_last_int);
                    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s}"), mqtt_data);
                    MqttPublishPrefixTopic_P(RESULT_OR_STAT, PSTR("MCP230XX_INT"));
                  }
                  if (int_event) {
                    char command[19];
                    sprintf(command,"event MCPINT_D%i=%i",intp+(mcp230xx_port*8),((mcp230xx_intcap >> intp) & 0x01));
                    ExecuteCommand(command, SRC_RULE);
                  }
                }
              }
            }
          }
        }
      }
    }
  }
}

void MCP230xx_Show(boolean json)
{
  if (mcp230xx_type) {
    if (json) {
      if (mcp230xx_type > 0) {
        uint8_t gpio = MCP230xx_readGPIO(0);
        snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s,\"MCP230XX\":{\"D0\":%i,\"D1\":%i,\"D2\":%i,\"D3\":%i,\"D4\":%i,\"D5\":%i,\"D6\":%i,\"D7\":%i"),
                   mqtt_data,(gpio>>0)&1,(gpio>>1)&1,(gpio>>2)&1,(gpio>>3)&1,(gpio>>4)&1,(gpio>>5)&1,(gpio>>6)&1,(gpio>>7)&1);
        if (2 == mcp230xx_type) {
          gpio = MCP230xx_readGPIO(1);
          snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s,\"D8\":%i,\"D9\":%i,\"D10\":%i,\"D11\":%i,\"D12\":%i,\"D13\":%i,\"D14\":%i,\"D15\":%i"),
                     mqtt_data,(gpio>>0)&1,(gpio>>1)&1,(gpio>>2)&1,(gpio>>3)&1,(gpio>>4)&1,(gpio>>5)&1,(gpio>>6)&1,(gpio>>7)&1);
        }
        snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s%s"),mqtt_data,"}");
      }
    }
  }
}

#ifdef USE_MCP230xx_OUTPUT

void MCP230xx_SetOutPin(uint8_t pin,uint8_t pinstate) {
  uint8_t portpins;
  uint8_t port = 0;
  uint8_t pinmo = Settings.mcp230xx_config[pin].pinmode;
  uint8_t interlock = Settings.flag.interlock;
  int pinadd = (pin % 2)+1-(3*(pin % 2));
  char cmnd[7], stt[4];
  if (pin > 7) { port = 1; }
  portpins = MCP230xx_readGPIO(port);
  if (interlock && (pinmo == Settings.mcp230xx_config[pin+pinadd].pinmode)) {
    if (pinstate < 2) {
      if (6 == pinmo) {
        if (pinstate) portpins |= (1 << (pin-(port*8))); else portpins |= (1 << (pin+pinadd-(port*8))),portpins &= ~(1 << (pin-(port*8)));
      } else {
        if (pinstate) portpins &= ~(1 << (pin+pinadd-(port*8))),portpins |= (1 << (pin-(port*8))); else portpins &= ~(1 << (pin-(port*8)));
      }
    } else {
      if (6 == pinmo) {
      portpins |= (1 << (pin+pinadd-(port*8))),portpins ^= (1 << (pin-(port*8)));
      } else {
      portpins &= ~(1 << (pin+pinadd-(port*8))),portpins ^= (1 << (pin-(port*8)));
      }
    }
  } else {
    if (pinstate < 2) {
      if (pinstate) portpins |= (1 << (pin-(port*8))); else portpins &= ~(1 << (pin-(port*8)));
    } else {
      portpins ^= (1 << (pin-(port*8)));
    }
  }
  I2cWrite8(USE_MCP230xx_ADDR, MCP230xx_GPIO + port, portpins);
  if (Settings.flag.save_state) {
    Settings.mcp230xx_config[pin].saved_state=portpins>>(pin-(port*8))&1;
    Settings.mcp230xx_config[pin+pinadd].saved_state=portpins>>(pin+pinadd-(port*8))&1;
  }
  sprintf(cmnd,ConvertNumTxt(pinstate, pinmo));
  sprintf(stt,ConvertNumTxt((portpins >> (pin-(port*8))&1), pinmo));
  if (interlock && (pinmo == Settings.mcp230xx_config[pin+pinadd].pinmode)) {
    char stt1[4];
    sprintf(stt1,ConvertNumTxt((portpins >> (pin+pinadd-(port*8))&1), pinmo));
    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"S29cmnd_D%i\":{\"COMMAND\":\"%s\",\"STATE\":\"%s\"},\"S29cmnd_D%i\":{\"STATE\":\"%s\"}}"),pin, cmnd, stt, pin+pinadd, stt1);
  } else {
    snprintf_P(mqtt_data, sizeof(mqtt_data), MCP230XX_CMND_RESPONSE, pin, cmnd, stt);
  }
}

#endif

void MCP230xx_Reset(uint8_t pinmode) {
  uint8_t pullup = 0;
  if ((pinmode > 1) && (pinmode < 5)) { pullup=1; }
  for (uint8_t pinx=0;pinx<16;pinx++) {
    Settings.mcp230xx_config[pinx].pinmode=pinmode;
    Settings.mcp230xx_config[pinx].pullup=pullup;
    Settings.mcp230xx_config[pinx].saved_state=0;
    if ((pinmode > 1) && (pinmode < 5)) {
      Settings.mcp230xx_config[pinx].int_report_mode=0;
    } else {
      Settings.mcp230xx_config[pinx].int_report_mode=3;
    }
    Settings.mcp230xx_config[pinx].int_report_defer=0;
    Settings.mcp230xx_config[pinx].int_count_en=0;
    Settings.mcp230xx_config[pinx].spare12=0;
    Settings.mcp230xx_config[pinx].spare13=0;
    Settings.mcp230xx_config[pinx].spare14=0;
    Settings.mcp230xx_config[pinx].spare15=0;
  }
  Settings.mcp230xx_int_prio = 0;
  Settings.mcp230xx_int_timer = 0;
  MCP230xx_ApplySettings();
  char pulluptxt[7];
  char intmodetxt[9];
  sprintf(pulluptxt,ConvertNumTxt(pullup));
  uint8_t intmode = 3;
  if ((pinmode > 1) && (pinmode < 5)) { intmode = 0; }
  sprintf(intmodetxt,IntModeTxt(intmode));
  snprintf_P(mqtt_data, sizeof(mqtt_data), MCP230XX_SENSOR_RESPONSE,99,pinmode,pulluptxt,intmodetxt,"");
}

bool MCP230xx_Command(void) {
  boolean serviced = true;
  boolean validpin = false;
  uint8_t paramcount = 0;
  if (XdrvMailbox.data_len > 0) {
    paramcount=1;
  } else {
    serviced = false;
    return serviced;
  }
  char sub_string[XdrvMailbox.data_len];
  for (uint8_t ca=0;ca<XdrvMailbox.data_len;ca++) {
    if ((' ' == XdrvMailbox.data[ca]) || ('=' == XdrvMailbox.data[ca])) { XdrvMailbox.data[ca] = ','; }
    if (',' == XdrvMailbox.data[ca]) { paramcount++; }
  }
  UpperCase(XdrvMailbox.data,XdrvMailbox.data);
  if (!strcmp(subStr(sub_string, XdrvMailbox.data, ",", 1),"RESET")) { MCP230xx_Reset(1); return serviced; }
  if (!strcmp(subStr(sub_string, XdrvMailbox.data, ",", 1),"RESET1")) { MCP230xx_Reset(1); return serviced; }
  if (!strcmp(subStr(sub_string, XdrvMailbox.data, ",", 1),"RESET2")) { MCP230xx_Reset(2); return serviced; }
  if (!strcmp(subStr(sub_string, XdrvMailbox.data, ",", 1),"RESET3")) { MCP230xx_Reset(3); return serviced; }
  if (!strcmp(subStr(sub_string, XdrvMailbox.data, ",", 1),"RESET4")) { MCP230xx_Reset(4); return serviced; }
#ifdef USE_MCP230xx_OUTPUT
  if (!strcmp(subStr(sub_string, XdrvMailbox.data, ",", 1),"RESET5")) { MCP230xx_Reset(5); return serviced; }
  if (!strcmp(subStr(sub_string, XdrvMailbox.data, ",", 1),"RESET6")) { MCP230xx_Reset(6); return serviced; }
#endif

  if (!strcmp(subStr(sub_string, XdrvMailbox.data, ",", 1),"INTPRI")) {
    if (paramcount > 1) {
      uint8_t intpri = atoi(subStr(sub_string, XdrvMailbox.data, ",", 2));
      if ((intpri >= 0) && (intpri <= 20)) {
        Settings.mcp230xx_int_prio = intpri;
        snprintf_P(mqtt_data, sizeof(mqtt_data), MCP230XX_INTCFG_RESPONSE,"PRI",99,Settings.mcp230xx_int_prio);
        return serviced;
      }
    } else {
      snprintf_P(mqtt_data, sizeof(mqtt_data), MCP230XX_INTCFG_RESPONSE,"PRI",99,Settings.mcp230xx_int_prio);
      return serviced;
    }
  }

  if (!strcmp(subStr(sub_string, XdrvMailbox.data, ",", 1),"INTTIMER")) {
    if (paramcount > 1) {
      uint8_t inttim = atoi(subStr(sub_string, XdrvMailbox.data, ",", 2));
      if ((inttim >= 0) && (inttim <= 3600)) {
        Settings.mcp230xx_int_timer = inttim;
        MCP230xx_CheckForIntCounter();
        snprintf_P(mqtt_data, sizeof(mqtt_data), MCP230XX_INTCFG_RESPONSE,"TIMER",99,Settings.mcp230xx_int_timer);
        return serviced;
      }
    } else {
      snprintf_P(mqtt_data, sizeof(mqtt_data), MCP230XX_INTCFG_RESPONSE,"TIMER",99,Settings.mcp230xx_int_timer);
      return serviced;
    }
  }

  if (!strcmp(subStr(sub_string, XdrvMailbox.data, ",", 1),"INTDEF")) {
    if (paramcount > 1) {
      uint8_t pin = atoi(subStr(sub_string, XdrvMailbox.data, ",", 2));
      if (pin < mcp230xx_pincount) {
        if (pin == 0) {
          if (!strcmp(subStr(sub_string, XdrvMailbox.data, ",", 2), "0")) validpin=true;
        } else {
          validpin = true;
        }
      }
      if (validpin) {
        if (paramcount > 2) {
          uint8_t intdef = atoi(subStr(sub_string, XdrvMailbox.data, ",", 3));
          if ((intdef >= 0) && (intdef <= 15)) {
            Settings.mcp230xx_config[pin].int_report_defer=intdef;
            if (Settings.mcp230xx_config[pin].int_count_en) {
              Settings.mcp230xx_config[pin].int_count_en=0;
              MCP230xx_CheckForIntCounter();
              snprintf_P(log_data, sizeof(log_data), PSTR("*** WARNING *** - Disabled INTCNT for pin D%i"),pin);
              AddLog(LOG_LEVEL_INFO);
            }
            snprintf_P(mqtt_data, sizeof(mqtt_data), MCP230XX_INTCFG_RESPONSE,"DEF",pin,Settings.mcp230xx_config[pin].int_report_defer);
            return serviced;
          } else {
            serviced=false;
            return serviced;
          }
        } else {
          snprintf_P(mqtt_data, sizeof(mqtt_data), MCP230XX_INTCFG_RESPONSE,"DEF",pin,Settings.mcp230xx_config[pin].int_report_defer);
          return serviced;
        }
      }
      serviced = false;
      return serviced;
    } else {
      serviced = false;
      return serviced;
    }
  }

  if (!strcmp(subStr(sub_string, XdrvMailbox.data, ",", 1),"INTCNT")) {
    if (paramcount > 1) {
      uint8_t pin = atoi(subStr(sub_string, XdrvMailbox.data, ",", 2));
      if (pin < mcp230xx_pincount) {
        if (pin == 0) {
          if (!strcmp(subStr(sub_string, XdrvMailbox.data, ",", 2), "0")) validpin=true;
        } else {
          validpin = true;
        }
      }
      if (validpin) {
        if (paramcount > 2) {
          uint8_t intcnt = atoi(subStr(sub_string, XdrvMailbox.data, ",", 3));
          if ((intcnt >= 0) && (intcnt <= 1)) {
            Settings.mcp230xx_config[pin].int_count_en=intcnt;
            if (Settings.mcp230xx_config[pin].int_report_defer) {
              Settings.mcp230xx_config[pin].int_report_defer=0;
              snprintf_P(log_data, sizeof(log_data), PSTR("*** WARNING *** - Disabled INTDEF for pin D%i"),pin);
              AddLog(LOG_LEVEL_INFO);
            }
            if (Settings.mcp230xx_config[pin].int_report_mode < 3) {
              Settings.mcp230xx_config[pin].int_report_mode=3;
              snprintf_P(log_data, sizeof(log_data), PSTR("*** WARNING *** - Disabled immediate interrupt/telemetry reporting for pin D%i"),pin);
              AddLog(LOG_LEVEL_INFO);
            }
            if ((Settings.mcp230xx_config[pin].int_count_en) && (!Settings.mcp230xx_int_timer)) {
              snprintf_P(log_data, sizeof(log_data), PSTR("*** WARNING *** - INTCNT enabled for pin D%i but global INTTIMER is disabled!"),pin);
              AddLog(LOG_LEVEL_INFO);
            }
            MCP230xx_CheckForIntCounter();
            snprintf_P(mqtt_data, sizeof(mqtt_data), MCP230XX_INTCFG_RESPONSE,"CNT",pin,Settings.mcp230xx_config[pin].int_count_en);
            return serviced;
          } else {
            serviced=false;
            return serviced;
          }
        } else {
          snprintf_P(mqtt_data, sizeof(mqtt_data), MCP230XX_INTCFG_RESPONSE,"CNT",pin,Settings.mcp230xx_config[pin].int_count_en);
          return serviced;
        }
      }
      serviced = false;
      return serviced;
    } else {
      serviced = false;
      return serviced;
    }
  }

  uint8_t pin = atoi(subStr(sub_string, XdrvMailbox.data, ",", 1));

  if (pin < mcp230xx_pincount) {
    if (0 == pin) {
      if (!strcmp(subStr(sub_string, XdrvMailbox.data, ",", 1), "0")) validpin=true;
    } else {
      validpin=true;
    }
  }
  if (validpin && (paramcount > 1)) {
    if (!strcmp(subStr(sub_string, XdrvMailbox.data, ",", 2), "?")) {
      uint8_t port = 0;
      if (pin > 7) { port = 1; }
      uint8_t portdata = MCP230xx_readGPIO(port);
      char pulluptxtr[7],pinstatustxtr[7];
      char intmodetxt[9];
      sprintf(intmodetxt,IntModeTxt(Settings.mcp230xx_config[pin].int_report_mode));
      sprintf(pulluptxtr,ConvertNumTxt(Settings.mcp230xx_config[pin].pullup));
#ifdef USE_MCP230xx_OUTPUT
      uint8_t pinmod = Settings.mcp230xx_config[pin].pinmode;
      sprintf(pinstatustxtr,ConvertNumTxt(portdata>>(pin-(port*8))&1,pinmod));
      snprintf_P(mqtt_data, sizeof(mqtt_data), MCP230XX_SENSOR_RESPONSE,pin,pinmod,pulluptxtr,intmodetxt,pinstatustxtr);
#else
      sprintf(pinstatustxtr,ConvertNumTxt(portdata>>(pin-(port*8))&1));
      snprintf_P(mqtt_data, sizeof(mqtt_data), MCP230XX_SENSOR_RESPONSE,pin,Settings.mcp230xx_config[pin].pinmode,pulluptxtr,intmodetxt,pinstatustxtr);
#endif
      return serviced;
    }
#ifdef USE_MCP230xx_OUTPUT
    if (Settings.mcp230xx_config[pin].pinmode >= 5) {
      uint8_t pincmd = Settings.mcp230xx_config[pin].pinmode - 5;
      if (!strcmp(subStr(sub_string, XdrvMailbox.data, ",", 2), "ON")) {
        MCP230xx_SetOutPin(pin,abs(pincmd-1));
        return serviced;
      }
      if (!strcmp(subStr(sub_string, XdrvMailbox.data, ",", 2), "OFF")) {
        MCP230xx_SetOutPin(pin,pincmd);
        return serviced;
      }
      if (!strcmp(subStr(sub_string, XdrvMailbox.data, ",", 2), "T")) {
        MCP230xx_SetOutPin(pin,2);
        return serviced;
      }
    }
#endif
    uint8_t pinmode = 0;
    uint8_t pullup = 0;
    uint8_t intmode = 0;
    if (paramcount > 1) {
      pinmode = atoi(subStr(sub_string, XdrvMailbox.data, ",", 2));
    }
    if (paramcount > 2) {
      pullup = atoi(subStr(sub_string, XdrvMailbox.data, ",", 3));
    }
    if (paramcount > 3) {
      intmode = atoi(subStr(sub_string, XdrvMailbox.data, ",", 4));
    }
#ifdef USE_MCP230xx_OUTPUT
    if ((pin < mcp230xx_pincount) && (pinmode > 0) && (pinmode < 7) && (pullup < 2)) {
#else
    if ((pin < mcp230xx_pincount) && (pinmode > 0) && (pinmode < 5) && (pullup < 2)) {
#endif
      Settings.mcp230xx_config[pin].pinmode=pinmode;
      Settings.mcp230xx_config[pin].pullup=pullup;
      if ((pinmode > 1) && (pinmode < 5)) {
        if ((intmode >= 0) && (intmode <= 3)) {
          Settings.mcp230xx_config[pin].int_report_mode=intmode;
        }
      } else {
        Settings.mcp230xx_config[pin].int_report_mode=3;
      }
      MCP230xx_ApplySettings();
      uint8_t port = 0;
      if (pin > 7) { port = 1; }
      uint8_t portdata = MCP230xx_readGPIO(port);
      char pulluptxtc[7], pinstatustxtc[7];
      char intmodetxt[9];
      sprintf(pulluptxtc,ConvertNumTxt(pullup));
      sprintf(intmodetxt,IntModeTxt(Settings.mcp230xx_config[pin].int_report_mode));
#ifdef USE_MCP230xx_OUTPUT
      sprintf(pinstatustxtc,ConvertNumTxt(portdata>>(pin-(port*8))&1,Settings.mcp230xx_config[pin].pinmode));
#else
      sprintf(pinstatustxtc,ConvertNumTxt(portdata>>(pin-(port*8))&1));
#endif
      snprintf_P(mqtt_data, sizeof(mqtt_data), MCP230XX_SENSOR_RESPONSE,pin,pinmode,pulluptxtc,intmodetxt,pinstatustxtc);
      return serviced;
    }
  } else {
    serviced=false;
    return serviced;
  }
  return serviced;
}

#ifdef USE_MCP230xx_DISPLAYOUTPUT

const char HTTP_SNS_MCP230xx_OUTPUT[] PROGMEM = "%s{s}MCP230XX D%d{m}%s{e}";

void MCP230xx_UpdateWebData(void) {
  uint8_t gpio1 = MCP230xx_readGPIO(0);
  uint8_t gpio2 = 0;
  if (2 == mcp230xx_type) {
    gpio2 = MCP230xx_readGPIO(1);
  }
  uint16_t gpio = (gpio2 << 8) + gpio1;
  for (uint8_t pin = 0; pin < mcp230xx_pincount; pin++) {
    if (Settings.mcp230xx_config[pin].pinmode >= 5) {
      char stt[7];
      sprintf(stt,ConvertNumTxt((gpio>>pin)&1,Settings.mcp230xx_config[pin].pinmode));
      snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_SNS_MCP230xx_OUTPUT, mqtt_data, pin, stt);
    }
  }
}

#endif

#ifdef USE_MCP230xx_OUTPUT

void MCP230xx_OutputTelemetry(void) {
  if (0 == mcp230xx_type) { return; }
  uint8_t outputcount = 0;
  uint16_t gpiototal = 0;
  uint8_t gpioa = 0;
  uint8_t gpiob = 0;
  gpioa=MCP230xx_readGPIO(0);
  if (2 == mcp230xx_type) { gpiob=MCP230xx_readGPIO(1); }
  gpiototal=((uint16_t)gpiob << 8) | gpioa;
  for (uint8_t pinx = 0;pinx < mcp230xx_pincount;pinx++) {
    if (Settings.mcp230xx_config[pinx].pinmode >= 5) outputcount++;
  }
  if (outputcount) {
    char stt[7];
    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"" D_JSON_TIME "\":\"%s\",\"MCP230_OUT\": {"), GetDateAndTime(DT_LOCAL).c_str());
    for (uint8_t pinx = 0;pinx < mcp230xx_pincount;pinx++) {
      if (Settings.mcp230xx_config[pinx].pinmode >= 5) {
        sprintf(stt,ConvertNumTxt(((gpiototal>>pinx)&1),Settings.mcp230xx_config[pinx].pinmode));
        snprintf_P(mqtt_data,sizeof(mqtt_data), PSTR("%s\"OUT_D%i\":\"%s\","),mqtt_data,pinx,stt);
      }
    }
    snprintf_P(mqtt_data,sizeof(mqtt_data),PSTR("%s\"END\":1}}"),mqtt_data);
    MqttPublishPrefixTopic_P(TELE, PSTR(D_RSLT_SENSOR), Settings.flag.mqtt_sensor_retain);
  }
}

#endif

void MCP230xx_Interrupt_Counter_Report(void) {
  snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"" D_JSON_TIME "\":\"%s\",\"MCP230_INTTIMER\": {"), GetDateAndTime(DT_LOCAL).c_str());
  for (uint8_t pinx = 0;pinx < mcp230xx_pincount;pinx++) {
    if (Settings.mcp230xx_config[pinx].int_count_en) {
      snprintf_P(mqtt_data,sizeof(mqtt_data), PSTR("%s\"INTCNT_D%i\":%i,"),mqtt_data,pinx,mcp230xx_int_counter[pinx]);
      mcp230xx_int_counter[pinx]=0;
    }
  }
  snprintf_P(mqtt_data,sizeof(mqtt_data),PSTR("%s\"END\":1}}"),mqtt_data);
  MqttPublishPrefixTopic_P(TELE, PSTR(D_RSLT_SENSOR), Settings.flag.mqtt_sensor_retain);
  mcp230xx_int_sec_counter = 0;
}






boolean Xsns29(byte function)
{
  boolean result = false;

  if (i2c_flg) {
    switch (function) {
      case FUNC_MQTT_DATA:
        break;
      case FUNC_EVERY_SECOND:
        MCP230xx_Detect();
        if (mcp230xx_int_counter_en) {
          mcp230xx_int_sec_counter++;
          if (mcp230xx_int_sec_counter >= Settings.mcp230xx_int_timer) {
            MCP230xx_Interrupt_Counter_Report();
          }
        }
#ifdef USE_MCP230xx_OUTPUT
        if (tele_period == 0) {
          MCP230xx_OutputTelemetry();
        }
#endif
        break;
      case FUNC_EVERY_50_MSECOND:
        if ((mcp230xx_int_en) && (mcp230xx_type)) {
          mcp230xx_int_prio_counter++;
          if ((mcp230xx_int_prio_counter) >= (Settings.mcp230xx_int_prio)) {
            MCP230xx_CheckForInterrupt();
            mcp230xx_int_prio_counter=0;
          }
        }
        break;
      case FUNC_JSON_APPEND:
        MCP230xx_Show(1);
        break;
      case FUNC_COMMAND:
        if (XSNS_29 == XdrvMailbox.index) {
          result = MCP230xx_Command();
        }
        break;
#ifdef USE_WEBSERVER
#ifdef USE_MCP230xx_OUTPUT
#ifdef USE_MCP230xx_DISPLAYOUTPUT
      case FUNC_WEB_APPEND:
        MCP230xx_UpdateWebData();
        break;
#endif
#endif
#endif
      default:
        break;
    }
  }
  return result;
}

#endif
#endif
# 1 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_30_mpr121.ino"
# 43 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_30_mpr121.ino"
#ifdef USE_I2C
#ifdef USE_MPR121







#define MPR121_ELEX_REG 0x00


#define MPR121_MHDR_REG 0x2B


#define MPR121_MHDR_VAL 0x01


#define MPR121_NHDR_REG 0x2C


#define MPR121_NHDR_VAL 0x01


#define MPR121_NCLR_REG 0x2D


#define MPR121_NCLR_VAL 0x0E


#define MPR121_MHDF_REG 0x2F


#define MPR121_MHDF_VAL 0x01


#define MPR121_NHDF_REG 0x30


#define MPR121_NHDF_VAL 0x05


#define MPR121_NCLF_REG 0x31


#define MPR121_NCLF_VAL 0x01


#define MPR121_MHDPROXR_REG 0x36


#define MPR121_MHDPROXR_VAL 0x3F


#define MPR121_NHDPROXR_REG 0x37


#define MPR121_NHDPROXR_VAL 0x5F


#define MPR121_NCLPROXR_REG 0x38


#define MPR121_NCLPROXR_VAL 0x04


#define MPR121_FDLPROXR_REG 0x39


#define MPR121_FDLPROXR_VAL 0x00


#define MPR121_MHDPROXF_REG 0x3A


#define MPR121_MHDPROXF_VAL 0x01


#define MPR121_NHDPROXF_REG 0x3B


#define MPR121_NHDPROXF_VAL 0x01


#define MPR121_NCLPROXF_REG 0x3C


#define MPR121_NCLPROXF_VAL 0x1F


#define MPR121_FDLPROXF_REG 0x3D


#define MPR121_FDLPROXF_VAL 0x04


#define MPR121_E0TTH_REG 0x41


#define MPR121_E0TTH_VAL 12


#define MPR121_E0RTH_REG 0x42


#define MPR121_E0RTH_VAL 6


#define MPR121_CDT_REG 0x5D


#define MPR121_CDT_VAL 0x20


#define MPR121_ECR_REG 0x5E


#define MPR121_ECR_VAL 0x8F



#define MPR121_SRST_REG 0x80


#define MPR121_SRST_VAL 0x63


#define BITC(sensor,position) ((pS->current[sensor] >> position) & 1)


#define BITP(sensor,position) ((pS->previous[sensor] >> position) & 1)
# 185 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_30_mpr121.ino"
typedef struct mpr121 mpr121;
struct mpr121 {
 const uint8_t i2c_addr[4] = { 0x5A, 0x5B, 0x5C, 0x5D };
 const char id[4] = { 'A', 'B', 'C', 'D' };
 bool connected[4] = { false, false, false, false };
 bool running[4] = { false, false, false, false };
 uint16_t current[4] = { 0x0000, 0x0000, 0x0000, 0x0000 };
 uint16_t previous[4] = { 0x0000, 0x0000, 0x0000, 0x0000 };
};
# 205 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_30_mpr121.ino"
void Mpr121Init(struct mpr121 *pS)
{


 for (uint8_t i = 0; i < sizeof(pS->i2c_addr[i]); i++) {


  pS->connected[i] = (I2cWrite8(pS->i2c_addr[i], MPR121_SRST_REG, MPR121_SRST_VAL)
        && (0x24 == I2cRead8(pS->i2c_addr[i], 0x5D)));
  if (pS->connected[i]) {


   snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_I2C "MPR121(%c) " D_FOUND_AT " 0x%X"), pS->id[i], pS->i2c_addr[i]);
   AddLog(LOG_LEVEL_INFO);


   for (uint8_t j = 0; j < 13; j++) {


    I2cWrite8(pS->i2c_addr[i], MPR121_E0TTH_REG + 2 * j, MPR121_E0TTH_VAL);


    I2cWrite8(pS->i2c_addr[i], MPR121_E0RTH_REG + 2 * j, MPR121_E0RTH_VAL);
   }


   I2cWrite8(pS->i2c_addr[i], MPR121_MHDR_REG, MPR121_MHDR_VAL);


   I2cWrite8(pS->i2c_addr[i], MPR121_NHDR_REG, MPR121_NHDR_VAL);


   I2cWrite8(pS->i2c_addr[i], MPR121_NCLR_REG, MPR121_NCLR_VAL);


   I2cWrite8(pS->i2c_addr[i], MPR121_MHDF_REG, MPR121_MHDF_VAL);


   I2cWrite8(pS->i2c_addr[i], MPR121_NHDF_REG, MPR121_NHDF_VAL);


   I2cWrite8(pS->i2c_addr[i], MPR121_NCLF_REG, MPR121_NCLF_VAL);


   I2cWrite8(pS->i2c_addr[i], MPR121_MHDPROXR_REG, MPR121_MHDPROXR_VAL);


   I2cWrite8(pS->i2c_addr[i], MPR121_NHDPROXR_REG, MPR121_NHDPROXR_VAL);


   I2cWrite8(pS->i2c_addr[i], MPR121_NCLPROXR_REG, MPR121_NCLPROXR_VAL);


   I2cWrite8(pS->i2c_addr[i], MPR121_FDLPROXR_REG, MPR121_FDLPROXR_VAL);


   I2cWrite8(pS->i2c_addr[i], MPR121_MHDPROXF_REG, MPR121_MHDPROXF_VAL);


   I2cWrite8(pS->i2c_addr[i], MPR121_NHDPROXF_REG, MPR121_NHDPROXF_VAL);


   I2cWrite8(pS->i2c_addr[i], MPR121_NCLPROXF_REG, MPR121_NCLPROXF_VAL);


   I2cWrite8(pS->i2c_addr[i], MPR121_FDLPROXF_REG, MPR121_FDLPROXF_VAL);


   I2cWrite8(pS->i2c_addr[i], MPR121_CDT_REG, MPR121_CDT_VAL);


   I2cWrite8(pS->i2c_addr[i], MPR121_ECR_REG, MPR121_ECR_VAL);


   pS->running[i] = (0x00 != I2cRead8(pS->i2c_addr[i], MPR121_ECR_REG));
   if (pS->running[i]) {
    snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_I2C "MPR121%c: Running"), pS->id[i]);
   } else {
    snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_I2C "MPR121%c: NOT Running"), pS->id[i]);
   }
   AddLog(LOG_LEVEL_INFO);
  } else {


   pS->running[i] = false;
  }
 }


 if (!(pS->connected[0] || pS->connected[1] || pS->connected[2]
       || pS->connected[3])) {
  snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_I2C "MPR121: No sensors found"));
  AddLog(LOG_LEVEL_DEBUG);
 }
}
# 315 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_30_mpr121.ino"
void Mpr121Show(struct mpr121 *pS, byte function)
{


 for (uint8_t i = 0; i < sizeof(pS->i2c_addr[i]); i++) {


  if (pS->connected[i]) {


   if (!I2cValidRead16LE(&pS->current[i], pS->i2c_addr[i], MPR121_ELEX_REG)) {
    snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_I2C "MPR121%c: ERROR: Cannot read data!"), pS->id[i]);
    AddLog(LOG_LEVEL_ERROR);
    Mpr121Init(pS);
    return;
   }

   if (BITC(i, 15)) {


    I2cWrite8(pS->i2c_addr[i], MPR121_ELEX_REG, 0x00);
    snprintf_P(log_data, sizeof(log_data),
        PSTR(D_LOG_I2C "MPR121%c: ERROR: Excess current detected! Fix circuits if it happens repeatedly! Soft-resetting MPR121 ..."), pS->id[i]);
    AddLog(LOG_LEVEL_ERROR);
    Mpr121Init(pS);
    return;
   }
  }

  if (pS->running[i]) {


   if (FUNC_JSON_APPEND == function) {
    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s,\"MPR121%c\":{"), mqtt_data, pS->id[i]);
   }

   for (uint8_t j = 0; j < 13; j++) {


    if ((FUNC_EVERY_50_MSECOND == function)
        && (BITC(i, j) != BITP(i, j))) {
     snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"MPR121%c\":{\"Button%i\":%i}}"), pS->id[i], j, BITC(i, j));
     MqttPublishPrefixTopic_P(RESULT_OR_STAT, mqtt_data);
    }

#ifdef USE_WEBSERVER
    if (FUNC_WEB_APPEND == function) {
     snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s{s}MPR121%c Button%d{m}%d{e}"), mqtt_data, pS->id[i], j, BITC(i, j));
    }
#endif


    if (FUNC_JSON_APPEND == function) {
     snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s%s\"Button%i\":%i"), mqtt_data, (j > 0 ? "," : ""), j, BITC(i, j));
    }
   }


   pS->previous[i] = pS->current[i];


   if (FUNC_JSON_APPEND == function) {
    snprintf_P(mqtt_data, sizeof(mqtt_data), "%s}", mqtt_data);
   }
  }
 }
}
# 391 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_30_mpr121.ino"
#define XSNS_30 
# 408 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_30_mpr121.ino"
boolean Xsns30(byte function)
{

 boolean result = false;


 static struct mpr121 mpr121;


 if (i2c_flg) {
  switch (function) {


  case FUNC_INIT:
   Mpr121Init(&mpr121);
   break;


  case FUNC_EVERY_50_MSECOND:
   Mpr121Show(&mpr121, FUNC_EVERY_50_MSECOND);
   break;


  case FUNC_JSON_APPEND:
   Mpr121Show(&mpr121, FUNC_JSON_APPEND);
   break;

#ifdef USE_WEBSERVER

  case FUNC_WEB_APPEND:
   Mpr121Show(&mpr121, FUNC_WEB_APPEND);
   break;
#endif
  }
 }

 return result;
}

#endif
#endif
# 1 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_31_ccs811.ino"
# 20 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_31_ccs811.ino"
#ifdef USE_I2C
#ifdef USE_CCS811
# 30 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_31_ccs811.ino"
#include "Adafruit_CCS811.h"

Adafruit_CCS811 ccs;
uint8_t CCS811_ready;
uint8_t CCS811_type;
uint16_t eCO2;
uint16_t TVOC;
uint8_t tcnt = 0;
uint8_t ecnt = 0;


#define EVERYNSECONDS 5

void CCS811Update()
{
  tcnt++;
  if (tcnt >= EVERYNSECONDS) {
    tcnt = 0;
    CCS811_ready = 0;
    if (!CCS811_type) {
      sint8_t res = ccs.begin(CCS811_ADDRESS);
      if (!res) {
        CCS811_type = 1;
        snprintf_P(log_data, sizeof(log_data), S_LOG_I2C_FOUND_AT, "CCS811", 0x5A);
        AddLog(LOG_LEVEL_DEBUG);
      } else {


      }
    } else {
      if (ccs.available()) {
        if (!ccs.readData()){
          TVOC = ccs.getTVOC();
          eCO2 = ccs.geteCO2();
          CCS811_ready = 1;
          if (global_update) { ccs.setEnvironmentalData((uint8_t)global_humidity, global_temperature); }
          ecnt = 0;
        }
      } else {

        ecnt++;
        if (ecnt > 6) {

          ccs.begin(CCS811_ADDRESS);
        }
      }
    }
  }
}

const char HTTP_SNS_CCS811[] PROGMEM = "%s"
  "{s}CCS811 " D_ECO2 "{m}%d " D_UNIT_PARTS_PER_MILLION "{e}"
  "{s}CCS811 " D_TVOC "{m}%d " D_UNIT_PARTS_PER_BILLION "{e}";

void CCS811Show(boolean json)
{
  if (CCS811_ready) {
    if (json) {
      snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s,\"CCS811\":{\"" D_JSON_ECO2 "\":%d,\"" D_JSON_TVOC "\":%d}"), mqtt_data,eCO2,TVOC);
#ifdef USE_DOMOTICZ
      if (0 == tele_period) DomoticzSensor(DZ_AIRQUALITY, eCO2);
#endif
#ifdef USE_WEBSERVER
    } else {
      snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_SNS_CCS811, mqtt_data, eCO2, TVOC);
#endif
    }
  }
}





#define XSNS_31 

boolean Xsns31(byte function)
{
  boolean result = false;

  if (i2c_flg) {
    switch (function) {
      case FUNC_EVERY_SECOND:
        CCS811Update();
        break;
      case FUNC_JSON_APPEND:
        CCS811Show(1);
        break;
#ifdef USE_WEBSERVER
      case FUNC_WEB_APPEND:
        CCS811Show(0);
        break;
#endif
    }
  }
  return result;
}

#endif
#endif
# 1 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_32_mpu6050.ino"
# 20 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_32_mpu6050.ino"
#ifdef USE_I2C
#ifdef USE_MPU6050
# 30 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_32_mpu6050.ino"
#define D_SENSOR_MPU6050 "MPU6050"

#define MPU_6050_ADDR_AD0_LOW 0x68
#define MPU_6050_ADDR_AD0_HIGH 0x69

uint8_t MPU_6050_address;
uint8_t MPU_6050_addresses[] = { MPU_6050_ADDR_AD0_LOW, MPU_6050_ADDR_AD0_HIGH };
uint8_t MPU_6050_found;

int16_t MPU_6050_ax = 0, MPU_6050_ay = 0, MPU_6050_az = 0;
int16_t MPU_6050_gx = 0, MPU_6050_gy = 0, MPU_6050_gz = 0;
int16_t MPU_6050_temperature = 0;

#include <MPU6050.h>
MPU6050 mpu6050;

void MPU_6050PerformReading()
{
  mpu6050.getMotion6(
    &MPU_6050_ax,
    &MPU_6050_ay,
    &MPU_6050_az,
    &MPU_6050_gx,
    &MPU_6050_gy,
    &MPU_6050_gz
  );

  MPU_6050_temperature = mpu6050.getTemperature();
}
# 76 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_32_mpu6050.ino"
void MPU_6050Detect()
{
  if (MPU_6050_found)
  {
    return;
  }

  for (byte i = 0; i < sizeof(MPU_6050_addresses); i++)
  {
    MPU_6050_address = MPU_6050_addresses[i];

    mpu6050.setAddr(MPU_6050_address);
    mpu6050.initialize();

    Settings.flag2.axis_resolution = 2;

    MPU_6050_found = mpu6050.testConnection();
  }

  if (MPU_6050_found)
  {
    snprintf_P(log_data, sizeof(log_data), S_LOG_I2C_FOUND_AT, D_SENSOR_MPU6050, MPU_6050_address);
    AddLog(LOG_LEVEL_DEBUG);
  }
}

#ifdef USE_WEBSERVER
const char HTTP_SNS_AX_AXIS[] PROGMEM = "%s{s}%s " D_AX_AXIS "{m}%s{e}";
const char HTTP_SNS_AY_AXIS[] PROGMEM = "%s{s}%s " D_AY_AXIS "{m}%s{e}";
const char HTTP_SNS_AZ_AXIS[] PROGMEM = "%s{s}%s " D_AZ_AXIS "{m}%s{e}";
const char HTTP_SNS_GX_AXIS[] PROGMEM = "%s{s}%s " D_GX_AXIS "{m}%s{e}";
const char HTTP_SNS_GY_AXIS[] PROGMEM = "%s{s}%s " D_GY_AXIS "{m}%s{e}";
const char HTTP_SNS_GZ_AXIS[] PROGMEM = "%s{s}%s " D_GZ_AXIS "{m}%s{e}";
#endif

#define D_JSON_AXIS_AX "AccelXAxis"
#define D_JSON_AXIS_AY "AccelYAxis"
#define D_JSON_AXIS_AZ "AccelZAxis"
#define D_JSON_AXIS_GX "GyroXAxis"
#define D_JSON_AXIS_GY "GyroYAxis"
#define D_JSON_AXIS_GZ "GyroZAxis"

void MPU_6050Show(boolean json)
{
  double tempConv = (MPU_6050_temperature / 340.0 + 35.53);

  if (MPU_6050_found) {
    MPU_6050PerformReading();

    char temperature[10];
    dtostrfd(tempConv, Settings.flag2.temperature_resolution, temperature);
    char axis_ax[10];
    dtostrfd(MPU_6050_ax, Settings.flag2.axis_resolution, axis_ax);
    char axis_ay[10];
    dtostrfd(MPU_6050_ay, Settings.flag2.axis_resolution, axis_ay);
    char axis_az[10];
    dtostrfd(MPU_6050_az, Settings.flag2.axis_resolution, axis_az);
    char axis_gx[10];
    dtostrfd(MPU_6050_gx, Settings.flag2.axis_resolution, axis_gx);
    char axis_gy[10];
    dtostrfd(MPU_6050_gy, Settings.flag2.axis_resolution, axis_gy);
    char axis_gz[10];
    dtostrfd(MPU_6050_gz, Settings.flag2.axis_resolution, axis_gz);

    if (json) {
      char json_axis_ax[40];
      snprintf_P(json_axis_ax, sizeof(json_axis_ax), PSTR(",\"" D_JSON_AXIS_AX "\":%s"), axis_ax);
      char json_axis_ay[40];
      snprintf_P(json_axis_ay, sizeof(json_axis_ay), PSTR(",\"" D_JSON_AXIS_AY "\":%s"), axis_ay);
      char json_axis_az[40];
      snprintf_P(json_axis_az, sizeof(json_axis_az), PSTR(",\"" D_JSON_AXIS_AZ "\":%s"), axis_az);
      char json_axis_gx[40];
      snprintf_P(json_axis_gx, sizeof(json_axis_gx), PSTR(",\"" D_JSON_AXIS_GX "\":%s"), axis_gx);
      char json_axis_gy[40];
      snprintf_P(json_axis_gy, sizeof(json_axis_gy), PSTR(",\"" D_JSON_AXIS_GY "\":%s"), axis_gy);
      char json_axis_gz[40];
      snprintf_P(json_axis_gz, sizeof(json_axis_gz), PSTR(",\"" D_JSON_AXIS_GZ "\":%s"), axis_gz);
      snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s,\"%s\":{\"" D_JSON_TEMPERATURE "\":%s%s%s%s%s%s%s,\"}"),
        mqtt_data, D_SENSOR_MPU6050, temperature, json_axis_ax, json_axis_ay, json_axis_az, json_axis_gx, json_axis_gy, json_axis_gz);
#ifdef USE_DOMOTICZ
      DomoticzTempHumSensor(temperature, 0);
#endif
#ifdef USE_WEBSERVER
    } else {
      snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_SNS_TEMP, mqtt_data, D_SENSOR_MPU6050, temperature, TempUnit());
      snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_SNS_AX_AXIS, mqtt_data, D_SENSOR_MPU6050, axis_ax);
      snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_SNS_AY_AXIS, mqtt_data, D_SENSOR_MPU6050, axis_ay);
      snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_SNS_AZ_AXIS, mqtt_data, D_SENSOR_MPU6050, axis_az);
      snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_SNS_GX_AXIS, mqtt_data, D_SENSOR_MPU6050, axis_gx);
      snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_SNS_GY_AXIS, mqtt_data, D_SENSOR_MPU6050, axis_gy);
      snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_SNS_GZ_AXIS, mqtt_data, D_SENSOR_MPU6050, axis_gz);
#endif
    }
  }
}





#define XSNS_32 

boolean Xsns32(byte function)
{
  boolean result = false;

  if (i2c_flg) {
    switch (function) {
      case FUNC_PREP_BEFORE_TELEPERIOD:
        MPU_6050Detect();
        break;
      case FUNC_EVERY_SECOND:
        if (tele_period == Settings.tele_period -3) {
          MPU_6050PerformReading();
        }
        break;
      case FUNC_JSON_APPEND:
        MPU_6050Show(1);
        break;
#ifdef USE_WEBSERVER
      case FUNC_WEB_APPEND:
        MPU_6050Show(0);
        MPU_6050PerformReading();
        break;
#endif
    }
  }
  return result;
}

#endif
#endif
# 1 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_33_ds3231.ino"
# 20 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_33_ds3231.ino"
#ifdef USE_I2C
#ifdef USE_DS3231
# 36 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_33_ds3231.ino"
#ifndef USE_RTC_ADDR
  #define USE_RTC_ADDR 0x68
#endif


#define RTC_SECONDS 0x00
#define RTC_MINUTES 0x01
#define RTC_HOURS 0x02
#define RTC_DAY 0x03
#define RTC_DATE 0x04
#define RTC_MONTH 0x05
#define RTC_YEAR 0x06
#define RTC_CONTROL 0x0E
#define RTC_STATUS 0x0F

#define OSF 7
#define EOSC 7
#define BBSQW 6
#define CONV 5
#define RS2 4
#define RS1 3
#define INTCN 2


#define HR1224 6
#define CENTURY 7
#define DYDT 6
boolean ds3231ReadStatus = false , ds3231WriteStatus = false;
boolean DS3231chipDetected;





boolean DS3231Detect()
{
  if (I2cValidRead(USE_RTC_ADDR, RTC_STATUS, 1))
  {
    snprintf_P(log_data, sizeof(log_data), S_LOG_I2C_FOUND_AT, "DS3231", USE_RTC_ADDR);
    AddLog(LOG_LEVEL_INFO);
    return true;
  }
  else
  {
    snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_I2C "%s *NOT* " D_FOUND_AT " 0x%x"), "DS3231", USE_RTC_ADDR);
    AddLog(LOG_LEVEL_INFO);
    return false;
  }
}




uint8_t bcd2dec(uint8_t n)
{
  return n - 6 * (n >> 4);
}




uint8_t dec2bcd(uint8_t n)
{
  return n + 6 * (n / 10);
}




uint32_t ReadFromDS3231()
{
  TIME_T tm;
  tm.second = bcd2dec(I2cRead8(USE_RTC_ADDR, RTC_SECONDS));
  tm.minute = bcd2dec(I2cRead8(USE_RTC_ADDR, RTC_MINUTES));
  tm.hour = bcd2dec(I2cRead8(USE_RTC_ADDR, RTC_HOURS) & ~_BV(HR1224));
  tm.day_of_week = I2cRead8(USE_RTC_ADDR, RTC_DAY);
  tm.day_of_month = bcd2dec(I2cRead8(USE_RTC_ADDR, RTC_DATE));
  tm.month = bcd2dec(I2cRead8(USE_RTC_ADDR, RTC_MONTH) & ~_BV(CENTURY));
  tm.year = bcd2dec(I2cRead8(USE_RTC_ADDR, RTC_YEAR));
  return MakeTime(tm);
}



void SetDS3231Time (uint32_t epoch_time) {
  TIME_T tm;
  BreakTime(epoch_time, tm);
  I2cWrite8(USE_RTC_ADDR, RTC_SECONDS, dec2bcd(tm.second));
  I2cWrite8(USE_RTC_ADDR, RTC_MINUTES, dec2bcd(tm.minute));
  I2cWrite8(USE_RTC_ADDR, RTC_HOURS, dec2bcd(tm.hour));
  I2cWrite8(USE_RTC_ADDR, RTC_DAY, tm.day_of_week);
  I2cWrite8(USE_RTC_ADDR, RTC_DATE, dec2bcd(tm.day_of_month));
  I2cWrite8(USE_RTC_ADDR, RTC_MONTH, dec2bcd(tm.month));
  I2cWrite8(USE_RTC_ADDR, RTC_YEAR, dec2bcd(tm.year));
  I2cWrite8(USE_RTC_ADDR, RTC_STATUS, I2cRead8(USE_RTC_ADDR, RTC_STATUS) & ~_BV(OSF));
}





#define XSNS_33 

boolean Xsns33(byte function)
{
  boolean result = false;

  if (i2c_flg) {
    switch (function) {
      case FUNC_INIT:
        DS3231chipDetected = DS3231Detect();
        result = DS3231chipDetected;
        break;

      case FUNC_EVERY_SECOND:
        TIME_T tmpTime;
        if (!ds3231ReadStatus && DS3231chipDetected && utc_time < 1451602800 ) {
          ntp_force_sync = 1;
          utc_time = ReadFromDS3231();


          BreakTime(utc_time, tmpTime);
          if (utc_time < 1451602800 ) {
            ds3231ReadStatus = true;
          }
          RtcTime.year = tmpTime.year + 1970;
          daylight_saving_time = RuleToTime(Settings.tflag[1], RtcTime.year);
          standard_time = RuleToTime(Settings.tflag[0], RtcTime.year);
          snprintf_P(log_data, sizeof(log_data), PSTR("Set time from DS3231 to RTC (" D_UTC_TIME ") %s, (" D_DST_TIME ") %s, (" D_STD_TIME ") %s"),
                     GetTime(0).c_str(), GetTime(2).c_str(), GetTime(3).c_str());
          AddLog(LOG_LEVEL_INFO);
          if (local_time < 1451602800) {
            rules_flag.time_init = 1;
          } else {
            rules_flag.time_set = 1;
          }
          result = true;
        }
        else if (!ds3231WriteStatus && DS3231chipDetected && utc_time > 1451602800 && abs(utc_time - ReadFromDS3231()) > 60) {
          snprintf_P(log_data, sizeof(log_data), PSTR("Write Time TO DS3231 from NTP (" D_UTC_TIME ") %s, (" D_DST_TIME ") %s, (" D_STD_TIME ") %s"),
                     GetTime(0).c_str(), GetTime(2).c_str(), GetTime(3).c_str());
          AddLog(LOG_LEVEL_INFO);
          SetDS3231Time (utc_time);
          ds3231WriteStatus = true;
        }
        else {
          result = false;
        }
        break;
    }
  }
  return result;
}

#endif
#endif
# 1 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_34_hx711.ino"
# 20 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_34_hx711.ino"
#ifdef USE_HX711
# 35 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_34_hx711.ino"
#define XSNS_34 34

#ifndef HX_MAX_WEIGHT
#define HX_MAX_WEIGHT 20000
#endif
#ifndef HX_REFERENCE
#define HX_REFERENCE 250
#endif
#ifndef HX_SCALE
#define HX_SCALE 120
#endif

#define HX_TIMEOUT 120
#define HX_SAMPLES 10
#define HX_CAL_TIMEOUT 15

#define HX_GAIN_128 1
#define HX_GAIN_32 2
#define HX_GAIN_64 3

#define D_JSON_WEIGHT_REF "WeightRef"
#define D_JSON_WEIGHT_CAL "WeightCal"
#define D_JSON_WEIGHT_MAX "WeightMax"
#define D_JSON_WEIGHT_ITEM "WeightItem"

enum HxCalibrationSteps { HX_CAL_END, HX_CAL_LIMBO, HX_CAL_FINISH, HX_CAL_FAIL, HX_CAL_DONE, HX_CAL_FIRST, HX_CAL_RESET, HX_CAL_START };

const char kHxCalibrationStates[] PROGMEM = D_HX_CAL_FAIL "|" D_HX_CAL_DONE "|" D_HX_CAL_REFERENCE "|" D_HX_CAL_REMOVE;

long hx_weight = 0;
long hx_sum_weight = 0;
long hx_offset = 0;
long hx_scale = 1;
uint8_t hx_type = 1;
uint8_t hx_sample_count = 0;
uint8_t hx_tare_flg = 0;
uint8_t hx_calibrate_step = HX_CAL_END;
uint8_t hx_calibrate_timer = 0;
uint8_t hx_calibrate_msg = 0;
uint8_t hx_pin_sck;
uint8_t hx_pin_dout;



bool HxIsReady(uint16_t timeout)
{

  uint32_t start = millis();
  while ((digitalRead(hx_pin_dout) == HIGH) && (millis() - start < timeout)) { yield(); }
  return (digitalRead(hx_pin_dout) == LOW);
}

long HxRead()
{
  if (!HxIsReady(HX_TIMEOUT)) { return -1; }

  uint8_t data[3] = { 0 };
  uint8_t filler = 0x00;


  data[2] = shiftIn(hx_pin_dout, hx_pin_sck, MSBFIRST);
  data[1] = shiftIn(hx_pin_dout, hx_pin_sck, MSBFIRST);
  data[0] = shiftIn(hx_pin_dout, hx_pin_sck, MSBFIRST);


  for (unsigned int i = 0; i < HX_GAIN_128; i++) {
    digitalWrite(hx_pin_sck, HIGH);
    digitalWrite(hx_pin_sck, LOW);
  }


  if (data[2] & 0x80) { filler = 0xFF; }


  unsigned long value = ( static_cast<unsigned long>(filler) << 24
                        | static_cast<unsigned long>(data[2]) << 16
                        | static_cast<unsigned long>(data[1]) << 8
                        | static_cast<unsigned long>(data[0]) );

  return static_cast<long>(value);
}



void HxReset()
{
  hx_tare_flg = 1;
  hx_sum_weight = 0;
  hx_sample_count = 0;
}

void HxCalibrationStateTextJson(uint8_t msg_id)
{
  char cal_text[30];

  hx_calibrate_msg = msg_id;
  snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_SENSOR_INDEX_SVALUE, XSNS_34, GetTextIndexed(cal_text, sizeof(cal_text), hx_calibrate_msg, kHxCalibrationStates));

  if (msg_id < 3) { MqttPublishPrefixTopic_P(RESULT_OR_STAT, PSTR("Sensor34")); }
}
# 152 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_34_hx711.ino"
bool HxCommand()
{
  bool serviced = true;
  bool show_parms = false;
  char sub_string[XdrvMailbox.data_len +1];

  for (byte ca = 0; ca < XdrvMailbox.data_len; ca++) {
    if ((' ' == XdrvMailbox.data[ca]) || ('=' == XdrvMailbox.data[ca])) { XdrvMailbox.data[ca] = ','; }
  }

  switch (XdrvMailbox.payload) {
    case 1:
      HxReset();
      snprintf_P(mqtt_data, sizeof(mqtt_data), S_JSON_SENSOR_INDEX_SVALUE, XSNS_34, "Reset");
      break;
    case 2:
      if (strstr(XdrvMailbox.data, ",")) {
        Settings.weight_reference = strtol(subStr(sub_string, XdrvMailbox.data, ",", 2), NULL, 10);
      }
      hx_scale = 1;
      HxReset();
      hx_calibrate_step = HX_CAL_START;
      hx_calibrate_timer = 1;
      HxCalibrationStateTextJson(3);
      break;
    case 3:
      if (strstr(XdrvMailbox.data, ",")) {
        Settings.weight_reference = strtol(subStr(sub_string, XdrvMailbox.data, ",", 2), NULL, 10);
      }
      show_parms = true;
      break;
    case 4:
      if (strstr(XdrvMailbox.data, ",")) {
        Settings.weight_calibration = strtol(subStr(sub_string, XdrvMailbox.data, ",", 2), NULL, 10);
        hx_scale = Settings.weight_calibration;
      }
      show_parms = true;
      break;
    case 5:
      if (strstr(XdrvMailbox.data, ",")) {
        Settings.weight_max = strtol(subStr(sub_string, XdrvMailbox.data, ",", 2), NULL, 10) / 1000;
      }
      show_parms = true;
      break;
    case 6:
      if (strstr(XdrvMailbox.data, ",")) {
        Settings.weight_item = (unsigned long)(CharToDouble(subStr(sub_string, XdrvMailbox.data, ",", 2)) * 10);
      }
      show_parms = true;
      break;
    default:
      serviced = false;
  }

  if (show_parms) {
    char item[10];
    dtostrfd((float)Settings.weight_item / 10, 1, item);
    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"Sensor34\":{\"" D_JSON_WEIGHT_REF "\":%d,\"" D_JSON_WEIGHT_CAL "\":%d,\"" D_JSON_WEIGHT_MAX "\":%d,\"" D_JSON_WEIGHT_ITEM "\":%s}}"),
      Settings.weight_reference, Settings.weight_calibration, Settings.weight_max * 1000, item);
  }

  return serviced;
}



long HxWeight()
{
  return (hx_calibrate_step < HX_CAL_FAIL) ? hx_weight : 0;
}

void HxInit()
{
  hx_type = 0;
  if ((pin[GPIO_HX711_DAT] < 99) && (pin[GPIO_HX711_SCK] < 99)) {
    hx_pin_sck = pin[GPIO_HX711_SCK];
    hx_pin_dout = pin[GPIO_HX711_DAT];

    pinMode(hx_pin_sck, OUTPUT);
    pinMode(hx_pin_dout, INPUT);

    digitalWrite(hx_pin_sck, LOW);

    if (HxIsReady(8 * HX_TIMEOUT)) {
      if (!Settings.weight_max) { Settings.weight_max = HX_MAX_WEIGHT / 1000; }
      if (!Settings.weight_calibration) { Settings.weight_calibration = HX_SCALE; }
      if (!Settings.weight_reference) { Settings.weight_reference = HX_REFERENCE; }
      hx_scale = Settings.weight_calibration;
      HxRead();
      HxReset();

      hx_type = 1;
    }
  }
}

void HxEvery100mSecond()
{
  hx_sum_weight += HxRead();

  hx_sample_count++;
  if (HX_SAMPLES == hx_sample_count) {
    long average = hx_sum_weight / hx_sample_count;
    long value = average - hx_offset;
    hx_weight = value / hx_scale;
    if (hx_weight < 0) { hx_weight = 0; }

    if (hx_tare_flg) {
      hx_tare_flg = 0;
      hx_offset = average;
    }

    if (hx_calibrate_step) {
      hx_calibrate_timer--;

      if (HX_CAL_START == hx_calibrate_step) {
        hx_calibrate_step--;
        hx_calibrate_timer = HX_CAL_TIMEOUT * (10 / HX_SAMPLES);
      }
      else if (HX_CAL_RESET == hx_calibrate_step) {
        if (hx_calibrate_timer) {
          if (hx_weight < Settings.weight_reference) {
            hx_calibrate_step--;
            hx_calibrate_timer = HX_CAL_TIMEOUT * (10 / HX_SAMPLES);
            HxCalibrationStateTextJson(2);
          }
        } else {
          hx_calibrate_step = HX_CAL_FAIL;
        }
      }
      else if (HX_CAL_FIRST == hx_calibrate_step) {
        if (hx_calibrate_timer) {
          if (hx_weight > Settings.weight_reference) {
            hx_calibrate_step--;
          }
        } else {
          hx_calibrate_step = HX_CAL_FAIL;
        }
      }
      else if (HX_CAL_DONE == hx_calibrate_step) {
        if (hx_weight > Settings.weight_reference) {
          hx_calibrate_step = HX_CAL_FINISH;
          Settings.weight_calibration = hx_weight / Settings.weight_reference;
          hx_weight = 0;
          HxCalibrationStateTextJson(1);
        } else {
          hx_calibrate_step = HX_CAL_FAIL;
        }
      }

      if (HX_CAL_FAIL == hx_calibrate_step) {
        hx_calibrate_step--;
        hx_tare_flg = 1;
        HxCalibrationStateTextJson(0);
      }
      if (HX_CAL_FINISH == hx_calibrate_step) {
        hx_calibrate_step--;
        hx_calibrate_timer = 3 * (10 / HX_SAMPLES);
        hx_scale = Settings.weight_calibration;
      }

      if (!hx_calibrate_timer) {
        hx_calibrate_step = HX_CAL_END;
      }
    }

    hx_sum_weight = 0;
    hx_sample_count = 0;
  }
}

#ifdef USE_WEBSERVER
const char HTTP_HX711_WEIGHT[] PROGMEM = "%s"
  "{s}HX711 " D_WEIGHT "{m}%s " D_UNIT_KILOGRAM "{e}";
const char HTTP_HX711_COUNT[] PROGMEM = "%s"
  "{s}HX711 " D_COUNT "{m}%d{e}";
const char HTTP_HX711_CAL[] PROGMEM = "%s"
  "{s}HX711 %s{m}{e}";
#endif

void HxShow(boolean json)
{
  char weight_chr[10];
  char scount[30] = { 0 };

  uint16_t count = 0;
  float weight = 0;
  if (hx_calibrate_step < HX_CAL_FAIL) {
    if (hx_weight && Settings.weight_item) {
      count = (hx_weight * 10) / Settings.weight_item;
      if (count > 1) {
        snprintf_P(scount, sizeof(scount), PSTR(",\"" D_JSON_COUNT "\":%d"), count);
      }
    }
    weight = (float)hx_weight / 1000;
  }
  dtostrfd(weight, Settings.flag2.weight_resolution, weight_chr);

  if (json) {
    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s,\"HX711\":{\"" D_JSON_WEIGHT "\":%s%s}"), mqtt_data, weight_chr, scount);
#ifdef USE_WEBSERVER
  } else {
    snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_HX711_WEIGHT, mqtt_data, weight_chr);
    if (count > 1) {
      snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_HX711_COUNT, mqtt_data, count);
    }
    if (hx_calibrate_step) {
      char cal_text[30];
      snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_HX711_CAL, mqtt_data, GetTextIndexed(cal_text, sizeof(cal_text), hx_calibrate_msg, kHxCalibrationStates));
    }
#endif
  }
}

#ifdef USE_WEBSERVER
#ifdef USE_HX711_GUI




#define WEB_HANDLE_HX711 "s34"

const char S_CONFIGURE_HX711[] PROGMEM = D_CONFIGURE_HX711;

const char HTTP_BTN_MENU_MAIN_HX711[] PROGMEM =
  "<br/><form action='" WEB_HANDLE_HX711 "' method='get'><button name='reset'>" D_RESET_HX711 "</button></form>";

const char HTTP_BTN_MENU_HX711[] PROGMEM =
  "<br/><form action='" WEB_HANDLE_HX711 "' method='get'><button>" D_CONFIGURE_HX711 "</button></form>";

const char HTTP_FORM_HX711[] PROGMEM =
  "<fieldset><legend><b>&nbsp;" D_CALIBRATION "&nbsp;</b></legend>"
  "<form method='post' action='" WEB_HANDLE_HX711 "'>"
  "<br/><b>" D_REFERENCE_WEIGHT "</b> (" D_UNIT_KILOGRAM ")<br/><input type='number' step='0.001' id='p1' name='p1' placeholder='0' value='{1'><br/>"
  "<br/><button name='calibrate' type='submit'>" D_CALIBRATE "</button><br/>"
  "</form>"
  "</fieldset><br/><br/>"

  "<fieldset><legend><b>&nbsp;" D_HX711_PARAMETERS "&nbsp;</b></legend>"
  "<form method='post' action='" WEB_HANDLE_HX711 "'>"
  "<br/><b>" D_ITEM_WEIGHT "</b> (" D_UNIT_KILOGRAM ")<br/><input type='number' max='6.5535' step='0.0001' id='p2' name='p2' placeholder='0.0' value='{2'><br/>";

void HandleHxAction()
{
  if (HttpUser()) { return; }
  if (!WebAuthenticate()) { return WebServer->requestAuthentication(); }
  AddLog_P(LOG_LEVEL_DEBUG, S_LOG_HTTP, S_CONFIGURE_HX711);

  if (WebServer->hasArg("save")) {
    HxSaveSettings();
    HandleConfiguration();
    return;
  }

  char tmp[100];

  if (WebServer->hasArg("reset")) {
    snprintf_P(tmp, sizeof(tmp), PSTR("Sensor34 1"));
    ExecuteWebCommand(tmp, SRC_WEBGUI);

    HandleRoot();
    return;
  }

  if (WebServer->hasArg("calibrate")) {
    WebGetArg("p1", tmp, sizeof(tmp));
    Settings.weight_reference = (!strlen(tmp)) ? 0 : (unsigned long)(CharToDouble(tmp) * 1000);

    HxLogUpdates();

    snprintf_P(tmp, sizeof(tmp), PSTR("Sensor34 2"));
    ExecuteWebCommand(tmp, SRC_WEBGUI);

    HandleRoot();
    return;
  }

  String page = FPSTR(HTTP_HEAD);
  page.replace(F("{v}"), FPSTR(D_CONFIGURE_HX711));
  page += FPSTR(HTTP_HEAD_STYLE);
  page += FPSTR(HTTP_FORM_HX711);
  dtostrfd((float)Settings.weight_reference / 1000, 3, tmp);
  page.replace("{1", String(tmp));
  dtostrfd((float)Settings.weight_item / 10000, 4, tmp);
  page.replace("{2", String(tmp));

  page += FPSTR(HTTP_FORM_END);
  page += FPSTR(HTTP_BTN_CONF);
  ShowPage(page);
}

void HxSaveSettings()
{
  char tmp[100];

  WebGetArg("p2", tmp, sizeof(tmp));
  Settings.weight_item = (!strlen(tmp)) ? 0 : (unsigned long)(CharToDouble(tmp) * 10000);

  HxLogUpdates();
}

void HxLogUpdates()
{
  char weigth_ref_chr[10];
  char weigth_item_chr[10];

  dtostrfd((float)Settings.weight_reference / 1000, Settings.flag2.weight_resolution, weigth_ref_chr);
  dtostrfd((float)Settings.weight_item / 10000, 4, weigth_item_chr);

  snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_WIFI D_JSON_WEIGHT_REF " %s, " D_JSON_WEIGHT_ITEM " %s"),
    weigth_ref_chr, weigth_item_chr);
  AddLog(LOG_LEVEL_INFO);
}

#endif
#endif





boolean Xsns34(byte function)
{
  boolean result = false;

  if (hx_type) {
    switch (function) {
      case FUNC_INIT:
        HxInit();
        break;
      case FUNC_EVERY_100_MSECOND:
        HxEvery100mSecond();
        break;
      case FUNC_COMMAND:
        if (XSNS_34 == XdrvMailbox.index) {
          result = HxCommand();
        }
        break;
      case FUNC_JSON_APPEND:
        HxShow(1);
        break;
#ifdef USE_WEBSERVER
      case FUNC_WEB_APPEND:
        HxShow(0);
        break;
#ifdef USE_HX711_GUI
      case FUNC_WEB_ADD_MAIN_BUTTON:
        strncat_P(mqtt_data, HTTP_BTN_MENU_MAIN_HX711, sizeof(mqtt_data));
        break;
      case FUNC_WEB_ADD_BUTTON:
        strncat_P(mqtt_data, HTTP_BTN_MENU_HX711, sizeof(mqtt_data));
        break;
      case FUNC_WEB_ADD_HANDLER:
        WebServer->on("/" WEB_HANDLE_HX711, HandleHxAction);
        break;
#endif
#endif
    }
  }
  return result;
}

#endif
# 1 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_35_tx20.ino"
# 20 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_35_tx20.ino"
#ifdef USE_TX20_WIND_SENSOR
# 29 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_35_tx20.ino"
#define TX20_BIT_TIME 1220
#define TX20_RESET_VALUES 60



extern "C" {
#include "gpio.h"
}

#ifdef USE_WEBSERVER

const char HTTP_SNS_TX20[] PROGMEM = "%s"
   "{s}TX20 " D_TX20_WIND_SPEED "{m}%s " D_UNIT_KILOMETER_PER_HOUR "{e}"
   "{s}TX20 " D_TX20_WIND_SPEED_AVG "{m}%s " D_UNIT_KILOMETER_PER_HOUR "{e}"
   "{s}TX20 " D_TX20_WIND_SPEED_MAX "{m}%s " D_UNIT_KILOMETER_PER_HOUR "{e}"
   "{s}TX20 " D_TX20_WIND_DIRECTION "{m}%s{e}";

#endif

const char kTx20Directions[] PROGMEM = D_TX20_NORTH "|"
                                       D_TX20_NORTH D_TX20_NORTH D_TX20_EAST "|"
                                       D_TX20_NORTH D_TX20_EAST "|"
                                       D_TX20_EAST D_TX20_NORTH D_TX20_EAST "|"
                                       D_TX20_EAST "|"
                                       D_TX20_EAST D_TX20_SOUTH D_TX20_EAST "|"
                                       D_TX20_SOUTH D_TX20_EAST "|"
                                       D_TX20_SOUTH D_TX20_SOUTH D_TX20_EAST "|"
                                       D_TX20_SOUTH "|"
                                       D_TX20_SOUTH D_TX20_SOUTH D_TX20_WEST "|"
                                       D_TX20_SOUTH D_TX20_WEST "|"
                                       D_TX20_WEST D_TX20_SOUTH D_TX20_WEST "|"
                                       D_TX20_WEST "|"
                                       D_TX20_WEST D_TX20_NORTH D_TX20_WEST "|"
                                       D_TX20_NORTH D_TX20_WEST "|"
                                       D_TX20_NORTH D_TX20_NORTH D_TX20_WEST;

uint8_t tx20_sa = 0;
uint8_t tx20_sb = 0;
uint8_t tx20_sd = 0;
uint8_t tx20_se = 0;
uint16_t tx20_sc = 0;
uint16_t tx20_sf = 0;

float tx20_wind_speed_kmh = 0;
float tx20_wind_speed_max = 0;
float tx20_wind_speed_avg = 0;
float tx20_wind_sum = 0;
int tx20_count = 0;
uint8_t tx20_wind_direction = 0;

boolean tx20_available = false;

void Tx20StartRead()
{
# 95 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_35_tx20.ino"
  tx20_available = false;

  tx20_sa = 0;
  tx20_sb = 0;
  tx20_sd = 0;
  tx20_se = 0;
  tx20_sc = 0;
  tx20_sf = 0;

  delayMicroseconds(TX20_BIT_TIME / 2);

  for (int bitcount = 41; bitcount > 0; bitcount--) {
    uint8_t dpin = (digitalRead(pin[GPIO_TX20_TXD_BLACK]));
    if (bitcount > 41 - 5) {

      tx20_sa = (tx20_sa << 1) | (dpin ^ 1);
    } else if (bitcount > 41 - 5 - 4) {

      tx20_sb = tx20_sb >> 1 | ((dpin ^ 1) << 3);
    } else if (bitcount > 41 - 5 - 4 - 12) {

      tx20_sc = tx20_sc >> 1 | ((dpin ^ 1) << 11);
    } else if (bitcount > 41 - 5 - 4 - 12 - 4) {

      tx20_sd = tx20_sd >> 1 | ((dpin ^ 1) << 3);
    } else if (bitcount > 41 - 5 - 4 - 12 - 4 - 4) {

      tx20_se = tx20_se >> 1 | (dpin << 3);
    } else {

      tx20_sf = tx20_sf >> 1 | (dpin << 11);
    }

    delayMicroseconds(TX20_BIT_TIME);
  }

  uint8_t chk = (tx20_sb + (tx20_sc & 0xf) + ((tx20_sc >> 4) & 0xf) + ((tx20_sc >> 8) & 0xf));
  chk &= 0xf;

  if ((chk == tx20_sd) && (tx20_sc < 400)) {
    tx20_available = true;
  }







  GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, 1 << pin[GPIO_TX20_TXD_BLACK]);
}

void Tx20Read()
{
  if (!(uptime % TX20_RESET_VALUES)) {
    tx20_count = 0;
    tx20_wind_sum = 0;
    tx20_wind_speed_max = 0;
  }
  else if (tx20_available) {
    tx20_wind_speed_kmh = float(tx20_sc) * 0.36;
    if (tx20_wind_speed_kmh > tx20_wind_speed_max) {
      tx20_wind_speed_max = tx20_wind_speed_kmh;
    }
    tx20_count++;
    tx20_wind_sum += tx20_wind_speed_kmh;
    tx20_wind_speed_avg = tx20_wind_sum / tx20_count;
    tx20_wind_direction = tx20_sb;
  }
}

void Tx20Init() {
  pinMode(pin[GPIO_TX20_TXD_BLACK], INPUT);
  attachInterrupt(pin[GPIO_TX20_TXD_BLACK], Tx20StartRead, RISING);
}

void Tx20Show(boolean json)
{
  char wind_speed_string[10];
  char wind_speed_max_string[10];
  char wind_speed_avg_string[10];
  char wind_direction_string[4];

  dtostrfd(tx20_wind_speed_kmh, 2, wind_speed_string);
  dtostrfd(tx20_wind_speed_max, 2, wind_speed_max_string);
  dtostrfd(tx20_wind_speed_avg, 2, wind_speed_avg_string);
  GetTextIndexed(wind_direction_string, sizeof(wind_direction_string), tx20_wind_direction, kTx20Directions);

  if (json) {
    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s,\"TX20\":{\"Speed\":%s,\"SpeedAvg\":%s,\"SpeedMax\":%s,\"Direction\":\"%s\"}"),
      mqtt_data, wind_speed_string, wind_speed_avg_string, wind_speed_max_string, wind_direction_string);
#ifdef USE_WEBSERVER
  } else {
    snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_SNS_TX20, mqtt_data, wind_speed_string, wind_speed_avg_string, wind_speed_max_string, wind_direction_string);
#endif
  }
}





#define XSNS_35 

boolean Xsns35(byte function)
{
  boolean result = false;

  if (pin[GPIO_TX20_TXD_BLACK] < 99) {
    switch (function) {
      case FUNC_INIT:
        Tx20Init();
        break;
      case FUNC_EVERY_SECOND:
        Tx20Read();
        break;
      case FUNC_JSON_APPEND:
        Tx20Show(1);
        break;
#ifdef USE_WEBSERVER
      case FUNC_WEB_APPEND:
        Tx20Show(0);
        break;
#endif
    }
  }
  return result;
}

#endif
# 1 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_interface.ino"
# 20 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_interface.ino"
boolean (* const xsns_func_ptr[])(byte) PROGMEM = {
#ifdef XSNS_01
  &Xsns01,
#endif

#ifdef XSNS_02
  &Xsns02,
#endif

#ifdef XSNS_03
  &Xsns03,
#endif

#ifdef XSNS_04
  &Xsns04,
#endif

#ifdef XSNS_05
  &Xsns05,
#endif

#ifdef XSNS_06
  &Xsns06,
#endif

#ifdef XSNS_07
  &Xsns07,
#endif

#ifdef XSNS_08
  &Xsns08,
#endif

#ifdef XSNS_09
  &Xsns09,
#endif

#ifdef XSNS_10
  &Xsns10,
#endif

#ifdef XSNS_11
  &Xsns11,
#endif

#ifdef XSNS_12
  &Xsns12,
#endif

#ifdef XSNS_13
  &Xsns13,
#endif

#ifdef XSNS_14
  &Xsns14,
#endif

#ifdef XSNS_15
  &Xsns15,
#endif

#ifdef XSNS_16
  &Xsns16,
#endif

#ifdef XSNS_17
  &Xsns17,
#endif

#ifdef XSNS_18
  &Xsns18,
#endif

#ifdef XSNS_19
  &Xsns19,
#endif

#ifdef XSNS_20
  &Xsns20,
#endif

#ifdef XSNS_21
  &Xsns21,
#endif

#ifdef XSNS_22
  &Xsns22,
#endif

#ifdef XSNS_23
  &Xsns23,
#endif

#ifdef XSNS_24
  &Xsns24,
#endif

#ifdef XSNS_25
  &Xsns25,
#endif

#ifdef XSNS_26
  &Xsns26,
#endif

#ifdef XSNS_27
  &Xsns27,
#endif

#ifdef XSNS_28
  &Xsns28,
#endif

#ifdef XSNS_29
  &Xsns29,
#endif

#ifdef XSNS_30
  &Xsns30,
#endif

#ifdef XSNS_31
  &Xsns31,
#endif

#ifdef XSNS_32
  &Xsns32,
#endif

#ifdef XSNS_33
  &Xsns33,
#endif

#ifdef XSNS_34
  &Xsns34,
#endif

#ifdef XSNS_35
  &Xsns35,
#endif

#ifdef XSNS_36
  &Xsns36,
#endif

#ifdef XSNS_37
  &Xsns37,
#endif

#ifdef XSNS_38
  &Xsns38,
#endif

#ifdef XSNS_39
  &Xsns39,
#endif

#ifdef XSNS_40
  &Xsns40,
#endif

#ifdef XSNS_41
  &Xsns41,
#endif

#ifdef XSNS_42
  &Xsns42,
#endif

#ifdef XSNS_43
  &Xsns43,
#endif

#ifdef XSNS_44
  &Xsns44,
#endif

#ifdef XSNS_45
  &Xsns45,
#endif

#ifdef XSNS_46
  &Xsns46,
#endif

#ifdef XSNS_47
  &Xsns47,
#endif

#ifdef XSNS_48
  &Xsns48,
#endif

#ifdef XSNS_49
  &Xsns49,
#endif

#ifdef XSNS_50
  &Xsns50,
#endif



#ifdef XSNS_91
  &Xsns91,
#endif

#ifdef XSNS_92
  &Xsns92,
#endif

#ifdef XSNS_93
  &Xsns93,
#endif

#ifdef XSNS_94
  &Xsns94,
#endif

#ifdef XSNS_95
  &Xsns95,
#endif

#ifdef XSNS_96
  &Xsns96,
#endif

#ifdef XSNS_97
  &Xsns97,
#endif

#ifdef XSNS_98
  &Xsns98,
#endif

#ifdef XSNS_99
  &Xsns99
#endif
};

const uint8_t xsns_present = sizeof(xsns_func_ptr) / sizeof(xsns_func_ptr[0]);
uint8_t xsns_index = 0;
# 278 "C:/Users/Terrortoertchen/Documents/GitHub/Sonoff-Tasmota/sonoff/xsns_interface.ino"
uint8_t XsnsPresent()
{
  return xsns_present;
}

boolean XsnsNextCall(byte Function)
{
  xsns_index++;
  if (xsns_index == xsns_present) xsns_index = 0;
  return xsns_func_ptr[xsns_index](Function);
}

boolean XsnsCall(byte Function)
{
  boolean result = false;

#ifdef PROFILE_XSNS_EVERY_SECOND
  uint32_t profile_start_millis = millis();
#endif

  for (byte x = 0; x < xsns_present; x++) {

#ifdef PROFILE_XSNS_SENSOR_EVERY_SECOND
  uint32_t profile_start_millis = millis();
#endif

    result = xsns_func_ptr[x](Function);

#ifdef PROFILE_XSNS_SENSOR_EVERY_SECOND
  uint32_t profile_millis = millis() - profile_start_millis;
  if (profile_millis) {
    if (FUNC_EVERY_SECOND == Function) {
      snprintf_P(log_data, sizeof(log_data), PSTR("PRF: At %08u XsnsCall %d to Sensor %d took %u mS"), uptime, Function, x, profile_millis);
      AddLog(LOG_LEVEL_DEBUG);
    }
  }
#endif

    if (result) break;
  }

#ifdef PROFILE_XSNS_EVERY_SECOND
  uint32_t profile_millis = millis() - profile_start_millis;
  if (profile_millis) {
    if (FUNC_EVERY_SECOND == Function) {
      snprintf_P(log_data, sizeof(log_data), PSTR("PRF: At %08u XsnsCall %d took %u mS"), uptime, Function, profile_millis);
      AddLog(LOG_LEVEL_DEBUG);
    }
  }
#endif

  return result;
}