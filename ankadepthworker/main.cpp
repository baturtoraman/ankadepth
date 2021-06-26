#include <QtCore/QCoreApplication>
#include <QObject>
#include <QTextStream>
#include <QMutex>
#include <QtConcurrent/qtconcurrentrun.h>
#include "workerapplication.h"
#include "computegridcommons.hpp"

using namespace ComputeGrid;

QTextStream
outStream(stdout),
inStream(stdin);

QMutex outStreamMutex;

void out(QString _cmd)
{
	outStreamMutex.lock();
	outStream << _cmd << endl;
	outStreamMutex.unlock();
}

int main(int argc, char *argv[])
{
	QCoreApplication coreApp(argc, argv);

#pragma region test call
	if (argc > 1 && QString(argv[1]) == "-test")
	{
		// to do: perform test
		return 1;
	}
#pragma endregion

	system("netmap.bat");

	qsrand(0);
	WorkerApplication * app = new WorkerApplication();
	QObject::connect(app, &WorkerApplication::out, out);
	QObject::connect(app, SIGNAL(finished()), app, SLOT(deleteLater()));
	app->start();

	QtConcurrent::run([=]() {
		QString cmd;
		ProcessCommand pc;
		QStringList args;
		while (true)
		{
			cmd = inStream.readLine();
			args.clear();

			if (ComputeGridGlobals::parseProcessCommand(cmd, pc, args))
			{
				switch (pc)
				{
				case ComputeGrid::PC_WORKER_DATA:
					QMetaObject::invokeMethod(app, "workerData", Q_ARG(QStringList, args));
					break;

				case ComputeGrid::PC_WORKER_EXIT:
					QMetaObject::invokeMethod(app, "workerExit", Q_ARG(QStringList, args));
					break;

				default:
					out(ComputeGridGlobals::makeLogCommand(
						LS_WP,
						LT_WARNING,
						QString("Unknown command: '%1'").arg(cmd.replace("\r\n", "").replace(ComputeGridGlobals::ProcessCommandSeperator, ComputeGridGlobals::ProcessCommandDataSeperator)))
					);
					break;
				}
			}
			else
				out(ComputeGridGlobals::makeLogCommand(
					LS_WP,
					LT_ERROR,
					QString("Command parse error: '%1'").arg(cmd.replace("\r\n", "").replace(ComputeGridGlobals::ProcessCommandSeperator, ComputeGridGlobals::ProcessCommandDataSeperator)))
				);
		}
	});

	return coreApp.exec();
}