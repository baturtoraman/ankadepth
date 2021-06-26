#pragma once

#include <QObject>
#include <QDataStream>
#include <QSqlQuery>
#include <QDateTime>

namespace AnkaDepthLib
{
	class DepthTask : public QObject
	{
		Q_OBJECT
	public:
		DepthTask(QObject * _parent = nullptr);
		DepthTask(QSqlQuery & _query, QObject * _parent = nullptr);
		DepthTask(const DepthTask & _other);
		DepthTask(DepthTask && _other);
		DepthTask(const QString & _string);
		~DepthTask();

		DepthTask & operator=(const DepthTask & _other);
		DepthTask & operator=(DepthTask && _other);

		friend QDataStream & operator<<(QDataStream & _out, const DepthTask & _dtp);
		friend QDataStream & operator>>(QDataStream & _in, DepthTask & _dtp);

		QString toString() const;
		void fromString(const QString & _string);

#pragma region Getters-Setters
		int id();
		void setId(int _id);

		double longtitude();
		void setLongtitude(double _longtitude);

		double latitude();
		void setLatitude(double _latitude);

		double x();
		void setX(double _x);

		double y();
		void setY(double _y);

		double altitude();
		void setAltitude(double _altitude);

		double heading();
		void setHeading(double _heading);

		double pitch();
		void setPitch(double _pitch);

		double roll();
		void setRoll(double _roll);

		QString parentDir();
		void setParentDir(QString & _parentDir);

		QString subDir();
		void setSubDir(QString & _subDir);

		QString fileName();
		void setFileName(QString & _fileName);

		QString timeStamp();
		void setTimeStamp(QString & _timeStamp);

		QDateTime assingmentDateTime();
		void setAssignmentDateTime(QDateTime & _dateTime);
#pragma endregion

	private:
		int mId;

		double
			mLongtitude,
			mLatitude,
			mX,
			mY,
			mAltitude,
			mHeading,
			mPitch,
			mRoll;

		QString
			mParentDir,
			mSubDir,
			mFileName,
			mTimeStamp;

		QDateTime mAssingmentDateTime;
	};

	typedef QList<DepthTask *> DepthTaskList;

#pragma region DepthTask Comparators
	static bool DepthTaskIdLessThan(const DepthTask * _left, const DepthTask * _right)
	{
		return const_cast<DepthTask *>(_left)->id() < const_cast<DepthTask *>(_right)->id();
	}
#pragma endregion
}