#pragma once

#include <QObject>
#include <QDataStream>
#include <QTime>

namespace AnkaDepthLib
{
	class DepthConfiguration : public QObject
	{
		Q_OBJECT

	public:
		DepthConfiguration(QObject * _parent = nullptr);
		~DepthConfiguration();

		friend QDataStream & operator<<(QDataStream & _out, const DepthConfiguration & _tc);
		friend QDataStream & operator>>(QDataStream & _in, DepthConfiguration & _tc);

		QString toString();
		void fromString(QString & _string);

		void fromIni(const QString & _iniFile);

#pragma region Getters
		QString pcDatabaseIp();
		QString pcDatabaseName();
		QString pcDatabaseUserName();
		QString pcDatabasePassword();
		QString pcDatabaseOptions();
		QString kgmDatabaseIp();
		QString kgmDatabaseName();
		QString kgmDatabaseUserName();
		QString kgmDatabasePassword();
		QString kgmDatabaseOptions();

		QString ankRootPath();
		QString outputRootPath();
		int pcDatabasePort();
		int kgmDatabasePort();
		int patchLimit();
		int patchThreshold();
		bool managerAutoStart();
		bool managerReprocess();
		bool workerReprocess();
		QStringList inputRootDirs();
		QStringList inputSubDirs();

		bool scheduledWork();
		bool fullDayWorkAtSaturday();
		bool fullDayWorkAtSunday();
		QTime startWorkTime();
		QTime stopWorkTime();
#pragma endregion

	private:
#pragma region Fields
		QString
			mPCDatabaseIp,
			mPCDatabaseName,
			mPCDatabaseUserName,
			mPCDatabasePassword,
			mPCDatabaseOptions,
			mKGMDatabaseIp,
			mKGMDatabaseName,
			mKGMDatabaseUserName,
			mKGMDatabasePassword,
			mKGMDatabaseOptions,
			mAnkRootPath,
			mOutputRootPath;

		int
			mPCDatabasePort,
			mKGMDatabasePort,
			mPatchLimit,
			mPatchThreshold;

		bool
			mManagerAutoStart,
			mManagerReprocess,
			mWorkerReprocess,
			mScheduledWork,
			mFullDayWorkAtSaturday,
			mFullDayWorkAtSunday;

		QStringList mInputRootDirs;
		QStringList mInputSubDirs;

		QTime
			mStartWorkTime,
			mStopWorkTime;
#pragma endregion

	};
}