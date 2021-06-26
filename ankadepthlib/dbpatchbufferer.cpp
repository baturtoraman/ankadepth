#include "dbpatchbufferer.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlResult>
#include <QSqlError>
#include <QThread>

using namespace AnkaDepthLib;

QQueue<int> DBPatchBufferer::mPatchBufferQueue;
QMap<int, LidarPointVector> DBPatchBufferer::mPatchBufferMap;
QReadWriteLock DBPatchBufferer::mRWLock;
DepthConfiguration * DBPatchBufferer::mDepthConfig = nullptr;

DBPatchBufferer::DBPatchBufferer(QObject * _parent)
	: QObject(_parent)
{
}

DBPatchBufferer::~DBPatchBufferer()
{
}

void AnkaDepthLib::DBPatchBufferer::init(DepthConfiguration * _depthConfig)
{
	mDepthConfig = _depthConfig;
}

bool DBPatchBufferer::loadPatch(int _patchId, QString _lon, QString _lat, LidarPointVector * _pointsOut)
{
	bool
		res = false,
		contains = false,
		dbContains = false,
		dbLoad = false;

	QString dbName = QString("db_patch_%1").arg(_patchId);

	mRWLock.lockForWrite();
	res = contains = mPatchBufferQueue.contains(_patchId);
	dbContains = QSqlDatabase::contains(dbName);
	if (dbLoad = (!contains && !dbContains))
	{
		QSqlDatabase db = QSqlDatabase::addDatabase("QPSQL", dbName);
		db.setHostName(mDepthConfig->pcDatabaseIp());
		db.setPort(mDepthConfig->pcDatabasePort());
		db.setDatabaseName(mDepthConfig->pcDatabaseName());
		db.setUserName(mDepthConfig->pcDatabaseUserName());
		db.setPassword(mDepthConfig->pcDatabasePassword());
		db.setConnectOptions(mDepthConfig->pcDatabaseOptions());
	}
	mRWLock.unlock();

	while (!dbLoad && !contains)
	{
		mRWLock.lockForRead();
		res = contains = mPatchBufferQueue.contains(_patchId) && !QSqlDatabase::contains(dbName);
		mRWLock.unlock();
	}

	if (dbLoad)
	{
		// db scope
		{
			QSqlDatabase db = QSqlDatabase::database(dbName);

			QString qStr = QString("\
			WITH points AS (\
				SELECT PC_Explode(pa) AS point\
				FROM pc_table\
				WHERE id=%1\
			)\
			SELECT\
			ST_X(ST_Transform(point::geometry, 32635)) as x,\
			ST_Y(ST_Transform(point::geometry, 32635)) as y,\
			ST_Z(ST_Transform(point::geometry, 32635)) as z,\
			PC_GET(point, 'gpstime') as gpstime,\
			PC_GET(point, 'intensity') as intensity\
			FROM points\
			").arg(_patchId);

			if (db.open())
			{
				QSqlQuery query(db);
				query.setForwardOnly(true);
				if (query.exec(qStr))
				{
					mRWLock.lockForWrite();
					while(mPatchBufferQueue.count() >= (QThread::idealThreadCount() * mDepthConfig->patchLimit()))
						mPatchBufferMap.remove(mPatchBufferQueue.dequeue());

					while (query.next())
					{
						LidarPoint p(
							query.value(0).toDouble(),
							query.value(1).toDouble(),
							query.value(2).toDouble(),
							query.value(3).toLongLong(),
							query.value(4).toInt()
						);

						mPatchBufferMap[_patchId].push_back(p);

						if (_pointsOut)
							_pointsOut->push_back(p);
					}
					mPatchBufferQueue.enqueue(_patchId);

					mRWLock.unlock();
					query.finish();
					res = true;
				}
				db.close();
			}
		}
		mRWLock.lockForWrite();
		QSqlDatabase::removeDatabase(dbName);
		mRWLock.unlock();
	}
	else if (contains && _pointsOut)
	{
		mRWLock.lockForRead();
		_pointsOut->append(mPatchBufferMap[_patchId]);
		mRWLock.unlock();
	}

	return res;
}

bool DBPatchBufferer::loadPatches(QVector<int> _patches, QString _lon, QString _lat, LidarPointVector * _pointsOut)
{
	for (QVector<int>::iterator it = _patches.begin(); it != _patches.end(); ++it)
		if (!loadPatch(*it, _lon, _lat, _pointsOut))
			return false;

	return true;
}

void DBPatchBufferer::clearBuffer()
{
	mRWLock.lockForWrite();

	mPatchBufferMap.clear();
	mPatchBufferQueue.clear();

	mRWLock.unlock();
}