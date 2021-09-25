#include <algorithm>

#include <QCollator>
#include <QDir>
#include <QFileInfoList>

#include "SavesModel.h"

SavesModel::SaveInfo::SaveInfo(const QFileInfo &fileInfo)
{
	name = fileInfo.completeBaseName();
	path = fileInfo.absolutePath() + QDir::separator();
	setBackuped(fileInfo.lastRead());
	setCreated(fileInfo.lastModified());
}

void SavesModel::SaveInfo::rename(const QString name)
{
	QFile::rename(path + this->name + SavesExtension, path + name + SavesExtension);
	this->name = name;
}

void SavesModel::SaveInfo::setBackuped(const QDateTime &dateTime)
{
	backupedDateTime = dateTime;
	backupedString = dateTime.toString(Qt::DateFormat::SystemLocaleShortDate);
}

void SavesModel::SaveInfo::setCreated(const QDateTime &dateTime)
{
	createdDateTime = dateTime;
	createdString = dateTime.toString(Qt::DateFormat::SystemLocaleShortDate);
}

SavesModel::SavesModel(QObject *parent) :
	QAbstractTableModel(parent)
{}

void SavesModel::clear()
{
	beginResetModel();
	m_savesInfo.clear();
	endResetModel();
}

int SavesModel::indexOf(const QString &name) const
{
	for (int i = 0; i < m_savesInfo.count(); ++i) {
		if (m_savesInfo.at(i).name == name) {
			return i;
		}
	}

	return -1;
}

void SavesModel::fill(const QFileInfoList &fileInfoList)
{
	clear();
	insertRows(0, fileInfoList.count());

	for (int i = 0; i < fileInfoList.count(); ++i) {
		m_savesInfo[i] = fileInfoList.at(i);
	}

	emit dataChanged(index(0, 0), index(rowCount(), columnCount()), QVector<int>(rowCount(), Qt::DisplayRole));
}

bool SavesModel::insertRow(int row, const QFileInfo &fileInfo)
{
	if (row > -1 && row <= rowCount()) {
		beginInsertRows(QModelIndex(), row, row);
		m_savesInfo.insert(row, SaveInfo(fileInfo));
		endInsertRows();

		return true;
	}

	return false;
}

bool SavesModel::setData(int row, const QFileInfo &fileInfo)
{
	if (row < 0 && row >= m_savesInfo.count()) {
		return false;
	}

	m_savesInfo[row] = fileInfo;
	emit dataChanged(index(row, 0), index(row, columnCount()));
	return true;
}

Qt::ItemFlags SavesModel::flags(const QModelIndex &index) const
{
	Qt::ItemFlags flags = QAbstractTableModel::flags(index);

	if (index.column() == 0) {
		flags |= Qt::ItemIsEditable;
	}

	return flags;
}

QVariant SavesModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	switch (role) {
		case Qt::DisplayRole:
		case Qt::EditRole:
			switch (index.column()) {
				case 0:
					return m_savesInfo[index.row()].name;
				break;

				case 1:
					return m_savesInfo[index.row()].backupedString;
				break;

				case 2:
					return m_savesInfo[index.row()].createdString;
				break;

				default:
				break;
			}
		break;

		default:
		break;
	}

	return QVariant();
}

bool SavesModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (!index.isValid()) {
		return false;
	}

	bool isDataChanged = true;

	switch (role) {
		case Qt::EditRole:
			switch (index.column()) {
				case 0:
					m_savesInfo[index.row()].rename(value.toString());
				break;

				case 1:
					if (value.type() == QVariant::Type::DateTime) {
						m_savesInfo[index.row()].setBackuped(value.toDateTime());
					} else {
						m_savesInfo[index.row()].backupedString = value.toString();
					}
				break;

				case 2:
					if (value.type() == QVariant::Type::DateTime) {
						m_savesInfo[index.row()].setCreated(value.toDateTime());
					} else {
						m_savesInfo[index.row()].createdString = value.toString();
					}
				break;

				default:
					isDataChanged = false;
				break;
			}
		break;

		default:
			isDataChanged = false;
		break;
	}

	if (isDataChanged) {
		emit dataChanged(index, index, QVector<int>(role));
	}

	return isDataChanged;
}

void SavesModel::sort(int column, Qt::SortOrder order)
{
	emit layoutAboutToBeChanged();

	switch (column) {
		case 0: {
			QCollator collator;
			collator.setNumericMode(true);

			std::sort(m_savesInfo.begin(), m_savesInfo.end(),
					  [order, &collator](const SaveInfo &first, const SaveInfo &second) {
				return (collator.compare(first.name, second.name) * (order == Qt::SortOrder::DescendingOrder ? 1 : -1) > 0);
			});
		}
		break;

		case 1:
			std::sort(m_savesInfo.begin(), m_savesInfo.end(),
					  [order](const SaveInfo &first, const SaveInfo &second){
				return (first.backupedDateTime > second.backupedDateTime) ^ (order == Qt::SortOrder::AscendingOrder);
			});
		break;

		case 2:
			std::sort(m_savesInfo.begin(), m_savesInfo.end(),
					  [order](const SaveInfo &first, const SaveInfo &second){
				return (first.createdDateTime > second.createdDateTime) ^ (order == Qt::SortOrder::AscendingOrder);
			});
		break;
	}

	emit layoutChanged();
}

QVariant SavesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role == Qt::DisplayRole) {
		if (orientation == Qt::Horizontal) {
			switch (section) {
				case 0:		return tr("Name");
				case 1:		return tr("Backuped");
				case 2:		return tr("Created by game");
				default:	break;
			}
		} else {
			return QString::number(section + 1);
		}
	}

	return QVariant();
}


bool SavesModel::insertRows(int row, int count, const QModelIndex &parent)
{
	if (row > -1 && row <= rowCount(parent) && count > 0) {
		beginInsertRows(parent, row, row + count - 1);
		for (int i = 0; i < count; ++i) {
			m_savesInfo.insert(row, SaveInfo(QFileInfo()));
		}
		endInsertRows();
		return true;
	}

	return false;
}

bool SavesModel::removeRows(int row, int count, const QModelIndex &parent)
{
	if (row > -1 && row <= rowCount(parent) && count > 0) {
		beginRemoveRows(parent, row, row + count - 1);
		for (int i = 0; i < count; ++i) {
			m_savesInfo.removeAt(row);
		}
		endRemoveRows();
		return true;
	}

	return false;
}

bool SavesModel::moveRows(const QModelIndex &sourceParent, int sourceRow, int count,
						  const QModelIndex &destinationParent, int destinationChild)
{
	if (count > 0 && sourceRow != destinationChild && sourceRow >= 0 && ((sourceRow + count) <= rowCount())
		&& destinationChild >= 0 && (destinationChild < (rowCount() - count))) {

		beginMoveRows(sourceParent, sourceRow, sourceRow + count - 1, destinationParent, destinationChild);
		for (int i = 0; i < count; ++i) {
			m_savesInfo.move(sourceRow + i, destinationChild + i);
		}
		endMoveRows();

		return true;
	}

	return false;
}
