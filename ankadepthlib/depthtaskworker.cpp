#include "depthtaskworker.h"
#include "dbpatchbufferer.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlResult>
#include <QSqlError>
#include <QDir>
#include <QThread>
#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

using namespace AnkaDepthLib;

DepthTaskWorker::DepthTaskWorker(DepthConfiguration * _config, DepthTask & _task, QObject * _parent)
	:
	QObject(_parent),
	mConfig(_config),
	mTask(_task),
	mStatus(DTWS_IDLE),
	mInterrupted(false),
	mCameraOffset(DEFAULT_CAM_OFFSET),
	mHeadingOffset(0),
	mPitchOffset(0),
	mRollOffset(0)
{
	setAutoDelete(false);

	mLPCenter = LidarPoint(mTask.x(), mTask.y(), mTask.altitude());
}

DepthTaskWorker::~DepthTaskWorker()
{
}

void DepthTaskWorker::run()
{
 	cv::TickMeter tm;
	tm.start();
	mStatus = DTWS_RUNNING;
	emit progress(this, QString("Region ID: %1 => Execution started.").arg(id()));

	mOutPath = QString("%1%2/%3/").arg(mConfig->outputRootPath()).arg(mTask.parentDir()).arg(mTask.subDir());
	QString outFile = QString("%1%2.png").arg(mOutPath).arg(mTask.fileName().split('.')[0]);
	if (QFile::exists(outFile) && !mConfig->workerReprocess())
	{
		mStatus = DTWS_COMPLETED;
		emit error(this, QString("Region ID: %1 => Execution halted, file already exists and 'WorkerReprocess' option is not specified. %2").arg(id()).arg(outFile));
		emit finished(this);
		return;
	}

	QFile ankFile(QString("%1%2/%3/setup.ank").arg(mConfig->ankRootPath()).arg(mTask.parentDir()).arg(mTask.subDir()));
	if (ankFile.exists() && ankFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		QJsonParseError jErr;
		QJsonDocument jDoc = QJsonDocument::fromJson(ankFile.readAll(), &jErr);
		if (jErr.error == QJsonParseError::NoError)
		{
			QJsonObject jObj = jDoc.object();
			if (!jObj.isEmpty()
				&& jObj.contains("camheight")
				&& jObj.contains("roll")
				&& jObj.contains("pitch")
				&& jObj.contains("heading"))
			{
				mCameraOffset = jObj["camheight"].toDouble();
				mRollOffset = jObj["roll"].toDouble();
				mPitchOffset = jObj["pitch"].toDouble();
				mHeadingOffset = jObj["heading"].toDouble();
			}
			else
				emit error(this, QString("WARNING: Region ID: %1 => setup.ank has incomplete JSON, defaults loaded. %2").arg(id()).arg(ankFile.fileName()));

			ankFile.close();
		}
		else
			emit error(this, QString("WARNING: Region ID: %1 => setup.ank has unknown JSON format, defaults loaded. %2").arg(id()).arg(ankFile.fileName()));
	}
	else
		emit error(this, QString("WARNING: Region ID: %1 => setup.ank couldn't located at path, defaults loaded. %2").arg(id()).arg(ankFile.fileName()));
#pragma region Definitions
	
	cv::Mat imgIn((int)H, (int)W, CV_32FC1, cv::Scalar(0));
	cv::Mat imgTemp((int)H, (int)W, CV_32FC1, cv::Scalar(0));
	cv::Mat imgOut((int)H, (int)W, CV_8UC3, cv::Scalar(0, 0, 0));
	float d = 0;
	double
		x = 0,
		y = 0,
		z = 0,
		phi = 0,
		theta = 0,
		phiPerPix = 180.0 / H,
		thetaPerPix = 360.0 / W;
#pragma endregion

	// directory check
	QDir dir(mOutPath);
	if (!dir.exists() && !dir.mkpath("."))
	{
		mStatus = DTWS_ERROR_STATE;
		emit error(this, QString("Region ID: %1 => Execution halted, file system error, directory couldn't created. %2").arg(id()).arg(mOutPath));
		emit finished(this);
		return;
	}

	// load lidar points
	if (!loadPoints() && !mInterrupted)
	{
		emit finished(this);
		return;
	}

	// interruption check
	if (mInterrupted)
	{
		mStatus = DTWS_IDLE;
		emit error(this, QString("Region ID: %1 => Execution cancelled.").arg(id()));
		emit finished(this);
		return;
	}

#pragma region Depth Image Creation
	int slice = (MAX_DISTANCE / DISTANCE_SLICE), sz = 0;
	while (!mInterrupted && --slice >= 0)
	while (!mInterrupted && --slice >= 0)
	{
		imgTemp = 0;
		for (LidarPointVector::iterator itp = mDistSliceMap[slice].begin(); !mInterrupted && itp != mDistSliceMap[slice].end(); ++itp)
		{
			d = itp->R;

			if (d > 0)
			{
				imgTemp.at<float>(itp->get2D_Y(H), itp->get2D_X(W)) = d;

				sz = (int)(DISTANCED_SIZE_FACTOR - (itp->R / MAX_DISTANCE * DISTANCED_SIZE_FACTOR)) + 1;

				if (itp->R < NEAR_DISTANCE_THRESHOLD && itp->get2D_Y(H) - sz >= 0 && itp->get2D_X(W) - sz >= 0)
				{
					imgTemp.at<float>(itp->get2D_Y(H) - sz, itp->get2D_X(W) - sz) = d;
					imgTemp.at<float>(itp->get2D_Y(H) - sz, (itp->get2D_X(W) + sz) % (int)W) = d;
					imgTemp.at<float>((itp->get2D_Y(H) + sz) % (int)H, itp->get2D_X(W) - sz) = d;
					imgTemp.at<float>((itp->get2D_Y(H) + sz) % (int)H, (itp->get2D_X(W) + sz) % (int)W) = d;

					sz *= 2;
					if (itp->get2D_Y(H) - sz >= 0 && itp->get2D_X(W) - sz >= 0)
					{
						imgIn.at<float>(itp->get2D_Y(H) - sz, itp->get2D_X(W) - sz) = d;
						imgIn.at<float>(itp->get2D_Y(H) - sz, (itp->get2D_X(W) + sz) % (int)W) = d;
						imgIn.at<float>((itp->get2D_Y(H) + sz) % (int)H, itp->get2D_X(W) - sz) = d;
						imgIn.at<float>((itp->get2D_Y(H) + sz) % (int)H, (itp->get2D_X(W) + sz) % (int)W) = d;
					}
				}
			}
		}

		sz = ((int)(6.0f - (slice * DISTANCE_SLICE / MAX_DISTANCE * 6.0f)) * 2) + 1;
		cv::morphologyEx(imgTemp, imgTemp, cv::MORPH_CLOSE, cv::Mat::ones(sz, sz, CV_32FC1), cv::Point(-1, -1), sz);

		for (int r = 0; !mInterrupted && r < imgTemp.rows; ++r)
		{
			for (int c = 0; !mInterrupted && c < imgTemp.cols; ++c)
			{
				if ((d = imgTemp.at<float>(r, c)) > 0)
					imgIn.at<float>(r, c) = d;
			}
		}
	}
#pragma endregion

#pragma region Near-Ground Surface Regeneration
	cv::Mat & rotMat = rotationMatrix(
		180.0 + mHeadingOffset, // face to the front of the car
		mTask.pitch() + mPitchOffset,
		mTask.roll() + mRollOffset
	);
	double phiInv = 0;
	for (y = H - 1.0; !mInterrupted && y > H / 2.0; y--)
	{
		phiInv = deg2rad(180.0 - y * phiPerPix);
		d = mCameraOffset / cos(phiInv);
		phi = deg2rad(y * phiPerPix);

		for (x = 0; !mInterrupted && x < W; x++)
		{
			theta = deg2rad(x * thetaPerPix);

			if (y * phiPerPix > (GROUND_ASSERTION_MIN_ANGLE + abs(cos(theta)) * VISION_CURVE))
			{
				cv::Mat rotated = rotMat * (
					cv::Mat_<double>(3, 1) <<
					mCameraOffset / cos(phiInv) * sin(phi) * cos(theta),
					mCameraOffset / cos(phiInv) * sin(phi) * sin(theta),
					mCameraOffset / cos(phiInv) * cos(phi)
					);

				LidarPoint p(
					rotated.at<double>(0) + mLPCenter.X,
					rotated.at<double>(1) + mLPCenter.Y,
					rotated.at<double>(2) + mLPCenter.Z
				);
				p.faceTo(mLPCenter); // no heading here

				imgIn.at<float>(p.get2D_Y(H), p.get2D_X(W)) = d;
			}
		}
	}
#pragma endregion

#pragma region Post Filtering
	holeFilter(imgIn);

	cv::medianBlur(imgIn, imgIn, 3);

	imgTemp = 0;
	cv::bilateralFilter(imgIn, imgTemp, 50, 1, 25);

	for (int r = 0; !mInterrupted && r < imgTemp.rows; ++r)
	{
		for (int c = 0; !mInterrupted && c < imgTemp.cols; ++c)
		{
			if ((d = imgTemp.at<float>(r, c)) > 0)
				imgOut.at<cv::Vec3b>(r, c) = dist2pix(d);
		}
	}
#pragma endregion

	// interruption check
	if (mInterrupted)
	{
		mStatus = DTWS_IDLE;
		emit error(this, QString("Region ID: %1 => Execution cancelled.").arg(id()));
		emit finished(this);
		return;
	}

	// save depth image & exit
	cv::imwrite(outFile.toStdString(), imgOut);
	tm.stop();
	mStatus = QFile(outFile).exists() ? DTWS_COMPLETED : DTWS_ERROR_STATE;

	if (mStatus == DTWS_COMPLETED)
		emit progress(this, QString("Region ID: %1 => Execution finished successfully in %3 seconds. Output: %2").arg(id()).arg(outFile).arg(tm.getTimeSec()));
	else
		emit error(this, QString("Region ID: %1 => Execution failed. Output verification failed at: %2").arg(id()).arg(outFile));

	emit finished(this);
}

void DepthTaskWorker::stop()
{
	mInterrupted = true;
}

bool DepthTaskWorker::isInterrupted()
{
	return mInterrupted;
}

int DepthTaskWorker::id()
{
	return mTask.id();
}

DepthTaskWorkerStatus DepthTaskWorker::status()
{
	return mStatus;
}

void DepthTaskWorker::setStatus(DepthTaskWorkerStatus _status)
{
	mStatus = _status;
}

bool DepthTaskWorker::loadPoints()
{
	bool res = false;

	QVector<int> patchIds;
	// db scope
	{
		QSqlDatabase db = QSqlDatabase::addDatabase("QPSQL", QString("db_dtw_%1").arg(id()));
		db.setHostName(mConfig->pcDatabaseIp());
		db.setPort(mConfig->pcDatabasePort());
		db.setDatabaseName(mConfig->pcDatabaseName());
		db.setUserName(mConfig->pcDatabaseUserName());
		db.setPassword(mConfig->pcDatabasePassword());
		db.setConnectOptions(mConfig->pcDatabaseOptions());

		if (db.open())
		{
			QString qStr = QString("\
			WITH patches AS\
			(\
				SELECT\
				((PC_PatchMax(pa, 'gpstime') + PC_PatchMin(pa, 'gpstime')) / 2.0) / 1000000 as patch_time_avg,\
				(SELECT regexp_matches(filename, '(20[0-9]{2}.[0-9]{2}.[0-9]{2})', 'g'))[1] as patch_date,\
				*\
				FROM pc_table\
				WHERE PC_Intersects(ST_Transform(ST_Buffer(ST_Transform(ST_GeomFromText('Point(%1 %2)', 4326), 32635), %3), 4326), pa)\
				)\
			SELECT\
			id\
			FROM patches\
			WHERE ABS(\
				(date_part('epoch', to_timestamp(patch_date, 'YYYY-MM-DD')) + patch_time_avg) -\
				(date_part('epoch', to_timestamp('%4', 'YYYY-MM-DD HH24:MI:SS.FF')))\
			) < 50\
			ORDER BY id\
			LIMIT %5\
			").arg(mTask.longtitude()).arg(mTask.latitude()).arg((int)MAX_DISTANCE).arg(mTask.timeStamp()).arg(mConfig->patchLimit());

			QSqlQuery query(db);
			query.setForwardOnly(true);
			if (query.exec(qStr))
			{
				while (!mInterrupted && query.next())
					patchIds.push_back(query.value(0).toInt());

				if (!(res = patchIds.count() >= mConfig->patchThreshold()))
				{
					mStatus = DTWS_ERROR_STATE;
					emit error(this, QString("Region ID: %1 => Not enough patches to process! Retrieved patch count:%2 < threshold:%3").arg(id()).arg(patchIds.count()).arg(patchIds.count()));
				}
			}
			else
			{
				mStatus = DTWS_ERROR_STATE;
				emit error(this, QString("Region ID: %1 => Point cloud database error: %2").arg(id()).arg(db.lastError().text()));
			}
		}
		else
		{
			mStatus = DTWS_ERROR_STATE;
			emit error(this, QString("Region ID: %1 => Point cloud database connection error: %2").arg(id()).arg(db.lastError().text()));
		}
	}
	QSqlDatabase::removeDatabase(QString("db_dtw_%1").arg(id()));

	if (!mInterrupted && res)
	{
		LidarPointVector lpv;
		if (res = DBPatchBufferer::loadPatches(patchIds, QString::number(mTask.longtitude(), 'f', 12), QString::number(mTask.latitude(), 'f', 12), &lpv))
		{
			for (LidarPointVector::iterator it = lpv.begin(); !mInterrupted && it != lpv.end(); ++it)
			{
				LidarPoint & p = *it;
				p.faceTo(mLPCenter, mTask.heading());
				mDistSliceMap[p.R / DISTANCE_SLICE].push_back(p);
			}
		}
		else
		{
			mStatus = DTWS_ERROR_STATE;
			emit error(this, QString("Region ID: %1 => Patch bufferer error.").arg(id()));
		}
	}
	return res;
}

void DepthTaskWorker::holeFilter(cv::Mat & _image)
{
	for (int r = 100.0 * H / 180.0; !mInterrupted && r < _image.rows; r++)
	{
		for (int c = 0; !mInterrupted && c < _image.cols; c++)
		{
			if(_image.at<float>(r, c) == 0)
				_image.at<float>(r, c) = averageDistance(_image, r, c, 4);
		}
	}
}