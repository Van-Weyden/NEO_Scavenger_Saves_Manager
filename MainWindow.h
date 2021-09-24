#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QSettings;
class QTranslator;

class SavesModel;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

static constexpr int applicationVersion(const int major, const int minor = 0, const int micro = 0)
{
	return ((major << 20) | (minor << 10) | micro);
}

inline constexpr int majorApplicationVersion(int version)
{
	return (version >> 20);
}

inline constexpr int minorApplicationVersion(int version)
{
	return ((version  & 1047552) >> 10);
}

inline constexpr int microApplicationVersion(int version)
{
	return (version & 1023);
}

QString applicationVersionToString(const int version);
int applicationVersionFromString(const QString &version);

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	static constexpr const int CurrentApplicationVersion = applicationVersion(1, 0, 0);
	static constexpr const char *AutosavePrefix = "autosave_";
	static constexpr const char *QuicksavePrefix = "quicksave_";
	static constexpr const char *GameSaveFileName = "nsSGv1.sol";


	MainWindow(QWidget *parent = nullptr);
	~MainWindow();

	bool backupSave(const QString &backupSaveName);
	void restoreSave(const QString &backupSaveName);
	bool setLanguage(const QString &lang);

public slots:
	void backupQuickSave();
	void restoreLastQuickSave();
	bool backupCurrentSave();
	void restoreSelectedSave();
	void deleteSelectedSave();
	void searchGameDataFolderPath();
	void setOriginSaveCheckInterval(const int interval);

protected:
	void timerEvent(QTimerEvent *event) override;

private slots:
	void onCurrentRowChanged(const QModelIndex &currentRowIndex);

private:
	void checkOriginSave();
	void scanSaves();
	void readSettings();
	void saveSettings() const;
	Ui::MainWindow *ui;

	QSettings *m_settings = nullptr;
	QTranslator *m_qtTranslator = nullptr;
	QTranslator *m_translator = nullptr;
	QString m_lang = "";

	int m_timerId = 0;

	QByteArray m_originSaveLastChecksum;
	int m_lastAutosaveIndex = -1;
	int m_lastQuicksaveIndex = -1;

	SavesModel *m_model = nullptr;
};
#endif // MAINWINDOW_H
