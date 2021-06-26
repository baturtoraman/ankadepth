#include "depthtask.h"
#include "computegridcommons.hpp"
#include <QVariant>

using namespace AnkaDepthLib;

AnkaDepthLib::DepthTask::DepthTask(QObject * _parent)
	: QObject(_parent)
{
}

AnkaDepthLib::DepthTask::DepthTask(QSqlQuery & _query, QObject * _parent)
	: QObject(_parent)
{
	mId = _query.value("id").toInt();
	mLongtitude = _query.value("lon").toDouble();
	mLatitude = _query.value("lat").toDouble();
	mAltitude = _query.value("altitude").toDouble();
	mX = _query.value("x").toDouble();
	mY = _query.value("y").toDouble();
	mParentDir = _query.value("parent_dir").toString();
	mSubDir = _query.value("sub_dir").toString();
	mFileName = _query.value("file_name").toString().split('.')[0] + ".png";
	mHeading = _query.value("heading").toDouble();
	mPitch = _query.value("pitch").toDouble();
	mRoll = _query.value("roll").toDouble();
	mTimeStamp = _query.value("stamp").toString();
}

AnkaDepthLib::DepthTask::DepthTask(const AnkaDepthLib::DepthTask & _other)
{
	(*this) = _other;
}

AnkaDepthLib::DepthTask::DepthTask(AnkaDepthLib::DepthTask && _other)
{
	(*this) = std::move(_other);
}

AnkaDepthLib::DepthTask::DepthTask(const QString & _string)
{
	fromString(_string);
}

AnkaDepthLib::DepthTask::~DepthTask()
{
}

DepthTask & AnkaDepthLib::DepthTask::operator=(const DepthTask & _other)
{
	if (this != &_other)
	{
		mId = _other.mId;
		mLongtitude = _other.mLongtitude;
		mLatitude = _other.mLatitude;
		mX = _other.mX;
		mY = _other.mY;
		mAltitude = _other.mAltitude;
		mHeading = _other.mHeading;
		mPitch = _other.mPitch;
		mRoll = _other.mRoll;
		mParentDir = _other.mParentDir;
		mSubDir = _other.mSubDir;
		mFileName = _other.mFileName;
		mTimeStamp = _other.mTimeStamp;
	}

	return *this;
}

DepthTask & AnkaDepthLib::DepthTask::operator=(DepthTask && _other)
{
	if (this != &_other)
	{
		mId = _other.mId;
		mLongtitude = _other.mLongtitude;
		mLatitude = _other.mLatitude;
		mX = _other.mX;
		mY = _other.mY;
		mAltitude = _other.mAltitude;
		mHeading = _other.mHeading;
		mPitch = _other.mPitch;
		mRoll = _other.mRoll;
		mParentDir = _other.mParentDir;
		mSubDir = _other.mSubDir;
		mFileName = _other.mFileName;
		mTimeStamp = _other.mTimeStamp;

		_other.mId = 0;
		_other.mLongtitude = 0;
		_other.mLatitude = 0;
		_other.mX = 0;
		_other.mY = 0;
		_other.mAltitude = 0;
		_other.mHeading = 0;
		_other.mPitch = 0;
		_other.mRoll = 0;
		_other.mParentDir.clear();
		_other.mSubDir.clear();
		_other.mFileName.clear();
		_other.mTimeStamp.clear();
	}

	return *this;
}

QDataStream & AnkaDepthLib::operator<<(QDataStream & _out, const DepthTask & _dtp)
{
	_out << _dtp.toString();
	return _out;
}

QDataStream & AnkaDepthLib::operator>>(QDataStream & _in, DepthTask & _dtp)
{
	QString s;
	_in >> s;
	_dtp.fromString(s);
	return _in;
}

QString AnkaDepthLib::DepthTask::toString() const
{
	QStringList sl;
	sl << QString::number(mId);
	sl << QString::number(mLongtitude, 'f', 12);
	sl << QString::number(mLatitude, 'f', 12);
	sl << QString::number(mX, 'f', 12);
	sl << QString::number(mY, 'f', 12);
	sl << QString::number(mAltitude, 'f', 12);
	sl << QString::number(mHeading, 'f', 12);
	sl << QString::number(mPitch, 'f', 12);
	sl << QString::number(mRoll, 'f', 12);
	sl << mParentDir;
	sl << mSubDir;
	sl << mFileName;
	sl << mTimeStamp;
	return sl.join(ComputeGrid::ComputeGridGlobals::ProcessCommandDataSeperator);
}
void AnkaDepthLib::DepthTask::fromString(const QString & _string)
{
	QStringList sl = _string.split(ComputeGrid::ComputeGridGlobals::ProcessCommandDataSeperator);
	mId = sl.takeFirst().toInt();
	mLongtitude = sl.takeFirst().toDouble();
	mLatitude = sl.takeFirst().toDouble();
	mX = sl.takeFirst().toDouble();
	mY = sl.takeFirst().toDouble();
	mAltitude = sl.takeFirst().toDouble();
	mHeading = sl.takeFirst().toDouble();
	mPitch = sl.takeFirst().toDouble();
	mRoll = sl.takeFirst().toDouble();
	mParentDir = sl.takeFirst();
	mSubDir = sl.takeFirst();
	mFileName = sl.takeFirst();
	mTimeStamp = sl.takeFirst();
}

#pragma region Getters-Setters
int AnkaDepthLib::DepthTask::id()
{
	return mId;
}

void AnkaDepthLib::DepthTask::setId(int _id)
{
	mId = _id;
}

double AnkaDepthLib::DepthTask::longtitude()
{
	return mLongtitude;
}

void AnkaDepthLib::DepthTask::setLongtitude(double _longtitude)
{
	mLongtitude = _longtitude;
}

double AnkaDepthLib::DepthTask::latitude()
{
	return mLatitude;
}

void AnkaDepthLib::DepthTask::setLatitude(double _latitude)
{
	mLatitude = _latitude;
}

double AnkaDepthLib::DepthTask::x()
{
	return mX;
}

void AnkaDepthLib::DepthTask::setX(double _x)
{
	mX = _x;
}

double AnkaDepthLib::DepthTask::y()
{
	return mY;
}

void AnkaDepthLib::DepthTask::setY(double _y)
{
	mY = _y;
}

double AnkaDepthLib::DepthTask::altitude()
{
	return mAltitude;
}

void AnkaDepthLib::DepthTask::setAltitude(double _altitude)
{
	mAltitude = _altitude;
}

double AnkaDepthLib::DepthTask::heading()
{
	return mHeading;
}

void AnkaDepthLib::DepthTask::setHeading(double _heading)
{
	mHeading = _heading;
}

double AnkaDepthLib::DepthTask::pitch()
{
	return mPitch;
}

void AnkaDepthLib::DepthTask::setPitch(double _pitch)
{
	mPitch = _pitch;
}

double AnkaDepthLib::DepthTask::roll()
{
	return mRoll;
}

void AnkaDepthLib::DepthTask::setRoll(double _roll)
{
	mRoll = _roll;
}

QString AnkaDepthLib::DepthTask::parentDir()
{
	return mParentDir;
}

void AnkaDepthLib::DepthTask::setParentDir(QString & _parentDir)
{
	mParentDir = _parentDir;
}

QString AnkaDepthLib::DepthTask::subDir()
{
	return mSubDir;
}

void AnkaDepthLib::DepthTask::setSubDir(QString & _subDir)
{
	mSubDir = _subDir;
}

QString AnkaDepthLib::DepthTask::fileName()
{
	return mFileName;
}

void AnkaDepthLib::DepthTask::setFileName(QString & _fileName)
{
	mFileName = _fileName;
}

QString AnkaDepthLib::DepthTask::timeStamp()
{
	return mTimeStamp;
}

void AnkaDepthLib::DepthTask::setTimeStamp(QString & _timeStamp)
{
	mTimeStamp = _timeStamp;
}

QDateTime AnkaDepthLib::DepthTask::assingmentDateTime()
{
	return mAssingmentDateTime;
}

void AnkaDepthLib::DepthTask::setAssignmentDateTime(QDateTime & _dateTime)
{
	mAssingmentDateTime = _dateTime;
}
#pragma endregion