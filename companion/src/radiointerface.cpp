#include "radiointerface.h"
#include "appdata.h"
#include "eeprominterface.h"
#include "process_flash.h"
#include "radionotfound.h"
#include "burnconfigdialog.h"
#include "firmwareinterface.h"
#include "helpers.h"
#include "mountlist.h"
#include "process_copy.h"
#include <QFile>
#include <QMessageBox>

#if defined WIN32 || !defined __GNUC__
  #include <windows.h>
#endif

QString getRadioInterfaceCmd()
{
  burnConfigDialog bcd;
  EEPROMInterface *eepromInterface = GetEepromInterface();
  if (IS_TARANIS(eepromInterface->getBoard())) {
    return bcd.getDFU();
  }
  else if (IS_SKY9X(GetEepromInterface()->getBoard())) {
    return bcd.getSAMBA();
  }
  else {
    return bcd.getAVRDUDE();
  }
}

QStringList getAvrdudeArgs(const QString &cmd, const QString &filename)
{
  QStringList args;
  burnConfigDialog bcd;
  QString programmer = bcd.getProgrammer();
  QString mcu   = bcd.getMCU();

  args << "-c" << programmer << "-p";
  if (GetEepromInterface()->getBoard() == BOARD_GRUVIN9X)
    args << "m2560";
  else if (GetEepromInterface()->getBoard() == BOARD_MEGA2560)
    args << "m2560";
  else if (GetEepromInterface()->getBoard() == BOARD_M128)
    args << "m128";
  else
    args << mcu;

  args << bcd.getAvrdudeArgs();

  QString fullcmd = cmd + filename;
  if (QFileInfo(filename).suffix().toUpper() == "HEX")
    fullcmd += ":i";
  else if (QFileInfo(filename).suffix().toUpper()=="BIN")
    fullcmd += ":r";
  else
    fullcmd += ":a";

  args << "-U" << fullcmd;

  return args;
}

QStringList getDfuArgs(const QString &cmd, const QString &filename)
{
  QStringList arguments;
  burnConfigDialog bcd;
  QString memory="0x08000000";
  if (cmd=="-U") {
    memory.append(QString(":%1").arg(MAX_FSIZE));
  }
  arguments << bcd.getDFUArgs() << "--dfuse-address" << memory << "-d" << "0483:df11";
  QString fullcmd = cmd + filename;

  arguments << "" << fullcmd;

  return arguments;
}

QStringList getSambaArgs(const QString &tcl)
{
  QStringList result;

  QString tclFilename = generateProcessUniqueTempFileName("temp.tcl");
  if (QFile::exists(tclFilename)) {
    qunlink(tclFilename);
  }
  QFile tclFile(tclFilename);
  if (!tclFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
    QMessageBox::warning(NULL, QObject::tr("Error"), QObject::tr("Cannot write file %1:\n%2.").arg(tclFilename).arg(tclFile.errorString()));
    return result;
  }

  QTextStream outputStream(&tclFile);
  outputStream << tcl;

  burnConfigDialog bcd;
  result << bcd.getSambaPort() << bcd.getArmMCU() << tclFilename ;
  return result;
}

QStringList getReadEEpromCmd(const QString &filename)
{
  QStringList result;
  EEPROMInterface *eepromInterface = GetEepromInterface();
  if (IS_TARANIS(eepromInterface->getBoard())) {
    // impossible
  }
  else if (IS_SKY9X(eepromInterface->getBoard())) {
    result = getSambaArgs(QString("SERIALFLASH::Init 0\n") + "receive_file {SerialFlash AT25} \"" + filename + "\" 0x0 0x80000 0\n");
  }
  else {
    result = getAvrdudeArgs("eeprom:r:", filename);
  }
  return result;
}

QStringList getWriteEEpromCmd(const QString &filename)
{
  EEPROMInterface *eepromInterface = GetEepromInterface();
  if (IS_TARANIS(eepromInterface->getBoard())) {
    // impossible
    return QStringList();
  }
  else if (IS_SKY9X(eepromInterface->getBoard())) {
    return getSambaArgs(QString("SERIALFLASH::Init 0\n") + "send_file {SerialFlash AT25} \"" + filename + "\" 0x0 0\n");
  }
  else {
    return getAvrdudeArgs("eeprom:w:", filename);
  }
}

QStringList getWriteFirmwareArgs(const QString &filename)
{
  EEPROMInterface *eepromInterface = GetEepromInterface();
  if (IS_TARANIS(eepromInterface->getBoard())) {
    return getDfuArgs("-D", filename);
  }
  else if (eepromInterface->getBoard() == BOARD_SKY9X) {
    return getSambaArgs(QString("send_file {Flash} \"") + filename + "\" 0x400000 0\n" + "FLASH::ScriptGPNMV 2\n");
  }
  else if (eepromInterface->getBoard() == BOARD_9XRPRO) {
    return getSambaArgs(QString("send_file {Flash} \"") + filename + "\" 0x400000 0\n" + "FLASH::ScriptGPNMV 2\n");
  }
  else {
    return getAvrdudeArgs("flash:w:", filename);
  }
}

QStringList getReadFirmwareArgs(const QString &filename)
{
  EEPROMInterface *eepromInterface = GetEepromInterface();
  if (IS_TARANIS(eepromInterface->getBoard())) {
    return getDfuArgs("-U", filename);
  }
  else if (eepromInterface->getBoard() == BOARD_SKY9X) {
    return getSambaArgs(QString("receive_file {Flash} \"") + filename + "\" 0x400000 0x40000 0\n");
  }
  else if (eepromInterface->getBoard() == BOARD_9XRPRO) {
    return getSambaArgs(QString("receive_file {Flash} \"") + filename + "\" 0x400000 0x80000 0\n");
  }
  else {
    return getAvrdudeArgs("flash:r:", filename);
  }
}

void readAvrdudeFuses(ProgressWidget *progress)
{
  burnConfigDialog bcd;
  QStringList args;
  args << "-c" << bcd.getProgrammer() << "-p" << bcd.getMCU() << bcd.getAvrdudeArgs() << "-U" << "lfuse:r:-:i" << "-U" << "hfuse:r:-:i" << "-U" << "efuse:r:-:i";
  FlashProcess flashProcess(bcd.getAVRDUDE(), args, progress);
  flashProcess.run();
}

void resetAvrdudeFuses(bool eepromProtect, ProgressWidget *progress)
{
  //fuses
  //avrdude -c usbasp -p m64 -U lfuse:w:<0x0E>:m
  //avrdude -c usbasp -p m64 -U hfuse:w:<0x89>:m  0x81 for eeprom protection
  //avrdude -c usbasp -p m64 -U efuse:w:<0xFF>:m

  burnConfigDialog bcd;
  QMessageBox::StandardButton ret = QMessageBox::No;
  ret = QMessageBox::warning(NULL, QObject::tr("Companion"),
                             QObject::tr("<b><u>WARNING!</u></b><br>This will reset the fuses of  %1 to the factory settings.<br>"
                                 "Writing fuses can mess up your radio.<br>Do this only if you are sure they are wrong!<br>"
                                 "Are you sure you want to continue?").arg(bcd.getMCU()),
                             QMessageBox::Yes | QMessageBox::No);
  if (ret == QMessageBox::Yes) {
    QStringList args = bcd.getAvrdudeArgs();
    QStringList str;
    if (bcd.getMCU() == "m2560") {
      args << "-B8";
      QString erStr = eepromProtect ? "hfuse:w:0x11:m" : "hfuse:w:0x19:m";
      str << "-U" << "lfuse:w:0xD7:m" << "-U" << erStr << "-U" << "efuse:w:0xFC:m";
      //use hfuse = 0x81 to prevent eeprom being erased with every flashing
    }
    else {
      QString lfuses;
      QString tempFile = generateProcessUniqueTempFileName("ftemp.bin");
      QStringList argread;
      argread << "-c" << bcd.getProgrammer() << "-p" << bcd.getMCU() << args  <<"-U" << "lfuse:r:"+tempFile+":r";
      FlashProcess flashProcess(bcd.getAVRDUDE(), argread, progress);
      flashProcess.run();
      QFile file(tempFile);
      if (file.exists() && file.size()==1) {
        file.open(QIODevice::ReadOnly);
        char bin_flash[1];
        file.read(bin_flash, 1);
        if (bin_flash[0]==0x0E) {
          lfuses = "lfuse:w:0x0E:m";
        }
        else {
          lfuses = "lfuse:w:0x3F:m";
        }
        file.close();
        qunlink(tempFile);
      }
      else {
        lfuses = "lfuse:w:0x3F:m";
      }

      QString erStr = eepromProtect ? "hfuse:w:0x81:m" : "hfuse:w:0x89:m";
      str << "-U" << lfuses << "-U" << erStr << "-U" << "efuse:w:0xFF:m";
      //use hfuse = 0x81 to prevent eeprom being erased with every flashing
    }

    QStringList arguments;
    if (bcd.getMCU() == "m2560") {
      arguments << "-c" << bcd.getProgrammer() << "-p" << bcd.getMCU() << args << "-u" << str;
    }
    else {
      arguments << "-c" << bcd.getProgrammer() << "-p" << bcd.getMCU() << args << "-B" << "100" << "-u" << str;
    }
    FlashProcess flashProcess(bcd.getAVRDUDE(), arguments, progress);
    flashProcess.run();
  }
}


bool readFirmware(const QString &filename, ProgressWidget *progress)
{
  bool result = false;

  QFile file(filename);
  if (file.exists() && !file.remove()) {
    QMessageBox::warning(NULL, QObject::tr("Error"), QObject::tr("Could not delete temporary file: %1").arg(filename));
    return false;
  }

  if (IS_ARM(GetCurrentFirmware()->getBoard())) {
    QString path = findMassstoragePath("FIRMWARE.BIN");
    if (!path.isEmpty()) {
      qDebug() << "readFirmware: reading" << path << "into" << filename;
      CopyProcess copyProcess(path, filename, progress);
      result = copyProcess.run();
    }
  }

  if (result == false) {
    qDebug() << "readFirmware: reading" << filename << "with" << getRadioInterfaceCmd() << getReadFirmwareArgs(filename);
    FlashProcess flashProcess(getRadioInterfaceCmd(), getReadFirmwareArgs(filename), progress);
    result = flashProcess.run();
  }

  if (!QFileInfo(filename).exists()) {
    result = false;
  }

  return result;
}

bool writeFirmware(const QString &filename, ProgressWidget *progress)
{
  if (IS_ARM(GetCurrentFirmware()->getBoard())) {
    QString path = findMassstoragePath("FIRMWARE.BIN");
    if (!path.isEmpty()) {
      qDebug() << "writeFirmware: writing" << path << "from" << filename;
      CopyProcess copyProcess(filename, path, progress);
      return copyProcess.run();
    }
  }

  qDebug() << "writeFirmware: writing" << filename << "with" << getRadioInterfaceCmd() << getWriteFirmwareArgs(filename);
  FlashProcess flashProcess(getRadioInterfaceCmd(), getWriteFirmwareArgs(filename), progress);
  return flashProcess.run();
}


bool readEeprom(const QString &filename, ProgressWidget *progress)
{
  bool result = false;

  QFile file(filename);
  if (file.exists() && !file.remove()) {
    QMessageBox::warning(NULL, QObject::tr("Error"), QObject::tr("Could not delete temporary file: %1").arg(filename));
    return false;
  }

  if (IS_ARM(GetCurrentFirmware()->getBoard())) {
    QString path = findMassstoragePath("EEPROM.BIN");
    if (path.isEmpty()) {
      // On previous OpenTX we called the EEPROM file "TARANIS.BIN" :(
      path = findMassstoragePath("TARANIS.BIN");
    }
    if (path.isEmpty()) {
      // Mike's bootloader calls the EEPROM file "ERSKY9X.BIN" :(
      path = findMassstoragePath("ERSKY9X.BIN");
    }
    if (!path.isEmpty()) {
      CopyProcess copyProcess(path, filename, progress);
      result = copyProcess.run();
    }
  }

  if (result == false && !IS_TARANIS(GetCurrentFirmware()->getBoard())) {
    FlashProcess flashProcess(getRadioInterfaceCmd(), getReadEEpromCmd(filename), progress);
    result = flashProcess.run();
  }

  if (result == false && IS_ARM(GetCurrentFirmware()->getBoard())) {
    RadioNotFoundDialog dialog;
    dialog.exec();
  }

  if (!QFileInfo(filename).exists()) {
    result = false;
  }

  return result;
}

bool writeEeprom(const QString &filename, ProgressWidget *progress)
{
  if (IS_ARM(GetCurrentFirmware()->getBoard())) {
    QString path = findMassstoragePath("EEPROM.BIN");
    if (path.isEmpty()) {
      // On previous OpenTX we called the EEPROM file "TARANIS.BIN" :(
      path = findMassstoragePath("TARANIS.BIN");
    }
    if (path.isEmpty()) {
      // Mike's bootloader calls the EEPROM file "ERSKY9X.BIN" :(
      path = findMassstoragePath("ERSKY9X.BIN");
    }
    if (!path.isEmpty()) {
      CopyProcess copyProcess(filename, path, progress);
      return copyProcess.run();
    }
  }

  if (!IS_TARANIS(GetCurrentFirmware()->getBoard())) {
    FlashProcess flashProcess(getRadioInterfaceCmd(), getWriteEEpromCmd(filename), progress);
    return flashProcess.run();
  }

  if (IS_ARM(GetCurrentFirmware()->getBoard())) {
    RadioNotFoundDialog dialog;
    dialog.exec();
  }

  return false;
}

#if defined WIN32 || !defined __GNUC__
bool isRemovableMedia(const QString & vol)
{
  char szDosDeviceName[MAX_PATH];
  QString volume = vol;
  UINT driveType = GetDriveType(volume.replace("/", "\\").toLatin1());
  if (driveType != DRIVE_REMOVABLE)
    return false;
  QueryDosDevice(volume.replace("/", "").toLatin1(), szDosDeviceName, MAX_PATH);
  if (strstr(szDosDeviceName, "\\Floppy") != NULL) { // it's a floppy
    return false;
  }
  return true;
}
#endif

QString findMassstoragePath(const QString &filename)
{
  QString temppath;
  QStringList drives;
  QString eepromfile;
  QString fsname;
  static QStringList blacklist;

#if defined WIN32 || !defined __GNUC__
  foreach(QFileInfo drive, QDir::drives()) {
    WCHAR szVolumeName[256] ;
    WCHAR szFileSystemName[256];
    DWORD dwSerialNumber = 0;
    DWORD dwMaxFileNameLength=256;
    DWORD dwFileSystemFlags=0;
    if (!blacklist.contains(drive.absolutePath())) {
      if (!isRemovableMedia( drive.absolutePath() )) {
        blacklist.append(drive.absolutePath());
      } else {
        bool ret = GetVolumeInformationW( (WCHAR *) drive.absolutePath().utf16(),szVolumeName,256,&dwSerialNumber,&dwMaxFileNameLength,&dwFileSystemFlags,szFileSystemName,256);
        if (ret) {
          QString vName = QString::fromUtf16 ( (const ushort *) szVolumeName) ;
          temppath = drive.absolutePath();
          eepromfile = temppath;
          eepromfile.append("/" + filename);
          if (QFile::exists(eepromfile)) {
            return eepromfile;
          }
        }
      }
    }
  }
#else
  struct mount_entry *entry;
  entry = read_file_system_list(true);
  while (entry != NULL) {
    if (!drives.contains(entry->me_devname)) {
      drives.append(entry->me_devname);
      temppath = entry->me_mountdir;
      eepromfile = temppath;
      eepromfile.append("/" + filename);
#if !defined __APPLE__
      QString fstype = entry->me_type;
      // qDebug() << eepromfile;
      
      if (fstype.contains("fat") && QFile::exists(eepromfile)) {
#else
      if (QFile::exists(eepromfile)) {
#endif
        return eepromfile;
      }
    }
    entry = entry->me_next;
  }
#endif

  return QString();
}
