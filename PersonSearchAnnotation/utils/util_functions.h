#ifndef UTIL_FUNCTIONS_H
#define UTIL_FUNCTIONS_H

#include <QString>
#include <QStringList>
#include <QPointF>

namespace psa
{

void listImageFiles(const QString& dirPath, QStringList* imageFilePaths);

qreal euclideanDist(const QPointF& a, const QPointF& b);

}

#endif // UTIL_FUNCTIONS_H

