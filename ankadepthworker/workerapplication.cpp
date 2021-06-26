#include "workerapplication.h"
#include "dbpatchbufferer.h"
#include <QCoreApplication>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlResult>
#include <QSqlError>
#include <QThread>
#include <QThreadPool>

using namespace ComputeGrid;

WorkerApplication::WorkerApplication(QObject * _parent)
	: QThread(_parent),
	mCompletedTaskCounter(0),
	mFailedTaskCounter(0)
{
	DBPatchBufferer::init(&mConfig);
}

WorkerApplication::~WorkerApplication()
{
	DBPatchBufferer::clearBuffer();
}

void WorkerApplication::run()
{
	QString status;
	while (!QThread::isInterruptionRequested())
	{
		DepthTaskWorker * tw = nullptr;
		
		mRWLock.lockForWrite();
		for (DepthTaskWorkerList::iterator it = mTaskWorkers.begin(); it != mTaskWorkers.end(); ++it)
		{
			tw = *it;
			if (tw->status() == DTWS_IDLE)
			{
				QObject::connect(tw, SIGNAL(progress(DepthTaskWorker *, QString)), this, SLOT(taskWorkerProgress(DepthTaskWorker *, QString)));
				QObject::connect(tw, SIGNAL(error(DepthTaskWorker *, QString)), this, SLOT(taskWorkerError(DepthTaskWorker *, QString)));
				QObject::connect(tw, SIGNAL(finished(DepthTaskWorker *)), this, SLOT(taskWorkerFinished(DepthTaskWorker *)));
				tw->setStatus(DTWS_POOLED);
				QThreadPool::globalInstance()->start(tw);
			}
		}

		status = QString("Anka-Depth v%1 - Queued: <font color=\"blue\">%2</font>, Completed: <font color=\"green\">%3</font>, Failed: <font color=\"red\">%4</font>")
			.arg(QString("%1.%2.%3.%4").arg(AnkaDepthLibGlobals::VersionMajor).arg(AnkaDepthLibGlobals::VersionMinor).arg(AnkaDepthLibGlobals::VersionPatch).arg(AnkaDepthLibGlobals::VersionBuild))
			.arg(mTaskWorkers.count())
			.arg(mCompletedTaskCounter)
			.arg(mFailedTaskCounter);
		mRWLock.unlock();

		if (mLastStatus != status)
			emit out(ComputeGridGlobals::makeProcessCommand(PC_STATUS_MESSAGE, QStringList() << (mLastStatus = status)));
	}

	emit finished();
}

void WorkerApplication::workerData(QStringList _args)
{
	if (_args.count() == 2)
	{
		DepthTaskParameterType pt = (DepthTaskParameterType)_args[0].toInt();

		switch (pt)
		{
		case AnkaDepthLib::DTPT_SYS_CALL:
			emit out(ComputeGridGlobals::makeLogCommand(LS_WP, LT_INFO, QString("System call: '%1'").arg(_args[1])));
			system(_args[1].toStdString().c_str());
			break;

		case AnkaDepthLib::DTPT_TASK_CONFIG:
			mConfig.fromString(_args[1]);
			emit out(ComputeGridGlobals::makeLogCommand(LS_WP, LT_INFO, "Depth task configuration loaded."));
			break;

		case AnkaDepthLib::DTPT_TASK_EXECUTE:
		{
			DepthTaskWorker * tw = new DepthTaskWorker(&mConfig, DepthTask(_args[1]));
			tw->setStatus(DTWS_IDLE);

			mRWLock.lockForWrite();
			mTaskWorkers.append(tw);
			mRWLock.unlock();

			emit out(ComputeGridGlobals::makeLogCommand(LS_WP, LT_INFO, QString("Region ID: %1 => Execution queued.").arg(tw->id())));
			break;
		}

		default:
			emit out(ComputeGridGlobals::makeLogCommand(LS_WP, LT_ERROR, QString("Unexpected task arguments.")));
			break;
		}
	}
	else
		emit out(ComputeGridGlobals::makeLogCommand(LS_WP, LT_ERROR, QString("Unexpected 'workerData' arguments: %1").arg(_args.join(ComputeGridGlobals::ProcessCommandDataSeperator))));
}

void WorkerApplication::workerExit(QStringList _args)
{
	mRWLock.lockForWrite();

	// cancel the pooled work items first
	int id = 0;
	for (DepthTaskWorkerList::iterator it = mTaskWorkers.begin(); it != mTaskWorkers.end();)
	{
		DepthTaskWorker * tw = *it;

		if (tw->status() == DTWS_POOLED)
		{
			QThreadPool::globalInstance()->cancel(tw);
			it = mTaskWorkers.erase(it);
			emit out(ComputeGridGlobals::makeLogCommand(LS_WP, LT_WARNING, QString("Region ID: %1 => Execution cancelled.").arg(tw->id())));
			delete tw;
		}
		else
			++it;
	}

	// then, stop the running work items
	for (DepthTaskWorkerList::iterator it = mTaskWorkers.begin(); it != mTaskWorkers.end(); ++it)
	{
		DepthTaskWorker * tw = *it;
		if (tw->status() == DTWS_RUNNING)
		{
			tw->stop();
			emit out(ComputeGridGlobals::makeLogCommand(LS_WP, LT_WARNING, QString("Region ID: %1 => Execution stopped.").arg(tw->id())));
		}
	}

	mRWLock.unlock();

	QThreadPool::globalInstance()->clear();

	DBPatchBufferer::clearBuffer();

	QThread::requestInterruption();
}

#pragma region Slots
void WorkerApplication::taskWorkerProgress(AnkaDepthLib::DepthTaskWorker * _taskWorker, QString _message)
{
	emit out(ComputeGridGlobals::makeLogCommand(LS_WP, LT_INFO, _message));
}

void WorkerApplication::taskWorkerError(AnkaDepthLib::DepthTaskWorker * _taskWorker, QString _message)
{
	emit out(ComputeGridGlobals::makeLogCommand(LS_WP, LT_ERROR, _message));
}

void WorkerApplication::taskWorkerFinished(AnkaDepthLib::DepthTaskWorker * _taskWorker)
{
	emit out(ComputeGridGlobals::makeProcessCommand(PC_WORKER_DATA, QStringList() << QString::number(DTPT_TASK_RESULT) << QString::number(_taskWorker->status()) << QString::number(_taskWorker->id())));

	mRWLock.lockForWrite();

	if (_taskWorker->status() == DTWS_COMPLETED)
		++mCompletedTaskCounter;
	else
		++mFailedTaskCounter;

	mTaskWorkers.removeAll(_taskWorker);
	_taskWorker->deleteLater();

	mRWLock.unlock();
}
#pragma endregion
