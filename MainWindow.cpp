#include <QDateTime>
#include <QDir>
#include <QCollator>
#include <QFile>
#include <QSettings>
#include <QTranslator>

#include "SavesModel.h"

#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <QCryptographicHash>
QByteArray fileChecksum(QFile &file, QCryptographicHash::Algorithm hashAlgorithm = QCryptographicHash::Md5)
{
	if (file.exists() && file.open(QFile::ReadOnly)) {
		QCryptographicHash hash(hashAlgorithm);
		if (hash.addData(&file)) {
			file.close();
			return hash.result();
		}
	}
	return QByteArray();
}

inline QByteArray fileChecksum(const QString &fileName, QCryptographicHash::Algorithm hashAlgorithm = QCryptographicHash::Md5)
{
	QFile file(fileName);
	return fileChecksum(file, hashAlgorithm);
}

QString applicationVersionToString(const int version)
{
	return QString::number(majorApplicationVersion(version)) + '.' +
		   QString::number(minorApplicationVersion(version)) + '.' +
		   QString::number(microApplicationVersion(version));
}

int applicationVersionFromString(const QString &version)
{
	QStringList subVersions = version.split('.');
	int subVersionsCount = subVersions.count();
	return applicationVersion(subVersionsCount > 0 ? subVersions[0].toInt() : 1,
							  subVersionsCount > 1 ? subVersions[1].toInt() : 0,
							  subVersionsCount > 2 ? subVersions[2].toInt() : 0);
}

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
	, ui(new Ui::MainWindow)
{
	ui->setupUi(this);

	m_settings = new QSettings(QString("settings.ini"), QSettings::IniFormat, this);
	m_qtTranslator = new QTranslator();
	m_translator = new QTranslator();

	m_model = new SavesModel(this);
	ui->tableView_saves->setModel(m_model);

	connect(ui->tableView_saves->selectionModel(), &QItemSelectionModel::currentRowChanged,
			this, &MainWindow::onCurrentRowChanged);
	connect(ui->pushButton_restoreSelectedSave, &QPushButton::clicked,
			this, &MainWindow::restoreSelectedSave);
	connect(ui->pushButton_deleteSelectedSave, &QPushButton::clicked,
			this, &MainWindow::deleteSelectedSave);

	connect(ui->spinBox_originSaveCheckInterval, QOverload<int>::of(&QSpinBox::valueChanged),
			this, &MainWindow::setOriginSaveCheckInterval);
	connect(ui->pushButton_backupCurrentSave, &QPushButton::clicked,
			this, &MainWindow::backupCurrentSave);
	connect(ui->pushButton_quicksave, &QPushButton::clicked,
			this, &MainWindow::backupQuickSave);
	connect(ui->pushButton_restoreLastQuicksave, &QPushButton::clicked,
			this, &MainWindow::restoreLastQuickSave);

	readSettings();
	scanSaves();

	//FIXME: remove line below after translation finishing
	m_lang = "en_US";
}

MainWindow::~MainWindow()
{
	saveSettings();

	QApplication::removeTranslator(m_translator);
	QApplication::removeTranslator(m_qtTranslator);
	delete m_translator;
	delete m_qtTranslator;

	delete ui;
}

bool MainWindow::backupSave(const QString &backupSaveName)
{
	QString gameDataFolderPath = ui->lineEdit_gameDataFolderPath->text();
	QString savesDirPath = gameDataFolderPath + "saves/";
	QString backupSavePath = savesDirPath + backupSaveName + SavesModel::SavesExtension;

	if (gameDataFolderPath.isEmpty() || backupSaveName.isEmpty()) {
		return false;
	}

	QFile saveFile(gameDataFolderPath + GameSaveFileName);

	if (!saveFile.exists()) {
		return false;
	}

	if (!QDir(savesDirPath).exists()) {
		QDir(gameDataFolderPath).mkdir("saves");
	}

	if ((!QFile::exists(backupSavePath) || QFile::remove(backupSavePath)) && saveFile.copy(backupSavePath)) {
		int index = m_model->indexOf(backupSaveName);
		if (index != -1) {
			m_model->moveRow(QModelIndex(), index, QModelIndex(), 0);
			m_model->setData(m_model->index(0, 1), QFileInfo(backupSavePath).lastModified());
		} else {
			m_model->insertRow(0, QFileInfo(backupSavePath));
		}

		return true;
	}

	return false;
}

void MainWindow::restoreSave(const QString &backupSaveName)
{
	QString gameDataFolderPath = ui->lineEdit_gameDataFolderPath->text();
	QString savesDirPath = gameDataFolderPath + "saves/";
	QFile backupFile(savesDirPath + backupSaveName + SavesModel::SavesExtension);

	if (!gameDataFolderPath.isEmpty() && QDir(savesDirPath).exists() && backupFile.exists()) {
		QFile::remove(gameDataFolderPath + GameSaveFileName);
		backupFile.copy(gameDataFolderPath + GameSaveFileName);
		m_originSaveLastChecksum = fileChecksum(backupFile);
	}
}

bool MainWindow::setLanguage(const QString &lang)
{
	m_lang = lang;
	bool isTranslationLoaded = m_translator->load(QString(":/lang/lang_") + m_lang, ":/lang/");
	QApplication::installTranslator(m_translator);
	m_qtTranslator->load(QString(":/lang/qtbase_") + m_lang, ":/lang/");
	QApplication::installTranslator(m_qtTranslator);

	return isTranslationLoaded;
}

//public slots:

void MainWindow::backupQuickSave()
{
	int quicksavesCount = ui->spinBox_quicksavesCount->value();
	QString gameDataFolderPath = ui->lineEdit_gameDataFolderPath->text();
	QString savesDirPath = gameDataFolderPath + "saves/";

	if (quicksavesCount > 0 && !gameDataFolderPath.isEmpty()) {
		m_lastQuicksaveIndex++;

		if (m_lastQuicksaveIndex >= quicksavesCount) {
			m_lastQuicksaveIndex = 0;
		}

		backupSave(QuicksavePrefix + QString::number(m_lastQuicksaveIndex + 1));
	}
}

void MainWindow::restoreLastQuickSave()
{
	restoreSave(QuicksavePrefix + QString::number(m_lastQuicksaveIndex));
}

bool MainWindow::backupCurrentSave()
{
	return backupSave(ui->lineEdit_backupCurrentSave->text());
}

void MainWindow::setOriginSaveCheckInterval(const int interval)
{
	if (interval > 0) {
		if (m_timerId) {
			QObject::killTimer(m_timerId);
		}

		m_timerId = QObject::startTimer(interval);
	}
}

void MainWindow::restoreSelectedSave()
{
	QModelIndex currentIndex = ui->tableView_saves->selectionModel()->currentIndex();

	if (currentIndex.isValid()) {
		restoreSave(m_model->data(m_model->index(currentIndex.row(), 0)).toString());
	}
}

void MainWindow::deleteSelectedSave()
{
	QString gameDataFolderPath = ui->lineEdit_gameDataFolderPath->text();
	QString savesDirPath = gameDataFolderPath + "saves/";
	QModelIndex currentIndex = ui->tableView_saves->selectionModel()->currentIndex();

	if (!gameDataFolderPath.isEmpty() && QDir(savesDirPath).exists() && currentIndex.isValid()) {
		QFile::remove(savesDirPath + m_model->data(m_model->index(currentIndex.row(), 0)).toString() + SavesModel::SavesExtension);
		m_model->removeRow(currentIndex.row());
	}
}

//protected:

void MainWindow::timerEvent(QTimerEvent *event)
{
	checkOriginSave();
	QObject::timerEvent(event);
}

//private slots:

void MainWindow::onCurrentRowChanged(const QModelIndex &currentRowIndex)
{
	if (currentRowIndex.isValid()) {
		ui->lineEdit_backupCurrentSave->setText(m_model->data(m_model->index(currentRowIndex.row(), 0)).toString());
	}
}

//private:

void MainWindow::checkOriginSave()
{
	int autosavesCount = ui->spinBox_autosavesCount->value();
	QString gameDataFolderPath = ui->lineEdit_gameDataFolderPath->text();
	QString savesDirPath = gameDataFolderPath + "saves/";

	if (autosavesCount > 0 && !gameDataFolderPath.isEmpty()) {
		QByteArray originSaveChecksum = fileChecksum(gameDataFolderPath + GameSaveFileName);

		if (originSaveChecksum.isEmpty()) {
			if (ui->checkBox_restoreSaveOnDelete->isChecked() && m_model->rowCount()) {
				restoreSave(m_model->data(m_model->index(0,0)).toString());
			}
		} else if (m_originSaveLastChecksum != originSaveChecksum) {
			m_originSaveLastChecksum = originSaveChecksum;
			m_lastAutosaveIndex++;

			if (m_lastAutosaveIndex >= autosavesCount) {
				m_lastAutosaveIndex = 0;
			}

			backupSave(AutosavePrefix + QString::number(m_lastAutosaveIndex + 1));
		}
	}
}

void MainWindow::scanSaves()
{
	m_model->clear();

	QString gameDataFolderPath = ui->lineEdit_gameDataFolderPath->text();
	QDir savesDir = gameDataFolderPath + "saves/";

	if (!gameDataFolderPath.isEmpty() && savesDir.exists()) {
		QFileInfoList savesFiles = savesDir.entryInfoList(QStringList() << "*.sol", QDir::Files, QDir::Time/* | QDir::Reversed*/);
		m_model->fill(savesFiles);
	}
}

void MainWindow::readSettings()
{
	QVariant value;
	QVariant invalidValue;
	int loadedApplicationVersion = applicationVersion(1, 0, 0);

	if (m_settings->contains("General/sAppVersion")) {
		value = m_settings->value("General/sAppVersion");
		if (value != invalidValue) {
			loadedApplicationVersion = applicationVersionFromString(value.toString());
		}
	}

	Q_UNUSED(loadedApplicationVersion)
	//applyBackwardCompatibilityFixes(loadedApplicationVersion);

	if (m_settings->contains("General/sLang")) {
		value = m_settings->value("General/sLang");
		if (value != invalidValue) {
			m_lang = value.toString();
		}
	}

	if (m_lang.isEmpty()) {
		if (m_translator->load(QString(":/lang/lang_") + QLocale::system().name(), ":/lang/")) {
			QApplication::installTranslator(m_translator);
			m_qtTranslator->load(QString(":/lang/qtbase_") + QLocale::system().name(), ":/lang/");
			QApplication::installTranslator(m_qtTranslator);
			m_lang = QLocale::system().name();
		} else {
			m_lang = "en_US";
		}
	} else {
		setLanguage(m_lang);
	}

	if (m_settings->contains("General/bMaximized")) {
		value = m_settings->value("General/bMaximized");
		if (value != invalidValue && value.toBool() == true) {
			this->setWindowState(Qt::WindowMaximized);
		} else if (m_settings->contains("General/qsSize")) {
			value = m_settings->value("General/qsSize");
			if (value != invalidValue) {
				this->resize(value.toSize());
			}
		}
	}

	if (m_settings->contains("General/sGameDataFolderPath")) {
		value = m_settings->value("General/sGameDataFolderPath");
		if (value != invalidValue) {
			ui->lineEdit_gameDataFolderPath->setText(value.toString());
		}
	}

	if (m_settings->contains("General/bRestoreSaveOnDelete")) {
		value = m_settings->value("General/bRestoreSaveOnDelete");
		if (value != invalidValue) {
			ui->checkBox_restoreSaveOnDelete->setChecked(value.toBool());
		}
	}

	if (m_settings->contains("General/baOriginSaveLastChecksum")) {
		value = m_settings->value("General/baOriginSaveLastChecksum");
		if (value != invalidValue) {
			m_originSaveLastChecksum = value.toByteArray();
		}
	}

	if (m_settings->contains("General/iLastAutosaveIndex")) {
		value = m_settings->value("General/iLastAutosaveIndex");
		if (value != invalidValue) {
			m_lastAutosaveIndex = value.toInt();
		}
	}

	if (m_settings->contains("General/iAutosavesCount")) {
		value = m_settings->value("General/iAutosavesCount");
		if (value != invalidValue) {
			ui->spinBox_autosavesCount->setValue(value.toInt());
		}
	}

	if (m_settings->contains("General/iOriginSaveCheckInterval")) {
		value = m_settings->value("General/iOriginSaveCheckInterval");
		if (value != invalidValue) {
			ui->spinBox_originSaveCheckInterval->setValue(value.toInt());
			setOriginSaveCheckInterval(value.toInt());
		}
	}

	if (m_settings->contains("General/iLastQuicksaveIndex")) {
		value = m_settings->value("General/iLastQuicksaveIndex");
		if (value != invalidValue) {
			m_lastQuicksaveIndex = value.toInt();
		}
	}

	if (m_settings->contains("General/iQuicksavesCount")) {
		value = m_settings->value("General/iQuicksavesCount");
		if (value != invalidValue) {
			ui->spinBox_quicksavesCount->setValue(value.toInt());
		}
	}
}

void MainWindow::saveSettings() const
{
	m_settings->clear();

	m_settings->setValue("General/sAppVersion", applicationVersionToString(CurrentApplicationVersion));
	m_settings->setValue("General/sLang", m_lang);
	m_settings->setValue("General/bMaximized", this->isMaximized());
	m_settings->setValue("General/qsSize", this->size());

	m_settings->setValue("General/sGameDataFolderPath", ui->lineEdit_gameDataFolderPath->text());
	m_settings->setValue("General/bRestoreSaveOnDelete", ui->checkBox_restoreSaveOnDelete->isChecked());
	m_settings->setValue("General/baOriginSaveLastChecksum", m_originSaveLastChecksum);

	m_settings->setValue("General/iLastAutosaveIndex", m_lastAutosaveIndex);
	m_settings->setValue("General/iAutosavesCount", ui->spinBox_autosavesCount->value());
	m_settings->setValue("General/iOriginSaveCheckInterval", ui->spinBox_originSaveCheckInterval->value());

	m_settings->setValue("General/iLastQuicksaveIndex", m_lastQuicksaveIndex);
	m_settings->setValue("General/iQuicksavesCount", ui->spinBox_quicksavesCount->value());
}
