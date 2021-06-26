#pragma once

#include <QObject>
#include <QQueue>
#include <QMap>
#include <QReadWriteLock>
#include "lidarpoint.h"
#include "depthconfiguration.h"

namespace AnkaDepthLib
{
	class DBPatchBufferer : public QObject
	{
		Q_OBJECT
	public:
		~DBPatchBufferer();
		static void init(DepthConfiguration * _depthConfig);
		static bool loadPatch(int _patchId, QString _lon, QString _lat, LidarPointVector * _pointsOut = nullptr);
		static bool loadPatches(QVector<int> _patchIds, QString _lon, QString _lat, LidarPointVector * _pointsOut = nullptr);
		static void clearBuffer();

	private:
		DBPatchBufferer(QObject * _parent = nullptr);
		static QQueue<int> mPatchBufferQueue;
		static QMap<int, LidarPointVector> mPatchBufferMap;
		static QReadWriteLock mRWLock;
		static DepthConfiguration * mDepthConfig;
	};
}