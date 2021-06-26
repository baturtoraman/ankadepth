#pragma once

#include <QObject>
#include <QRunnable>
#include "ankadepthlibglobals.h"
#include "depthconfiguration.h"
#include "depthtask.h"

namespace AnkaDepthLib
{
	class DepthTaskWorker : public QObject, public QRunnable
	{
		Q_OBJECT
	public:
		DepthTaskWorker(DepthConfiguration * _config, DepthTask & _task, QObject * _parent = nullptr);
		~DepthTaskWorker();

		void run();
		void stop();
		bool isInterrupted();
		int id();
		DepthTaskWorkerStatus status();
		void setStatus(DepthTaskWorkerStatus _status);

	private:
		bool loadPoints();
		void holeFilter(cv::Mat & _image);

		bool mInterrupted;
		DepthTaskWorkerStatus mStatus;
		DepthConfiguration * mConfig;
		DepthTask mTask;
		LidarPoint mLPCenter;
		LidarPointVector mPoints;
		LidarPointDistSliceMap mDistSliceMap;
		QString mOutPath;
		double mCameraOffset;
		double mHeadingOffset;
		double mPitchOffset;
		double mRollOffset;

#pragma region Signals-Slots
	signals:
		void finished(DepthTaskWorker * _taskWorker);
		void progress(DepthTaskWorker * _taskWorker, QString _message);
		void error(DepthTaskWorker * _taskWorker, QString _error);
#pragma endregion

	};

	typedef QList<DepthTaskWorker *> DepthTaskWorkerList;

#pragma region DepthWorkItem Comparators
	static bool DepthTaskWorkerIdLessThan(const DepthTaskWorker * _left, const DepthTaskWorker * _right)
	{
		return const_cast<DepthTaskWorker *>(_left)->id() < const_cast<DepthTaskWorker *>(_right)->id();
	}
#pragma endregion
}