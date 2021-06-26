#pragma once

#include <QObject>
#include <QThread>
#include <QMap>
#include <QList>
#include <QReadWriteLock>
#include <QFile>
#include "depthconfiguration.h"
#include "depthtask.h"

class ManagerApplication : public QThread
{
	Q_OBJECT
public:
	ManagerApplication(QObject * _parent = nullptr);
	~ManagerApplication();

	void run() override;

	// grid worker in callback
	Q_INVOKABLE void workerIn(QStringList _args);

	// grid worker out callback
	Q_INVOKABLE void workerOut(QStringList _args);

	// grid worker data callback
	Q_INVOKABLE void workerData(QStringList _args);

	// grid worker exit callback
	Q_INVOKABLE void workerExit(QStringList _args);

	// terminal command callback
	Q_INVOKABLE void terminalCommand(QStringList _args);

private:
	void logFailedTask(AnkaDepthLib::DepthTask & _task);
	void logCompletedTask(AnkaDepthLib::DepthTask & _task);
	bool checkIfNeedToWork();

	// start flag
	bool mStartFlag;

	// lock object to use in invoke calls
	QReadWriteLock mRWLock;

	// configuration
	AnkaDepthLib::DepthConfiguration mConfig;

	// list of tasks
	AnkaDepthLib::DepthTaskList mTasks;

	// list of active workers
	QStringList mWorkers;

	// map of worker's parallel computing capacities
	QMap<QString, int> mWorkerCapacityMap;
	
	// map of worker's assigned tasks
	QMap<QString, AnkaDepthLib::DepthTaskList> mWorkerTasksMap;

	// completed tasks log file
	QFile mCompletedTasksFile;

	// failed tasks log file
	QFile mFailedTasksFile;

	int mTotalTasksCount;
	int mCompletedTaskCounter;
	int mFailedTaskCounter;

	QString mLastStatus;

#pragma region Signals-Slots
signals:
	// command out signal
	void out(QString _cmd);
#pragma endregion

};
