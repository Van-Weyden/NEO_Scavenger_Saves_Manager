#ifndef SAVESMODEL_H
#define SAVESMODEL_H

#include <QAbstractTableModel>
#include <QDateTime>

class QFileInfo;
typedef QList<QFileInfo> QFileInfoList;

class SavesModel : public QAbstractTableModel
{
	Q_OBJECT

public:
	static constexpr const char *SavesExtension = ".sol";

	SavesModel(QObject *parent = nullptr);
	virtual ~SavesModel() = default;

	void clear();
	bool contains(const QString &name) const; //inline
	int indexOf(const QString &name) const;
	void fill(const QFileInfoList &fileInfoList);
	bool insertRow(int row, const QFileInfo &fileInfo);
	bool setData(int row, const QFileInfo &fileInfo);

	Qt::ItemFlags flags(const QModelIndex &index) const override;

	inline int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	inline int columnCount(const QModelIndex &parent = QModelIndex()) const override;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
	void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

	bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
	bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
	bool moveRows(const QModelIndex &sourceParent, int sourceRow, int count,
				  const QModelIndex &destinationParent, int destinationChild) override;

private:
	struct SaveInfo
	{
		SaveInfo(const QFileInfo &fileInfo);
		void rename(const QString name);
		void setBackuped(const QDateTime &dateTime);
		void setCreated(const QDateTime &dateTime);

		QString name;
		QDateTime backupedDateTime;
		QDateTime createdDateTime;
		QString backupedString;
		QString createdString;
		QString path;

	};

	QList<SaveInfo> m_savesInfo;
};




inline bool SavesModel::contains(const QString &name) const
{
	return (indexOf(name) != -1);
}

inline int SavesModel::rowCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);
	return m_savesInfo.count();
}

inline int SavesModel::columnCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);
	return 3;
}

#endif // SAVESMODEL_H
