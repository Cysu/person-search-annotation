#include "utils/util_functions.h"
#include <cmath>
#include <QFileInfoList>
#include <QDir>

namespace psa {

void listImageFiles(const QString& dirPath, QStringList* imageFilePaths)
{
  QStringList filter;
  filter << "*.png" << "*.bmp" << "*.jpg" << "*.jpeg";
  QFileInfoList files = QDir(dirPath).entryInfoList(filter);
  if (files.isEmpty()) return;
  imageFilePaths->clear();
  for (int i = 0; i < files.size(); ++i) {
    imageFilePaths->push_back(files.at(i).absoluteFilePath());
  }
}

qreal euclideanDist(const QPointF& a, const QPointF& b)
{
  return std::sqrt((a.x() - b.x()) * (a.x() - b.x()) +
                   (a.y() - b.y()) * (a.y() - b.y()));
}

}
