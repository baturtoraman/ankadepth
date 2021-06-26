#include "depthconfiguration.h"
#include "computegridcommons.hpp"
#include <QSettings>

AnkaDepthLib::DepthConfiguration::DepthConfiguration(QObject * _parent)
	: QObject(_parent)
{
}

AnkaDepthLib::DepthConfiguration::~DepthConfiguration()
{
}

QDataStream & AnkaDepthLib::operator<<(QDataStream & _out, const DepthConfiguration & _dc)
{
	_out << const_cast<DepthConfiguration &>(_dc).toString();
	return _out;
}

QDataStream & AnkaDepthLib::operator>>(QDataStream & _in, DepthConfiguration & _dc)
{
	QString s;
	_in >> s;
	_dc.fromString(s);
	return _in;
}

QString AnkaDepthLib::DepthConfiguration::toString()
{
	QStringList sl;
	sl << mPCDatabaseIp;
	sl << mPCDatabaseName;
	sl << mPCDatabaseUserName;
	sl << mPCDatabasePassword;
	sl << mPCDatabaseOptions;
	sl << QString::number(mPCDatabasePort);
	sl << mKGMDatabaseIp;
	sl << mKGMDatabaseName;
	sl << mKGMDatabaseUserName;
	sl << mKGMDatabasePassword;
	sl << mKGMDatabaseOptions;
	sl << QString::number(mKGMDatabasePort);

	sl << mAnkRootPath;
	sl << mOutputRootPath;
	sl << QString::number(mPatchLimit);
	sl << QString::number(mPatchThreshold);
	sl << QString::number(mManagerAutoStart ? 1 : 0);
	sl << QString::number(mManagerReprocess ? 1 : 0);
	sl << QString::number(mWorkerReprocess ? 1 : 0);
	sl << mInputRootDirs.join(',');
	sl << mInputSubDirs.join(',');

	sl << QString::number(mScheduledWork ? 1 : 0);
	sl << QString::number(mFullDayWorkAtSaturday ? 1 : 0);
	sl << QString::number(mFullDayWorkAtSunday ? 1 : 0);
	sl << mStartWorkTime.toString("hh:mm:ss");
	sl << mStopWorkTime.toString("hh:mm:ss");

	return sl.join(ComputeGrid::ComputeGridGlobals::ProcessCommandDataSeperator);
}

void AnkaDepthLib::DepthConfiguration::fromString(QString & _string)
{
	mInputRootDirs.clear();
	mInputSubDirs.clear();

	QStringList sl = _string.split(ComputeGrid::ComputeGridGlobals::ProcessCommandDataSeperator);
	mPCDatabaseIp = sl.takeFirst();
	mPCDatabaseName = sl.takeFirst();
	mPCDatabaseUserName = sl.takeFirst();
	mPCDatabasePassword = sl.takeFirst();
	mPCDatabaseOptions = sl.takeFirst();
	mPCDatabasePort = sl.takeFirst().toInt();
	mKGMDatabaseIp = sl.takeFirst();
	mKGMDatabaseName = sl.takeFirst();
	mKGMDatabaseUserName = sl.takeFirst();
	mKGMDatabasePassword = sl.takeFirst();
	mKGMDatabaseOptions = sl.takeFirst();
	mKGMDatabasePort = sl.takeFirst().toInt();

	mAnkRootPath = sl.takeFirst();
	mOutputRootPath = sl.takeFirst();
	mPatchLimit = sl.takeFirst().toInt();
	mPatchThreshold = sl.takeFirst().toInt();
	mManagerAutoStart = (sl.takeFirst().toInt() > 0);
	mManagerReprocess = (sl.takeFirst().toInt() > 0);
	mWorkerReprocess = (sl.takeFirst().toInt() > 0);
	mInputRootDirs = sl.takeFirst().split(',');
	mInputSubDirs = sl.takeFirst().split(',');

	mScheduledWork = (sl.takeFirst().toInt() > 0);
	mFullDayWorkAtSaturday = (sl.takeFirst().toInt() > 0);
	mFullDayWorkAtSunday = (sl.takeFirst().toInt() > 0);
	mStartWorkTime = QTime::fromString(sl.takeFirst(), "hh:mm:ss");
	mStopWorkTime = QTime::fromString(sl.takeFirst(), "hh:mm:ss");
}

void AnkaDepthLib::DepthConfiguration::fromIni(const QString & _iniFile)
{
	mInputRootDirs.clear();
	mInputSubDirs.clear();

	QSettings settings(_iniFile, QSettings::IniFormat);
	settings.beginGroup("DatabaseParameters");
	mPCDatabaseIp = settings.value("PCDatabaseIp").toString();
	mPCDatabaseName = settings.value("PCDatabaseName").toString();
	mPCDatabaseUserName = settings.value("PCDatabaseUserName").toString();
	mPCDatabasePassword = settings.value("PCDatabasePassword").toString();
	mPCDatabaseOptions = settings.value("PCDatabaseOptions").toString();
	mPCDatabasePort = settings.value("PCDatabasePort", 5432).toInt();
	mKGMDatabaseIp = settings.value("KGMDatabaseIp").toString();
	mKGMDatabaseName = settings.value("KGMDatabaseName").toString();
	mKGMDatabaseUserName = settings.value("KGMDatabaseUserName").toString();
	mKGMDatabasePassword = settings.value("KGMDatabasePassword").toString();
	mKGMDatabaseOptions = settings.value("KGMDatabaseOptions").toString();
	mKGMDatabasePort = settings.value("KGMDatabasePort", 5432).toInt();
	settings.endGroup();

	settings.beginGroup("WorkParameters");
	mAnkRootPath = settings.value("AnkRootPath").toString();
	mOutputRootPath = settings.value("OutputRootPath").toString();
	mPatchLimit = settings.value("PatchLimit", 250).toInt();
	mPatchThreshold = settings.value("PatchThreshold", 50).toInt();
	mManagerAutoStart = (settings.value("ManagerAutoStart", 0).toInt() > 0);
	mManagerReprocess = (settings.value("ManagerReprocess", 0).toInt() > 0);
	mWorkerReprocess = (settings.value("WorkerReprocess", 0).toInt() > 0);
	mInputRootDirs = settings.value("InputRootDirs").toStringList();
	mInputSubDirs = settings.value("InputSubDirs").toStringList();
	settings.endGroup();

	settings.beginGroup("WorkSchedule");
	mScheduledWork = (settings.value("ScheduledWork", 0).toInt() > 0);
	mFullDayWorkAtSaturday = (settings.value("FullDayWorkAtSaturday", 0).toInt() > 0);
	mFullDayWorkAtSunday = (settings.value("FullDayWorkAtSunday", 0).toInt() > 0);
	mStartWorkTime = QTime::fromString(settings.value("StartWorkTime").toString(), "hh:mm:ss");
	mStopWorkTime = QTime::fromString(settings.value("StopWorkTime").toString(), "hh:mm:ss");
	settings.endGroup();

	//clean empty list entries
	for (int i = mInputRootDirs.count() - 1; i >= 0; --i)
	{
		if (mInputRootDirs[i].isEmpty())
			mInputRootDirs.removeAt(i);
	}

	for (int i = mInputSubDirs.count() - 1; i >= 0; --i)
	{
		if (mInputSubDirs[i].isEmpty())
			mInputSubDirs.removeAt(i);
	}
}

#pragma region Getters
QString AnkaDepthLib::DepthConfiguration::pcDatabaseIp()
{
	return mPCDatabaseIp;
}

QString AnkaDepthLib::DepthConfiguration::pcDatabaseName()
{
	return mPCDatabaseName;
}

QString AnkaDepthLib::DepthConfiguration::pcDatabaseUserName()
{
	return mPCDatabaseUserName;
}

QString AnkaDepthLib::DepthConfiguration::pcDatabasePassword()
{
	return mPCDatabasePassword;
}

QString AnkaDepthLib::DepthConfiguration::pcDatabaseOptions()
{
	return mPCDatabaseOptions;
}

QString AnkaDepthLib::DepthConfiguration::kgmDatabaseIp()
{
	return mKGMDatabaseIp;
}

QString AnkaDepthLib::DepthConfiguration::kgmDatabaseName()
{
	return mKGMDatabaseName;
}

QString AnkaDepthLib::DepthConfiguration::kgmDatabaseUserName()
{
	return mKGMDatabaseUserName;
}

QString AnkaDepthLib::DepthConfiguration::kgmDatabasePassword()
{
	return mKGMDatabasePassword;
}

QString AnkaDepthLib::DepthConfiguration::kgmDatabaseOptions()
{
	return mKGMDatabaseOptions;
}

QString AnkaDepthLib::DepthConfiguration::ankRootPath()
{
	return mAnkRootPath;
}

QString AnkaDepthLib::DepthConfiguration::outputRootPath()
{
	return mOutputRootPath;
}

int AnkaDepthLib::DepthConfiguration::pcDatabasePort()
{
	return mPCDatabasePort;
}

int AnkaDepthLib::DepthConfiguration::kgmDatabasePort()
{
	return mKGMDatabasePort;
}

int AnkaDepthLib::DepthConfiguration::patchLimit()
{
	return mPatchLimit;
}

int AnkaDepthLib::DepthConfiguration::patchThreshold()
{
	return mPatchThreshold;
}

bool AnkaDepthLib::DepthConfiguration::managerAutoStart()
{
	return mManagerAutoStart;
}

bool AnkaDepthLib::DepthConfiguration::managerReprocess()
{
	return mManagerReprocess;
}

bool AnkaDepthLib::DepthConfiguration::workerReprocess()
{
	return mWorkerReprocess;
}

QStringList AnkaDepthLib::DepthConfiguration::inputRootDirs()
{
	return mInputRootDirs;
}

QStringList AnkaDepthLib::DepthConfiguration::inputSubDirs()
{
	return mInputSubDirs;
}

bool AnkaDepthLib::DepthConfiguration::scheduledWork()
{
	return mScheduledWork;
}

bool AnkaDepthLib::DepthConfiguration::fullDayWorkAtSaturday()
{
	return mFullDayWorkAtSaturday;
}

bool AnkaDepthLib::DepthConfiguration::fullDayWorkAtSunday()
{
	return mFullDayWorkAtSunday;
}

QTime AnkaDepthLib::DepthConfiguration::startWorkTime()
{
	return mStartWorkTime;
}

QTime AnkaDepthLib::DepthConfiguration::stopWorkTime()
{
	return mStopWorkTime;
}
#pragma endregion