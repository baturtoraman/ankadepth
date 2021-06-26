#pragma once

#include <QObject>
#include <QThread>
#include <QReadWriteLock>
#include "computegridcommons.hpp"
#include "ankadepthlibglobals.h"
#include "depthconfiguration.h"
#include "depthtask.h"
#include "depthtaskworker.h"

using namespace AnkaDepthLib;

class WorkerApplication : public QThread
{ 
	Q_OBJECT

public:
	WorkerApplication(QObject * _parent = nullptr);
	~WorkerApplication();

	void run() override;

	// worker data command callback
	Q_INVOKABLE void workerData(QStringList _args);

	// worker exit command callback
	Q_INVOKABLE void workerExit(QStringList _args);

private:
	// lock object to use in invoke calls
	QReadWriteLock mRWLock;

	// configuration
	DepthConfiguration mConfig;

	// task worker pool
	DepthTaskWorkerList mTaskWorkers;

	int mCompletedTaskCounter;
	int mFailedTaskCounter;

	QString mLastStatus;

#pragma region Signals-Slots
signals:
	void finished();
	void out(QString _cmd);

public slots:
	void taskWorkerProgress(DepthTaskWorker * _taskWorker, QString _message);
	void taskWorkerError(DepthTaskWorker * _taskWorker, QString _message);
	void taskWorkerFinished(DepthTaskWorker * _taskWorker);
#pragma endregion

};
