#include "managerapplication.h"
#include "computegridcommons.hpp"
#include "ankadepthlibglobals.h"
#include <QCoreApplication>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlResult>
#include <QSqlError>
#include <QTextStream>

using namespace ComputeGrid;
using namespace AnkaDepthLib;

ManagerApplication::ManagerApplication(QObject * _parent)
	: QThread(_parent),
	mTotalTasksCount(0),
	mCompletedTaskCounter(0),
	mFailedTaskCounter(0)
{
	mConfig.fromIni(QCoreApplication::applicationName() + "_config.ini");
	
	mStartFlag = mConfig.managerAutoStart();

	mCompletedTasksFile.setFileName("completed.txt");
	mFailedTasksFile.setFileName("failed.txt");

	//if (QFile::exists(mFailedTasksFile.fileName()))
	//	QFile::remove(mFailedTasksFile.fileName());
}

ManagerApplication::~ManagerApplication()
{
}

void ManagerApplication::run()
{
	bool exitFlag = false;
	int exitCode = -1;

#pragma region Initialization
	emit out(ComputeGridGlobals::makeProcessCommand(PC_STATUS_MESSAGE, QStringList() << QString("Anka-Depth %1 - Initializing...")
			.arg(QString("%1.%2.%3.%4")
			.arg(AnkaDepthLibGlobals::VersionMajor)
			.arg(AnkaDepthLibGlobals::VersionMinor)
			.arg(AnkaDepthLibGlobals::VersionPatch)
			.arg(AnkaDepthLibGlobals::VersionBuild)
		)));

	QStringList & rootDirs = mConfig.inputRootDirs();
	QStringList & subDirs = mConfig.inputSubDirs();
	if (rootDirs.count() > 0)
	{
		QSqlDatabase db = QSqlDatabase::addDatabase("QPSQL", "kgmdb");
		db.setHostName(mConfig.kgmDatabaseIp());
		db.setPort(mConfig.kgmDatabasePort());
		db.setDatabaseName(mConfig.kgmDatabaseName());
		db.setUserName(mConfig.kgmDatabaseUserName());
		db.setPassword(mConfig.kgmDatabasePassword());
		db.setConnectOptions(mConfig.kgmDatabaseOptions());

		if (db.open())
		{
			emit out(ComputeGridGlobals::makeLogCommand(LS_MP, LT_INFO, QString("Retrieving regions from the database...")));

			QString roots = "(";
			for (QStringList::iterator it = rootDirs.begin(); it != rootDirs.end(); ++it)
				roots += QString("'%1',").arg(*it);

			if (roots.endsWith(','))
				roots[roots.length() - 1] = ')';

			QString subs = "(";
			for (QStringList::iterator it = subDirs.begin(); it != subDirs.end(); ++it)
				subs += QString("'%1',").arg(*it);

			if (subs.endsWith(','))
				subs[subs.length() - 1] = ')';

			QString qStr = QString(
				"SELECT \
					id,\
					coordx as lon,\
					coordy as lat,\
					altitude,\
					ST_X(ST_Transform(ST_GeomFromText(CONCAT('Point(', CAST(coordx as text), ' ', CAST(coordy as text), ')'), 4326), 32635)) as x,\
					ST_Y(ST_Transform(ST_GeomFromText(CONCAT('Point(', CAST(coordx as text), ' ', CAST(coordy as text), ')'), 4326), 32635)) as y,\
					dirname as parent_dir,\
					filename as sub_dir,\
					imgname as file_name,\
					heading,\
					pitch,\
					roll,\
					stamp\
				FROM public.panogps\
				WHERE dirname IN %1 %2"
			).arg(roots).arg((subDirs.count() > 0 ? QString("AND filename IN %1").arg(subs) : ""));

			QSqlQuery query(db);
			query.setForwardOnly(true);
			if (query.exec(qStr))
			{
				mRWLock.lockForWrite();
				while (query.next())
					mTasks.append(new DepthTask(query));

				// sort by id
				qSort(mTasks.begin(), mTasks.end(), AnkaDepthLib::DepthTaskIdLessThan);
				mTotalTasksCount = mTasks.count();
				mRWLock.unlock();

				emit out(ComputeGridGlobals::makeLogCommand(LS_MP, LT_INFO, QString("%1 regions are retrieved from the database.").arg(mTotalTasksCount)));

				if (!mConfig.managerReprocess())
				{
					emit out(ComputeGridGlobals::makeLogCommand(LS_MP, LT_WARNING, QString("Dropping processed regions in the past runs due to 'ManagerReprocess' option is not specified...")));

					QList<int> dropRegions;
					if (mCompletedTasksFile.exists() && mCompletedTasksFile.open(QIODevice::ReadOnly))
					{
						emit out(ComputeGridGlobals::makeLogCommand(LS_MP, LT_INFO, QString("Parsing the file %1 ...").arg(mCompletedTasksFile.fileName())));

						QTextStream in(&mCompletedTasksFile);
						while (!in.atEnd())
						{
							dropRegions.push_back(in.readLine().toInt());
							++mCompletedTaskCounter;
						}
						mCompletedTasksFile.close();
						emit out(ComputeGridGlobals::makeLogCommand(LS_MP, LT_INFO, QString("%1 regions marked as completed.").arg(mCompletedTaskCounter)));
					}

					if (mFailedTasksFile.exists() && mFailedTasksFile.open(QIODevice::ReadOnly))
					{
						emit out(ComputeGridGlobals::makeLogCommand(LS_MP, LT_INFO, QString("Parsing the file %1 ...").arg(mFailedTasksFile.fileName())));

						QTextStream in(&mFailedTasksFile);
						while (!in.atEnd())
						{
							dropRegions.push_back(in.readLine().toInt());
							++mFailedTaskCounter;
						}
						mFailedTasksFile.close();
						emit out(ComputeGridGlobals::makeLogCommand(LS_MP, LT_INFO, QString("%1 regions marked as failed.").arg(mFailedTaskCounter)));
					}

					if (dropRegions.size() > 0)
					{
						qSort(dropRegions.begin(), dropRegions.end());

						mRWLock.lockForWrite();
						DepthTaskList::iterator it = mTasks.begin();
						int ind = -1;
						while (it != mTasks.end())
						{
							DepthTask * dt = *it;
							if ((ind = dropRegions.indexOf(dt->id())) >= 0)
							{
								it = mTasks.erase(it);
								delete dt;
								dropRegions.removeAt(ind);
							}
							else
								++it;
						}
						mRWLock.unlock();

						emit out(ComputeGridGlobals::makeLogCommand(LS_MP, LT_WARNING, QString("%1 regions are dropped.").arg(mCompletedTaskCounter + mFailedTaskCounter)));
					}
				}
			}
			else
			{
				emit out(ComputeGridGlobals::makeLogCommand(LS_MP, LT_ERROR, QString("Query execution error: %1").arg(db.lastError().text())));
				exitFlag = true;
				exitCode = -3;
			}

			db.close();
		}
		else
		{
			emit out(ComputeGridGlobals::makeLogCommand(LS_MP, LT_ERROR, QString("Database connection error: %1").arg(db.lastError().text())));
			exitFlag = true;
			exitCode = -2;
		}
	}
	else
	{
		emit out(ComputeGridGlobals::makeLogCommand(LS_MP, LT_ERROR, QString("Invalid input parameters. Check configuration!")));
		exitFlag = true;
	}
	QSqlDatabase::removeDatabase("kgmdb");
#pragma endregion

#pragma region Main-Loop
	bool running = false;
	int pendingTasks = 0;
	QString status;
	while (!exitFlag)
	{
		if (running = checkIfNeedToWork())
		{
			pendingTasks = 0;

			// task assign
			mRWLock.lockForWrite();
			DepthTask * task = nullptr;
			for (int i = 0; i < mWorkers.count(); ++i)
			{
				QString worker = mWorkers[i];

				// timeout check
				for (DepthTaskList::iterator it = mWorkerTasksMap[worker].begin(); it != mWorkerTasksMap[worker].end();)
				{
					task = *it;
					if (task->assingmentDateTime().secsTo(QDateTime::currentDateTime()) >= 600) // to do: make parametric
					{
						emit out(ComputeGridGlobals::makeLogCommand(LS_MP, LT_WARNING, QString("Region ID: %1 => Execution timed out on worker: %2. Task reqeueued.").arg(task->id()).arg(worker)));
						mTasks.append(task);
						it = mWorkerTasksMap[worker].erase(it);
					}
					else
						++it;
				}

				// assign new tasks
				task = nullptr;
				while (mWorkerTasksMap[worker].count() < mWorkerCapacityMap[worker] && mTasks.count() > 0)
				{
					task = mTasks.takeAt(mTasks.count() > mWorkers.count() ? ((mTasks.count() / mWorkers.count()) * i) : 0);
					task->setAssignmentDateTime(QDateTime::currentDateTime());
					mWorkerTasksMap[worker].push_back(task);
					emit out(ComputeGridGlobals::makeProcessCommand(PC_WORKER_DATA, QStringList() << worker << QString::number(DTPT_TASK_EXECUTE) << task->toString()));
				}
				pendingTasks += mWorkerTasksMap[worker].count();
			}

			if (mTasks.count() == 0 && pendingTasks == 0)
			{
				emit out(ComputeGridGlobals::makeLogCommand(LS_MP, LT_WARNING, QString("All tasks are completed.")));
				exitCode = 0;
				exitFlag = true;
			}
			mRWLock.unlock();
		}

		// status update
		mRWLock.lockForRead();
		status = QString("Anka-Depth v%1 - Status: %2, Completed: <font color=\"green\">%3</font>, Failed: <font color=\"red\">%4</font>, Total: <font color=\"blue\">%5</font>, Workers: <font color=\"blue\">%6</font>, Progress: <font color=\"blue\">%%7</font>")
			.arg(QString("%1.%2.%3.%4").arg(AnkaDepthLibGlobals::VersionMajor).arg(AnkaDepthLibGlobals::VersionMinor).arg(AnkaDepthLibGlobals::VersionPatch).arg(AnkaDepthLibGlobals::VersionBuild))
			.arg(running ? "<font color=\"green\">Running</font>" : "<font color=\"red\">Waiting</font>")
			.arg(mCompletedTaskCounter)
			.arg(mFailedTaskCounter)
			.arg(mTotalTasksCount)
			.arg(mWorkers.count())
			.arg(QString::number((double)(mFailedTaskCounter + mCompletedTaskCounter) / (double)mTotalTasksCount * 100.0, 'f', 2));
		mRWLock.unlock();

		if (mLastStatus != status)
			emit out(ComputeGridGlobals::makeProcessCommand(PC_STATUS_MESSAGE, QStringList() << (mLastStatus = status)));
	}
#pragma endregion

	emit out(ComputeGridGlobals::makeLogCommand(LS_MP, LT_WARNING, QString("Process is exiting...")));
	qApp->exit(exitCode);
}

void ManagerApplication::workerIn(QStringList _args)
{
	if (_args.count() == 2)
	{
		QString worker = _args[0];
		int cap = _args[1].toInt();

		mRWLock.lockForWrite();
		mWorkers.append(worker);
		mWorkerCapacityMap[worker] = cap;
		emit out(ComputeGridGlobals::makeProcessCommand(PC_WORKER_DATA, QStringList() << worker << QString::number(DTPT_TASK_CONFIG) << mConfig.toString()));
		mRWLock.unlock();

		emit out(ComputeGridGlobals::makeLogCommand(LS_MP, LT_WARNING, QString("Worker: %1 has joined to the compute-grid with %2 parallel computing capacity.").arg(worker).arg(cap)));
	}
	else
		emit out(ComputeGridGlobals::makeLogCommand(LS_MP, LT_ERROR, QString("Unexpected 'workerIn' arguments: %1").arg(_args.join(ComputeGridGlobals::ProcessCommandDataSeperator))));
}

void ManagerApplication::workerOut(QStringList _args)
{
	if (_args.count() == 1)
	{
		QString worker = _args[0];

		mRWLock.lockForWrite();

		// de-assign worker's tasks
		if (mWorkerTasksMap.contains(worker))
		{
			for (DepthTaskList::reverse_iterator it = mWorkerTasksMap[worker].rbegin(); it != mWorkerTasksMap[worker].rend(); ++it)
				mTasks.insert(mTasks.begin(), *it);

			mWorkerTasksMap.remove(worker);
		}

		// remove worker capacity
		if(mWorkerCapacityMap.contains(worker))
			mWorkerCapacityMap.remove(worker);
		
		// remove worker
		int ind = -1;
		if ((ind = mWorkers.indexOf(worker)) >= 0)
		{
			mWorkers.removeAt(ind);
			emit out(ComputeGridGlobals::makeLogCommand(LS_MP, LT_WARNING, QString("Worker: %1 is out of grid.").arg(worker)));
		}

		mRWLock.unlock();
	}
	else
		emit out(ComputeGridGlobals::makeLogCommand(LS_MP, LT_ERROR, QString("Unexpected 'workerOut' arguments: %1").arg(_args.join(ComputeGridGlobals::ProcessCommandDataSeperator))));
}

void ManagerApplication::workerData(QStringList _args)
{
	if (_args.count() >= 3)
	{
		QString worker = _args[0];
		DepthTaskParameterType dtpt = (DepthTaskParameterType)_args[1].toInt();

		switch (dtpt)
		{
		case AnkaDepthLib::DTPT_TASK_RESULT:
			mRWLock.lockForWrite();
			for (int i = 0; i < mWorkerTasksMap[worker].count(); i++)
			{
				DepthTask * task = mWorkerTasksMap[worker].at(i);
				if (task->id() == _args[3].toInt())
				{
					if ((DepthTaskWorkerStatus)_args[2].toInt() == DTWS_COMPLETED)
					{
						logCompletedTask(*task);
						mCompletedTaskCounter++;
					}
					else
					{
						logFailedTask(*task);
						mFailedTaskCounter++;
					}
						
					mWorkerTasksMap[worker].removeAt(i);
					delete task;
					break; //for
				}
			}
			mRWLock.unlock();
			break;

		default:
			emit out(ComputeGridGlobals::makeLogCommand(LS_MP, LT_ERROR, QString("Unexpected task arguments.")));
			break;
		}
	}
	else
		emit out(ComputeGridGlobals::makeLogCommand(LS_MP, LT_ERROR, QString("Unexpected 'workerData' arguments: %1").arg(_args.join(ComputeGridGlobals::ProcessCommandDataSeperator))));
}

void ManagerApplication::workerExit(QStringList _args)
{
	workerOut(_args);
}

void ManagerApplication::terminalCommand(QStringList _args)
{
	if (_args.count() >= 1)
	{
		QString cmd = _args.takeFirst();
		if (cmd == "start" || cmd == "stop")
		{
			mRWLock.lockForWrite();
			mStartFlag = (cmd == "start");
			mRWLock.unlock();
		}
		else if (cmd == "tasks")
		{
			mRWLock.lockForRead();
			for (int i = 0; i < mWorkers.count(); ++i)
			{
				for (int j = 0; j < mWorkerTasksMap[mWorkers[i]].count(); j++)
					emit out(ComputeGridGlobals::makeLogCommand(LS_MP, LT_INFO, QString("* Worker: %1 Task Id: %2").arg(mWorkers[i]).arg(mWorkerTasksMap[mWorkers[i]][j]->id())));
			}
			mRWLock.unlock();
		}
		else if (_args.count() > 1 && cmd == "dropworker")
			workerOut(QStringList() << _args.first());
		else if (_args.count() > 0 && cmd == "sysworkers")
		{
			mRWLock.lockForRead();
			for (int i = 0; i < mWorkers.count(); ++i)
				emit out(ComputeGridGlobals::makeProcessCommand(PC_WORKER_DATA, QStringList() << mWorkers[i] << QString::number(DTPT_SYS_CALL) << _args.join(' ')));
			mRWLock.unlock();
		}
		else if (_args.count() == 3 && cmd == "sysworker")
		{
			QString worker = _args.takeFirst();
			emit out(ComputeGridGlobals::makeProcessCommand(PC_WORKER_DATA, QStringList() << worker << QString::number(DTPT_SYS_CALL) << _args.join(' ')));
		}
		else
			emit out(ComputeGridGlobals::makeLogCommand(LS_MP, LT_ERROR, QString("Unknown terminal command: %1").arg(_args[0])));
	}
	else
		emit out(ComputeGridGlobals::makeLogCommand(LS_MP, LT_ERROR, QString("Unexpected 'terminalCommand' arguments: %1").arg(_args.join(ComputeGridGlobals::ProcessCommandDataSeperator))));
}

void ManagerApplication::logFailedTask(DepthTask & _task)
{
	if (mFailedTasksFile.open(QIODevice::Append | QIODevice::Text))
	{
		QTextStream out(&mFailedTasksFile);
		out << QString::number(_task.id()) << endl;
		mFailedTasksFile.close();
	}
	else
		emit out(ComputeGridGlobals::makeLogCommand(LS_MP, LT_ERROR, QString("Log file %1 couldn't open.").arg(mFailedTasksFile.fileName())));
}

void ManagerApplication::logCompletedTask(DepthTask & _task)
{
	if (mCompletedTasksFile.open(QIODevice::Append | QIODevice::Text))
	{
		QTextStream out(&mCompletedTasksFile);
		out << QString::number(_task.id()) << endl;
		mCompletedTasksFile.close();
	}
	else
		emit out(ComputeGridGlobals::makeLogCommand(LS_MP, LT_ERROR, QString("Log file %1 couldn't open.").arg(mCompletedTasksFile.fileName())));
}

bool ManagerApplication::checkIfNeedToWork()
{
	bool res = false;

	mRWLock.lockForRead();
	res = mWorkers.count() > 0 && mStartFlag;
	mRWLock.unlock();

	// schedule check
	if (res && mConfig.scheduledWork())
	{
		QDateTime dt = QDateTime::currentDateTime();
		if ((dt.date().dayOfWeek() == 6 && mConfig.fullDayWorkAtSaturday())
			|| (dt.date().dayOfWeek() == 7 && mConfig.fullDayWorkAtSunday()))
			res = true;
		else
		{
			QTime tCur = dt.time();
			int msStart = mConfig.startWorkTime().msecsSinceStartOfDay();
			int msStop = mConfig.stopWorkTime().msecsSinceStartOfDay();
			int msCur = tCur.msecsSinceStartOfDay();

			if (msStop <= msStart)
			{
				msCur = tCur.addMSecs(-msStop).msecsSinceStartOfDay();
				msStart -= msStop;
				msStop = 86400000;
			}

			res = msCur >= msStart && msCur < msStop;
		}
	}

	return res;
}
